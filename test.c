#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>
#include <string.h>
#include <pty.h>
#include <stdio.h>
#include "vt100.h"

#define CHILD 0
#define SCROLLBACK 3

struct headless_terminal
{
    unsigned int width;
    unsigned int height;
    unsigned int x;
    unsigned int y;
    unsigned int saved_x;
    unsigned int saved_y;
    unsigned int top_line; /* Line at the top of the display */
    char         *screen;
};

void set(struct headless_terminal *term, unsigned int x, unsigned int y, char c)
{
    term->screen[(term->top_line * term->width + x + term->width * y)
                 % term->width * SCROLLBACK * term->height] = c;
}

/*
DECSC – Save Cursor (DEC Private)

ESC 7

This sequence causes the cursor position, graphic rendition, and
character set to be saved. (See DECRC).
*/
void DECSC(struct vt100_emul *vt100)
{
    /*TODO: Save graphic rendition and charset.*/
    struct headless_terminal *term;

    term = (struct headless_terminal *)vt100->user_data;
    term->saved_x = term->x;
    term->saved_y = term->y;
}

/*
DECRC – Restore Cursor (DEC Private)

ESC 8

This sequence causes the previously saved cursor position, graphic
rendition, and character set to be restored.
*/
void DECRC(struct vt100_emul *vt100)
{
    /*TODO Save graphic rendition and charset */
    struct headless_terminal *term;

    term = (struct headless_terminal *)vt100->user_data;
    term->x = term->saved_x;
    term->y = term->saved_y;
}

/*
IND – Index

ESC D

This sequence causes the active position to move downward one line
without changing the column position. If the active position is at the
bottom margin, a scroll up is performed. Format Effector
*/
void IND(struct vt100_emul *vt100)
{
    struct headless_terminal *term;

    term = (struct headless_terminal *)vt100->user_data;
    if (term->y >= term->height - 1)
    {
        /* SCROLL */
        term->top_line = (term->top_line + 1) % (term->height * SCROLLBACK);
    }
    else
    {
        /* Do not scroll, just move downward on the current display space */
        term->y += 1;
    }
}

/*
NEL – Next Line

ESC E

This sequence causes the active position to move to the first position
on the next line downward. If the active position is at the bottom
margin, a scroll up is performed. Format Effector
*/
void NEL(struct vt100_emul *vt100)
{
    struct headless_terminal *term;

    term = (struct headless_terminal *)vt100->user_data;
    if (term->y >= term->height - 1)
    {
        /* SCROLL */
        term->top_line = (term->top_line + 1) % (term->height * SCROLLBACK);
    }
    else
    {
        /* Do not scroll, just move downward on the current display space */
        term->y += 1;
    }
    term->x = 0;
}

/*
CUU – Cursor Up – Host to VT100 and VT100 to Host

ESC [ Pn A        default value: 1

Moves the active position upward without altering the column
position. The number of lines moved is determined by the parameter. A
parameter value of zero or one moves the active position one line
upward. A parameter value of n moves the active position n lines
upward. If an attempt is made to move the cursor above the top margin,
the cursor stops at the top margin. Editor Function
*/
void CUU(struct vt100_emul *vt100)
{
    struct headless_terminal *term;
    unsigned int arg0;

    term = (struct headless_terminal *)vt100->user_data;
    arg0 = 1;
    if (vt100->argc > 0)
        arg0 = vt100->argv[0];
    if (arg0 == 0)
        arg0 = 1;
    if (arg0 <= term->y)
        term->y -= arg0;
    else
        term->y = 0;
}

/*
CUD – Cursor Down – Host to VT100 and VT100 to Host

ESC [ Pn B        default value: 1

The CUD sequence moves the active position downward without altering
the column position. The number of lines moved is determined by the
parameter. If the parameter value is zero or one, the active position
is moved one line downward. If the parameter value is n, the active
position is moved n lines downward. In an attempt is made to move the
cursor below the bottom margin, the cursor stops at the bottom
margin. Editor Function
*/
void CUD(struct vt100_emul *vt100)
{
    struct headless_terminal *term;
    unsigned int arg0;

    term = (struct headless_terminal *)vt100->user_data;
    arg0 = 1;
    if (vt100->argc > 0)
        arg0 = vt100->argv[0];
    if (arg0 == 0)
        arg0 = 1;
    term->y += arg0;
    if (term->y >= term->height)
        term->y = term->height - 1;
}

/*
CUF – Cursor Forward – Host to VT100 and VT100 to Host

ESC [ Pn C        default value: 1

The CUF sequence moves the active position to the right. The distance
moved is determined by the parameter. A parameter value of zero or one
moves the active position one position to the right. A parameter value
of n moves the active position n positions to the right. If an attempt
is made to move the cursor to the right of the right margin, the
cursor stops at the right margin. Editor Function
*/
void CUF(struct vt100_emul *vt100)
{
    struct headless_terminal *term;
    unsigned int arg0;

    term = (struct headless_terminal *)vt100->user_data;
    arg0 = 1;
    if (vt100->argc > 0)
        arg0 = vt100->argv[0];
    if (arg0 == 0)
        arg0 = 1;
    term->x += arg0;
    if (term->x >= term->width)
        term->x = term->width - 1;
}

/*
CUB – Cursor Backward – Host to VT100 and VT100 to Host

ESC [ Pn D        default value: 1

The CUB sequence moves the active position to the left. The distance
moved is determined by the parameter. If the parameter value is zero
or one, the active position is moved one position to the left. If the
parameter value is n, the active position is moved n positions to the
left. If an attempt is made to move the cursor to the left of the left
margin, the cursor stops at the left margin. Editor Function
*/
void CUB(struct vt100_emul *vt100)
{
    struct headless_terminal *term;
    unsigned int arg0;

    term = (struct headless_terminal *)vt100->user_data;
    arg0 = 1;
    if (vt100->argc > 0)
        arg0 = vt100->argv[0];
    if (arg0 == 0)
        arg0 = 1;
    if (arg0 < term->x)
        term->x -= arg0;
    else
        term->x = 0;
}

/*
CUP – Cursor Position

ESC [ Pn ; Pn H        default value: 1

The CUP sequence moves the active position to the position specified
by the parameters. This sequence has two parameter values, the first
specifying the line position and the second specifying the column
position. A parameter value of zero or one for the first or second
parameter moves the active position to the first line or column in the
display, respectively. The default condition with no parameters
present is equivalent to a cursor to home action. In the VT100, this
control behaves identically with its format effector counterpart,
HVP. Editor Function

The numbering of lines depends on the state of the Origin Mode
(DECOM).
*/
void CUP(struct vt100_emul *vt100)
{
    struct headless_terminal *term;
    int arg0;
    int arg1;

    term = (struct headless_terminal *)vt100->user_data;
    arg0 = 1;
    arg1 = 1;
    if (vt100->argc > 0)
        arg0 = vt100->argv[0] - 1;
    if (vt100->argc > 1)
        arg1 = vt100->argv[1] - 1;
    if (arg0 < 0)
        arg0 = 0;
    if (arg1 < 0)
        arg1 = 0;
    term->y = arg0;
    term->x = arg1;
}

/*
ED – Erase In Display

ESC [ Ps J        default value: 0

This sequence erases some or all of the characters in the display
according to the parameter. Any complete line erased by this sequence
will return that line to single width mode. Editor Function

Parameter Parameter Meaning
0         Erase from the active position to the end of the screen,
          inclusive (default)
1         Erase from start of the screen to the active position, inclusive
2         Erase all of the display – all lines are erased, changed to
          single-width, and the cursor does not move.
*/
void ED(struct vt100_emul *vt100)
{
    struct headless_terminal *term;
    unsigned int arg0;
    unsigned int x;
    unsigned int y;

    term = (struct headless_terminal *)vt100->user_data;
    arg0 = 0;
    if (vt100->argc > 0)
        arg0 = vt100->argv[0];
    if (arg0 == 0)
    {
        for (x = term->x; x < term->width; ++x)
            set(term, x, term->y, '\0');
        for (x = 0 ; x < term->width; ++x)
            for (y = term->y + 1; y < term->height; ++y)
                set(term, x, y, '\0');
    }
    else if (arg0 == 1)
    {
        for (x = 0 ; x < term->width; ++x)
            for (y = 0; y < term->y; ++y)
                set(term, x, y, '\0');
        for (x = 0; x <= term->x; ++x)
            set(term, x, term->y, '\0');
    }
    else if (arg0 == 2)
    {
        for (x = 0 ; x < term->width; ++x)
            for (y = 0; y < term->height; ++y)
                set(term, x, y, '\0');
    }
}

/*
EL – Erase In Line

ESC [ Ps K        default value: 0

Erases some or all characters in the active line according to the
parameter. Editor Function

Parameter Parameter Meaning
0         Erase from the active position to the end of the line, inclusive
          (default)
1         Erase from the start of the screen to the active position, inclusive
2         Erase all of the line, inclusive
*/
void EL(struct vt100_emul *vt100)
{
    struct headless_terminal *term;
    unsigned int arg0;
    unsigned int x;

    term = (struct headless_terminal *)vt100->user_data;
    arg0 = 0;
    if (vt100->argc > 0)
        arg0 = vt100->argv[0];
    if (arg0 == 0)
    {
        for (x = term->x; x < term->width; ++x)
            set(term, x, term->y, '\0');
    }
    else if (arg0 == 1)
    {
        for (x = 0; x <= term->x; ++x)
            set(term, x, term->y, '\0');
    }
    else if (arg0 == 2)
    {
        for (x = 0; x < term->width; ++x)
            set(term, x, term->y, '\0');
    }
}

/*
HVP – Horizontal and Vertical Position

ESC [ Pn ; Pn f        default value: 1

Moves the active position to the position specified by the
parameters. This sequence has two parameter values, the first
specifying the line position and the second specifying the column. A
parameter value of either zero or one causes the active position to
move to the first line or column in the display, respectively. The
default condition with no parameters present moves the active position
to the home position. In the VT100, this control behaves identically
with its editor function counterpart, CUP. The numbering of lines and
columns depends on the reset or set state of the origin mode
(DECOM). Format Effector
*/
void HVP(struct vt100_emul *vt100)
{
    CUP(vt100);
}

static void vt100_write(struct vt100_emul *vt100, char c __attribute__((unused)))
{
    struct headless_terminal *term;

    term = (struct headless_terminal *)vt100->user_data;
    printf("%c", c);
}

int main(void)
{
    struct vt100_emul *vt100;
    struct vt100_callbacks callbacks;
    struct headless_terminal terminal;
    int master;
    int child;
    struct winsize winsize;
    char buffer[4096];
    ssize_t read_size;

    memset(&callbacks, 0, sizeof(callbacks));
    winsize.ws_row = terminal.height = 24;
    winsize.ws_col = terminal.width = 80;
    terminal.screen = calloc(terminal.width * SCROLLBACK * terminal.height,
                             sizeof(*terminal.screen));
    terminal.x = 0;
    terminal.y = 0;
    terminal.top_line = 0;
    child = forkpty(&master, NULL, NULL, NULL);
    if (child == CHILD)
    {
        setsid();
        execlp("top", "top");
        return EXIT_SUCCESS;
    }
    else
    {
        callbacks.csi.HVP = (vt100_action)HVP;
        callbacks.csi.EL = (vt100_action)EL;
        callbacks.csi.ED = (vt100_action)ED;
        callbacks.csi.CUP = (vt100_action)CUP;
        callbacks.csi.CUF = (vt100_action)CUF;
        callbacks.csi.CUD = (vt100_action)CUD;
        callbacks.csi.CUU = (vt100_action)CUU;
        callbacks.csi.CUB = (vt100_action)CUB;
        callbacks.esc.NEL = (vt100_action)NEL;
        callbacks.esc.IND = (vt100_action)IND;
        callbacks.esc.DECRC = (vt100_action)DECRC;
        callbacks.esc.DECSC = (vt100_action)DECSC;

        vt100 = vt100_init(80, 24, &callbacks, vt100_write);
        vt100->user_data = &terminal;
        ioctl(master, TIOCSWINSZ, &winsize);
        while (42)
        {
            read_size = read(master, &buffer, 4096);
            if (read_size == -1)
            {
                perror("read");
                return EXIT_FAILURE;
            }
            buffer[read_size] = '\0';
            vt100_read_str(vt100, buffer);
        }
    }
    return EXIT_SUCCESS;
}
