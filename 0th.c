#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Although I want this program to be platform-agnostic, it would be nice to
 * leverage compiler-specific intrinsics. I'm sure other compilers provide
 * something like __builtin_expect.
 */
#ifdef __GNUC__
    #define UNLIKELY(cond) __builtin_expect(cond, 0)
#else
    #define UNLIKELY(cond) (cond)
#endif

#define USED(x)      ((void)x)
#define EXPECT(cond) if (UNLIKELY(!(cond)))

/* Fundamental data types of the interpreter.
 *
 * CELL_TYPE represents the unit of computation. It can hold pointers and
 * integer values efficiently, typically corresponding to the native word size
 * of the system.
 *
 * BYTE_TYPE represents the smallest addressable unit of memory. It typically
 * corresponds to an 8-bit byte on most systems.
 *
 * FCELL_TYPE represents the unit of floating-point calculations. It defaults
 * to double-precision floating-point arithmetic (IEEE 754), but may be
 * redefined as single-precision (float) if the target platform doesn't support
 * double-precision.
 */
#define BYTE_TYPE  unsigned char
#define BYTE_SIZE  (sizeof(BYTE_TYPE))
#define CELL_TYPE  uintptr_t
#define CELL_SIZE  (sizeof(CELL_TYPE))
#define FCELL_TYPE double
#define FCELL_SIZE (sizeof(FCELL_TYPE))

/* Memory layout and stack sizes for the interpreter.
 *
 * MEMORY_SIZE defines the total size of the main memory, set to 5 KB
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
#define MEMORY_SIZE  (5UL * 1024UL * BYTE_SIZE)
#define PSTACK_ITEMS (64U)
#define RSTACK_ITEMS (64U)
#define FSTACK_ITEMS (6U)
#define PSTACK_SIZE  (PSTACK_ITEMS * CELL_SIZE)
#define RSTACK_SIZE  (RSTACK_ITEMS * CELL_SIZE)
#define FSTACK_SIZE  (FSTACK_ITEMS * FCELL_SIZE)

#define FLAG_IMMEDIATE (0x80)
#define FLAG_HIDDEN    (0x20)
#define MASK_LENGTH    (0x1f)

typedef BYTE_TYPE Byte;
typedef CELL_TYPE Cell;
typedef FCELL_TYPE FCell;

/* A list of all possible error codes.
 *
 * New errors may be added, but ERROR_UNKNOWN must always be the last entry.
 */
#define LIST_OF_ERRORS                                                         \
    ERROR(OK, 0)                                                               \
    ERROR(ABORT, -1)                                                           \
    ERROR(PSTACK_OVERFLOW, -2)                                                 \
    ERROR(PSTACK_UNDERFLOW, -3)                                                \
    ERROR(RSTACK_OVERFLOW, -4)                                                 \
    ERROR(RSTACK_UNDERFLOW, -5)                                                \
    ERROR(FSTACK_OVERFLOW, -6)                                                 \
    ERROR(FSTACK_UNDERFLOW, -7)                                                \
    ERROR(UNKNOWN, ERROR_FSTACK_UNDERFLOW - 1)

/* A fixed set of return values for all interpreter operations, indicating
 * success or failure.
 *
 * ERROR_OK indicates success, while all other values represent specific failure
 * conditions.
 */
typedef enum
{
#define ERROR(error_name, error_value) ERROR_##error_name = (error_value),
    LIST_OF_ERRORS
#undef ERROR
} Error;

/* Human-readable representations of all the error codes defined above. */
const char *error_names[] = {
#define ERROR(error_name, _) "ERROR_" #error_name,
    LIST_OF_ERRORS
#undef ERROR
};

typedef struct
{
    /* The program's main memory. */
    Byte memory[MEMORY_SIZE];

    /* Is the VM executing code (0) or compiling a word (non-zero)? */
    Cell mode;
    /* Address to the most recently-defined word in the dictionary. */
    Cell latest;
    /* Next free cell in memory. Compiled words will go here. */
    Cell here;

    /* Index of the current position in the parameter stack. */
    Cell psp;
    /* Base of the parameter stack. */
    Cell pstack[PSTACK_ITEMS];

    /* Index of the current position in the return stack. */
    Cell rsp;
    /* Base of the return stack. */
    Cell rstack[RSTACK_ITEMS];

    /* Index of the current position in the floating point stack. */
    Cell fsp;
    /* Base of the floating point stack. */
    FCell fstack[FSTACK_ITEMS];
} VmState;

void
vm_initialize(VmState *vm)
{
    vm->mode = 0;
    vm->latest = 0;
    vm->here = 0;

    vm->psp = 0;
    vm->rsp = 0;
    vm->fsp = 0;

    memset(vm->memory, 0, MEMORY_SIZE);
    memset(vm->pstack, 0, PSTACK_SIZE);
    memset(vm->rstack, 0, RSTACK_SIZE);
    memset(vm->fstack, 0, FSTACK_SIZE);
}

/* Pops the top cell from the parameter stack and stores it in `value`.
 *
 * If the stack is empty, returns ERROR_PSTACK_UNDERFLOW.
 */
Error
pop(VmState *vm, Cell *value)
{
    EXPECT(vm->psp > 0)
    {
        return ERROR_PSTACK_UNDERFLOW;
    }

    vm->psp -= 1;
    *value = vm->pstack[vm->psp];
    return ERROR_OK;
}

/* Pushes `data` onto the parameter stack.
 *
 * If there is no space left on the stack, returns ERROR_PSTACK_OVERFLOW.
 */
Error
push(VmState *vm, Cell data)
{
    EXPECT(vm->psp < PSTACK_SIZE)
    {
        return ERROR_PSTACK_OVERFLOW;
    }

    vm->pstack[vm->psp] = data;
    vm->psp += 1;
    return ERROR_OK;
}

/* Returns the string representation of the given error code.
 *
 * If the provided error code is invalid, returns the string representation of
 * ERROR_UNKNOWN instead.
 */
const char *
error_as_string(Error error)
{
    EXPECT(ERROR_OK >= error && error >= ERROR_UNKNOWN)
    {
        return error_names[abs(ERROR_UNKNOWN)];
    }

    return error_names[abs(error)];
}

int
main(void)
{
    VmState *vm = malloc(sizeof(VmState));
    if (!vm)
    {
        fprintf(stderr, "ERROR: Failed to allocate VM\n");
        return EXIT_FAILURE;
    }

    vm_initialize(vm);

    /* BRK: BEFORE RETURN */
    free(vm);
    return EXIT_SUCCESS;
}
