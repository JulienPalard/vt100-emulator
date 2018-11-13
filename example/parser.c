#include <stdlib.h>
#include <stdio.h>
#include "../src/lw_terminal_parser.h"

static void vt100_write(struct lw_terminal *term_emul __attribute__((unused)),
                        char c)
{
    printf("Got a char : %c\n", c);
}

static void csi_f(struct lw_terminal *term_emul)
{
    printf("\\033[...f with %d parameters\n", term_emul->argc);
}

static void csi_K(struct lw_terminal *term_emul)
{
    printf("\\033[...K with %d parameters\n", term_emul->argc);
}

int main(void)
{
    struct lw_terminal *lw_terminal;

    lw_terminal = lw_terminal_parser_init();
    if (lw_terminal == NULL)
        return EXIT_FAILURE;
    lw_terminal->write = vt100_write;
    lw_terminal->callbacks.csi.f = csi_f;
    lw_terminal->callbacks.csi.K = csi_K;
    lw_terminal_parser_read_str(lw_terminal, "\033[2KHello world !\033[f");
    return EXIT_SUCCESS;
}
