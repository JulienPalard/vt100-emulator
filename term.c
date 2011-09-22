#include <stdlib.h>
#ifndef NDEBUG
#    include <stdio.h>
#endif

#include "term.h"


/*
** Term implement a terminal, (vt100 like)
** It expose an API to implement a specific terminal, actually I implement
** vt100.
**
** term.c parses escape sequences like
** \033[4;2H
** It allows control chars to be inside the sequence like :
** \033[4\n;2H
** and accept variations like :
** \033#...
** \033(...
**
** The API is simple, it consists of a structure term_callbacks (see
** term.h) where 4 members points to a ascii_callbacks structure.
** Ascii callbacks is only a struct with some ascii chars where you
** can plug your callbacks functions. see vt100.c as an example.
**
*/

static void term_push(struct terminal *term, char c)
{
    if (term->stack_ptr >= TERM_STACK_SIZE)
        return ;
    term->stack[term->stack_ptr++] = c;
}

static void term_parse_params(struct terminal *term)
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

static void term_call_CSI(struct terminal *term, char c)
{
    term_parse_params(term);
    if (((term_action *)&term->callbacks.csi)[c - '0'] == NULL)
    {
        if (term->unimplemented != NULL)
            term->unimplemented(term, "CSI", c);
        goto leave;
    }
    ((term_action *)&term->callbacks.csi)[c - '0'](term);
leave:
    term->state = INIT;
    term->flag = '\0';
    term->stack_ptr = 0;
    term->argc = 0;
}

static void term_call_ESC(struct terminal *term, char c)
{
    if (((term_action *)&term->callbacks.esc)[c - '0'] == NULL)
    {
        if (term->unimplemented != NULL)
            term->unimplemented(term, "ESC", c);
        goto leave;
    }
    ((term_action *)&term->callbacks.esc)[c - '0'](term);
leave:
    term->state = INIT;
    term->stack_ptr = 0;
    term->argc = 0;
}

static void term_call_HASH(struct terminal *term, char c)
{
    if (((term_action *)&term->callbacks.hash)[c - '0'] == NULL)
    {
        if (term->unimplemented != NULL)
            term->unimplemented(term, "HASH", c);
        goto leave;
    }
    ((term_action *)&term->callbacks.hash)[c - '0'](term);
leave:
    term->state = INIT;
    term->stack_ptr = 0;
    term->argc = 0;
}

static void term_call_GSET(struct terminal *term, char c)
{
    if (c < '0' || c > 'B'
        || ((term_action *)&term->callbacks.scs)[c - '0'] == NULL)
    {
        if (term->unimplemented != NULL)
            term->unimplemented(term, "GSET", c);
        goto leave;
    }
    ((term_action *)&term->callbacks.scs)[c - '0'](term);
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
void term_read(struct terminal *term, char c)
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

void term_read_str(struct terminal *term, char *c)
{
    while (*c)
        term_read(term, *c++);
}

#ifndef NDEBUG
void term_default_unimplemented(struct terminal* term, char *seq, char chr)
{
    unsigned int argc;

    fprintf(stderr, "WARNING: UNIMPLEMENTED %s (", seq);
    for (argc = 0; argc < term->argc; ++argc)
    {
        fprintf(stderr, "%d", term->argv[argc]);
        if (argc != term->argc - 1)
            fprintf(stderr, ", ");
    }
    fprintf(stderr, ")%o\n", chr);
}
#else
void term_default_unimplemented(struct terminal* term, char *seq, char chr)
{
    term = term;
    seq = seq;
    chr = chr;
}
#endif

struct terminal *term_init(unsigned int width, unsigned int height,
                              void (*vtwrite)(struct terminal *, char))
{
    struct terminal *term;

    term = calloc(1, sizeof(*term));
    term->width = width;
    term->height = height;
    term->cursor_pos_x = 0;
    term->cursor_pos_y = 0;
    term->stack_ptr = 0;
    term->state = INIT;
    term->write = vtwrite;
    return term;
}
