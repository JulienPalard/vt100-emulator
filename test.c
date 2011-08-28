#include <stdlib.h>

/*
DECSC – Save Cursor (DEC Private)

ESC 7

This sequence causes the cursor position, graphic rendition, and
character set to be saved. (See DECRC).
*/
void DECSC(struct vt100 * vt100)
{
}

/*
DECRC – Restore Cursor (DEC Private)

ESC 8

This sequence causes the previously saved cursor position, graphic
rendition, and character set to be restored.
*/
void DECRC(struct vt100 * vt100)
{
}

/*
IND – Index

ESC D

This sequence causes the active position to move downward one line
without changing the column position. If the active position is at the
bottom margin, a scroll up is performed. Format Effector
*/
void IND(struct vt100 * vt100)
{
}

/*
NEL – Next Line

ESC E

This sequence causes the active position to move to the first position
on the next line downward. If the active position is at the bottom
margin, a scroll up is performed. Format Effector
*/
void NEL(struct vt100 * vt100)
{
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
void CUU(struct vt100 * vt100)
{
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
void CUD(struct vt100 * vt100)
{
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
void CUF(struct vt100 * vt100)
{
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
void CUB(struct vt100 * vt100)
{
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
void CUP(struct vt100 * vt100)
{
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
void ED(struct vt100 * vt100)
{
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
void EL(struct vt100 * vt100)
{
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
void HVP(struct vt100 * vt100)
{
}

int main(int ac, char **av)
{
    return EXIT_SUCCESS;
}
