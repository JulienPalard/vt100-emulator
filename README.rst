INSTALL
=======

Python module
-------------

Run :
$ make python_module && su -c 'python setup.py install'

Overview
========

lw_terminal_parser, lw_terminal_vt100, and hl_vt100 are three modules used to emulate a vt100 terminal::

                                  -------------
                                  |           |
                                  | Your Code |
                                  |           |
                                  -------------
                                    |      ^
 vt100 = vt100_headless_init()      |      |
 vt100->changed = changed;          |      | hl_vt100 raises 'changed'
 vt100_headless_fork(vt100, ...     |      | when the screen has changed.
                                    |      | You get the content of the screen
                                    |      | calling vt100_headless_getlines.
                                    V      |
                                  -------------              -------------
 Read from PTY master and write | |           |     PTY      |           |
 to lw_terminal_vt100_read_str  | |  hl_vt100 |<------------>|  Program  |
                                V |           |Master   Slave|           |
                                  -------------              -------------
                                   |        |^ hl_vt100 gets lw_terminal_vt100's
                                   |        || lines by calling
                                   |        || lw_terminal_vt100_getlines
                                   |        ||
                                   |        ||
                                   V        V|
                              ----------------------
 Got data from              | |                    | Recieve data from callbacks
 lw_terminal_vt100_read_str | | lw_terminal_vt100  | And store an in-memory
 give it to                 | |                    | state of the vt100 terminal
 lw_terminal_read_str       V ----------------------
                                 |              ^
                                 |              |
                                 |              |
                                 |              |
                                 |              |
                                 |              | Callbacks
                                 |              |
                                 |              |
                                 |              |
                                 |              |
                                 |              |
                                 V              |
                              ----------------------
 Got data from                |                    |
 lw_terminal_pasrser_read_str | lw_terminal_parser |
 parses, and call callbacks   |                    |
                              ----------------------

lw_terminal_parser
==================

lw_terminal_parser parses terminal escape sequences, calling callbacks
when a sequence is sucessfully parsed, read example/parse.c.

Provides :

 * struct lw_terminal \*lw_terminal_parser_init(void);
 * void lw_terminal_parser_destroy(struct lw_terminal\* this);
 * void lw_terminal_parser_default_unimplemented(struct lw_terminal\* this, char \*seq, char chr);
 * void lw_terminal_parser_read(struct lw_terminal \*this, char c);
 * void lw_terminal_parser_read_str(struct lw_terminal \*this, char \*c);


lw_terminal_vt100
=================

Hooks into a lw_terminal_parser and keep an in-memory state of the
screen of a vt100.

Provides :
 * struct lw_terminal_vt100 \*lw_terminal_vt100_init(void \*user_data, void (\*unimplemented)(struct lw_terminal\* term_emul, char \*seq, char chr));
 * char lw_terminal_vt100_get(struct lw_terminal_vt100 \*vt100, unsigned int x, unsigned int y);
 * const char \*\*lw_terminal_vt100_getlines(struct lw_terminal_vt100 \*vt100);
 * void lw_terminal_vt100_destroy(struct lw_terminal_vt100 \*this);
 * void lw_terminal_vt100_read_str(struct lw_terminal_vt100 \*this, char \*buffer);


hl_vt100
========

Forks a program, plug its io to a pseudo terminal and emulate a vt100
using lw_terminal_vt100.

Provides :
 * void vt100_headless_fork(struct vt100_headless \*this, const char \*progname, char \*const argv[]);
 * struct vt100_headless \*vt100_headless_init(void);
 * const char \*\*vt100_headless_getlines(struct vt100_headless \*this);
