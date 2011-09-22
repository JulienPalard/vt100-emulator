#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>
#include <string.h>
#include <pty.h>
#include <stdio.h>
#include "vt100_headless.h"
#include "vt100.h"

void disp(struct vt100_headless *vt100)
{
    unsigned int y;
    struct vt100_term *term;
    const char **lines;

    term = vt100->term;
    lines = vt100_dump(term);
    for (y = 0; y < term->height; ++y)
    {
        write(1, lines[y], term->width);
        write(1, "\n", 1);
    }
}
int main(int ac, char **av)
{
    struct vt100_headless *vt100_headless;

    if (ac == 1)
    {
        puts("Usage: test PROGNAME");
        return EXIT_FAILURE;
    }
    vt100_headless = vt100_headless_init();
    vt100_headless->changed = disp;
    vt100_headless_fork(vt100_headless, av[1], (av + 1));
    return EXIT_SUCCESS;
}
