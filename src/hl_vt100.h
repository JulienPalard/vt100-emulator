#ifndef __VT100_HEADLESS_H__
#define __VT100_HEADLESS_H__

#define CHILD 0

#include <utmp.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include "lw_terminal_vt100.h"

struct vt100_headless
{
    int master;
    struct termios backup;
    struct lw_terminal_vt100 *term;
    int should_quit;
    void (*changed)(struct vt100_headless *this);
};


void vt100_headless_fork(struct vt100_headless *this, const char *progname, char **argv);
int vt100_headless_main_loop(struct vt100_headless *this);
void delete_vt100_headless(struct vt100_headless *this);
struct vt100_headless *new_vt100_headless(void);
const char **vt100_headless_getlines(struct vt100_headless *this);
void vt100_headless_stop(struct vt100_headless *this);

#endif
