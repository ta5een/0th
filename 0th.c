#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Although I want this program to be platform-agnostic, it would be nice to
 * leverage compiler-specific intrinsics. I'm sure other compilers provide
 * something like __builtin_expect.
 */
#if defined(__GNUC__) || defined(__clang__)
    #define UNLIKELY(cond) __builtin_expect(cond, 0)
#else
    #define UNLIKELY(cond) (cond)
#endif

#define UNUSED(var)        ((void)(var))
#define UNIMPLEMENTED(msg) assert(0 && (msg))

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

#define FLAG_IMMEDIATE (0x80)
#define FLAG_HIDDEN    (0x20)
#define MASK_LENGTH    (0x1f)

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
    VM_ERROR_ITEM(UNKNOWN, -8) // Must be last

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

    /* The program's main memory.
     *
     * Stores dictionary entries for all words (builtin and user-defined),
     * compiled code for user-defined words, and variables.
     */
    Byte memory[VM_MEMORY_SIZE];
} Vm;

/* Pushes `data` onto the parameter stack.
 *
 * If there is no space left on the stack, returns VM_ERROR_PSTACK_OVERFLOW.
 */
Vm_Error
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

/* Pops the top cell from the parameter stack and stores it in `data`.
 *
 * If the stack is empty, returns VM_ERROR_PSTACK_UNDERFLOW.
 */
Vm_Error
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

/* Human-readable representations of Vm_Error enum values. */
static const char *vm_error_names[] = {
#define VM_ERROR_ITEM(name, _) "VM_ERROR_" #name,
    VM_ERROR_LIST
#undef VM_ERROR_ITEM
};

/* Returns the string representation of the given error code.
 *
 * If the provided error code is invalid, returns the string representation of
 * VM_ERROR_UNKNOWN instead.
 */
const char *
vm_error_as_string(Vm_Error error)
{
    REQUIRE(VM_ERROR_OK >= error && error >= VM_ERROR_UNKNOWN)
    {
        return vm_error_names[abs(VM_ERROR_UNKNOWN)];
    }

    return vm_error_names[abs(error)];
}

void
vm_report_error(Vm_Error error, const char *context_msg)
{
    fprintf(stderr, "%s: %s\n", vm_error_as_string(error), context_msg);
}

int
main(void)
{
    Vm vm = {0};
    UNUSED(vm);

    /* BRK: BEFORE RETURN */
    return EXIT_SUCCESS;
}
