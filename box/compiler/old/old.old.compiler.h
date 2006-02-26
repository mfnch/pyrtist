/* compiler.h - Autore: Franchin Matteo - giugno 2004
 *
 *  Dichiarazioni utili per poter usare le funzioni definite in 'compiler.c'
 */

/* Tipo oggetto */
typedef void *Object;

/* Tipo stringa */
typedef struct {
	UInt	leng;
	char	*str;
} String;

/* Enumero tutti i tipi di dato intrinseci */
typedef enum { INTG, REAL, POINT, STRING } Intrinsic;

typedef union {
	Intrinsic	intr;
	Symbol		*user;
} ExprType;

/* Tipo di dato */
typedef struct {
	/* Indica se si tratta di un registro o di un valore */
	unsigned int is_register : 1;
	/* Indica se l'oggetto e' di tipo intrinseco o definito dall'utente */
	unsigned int type_is_intrinsic : 1;

	/* Descrive il tipo di dato */
	ExprType type;

	/* Contiene il valore del dato */
	union {
		UInt	reg;	/* Registro (intero, real, point) */
		Intg	i;		/* Valore intero */
		Real	r;		/* Valore reale */
		Point	p;		/* Valore punto */
		String	s;		/* Valore stringa */
		Object	o;		/* Valore oggetto */
	} data;
} Expr;

void Cmpl_Immediate_Intg_Create(Expr *e, Intg i);
void Cmpl_Immediate_Real_Create(Expr *e, Real r);

/*int pars_toreg(value *v);
int pars_ntofreg(value *v);
int pars_numop(value *v1, value *v2, long iins, long fins);
int pars_pntop(value *v1, value *v2, long pins);
int pars_2ntop(value *vp, value *vn1, value *vn2);
int pars_ntof(value *vn);
int pars_popf(value *vp, value *vn, long pinsf);
int pars_ntofvar(value *var, value *num);
int pars_ntoivar(value *var, value *num);
int pars_ntovar(name *var, value *num);
*/
