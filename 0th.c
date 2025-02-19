#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define USED(x) ((void)x)

#define CELL_TYPE uintptr_t
#define CELL_SIZE (sizeof(CELL_TYPE))
#define BYTE_TYPE char
#define BYTE_SIZE (sizeof(BYTE_TYPE))

#define MEMORY_SIZE      (UINTMAX_C(5) * 1024 * BYTE_SIZE)
#define PSTACK_NUM_CELLS (64)
#define RSTACK_NUM_CELLS (64)
#define PSTACK_SIZE      (PSTACK_NUM_CELLS * CELL_SIZE)
#define RSTACK_SIZE      (RSTACK_NUM_CELLS * CELL_SIZE)

/**
 * These constants define where exactly each global variable is defined in main
 * memory. These values will be placed at the very beginning of memory, although
 * this can be changed by adjusting IDX_STATE.
 *
 * The layout of these variables resembles the following struct:
 *
 *     struct memory_header
 *     {
 *         cell_t state;
 *         cell_t latest;
 *         cell_t here;
 *         cell_t psp;
 *         cell_t pstack[PSTACK_NUM_CELLS];
 *         cell_t rsp;
 *         cell_t rstack[RSTACK_NUM_CELLS];
 *     };
 *
 * You may have noticed that IDX_HERE_START is not listed in this struct. This
 * is because it is to be only used as a marker, indicating the cell right after
 * rstack's last cell.
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

/**
 * A cell is the primary unit of information of a machine.
 */
typedef CELL_TYPE cell_t;

/**
 * A byte is the smallest addressable unit of a machine that can contain a basic
 * character set.
 */
typedef BYTE_TYPE byte_t;

/**
 * A contiguous array of bytes representing the program's main memory. To keep
 * things simple, the program will not allocate memory dynamically.
 */
byte_t memory[MEMORY_SIZE] = {0};

/**
 * Is the interpreter executing code (0) or compiling a word (non-zero)?
 */
cell_t *state;
/**
 * The most recently defined word in the dictionary.
 */
cell_t *latest;
/**
 * The next free cell in memory. When compiling, compiled words will go here.
 */
cell_t *here;
/**
 * The index into the parameter stack currently being pointed to.
 */
cell_t *psp;
/**
 * The address of the beginning of the parameter stack.
 */
cell_t *pstack;
/**
 * The index into the return stack currently being pointed to.
 */
cell_t *rsp;
/**
 * The address of the beginning of the return stack.
 */
cell_t *rstack;

int
main(void)
{
    int result = EXIT_SUCCESS;

    // Ensure there is enough memory to be used at runtime.
    assert(IDX_HERE_START < MEMORY_SIZE);

    // Assign each variable its appropriate location in main memory.
    state = (cell_t *)&memory[IDX_STATE];
    latest = (cell_t *)&memory[IDX_LATEST];
    here = (cell_t *)&memory[IDX_HERE];
    psp = (cell_t *)&memory[IDX_PSP];
    pstack = (cell_t *)&memory[IDX_PSTACK];
    rsp = (cell_t *)&memory[IDX_RSP];
    rstack = (cell_t *)&memory[IDX_RSTACK];

    *state = 0;
    *latest = 0;
    *psp = *rsp = 0;
    *here = IDX_HERE_START;

    return result;
}
