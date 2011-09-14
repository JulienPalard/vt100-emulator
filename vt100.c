#include <stdlib.h>
#include "vt100.h"

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


void vt100_push(struct vt100_emul *vt100, char c)
{
    if (vt100->stack_ptr >= VT100_STACK_SIZE)
        return ;
    vt100->stack[vt100->stack_ptr++] = c;
}

void vt100_parse_params(struct vt100_emul *vt100)
{
    unsigned int i;
    int got_something;

    got_something = 0;
    vt100->argc = 0;
    vt100->argv[0] = 0;
    for (i = 0; i < vt100->stack_ptr; ++i)
    {
        if (vt100->stack[i] >= '0' && vt100->stack[i] <= '9')
        {
            got_something = 1;
            vt100->argv[vt100->argc] = vt100->argv[vt100->argc] * 10
                + vt100->stack[i] - '0';
        }
        else if (vt100->stack[i] == ';')
        {
            got_something = 0;
            vt100->argc += 1;
            vt100->argv[vt100->argc] = 0;
        }
    }
    vt100->argc += got_something;
}

void vt100_call_CSI(struct vt100_emul *vt100, char c)
{
    vt100_parse_params(vt100);
    if (c < '?' || c > 'z'
        || ((vt100_action *)&vt100->callbacks->csi)[c - '?'] == NULL)
    {
        if (vt100->unimplemented != NULL)
            vt100->unimplemented(vt100, "CSI", c);
        goto leave;
    }
    ((vt100_action *)&vt100->callbacks->csi)[c - '?'](vt100);
leave:
    vt100->state = INIT;
    vt100->flag = '\0';
    vt100->stack_ptr = 0;
    vt100->argc = 0;
}

void vt100_call_ESC(struct vt100_emul *vt100, char c)
{
    if (c < '0' || c > 'z'
        || ((vt100_action *)&vt100->callbacks->esc)[c - '0'] == NULL)
    {
        if (vt100->unimplemented != NULL)
            vt100->unimplemented(vt100, "ESC", c);
        goto leave;
    }
    ((vt100_action *)&vt100->callbacks->esc)[c - '0'](vt100);
leave:
    vt100->state = INIT;
    vt100->stack_ptr = 0;
    vt100->argc = 0;
}

void vt100_call_HASH(struct vt100_emul *vt100, char c)
{
    if (c < '0' || c > '9'
        || ((vt100_action *)&vt100->callbacks->hash)[c - '0'] == NULL)
    {
        if (vt100->unimplemented != NULL)
            vt100->unimplemented(vt100, "HASH", c);
        goto leave;
    }
    ((vt100_action *)&vt100->callbacks->hash)[c - '0'](vt100);
leave:
    vt100->state = INIT;
    vt100->stack_ptr = 0;
    vt100->argc = 0;
}

void vt100_call_GSET(struct vt100_emul *vt100, char c)
{
    if (c < '0' || c > 'B'
        || ((vt100_action *)&vt100->callbacks->scs)[c - '0'] == NULL)
    {
        if (vt100->unimplemented != NULL)
            vt100->unimplemented(vt100, "GSET", c);
        goto leave;
    }
    ((vt100_action *)&vt100->callbacks->scs)[c - '0'](vt100);
leave:
    vt100->state = INIT;
    vt100->stack_ptr = 0;
    vt100->argc = 0;
}

/*
** INIT
**  \_ ESC "\033"
**  |   \_ CSI   "\033["
**  |   |   \_ c == '?' : vt100->flag = '?'
**  |   |   \_ c == ';' || (c >= '0' && c <= '9') : vt100_push
**  |   |   \_ else : vt100_call_CSI()
**  |   \_ HASH  "\033#"
**  |   |   \_ vt100_call_hash()
**  |   \_ G0SET "\033("
**  |   |   \_ vt100_call_GSET()
**  |   \_ G1SET "\033)"
**  |   |   \_ vt100_call_GSET()
**  \_ vt100->write()
*/
void vt100_read(struct vt100_emul *vt100, char c)
{
    if (vt100->state == INIT)
    {
        if (c == '\033')
            vt100->state = ESC;
        else
            vt100->write(vt100, c);
    }
    else if (vt100->state == ESC)
    {
        if (c == '[')
            vt100->state = CSI;
        else if (c == '#')
            vt100->state = HASH;
        else if (c == '(')
            vt100->state = G0SET;
        else if (c == ')')
            vt100->state = G1SET;
        else
            vt100_call_ESC(vt100, c);
    }
    else if (vt100->state == HASH)
    {
        vt100_call_HASH(vt100, c);
    }
    else if (vt100->state == G0SET || vt100->state == G1SET)
    {
        vt100_call_GSET(vt100, c);
    }
    else if (vt100->state == CSI)
    {
        if (c == '?')
            vt100->flag = '?';
        else if (c == ';' || (c >= '0' && c <= '9'))
            vt100_push(vt100, c);
        else
            vt100_call_CSI(vt100, c);
    }
}

void vt100_read_str(struct vt100_emul *vt100, char *c)
{
    while (*c)
        vt100_read(vt100, *c++);
}

struct vt100_emul *vt100_init(unsigned int width, unsigned int height,
                              struct vt100_callbacks *callbacks,
                              void (*vtwrite)(struct vt100_emul *, char))
{
    struct vt100_emul *vt100;

    vt100 = malloc(sizeof(*vt100));
    vt100->width = width;
    vt100->height = height;
    vt100->cursor_pos_x = 0;
    vt100->cursor_pos_y = 0;
    vt100->stack_ptr = 0;
    vt100->callbacks = callbacks;
    vt100->state = INIT;
    vt100->write = vtwrite;
    return vt100;
}
