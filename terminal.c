#include <stdlib.h>
#ifndef NDEBUG
#    include <stdio.h>
#endif

#include "terminal.h"


/*
** Term implement a terminal, (vt100 like)
** It expose an API to implement a specific terminal.
** Actually I implement vt100 in vt100.c
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

static void terminal_push(struct terminal *this, char c)
{
    if (this->stack_ptr >= TERM_STACK_SIZE)
        return ;
    this->stack[this->stack_ptr++] = c;
}

static void terminal_parse_params(struct terminal *this)
{
    unsigned int i;
    int got_something;

    got_something = 0;
    this->argc = 0;
    this->argv[0] = 0;
    for (i = 0; i < this->stack_ptr; ++i)
    {
        if (this->stack[i] >= '0' && this->stack[i] <= '9')
        {
            got_something = 1;
            this->argv[this->argc] = this->argv[this->argc] * 10
                + this->stack[i] - '0';
        }
        else if (this->stack[i] == ';')
        {
            got_something = 0;
            this->argc += 1;
            this->argv[this->argc] = 0;
        }
    }
    this->argc += got_something;
}

static void terminal_call_CSI(struct terminal *this, char c)
{
    terminal_parse_params(this);
    if (((term_action *)&this->callbacks.csi)[c - '0'] == NULL)
    {
        if (this->unimplemented != NULL)
            this->unimplemented(this, "CSI", c);
        goto leave;
    }
    ((term_action *)&this->callbacks.csi)[c - '0'](this);
leave:
    this->state = INIT;
    this->flag = '\0';
    this->stack_ptr = 0;
    this->argc = 0;
}

static void terminal_call_ESC(struct terminal *this, char c)
{
    if (((term_action *)&this->callbacks.esc)[c - '0'] == NULL)
    {
        if (this->unimplemented != NULL)
            this->unimplemented(this, "ESC", c);
        goto leave;
    }
    ((term_action *)&this->callbacks.esc)[c - '0'](this);
leave:
    this->state = INIT;
    this->stack_ptr = 0;
    this->argc = 0;
}

static void terminal_call_HASH(struct terminal *this, char c)
{
    if (((term_action *)&this->callbacks.hash)[c - '0'] == NULL)
    {
        if (this->unimplemented != NULL)
            this->unimplemented(this, "HASH", c);
        goto leave;
    }
    ((term_action *)&this->callbacks.hash)[c - '0'](this);
leave:
    this->state = INIT;
    this->stack_ptr = 0;
    this->argc = 0;
}

static void terminal_call_GSET(struct terminal *this, char c)
{
    if (c < '0' || c > 'B'
        || ((term_action *)&this->callbacks.scs)[c - '0'] == NULL)
    {
        if (this->unimplemented != NULL)
            this->unimplemented(this, "GSET", c);
        goto leave;
    }
    ((term_action *)&this->callbacks.scs)[c - '0'](this);
leave:
    this->state = INIT;
    this->stack_ptr = 0;
    this->argc = 0;
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
void terminal_read(struct terminal *this, char c)
{
    if (this->state == INIT)
    {
        if (c == '\033')
            this->state = ESC;
        else
            this->write(this, c);
    }
    else if (this->state == ESC)
    {
        if (c == '[')
            this->state = CSI;
        else if (c == '#')
            this->state = HASH;
        else if (c == '(')
            this->state = G0SET;
        else if (c == ')')
            this->state = G1SET;
        else if (c >= '0' && c <= 'z')
            terminal_call_ESC(this, c);
        else this->write(this, c);
    }
    else if (this->state == HASH)
    {
        if (c >= '0' && c <= '9')
            terminal_call_HASH(this, c);
        else
            this->write(this, c);
    }
    else if (this->state == G0SET || this->state == G1SET)
    {
        terminal_call_GSET(this, c);
    }
    else if (this->state == CSI)
    {
        if (c == '?')
            this->flag = '?';
        else if (c == ';' || (c >= '0' && c <= '9'))
            terminal_push(this, c);
        else if (c >= '?' && c <= 'z')
            terminal_call_CSI(this, c);
        else
            this->write(this, c);
    }
}

void terminal_read_str(struct terminal *this, char *c)
{
    while (*c)
        terminal_read(this, *c++);
}

#ifndef NDEBUG
void terminal_default_unimplemented(struct terminal* this, char *seq, char chr)
{
    unsigned int argc;

    fprintf(stderr, "WARNING: UNIMPLEMENTED %s (", seq);
    for (argc = 0; argc < this->argc; ++argc)
    {
        fprintf(stderr, "%d", this->argv[argc]);
        if (argc != this->argc - 1)
            fprintf(stderr, ", ");
    }
    fprintf(stderr, ")%o\n", chr);
}
#else
void terminal_default_unimplemented(struct terminal* this, char *seq, char chr)
{
    this = this;
    seq = seq;
    chr = chr;
}
#endif

struct terminal *terminal_init(void)
{
    return calloc(1, sizeof(struct terminal));
}
