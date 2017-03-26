/* assembler.c - Autore: Franchin Matteo - maggio 2004
 *
 * Questo file contiene le funzioni che eseguono le istruzioni elementari
 * della macchina virtuale e le funzioni per scrivere le istruzioni
 * da "dare in pasto" alla macchina virtuale.
 */

#include <stdarg.h>

#include "types.h"
#include "messages.h"
#include "array.h"

typedef enum {
	Asm_Add_II=0, Asm_Add_RR, Asm_Add_PP, Asm_Add_SS, 
	Asm_Sub_II, Asm_Sub_RR, Asm_Sub_PP 
} AsmInstruction;

struct  {
	char *name;			/* Nome dell'istruzione */
	UInt num_arg;		/* Numero degli argomenti */
	char arg_type[4];	/* Tipo degli argomenti */
	Task (*exec)();		/* Funzione che esegue l'istruzione */

} asm_instr_exec[] = {
	{ "end", 0, "", Asm_Exec_End }
};

/*******************************************************************************
 *   SEGUONO LE FUNZIONI CHE ESEGUONO LE ISTRUZIONI DELLA MACCHINA VIRTUALE    *
 *******************************************************************************
 */

/* Addizione */
Task Asm_Exec_Add_II(Intg *r1, Intg *r2) { *r1 += *r2; return Success; }
Task Asm_Exec_Add_RR(Real *r1, Real *r2) { *r1 += *r2; return Success; }
Task Asm_Exec_Add_PP(Point *r1, Point *r2) {
 r1->x += r2->x; r1->y += r2->y; return Success; }
Task Asm_Exec_Add_SS(String *r1, String *r2) {
	UInt l = r1->leng;
	char *s;

	s = (char *) malloc( r1->leng += r2->leng; )
	strncpy(s, r1->str, l);
	strncpy(s+l, r2->str, r2->leng);
	free(r1->str); r1->str = s;
	free(r2->str); r2->leng = 0;
	return Success;
}

/* Sottrazione */
Task Asm_Exec_Sub_II(Intg *r1, Intg *r2) { *r1 -= *r2; return Success; }
Task Asm_Exec_Sub_RR(Real *r1, Real *r2) { *r1 -= *r2; return Success; }
Task Asm_Exec_Sub_PP(Point *r1, Point *r2) {
 r1->x -= r2->x; r1->y -= r2->y; return Success; }

/*******************************************************************************
 *      FUNZIONI CHE SERVONO A SCRIVERE LE ISTRUZIONI IN FORMATO BINARIO       *
 *******************************************************************************
 */

static Array asm_output = NULL;
 
/* DESCRIZIONE: Imposta la destinazione del codice assemblato da Asm_assemble.
 */
Task Asm_Set_Output(Array *o)
{
	if ( o == NULL ) {
		MSG_ERROR("Impossibile impostare l'output dell'assemblatore!");
		return Failed;
	}

	asm_output = o;
	return Success;
}

/* DESCRIZIONE: 
 */
Task Asm_Assemble(AsmInstruction ins, ...)
{
	va_list ap;

	/* Cerco l'istruzione ins per sapere come e' fatta! */
	

	va_start(ap, ins);
	
	va_end(ap);

	return Success;
}













/*#include <string.h>

#include "buffer.h"
#include "types.h"
#include "opstrucs.h"
#include "opcodes.h"*/

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

/* Buffer contenente il programma */
buff program = {0};

/* NOME: op_init
 * DESCRIZIONE: inizializza la zona della memoria destinata a contenere
 *  il codice binario del programma.
 *  idim specifica la dimensione iniziale di questo buffer.
 */
int op_init(long idim)
{
	return buff_create(&program, 1, idim);
}

/* Seguono le procedure per aggiungere(in modo sequenziale) un'istruzione al programma
 */
void op_line(long num)
{
	if (! buff_bigenough(&program, program.numel
	 + sizeof(struct ophead) + sizeof(struct opline)) ) return;	
	((struct ophead *) (program.ptr + program.numel))->type = ins_line;
	((struct ophead *) (program.ptr + program.numel))->dim = sizeof(struct ophead) + sizeof(struct opline);
	((struct opline *) (program.ptr + program.numel + sizeof(struct ophead)))->num = num;
	program.numel += sizeof(struct ophead) + sizeof(struct opline);
	return;
}

void op_ins0r(long ins)
{
	if ( buff_bigenough(&program, program.numel + sizeof(struct ophead)) ) {
		((struct ophead *) (program.ptr + program.numel))->type = ins;
		((struct ophead *) (program.ptr + program.numel))->dim = sizeof(struct ophead);
		program.numel += sizeof(struct ophead);
	}
	return;
}

void op_ins1r(long ins, long reg)
{
	if ( buff_bigenough(&program, program.numel + sizeof(struct ophead) +
	 sizeof(struct op1reg)) ) {
		((struct ophead *) (program.ptr + program.numel))->type = ins;
		((struct ophead *) (program.ptr + program.numel))->dim =
		 sizeof(struct ophead) + sizeof(struct op1reg);
		((struct op1reg *) (program.ptr + program.numel + sizeof(struct ophead)))->reg = reg;
		program.numel += sizeof(struct ophead) + sizeof(struct op1reg);
	}
	return;
}

void op_ins2r(long ins, long reg1, long reg2)
{
	if ( buff_bigenough(&program, program.numel + sizeof(struct ophead) +
	 sizeof(struct op2reg)) ) {
		((struct ophead *) (program.ptr + program.numel))->type = ins;
		((struct ophead *) (program.ptr + program.numel))->dim =
		 sizeof(struct ophead) + sizeof(struct op2reg);
		((struct op2reg *) (program.ptr + program.numel + sizeof(struct ophead)))->reg1 = reg1;
		((struct op2reg *) (program.ptr + program.numel + sizeof(struct ophead)))->reg2 = reg2;
		program.numel += sizeof(struct ophead) + sizeof(struct op2reg);
	}
	return;
}

void op_ins3r(long ins, long reg1, long reg2, long reg3)
{
	if ( buff_bigenough(&program, program.numel + sizeof(struct ophead) +
	 sizeof(struct op3reg)) ) {
		((struct ophead *) (program.ptr + program.numel))->type = ins;
		((struct ophead *) (program.ptr + program.numel))->dim =
		 sizeof(struct ophead) + sizeof(struct op3reg);
		((struct op3reg *) (program.ptr + program.numel + sizeof(struct ophead)))->reg1 = reg1;
		((struct op3reg *) (program.ptr + program.numel + sizeof(struct ophead)))->reg2 = reg2;
		((struct op3reg *) (program.ptr + program.numel + sizeof(struct ophead)))->reg3 = reg3;
		program.numel += sizeof(struct ophead) + sizeof(struct op3reg);
	}
	return;
}

void op_ldi(long reg, long val)
{
	if (! buff_bigenough(&program, program.numel
	 + sizeof(struct ophead) + sizeof(struct opldi)) ) return;	
	((struct ophead *) (program.ptr + program.numel))->type = ins_ldi;
	((struct ophead *) (program.ptr + program.numel))->dim = sizeof(struct ophead) + sizeof(struct opldi);
	((struct opldi *) (program.ptr + program.numel + sizeof(struct ophead)))->reg = reg;
	((struct opldi *) (program.ptr + program.numel + sizeof(struct ophead)))->val = val;
	program.numel += sizeof(struct ophead) + sizeof(struct opldi);
	return;
}

void op_ldf(long reg, double val)
{
	if (! buff_bigenough(&program, program.numel
	 + sizeof(struct ophead) + sizeof(struct opldf)) ) return;
	((struct ophead *) (program.ptr + program.numel))->type = ins_ldf;
	((struct ophead *) (program.ptr + program.numel))->dim = sizeof(struct ophead) + sizeof(struct opldf);
	((struct opldf *) (program.ptr + program.numel + sizeof(struct ophead)))->reg = reg;
	((struct opldf *) (program.ptr + program.numel + sizeof(struct ophead)))->val = val;
	program.numel += sizeof(struct ophead) + sizeof(struct opldf);
	return;
}

void op_ldp(long reg, double valx, double valy)
{
	if (! buff_bigenough(&program, program.numel
	 + sizeof(struct ophead) + sizeof(struct opldp)) ) return;
	((struct ophead *) (program.ptr + program.numel))->type = ins_ldp;
	((struct ophead *) (program.ptr + program.numel))->dim = sizeof(struct ophead) + sizeof(struct opldp);
	((struct opldp *) (program.ptr + program.numel + sizeof(struct ophead)))->reg = reg;	
	((struct opldp *) (program.ptr + program.numel + sizeof(struct ophead)))->val.x = valx;
	((struct opldp *) (program.ptr + program.numel + sizeof(struct ophead)))->val.y = valy;
	program.numel += sizeof(struct ophead) + sizeof(struct opldp);
	return;
}

void op_lds(long reg, char *s, int leng)
{
	int d;
	char *ptr;

	d = sizeof(struct ophead) + sizeof(struct oplds) + leng + 1;
	d = ((d+3)/4)*4;

	if (! buff_bigenough(&program, program.numel + d )) return;
	((struct ophead *) (program.ptr + program.numel))->type = ins_lds;
	((struct ophead *) (program.ptr + program.numel))->dim = d;
	((struct oplds *) (program.ptr + program.numel + sizeof(struct ophead)))->reg = reg;
	
	ptr = & (((struct oplds *) (program.ptr + program.numel + sizeof(struct ophead)))->s);
	strncpy(ptr, s, leng+1);
	
	program.numel += d;
	return;
}

void op_call(long num)
{
	if (! buff_bigenough(&program, program.numel
	 + sizeof(struct ophead) + sizeof(struct opcall)) ) return;	
	((struct ophead *) (program.ptr + program.numel))->type = ins_call;
	((struct ophead *) (program.ptr + program.numel))->dim = sizeof(struct ophead) + sizeof(struct opcall);
	((struct opcall *) (program.ptr + program.numel + sizeof(struct ophead)))->fnid = num;
	program.numel += sizeof(struct ophead) + sizeof(struct opcall);
	return;
}

/* Le seguenti procedure simulano l'azione di alcune istruzioni
 * (servono al compilatore per ottimizzare il codice)
 */

/* NOME: op_imm1r
 * DESCRIZIONE: Dato il puntatore al valore numerico v1, esegue l'istruzione
 *  ins mettendo il risultato in v1.
 *  Il tipo dei dati a cui punta v1 varia a seconda dell'istruzione considerata.
 * NOTA: Restituisce 1 se non ci sono problemi, 0 altrimenti.
 */
int op_imm1r(long ins, void *v1)
{
	switch (ins) {
	 case ins_negi:
		*((TYPE_INTG *) v1) = - *((TYPE_INTG *) v1);
		return 1; break;
	 case ins_negf:
		*((TYPE_FLT *) v1) = - *((TYPE_FLT *) v1);
		return 1; break;	
	 case ins_negp:
		((TYPE_PNT *) v1)->x = - ((TYPE_PNT *) v1)->x;
		((TYPE_PNT *) v1)->y = - ((TYPE_PNT *) v1)->y;
		return 1; break;
	 default:
		return 0; break;
	}

	return 0;
}

/* NOME: op_imm2r
 * DESCRIZIONE: Dati i puntatori ai valori numerici, esegue l'istruzione
 *  ins mettendo il risultato in v1.
 *  Il tipo dei dati a cui puntano v1 e v2 variano a seconda dell'istruzione
 *  considerata.
 * NOTA: Restituisce 1 se non ci sono problemi, 0 altrimenti.
 */
int op_imm2r(long ins, void *v1, void *v2)
{
	switch (ins) {
	 case ins_itof:
		*((TYPE_FLT *) v1) = (TYPE_FLT) *((TYPE_INTG *) v2);
		return 1; break;
	 case ins_ftoi:
		*((TYPE_INTG *) v1) = (TYPE_INTG) *((TYPE_FLT *) v2);
		return 1; break;
	 case ins_pxtof:
		*((TYPE_FLT *) v1) = ((TYPE_PNT *) v2)->x;
		return 1; break;
	 case ins_pytof:
		*((TYPE_FLT *) v1) = ((TYPE_PNT *) v2)->y;
		return 1; break;
	 case ins_addi:
		*((TYPE_INTG *) v1) += *((TYPE_INTG *) v2);
		return 1; break;
	 case ins_addf:
		*((TYPE_FLT *) v1) += *((TYPE_FLT *) v2);
		return 1; break;
	 case ins_addp:
		((TYPE_PNT *) v1)->x += ((TYPE_PNT *) v2)->x;
		((TYPE_PNT *) v1)->y += ((TYPE_PNT *) v2)->y;
		return 1; break;
	 case ins_subi:
		*((TYPE_INTG *) v1) -= *((TYPE_INTG *) v2);
		return 1; break;
	 case ins_subf:
		*((TYPE_FLT *) v1) -= *((TYPE_FLT *) v2);
		return 1; break;
	 case ins_subp:
		((TYPE_PNT *) v1)->x -= ((TYPE_PNT *) v2)->x;
		((TYPE_PNT *) v1)->y -= ((TYPE_PNT *) v2)->y;
		return 1; break;
	 case ins_muli:
		*((TYPE_INTG *) v1) *= *((TYPE_INTG *) v2);
		return 1; break;
	 case ins_mulf:
		*((TYPE_FLT *) v1) *= *((TYPE_FLT *) v2);
		return 1; break;
	 case ins_divi:
		*((TYPE_INTG *) v1) /= *((TYPE_INTG *) v2);
		return 1; break;
	 case ins_divf:
		*((TYPE_FLT *) v1) /= *((TYPE_FLT *) v2);
		return 1; break;
	 case ins_pmulf:
		((TYPE_PNT *) v1)->x *= *((TYPE_FLT *) v2);
		((TYPE_PNT *) v1)->y *= *((TYPE_FLT *) v2);
		return 1; break;	
	 case ins_pdivf:
		((TYPE_PNT *) v1)->x /= *((TYPE_FLT *) v2);
		((TYPE_PNT *) v1)->y /= *((TYPE_FLT *) v2);
		return 1; break;
	 default:
	 	return 0; break;
	};

	return 0;
}

/* NOME: op_imm3r
 * DESCRIZIONE: Dati i puntatori ai valori numerici, esegue l'istruzione
 *  ins mettendo il risultato in v1.
 *  Il tipo dei dati a cui puntano v1 e v2 variano a seconda dell'istruzione
 *  considerata.
 * NOTA: Restituisce 1 se non ci sono problemi, 0 altrimenti.
 */
int op_imm3r(long ins, void *v1, void *v2, void *v3)
{
	switch (ins) {
	 case ins_2ftop:
		((TYPE_PNT *) v1)->x = *((TYPE_FLT *) v2);
		((TYPE_PNT *) v1)->y = *((TYPE_FLT *) v3);
		return 1; break;
	 default:
	 	return 0; break;
	};

	return 0;
}
