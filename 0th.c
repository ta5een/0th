#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Arbitrary buffer size.
 */
#define SOURCE_BUFFER_SIZE (1024)

int
main(void)
{
    int result = EXIT_SUCCESS;

    char source_buffer[SOURCE_BUFFER_SIZE];
    memset(source_buffer, '\0', SOURCE_BUFFER_SIZE);
    if (fgets(source_buffer, SOURCE_BUFFER_SIZE, stdin) == NULL)
    {
        result = EXIT_FAILURE;
        perror("Failed to read from stdin");
        goto exit;
    }

exit:
    return result;
}
