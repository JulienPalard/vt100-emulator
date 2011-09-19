#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>
#include <string.h>
#include <pty.h>
#include <stdio.h>
#include "term.h"

/*
 * Source : http://vt100.net/docs/vt100-ug/chapter3.html
            http://vt100.net/docs/tp83/appendixb.html
 * It's a vt100 implementation, that implements ANSI control function.
 */

#define CHILD 0
#define SCROLLBACK 3

#define MASK_LNM     1
#define MASK_DECCKM  2
#define MASK_DECANM  4
#define MASK_DECCOLM 8
#define MASK_DECSCLM 16
#define MASK_DECSCNM 32
#define MASK_DECOM   64
#define MASK_DECAWM  128
#define MASK_DECARM  256
#define MASK_DECINLM 512

#define LNM     20
#define DECCKM  1
#define DECANM  2
#define DECCOLM 3
#define DECSCLM 4
#define DECSCNM 5
#define DECOM   6
#define DECAWM  7
#define DECARM  8
#define DECINLM 9


void my_putchar(char c)
{
    write(1, &c, 1);
}

void my_putstr(char *str)
{
    write(1, str, strlen(str));
}

void my_putnbr(int nb)
{
    if (nb >= 0)
        nb = -nb;
    else
        my_putchar('-');
    if (nb <= -10)
        my_putnbr(-nb / 10);
    my_putchar(-(nb % 10) + '0');
}

int	my_putnbr_base(int nbr, char *base)
{
    int len;
    int neg_flag;

    len = -1;
    neg_flag = 1;
    while (*(base + ++len));
    if (len < 2)
        return (-1);
    if (nbr < 0)
    {
        my_putchar('-');
        neg_flag = -1;
    }
    if (nbr / len)
        my_putnbr_base((nbr / len) * neg_flag, base);
    my_putchar(base[(nbr % len) * neg_flag]);
    return (nbr);
}

unsigned int get_mode_mask(unsigned int mode)
{
    switch (mode)
    {
    case LNM     : return MASK_LNM;
    case DECCKM  : return MASK_DECCKM;
    case DECANM  : return MASK_DECANM;
    case DECCOLM : return MASK_DECCOLM;
    case DECSCLM : return MASK_DECSCLM;
    case DECSCNM : return MASK_DECSCNM;
    case DECOM   : return MASK_DECOM;
    case DECAWM  : return MASK_DECAWM;
    case DECARM  : return MASK_DECARM;
    case DECINLM : return MASK_DECINLM;
    default:       return 0;
    }
}

#define SET_MODE(vt100, mode) ((vt100)->modes |= get_mode_mask(mode))
#define UNSET_MODE(vt100, mode) ((vt100)->modes &= ~get_mode_mask(mode))
#define MODE_IS_SET(vt100, mode) ((vt100)->modes & get_mode_mask(mode))

/*
** frozen_screen is the frozen part of the screen
** when margins are set.
** The top of the frozen_screen holds the top margin
** while the bottom holds the bottom margin.
*/
struct vt100_term
{
    unsigned int width;
    unsigned int height;
    unsigned int x;
    unsigned int y;
    unsigned int saved_x;
    unsigned int saved_y;
    unsigned int margin_top;
    unsigned int margin_bottom;
    unsigned int top_line; /* Line at the top of the display */
    char         *screen;
    char         *frozen_screen;
    char         *tabulations;
    unsigned int selected_charset;
    unsigned int modes;
};

/*
Modes
=====

The following is a list of VT100 modes which may be changed with set
mode (SM) and reset mode (RM) controls.

ANSI Specified Modes
--------------------

Parameter    Mode Mnemonic    Mode Function
0                             Error (ignored)
20           LNM              Line feed new line mode


DEC Private Modes
=================
If the first character in the parameter string is ? (077), the
parameters are interpreted as DEC private parameters according to the
following:

Parameter    Mode Mnemonic    Mode Function
0                             Error (ignored)
1            DECCKM           Cursor key
2            DECANM           ANSI/VT52
3            DECCOLM          Column
4            DECSCLM          Scrolling
5            DECSCNM          Screen
6            DECOM            Origin
7            DECAWM           Auto wrap
8            DECARM           Auto repeating
9            DECINLM          Interlace

LNM – Line Feed/New Line Mode
-----------------------------
This is a parameter applicable to set mode (SM) and reset mode (RM)
control sequences. The reset state causes the interpretation of the
line feed (LF), defined in ANSI Standard X3.4-1977, to imply only
vertical movement of the active position and causes the RETURN key
(CR) to send the single code CR. The set state causes the LF to imply
movement to the first position of the following line and causes the
RETURN key to send the two codes (CR, LF). This is the New Line (NL)
option.

This mode does not affect the index (IND), or next line (NEL) format
effectors.

DECCKM – Cursor Keys Mode (DEC Private)
---------------------------------------
This is a private parameter applicable to set mode (SM) and reset mode
(RM) control sequences. This mode is only effective when the terminal
is in keypad application mode (see DECKPAM) and the ANSI/VT52 mode
(DECANM) is set (see DECANM). Under these conditions, if the cursor
key mode is reset, the four cursor function keys will send ANSI cursor
control commands. If cursor key mode is set, the four cursor function
keys will send application functions.

DECANM – ANSI/VT52 Mode (DEC Private)
-------------------------------------
This is a private parameter applicable to set mode (SM) and reset mode
(RM) control sequences. The reset state causes only VT52 compatible
escape sequences to be interpreted and executed. The set state causes
only ANSI "compatible" escape and control sequences to be interpreted
and executed.

DECCOLM – Column Mode (DEC Private)
-----------------------------------
This is a private parameter applicable to set mode (SM) and reset mode
(RM) control sequences. The reset state causes a maximum of 80 columns
on the screen. The set state causes a maximum of 132 columns on the
screen.

DECSCLM – Scrolling Mode (DEC Private)
--------------------------------------
This is a private parameter applicable to set mode (SM) and reset mode
(RM) control sequences. The reset state causes scrolls to "jump"
instantaneously. The set state causes scrolls to be "smooth" at a
maximum rate of six lines per second.

DECSCNM – Screen Mode (DEC Private)
-----------------------------------
This is a private parameter applicable to set mode (SM) and reset mode
(RM) control sequences. The reset state causes the screen to be black
with white characters. The set state causes the screen to be white
with black characters.

DECOM – Origin Mode (DEC Private)
---------------------------------
This is a private parameter applicable to set mode (SM) and reset mode
(RM) control sequences. The reset state causes the origin to be at the
upper-left character position on the screen. Line and column numbers
are, therefore, independent of current margin settings. The cursor may
be positioned outside the margins with a cursor position (CUP) or
horizontal and vertical position (HVP) control.

The set state causes the origin to be at the upper-left character
position within the margins. Line and column numbers are therefore
relative to the current margin settings. The cursor is not allowed to
be positioned outside the margins.

The cursor is moved to the new home position when this mode is set or
reset.

Lines and columns are numbered consecutively, with the origin being
line 1, column 1.

DECAWM – Autowrap Mode (DEC Private)
------------------------------------
This is a private parameter applicable to set mode (SM) and reset mode
(RM) control sequences. The reset state causes any displayable
characters received when the cursor is at the right margin to replace
any previous characters there. The set state causes these characters
to advance to the start of the next line, doing a scroll up if
required and permitted.

DECARM – Auto Repeat Mode (DEC Private)
---------------------------------------
This is a private parameter applicable to set mode (SM) and reset mode
(RM) control sequences. The reset state causes no keyboard keys to
auto-repeat. The set state causes certain keyboard keys to auto-repeat.

DECINLM – Interlace Mode (DEC Private)
--------------------------------------
This is a private parameter applicable to set mode (SM) and reset mode
(RM) control sequences. The reset state (non-interlace) causes the video
processor to display 240 scan lines per frame. The set state (interlace)
causes the video processor to display 480 scan lines per frame. There is
no increase in character resolution.
*/

#define SCREEN_PTR(vt100, x, y) \
    ((vt100->top_line * vt100->width + x + vt100->width * y) \
    % (vt100->width * SCROLLBACK * vt100->height))

#define FROZEN_SCREEN_PTR(vt100, x, y)                     \
    ((x + vt100->width * y) \
    % (vt100->width * SCROLLBACK * vt100->height))

void set(struct vt100_term *headless_term,
         unsigned int x, unsigned int y,
         char c)
{
    if (y < headless_term->margin_top || y > headless_term->margin_bottom)
        headless_term->frozen_screen[FROZEN_SCREEN_PTR(headless_term, x, y)] = c;
    else
        headless_term->screen[SCREEN_PTR(headless_term, x, y)] = c;
}


char get(struct vt100_term *vt100, unsigned int x, unsigned int y)
{
    if (y < vt100->margin_top || y > vt100->margin_bottom)
        return vt100->frozen_screen[FROZEN_SCREEN_PTR(vt100, x, y)];
    else
        return vt100->screen[SCREEN_PTR(vt100, x, y)];
}

void froze_line(struct vt100_term *vt100, unsigned int y)
{
    my_putstr("Frozing line ");
    my_putnbr(y);
    my_putchar('\n');
    memcpy(vt100->frozen_screen + vt100->width * y,
           vt100->screen + SCREEN_PTR(vt100, 0, y),
           vt100->width);
}

void unfroze_line(struct vt100_term *vt100, unsigned int y)
{
    my_putstr("Unfrozing line ");
    my_putnbr(y);
    my_putchar('\n');
    memcpy(vt100->screen + SCREEN_PTR(vt100, 0, y),
           vt100->frozen_screen + vt100->width * y,
           vt100->width);
}

void blank_screen(struct vt100_term *vt100_term)
{
    unsigned int x;
    unsigned int y;

    for (x = 0; x < vt100_term->width; ++x)
        for (y = 0; y < vt100_term->height; ++y)
            set(vt100_term, x, y, '\0');
}

void my_strdump(char *str)
{
    while (*str != '\0')
    {
        if (*str >= ' ' && *str <= '~')
        {
            write(1, str, 1);
        }
        else
        {
            write(1, "\\0", 2);
            my_putnbr_base(*str, "01234567");
        }
        str += 1;
    }
    write(1, "\n", 1);
}

void disp(struct vt100_term *vt100)
{
    char c;
    unsigned int x;
    unsigned int y;

    write(1, "\n", 1);
    for (x = 0; x < vt100->width + 1; ++x)
        write(1, "-", 1);
    write(1, "\n", 1);
    for (y = 0; y < vt100->height; ++y)
    {
        if (y < vt100->margin_top || y > vt100->margin_bottom)
            write(1, "#", 1);
        else
            write(1, "|", 1);
        for (x = 0; x < vt100->width; ++x)
        {
            if (x == vt100->x && y == vt100->y)
                write(1, "\033[7m", 4);
            c = get(vt100, x, y);
            if (c == '\0')
                c = ' ';
            if (c > 31)
            {
                write(1, &c, 1);
            }
            else
            {
                my_putstr("Don't know how to print char ");
                my_putnbr_base((int)c, "01234567");
                my_putchar('\n');
                exit(EXIT_FAILURE);
            }
            if (x == vt100->x && y == vt100->y)
                write(1, "\033[0m", 4);
        }
        write(1, "|", 1);
        write(1, "\n", 1);
    }
    for (x = 0; x < vt100->width + 2; ++x)
        write(1, "-", 1);
    write(1, "\n\n", 2);
}

void dump(char *title, struct term_emul *term_emul,
          struct vt100_term *vt100)
{
    unsigned int argc;

    write(1, title, strlen(title));
    write(1, " ", 1);
    for (argc = 0; argc < term_emul->argc; ++argc)
    {
        my_putnbr(term_emul->argv[argc]);
        if (argc != term_emul->argc - 1)
            write(1, ", ", 2);
    }
    write(1, "\n", 1);
    /* disp(vt100); */
}

/*
DECSC – Save Cursor (DEC Private)

ESC 7

This sequence causes the cursor position, graphic rendition, and
character set to be saved. (See DECRC).
*/
void DECSC(struct term_emul *term_emul)
{
    /*TODO: Save graphic rendition and charset.*/
    struct vt100_term *vt100;

    vt100 = (struct vt100_term *)term_emul->user_data;
    vt100->saved_x = vt100->x;
    vt100->saved_y = vt100->y;
    dump("DECSC", term_emul, vt100);
}

/*
RM – Reset Mode

ESC [ Ps ; Ps ; . . . ; Ps l

Resets one or more VT100 modes as specified by each selective
parameter in the parameter string. Each mode to be reset is specified
by a separate parameter. [See Set Mode (SM) control sequence]. (See
Modes following this section).

*/
void RM(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    unsigned int mode;

    vt100 = (struct vt100_term *)term_emul->user_data;
    if (term_emul->argc > 0)
    {
        mode = term_emul->argv[0];
        if (mode == DECCOLM)
        {
            vt100->width = 80;
            vt100->x = vt100->y = 0;
            blank_screen(vt100);
        }
        UNSET_MODE(vt100, mode);
    }
    dump("RM", term_emul, vt100);
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
void CUP(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    int arg0;
    int arg1;

    vt100 = (struct vt100_term *)term_emul->user_data;
    arg0 = 0;
    arg1 = 0;
    if (term_emul->argc > 0)
        arg0 = term_emul->argv[0] - 1;
    if (term_emul->argc > 1)
        arg1 = term_emul->argv[1] - 1;
    if (arg0 < 0)
        arg0 = 0;
    if (arg1 < 0)
        arg1 = 0;

    if (MODE_IS_SET(vt100, DECOM))
    {
        arg0 += vt100->margin_top;
        if ((unsigned int)arg0 > vt100->margin_bottom)
            arg0 = vt100->margin_bottom;
    }
    vt100->y = arg0;
    vt100->x = arg1;
    dump("CUP", term_emul, vt100);
}

/*
SM – Set Mode

ESC [ Ps ; . . . ; Ps h

Causes one or more modes to be set within the VT100 as specified by
each selective parameter in the parameter string. Each mode to be set
is specified by a separate parameter. A mode is considered set until
it is reset by a reset mode (RM) control sequence.

*/
void SM(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    unsigned int mode;
    unsigned int saved_argc;

    vt100 = (struct vt100_term *)term_emul->user_data;
    if (term_emul->argc > 0)
    {
        mode = term_emul->argv[0];
        SET_MODE(vt100, mode);
        if (mode == DECANM)
        {
            write(2, "TODO: Support vt52 mode\n", 24);
            exit(EXIT_FAILURE);
        }
        if (mode == DECCOLM)
        {
            vt100->width = 132;
            vt100->x = vt100->y = 0;
            blank_screen(vt100);
        }
        if (mode == DECOM)
        {
            saved_argc = term_emul->argc;
            term_emul->argc = 0;
            CUP(term_emul);
            term_emul->argc = saved_argc;
        }
    }
    dump("SM", term_emul, vt100);
}

/*
DECSTBM – Set Top and Bottom Margins (DEC Private)

ESC [ Pn; Pn r

This sequence sets the top and bottom margins to define the scrolling
region. The first parameter is the line number of the first line in
the scrolling region; the second parameter is the line number of the
bottom line in the scrolling region. Default is the entire screen (no
margins). The minimum size of the scrolling region allowed is two
lines, i.e., the top margin must be less than the bottom margin. The
cursor is placed in the home position (see Origin Mode DECOM).

*/
void DECSTBM(struct term_emul *term_emul)
{
    unsigned int margin_top;
    unsigned int margin_bottom;
    struct vt100_term *vt100;
    unsigned int line;

    vt100 = (struct vt100_term *)term_emul->user_data;

    if (term_emul->argc == 2)
    {
        margin_top = term_emul->argv[0] - 1;
        margin_bottom = term_emul->argv[1] - 1;
        if (margin_bottom >= vt100->height)
            return ;
        if (margin_bottom - margin_top <= 0)
            return ;
    }
    else
    {
        margin_top = 0;
        margin_bottom = vt100->height - 1;
    }
    for (line = vt100->margin_top; line < margin_top; ++line)
        froze_line(vt100, line);
    for (line = vt100->margin_bottom; line < margin_bottom; ++line)
        unfroze_line(vt100, line);
    for (line = margin_top; line < vt100->margin_top; ++line)
        unfroze_line(vt100, line);
    for (line = margin_bottom; line < vt100->margin_bottom; ++line)
        froze_line(vt100, line);
    vt100->margin_bottom = margin_bottom;
    vt100->margin_top = margin_top;
    dump("DECSTBM", term_emul, vt100);
    term_emul->argc = 0;
    CUP(term_emul);
}

/*
SGR – Select Graphic Rendition

ESC [ Ps ; . . . ; Ps m

Invoke the graphic rendition specified by the parameter(s). All
following characters transmitted to the VT100 are rendered according
to the parameter(s) until the next occurrence of SGR. Format Effector

Parameter    Parameter Meaning
0            Attributes off
1            Bold or increased intensity
4            Underscore
5            Blink
7            Negative (reverse) image

All other parameter values are ignored.

With the Advanced Video Option, only one type of character attribute
is possible as determined by the cursor selection; in that case
specifying either the underscore or the reverse attribute will
activate the currently selected attribute. (See cursor selection in
Chapter 1).
*/
void SGR(struct term_emul *term_emul)
{
    term_emul = term_emul;
    /* Just ignore them for now, we are rendering pure text only */
}

/*
DA – Device Attributes

ESC [ Pn c


The host requests the VT100 to send a device attributes (DA) control
sequence to identify itself by sending the DA control sequence with
either no parameter or a parameter of 0.  Response to the request
described above (VT100 to host) is generated by the VT100 as a DA
control sequence with the numeric parameters as follows:

Option Present              Sequence Sent
No options                  ESC [?1;0c
Processor option (STP)      ESC [?1;1c
Advanced video option (AVO) ESC [?1;2c
AVO and STP                 ESC [?1;3c
Graphics option (GPO)       ESC [?1;4c
GPO and STP                 ESC [?1;5c
GPO and AVO                 ESC [?1;6c
GPO, STP and AVO            ESC [?1;7c

*/
void DA(struct term_emul *term_emul)
{
    struct vt100_term *vt100;

    vt100 = (struct vt100_term *)term_emul->user_data;
    write(term_emul->fd, "\033[?1;0c", 7);
}

/*
DECRC – Restore Cursor (DEC Private)

ESC 8

This sequence causes the previously saved cursor position, graphic
rendition, and character set to be restored.
*/
void DECRC(struct term_emul *term_emul)
{
    /*TODO Save graphic rendition and charset */
    struct vt100_term *vt100;

    vt100 = (struct vt100_term *)term_emul->user_data;
    vt100->x = vt100->saved_x;
    vt100->y = vt100->saved_y;
    dump("DECRC", term_emul, vt100);
}

/*
DECALN – Screen Alignment Display (DEC Private)

ESC # 8

This command fills the entire screen area with uppercase Es for screen
focus and alignment. This command is used by DEC manufacturing and
Field Service personnel.
*/
void DECALN(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    unsigned int x;
    unsigned int y;

    vt100 = (struct vt100_term *)term_emul->user_data;
    for (x = 0; x < vt100->width; ++x)
        for (y = 0; y < vt100->height; ++y)
            set(vt100, x, y, 'E');
    dump("DEALN", term_emul, vt100);
}

/*
IND – Index

ESC D

This sequence causes the active position to move downward one line
without changing the column position. If the active position is at the
bottom margin, a scroll up is performed. Format Effector
*/
void IND(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    unsigned int x;

    vt100 = (struct vt100_term *)term_emul->user_data;
    if (vt100->y >= vt100->margin_bottom)
    {
        /* SCROLL */
        vt100->top_line = (vt100->top_line + 1) % (vt100->height * SCROLLBACK);
        for (x = 0; x < vt100->width; ++x)
         set(vt100, x, vt100->margin_bottom, '\0');

    }
    else
    {
        /* Do not scroll, just move downward on the current display space */
        vt100->y += 1;
    }
    dump("IND", term_emul, vt100);
}
/*
RI – Reverse Index

ESC M

Move the active position to the same horizontal position on the
preceding line. If the active position is at the top margin, a scroll
down is performed. Format Effector
*/
void RI(struct term_emul *term_emul)
{
    struct vt100_term *vt100;

    vt100 = (struct vt100_term *)term_emul->user_data;
    if (vt100->y == 0)
    {
        /* SCROLL */
        vt100->top_line = (vt100->top_line - 1) % (vt100->height * SCROLLBACK);
    }
    else
    {
        /* Do not scroll, just move upward on the current display space */
        vt100->y -= 1;
    }
    dump("RI", term_emul, vt100);
}

/*
NEL – Next Line

ESC E

This sequence causes the active position to move to the first position
on the next line downward. If the active position is at the bottom
margin, a scroll up is performed. Format Effector
*/
void NEL(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    unsigned int x;

    vt100 = (struct vt100_term *)term_emul->user_data;
    if (vt100->y >= vt100->margin_bottom)
    {
        /* SCROLL */
        vt100->top_line = (vt100->top_line + 1) % (vt100->height * SCROLLBACK);
        for (x = 0; x < vt100->width; ++x)
            set(vt100, x, vt100->margin_bottom, '\0');
    }
    else
    {
        /* Do not scroll, just move downward on the current display space */
        vt100->y += 1;
    }
    vt100->x = 0;
    dump("NEL", term_emul, vt100);
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
void CUU(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    unsigned int arg0;

    vt100 = (struct vt100_term *)term_emul->user_data;
    arg0 = 1;
    if (term_emul->argc > 0)
        arg0 = term_emul->argv[0];
    if (arg0 == 0)
        arg0 = 1;
    if (arg0 <= vt100->y)
        vt100->y -= arg0;
    else
        vt100->y = 0;
    dump("CUU", term_emul, vt100);
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
void CUD(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    unsigned int arg0;

    vt100 = (struct vt100_term *)term_emul->user_data;
    arg0 = 1;
    if (term_emul->argc > 0)
        arg0 = term_emul->argv[0];
    if (arg0 == 0)
        arg0 = 1;
    vt100->y += arg0;
    if (vt100->y >= vt100->height)
        vt100->y = vt100->height - 1;
    dump("CUD", term_emul, vt100);
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
void CUF(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    unsigned int arg0;

    vt100 = (struct vt100_term *)term_emul->user_data;
    arg0 = 1;
    if (term_emul->argc > 0)
        arg0 = term_emul->argv[0];
    if (arg0 == 0)
        arg0 = 1;
    vt100->x += arg0;
    if (vt100->x >= vt100->width)
        vt100->x = vt100->width - 1;
    dump("CUF", term_emul, vt100);
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
void CUB(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    unsigned int arg0;

    vt100 = (struct vt100_term *)term_emul->user_data;
    arg0 = 1;
    if (term_emul->argc > 0)
        arg0 = term_emul->argv[0];
    if (arg0 == 0)
        arg0 = 1;
    if (arg0 < vt100->x)
        vt100->x -= arg0;
    else
        vt100->x = 0;
    dump("CUB", term_emul, vt100);
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
void ED(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    unsigned int arg0;
    unsigned int x;
    unsigned int y;

    vt100 = (struct vt100_term *)term_emul->user_data;
    arg0 = 0;
    if (term_emul->argc > 0)
        arg0 = term_emul->argv[0];
    if (arg0 == 0)
    {
        for (x = vt100->x; x < vt100->width; ++x)
            set(vt100, x, vt100->y, '\0');
        for (x = 0 ; x < vt100->width; ++x)
            for (y = vt100->y + 1; y < vt100->height; ++y)
                set(vt100, x, y, '\0');
    }
    else if (arg0 == 1)
    {
        for (x = 0 ; x < vt100->width; ++x)
            for (y = 0; y < vt100->y; ++y)
                set(vt100, x, y, '\0');
        for (x = 0; x <= vt100->x; ++x)
            set(vt100, x, vt100->y, '\0');
    }
    else if (arg0 == 2)
    {
        for (x = 0 ; x < vt100->width; ++x)
            for (y = 0; y < vt100->height; ++y)
                set(vt100, x, y, '\0');
    }
    dump("ED", term_emul, vt100);
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
void EL(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    unsigned int arg0;
    unsigned int x;

    vt100 = (struct vt100_term *)term_emul->user_data;
    arg0 = 0;
    if (term_emul->argc > 0)
        arg0 = term_emul->argv[0];
    if (arg0 == 0)
    {
        for (x = vt100->x; x < vt100->width; ++x)
            set(vt100, x, vt100->y, '\0');
    }
    else if (arg0 == 1)
    {
        for (x = 0; x <= vt100->x; ++x)
            set(vt100, x, vt100->y, '\0');
    }
    else if (arg0 == 2)
    {
        for (x = 0; x < vt100->width; ++x)
            set(vt100, x, vt100->y, '\0');
    }
    dump("EL", term_emul, vt100);
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
void HVP(struct term_emul *term_emul)
{
    CUP(term_emul);
}

void TBC(struct term_emul *term_emul)
{
    struct vt100_term *vt100;
    unsigned int i;

    vt100 = (struct vt100_term *)term_emul->user_data;
    if (term_emul->argc == 0 || term_emul->argv[0] == 0)
    {
        vt100->tabulations[vt100->x] = '-';
    }
    else if (term_emul->argc == 1 && term_emul->argv[0] == 3)
    {
        for (i = 0; i < 132; ++i)
            vt100->tabulations[i] = '-';
    }
    dump("TBC", term_emul, vt100);
    write(1, vt100->tabulations, 132);
    my_putchar('\n');
}

void HTS(struct term_emul *term_emul)
{
    struct vt100_term *vt100;

    vt100 = (struct vt100_term *)term_emul->user_data;
    vt100->tabulations[vt100->x] = '|';
    dump("HTS", term_emul, vt100);
    my_putstr(vt100->tabulations);
    my_putchar('\n');
}

static void vt100_write(struct term_emul *term_emul, char c __attribute__((unused)))
{
    struct vt100_term *vt100;

    vt100 = (struct vt100_term *)term_emul->user_data;
    my_putstr("Writing(\\");
    my_putnbr_base((int)c, "01234567");
    if (c > ' ')
    {
        my_putchar('[');
        my_putchar(c);
        my_putchar(']');
    }
    my_putstr(")\n");
    if (c == '\r')
    {
        vt100->x = 0;
        return ;
    }
    if (c == '\n' || c == '\013' || c == '\014')
    {
        if (MODE_IS_SET(vt100, LNM))
            NEL(term_emul);
        else
            IND(term_emul);
        return ;
    }
    if (c == '\010' && vt100->x > 0)
    {
        if (vt100->x == vt100->width)
            vt100->x -= 1;
        vt100->x -= 1;
        return ;
    }
    if (c == '\t')
    {
        do
        {
            set(vt100, vt100->x, vt100->y, ' ');
            vt100->x += 1;
        } while (vt100->x < vt100->width && vt100->tabulations[vt100->x] == '-');
        return ;
    }
    if (c == '\016')
    {
        vt100->selected_charset = 0;
        return ;
    }
    if (c == '\017')
    {
        vt100->selected_charset = 1;
        return ;
    }
    if (vt100->x == vt100->width)
    {
        if (MODE_IS_SET(vt100, DECAWM))
        {
            my_putstr("DECAWM is set, so we auto wrap\n");
            NEL(term_emul);
        }
        else
        {
            my_putstr("DECAWM is not set, so we're not moving\n");
            vt100->x -= 1;
        }
    }
    my_putstr("To ");
    my_putnbr(vt100->x); my_putstr(", "); my_putnbr(vt100->y);
    my_putstr("\n");
    set(vt100, vt100->x, vt100->y, c);
    vt100->x += 1;
    my_putstr("Cursor is now at ");
    my_putnbr(vt100->x); my_putstr(", "); my_putnbr(vt100->y);
    my_putstr("\n");
    dump("WRITE", term_emul, vt100);
}

int main_loop(struct term_emul *term_emul, int master)
{
    char buffer[4096];
    fd_set rfds;
    int retval;
    ssize_t read_size;

    while (42)
    {
        FD_ZERO(&rfds);
        FD_SET(master, &rfds);
        FD_SET(0, &rfds);
        retval = select(master + 1, &rfds, NULL, NULL, NULL);
        if (retval == -1)
        {
            perror("select()");
        }
        if (FD_ISSET(0, &rfds))
        {
            read_size = read(0, &buffer, 4096);
            if (read_size == -1)
            {
                perror("read");
                return EXIT_FAILURE;
            }
            buffer[read_size] = '\0';
            write(master, buffer, read_size);
        }
        if (FD_ISSET(master, &rfds))
        {
            read_size = read(master, &buffer, 4096);
            if (read_size == -1)
            {
                perror("read");
                return EXIT_FAILURE;
            }
            buffer[read_size] = '\0';
            my_strdump(buffer);
            term_read_str(term_emul, buffer);
            disp(term_emul->user_data);
        }
    }
}

struct termios backup;
void set_non_canonical(int fd)
{
    struct termios termios;
    ioctl(fd, TCGETS, &backup);
    ioctl(fd, TCGETS, &termios);
    termios.c_iflag |= ICANON;
    termios.c_cc[VMIN] = 1;
    termios.c_cc[VTIME] = 0;
    ioctl(fd, TCSETS, &termios);
}

void restore_termios(int fd)
{
    ioctl(fd, TCSETS, &backup);
}

void unimplemented(struct term_emul* term_emul, char *seq, char chr)
{
    unsigned int argc;

    write(1, "UNIMPLEMENTED ", 14);
    write(1, seq, strlen(seq));
    write(1, "(", 1);
    for (argc = 0; argc < term_emul->argc; ++argc)
    {
        my_putnbr(term_emul->argv[argc]);
        if (argc != term_emul->argc - 1)
            write(1, ", ", 2);
    }
    write(1, ")\\0", 1);
    my_putnbr_base(chr, "01234567");
    write(1, "\n", 1);
}

struct term_emul *vt100(void)
{
    struct term_emul *term;
    struct vt100_term *vt100;

    vt100 = calloc(1, sizeof(*vt100));
    if (vt100 == NULL)
        return NULL;
    vt100->height = 24;
    vt100->width = 80;
    vt100->screen = calloc(132 * SCROLLBACK * vt100->height,
                          sizeof(*vt100->screen));
    vt100->frozen_screen = calloc(132 * vt100->height,
                                 sizeof(*vt100->frozen_screen));
    vt100->tabulations = malloc(132);
    if (vt100->tabulations == NULL)
        return NULL; /* Need to free before returning ... */
    vt100->margin_top = 0;
    vt100->margin_bottom = vt100->height - 1;
    vt100->selected_charset = 0;
    vt100->x = 0;
    vt100->y = 0;
    vt100->modes = MASK_DECANM;
    vt100->top_line = 0;
    term = term_init(80, 24, vt100_write);
    term->callbacks.csi.f = HVP;
    term->callbacks.csi.K = EL;
    term->callbacks.csi.c = DA;
    term->callbacks.csi.h = SM;
    term->callbacks.csi.l = RM;
    term->callbacks.csi.J = ED;
    term->callbacks.csi.H = CUP;
    term->callbacks.csi.C = CUF;
    term->callbacks.csi.B = CUD;
    term->callbacks.csi.r = DECSTBM;
    term->callbacks.csi.m = SGR;
    term->callbacks.csi.A = CUU;
    term->callbacks.csi.g = TBC;
    term->callbacks.esc.H = HTS;
    term->callbacks.csi.D = CUB;
    term->callbacks.esc.E = NEL;
    term->callbacks.esc.D = IND;
    term->callbacks.esc.M = RI;
    term->callbacks.esc.n8 = DECRC;
    term->callbacks.esc.n7 = DECSC;
    term->callbacks.hash.n8 = DECALN;
    term->user_data = vt100;
    term->unimplemented = unimplemented;
    return term;
}

int main(int ac, char **av)
{
    int child;
    int master;
    struct winsize winsize;
    struct term_emul *term;

    if (ac == 1)
    {
        puts("Usage: test PROGNAME");
        return EXIT_FAILURE;
    }
    set_non_canonical(0);
    winsize.ws_row = 24;
    winsize.ws_col = 24;
    child = forkpty(&master, NULL, NULL, NULL);
    if (child == CHILD)
    {
        setsid();
        putenv("TERM=vt100");
        execvp(av[1], av + 1);
        return EXIT_SUCCESS;
    }
    else
    {
        term = vt100();
        ioctl(master, TIOCSWINSZ, &winsize);
        term->fd = master;
        main_loop(term, master);
    }
    restore_termios(0);
    return EXIT_SUCCESS;
}
