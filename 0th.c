#include <assert.h>
#include <stdlib.h>

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

/* These constants define the fundamental data types of the interpreter.
 *
 * CELL_TYPE represents the fundamental unit of computation in the intepreter.
 * It is an implementation-defined type that can hold pointers and integer
 * values efficiently, typically corresponding to the native word size of the
 * system.
 *
 * BYTE_TYPE represents the smallest addressable unit of memory. It is an
 * implementation-defined type that corresponds to an 8-bit byte on most
 * systems.
 */
#define BYTE_TYPE unsigned char
#define BYTE_SIZE (sizeof(BYTE_TYPE))
#define CELL_TYPE uintptr_t
#define CELL_SIZE (sizeof(CELL_TYPE))

/* Memory layout and stack sizes for the interpreter.
 *
 * MEMORY_SIZE defines the total size of the main memory, set to 5 KB
 * (5 * 1024 bytes) as a reasonable starting size for the system.
 *
 * The interpreter has two stacks:
 * - PSTACK (parameter stack, also known as the data stack) is used for
 *   passing values between words.
 * - RSTACK (return stack) is used for storing return addresses during
 *   execution control flow.
 *
 * PSTACK_CELLS and RSTACK_CELLS define the number of cells each stack can hold.
 * Both are set to 64 cells, following Brad Rodriguez's recommendation in
 * "MOVING FORTH Part 1: Design Decisions in the Forth Kernel". While ANS Forth
 * specifies a minimum of 32 cells for the parameter stack and 24 cells for
 * the return stack, 64 cells for each provides a more practical balance.
 *
 * PSTACK_SIZE and RSTACK_SIZE calculate the actual memory size of each
 * stack by multiplying the number of cells by the size of a single cell.
 */
#define MEMORY_SIZE  (5UL * 1024UL * BYTE_SIZE)
#define PSTACK_ITEMS (64U)
#define RSTACK_ITEMS (64U)
#define PSTACK_SIZE  (PSTACK_ITEMS * CELL_SIZE)
#define RSTACK_SIZE  (RSTACK_ITEMS * CELL_SIZE)

#define FLAG_IMMEDIATE (0x80)
#define FLAG_HIDDEN    (0x20)
#define MASK_LENGTH    (0x1f)

typedef BYTE_TYPE Byte;
typedef CELL_TYPE Cell;

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
    ERROR(UNKNOWN, -6)

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
} VmState;

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
    int result = EXIT_SUCCESS;
    VmState vm = {0};
    USED(vm);

    return result;
}
