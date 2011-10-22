%module hl_vt100

%{
#include "src/lw_terminal_vt100.h"
#include "src/hl_vt100.h"
%}

%typemap(in) char ** {
    /* Check if is a list */
    if (PyList_Check($input)) {
        int size = PyList_Size($input);
        int i = 0;
        $1 = (char **) malloc((size+1)*sizeof(char *));
        for (i = 0; i < size; i++) {
            PyObject *o = PyList_GetItem($input,i);
            if (PyString_Check(o))
                $1[i] = PyString_AsString(PyList_GetItem($input,i));
            else {
                PyErr_SetString(PyExc_TypeError,"list must contain strings");
                free($1);
                return NULL;
            }
        }
        $1[i] = 0;
    } else {
        PyErr_SetString(PyExc_TypeError,"not a list");
        return NULL;
    }
 }

%typemap(out) char ** {
    /* Check if is a list */
    int i;

    $result = PyList_New(0);
    for (i = 0; i < arg1->term->height; i++)
        PyList_Append($result, PyString_FromStringAndSize($1[i], arg1->term->width));
 }


// This cleans up the char ** array we malloc'd before the function call
%typemap(freearg) char ** {
    free((char *) $1);
 }

struct vt100_headless
{
    void (*changed)(struct vt100_headless *this);
%immutable;
    int master;
    struct termios backup;
    struct lw_terminal_vt100 *term;
    %extend {
        vt100_headless();
        ~vt100_headless();
        void fork(const char *progname, char **argv);
        char **getlines();
        int main_loop();
    }
};
