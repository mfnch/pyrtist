
/* DESCRIZIONE: Crea una espressione di tipo "target" locale, cioe' che puo'
 *  essere il target di un'operazione di assegnazione. Quindi a tale espressione
 *  corrisponde una variabile locale di tipo type.
 */
Task Cmp_Expr_Target_New(Expression *e, Intg type) {
  MSG_LOCATION("Cmp_Expr_New_Target");
  assert( (type >= 0) || (type < NUM_TYPES) );

  if (type < CMP_PRIVILEGED) {
    e->is.imm = 0;
    e->is.value = 1;
    e->is.ignore = 0;
    e->is.target = 1;
    e->is.typed = 1;

    e->type = type;
    e->categ = CAT_LREG;

    if ( (e->value.i = -Var_Occupy(type, cmp_box_level)) < 0 )
      return Success;
    return Failed;

  } else {
    TypeDesc *td;

    e->is.imm = 0;
    e->is.value = 1;
    e->is.ignore = 0;
    e->is.target = 1;
    e->is.typed = 1;

    e->type = type;
    e->categ = CAT_LREG;
    if ( (e->value.i = -Var_Occupy(TYPE_OBJ, cmp_box_level)) >= 0 )
      return Failed;

    td = Tym_Type_Get(type);
    if ( td == NULL ) return Failed;

    VM_Assemble(ASM_MALLOC_I, CAT_IMM, td->size);
    VM_Assemble(ASM_MOV_OO, e->categ, e->value.i, CAT_LREG, 0);
    return Success;
  }
}

Task Cmp_Expr_LReg(Expression *e, Intg type, int zero) {
  MSG_LOCATION("Cmp_Expr_LReg");

  e->is.imm = 0;
  e->is.value = 1;
  e->is.ignore = 0;
  e->is.target = 0;
  e->is.typed = 1;

  e->categ = CAT_LREG;
  e->type = type;

  if (type < CMP_PRIVILEGED) {
    if ( zero ) { e->value.i = 0; return Success; }
    if ( (e->value.i = Reg_Occupy(type)) >= 1 ) return Success;
    return Failed;

  } else {
    TypeDesc *td;

    td = Tym_Type_Get(type);
    if ( td == NULL ) return Failed;

    {
      register Intg s = td->size;
      if ( s > 0 ) {
        if ( zero ) { e->value.i = 0; }
         else { if ( (e->value.i = Reg_Occupy(TYPE_OBJ)) < 1 ) return Failed; }

        VM_Assemble(ASM_MALLOC_I, CAT_IMM, s);
        VM_Assemble(ASM_MOV_OO, e->categ, e->value.i, CAT_LREG, 0);
      }
    }
    return Success;
  }
  return Failed;
}











/* DESCRIPTION:
 */
Task Cmp_Expr_Create(Expression *e, Intg type, int temporary) {
  MSG_WARNING("Cmp_Expr_Create non ancora implementata!")
  return Success;
}

/* DESCRIPTION:
 */
Task Cmp_Expr_Destroy(Expression *e) {
  MSG_WARNING("Cmp_Expr_Destroy non ancora implementata!")
  return Success;
}







     case TOT_STRUCTURE: {
      Expression new;
      register Intg t1 = td1->target, t2 = td2->target;
      int must_expand;

      if ( td2->tot != TOT_STRUCTURE ) goto type_mismatch;

      /* Ora cerco di capire se e' necessaria la conversione della struttura:
       * se la dimensione di e differisce da quella di specie, lo e'!
       */
      must_expand = 1;
      if ( td1->size == td2->size ) {
        tym_must_expand = 0;
        if ( ! Tym_Compare_Types(type1, type2) ) goto type_mismatch;
        must_expand = tym_must_expand;
      }

      TASK( Cmp_Expr_Create(& new, type1, 1) );

        do {
          td1 = Tym_Type_Get(t1);
          td2 = Tym_Type_Get(t2);
          if ( (td1 == NULL) || (td2 == NULL) ) {
            MSG_ERROR("Array dei tipi danneggiata(2)!");
            return Failed;
          }

          if ( ! Tym_Compare_Types(t1, t2) ) return 0;
          t1 = td1->greater;
          t2 = td2->greater;
        } while( (t1 != TYPE_NONE) && (t2 != TYPE_NONE) );
        if ( t1 == t2 ) return 1;
        return 0;*/
      }













