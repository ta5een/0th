#include <assert.h>
#include <stdlib.h>

#define USED(x) ((void)x)

#ifdef __GNUC__
    #define EXPECT(cond) if (__builtin_expect(!(cond), 0))
#else
    #define EXPECT(cond) if (!(cond))
#endif

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
#define CELL_TYPE uintptr_t
#define CELL_SIZE (sizeof(CELL_TYPE))
#define BYTE_TYPE unsigned char
#define BYTE_SIZE (sizeof(BYTE_TYPE))

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
#define MEMORY_SIZE  (UINTMAX_C(5) * 1024 * BYTE_SIZE)
#define PSTACK_CELLS (64)
#define RSTACK_CELLS (64)
#define PSTACK_SIZE  (PSTACK_CELLS * CELL_SIZE)
#define RSTACK_SIZE  (RSTACK_CELLS * CELL_SIZE)

/* These constants define the exact locations of the interpreter context in main
 * memory. By default, the context is placed at the beginning of memory, but
 * this can be adjusted by modifying IDX_STATE.
 *
 * The layout of these variables resembles the following struct:
 *
 *     struct interpreter_context
 *     {
 *         Zth_Cell state;
 *         Zth_Cell latest;
 *         Zth_Cell here;
 *         Zth_Cell psp;
 *         Zth_Cell pstack[PSTACK_CELLS];
 *         Zth_Cell rsp;
 *         Zth_Cell rstack[RSTACK_CELLS];
 *     };
 *
 * Note that IDX_HERE_START is not part of this struct. Instead, it serves as a
 * marker indicating the cell immediately after the last element of rstack.
 */
#define IDX_STATE      (0)
#define IDX_LATEST     (IDX_STATE + CELL_SIZE)
#define IDX_HERE       (IDX_LATEST + CELL_SIZE)
#define IDX_PSP        (IDX_HERE + CELL_SIZE)
#define IDX_PSTACK     (IDX_PSP + CELL_SIZE)
#define IDX_RSP        (IDX_PSTACK + PSTACK_SIZE)
#define IDX_RSTACK     (IDX_RSP + CELL_SIZE)
#define IDX_HERE_START (IDX_RSTACK + RSTACK_SIZE)

#define FLAG_IMMEDIATE (0x80)
#define FLAG_HIDDEN    (0x20)
#define MASK_LENGTH    (0x1f)

/* A cell is the primary unit of information in the machine. */
typedef CELL_TYPE Zth_Cell;

/* A byte is the smallest addressable unit in the machine that can store a basic
 * character set.
 */
typedef BYTE_TYPE Zth_Byte;

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
} Zth_Error;

/* Human-readable representations of all the error codes defined above. */
const char *error_names[] = {
#define ERROR(error_name, _) "ERROR_" #error_name,
    LIST_OF_ERRORS
#undef ERROR
};

/* A contiguous array of bytes representing the program's main memory. For
 * simplicity, the program does not allocate memory dynamically.
 */
Zth_Byte memory[MEMORY_SIZE] = {0};

/* Indicates whether the interpreter is executing code (0) or compiling a word
 * (non-zero).
 */
Zth_Cell *state;

/* Points to the most recently defined word in the dictionary. */
Zth_Cell *latest;

/* Points to the next free cell in memory. Compiled words will go here. */
Zth_Cell *here;

/* Points to the index of the current position in the parameter stack. */
Zth_Cell *psp;

/* Points to the base of the parameter stack. */
Zth_Cell *pstack;

/* Points to the index of the current position in the return stack. */
Zth_Cell *rsp;

/* Points to the base of the return stack. */
Zth_Cell *rstack;

/* Pops the top cell from the parameter stack and stores it in `value`.
 *
 * If the stack is empty, returns ERROR_PSTACK_UNDERFLOW.
 */
Zth_Error
pop(Zth_Cell *value)
{
    EXPECT(*psp > 0)
    {
        return ERROR_PSTACK_UNDERFLOW;
    }

    (*psp) -= 1;
    *value = pstack[(*psp)];
    return ERROR_OK;
}

/* Pushes `data` onto the parameter stack.
 *
 * If there is no space left on the stack, returns ERROR_PSTACK_OVERFLOW.
 */
Zth_Error
push(Zth_Cell data)
{
    EXPECT(*psp < PSTACK_SIZE)
    {
        return ERROR_PSTACK_OVERFLOW;
    }

    pstack[(*psp)] = data;
    (*psp) += 1;
    return ERROR_OK;
}

/* Returns the string representation of the given error code.
 *
 * If the provided error code is invalid, returns the string representation of
 * ERROR_UNKNOWN instead.
 */
const char *
error_as_string(Zth_Error error)
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

    /* Verify that the intepreter context, parameter stack, and return stack all
     * fit within the program's main memory. Ideally, there should be enough
     * remaining memory to store the dictionary at runtime.
     */
    assert(IDX_HERE_START < MEMORY_SIZE);

    /* Map each field in the intepreter context to their respective locations in
     * main memory.
     */
    state = (Zth_Cell *)&memory[IDX_STATE];
    latest = (Zth_Cell *)&memory[IDX_LATEST];
    here = (Zth_Cell *)&memory[IDX_HERE];
    psp = (Zth_Cell *)&memory[IDX_PSP];
    pstack = (Zth_Cell *)&memory[IDX_PSTACK];
    rsp = (Zth_Cell *)&memory[IDX_RSP];
    rstack = (Zth_Cell *)&memory[IDX_RSTACK];

    /* Initialize the intepreter context. */
    *state = 0;
    *latest = 0;
    *psp = *rsp = 0;
    *here = IDX_HERE_START;

    return result;
}
