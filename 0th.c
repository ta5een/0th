#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Although I want this program to be platform-agnostic, it would be nice to
 * leverage compiler-specific intrinsics. I'm sure other compilers provide
 * something like __builtin_expect.
 */
#if defined(__GNUC__) || defined(__clang__)
    #define UNLIKELY(cond) __builtin_expect(cond, 0)
#else
    #define UNLIKELY(cond) (cond)
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
    #define WARN_UNUSED_RESULT
#endif

#define UNUSED(var) ((void)(var))
#define TODO(msg)                                                              \
    do                                                                         \
    {                                                                          \
        fprintf(stderr, "TODO: %s:%d %s\n", __FILE__, __LINE__, (msg));        \
        abort();                                                               \
    } while (0)

#define REQUIRE(cond) if (UNLIKELY(!(cond)))
#define TRY(expr)                                                              \
    do                                                                         \
    {                                                                          \
        Vm_Error err = (expr);                                                 \
        if (err != VM_ERROR_OK)                                                \
        {                                                                      \
            return err;                                                        \
        }                                                                      \
    } while (0)

/* Fundamental data types of the interpreter.
 *
 * VM_BYTE_TYPE represents the smallest addressable unit of memory (the address
 * unit). It typically corresponds to an 8-bit byte on most systems.
 *
 * CELL_TYPE represents the unit of computation. It can hold pointers and
 * integer values efficiently, typically corresponding to the native word size
 * of the system.
 *
 * VM_FCELL_TYPE represents the unit of floating-point calculations. It defaults
 * to double-precision floating-point arithmetic (IEEE 754), but may be
 * redefined as single-precision (float) if the target platform doesn't support
 * double-precision.
 */
#define VM_BYTE_TYPE  unsigned char
#define VM_BYTE_SIZE  (sizeof(VM_BYTE_TYPE))
#define VM_CELL_TYPE  uintptr_t
#define VM_CELL_SIZE  (sizeof(VM_CELL_TYPE))
#define VM_FCELL_TYPE double
#define VM_FCELL_SIZE (sizeof(VM_FCELL_TYPE))

typedef VM_BYTE_TYPE Byte;
typedef VM_CELL_TYPE Cell;
typedef VM_FCELL_TYPE FCell;

/* Memory layout and stack sizes for the interpreter.
 *
 * VM_MEMORY_SIZE defines the total size of the main memory, set to 5 KB
 * (5 * 1024 bytes) as a reasonable starting size for the system.
 *
 * The interpreter has three LIFO stacks:
 * - PSTACK (parameter stack, or data stack), for passing values between words.
 * - RSTACK (return stack), for storing return addresses for control flow.
 * - FSTACK (floating-point stack), for floating-point operations.
 *
 * PSTACK and RSTACK each have a capacity of 64 cells, following Brad
 * Rodriguez's recommendation in "MOVING FORTH Part 1: Design Decisions in the
 * Forth Kernel". This should be more than sufficient as ANS Forth specifies a
 * minimum of 32 cells for the PSTACK and 24 cells for the RSTACK.
 *
 * FSTACK has a capacity of 6 floating-point cells, which is the minimum
 * required by ANS Forth.
 */
#define VM_MEMORY_SIZE  (5UL * 1024UL * VM_BYTE_SIZE)
#define VM_PSTACK_COUNT (64U)
#define VM_RSTACK_COUNT (64U)
#define VM_FSTACK_COUNT (6U)
#define VM_PSTACK_SIZE  (VM_PSTACK_COUNT * VM_CELL_SIZE)
#define VM_RSTACK_SIZE  (VM_RSTACK_COUNT * VM_CELL_SIZE)
#define VM_FSTACK_SIZE  (VM_FSTACK_COUNT * VM_FCELL_SIZE)

#define VM_INPUT_BUFFER_NUM_BYTES (80U)

/* Length and flags byte for dictionary word names
 *
 * Each word's name is prefixed by a single byte that packs both the name
 * length and flags. This is illustrated in the bit diagram below:
 *
 *   7 6 5 4 3 2 1 0
 *   │ │ │ └─┴─┴─┴─┴─ word name length (5 bits, 0-31 chars)
 *   │ │ └─────────── hidden flag
 *   │ └───────────── (unused, reserved for future)
 *   └─────────────── immediate flag
 *
 * VM_DICT_NAME_LENGTH_MASK extracts just the length bits when reading
 * dictionary entries during word lookup.
 */
#define VM_DICT_NAME_LENGTH_MASK (0x1f)
#define VM_DICT_FLAG_NONE        (0x0)
#define VM_DICT_FLAG_HIDDEN      (0x20)
#define VM_DICT_FLAG_IMMEDIATE   (0x80)

/* A list of all possible error codes.
 *
 * New errors may be added, but VM_ERROR_UNKNOWN must always be the last entry.
 */
#define VM_ERROR_LIST                                                          \
    VM_ERROR_ITEM(OK, 0)                                                       \
    VM_ERROR_ITEM(ABORT, -1)                                                   \
    VM_ERROR_ITEM(PSTACK_OVERFLOW, -2)                                         \
    VM_ERROR_ITEM(PSTACK_UNDERFLOW, -3)                                        \
    VM_ERROR_ITEM(RSTACK_OVERFLOW, -4)                                         \
    VM_ERROR_ITEM(RSTACK_UNDERFLOW, -5)                                        \
    VM_ERROR_ITEM(FSTACK_OVERFLOW, -6)                                         \
    VM_ERROR_ITEM(FSTACK_UNDERFLOW, -7)                                        \
    VM_ERROR_ITEM(INVALID_MEMORY_ADDRESS, -8)                                  \
    VM_ERROR_ITEM(OUT_OF_MEMORY, -9)                                           \
    VM_ERROR_ITEM(UNDEFINED_WORD, -10)                                         \
    VM_ERROR_ITEM(UNKNOWN, -11) // Must be last

/* A fixed set of return values for all interpreter operations, indicating
 * success or failure.
 *
 * VM_ERROR_OK indicates success, while all other values represent specific
 * failure conditions.
 */
typedef enum
{
#define VM_ERROR_ITEM(name, value) VM_ERROR_##name = (value),
    VM_ERROR_LIST
#undef VM_ERROR_ITEM
} Vm_Error;

/* Controls how the interpreter processes words. */
typedef enum
{
    /* Executes words immediately as encountered. */
    VM_MODE_INTERPRET = 0,
    /* Compiles words into the current definition in the dictionary. */
    VM_MODE_COMPILE = 1,
} Vm_Mode;

typedef struct
{
    /* Current execution mode. */
    Vm_Mode mode;
    /* Address to the most recently-defined word in the dictionary. */
    Cell latest;
    /* Next free cell in memory. Compiled words will go here. */
    Cell here;
    /* Current position in the instruction stream. */
    Cell ip;

    /* Index of the current position in the parameter stack. */
    Cell psp;
    /* Base of the parameter stack. */
    Cell pstack[VM_PSTACK_COUNT];

    /* Index of the current position in the return stack. */
    Cell rsp;
    /* Base of the return stack. */
    Cell rstack[VM_RSTACK_COUNT];

    /* Index of the current position in the floating point stack. */
    Cell fsp;
    /* Base of the floating point stack. */
    FCell fstack[VM_FSTACK_COUNT];

    Byte input_buffer[VM_INPUT_BUFFER_NUM_BYTES];
    size_t input_buffer_len;
    size_t input_buffer_pos;

    /* The program's main memory.
     *
     * Stores dictionary entries for all words (builtin and user-defined),
     * compiled code for user-defined words, and variables.
     */
    Byte memory[VM_MEMORY_SIZE];
} Vm;

WARN_UNUSED_RESULT
static Vm_Error
vm_write_byte(Vm *vm, Cell address, Byte data)
{
    REQUIRE(address + VM_BYTE_SIZE <= VM_MEMORY_SIZE)
    {
        return VM_ERROR_OUT_OF_MEMORY;
    }

    vm->memory[address] = data;
    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_write_cell(Vm *vm, Cell address, Cell data)
{
    REQUIRE(address + VM_CELL_SIZE <= VM_MEMORY_SIZE)
    {
        return VM_ERROR_OUT_OF_MEMORY;
    }

    memcpy(vm->memory + address, &data, VM_CELL_SIZE);
    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_write_data(Vm *vm, Cell address, size_t len, const void *data)
{
    REQUIRE(address + len <= VM_MEMORY_SIZE)
    {
        return VM_ERROR_INVALID_MEMORY_ADDRESS;
    }

    memcpy(vm->memory + address, data, len);
    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_store_byte(Vm *vm, Byte data)
{
    TRY(vm_write_byte(vm, vm->here, data));
    vm->here += VM_BYTE_SIZE;
    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_align_here(Vm *vm)
{
    while (vm->here & (VM_CELL_SIZE - 1))
    {
        TRY(vm_store_byte(vm, 0x0));
    }
    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_store_cell(Vm *vm, Cell data)
{
    // Align vm->here to cell boundary if needed
    TRY(vm_align_here(vm));
    TRY(vm_write_cell(vm, vm->here, data));
    vm->here += VM_CELL_SIZE;
    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_store_data(Vm *vm, size_t len, const void *data)
{
    TRY(vm_write_data(vm, vm->here, len, data));
    vm->here += len;
    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_pstack_push(Vm *vm, Cell data)
{
    REQUIRE(vm->psp < VM_PSTACK_SIZE)
    {
        return VM_ERROR_PSTACK_OVERFLOW;
    }

    vm->pstack[vm->psp] = data;
    vm->psp += 1;
    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_pstack_pop(Vm *vm, Cell *data)
{
    REQUIRE(vm->psp > 0)
    {
        return VM_ERROR_PSTACK_UNDERFLOW;
    }

    vm->psp -= 1;
    *data = vm->pstack[vm->psp];
    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_pstack_peek(Vm *vm, Cell *data)
{
    REQUIRE(vm->psp > 0)
    {
        return VM_ERROR_PSTACK_UNDERFLOW;
    }

    *data = vm->pstack[vm->psp - 1];
    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_rstack_push(Vm *vm, Cell address)
{
    REQUIRE(vm->rsp < VM_RSTACK_SIZE)
    {
        return VM_ERROR_RSTACK_OVERFLOW;
    }

    vm->rstack[vm->rsp] = address;
    vm->rsp += 1;
    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_rstack_pop(Vm *vm, Cell *address)
{
    REQUIRE(vm->rsp > 0)
    {
        return VM_ERROR_RSTACK_UNDERFLOW;
    }

    // The return address previously at vm->rsp can now be overwritten.
    vm->rsp -= 1;
    // The out parameter is given a copy of the now-discarded return address.
    *address = vm->rstack[vm->rsp];
    return VM_ERROR_OK;
}

typedef struct
{
    enum
    {
        VM_NONE,
        VM_SOME,
    } is;
    Cell data;
} Vm_Maybe_Cell;

#define SOME(maybe) ((maybe).is == VM_SOME)
#define NONE(maybe) ((maybe).is == VM_NONE)

WARN_UNUSED_RESULT static Vm_Error
vm_find_word(Vm *vm, size_t name_len, const char name[name_len],
             Vm_Maybe_Cell *result)
{
    UNUSED(vm);
    UNUSED(name_len);
    UNUSED(name);
    UNUSED(result);
    TODO("vm_find_word");
    return VM_ERROR_OK;
}

#define VM_BUILTIN_LIST                                                        \
    VM_BUILTIN_ITEM(drop, "drop", VM_DICT_FLAG_NONE)                           \
    VM_BUILTIN_ITEM(swap, "swap", VM_DICT_FLAG_NONE)                           \
    VM_BUILTIN_ITEM(dup, "dup", VM_DICT_FLAG_NONE)

typedef enum
{
#define VM_BUILTIN_ITEM(c_name, _name, _flags) VM_BUILTIN_ID_##c_name,
    VM_BUILTIN_LIST
#undef VM_BUILTIN_ITEM
} Vm_Builtin_Id;

/* Helper macro for defining builtin handler functions.
 *
 * Expands to a function signature for a builtin handler with the given name.
 * All builtin handlers have the signature: Vm_Error vm_builtin_NAME(Vm *vm)
 */
#define VM_BUILTIN_DEFINE(word)                                                \
    WARN_UNUSED_RESULT static Vm_Error vm_builtin_##word

VM_BUILTIN_DEFINE(drop)(Vm *vm)
{
    Cell ignored_dropped_value = 0;
    TRY(vm_pstack_pop(vm, &ignored_dropped_value));
    return VM_ERROR_OK;
}

VM_BUILTIN_DEFINE(swap)(Vm *vm)
{
    Cell x = 0;
    Cell y = 0;
    TRY(vm_pstack_pop(vm, &x));
    TRY(vm_pstack_pop(vm, &y));
    TRY(vm_pstack_push(vm, x));
    TRY(vm_pstack_push(vm, y));
    return VM_ERROR_OK;
}

VM_BUILTIN_DEFINE(dup)(Vm *vm)
{
    Cell top = 0;
    TRY(vm_pstack_peek(vm, &top));
    TRY(vm_pstack_push(vm, top));
    return VM_ERROR_OK;
}

/* Function pointer type for builtin primitive handlers.
 *
 * Each builtin takes a pointer to the VM and returns an error code.
 */
typedef Vm_Error (*Vm_Builtin_Handler)(Vm *vm);

/* Dispatch table mapping builtin indices to handler functions.
 *
 * When the VM encounters a codeword in the range [0, VM_BUILTIN_HANDLER_COUNT),
 * it treats it as a builtin primitive and calls the handler at that index in
 * this array. The order of handlers matches the order in VM_BUILTIN_LIST.
 */
static const Vm_Builtin_Handler vm_builtin_handlers[] = {
#define VM_BUILTIN_ITEM(c_name, _name, _flags) vm_builtin_##c_name,
    VM_BUILTIN_LIST
#undef VM_BUILTIN_ITEM
};

#define VM_BUILTIN_HANDLER_COUNT                                               \
    (sizeof(vm_builtin_handlers) / sizeof(Vm_Builtin_Handler))

/* Add a builtin word to the dictionary.
 *
 * Creates a dictionary entry with the following structure:
 *
 *   <───────────── dictionary entry header ───────────────>
 *   ┌──────────────────┬──────────┬────────────────╴╴╴╴╴╴╴┬──────────────────┐
 *   │ link index       │ length + │ name + padding        │ codeword         │
 *   │                  │ flags    │                       │                  │
 *   └─(1 cell)─────────┴─(1 byte)─┴─(n bytes)──────╴╴╴╴╴╴╴┴─(1 cell)─────────┘
 *
 * - link:          index of previous word's header (forms linked list)
 * - length+flags:  packed byte containing name length and flags
 * - name:          variable-length name string (not null-terminated)
 * - padding:       zero-padding to align codeword to cell boundary
 * - codeword:      builtin index used to dispatch to vm_builtin_handlers[index]
 */
WARN_UNUSED_RESULT
static Vm_Error
vm_add_builtin(Vm *vm, size_t index, const char *name, Byte name_len,
               Byte flags)
{
    assert(index < VM_BUILTIN_HANDLER_COUNT && "index out of bounds");
    assert(name != NULL && "name is NULL");
    assert(name_len > 0 && "name is empty");
    assert(name_len <= VM_DICT_NAME_LENGTH_MASK && "name too long");
    assert((flags & VM_DICT_NAME_LENGTH_MASK) == 0 && "flags overlap length");

    Cell new_latest = vm->here;

    TRY(vm_store_cell(vm, vm->latest));
    TRY(vm_store_byte(vm, name_len | flags));
    TRY(vm_store_data(vm, name_len, name));

    // Store builtin index as codeword. The VM treats codewords in the range
    // [0, VM_BUILTIN_HANDLER_COUNT) as builtin primitives and dispatches them
    // via the vm_builtin_handlers[] function-pointer array.
    TRY(vm_store_cell(vm, index));

    vm->latest = new_latest;

    return VM_ERROR_OK;
}

/* Number of bytes in a string literal, excluding the final NULL terminator.
 * Evaluated at compile time.
 *
 * NOTE: Only use with string literals, not char pointers.
 */
#define VM_STR_LIT_LENGTH(str) (Byte)(sizeof(str) - 1)

WARN_UNUSED_RESULT
static Vm_Error
vm_register_builtins(Vm *vm)
{
#define VM_BUILTIN_ITEM(c_name, name, flags)                                   \
    TRY(vm_add_builtin(vm, VM_BUILTIN_ID_##c_name, name,                       \
                       VM_STR_LIT_LENGTH(name), flags));
    VM_BUILTIN_LIST
#undef VM_BUILTIN_ITEM

    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_parse_integer_literal(size_t str_len, const char str[str_len],
                         Vm_Maybe_Cell *result)
{
    UNUSED(str_len);
    UNUSED(str);
    UNUSED(result);
    TODO("vm_parse_digit_string");
    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static bool
vm_is_whitespace(Byte byte)
{
    switch ((char)byte)
    {
    case ' ':  /* fallthrough */
    case '\t': /* fallthrough */
    case '\n': /* fallthrough */
    case '\r':
        return true;
    default:
        return false;
    }
}

typedef struct
{
    /* Inclusive start index. */
    size_t start;
    /* Exclusive end index. */
    size_t end;
} Vm_Buffer_Range;

WARN_UNUSED_RESULT
static Vm_Error
vm_next_word(Vm *vm, Vm_Buffer_Range *out_word_range)
{
    // Skip leading whitespaces
    while (vm->input_buffer_pos < vm->input_buffer_len &&
           vm_is_whitespace(vm->input_buffer[vm->input_buffer_pos]))
    {
        vm->input_buffer_pos += 1;
    }

    out_word_range->start = vm->input_buffer_pos;

    // Extract word
    while (vm->input_buffer_pos < vm->input_buffer_len &&
           !vm_is_whitespace(vm->input_buffer[vm->input_buffer_pos]))
    {
        vm->input_buffer_pos += 1;
    }

    out_word_range->end = vm->input_buffer_pos;

    return VM_ERROR_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_interpret(Vm *vm)
{
    vm->mode = VM_MODE_INTERPRET;

    // TODO: Read input from source (e.g. from file, stdin)
    const char source[] = "";
    size_t source_len = VM_STR_LIT_LENGTH(source);

    // FIXME: This just truncates the input
    if (source_len >= VM_INPUT_BUFFER_NUM_BYTES)
    {
        source_len = VM_INPUT_BUFFER_NUM_BYTES;
    }

    memcpy(vm->input_buffer, source, source_len);
    vm->input_buffer_len = source_len;

    while (vm->input_buffer_pos < vm->input_buffer_len)
    {
        Vm_Buffer_Range word_range = {0};
        TRY(vm_next_word(vm, &word_range));
        const size_t word_len = word_range.end - word_range.start;

        // No more words to process
        if (word_len == 0)
        {
            break;
        }

        Vm_Maybe_Cell xt = {0};
        TRY(vm_find_word(vm, word_len,
                         (const char *)vm->input_buffer + word_range.start,
                         &xt));
        if (SOME(xt))
        {
            if (xt.data < VM_BUILTIN_HANDLER_COUNT)
            {
                Vm_Error err = vm_builtin_handlers[xt.data](vm);
                if (err != VM_ERROR_OK)
                {
                    // TODO: Give user more context about this error
                    return err;
                }
                continue;
            }

            TODO("execute colon definition");
        }

        Vm_Maybe_Cell parsed_integer = {0};
        TRY(vm_parse_integer_literal(
            word_len, (const char *)vm->input_buffer + word_range.start,
            &parsed_integer));
        if (SOME(parsed_integer))
        {
            TRY(vm_pstack_push(vm, parsed_integer.data));
            continue;
        }

        // TODO: Handle other literals (e.g. floats, strings)

        // TODO: Give user more context about this error
        // NOTE: When an error occurs, should we go back to the prompt?
        return VM_ERROR_UNDEFINED_WORD;
    }

    return VM_ERROR_OK;
}

/* Human-readable representations of Vm_Error enum values. */
static const char *vm_error_names[] = {
#define VM_ERROR_ITEM(name, _) "VM_ERROR_" #name,
    VM_ERROR_LIST
#undef VM_ERROR_ITEM
};

/* Return the string representation of the given error code.
 *
 * If the provided error code is invalid, returns the string representation
 * of VM_ERROR_UNKNOWN instead.
 */
static const char *
vm_error_as_string(Vm_Error error)
{
    REQUIRE(VM_ERROR_OK >= error && error >= VM_ERROR_UNKNOWN)
    {
        return vm_error_names[abs(VM_ERROR_UNKNOWN)];
    }

    return vm_error_names[abs(error)];
}

static void
vm_report_error(Vm_Error error, const char *context_msg)
{
    fprintf(stderr, "err (%s)", vm_error_as_string(error));
    if (context_msg != NULL)
    {
        fprintf(stderr, " %s", context_msg);
    }
    fprintf(stderr, "\n");
}

static void
vm_print_stack(Vm *vm)
{
    if (vm->psp > 0)
    {
        printf("<%lu>", vm->psp);
        for (Cell i = 0; i < vm->psp; i++)
        {
            printf(" 0x%0*lX", (int)VM_CELL_SIZE, vm->pstack[i]);
        }
    }
    else
    {
        printf("<0> (empty)");
    }
}

int
main(void)
{
    Vm vm = {0};
    Vm_Error err = VM_ERROR_OK;

    // Reserve first cell with a sentinel value so dictionary entries never
    // start at address 0. This avoids the ambiguity where the address 0 could
    // mean an empty value (i.e. NULL) or the first entry in the dictionary.
    err = vm_store_cell(&vm, (Cell)-1);
    if (err != VM_ERROR_OK)
    {
        vm_report_error(err, "failed to reserve sentinel cell");
        return EXIT_FAILURE;
    }

    err = vm_register_builtins(&vm);
    if (err != VM_ERROR_OK)
    {
        vm_report_error(err, "failed to register builtins");
        return EXIT_FAILURE;
    }

    err = vm_interpret(&vm);
    if (err == VM_ERROR_OK)
    {
        vm_print_stack(&vm);
        puts(" ok");
    }
    else
    {
        // TODO: Provide better error diagnostics
        vm_print_stack(&vm);
        puts(" err");
        fprintf(stderr, "%s\n", vm_error_as_string(err));
        return EXIT_FAILURE;
    }

    // Ignore -Wunneeded-internal-declaration errors for now
    UNUSED(vm_rstack_pop);
    UNUSED(vm_rstack_push);

    return EXIT_SUCCESS;
}
