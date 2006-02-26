/* compiler.c - Autore: Franchin Matteo - giugno 2004
 *
 * Questo file contiene il codice importante per il compilatore:
 * gestione delle espressioni, ...
 */

#include "types.h"
#include "messages.h"
#include "symbols.h"
#include "compiler.h"

/* DESCRIZIONE: Crea in *e un'espressione immediata intera,
 *  il cui valore sara' i.
 */
void Cmpl_Immediate_Intg_Create(Expr *e, Intg i)
{
	e->is_register = 0;
	e->type_is_intrinsic = 1;
	e->type.intr = INTG;
	e->data.i = i;
	return;
}

/* DESCRIZIONE: Crea in *e un'espressione immediata reale,
 *  il cui valore sara' r.
 */
void Cmpl_Immediate_Real_Create(Expr *e, Real r)
{
	e->is_register = 0;
	e->type_is_intrinsic = 1;
	e->type.intr = REAL;
	e->data.r = r;
	return;
}

/* DESCRIZIONE: Crea in *e un'espressione immediata di tipo punto,
 *  il cui valore sara' *p.
 *
void Cmpl_Immediate_Point_Create(Expr *e, Point *p)
{
	e->is_register = 0;
	e->type_is_intrinsic = 1;
	e->type.intr = POINT;
	e->data.p = *p;
	return;
}*/

/* DESCRIZIONE: Crea in *e un'espressione immediata di tipo stringa,
 *  il cui valore sara' s.
 *
Task Cmpl_Immediate_String_Create(Expr *e, String *s)
{

}*/







/*typedef struct {
	enum {ADD, SUB, MUL, DIV}

} Operation;


Task Cmpl_Operation( Operation *op, Expr *a, Expr *b )
{
	* Prima di tutto controllo che sia possibile eseguire l'operazione
	 * tra a e b
	 *
	Cmp_

	* Se l'operazione non esiste, allora cerco di convertire
	 * a in un'espressione dello stesso tipo di b o viceversa.
	 *

	* Per eseguire l'operazione, a e b devono essere o entrambi registri
	 * o entambi immediati. In caso contrario devo convertire l'immediato
	 * in registro.
	 *
	if ( a->is_register != b->is_register ) {
		if ( a->is_register ) {
			* Converto b in registro *
		} else {
			* Converto a in registro *
		}
	}

}*/

