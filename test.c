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
    /* disp(term); */
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
        term->x = 0;
        vt100->argc = 0;
        NEL(vt100);
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
    dump("WRITE", vt100, term);
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
    int master;
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
    terminal.screen = calloc(terminal.width * SCROLLBACK * terminal.height,
                             sizeof(*terminal.screen));
    terminal.x = 0;
    terminal.y = 0;
    terminal.top_line = 0;
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
        callbacks.esc.RI = (vt100_action)RI;
        callbacks.esc.DECRC = (vt100_action)DECRC;
        callbacks.esc.DECSC = (vt100_action)DECSC;
        callbacks.hash.DECALN = (vt100_action)DECALN;

        vt100 = vt100_init(80, 24, &callbacks, vt100_write);
        vt100->user_data = &terminal;
        ioctl(master, TIOCSWINSZ, &winsize);
        main_loop(vt100, master);
        vt100->unimplemented = unimplemented;
    }
    restore_termios(0);
    return EXIT_SUCCESS;
}
