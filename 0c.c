#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define SHIFT_ARGS(argv, argc) (assert((argc) > 0), (argc)--, *(argv)++)

void
cmd_print_usage(const char *exe_path)
{
    fprintf(stderr, "USAGE: %s file\n", exe_path);
}

int
main(int argc, char *argv[])
{
    const char *exe_path = SHIFT_ARGS(argv, argc);

    if (argc < 1)
    {
        cmd_print_usage(exe_path);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
