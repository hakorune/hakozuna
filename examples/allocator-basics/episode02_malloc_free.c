/*
 * Episode 2: the basic malloc -> use -> free flow.
 *
 * This is intentionally small and portable.  It shows the application-side
 * contract; the allocator's internal thread-local fronts and shared backing
 * storage are hidden behind malloc/free.
 */

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const size_t count = 4;

    /* malloc asks the allocator for a block of memory. */
    int *p = malloc(count * sizeof *p);
    if (p == NULL) {
        fprintf(stderr, "malloc failed\n");
        return EXIT_FAILURE;
    }

    /* The returned pointer is the application's handle for that block. */
    for (size_t i = 0; i < count; ++i) {
        p[i] = (int)(i + 1);
    }
    printf("values: %d %d %d %d\n", p[0], p[1], p[2], p[3]);

    /* free returns the block to the allocator when it is no longer needed. */
    free(p);
    p = NULL; /* avoid accidentally using the old address again */

    return EXIT_SUCCESS;
}
