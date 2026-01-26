#include <assert.h>
#include <limits.h>
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
        if (err != VM_OK)                                                      \
        {                                                                      \
            return err;                                                        \
        }                                                                      \
    } while (0)

/* The smallest addressable unit of memory (the address unit). It typically
 * corresponds to an 8-bit byte on most systems.
 */
typedef unsigned char Byte;

/* The unit of computation. It can hold pointers and integer values efficiently,
 * typically corresponding to the native word size of the system.
 */
typedef uintptr_t Cell;

#if false
/* The unit of floating-point calculations. It defaults to double-precision
 * floating-point arithmetic (IEEE 754), but may be redefined as
 * single-precision (float) if the target platform doesn't support
 * double-precision.
 */
typedef double FCell;
#endif

/* Canonical boolean values. */
#define VM_TRUE  ((Cell)(-1))
#define VM_FALSE ((Cell)0)

/* Check if an address is cell-aligned. */
#define VM_CELL_ALIGNED(addr) (((Cell)(addr) & (sizeof(Cell) - 1)) == 0)

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
#define VM_MEMORY_SIZE  (5UL * 1024UL * sizeof(Byte))
#define VM_PSTACK_COUNT (64U)
#define VM_RSTACK_COUNT (64U)
#if false
    #define VM_FSTACK_COUNT (6U)
#endif

/* A list of all possible error codes.
 *
 * New errors may be added, but VM_ERROR_UNKNOWN must always be the last entry.
 */
#define VM_FOR_EACH_ERROR(DO)                                                  \
    DO(OK, 0)                                                                  \
    DO(BYE, -1)                                                                \
    DO(ERROR_ABORT, -2)                                                        \
    DO(ERROR_PSTACK_OVERFLOW, -3)                                              \
    DO(ERROR_PSTACK_UNDERFLOW, -4)                                             \
    DO(ERROR_RSTACK_OVERFLOW, -5)                                              \
    DO(ERROR_RSTACK_UNDERFLOW, -6)                                             \
    DO(ERROR_FSTACK_OVERFLOW, -7)                                              \
    DO(ERROR_FSTACK_UNDERFLOW, -8)                                             \
    DO(ERROR_INVALID_MEMORY_ADDRESS, -9)                                       \
    DO(ERROR_OUT_OF_MEMORY, -10)                                               \
    DO(ERROR_UNDEFINED_WORD, -11)                                              \
    DO(ERROR_RESULT_OUT_OF_RANGE, -12)                                         \
    DO(ERROR_WORD_TOO_LONG, -13)                                               \
    DO(ERROR_UNKNOWN, -14) // Must be last

/* Return values for interpreter operations.
 *
 * VM_OK means success, VM_BYE means clean exit, VM_ERROR_* means failure.
 */
typedef enum
{
#define VM_EMIT_ERROR_ENUM(name, value) VM_##name = (value),
    VM_FOR_EACH_ERROR(VM_EMIT_ERROR_ENUM)
#undef VM_EMIT_ERROR_ENUM
} Vm_Error;

/* Function pointer type for builtin primitive handlers. */
typedef Vm_Error (*Vm_Builtin_Handler)(void);

/* The program's main memory.
 *
 * Stores dictionary entries for all words (builtin and user-defined),
 * compiled code for user-defined words, and variables.
 */
static union {
    Byte bytes[VM_MEMORY_SIZE];
    Cell cells[VM_MEMORY_SIZE / sizeof(Cell)];
} memory;

/* Next free cell in memory. Compiled words will go here. */
static Byte *here = memory.bytes;
/* Cell-aligned address to the most recently-defined word in the dictionary. */
static Byte *latest = NULL;

/* Instruction pointer: points to the code field being executed. */
static Vm_Builtin_Handler **iptr = NULL;
/* Next pointer: where execution continues in the compiled code stream. */
static Vm_Builtin_Handler **nptr = NULL;

/* Base of the parameter stack. */
static Cell pstack[VM_PSTACK_COUNT];
/* Current position in the parameter stack. */
static Cell *psp = pstack;

/* Base of the return stack. */
static Cell rstack[VM_RSTACK_COUNT];
/* Current position in the return stack. */
static Cell *rsp = rstack;

#if false
/* Base of the floating point stack. */
static FCell fstack[VM_FSTACK_COUNT];
/* Current position in the floating point stack. */
static FCell *fsp = fstack;
#endif

WARN_UNUSED_RESULT
static Vm_Error
vm_store_byte(Byte data)
{
    *here = data;
    here += 1;
    return VM_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_align_here(void)
{
    while (!VM_CELL_ALIGNED(here))
    {
        TRY(vm_store_byte(0x0));
    }
    return VM_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_store_cell(Cell data)
{
    TRY(vm_align_here());
    memcpy(here, &data, sizeof(Cell));
    here += sizeof(Cell);
    return VM_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_pstack_push(Cell data)
{
    REQUIRE(psp < &pstack[VM_PSTACK_COUNT - 1])
    {
        return VM_ERROR_PSTACK_OVERFLOW;
    }

    *psp = data;
    psp += 1;
    return VM_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_pstack_pop(Cell *data)
{
    REQUIRE(psp > &pstack[0])
    {
        return VM_ERROR_PSTACK_UNDERFLOW;
    }

    psp -= 1;
    *data = *psp;
    return VM_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_rstack_push(Cell address)
{
    REQUIRE(rsp < &rstack[VM_RSTACK_COUNT - 1])
    {
        return VM_ERROR_RSTACK_OVERFLOW;
    }

    *rsp = address;
    rsp += 1;
    return VM_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_rstack_pop(Cell *address)
{
    REQUIRE(rsp > &rstack[0])
    {
        return VM_ERROR_RSTACK_UNDERFLOW;
    }

    rsp -= 1;
    *address = *rsp;
    return VM_OK;
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

WARN_UNUSED_RESULT
static Vm_Error
vm_find_builtin(char name_to_find, Vm_Maybe_Cell *result)
{
    Byte *entry = latest;
    while (entry != NULL)
    {
        // Name is at offset 1 cell + 1 byte (after link and the len+flags byte)
        char name = (char)entry[sizeof(Cell) + 1];

        if (name == name_to_find)
        {
            result->is = VM_SOME;
            result->data = (Cell)&entry[sizeof(Cell) * 2];
            return VM_OK;
        }

        // Follow link to previous entry
        Cell link;
        memcpy(&link, entry, sizeof(Cell));
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        entry = (Byte *)link;
    }

    // Not found
    result->is = VM_NONE;
    result->data = VM_FALSE;
    return VM_OK;
}

#define VM_FOR_EACH_BUILTIN(DO)                                                \
    DO('{', begin_definition)                                                  \
    DO('}', end_definition)                                                    \
    DO('C', push_cell_size)                                                    \
    DO('H', push_here_address)                                                 \
    DO('L', push_latest_address)                                               \
    DO('@', fetch_cell)                                                        \
    DO('!', store_cell)                                                        \
    DO('$', fetch_byte)                                                        \
    DO('.', store_byte)                                                        \
    DO('f', find_builtin)                                                      \
    DO('j', jump_conditional)                                                  \
    DO('J', jump_unconditional)                                                \
    DO('k', get_next_char)                                                     \
    DO('l', push_literal)                                                      \
    DO('x', execute)                                                           \
    DO('+', plus)                                                              \
    DO('-', minus)                                                             \
    DO('=', equal)

#define VM_EMIT_BUILTIN_PROTO(_name, fn_name)                                  \
    static Vm_Error vm_builtin_##fn_name(void);
VM_FOR_EACH_BUILTIN(VM_EMIT_BUILTIN_PROTO)
#undef VM_EMIT_BUILTIN_PROTO

/* Dispatch table of builtin handler functions.
 *
 * Code fields store pointers INTO this array (e.g., &vm_builtin_handlers[0]),
 * not function pointers directly. This avoids undefined behavior (Cell can't
 * portably hold function pointers) and enables uniform double-dereference for
 * both builtins and user-defined words.
 */
static const Vm_Builtin_Handler vm_builtin_handlers[] = {
#define VM_EMIT_BUILTIN_FN(_name, fn_name) vm_builtin_##fn_name,
    VM_FOR_EACH_BUILTIN(VM_EMIT_BUILTIN_FN)
#undef VM_EMIT_BUILTIN_FN
};

#define VM_BUILTIN_HANDLER_COUNT                                               \
    (sizeof(vm_builtin_handlers) / sizeof(Vm_Builtin_Handler))

/* Helper macro for defining builtin handler functions. */
#define VM_BUILTIN_DEFINE(word)                                                \
    WARN_UNUSED_RESULT static Vm_Error vm_builtin_##word

VM_BUILTIN_DEFINE(begin_definition)(void)
{
    TRY(vm_rstack_push((Cell)nptr));
    nptr = iptr + 1;
    return VM_OK;
}

VM_BUILTIN_DEFINE(end_definition)(void)
{
    Cell return_addr;
    TRY(vm_rstack_pop(&return_addr));
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    nptr = (Vm_Builtin_Handler **)return_addr;
    return VM_OK;
}

VM_BUILTIN_DEFINE(push_cell_size)(void)
{
    TRY(vm_pstack_push(sizeof(Cell)));
    return VM_OK;
}

VM_BUILTIN_DEFINE(push_here_address)(void)
{
    TRY(vm_pstack_push((Cell)&here));
    return VM_OK;
}

VM_BUILTIN_DEFINE(push_latest_address)(void)
{
    TRY(vm_pstack_push((Cell)&latest));
    return VM_OK;
}

VM_BUILTIN_DEFINE(fetch_cell)(void)
{
    Cell addr = 0;
    TRY(vm_pstack_pop(&addr));
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    Cell data = *(Cell *)addr;
    TRY(vm_pstack_push(data));
    return VM_OK;
}

VM_BUILTIN_DEFINE(store_cell)(void)
{
    Cell data = 0;
    Cell addr = 0;
    TRY(vm_pstack_pop(&addr));
    TRY(vm_pstack_pop(&data));
    assert(VM_CELL_ALIGNED(addr) && "addr not cell-aligned");
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    *((Cell *)addr) = data;
    return VM_OK;
}

VM_BUILTIN_DEFINE(fetch_byte)(void)
{
    Cell addr = 0;
    TRY(vm_pstack_pop(&addr));
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    Byte data = *(Byte *)addr;
    TRY(vm_pstack_push(data));
    return VM_OK;
}

VM_BUILTIN_DEFINE(store_byte)(void)
{
    Cell data = 0;
    Cell addr = 0;
    TRY(vm_pstack_pop(&addr));
    TRY(vm_pstack_pop(&data));
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    *((Byte *)addr) = (Byte)data;
    return VM_OK;
}

VM_BUILTIN_DEFINE(find_builtin)(void)
{
    Cell name_to_find = 0;
    TRY(vm_pstack_pop(&name_to_find));

    Vm_Maybe_Cell found_xt = {0};
    TRY(vm_find_builtin((char)name_to_find, &found_xt));
    if (SOME(found_xt))
    {
        TRY(vm_pstack_push(found_xt.data));
        return VM_OK;
    }

    TRY(vm_pstack_push(VM_FALSE));
    return VM_OK;
}

VM_BUILTIN_DEFINE(jump_conditional)(void)
{
    Cell test = 0;
    TRY(vm_pstack_pop(&test));
    if (test == VM_FALSE)
    {
        Cell addr = 0;
        memcpy(&addr, (const void *)nptr, sizeof(Cell));
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        nptr = (Vm_Builtin_Handler **)addr;
    }
    else
    {
        nptr += 1;
    }
    return VM_OK;
}

VM_BUILTIN_DEFINE(jump_unconditional)(void)
{
    Cell addr = 0;
    memcpy(&addr, (const void *)nptr, sizeof(Cell));
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    nptr = (Vm_Builtin_Handler **)addr;
    return VM_OK;
}

VM_BUILTIN_DEFINE(get_next_char)(void)
{
    int chr = getchar();
    if (feof(stdin))
    {
        return VM_BYE;
    }
    if (ferror(stdin))
    {
        perror("Failed to read from stdin");
        return VM_ERROR_ABORT;
    }
    TRY(vm_pstack_push((Cell)chr));
    return VM_OK;
}

VM_BUILTIN_DEFINE(push_literal)(void)
{
    Cell value = (Cell)*nptr;
    TRY(vm_pstack_push(value));
    nptr += 1;
    return VM_OK;
}

VM_BUILTIN_DEFINE(execute)(void)
{
    Cell xt = 0;
    TRY(vm_pstack_pop(&xt));

    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    Cell *validated_xt = (Cell *)xt;
    REQUIRE(VM_CELL_ALIGNED(validated_xt) && &memory.cells[0] <= validated_xt &&
            validated_xt < &memory.cells[VM_MEMORY_SIZE / sizeof(Cell)])
    {
        return VM_ERROR_INVALID_MEMORY_ADDRESS;
    }

    iptr = (Vm_Builtin_Handler **)validated_xt;
    return (**iptr)();
}

VM_BUILTIN_DEFINE(plus)(void)
{
    Cell x1 = 0;
    Cell x2 = 0;
    TRY(vm_pstack_pop(&x2));
    TRY(vm_pstack_pop(&x1));
    TRY(vm_pstack_push(x1 + x2));
    return VM_OK;
}

VM_BUILTIN_DEFINE(minus)(void)
{
    Cell x1 = 0;
    Cell x2 = 0;
    TRY(vm_pstack_pop(&x2));
    TRY(vm_pstack_pop(&x1));
    TRY(vm_pstack_push(x1 - x2));
    return VM_OK;
}

VM_BUILTIN_DEFINE(equal)(void)
{
    Cell x1 = 0;
    Cell x2 = 0;
    TRY(vm_pstack_pop(&x2));
    TRY(vm_pstack_pop(&x1));
    TRY(vm_pstack_push(x1 == x2 ? VM_TRUE : VM_FALSE));
    return VM_OK;
}

/* Add a builtin word to the dictionary.
 *
 * Creates a dictionary entry with the following structure:
 *
 *   <───────────── dictionary entry header ───────────────>
 *   ┌──────────────────┬──────────┬───────────────────────┬──────────────────┐
 *   │ link             │ length   │ name + padding        │ code field       │
 *   │                  │          │                       │                  │
 *   └─(1 cell)─────────┴─(1 byte)─┴─(1 byte + padding)────┴─(1 cell)─────────┘
 *
 * - link:         address of previous word's header (forms linked list)
 * - length:       name length byte (1 for builtins, flag bits unused)
 * - name:         single character name (not null-terminated)
 * - padding:      zero-padding to align code field to cell boundary
 * - code field:   pointer into vm_builtin_handlers (see dispatch table comment)
 */
WARN_UNUSED_RESULT
static Vm_Error
vm_add_builtin(char name, const Vm_Builtin_Handler *handler)
{
    Byte *new_latest = here;
    TRY(vm_store_cell((Cell)latest));
    TRY(vm_store_byte(1));
    TRY(vm_store_byte((Byte)name));
    TRY(vm_store_cell((Cell)handler));
    latest = new_latest;

    return VM_OK;
}

/* Number of bytes in a string literal, excluding the final NULL terminator.
 * Evaluated at compile time.
 *
 * NOTE: Only use with string literals, not char pointers.
 */
#define VM_STR_LIT_LENGTH(str) (Byte)(sizeof(str) - 1)

WARN_UNUSED_RESULT
static Vm_Error
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
vm_register_builtins(void)
{
    const Vm_Builtin_Handler *ptr = vm_builtin_handlers;
#define VM_REGISTER_BUILTIN(name, _fn_name) TRY(vm_add_builtin(name, ptr++));
    VM_FOR_EACH_BUILTIN(VM_REGISTER_BUILTIN)
#undef VM_REGISTER_BUILTIN

    return VM_OK;
}

WARN_UNUSED_RESULT
static Vm_Error
vm_start(void)
{
    TRY(vm_align_here());
    // NOLINTNEXTLINE(bugprone-casting-through-void)
    Vm_Builtin_Handler **start = (Vm_Builtin_Handler **)(void *)here;

    const char main_loop[] = {'k', 'f', 'x', 'J'};
    size_t main_loop_len = sizeof(main_loop);
    for (size_t i = 0; i < main_loop_len; ++i)
    {
        Vm_Maybe_Cell found_xt_addr = {0};
        TRY(vm_find_builtin(main_loop[i], &found_xt_addr));
        if (NONE(found_xt_addr))
        {
            return VM_ERROR_ABORT;
        }

        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        TRY(vm_store_cell((Cell)(*(Vm_Builtin_Handler **)found_xt_addr.data)));
    }

    TRY(vm_store_cell((Cell)start));
    nptr = start;

    Vm_Error err = VM_OK;
    do
    {
        iptr = nptr;
        nptr += 1;
        err = (**iptr)();
    } while (err == VM_OK);

    return err;
}

/* Human-readable representations of Vm_Error enum values. */
static const char *vm_error_names[] = {
#define VM_EMIT_ERROR_STR(name, _) "VM_ERROR_" #name,
    VM_FOR_EACH_ERROR(VM_EMIT_ERROR_STR)
#undef VM_EMIT_ERROR_STR
};

/* Return the string representation of the given error code.
 *
 * If the provided error code is invalid, returns the string representation
 * of VM_ERROR_UNKNOWN instead.
 */
static const char *
vm_error_as_string(Vm_Error error)
{
    REQUIRE(VM_OK >= error && error >= VM_ERROR_UNKNOWN)
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
vm_print_stack(void)
{
    if (psp > &pstack[0])
    {
        printf("<%lu>", psp - &pstack[0]);
        for (Cell *ptr = &pstack[0]; ptr < psp; ++ptr)
        {
            printf(" 0x%0*lX", (int)sizeof(Cell), *ptr);
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
    Vm_Error err = VM_OK;

    err = vm_register_builtins();
    if (err != VM_OK)
    {
        vm_report_error(err, "failed to register builtins");
        return EXIT_FAILURE;
    }

    err = vm_start();
    assert(err != VM_OK && "main loop exited unexpectedly with VM_OK");
    if (err == VM_BYE)
    {
        vm_print_stack();
        puts(" ok");
    }
    else
    {
        // TODO: Provide better error diagnostics
        vm_print_stack();
        puts(" err");
        fprintf(stderr, "%s\n", vm_error_as_string(err));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
