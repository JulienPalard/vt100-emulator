#include <stdlib.h>
#include "vt100.h"

/*
 * Source : http://vt100.net/docs/vt100-ug/chapter3.html#S3.3
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

    vt100->argc = 0;
    vt100->argv[0] = 0;
    for (i = 0; i < vt100->stack_ptr; ++i)
    {
        if (vt100->stack[i] >= '0' && vt100->stack[i] <= '9')
        {
            vt100->argv[vt100->argc] = vt100->argv[vt100->argc] * 10
                + vt100->stack[i] - '0';
        }
        else if (vt100->stack[i] == ';')
        {
            vt100->argc += 1;
            vt100->argv[vt100->argc] = 0;
        }
    }
}

void vt100_call_CSI(struct vt100_emul *vt100, char c)
{
    if (c < 'A' || c > 'z')
        return ;
    if (((vt100_action *)vt100->csi_callbacks)[c - 'A'] == NULL)
        return ;
    vt100_parse_params(vt100);
    ((vt100_action *)vt100->csi_callbacks)[c - 'A'](vt100);
}

void vt100_call_ESC(struct vt100_emul *vt100, char c)
{
    if (c < '0' || c > 'z')
        return ;
    if (((vt100_action *)vt100->esc_callbacks)[c - '0'] == NULL)
        return ;
    ((vt100_action *)vt100->esc_callbacks)[c - '0'](vt100);
}

void vt100_call_HASH(struct vt100_emul *vt100, char c)
{
    if (c < '0' || c > '9')
        return ;
    if (((vt100_action *)vt100->hash_callbacks)[c - '0'] == NULL)
        return ;
    ((vt100_action *)vt100->hash_callbacks)[c - '0'](vt100);
}

void vt100_call_GSET(struct vt100_emul *vt100, char c)
{
    if (c < '0' || c > 'B')
        return ;
    if (((vt100_action *)vt100->scs_callbacks)[c - '0'] == NULL)
        return ;
    ((vt100_action *)vt100->scs_callbacks)[c - '0'](vt100);
}

void vt100_read(struct vt100_emul *vt100, char c)
{
    if (vt100->state == INIT)
    {
        if (c != '\033')
            vt100->write(vt100, c);
        else
            vt100->state = ESC;
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
        if (c == ';' || (c >= '0' && c <= '9'))
            vt100_push(vt100, c);
        else
            vt100_call_CSI(vt100, c);
    }
}

struct vt100_emul *vt100_init(unsigned int width, unsigned int height,
                              struct vt100_ESC_callbacks *esc,
                              struct vt100_CSI_callbacks *csi,
                              struct vt100_HASH_callbacks *hash,
                              struct vt100_SCS_callbacks *scs)
{
    struct vt100_emul *vt100;

    vt100 = malloc(sizeof(*vt100));
    vt100->width = width;
    vt100->height = height;
    vt100->cursor_pos_x = 0;
    vt100->cursor_pos_y = 0;
    vt100->stack_ptr = 0;
    vt100->csi_callbacks = csi;
    vt100->hash_callbacks = hash;
    vt100->esc_callbacks = esc;
    vt100->scs_callbacks = scs;
    vt100->state = INIT;
    return vt100;
}
