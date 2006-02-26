/* DESCRIZIONE: Converte l'espressione expr in un'espressione contenuta
 *  in un registro locale. Se force = 1, tale operazione viene eseguita
 *  in tutti i casi, se invece force = 0, l'operazione viene eseguita
 *  solo per espressioni immediate di tipo TYPE_REAL e TYPE_POINT.
 */
Expression *Cmp__Expr_To_LReg(Expression *expr, int force, int zero)
{
    int is_integer;
	Expression lreg;

	MSG_LOCATION("Cmp__Expr_To_LReg");

	/* Se l'espressione e' gia' un registro locale, allora esco! */
	if ( expr->categ == CAT_LREG ) {
		if ( expr->value.reg > 0 ) return expr;
	}

    is_integer = (expr->type == TYPE_CHAR) || (expr->type == TYPE_INTG);
	if ( (expr->categ == CAT_IMM) && (! is_integer) ) {
		/* Metto in lreg un nuovo registro locale */
		if IS_FAILED( Cmp_Expr_LReg(& lreg, expr->type, zero) ) return NULL;

		switch ( expr->type ) {
		 case TYPE_REAL:
			VM_Assemble(ASM_MOV_Rimm,
			 CAT_LREG, lreg.value.reg, CAT_IMM, expr->value.r);
			break;
		 case TYPE_POINT:
			VM_Assemble(ASM_MOV_Pimm,
			 CAT_LREG, lreg.value.reg, CAT_IMM, expr->value.p);
			break;
		 default:
			MSG_ERROR( "Errore interno: non sono ammessi valori immediati"
			 " per il tipo '%s'.", Tym_Type_Name(expr->type) );
			return NULL;
			break;
		}
		(void) Cmp_Free(expr);
		*expr = lreg;
		return expr;

	} else {
		if ( ! force ) return expr;

		if ( expr->categ == CAT_PTR ) {
			/* L'espressione e' un puntatore, allora devo settare il puntatore
			 * di riferimento, in modo da realizzare una cosa simile a:
			 *   mov ro0, ro1 <-- setto il puntatore di riferimento (ro0)
			 *   mov rr1, real[ro0+8] <-- prelevo il valore
			 */
			Intg addr_categ = ( expr->is.gaddr ) ? CAT_GREG : CAT_LREG;
			VM_Assemble( ASM_MOV_OO, CAT_LREG, (Intg) 0,
			 addr_categ, expr->addr );
		}

		/* Metto in lreg un nuovo registro locale */
		if IS_FAILED( Cmp_Expr_LReg(& lreg, expr->type, zero) ) return NULL;

		switch ( expr->type ) {
		 case TYPE_NONE:
			MSG_ERROR("Errore interno in Cmp_Expr_To_LReg");
			return NULL;
			break;
		 case TYPE_CHAR:
			VM_Assemble( ASM_MOV_CC,
			 CAT_LREG, lreg.value.reg, expr->categ, expr->value.i);
			break;
		 case TYPE_INTG:
			VM_Assemble(ASM_MOV_II,
			 CAT_LREG, lreg.value.reg, expr->categ, expr->value.i);
			break;
		 case TYPE_REAL:
			VM_Assemble(ASM_MOV_RR,
			 CAT_LREG, lreg.value.reg, expr->categ, expr->value.i);
			break;
		 case TYPE_POINT:
			VM_Assemble(ASM_MOV_PP,
			 CAT_LREG, lreg.value.reg, expr->categ, expr->value.i);
			break;
		 default:
			VM_Assemble(ASM_MOV_OO,
			 CAT_LREG, lreg.value.reg, expr->categ, expr->value.i);
			break;
		}
		(void) Cmp_Free(expr);
		*expr = lreg;
		return expr;
	}
}
