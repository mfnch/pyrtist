/*****************************************************************************
 * The following part of this file handles structures.                       *
 *****************************************************************************/

static Task Cmp__Structure_Backgnd(StructItem *first, Int num, Int size,
                                   int *only_backgnd, int *only_foregnd);
static Task Cmp__Structure_Foregnd(Expr *new_struct, StructItem *first,
                                   Int num, int only_foregnd);

/* DESCRIPTION: This function starts the creation of a new structure.
 * NOTE: It is possible to build a new structure inside another.
 */
Task Cmp_Structure_Begin(void) {
  StructItem header;
  header.expr.type = cmp->struc.type;
  BoxArr_Clear(& cmp->struc.exprs);
  BoxArr_Push(& cmp->struc.exprs, & header);
  cmp->struc.type = TYPE_NONE;
  cmp->struc.num = 0;
  cmp->struc.size = 0;
  return Success;
}

/* DESCRIPTION: This function adds a new element to the structure
 *  which is currently being constructed.
 */
Task Cmp_Structure_Add(Expr *e) {
  StructItem si;
  Int t, item_size;

  TASK( Expr_Must_Have_Value(e) );

  assert(e->is.typed && e->is.value);
  item_size = Tym_Type_Size(e->type);
  if ( item_size < 1 ) {
    assert(item_size == 0);
    MSG_ERROR("Cannot add to the structure a zero-sized object "
              " (with type '%s')", Tym_Type_Name(e->type));
    return Failed;
  }

  /* Creo un tipo opportuno associato alla struttura */
  e->type = t = Tym_Type_Resolve_All(e->type);
  TASK( Tym_Def_Structure(& cmp->struc.type, t) );

  /* Memorizzo l'espressione *e per poterla trattare
   * in seguito in Cmp_Structure_End
   */
  si.num = ++cmp->struc.num;
  si.size = (cmp->struc.size += item_size);
  si.expr = *e;
  BoxArr_Push(& cmp->struc.exprs, & si);
  return Success;
}

/* DESCRIPTION: This function ends the creation of the current structure.
 */
Task Cmp_Structure_End(Expr *new_struct) {
  StructItem *si, *first;
  int only_backgnd, only_foregnd;
  Int num = cmp->struc.num, size = cmp->struc.size, type = cmp->struc.type;

  if (num < 1) {
    MSG_ERROR("Trying to create an empty structure.");
    return Failed;
  }

  si = (StructItem *) BoxArr_Last_Item_Ptr(& cmp->struc.exprs);
  assert(si->num == num && si->size == size);
  first = si - (num - 1);

  /* Creo la parte immediata della struttura */
  TASK( Cmp__Structure_Backgnd(first, num, size,
                               & only_backgnd, & only_foregnd) );
  only_backgnd = 0;

  if ( only_backgnd ) {
    Int imm_addr;
    /* La struttura e' interamente immediata */
    new_struct->categ = CAT_IMM;
    new_struct->type = type;
    new_struct->addr = imm_addr =
     Cmp_Imm_Add(cmp, type, BoxArr_First_Item_Ptr(& cmp->struc.data), size);
    if (imm_addr < 0) return Failed;

  } else {
    /* Alloco lo spazio dove mettero' la struttura */
    Expr_Container_New(new_struct, type, CONTAINER_LREG_AUTO);
    Expr_Alloc(new_struct);

    /* Copio il background della struttura, se presente */
    if ( ! only_foregnd ) {
      Int struct_addr;
      /* Trasferisco il background nel segmento dati */
      struct_addr = BoxVM_Data_Add(cmp->vm,
                                   BoxArr_First_Item_Ptr(& cmp->struc.data),
                                   size, type);
      if ( struct_addr < 0 ) return Failed;
      Cmp_Assemble(ASM_MOV_OO, CAT_LREG, 0, CAT_GREG, 0);
      Cmp_Assemble(ASM_LEA_OO, CAT_LREG, 0, CAT_PTR, struct_addr);
      Cmp_Assemble(ASM_MOV_II, CAT_LREG, 0, CAT_IMM, size);
      Cmp_Assemble(ASM_MCOPY_OO, new_struct->categ, new_struct->value.i,
       CAT_LREG, 0);
    }

    TASK( Cmp__Structure_Foregnd(new_struct, first, num, only_foregnd) );
  }

  if (cmp->struc.num + 1 > BoxArr_Num_Items(& cmp->struc.exprs) ) {
    BoxArr_MPop(& cmp->struc.exprs, NULL, cmp->struc.num);
    si = (StructItem *) BoxArr_Last_Item_Ptr(& cmp->struc.exprs);
    cmp->struc.type = si->expr.type;
    BoxArr_Pop(& cmp->struc.exprs, NULL);
    si = (StructItem *) BoxArr_Last_Item_Ptr(& cmp->struc.exprs);
    cmp->struc.num = si->num;
    cmp->struc.size = si->size;
    return Success;

  } else {
    assert(cmp->struc.num + 1 == BoxArr_Num_Items(& cmp->struc.exprs));
    cmp->struc.num = TYPE_NONE;
    cmp->struc.num = 0;
    cmp->struc.size = 0;
    BoxArr_Clear(& cmp->struc.exprs);
    return Success;
  }
}

/* DESCRIPTION: This function creates the immediate part of the structure
 *  (into the array cmp_structure_data).
 * NOTE: If the array is entirely immediate the function sets
 *  *only_backgnd = 1 (in other cases only_backgnd = 0).
 */
static Task Cmp__Structure_Backgnd(StructItem *first, Int num, Int size,
                                   int *only_backgnd, int *only_foregnd) {
  Int i, arg_size, arg_size_old;
  StructItem *si;

  /* Empty the structure containing the immediate background */
  BoxArr_Clear(& cmp->struc.data);

  /* Mi assicuro che riesca a contenere tutta la struttura
   * (cosi' evito inutili riallocazioni)if ( Cmp_Structure_Add(& $1) ) MY_ERR
   *
  TASK( Arr_BigEnough(& cmp->struc.data, size) ); */

  /* Scorro le espressioni che compongono la struttura
   * alla ricerca di quelle immediate
   */
  *only_backgnd = 1;
  *only_foregnd = 1;
  si = first;
  arg_size_old = 0;
  for(i = 0; i < num; i++) {
    Expr *e = & si->expr;

    arg_size = si->size - arg_size_old;
    arg_size_old = si->size;

    if ( e->categ == CAT_IMM ) {
      /* Remember that e->type is already resolved (see Cmp_Structure_Add)*/
      Int t = e->type;
      *only_foregnd = 0;
      if ( t < NUM_INTRINSICS ) {
        assert( (t >= 0) && (t < NUM_TYPES) );
        BoxArr_MPush(& cmp->struc.data, & e->value, size_of_type[t]);

      } else {
        char *imm_addr;
        imm_addr = (char *) BoxArr_Item_Ptr(& cmp->struc.data, e->addr);
        BoxArr_MPush(& cmp->struc.data, imm_addr, arg_size);
      }

    } else {
      *only_backgnd = 0;
      BoxArr_MPush(& cmp->struc.data, NULL, si->size);
    }

    ++si;
  }

  return Success;
}

/* DESCRIPTION: This function creates the non-immediate part of the structure.
 *  It fills all the members of the structure which have not a definite value
 *  at compile-time.
 */
static Task Cmp__Structure_Foregnd(Expr *new_struct, StructItem *first,
                                   Int num, int only_foregnd) {
  Int i, arg_size, arg_size_old;
  StructItem *si;
  Expr e_dest;

  assert( (new_struct->categ == CAT_LREG)
   || (new_struct->categ == CAT_GREG) );

  e_dest.categ = CAT_PTR;
  e_dest.is.imm = 0; e_dest.is.value = 1; e_dest.is.typed = 1;
  e_dest.is.ignore = 1; e_dest.is.target = 1;
  e_dest.is.gaddr = (new_struct->categ == CAT_GREG) ? 1 : 0;
  e_dest.addr = new_struct->value.i;

  si = first;
  arg_size_old = 0;
  for(i = 0; i < num; i++) {
    Expr *e_src = & si->expr;

    if ( (e_src->categ != CAT_IMM) || only_foregnd ) {
      arg_size = si->size - arg_size_old;
      e_dest.type = e_src->type;
      e_dest.resolved = e_src->resolved;
      e_dest.value.i = arg_size_old;
      TASK( Expr_Move(& e_dest, e_src) );
    } else {
      TASK( Cmp_Expr_Destroy_Tmp(e_src) );
    }

    arg_size_old = si->size;
    ++si;
  }

  return Success;
}

#if 0
/* DESCRIPTION: This function frees the expression that were collected
 *  to build the structure.
 * shouldn't be necessary, since this is now done by Cmp__Structure_Foregnd
 */
static Task Cmp__Structure_Free(StructItem *first, Int num) {
  Int i;
  for(i = 0; i < num; i++) {
    TASK( Cmp_Expr_Destroy_Tmp(& first->expr) );
    ++first;
  }
  return Success;
}
#endif

/* DESCRIPTION: This function converts an expression into the greater element
 *  of the species it belongs to. For example if *e is an Int, and Scalar is
 *  the species (Real < Int < Char), then:
 *    Cmp_Expr_Expand( Scalar --> species, 1 --> e )
 *  will convert the expression *e into a real valued expression.
 */

/*
Scalar = (Real < Int < Char)
species e' il tipo (Point < (Scalar, Scalar))
e ha tipo (Int , Real)
---

 */

Task Cmp_Expr_Expand(Int species, Expr *e) {
  Int type1, type2;

  assert(e->is.typed && e->is.value);

  type1 = species;
  type2 = e->type;

#ifdef DEBUG_SPECIES_EXPANSION
  printf("Espando: '%s' --> '%s'\n",
         Tym_Type_Names(e->type), Tym_Type_Names(species));
#endif

  if (type1 == type2) return Success;
  type1 = Tym_Type_Resolve_Alias(type1);
  type2 = Tym_Type_Resolve_All(type2);
  if (type1 == type2) return Success;

  switch(TS_Kind(cmp->ts, type1)) {
  case TS_KIND_INTRINSIC: /* type1 != type2 !!! */
    MSG_ERROR("Type forbidden in species conversions.");
    return Failed;

  case TS_KIND_POINTER:
    MSG_ERROR("Not implemented yet!"); return Failed;
    /*if (td2->tot == TOT_PTR_TO) break;
    return 0;*/

  case TS_KIND_ARRAY:
    switch(TS_Compare(cmp->ts, type1, type2)) {
    case TS_TYPES_EQUAL:
    case TS_TYPES_MATCH:
      return Success;
    case TS_TYPES_EXPAND:
      MSG_ERROR("Expansion of array of species is not implemented yet!");
      return Failed;
    default:
      MSG_ERROR("Cmp_Expr_Expand: Expansion to array involves "
                "an incompatible type.");
      return Failed;
    }

  case TS_KIND_SPECIES: {
      Int member_type = type1;
      Int target_type = Tym_Specie_Get_Target(type1);
      TASK(Tym_Specie_Get(& member_type));
      while (member_type != TYPE_NONE) {
        if ( Tym_Compare_Types(member_type, type2, NULL) ) {
          TASK( Cmp_Expr_Expand(member_type, e) );
          return Cmp_Conversion(e->type, target_type, e);
        }
        TASK( Tym_Specie_Get(& member_type) );
      };
      MSG_ERROR("Cannot expand the species!");
      return Failed;
    }

  case TS_KIND_PROC:
    MSG_ERROR("Not implemented yet!"); return Failed;
    /*if (td2->tot != TOT_PROCEDURE) return 0;
    return (
          Tym_Compare_Types(td1->parent, td2->parent)
      && Tym_Compare_Types(td1->target, td2->target) );*/

  case TS_KIND_STRUCTURE: {
    int need_expansion = 0;

    /* Prima di eseguire la conversione verifico che si possa
      * effettivamente fare!
      */
    if ( ! Tym_Compare_Types(type1, type2, & need_expansion) ) {
      MSG_ERROR("Cmp_Expr_Expand: "
                "Expansion involves incompatible types!");
      return Failed;
    }

    /* We have to expand the structure: we have to create a new structure
     * which can contain the expanded one.
     */
    if (need_expansion) {
      int src_n, dest_n;
      Expr new_struc, src_iter_e, dest_iter_e, src_e, dest_e;
      Expr_Container_New(& new_struc, type1, CONTAINER_LREG_AUTO);
      Expr_Alloc(& new_struc);
      src_iter_e = *e;
      dest_iter_e = new_struc;

      Expr_Struc_Iter(& dest_e, & dest_iter_e, & dest_n);
      Expr_Struc_Iter(& src_e, & src_iter_e, & src_n);
      while (dest_n > 0) {
        TASK( Cmp_Expr_Expand(dest_e.type, & src_e) );
        TASK( Expr_Move(& dest_e, & src_e) );

        Expr_Struc_Iter(& src_e, & src_iter_e, & src_n);
        Expr_Struc_Iter(& dest_e, & dest_iter_e, & dest_n);
      };

      TASK( Cmp_Expr_Destroy_Tmp(e) );
      *e = new_struc;
    }
    return Success;
    }

  default:
    MSG_ERROR("Cmp_Expr_Expand not fully implemented!");
    return Failed;
  }

  return Failed;
}
