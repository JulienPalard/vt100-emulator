#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>
#include <string.h>
#include <pty.h>
#include <stdio.h>
#include "vt100.h"

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

#define SET_MODE(term, mode) ((term)->modes |= get_mode_mask(mode))
#define UNSET_MODE(term, mode) ((term)->modes &= ~get_mode_mask(mode))
#define MODE_IS_SET(term, mode) ((term)->modes & get_mode_mask(mode))

struct headless_terminal
{
    unsigned int width;
    unsigned int height;
    unsigned int x;
    unsigned int y;
    unsigned int saved_x;
    unsigned int saved_y;
    unsigned int top_line; /* Line at the top of the display */
    int          master;
    char         *screen;
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

void set(struct headless_terminal *term, unsigned int x, unsigned int y, char c)
{
    term->screen[(term->top_line * term->width + x + term->width * y)
                 % (term->width * SCROLLBACK * term->height)] = c;
}

char get(struct headless_terminal *term, unsigned int x, unsigned int y)
{
    return term->screen[(term->top_line * term->width + x + term->width * y)
                        % (term->width * SCROLLBACK * term->height)];
}

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

void disp(struct headless_terminal *term)
{
    char c;
    unsigned int x;
    unsigned int y;

    write(1, "\n", 1);
    for (x = 0; x < term->width + 1; ++x)
        write(1, "-", 1);
    write(1, "\n", 1);
    for (y = 0; y < term->height; ++y)
    {
        write(1, "|", 1);
        for (x = 0; x < term->width; ++x)
        {
            if (x == term->x && y == term->y)
                write(1, "X", 1);
            else
            {
                c = get(term, x, y);
                if (c == '\0')
                    c = ' ';
                if (c > 31 || c == '\t')
                    write(1, &c, 1);
                else
                {
                    fprintf(stderr,
                            "Don't know how to print char '%c' (%i)\n",
                            c, (int)c);
                    exit(EXIT_FAILURE);
                }
            }
        }
        write(1, "|", 1);
        write(1, "\n", 1);
    }
    for (x = 0; x < term->width + 2; ++x)
        write(1, "-", 1);
    write(1, "\n\n", 2);
}

void dump(char *title, struct vt100_emul *vt100,
          struct headless_terminal *term)
{
    unsigned int argc;

    write(1, title, strlen(title));
    write(1, " ", 1);
    for (argc = 0; argc < vt100->argc; ++argc)
    {
        my_putnbr(vt100->argv[argc]);
        if (argc != vt100->argc - 1)
            write(1, ", ", 2);
    }
    write(1, "\n", 1);
    disp(term);
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
    dump("DECSC", vt100, term);
}

/*
RM – Reset Mode

ESC [ Ps ; Ps ; . . . ; Ps l

Resets one or more VT100 modes as specified by each selective
parameter in the parameter string. Each mode to be reset is specified
by a separate parameter. [See Set Mode (SM) control sequence]. (See
Modes following this section).

*/
void RM(struct vt100_emul *vt100)
{
    struct headless_terminal *term;
    unsigned int mode;

    term = (struct headless_terminal *)vt100->user_data;
    if (vt100->argc > 0)
    {
        mode = vt100->argv[0];
        if (mode == DECCOLM)
        {
            term->width = 80;
        }
        UNSET_MODE(term, mode);
    }
}

/*
SM – Set Mode

ESC [ Ps ; . . . ; Ps h

Causes one or more modes to be set within the VT100 as specified by
each selective parameter in the parameter string. Each mode to be set
is specified by a separate parameter. A mode is considered set until
it is reset by a reset mode (RM) control sequence.

*/
void SM(struct vt100_emul *vt100)
{
    struct headless_terminal *term;
    unsigned int mode;

    term = (struct headless_terminal *)vt100->user_data;
    if (vt100->argc > 0)
    {
        mode = vt100->argv[0];
        if (mode == DECANM)
        {
            write(2, "TODO: Support vt52 mode\n", 24);
            exit(EXIT_FAILURE);
        }
        if (mode == DECCOLM)
        {
            term->width = 132;
        }
        SET_MODE(term, mode);
    }
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
void DA(struct vt100_emul *vt100)
{
    struct headless_terminal *term;

    term = (struct headless_terminal *)vt100->user_data;
    write(term->master, "\033[?1;0c", 7);
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
    dump("DECRC", vt100, term);
}

/*
DECALN – Screen Alignment Display (DEC Private)

ESC # 8

This command fills the entire screen area with uppercase Es for screen
focus and alignment. This command is used by DEC manufacturing and
Field Service personnel.
*/
void DECALN(struct vt100_emul *vt100)
{
    struct headless_terminal *term;
    unsigned int x;
    unsigned int y;

    term = (struct headless_terminal *)vt100->user_data;
    for (x = 0; x < term->width; ++x)
        for (y = 0; y < term->height; ++y)
            set(term, x, y, 'E');
    dump("DEALN", vt100, term);
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
    dump("IND", vt100, term);
}
/*
RI – Reverse Index

ESC M

Move the active position to the same horizontal position on the
preceding line. If the active position is at the top margin, a scroll
down is performed. Format Effector
*/
void RI(struct vt100_emul *vt100)
{
    struct headless_terminal *term;

    term = (struct headless_terminal *)vt100->user_data;
    if (term->y == 0)
    {
        /* SCROLL */
        term->top_line = (term->top_line - 1) % (term->height * SCROLLBACK);
    }
    else
    {
        /* Do not scroll, just move upward on the current display space */
        term->y -= 1;
    }
    dump("RI", vt100, term);
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
    dump("NEL", vt100, term);
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
    dump("CUU", vt100, term);
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
    dump("CUD", vt100, term);
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
    dump("CUF", vt100, term);
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
    dump("CUB", vt100, term);
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
    arg0 = 0;
    arg1 = 0;
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
    dump("CUP", vt100, term);
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
    dump("ED", vt100, term);
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
    dump("EL", vt100, term);
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
    if (c == '\r')
    {
        term->x = 0;
        return ;
    }
    if (c == '\n')
    {
        if (MODE_IS_SET(term, LNM))
            NEL(vt100);
        else
            IND(vt100);
        return ;
    }
    if (c == '\010' && term->x > 0)
    {
        term->x -= 1;
        return ;
    }
    set(term, term->x, term->y, c);
    term->x += 1;
    if (term->x == term->width + 1)
    {
        term->x = 0;
        term->y += 1;
    }
    my_putstr("Writing(");
    my_putchar(c);
    my_putstr(", ");
    my_putnbr((unsigned int)c);
    my_putstr(")\n");
}

int main_loop(struct vt100_emul *vt100, int master)
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
            vt100_read_str(vt100, buffer);
            disp(vt100->user_data);
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

void unimplemented(struct vt100_emul* vt100, char *seq, char chr)
{
    unsigned int argc;

    write(1, "UNIMPLEMENTED ", 14);
    write(1, seq, strlen(seq));
    write(1, "(", 1);
    for (argc = 0; argc < vt100->argc; ++argc)
    {
        my_putnbr(vt100->argv[argc]);
        if (argc != vt100->argc - 1)
            write(1, ", ", 2);
    }
    write(1, ")", 1);
    write(1, &chr, 1);
    write(1, "\n", 1);
}

int main(int ac, char **av)
{
    struct vt100_emul *vt100;
    struct vt100_callbacks callbacks;
    struct headless_terminal terminal;
    int child;
    struct winsize winsize;

    if (ac == 1)
    {
        puts("Usage : test PROGNAME");
        return EXIT_FAILURE;
    }
    set_non_canonical(0);
    memset(&callbacks, 0, sizeof(callbacks));
    winsize.ws_row = terminal.height = 24;
    winsize.ws_col = terminal.width = 80;
    terminal.screen = calloc(132 * SCROLLBACK * terminal.height,
                             sizeof(*terminal.screen));
    terminal.x = 0;
    terminal.y = 0;
    terminal.modes = MASK_DECANM;
    terminal.top_line = 0;
    child = forkpty(&terminal.master, NULL, NULL, NULL);
    if (child == CHILD)
    {
        setsid();
        putenv("TERM=vt100");
        execvp(av[1], av + 1);
        return EXIT_SUCCESS;
    }
    else
    {
        callbacks.csi.HVP = (vt100_action)HVP;
        callbacks.csi.EL = (vt100_action)EL;
        callbacks.csi.DA = (vt100_action)DA;
        callbacks.csi.ED = (vt100_action)ED;
        callbacks.csi.CUP = (vt100_action)CUP;
        callbacks.csi.CUF = (vt100_action)CUF;
        callbacks.csi.RM = (vt100_action)RM;
        callbacks.csi.CUD = (vt100_action)CUD;
        callbacks.csi.CUU = (vt100_action)CUU;
        callbacks.csi.CUB = (vt100_action)CUB;
        callbacks.esc.NEL = (vt100_action)NEL;
        callbacks.esc.IND = (vt100_action)IND;
        callbacks.esc.RI = (vt100_action)RI;
        callbacks.esc.DECRC = (vt100_action)DECRC;
        callbacks.esc.DECSC = (vt100_action)DECSC;
        callbacks.hash.DECALN = (vt100_action)DECALN;

        vt100 = vt100_init(80, 24, &callbacks, vt100_write);
        vt100->user_data = &terminal;
        vt100->unimplemented = unimplemented;
        ioctl(terminal.master, TIOCSWINSZ, &winsize);
        main_loop(vt100, terminal.master);
    }
    restore_termios(0);
    return EXIT_SUCCESS;
}
