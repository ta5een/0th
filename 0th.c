#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define USED(x) ((void)x)

/* These constants define the fundamental data types of the interpreter.
 *
 * CELL_TYPE represents the primary unit of computation, typically used for
 * integers and addresses stored on the stack. uintptr_t was chosen because
 * it is an unsigned integer type capable of holding a pointer, ensuring
 * compatibility across different architectures.
 *
 * BYTE_TYPE represents the smallest addressable unit in memory. unsigned char
 * was chosen instead of char to align with the ANS Forth standard, which
 * specifies that characters must be unsigned, at least 8 bits wide, of fixed
 * width, and no larger than a cell. This also prevents implementation-defined
 * behavior when working with raw memory operations.
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
 *         cell_t state;
 *         cell_t latest;
 *         cell_t here;
 *         cell_t psp;
 *         cell_t pstack[PSTACK_CELLS];
 *         cell_t rsp;
 *         cell_t rstack[RSTACK_CELLS];
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
typedef CELL_TYPE cell_t;

/* A byte is the smallest addressable unit in the machine that can store a basic
 * character set.
 */
typedef BYTE_TYPE byte_t;

/* A contiguous array of bytes representing the program's main memory. For
 * simplicity, the program does not allocate memory dynamically.
 */
byte_t memory[MEMORY_SIZE] = {0};

/* Indicates whether the interpreter is executing code (0) or compiling a word
 * (non-zero).
 */
cell_t *state;

/* Points to the most recently defined word in the dictionary. */
cell_t *latest;

/* Points to the next free cell in memory. Compiled words will go here. */
cell_t *here;

/* Points to the index of the current position in the parameter stack. */
cell_t *psp;

/* Points to the base of the parameter stack. */
cell_t *pstack;

/* Points to the index of the current position in the return stack. */
cell_t *rsp;

/* Points to the base of the return stack. */
cell_t *rstack;

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
    state = (cell_t *)&memory[IDX_STATE];
    latest = (cell_t *)&memory[IDX_LATEST];
    here = (cell_t *)&memory[IDX_HERE];
    psp = (cell_t *)&memory[IDX_PSP];
    pstack = (cell_t *)&memory[IDX_PSTACK];
    rsp = (cell_t *)&memory[IDX_RSP];
    rstack = (cell_t *)&memory[IDX_RSTACK];

    /* Initialize the intepreter context. */
    *state = 0;
    *latest = 0;
    *psp = *rsp = 0;
    *here = IDX_HERE_START;

    return result;
}
