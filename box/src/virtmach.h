/* virtmach.h - Autore: Franchin Matteo - giugno 2004
 *
 * Dichiarazioni utili per il file 'virtmach.c' per scrivere
 */

 /* Definisco il tipo Obj, che e' semplicemente il tipo puntatore */
typedef void *Obj;

/* Associo un numero a ciascun tipo, per poterlo identificare */
typedef enum {
  TYPE_NONE  = -1,
  TYPE_CHAR  =  0,
  TYPE_INTG  =  1,
  TYPE_REAL  =  2,
  TYPE_POINT =  3,
  TYPE_OBJ   =  4,
  TYPE_VOID  =  5,
  TYPE_OPEN  =  6,
  TYPE_CLOSE =  7,
  TYPE_PAUSE =  8,
  TYPE_DESTROY= 9
} TypeID;

/* Associo un numero a ciascun tipo, per poterlo identificare */
typedef enum {
  SIZEOF_CHAR  = sizeof(Char),
  SIZEOF_INTG  = sizeof(Intg),
  SIZEOF_REAL  = sizeof(Real),
  SIZEOF_POINT = sizeof(Point),
  SIZEOF_OBJ   = sizeof(Obj), SIZEOF_PTR = SIZEOF_OBJ
} SizeOfType;

/* This is an union of all possible types */
typedef union {
  Char c;
  Intg i;
  Real r;
  Point p;
  Obj o;
} Generic;

/* Enumero gli header di istruzione della macchina virtuale.
 * ATTENZIONE: L'ordine nella seguente enumerazione deve rispettare l'ordine
 *  nella tabella dei descrittori di istruzione ( vm_instr_desc_table[] )
 * NUMERO ISTRUZIONI: 77
 */
typedef enum {
  ASM_LINE_Iimm=1, ASM_CALL_I, ASM_CALL_Iimm,
  ASM_NEWC_II, ASM_NEWI_II, ASM_NEWR_II, ASM_NEWP_II, ASM_NEWO_II,
  ASM_MOV_Cimm, ASM_MOV_Iimm, ASM_MOV_Rimm, ASM_MOV_Pimm,
  ASM_MOV_CC, ASM_MOV_II, ASM_MOV_RR, ASM_MOV_PP, ASM_MOV_OO,
  ASM_BNOT_I, ASM_BAND_II, ASM_BXOR_II, ASM_BOR_II,
  ASM_SHL_II, ASM_SHR_II,
  ASM_INC_I, ASM_INC_R, ASM_DEC_I, ASM_DEC_R,
  ASM_POW_II, ASM_POW_RR,
  ASM_ADD_II, ASM_ADD_RR, ASM_ADD_PP, ASM_SUB_II, ASM_SUB_RR, ASM_SUB_PP,
  ASM_MUL_II, ASM_MUL_RR, ASM_DIV_II, ASM_DIV_RR, ASM_REM_II,
  ASM_NEG_I, ASM_NEG_R, ASM_NEG_P, ASM_PMULR_P, ASM_PDIVR_P,
  ASM_EQ_II, ASM_EQ_RR, ASM_EQ_PP, ASM_NE_II, ASM_NE_RR, ASM_NE_PP,
  ASM_LT_II, ASM_LT_RR, ASM_LE_II, ASM_LE_RR,
  ASM_GT_II, ASM_GT_RR, ASM_GE_II, ASM_GE_RR,
  ASM_LNOT_I, ASM_LAND_II, ASM_LOR_II,
  ASM_REAL_C, ASM_REAL_I, ASM_INTG_R, ASM_POINT_II, ASM_POINT_RR,
  ASM_PROJX_P, ASM_PROJY_P, ASM_PPTRX_P, ASM_PPTRY_P,
  ASM_RET,
  ASM_MALLOC_I, ASM_MFREE_O, ASM_MCOPY_OO,
  ASM_LEA_C, ASM_LEA_I, ASM_LEA_R, ASM_LEA_P, ASM_LEA_OO,
  ASM_ILLEGAL
} AsmCode;

/* Enumerazione delle categorie di argomento, utilizzata da Asm_Assemble
 * per assemblare le istruzioni.
 * NOTA: Questa enumerazione deve essere coerente con l'ordine dell'array
 *  vm_gets[].
 */
typedef enum {CAT_NONE = -1, CAT_GREG = 0, CAT_LREG, CAT_PTR, CAT_IMM} AsmArg;

/* Names of categories (see AsmArg) */
extern const char *asm_arg_str[4];

/* Enumerazione dei tipi di moduli */
typedef enum {
  MODULE_IS_VM_CODE,
  MODULE_IS_C_FUNC,
  MODULE_UNDEFINED
} ModuleType;

/* Tipo che descrive l'indirizzo del (codice-VM / funzione in C)
 * associato al modulo.
 */
typedef union {
  struct {
    Intg dim;
    void *code;
  } vm;
  Task (*c_func)(void);
} ModulePtr;

/* Tipo che serve a gestire la scrittura di codice per la VM */
typedef struct {
  struct {
    unsigned int error : 1;
    unsigned int inhibit : 1;
  } status;
  Array *program;
} AsmOut;

/* size_of_type[x] e' la dimensione del tipo x = TYPE_CHAR, TYPE_INTG, ...
 * (cioe' x e' un TypeID)
 */
extern UInt size_of_type[NUM_TYPES];

/* Queste variabili servono per il passaggio di argomenti tra la VM
 * le procedure scritte in C
 */
extern Obj *box_vm_current, *box_vm_arg1, *box_vm_arg2;
#define BOX_VM_CURRENT(Type) *((Type *) *box_vm_current)
#define BOX_VM_ARG1(Type) *((Type *) *box_vm_arg1)
#define BOX_VM_ARG2(Type) *((Type *) *box_vm_arg2)
#define BOX_VM_ARGPTR1(Type) ((Type *) *box_vm_arg1)
#define BOX_VM_ARGPTR2(Type) ((Type *) *box_vm_arg2)
extern int vm_globals;
extern void *vm_global[NUM_TYPES];
extern Intg vm_gmin[NUM_TYPES], vm_gmax[NUM_TYPES];

/* Numero minimo di ByteX4 che riesce a contenere tutti i tipi possibili
 * di argomenti (Intg, Real, Point, Obj)
 */
#define MAX_SIZE_IN_IWORDS \
 ((sizeof(Point) + sizeof(ByteX4) - 1) / sizeof(ByteX4))

Intg VM_Module_Install(ModuleType t, const char *name, ModulePtr p);
Intg VM_Module_Undefined(const char *name);
Task VM_Module_Define(Intg module_num, ModuleType t, ModulePtr p);
Intg VM_Module_Next(void);
Task VM_Module_Check(int report_errs);
Task VM_Module_Globals(Intg *num_var, Intg *num_reg);
Task VM_Module_Global_Set(Intg type, Intg reg, void *value);
Task VM_Module_Execute(Intg mnum);
void VM_DSettings(int hexcode);
Task VM_Module_Disassemble(FILE *stream, Intg module_num);
Task VM_Module_Disassemble_All(FILE *stream);
Task VM_Disassemble(FILE *output, void *prog, UInt dim);
AsmOut *VM_Asm_Out_New(Intg dim);
Task VM_Asm_Out_Set(AsmOut *out);
Task VM_Asm_Prepare(Intg *num_var, Intg *num_reg);
Task VM_Asm_Install(Intg module, AsmOut *program);
void VM_Assemble(AsmCode instr, ...);
int VM_Save(FILE *out, Intg module_num);
