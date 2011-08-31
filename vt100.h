#ifndef __VT100_H__
#define __VT100_H__

#define VT100_STACK_SIZE 1024

enum vt100_state
{
    INIT,
    ESC,
    HASH,
    G0SET,
    G1SET,
    CSI
};

struct vt100_emul;

typedef void (*vt100_action)(struct vt100_emul *emul);

/*
** [w] -> VT100 to Host
** [r] -> Host to VT100
** [rw] -> VT100 to Host and Host to VT100
** Please read http://vt100.net/docs/vt100-ug/chapter3.html#S3.3
*/
struct vt100_hash_callbacks
{
    vt100_action hash_0;
    vt100_action hash_1;
    vt100_action hash_2;
    vt100_action DECDHL_TOP; /* Double Height Line   ESC # 3 */
    vt100_action DECDHL_BOTTOM; /*Double Height Line ESC # 4 */
    vt100_action DECSWL; /* Single-width Line        ESC # 5 */
    vt100_action DECDWL; /* Double-Width Line        ESC # 6 */
    vt100_action hash_7;
    vt100_action DECALN; /* Screen Alignment Display ESC # 8 */
    vt100_action hash_9;
};

struct vt100_SCS_callbacks
{
    vt100_action SCS_0; /* Special Graphics */
    vt100_action SCS_1; /* Alternate Character ROM Standard Character Set */
    vt100_action SCS_2; /* Alternate Character ROM Special Graphics */
    vt100_action SCS_3;
    vt100_action SCS_4;
    vt100_action SCS_5;
    vt100_action SCS_6;
    vt100_action SCS_7;
    vt100_action SCS_8;
    vt100_action SCS_9;

    vt100_action SCS_3A;
    vt100_action SCS_3B;
    vt100_action SCS_3C;
    vt100_action SCS_3D;
    vt100_action SCS_3E;
    vt100_action SCS_3F;
    vt100_action SCS_40;

    vt100_action SCS_A; /* United kingdom Set */
    vt100_action SCS_B; /* ASCII Set*/
};

struct vt100_ESC_callbacks
{
    vt100_action ESC_0;
    vt100_action ESC_1;
    vt100_action ESC_2;
    vt100_action ESC_3;
    vt100_action ESC_4;
    vt100_action ESC_5;
    vt100_action ESC_6;
    vt100_action DECSC; /* Save Cursor               ESC 7 */
    vt100_action DECRC; /* Restore Cursor            ESC 8 */
    vt100_action ESC_9;

    vt100_action ESC_3A;
    vt100_action ESC_3B;
    vt100_action ESC_3C;
    vt100_action DECKPAM; /* Keypad Application Mode ESC = */
    vt100_action DECKPNM; /* Keypad Numeric Mode     ESC > */
    vt100_action ESC_3F;
    vt100_action ESC_40;

    vt100_action ESC_A;
    vt100_action ESC_B;
    vt100_action ESC_C;
    vt100_action IND; /* Index                       ESC D */
    vt100_action NEL; /* Next Line                   ESC E */
    vt100_action ESC_F;
    vt100_action ESC_G;
    vt100_action HTS; /* Horizontal Tabulation Set   ESC H */
    vt100_action ESC_I;
    vt100_action ESC_J;
    vt100_action ESC_K;
    vt100_action ESC_L;
    vt100_action RI;  /* Reverse Index               ESC M */
    vt100_action ESC_N;
    vt100_action ESC_O;
    vt100_action ESC_P;
    vt100_action ESC_Q;
    vt100_action CPR; /* [w] Cursor Position Report  ESC [ Pn ; Pn R */
    vt100_action ESC_S;
    vt100_action ESC_T;
    vt100_action ESC_U;
    vt100_action ESC_V;
    vt100_action ESC_W;
    vt100_action ESC_X;
    vt100_action ESC_Y;
    vt100_action DECID; /* Identify Terminal         ESC Z */

    vt100_action ESC_5B;
    vt100_action ESC_5C;
    vt100_action ESC_5D;
    vt100_action ESC_5E;
    vt100_action ESC_5F;
    vt100_action ESC_60;

    vt100_action ESC_a;
    vt100_action ESC_b;
    vt100_action RIS; /* Reset To Initial State      ESC c */
    vt100_action ESC_d;
    vt100_action ESC_e;
    vt100_action ESC_f;
    vt100_action ESC_g;
    vt100_action ESC_h;
    vt100_action ESC_i;
    vt100_action ESC_j;
    vt100_action ESC_k;
    vt100_action ESC_l;
    vt100_action ESC_m;
    vt100_action ESC_n;
    vt100_action ESC_o;
    vt100_action ESC_p;
    vt100_action ESC_q;
    vt100_action ESC_r;
    vt100_action ESC_s;
    vt100_action ESC_t;
    vt100_action ESC_u;
    vt100_action ESC_v;
    vt100_action ESC_w;
    vt100_action ESC_x;
    vt100_action ESC_y;
    vt100_action ESC_z;
};

struct vt100_CSI_callbacks
{
    vt100_action CUU; /* [rw] Cursor Up              ESC [ Pn A */
    vt100_action CUD; /* [rw] Cursor Down            ESC [ Pn B */
    vt100_action CUF; /* [rw] Cursor Forward         ESC [ Pn C */
    vt100_action CUB; /* [rw] Cursor Backward        ESC [ Pn D */
    vt100_action CSI_E;
    vt100_action CSI_F;
    vt100_action CSI_G;
    vt100_action CUP; /* [??] Cursor Position        ESC [ Pn ; Pn H */
    vt100_action CSI_I;
    vt100_action ED; /* Erase In Display             ESC [ Ps J */
    vt100_action EL; /* Erase In Line                ESC [ Ps K */
    vt100_action CSI_L;
    vt100_action CSI_M;
    vt100_action CSI_N;
    vt100_action CSI_O;
    vt100_action CSI_P;
    vt100_action CSI_Q;
    vt100_action CPR; /* [w] Cursor Position Report  ESC [ Pn ; Pn R */
    vt100_action CSI_S;
    vt100_action CSI_T;
    vt100_action CSI_U;
    vt100_action CSI_V;
    vt100_action CSI_W;
    vt100_action CSI_X;
    vt100_action CSI_Y;
    vt100_action CSI_Z;

    vt100_action CSI_5B;
    vt100_action CSI_5C;
    vt100_action CSI_5D;
    vt100_action CSI_5E;
    vt100_action CSI_5F;
    vt100_action CSI_60;

    vt100_action CSI_a;
    vt100_action CSI_b;
    vt100_action DA;  /* [??] Device Attributes      ESC [ Pn c */
    vt100_action CSI_d;
    vt100_action CSI_e;
    vt100_action HVP; /* Horiz. and Vert. Position   ESC [ Pn ; Pn f */
    vt100_action TBC; /* Tabulation Clear            ESC [ Ps g */
    vt100_action SM; /* Set Mode                     ESC [ Ps ; . . . ; Ps h */
    vt100_action CSI_i;
    vt100_action CSI_j;
    vt100_action CSI_k;
    vt100_action RM; /* Reset Mode              ESC [ Ps ; Ps ; . . . ; Ps l */
    vt100_action SGR; /* Select Graphic Rendition    ESC [ Ps ; . . . ; Ps m */
    vt100_action DSR; /* Device Status Report        ESC [ Ps n */
    vt100_action CSI_o;
    vt100_action CSI_p;
    vt100_action DECLL; /* Load LEDS                 ESC [ Ps q */
    vt100_action DECSTBM; /* Set Top and Bottom Margins ESC [ Pn; Pn r */
    vt100_action CSI_s;
    vt100_action CSI_t;
    vt100_action CSI_u;
    vt100_action CSI_v;
    vt100_action CSI_w;
    vt100_action DECRE_TPARM; /* Re{port,quest} Terminal Parameters ESC [ . x */
    vt100_action DECTST; /* Invoke Confidence Test   ESC [ 2 ; Ps y */
    vt100_action CSI_z;
};

struct vt100_callbacks
{
    struct vt100_ESC_callbacks esc;
    struct vt100_CSI_callbacks csi;
    struct vt100_hash_callbacks hash;
    struct vt100_SCS_callbacks scs;
};

struct vt100_emul
{
    unsigned int           width;
    unsigned int           height;
    unsigned int           cursor_pos_x;
    unsigned int           cursor_pos_y;
    enum vt100_state       state;
    unsigned int           argc;
    unsigned int           argv[VT100_STACK_SIZE];
    void                   (*write)(struct vt100_emul *, char c);
    char                   stack[VT100_STACK_SIZE];
    unsigned int           stack_ptr;
    struct vt100_callbacks *callbacks;
    void                   *user_data;
};

struct vt100_emul *vt100_init(unsigned int width, unsigned int height,
                              struct vt100_callbacks *callbacks,
                              void (*write)(struct vt100_emul *, char));

void vt100_read(struct vt100_emul *vt100, char c);
void vt100_read_str(struct vt100_emul *vt100, char *c);

#endif
