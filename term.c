#include <stdlib.h>
#include "term.h"

/*
 * Source : http://vt100.net/docs/vt100-ug/chapter3.html
            http://vt100.net/docs/tp83/appendixb.html
 * It's a vt100 implementation, that implements ANSI control function.
 */

/*
 * Vocabulary
 * CSI | Control Sequence Introducer | ESC [
 * Pn  | Numeric parameter           | [0-9]+
 * Ps  | Selective parameter         |
 *     | Parameter string            | List of parameters separated by ';'
 */

/*
 *  Control sequences - VT100 to host ( write )
 * CPR | Cursor Position Report | ESC [ Pn ; Pn R
 *
 *
 * Control sequencs - Host to VT100 AND VT100 to host ( read write )
 * CUB | Cursor Backward        | ESC [ Pn D
 *
 */

/*
 * Naming convention :
 * [rw]_shortname(struct vt100emul vt100, ...);
 * So CPR is w_CPR(struct vt100emul vt100, int pn1, int pn2);
 * And CUB is r_CUB(struct vt100emul vt100, int pn1, int pn2);
 *        AND w_CUB(struct vt100emul vt100, int pn1, int pn2);
 */


void term_push(struct term_emul *term, char c)
{
    if (term->stack_ptr >= TERM_STACK_SIZE)
        return ;
    term->stack[term->stack_ptr++] = c;
}

void term_parse_params(struct term_emul *term)
{
    unsigned int i;
    int got_something;

    got_something = 0;
    term->argc = 0;
    term->argv[0] = 0;
    for (i = 0; i < term->stack_ptr; ++i)
    {
        if (term->stack[i] >= '0' && term->stack[i] <= '9')
        {
            got_something = 1;
            term->argv[term->argc] = term->argv[term->argc] * 10
                + term->stack[i] - '0';
        }
        else if (term->stack[i] == ';')
        {
            got_something = 0;
            term->argc += 1;
            term->argv[term->argc] = 0;
        }
    }
    term->argc += got_something;
}

void term_call_CSI(struct term_emul *term, char c)
{
    term_parse_params(term);
    if (((term_action *)&term->callbacks->csi)[c - '?'] == NULL)
    {
        if (term->unimplemented != NULL)
            term->unimplemented(term, "CSI", c);
        goto leave;
    }
    ((term_action *)&term->callbacks->csi)[c - '?'](term);
leave:
    term->state = INIT;
    term->flag = '\0';
    term->stack_ptr = 0;
    term->argc = 0;
}

void term_call_ESC(struct term_emul *term, char c)
{
    if (((term_action *)&term->callbacks->esc)[c - '0'] == NULL)
    {
        if (term->unimplemented != NULL)
            term->unimplemented(term, "ESC", c);
        goto leave;
    }
    ((term_action *)&term->callbacks->esc)[c - '0'](term);
leave:
    term->state = INIT;
    term->stack_ptr = 0;
    term->argc = 0;
}

void term_call_HASH(struct term_emul *term, char c)
{
    if (((term_action *)&term->callbacks->hash)[c - '0'] == NULL)
    {
        if (term->unimplemented != NULL)
            term->unimplemented(term, "HASH", c);
        goto leave;
    }
    ((term_action *)&term->callbacks->hash)[c - '0'](term);
leave:
    term->state = INIT;
    term->stack_ptr = 0;
    term->argc = 0;
}

void term_call_GSET(struct term_emul *term, char c)
{
    if (c < '0' || c > 'B'
        || ((term_action *)&term->callbacks->scs)[c - '0'] == NULL)
    {
        if (term->unimplemented != NULL)
            term->unimplemented(term, "GSET", c);
        goto leave;
    }
    ((term_action *)&term->callbacks->scs)[c - '0'](term);
leave:
    term->state = INIT;
    term->stack_ptr = 0;
    term->argc = 0;
}

/*
** INIT
**  \_ ESC "\033"
**  |   \_ CSI   "\033["
**  |   |   \_ c == '?' : term->flag = '?'
**  |   |   \_ c == ';' || (c >= '0' && c <= '9') : term_push
**  |   |   \_ else : term_call_CSI()
**  |   \_ HASH  "\033#"
**  |   |   \_ term_call_hash()
**  |   \_ G0SET "\033("
**  |   |   \_ term_call_GSET()
**  |   \_ G1SET "\033)"
**  |   |   \_ term_call_GSET()
**  \_ term->write()
*/
void term_read(struct term_emul *term, char c)
{
    if (term->state == INIT)
    {
        if (c == '\033')
            term->state = ESC;
        else
            term->write(term, c);
    }
    else if (term->state == ESC)
    {
        if (c == '[')
            term->state = CSI;
        else if (c == '#')
            term->state = HASH;
        else if (c == '(')
            term->state = G0SET;
        else if (c == ')')
            term->state = G1SET;
        else if (c >= '0' && c <= 'z')
            term_call_ESC(term, c);
        else term->write(term, c);
    }
    else if (term->state == HASH)
    {
        if (c >= '0' && c <= '9')
            term_call_HASH(term, c);
        else
            term->write(term, c);
    }
    else if (term->state == G0SET || term->state == G1SET)
    {
        term_call_GSET(term, c);
    }
    else if (term->state == CSI)
    {
        if (c == '?')
            term->flag = '?';
        else if (c == ';' || (c >= '0' && c <= '9'))
            term_push(term, c);
        else if (c >= '?' && c <= 'z')
            term_call_CSI(term, c);
        else
            term->write(term, c);
    }
}

void term_read_str(struct term_emul *term, char *c)
{
    while (*c)
        term_read(term, *c++);
}

struct term_emul *term_init(unsigned int width, unsigned int height,
                              struct term_callbacks *callbacks,
                              void (*vtwrite)(struct term_emul *, char))
{
    struct term_emul *term;

    term = malloc(sizeof(*term));
    term->width = width;
    term->height = height;
    term->cursor_pos_x = 0;
    term->cursor_pos_y = 0;
    term->stack_ptr = 0;
    term->callbacks = callbacks;
    term->state = INIT;
    term->write = vtwrite;
    return term;
}
