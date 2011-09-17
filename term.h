#ifndef __TERM_H__
#define __TERM_H__

#define TERM_STACK_SIZE 1024

enum term_state
{
    INIT,
    ESC,
    HASH,
    G0SET,
    G1SET,
    CSI
};

struct term_emul;

typedef void (*term_action)(struct term_emul *emul);

/*
** [w] -> VT100 to Host
** [r] -> Host to VT100
** [rw] -> VT100 to Host and Host to VT100
** Please read http://vt100.net/docs/vt100-ug/chapter3.html#S3.3
*/
struct term_hash_callbacks
{
    term_action hash_0;
    term_action hash_1;
    term_action hash_2;
    term_action DECDHL_TOP; /* Double Height Line   ESC # 3 */
    term_action DECDHL_BOTTOM; /*Double Height Line ESC # 4 */
    term_action DECSWL; /* Single-width Line        ESC # 5 */
    term_action DECDWL; /* Double-Width Line        ESC # 6 */
    term_action hash_7;
    term_action DECALN; /* Screen Alignment Display ESC # 8 */
    term_action hash_9;
};

struct term_SCS_callbacks
{
    term_action SCS_0; /* Special Graphics */
    term_action SCS_1; /* Alternate Character ROM Standard Character Set */
    term_action SCS_2; /* Alternate Character ROM Special Graphics */
    term_action SCS_3;
    term_action SCS_4;
    term_action SCS_5;
    term_action SCS_6;
    term_action SCS_7;
    term_action SCS_8;
    term_action SCS_9;

    term_action SCS_3A;
    term_action SCS_3B;
    term_action SCS_3C;
    term_action SCS_3D;
    term_action SCS_3E;
    term_action SCS_3F;
    term_action SCS_40;

    term_action SCS_A; /* United kingdom Set */
    term_action SCS_B; /* ASCII Set*/
};

struct term_ESC_callbacks
{
    term_action ESC_0;
    term_action ESC_1;
    term_action ESC_2;
    term_action ESC_3;
    term_action ESC_4;
    term_action ESC_5;
    term_action ESC_6;
    term_action DECSC; /* Save Cursor               ESC 7 */
    term_action DECRC; /* Restore Cursor            ESC 8 */
    term_action ESC_9;

    term_action ESC_3A;
    term_action ESC_3B;
    term_action ESC_3C;
    term_action DECKPAM; /* Keypad Application Mode ESC = */
    term_action DECKPNM; /* Keypad Numeric Mode     ESC > */
    term_action ESC_3F;
    term_action ESC_40;

    term_action ESC_A;
    term_action ESC_B;
    term_action ESC_C;
    term_action IND; /* Index                       ESC D */
    term_action NEL; /* Next Line                   ESC E */
    term_action ESC_F;
    term_action ESC_G;
    term_action HTS; /* Horizontal Tabulation Set   ESC H */
    term_action ESC_I;
    term_action ESC_J;
    term_action ESC_K;
    term_action ESC_L;
    term_action RI;  /* Reverse Index               ESC M */
    term_action ESC_N;
    term_action ESC_O;
    term_action ESC_P;
    term_action ESC_Q;
    term_action CPR; /* [w] Cursor Position Report  ESC [ Pn ; Pn R */
    term_action ESC_S;
    term_action ESC_T;
    term_action ESC_U;
    term_action ESC_V;
    term_action ESC_W;
    term_action ESC_X;
    term_action ESC_Y;
    term_action DECID; /* Identify Terminal         ESC Z */

    term_action ESC_5B;
    term_action ESC_5C;
    term_action ESC_5D;
    term_action ESC_5E;
    term_action ESC_5F;
    term_action ESC_60;

    term_action ESC_a;
    term_action ESC_b;
    term_action RIS; /* Reset To Initial State      ESC c */
    term_action ESC_d;
    term_action ESC_e;
    term_action ESC_f;
    term_action ESC_g;
    term_action ESC_h;
    term_action ESC_i;
    term_action ESC_j;
    term_action ESC_k;
    term_action ESC_l;
    term_action ESC_m;
    term_action ESC_n;
    term_action ESC_o;
    term_action ESC_p;
    term_action ESC_q;
    term_action ESC_r;
    term_action ESC_s;
    term_action ESC_t;
    term_action ESC_u;
    term_action ESC_v;
    term_action ESC_w;
    term_action ESC_x;
    term_action ESC_y;
    term_action ESC_z;
};

struct term_CSI_callbacks
{
    term_action CSI_3F; /*                          ESC [ ? */
    term_action CSI_40; /*                          ESC [ @ */
    term_action CUU; /* [rw] Cursor Up              ESC [ Pn A */
    term_action CUD; /* [rw] Cursor Down            ESC [ Pn B */
    term_action CUF; /* [rw] Cursor Forward         ESC [ Pn C */
    term_action CUB; /* [rw] Cursor Backward        ESC [ Pn D */
    term_action CSI_E;
    term_action CSI_F;
    term_action CSI_G;
    term_action CUP; /* [??] Cursor Position        ESC [ Pn ; Pn H */
    term_action CSI_I;
    term_action ED; /* Erase In Display             ESC [ Ps J */
    term_action EL; /* Erase In Line                ESC [ Ps K */
    term_action CSI_L;
    term_action CSI_M;
    term_action CSI_N;
    term_action CSI_O;
    term_action CSI_P;
    term_action CSI_Q;
    term_action CPR; /* [w] Cursor Position Report  ESC [ Pn ; Pn R */
    term_action CSI_S;
    term_action CSI_T;
    term_action CSI_U;
    term_action CSI_V;
    term_action CSI_W;
    term_action CSI_X;
    term_action CSI_Y;
    term_action CSI_Z;

    term_action CSI_5B;
    term_action CSI_5C;
    term_action CSI_5D;
    term_action CSI_5E;
    term_action CSI_5F;
    term_action CSI_60;

    term_action CSI_a;
    term_action CSI_b;
    term_action DA;  /* [??] Device Attributes      ESC [ Pn c */
    term_action CSI_d;
    term_action CSI_e;
    term_action HVP; /* Horiz. and Vert. Position   ESC [ Pn ; Pn f */
    term_action TBC; /* Tabulation Clear            ESC [ Ps g */
    term_action SM; /* Set Mode                     ESC [ Ps ; . . . ; Ps h */
    term_action CSI_i;
    term_action CSI_j;
    term_action CSI_k;
    term_action RM; /* Reset Mode              ESC [ Ps ; Ps ; . . . ; Ps l */
    term_action SGR; /* Select Graphic Rendition    ESC [ Ps ; . . . ; Ps m */
    term_action DSR; /* Device Status Report        ESC [ Ps n */
    term_action CSI_o;
    term_action CSI_p;
    term_action DECLL; /* Load LEDS                 ESC [ Ps q */
    term_action DECSTBM; /* Set Top and Bottom Margins ESC [ Pn; Pn r */
    term_action CSI_s;
    term_action CSI_t;
    term_action CSI_u;
    term_action CSI_v;
    term_action CSI_w;
    term_action DECRE_TPARM; /* Re{port,quest} Terminal Parameters ESC [ . x */
    term_action DECTST; /* Invoke Confidence Test   ESC [ 2 ; Ps y */
    term_action CSI_z;
};

struct term_callbacks
{
    struct term_ESC_callbacks esc;
    struct term_CSI_callbacks csi;
    struct term_hash_callbacks hash;
    struct term_SCS_callbacks scs;
};

struct term_emul
{
    unsigned int           width;
    unsigned int           height;
    unsigned int           cursor_pos_x;
    unsigned int           cursor_pos_y;
    enum term_state       state;
    unsigned int           argc;
    unsigned int           argv[TERM_STACK_SIZE];
    void                   (*write)(struct term_emul *, char c);
    char                   stack[TERM_STACK_SIZE];
    unsigned int           stack_ptr;
    struct term_callbacks *callbacks;
    char                   flag;
    void                   *user_data;
    void                   (*unimplemented)(struct term_emul*,
                                            char *seq,
                                            char chr);
};

struct term_emul *term_init(unsigned int width, unsigned int height,
                              struct term_callbacks *callbacks,
                              void (*write)(struct term_emul *, char));

void term_read(struct term_emul *term, char c);
void term_read_str(struct term_emul *term, char *c);

#endif
