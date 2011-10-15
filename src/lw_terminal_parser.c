#include <stdlib.h>
#ifndef NDEBUG
#    include <stdio.h>
#endif

#include "lw_terminal_parser.h"

static void lw_terminal_push(struct lw_terminal *this, char c)
{
    if (this->stack_ptr >= TERM_STACK_SIZE)
        return ;
    this->stack[this->stack_ptr++] = c;
}

static void lw_terminal_parse_params(struct lw_terminal *this)
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

static void lw_terminal_call_CSI(struct lw_terminal *this, char c)
{
    lw_terminal_parse_params(this);
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

static void lw_terminal_call_ESC(struct lw_terminal *this, char c)
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

static void lw_terminal_call_HASH(struct lw_terminal *this, char c)
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

static void lw_terminal_call_GSET(struct lw_terminal *this, char c)
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
void lw_terminal_read(struct lw_terminal *this, char c)
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
            lw_terminal_call_ESC(this, c);
        else this->write(this, c);
    }
    else if (this->state == HASH)
    {
        if (c >= '0' && c <= '9')
            lw_terminal_call_HASH(this, c);
        else
            this->write(this, c);
    }
    else if (this->state == G0SET || this->state == G1SET)
    {
        lw_terminal_call_GSET(this, c);
    }
    else if (this->state == CSI)
    {
        if (c == '?')
            this->flag = '?';
        else if (c == ';' || (c >= '0' && c <= '9'))
            lw_terminal_push(this, c);
        else if (c >= '?' && c <= 'z')
            lw_terminal_call_CSI(this, c);
        else
            this->write(this, c);
    }
}

void lw_terminal_read_str(struct lw_terminal *this, char *c)
{
    while (*c)
        lw_terminal_read(this, *c++);
}

#ifndef NDEBUG
void lw_terminal_default_unimplemented(struct lw_terminal* this, char *seq, char chr)
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
void lw_terminal_default_unimplemented(struct lw_terminal* this, char *seq, char chr)
{
    this = this;
    seq = seq;
    chr = chr;
}
#endif

struct lw_terminal *lw_terminal_init(void)
{
    return calloc(1, sizeof(struct lw_terminal));
}

void lw_terminal_destroy(struct lw_terminal* this)
{
    free(this);
}
