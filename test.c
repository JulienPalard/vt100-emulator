#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>
#include <string.h>
#include <pty.h>
#include <stdio.h>
#include "vt100.h"

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

void disp(struct vt100_term *vt100)
{
    unsigned int y;
    const char **lines;

    lines = vt100_dump(vt100);
    for (y = 0; y < vt100->height; ++y)
    {
        write(1, lines[y], vt100->width);
        write(1, "\n", 1);
    }
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
        term = vt100_init(unimplemented);
        ioctl(master, TIOCSWINSZ, &winsize);
        term->fd = master;
        main_loop(term, master);
    }
    restore_termios(0);
    return EXIT_SUCCESS;
}
