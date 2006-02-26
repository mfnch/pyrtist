
/* NOME: pars_toreg
 * DESCRIZIONE: Se il valore v e' immediato, lo mette in un registro
 *  (crea il codice necessario e converte v in registro).
 * NOTA: Il valore v puo' essere di tipo intero, floating o point.
 */
int pars_toreg(value *v)
{
	if ((v->type & 0x80) != 0) {
		/* Ho a che fare con un valore numerico: lo metto in un registro */
		long nreg;

		switch (v->type) {
		 case TVAL_IMMINTG:
			if ( (nreg = reg_occupy(TVAL_REGINTG)) == 0 ) return 0;
			op_ldi(nreg, v->num.intg);
			v->type = TVAL_REGINTG;
			v->num.reg = nreg;
			return 1;
		
		 case TVAL_IMMFLT:
			if ( (nreg = reg_occupy(TVAL_REGFLT)) == 0 ) return 0;
			op_ldf(nreg, v->num.flt);
			v->type = TVAL_REGFLT;
			v->num.reg = nreg;
			return 1;
		
		 case TVAL_IMMPNT:
			if ( (nreg = reg_occupy(TVAL_REGPNT)) == 0 ) return 0;
			op_ldp(nreg, v->num.pnt.x, v->num.pnt.y);
			v->type = TVAL_REGPNT;
			v->num.reg = nreg;
			return 1;
		}
		return 0;
	}

	return 1;
}

/* NOME: pars_ntofreg
 * DESCRIZIONE: Converte il valore v in un registro di tipo float.
 * NOTA: Restituisce 1 se tutto va bene, 0 altrimenti.
 */
int pars_ntofreg(value *v)
{
	long nreg;
			
	switch (v->type) {	
	 case TVAL_IMMINTG:
		/* Chiedo un nuovo registro */
		if ( (nreg = reg_occupy(TVAL_REGFLT)) == 0 ) return 0;
		
		/* Converto l'immediato intero in immediato float */
		if (!op_imm2r(ins_itof, & v->num.flt, & v->num.intg)) return 0;	
		
		/* Assegno il valore al registro nel programma e converto v */
		op_ldf(nreg, v->num.flt);
		v->type = TVAL_REGFLT;
		v->num.reg = nreg;	
		return 1; break;

	 case TVAL_IMMFLT:
		/* Chiedo un nuovo registro */
		if ( (nreg = reg_occupy(TVAL_REGFLT)) == 0 ) return 0;
		
		/* Assegno il valore al registro nel programma e converto v */
		op_ldf(nreg, v->num.flt);
		v->type = TVAL_REGFLT;
		v->num.reg = nreg;	
		return 1; break;
	
	 case TVAL_REGINTG:
		/* Chiedo un nuovo registro */
		if ( (nreg = reg_occupy(TVAL_REGFLT)) == 0 ) return 0;
		
		/* Creo l'istruzione di conversione registro nel programma */
		op_ins2r(ins_itof, nreg, v->num.reg);
	
		/* Rilascio il registro che conteneva l'intero e converto v */
		if (!reg_release(v->type, v->num.reg)) return 0;
		v->type = TVAL_REGFLT;
		v->num.reg = nreg;
		return 1; break;
	}

	return 1;
}

/* NOME: pars_numop
 * DESCRIZIONE: Questa procedura esegue un'operazione fra due "valori".
 *  Un valore puo' essere "immediato" (cioe' puo' essere un numero ben definito)
 *  oppure puo' essere un registro.
 *  Se ho a che fare con due quantita' immediate, posso calcolare il risultato
 *  dell'operazione, che sara' ancora un valore immediato.
 *  Se invece uno dei due valori e' un registro, allora devo produrre il codice
 *  corrispondente all'operazione e il risultato finale sara' contenuto
 *  in un registro.
 * NOTA: questa funzione tratta registri o numeri interi/float.
 */
int pars_numop(value *v1, value *v2, long iins, long fins)
{		
	if ((v1->type & v2->type & 0x80) != 0) {
		/* Se ho a che fare con due valori numerici, posso calcolare
		 * direttamente il risultato
		 */
		if (v1->type != v2->type) {
			/* I numeri sono di tipo diverso: converto l'intero in float
			 * ed eseguo l'operazione fra float
			 */
			if (v1->type == TVAL_IMMINTG) {			
				if (!op_imm2r(ins_itof, & v1->num.flt, & v1->num.intg))
					return 0;
				v1->type = TVAL_IMMFLT;
			} else {
				if (!op_imm2r(ins_itof, & v2->num.flt, & v2->num.intg))
					return 0;
				v2->type = TVAL_IMMFLT;
			}

			if (!op_imm2r(fins, & v1->num.flt, & v2->num.flt)) return 0;
			return 1;

		} else {
			/* I due numeri sono dello stesso tipo */
			if (v1->type == TVAL_IMMINTG) {
				/* Sono due interi */
				if (!op_imm2r(iins, & v1->num.intg, & v2->num.intg)) return 0;
				return 1;

			} else {
				/* Sono due float */
				if (!op_imm2r(fins, & v1->num.flt, & v2->num.flt)) return 0;
				return 1;
			}
		}
		return 0;

	} else {
		/* Se uno dei due valori e' contenuto in un registro
		 * devo produrre il codice corrispondente all'operazione
		 */
		if (((v1->type ^ v2->type) & 0x80) != 0) {
			/* Se uno dei due valori e' un numero, prima di tutto lo devo
			 * mettere in un registro, ma prima lo converto, se necessario!
			 */
			long nreg;
			
			if ((v1->type & 0x80) != 0) {
				
				/* Il numero si trova in v1 */
				if ((v1->type == TVAL_IMMINTG) && (v2->type == TVAL_REGFLT)) {
					/* Se devo convertire il numero, lo faccio prima
					 * di metterlo nel registro */
					if (!op_imm2r(ins_itof, & v1->num.flt, & v1->num.intg))
						return 0;
					v1->type = TVAL_IMMFLT;
				}

				/* Chiedo un nuovo registro e vi metto il numero */
				if (v1->type == TVAL_IMMINTG) {
					if ( (nreg = reg_occupy(TVAL_REGINTG)) == 0 ) return 0;
					op_ldi(nreg, v1->num.intg);
					v1->type = TVAL_REGINTG;
					v1->num.reg = nreg;
				
				} else {
					if ( (nreg = reg_occupy(TVAL_REGFLT)) == 0 ) return 0;
					op_ldf(nreg, v1->num.flt);
					v1->type = TVAL_REGFLT;
					v1->num.reg = nreg;
				}
			
			} else {
				/* Il numero si trova in v2 */
				if ((v2->type == TVAL_IMMINTG) && (v1->type == TVAL_REGFLT)) {
					if (!op_imm2r(ins_itof, & v2->num.flt, & v2->num.intg))
						return 0;
					v2->type = TVAL_IMMFLT;
				}
				
				/* Chiedo un nuovo registro e vi metto il numero */
				if (v2->type == TVAL_IMMINTG) {
					if ( (nreg = reg_occupy(TVAL_REGINTG)) == 0 ) return 0;
					op_ldi(nreg, v2->num.intg);
					v2->type = TVAL_REGINTG;
					v2->num.reg = nreg;
				
				} else {
					if ( (nreg = reg_occupy(TVAL_REGFLT)) == 0 ) return 0;
					op_ldf(nreg, v2->num.flt);
					v2->type = TVAL_REGFLT;
					v2->num.reg = nreg;
				}
			}
		}
	
		/* Se i registri sono di tipo diverso, converte il registro int in float */
		if (v1->type != v2->type) {
			int nreg;
	
			/* Chiedo un nuovo registro float */
			if ( (nreg = reg_occupy(TVAL_REGFLT)) == 0 ) return 0;
			
			/* Converto il registro intero in float */
			if (v1->type == TVAL_REGINTG) {
				op_ins2r(ins_itof, nreg, v1->num.reg);
				op_ins2r(fins, v2->num.reg, nreg);
				if (!reg_release(v1->type, v1->num.reg)) return 0;
				if (!reg_release(TVAL_REGFLT, nreg)) return 0;
				v1->type = TVAL_REGFLT;
				v1->num.reg = v2->num.reg;
				return 1;

			} else {
				op_ins2r(ins_itof, nreg, v2->num.reg);
				op_ins2r(fins, v1->num.reg, nreg);
				if (!reg_release(v2->type, v2->num.reg)) return 0;
				if (!reg_release(TVAL_REGFLT, nreg)) return 0;
				return 1;
			}
		} else {
			if (v1->type == TVAL_REGINTG) {
				/* Entrambi i registri sono interi */
				op_ins2r(iins, v1->num.reg, v2->num.reg);
				if (!reg_release(v2->type, v2->num.reg)) return 0;
				return 1;
			
			} else {
				/* Entrambi i registri sono floating point */
				op_ins2r(fins, v1->num.reg, v2->num.reg);
				if (!reg_release(v2->type, v2->num.reg)) return 0;
				return 1;
			}
		}
	}
	return 0;
}			

/* NOME: pars_pntop
 * DESCRIZIONE: Analoga a pars_numop, solo che tratta con valori di tipo point.
 */
int pars_pntop(value *v1, value *v2, long pins)
{
	if ((v1->type & v2->type & 0x80) != 0) {
		/* I due punti sono "immediati" */
		if (!op_imm2r(pins, & v1->num.pnt, & v2->num.pnt)) return 0;
		return 1;

	} else {
		/* Almeno uno dei due valori e' un registro */
		if (((v1->type ^ v2->type) & 0x80) == 0) {
			/* Ho a che fare con due registri */
			op_ins2r(pins, v1->num.reg, v2->num.reg);
			if (!reg_release(v2->type, v2->num.reg)) return 0;
			return 1;
			
		} else {
			/* Uno dei due valori e' un registro, l'altro e' immediato */
			
			/* Metto l'immediato in un registro */
			if (!pars_toreg(v1) || !pars_toreg(v2)) return 0;
			
			/* Eseguo l'operazione fra registri */
			op_ins2r(pins, v1->num.reg, v2->num.reg);
			if (!reg_release(v2->type, v2->num.reg)) return 0;
			return 1;
		}
	}
	return 0;
}

/* NOME: pars_2ntop
 * DESCRIZIONE: Converte due valori numerici(immediati o punti) in un valore
 *  di tipo punto(immediato se i due valori sono immediati, registro in caso
 *  contrario). Gli eventuali registri vengono liberati.
 */
int pars_2ntop(value *vp, value *vn1, value *vn2)
{
	if ((vn1->type & vn2->type & 0x80) != 0) {
		/* I due numeri sono immediati: se non sono float li converto! */
		if (vn1->type == TVAL_IMMINTG) {
			if (!op_imm2r(ins_itof, & vn1->num.flt, & vn1->num.intg)) return 0;
			vn1->type = TVAL_IMMFLT;
		}
		
		if (vn2->type == TVAL_IMMINTG) {
			if (!op_imm2r(ins_itof, & vn2->num.flt, & vn2->num.intg)) return 0;
			vn2->type = TVAL_IMMFLT;
		}
		
		/* Ora creo il punto(immediato) dai due valori immediati */
		if (!op_imm3r(ins_2ftop, &vp->num.pnt, &vn1->num.flt, &vn2->num.flt))
			return 0;
		vp->type = TVAL_IMMPNT;
		return 1;

	} else {
		/* Almeno uno dei due numeri e' un registro: devo mettere il punto
		 * in un registro
		 */
		long nreg;
		
		/* Metto i valori in registri di tipo float, se non lo sono gia' */
		if (!pars_ntofreg(vn1) || !pars_ntofreg(vn2)) return 0;
	
		/* Chiedo un nuovo registro di tipo point */
		if ( (nreg = reg_occupy(TVAL_REGPNT)) == 0 ) return 0;
		
		/* Eseguo la conversione in punto */
		op_ins3r(ins_2ftop, nreg, vn1->num.reg, vn2->num.reg);
		vp->type = TVAL_REGPNT;
		vp->num.reg = nreg;
		
		/* Libero i due registri */
		if (!reg_release(vn1->type, vn1->num.reg)) return 0;
		if (!reg_release(vn2->type, vn2->num.reg)) return 0;
		return 1;
	}

	return 0;
}

/* DESCRIZIONE: Converte un valore numerico in float, mantenendolo immediato
 *  se era immediato o registro se era registro.
 */
int pars_ntof(value *vn)
{
	if (vn->type == TVAL_IMMINTG) {
		vn->type = TVAL_IMMFLT;
		return op_imm2r(ins_itof, & vn->num.flt, & vn->num.intg);
	
	} else if (vn->type == TVAL_REGINTG) {
		return pars_ntofreg(vn);
	}
	
	return 1;
}

/* DESCRIZIONE: Esegue un'operazione fra un punto e un numero: il numero viene
 *  convertito in float se e' intero, poi viene eseguita l'operazione.
 *  Se l'operazione puo' essere svolta tra immediati, allora lo fa, evitando
 *  la creazione di nuovi registri temporanei.
 */
int pars_popf(value *vp, value *vn, long pinsf)
{
	int situation;

	/* Converto il numero in float se ce n'e' bisogno! */
	if (!pars_ntof(vn)) return 0;

	situation = ((vp->type == TVAL_IMMPNT) << 1) | (vn->type == TVAL_IMMFLT);
	
	/* Se uno dei due valori e' registro, devo convertire l'altro in registro
	 * (se non lo e' gia') e il risultato sara' un registro.
	 */
	switch (situation) {
	 case 1:	/* Numero immediato, ma punto no: metto numero in registro! */
		if (!pars_toreg(vn)) return 0;
		break;
	
	 case 2:	/* Punto immediato, ma numero no: metto punto in registro! */
		if (!pars_toreg(vp)) return 0;
		break;

	 case 3:	/* Tutti valori immediati: risultato immediato! */
		return op_imm2r(pinsf, & vp->num.pnt, & vn->num.flt);
		break;
	}

	/* Eseguo l'operazione fra registri */
	op_ins2r(pinsf, vp->num.reg, vn->num.reg);
	
	/* Distruggo il registro float */
	return reg_release(vn->type, vn->num.reg);
}

/* NOME: pars_ntofvar
 * DESCRIZIONE: Mette un valore numerico(intero o float, immediato o registro)
 *  nella variabile float var.
 * NOTA: Se il valore numerico e' un registro, lo libera. Ritorna 1 se tutto ok,
 *  0 altrimenti.
 */
int pars_ntofvar(value *var, value *num)
{
	switch (num->type) {
	 case TVAL_IMMFLT:
		op_ldf(var->num.reg, num->num.flt);
		return 1; break;
	
	 case TVAL_REGFLT:
		op_ins2r(ins_movf, var->num.reg, num->num.reg);
		if (!reg_release(num->type, num->num.reg)) return 0;
		return 1; break;
		
	 case TVAL_IMMINTG:
		if (!op_imm2r(ins_itof, & num->num.flt, & num->num.intg)) return 0;
		num->type = TVAL_IMMFLT;
		op_ldf(var->num.reg, num->num.flt);
		return 1; break;

	 case TVAL_REGINTG:
		op_ins2r(ins_itof, var->num.reg, num->num.reg);
		if (!reg_release(num->type, num->num.reg)) return 0;
		return 1; break;
	}

	return 0;
}

/* NOME: pars_ntoivar
 * DESCRIZIONE: Mette un valore numerico(intero o float, immediato o registro)
 *  nella variabile intera var.
 * NOTA: Se il valore numerico e' un registro, lo libera. Ritorna 1 se tutto ok,
 *  0 altrimenti.
 */
int pars_ntoivar(value *var, value *num)
{
	switch (num->type) {
	 case TVAL_IMMINTG:
		op_ldi(var->num.reg, num->num.intg);
		return 1; break;
		
	 case TVAL_REGINTG:
		op_ins2r(ins_movi, var->num.reg, num->num.reg);
		if (!reg_release(num->type, num->num.reg)) return 0;
		return 1; break;
		
	 case TVAL_IMMFLT:
		if (!op_imm2r(ins_ftoi, & num->num.intg, & num->num.flt)) return 0;
		num->type = TVAL_IMMINTG;
		op_ldi(var->num.reg, num->num.intg);
		return 1; break;
	
	 case TVAL_REGFLT:
		op_ins2r(ins_ftoi, var->num.reg, num->num.reg);
		if (!reg_release(num->type, num->num.reg)) return 0;
		return 1; break;	
	}

	return 0;
}

/* NOME: pars_ntovar
 * DESCRIZIONE: Crea una variabile dello stesso tipo del valore num e ci mette
 *  questo dentro.
 * NOTA: Se il valore numerico e' un registro, lo libera. Ritorna 1 se tutto ok,
 *  0 altrimenti.
 */
int pars_ntovar(name *var, value *num)
{
	long vnum;

	switch (num->type) {
	 case TVAL_IMMINTG:
		vnum = var_define(var->ptr, var->leng, TVAL_INTG, TVAL_NOSUBTYPE);
		if (vnum == 0) return 0;
		op_ldi(vnum, num->num.intg);
		return 1; break;
	
	 case TVAL_REGINTG:
		vnum = var_define(var->ptr, var->leng, TVAL_INTG, TVAL_NOSUBTYPE);
		if (vnum == 0) return 0;
		op_ins2r(ins_movi, vnum, num->num.reg);
		if (!reg_release(num->type, num->num.reg)) return 0;
		return 1; break;
	
	 case TVAL_IMMFLT:
		vnum = var_define(var->ptr, var->leng, TVAL_FLT, TVAL_NOSUBTYPE);
		if (vnum == 0) return 0;
		op_ldf(vnum, num->num.flt);
		return 1; break;
	
	 case TVAL_REGFLT:
		vnum = var_define(var->ptr, var->leng, TVAL_FLT, TVAL_NOSUBTYPE);
		if (vnum == 0) return 0;
		op_ins2r(ins_movf, vnum, num->num.reg);
		if (!reg_release(num->type, num->num.reg)) return 0;
		return 1; break;
	}

	return 0;
}

int parser_init(void)
{
	if (!op_init(8192)) return 0;
 	if (!reg_init(10, 10, 10, 10)) return 0;
	/* Numero massimo include = 10, file iniziale = no! */
	if (!tok_init( 10, (char *) NULL )) return 0;
 	return 1;
}

int parser_finish(void)
{
 	return 1;
}
