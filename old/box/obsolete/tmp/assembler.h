



/* Assegno ad ogni istruzione una distinta "etichetta" numerica */
enum optype { ins_end = 1, ins_nop, ins_line, ins_ast, ins_aclr, ins_argi, ins_argf, ins_argp,
 ins_argo, ins_args, ins_ldi, ins_ldf, ins_ldp, ins_lds, ins_itof, ins_ftoi, ins_pxtof, ins_pytof, ins_2ftop,
 ins_movi, ins_movf, ins_movp, ins_movo, ins_negi, ins_negf, ins_negp, ins_addi, ins_addf, ins_addp,
 ins_subi, ins_subf, ins_subp, ins_muli, ins_mulf, ins_divi, ins_divf, ins_pmulf, ins_pdivf,
 ins_prni, ins_call };

/* Buffer contenente il programma (definito in "opcodes.c")*/
extern buff program;

int op_init(long idim);
void op_line(long num);
void op_ins0r(long ins);
void op_ins1r(long ins, long reg);
void op_ins2r(long ins, long reg1, long reg2);
void op_ins3r(long ins, long reg1, long reg2, long reg3);
void op_ldi(long reg, long val);
void op_ldf(long reg, double val);
void op_ldp(long reg, double valx, double valy);
void op_lds(long reg, char *s, int leng);
void op_call(long num);

int op_imm1r(long ins, void *v1);
int op_imm2r(long ins, void *v1, void *v2);
int op_imm3r(long ins, void *v1, void *v2, void *v3);
