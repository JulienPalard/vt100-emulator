/*
 * Copyright (c) 2016 Julien Palard.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>
#include <string.h>
#include <pty.h>
#include <stdio.h>
#include "hl_vt100.h"

void disp(struct vt100_headless *vt100)
{
    unsigned int y;
    const char **lines;

    lines = vt100_headless_getlines(vt100);
    write(1, "\n", 1);
    for (y = 0; y < vt100->term->height; ++y)
    {
        write(1, "|", 1);
        write(1, lines[y], vt100->term->width);
        write(1, "|\n", 2);
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
    vt100_headless = new_vt100_headless();
    vt100_headless->changed = disp;
    vt100_headless_fork(vt100_headless, av[1], (av + 1));
    vt100_headless_main_loop(vt100_headless);
    return EXIT_SUCCESS;
}
