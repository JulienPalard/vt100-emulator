#ifndef __VT100_H__
#define __VT100_H__

#include "term.h"

/*
 * Source : http://vt100.net/docs/vt100-ug/chapter3.html
            http://vt100.net/docs/tp83/appendixb.html
 * It's a vt100 implementation, that implements ANSI control function.
 */

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
    char         *lines[80];
};

struct terminal *vt100_init(void (*unimplemented)(struct terminal* term_emul, char *seq, char chr));
char get(struct vt100_term *vt100, unsigned int x, unsigned int y);
const char **vt100_dump(struct vt100_term *vt100);
#endif
