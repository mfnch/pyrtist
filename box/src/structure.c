/*****************************************************************************
 * The following part of this file handles structures.                       *
 *****************************************************************************/

static Int cmp_structure_type, cmp_structure_num, cmp_structure_size;
static Array *cmp_structure_exprs;
static Array *cmp_structure_data;

typedef struct {
  Int  num;  /* Numero dell'elemento */
  Int  size; /* Posizione dell'elemento nella struttura */
  Expr expr; /* Espressione contenente il valore dell'elemento */
} StructItem;

static Task Cmp__Structure_Free(StructItem *first, Int num);
static Task Cmp__Structure_Backgnd(StructItem *first, Int num, Int size,
                                   int *only_backgnd, int *only_foregnd);
static Task Cmp__Structure_Foregnd(Expr *new_struct, StructItem *first,
                                   Int num, int only_foregnd);

/* DESCRIPTION: This function starts the creation of a new structure.
 * NOTE: It is possible to build a new structure inside another.
 */
Task Cmp_Structure_Begin(void) {
  StructItem header;

  if ( cmp_structure_exprs == NULL ) {
    cmp_structure_exprs = Array_New(sizeof(StructItem), CMP_TYPICAL_STRUC_DIM);
    if ( cmp_structure_exprs == NULL ) return Failed;
    cmp_structure_type = TYPE_NONE;

  } else {
    TASK( Arr_Clear(cmp_structure_exprs) );
  }

  header.expr.type = cmp_structure_type;
  TASK( Arr_Push(cmp_structure_exprs, & header) );
  cmp_structure_type = TYPE_NONE;
  cmp_structure_num = 0;
  cmp_structure_size = 0;
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
  TASK( Tym_Def_Structure(& cmp_structure_type, t) );

  /* Memorizzo l'espressione *e per poterla trattare
   * in seguito in Cmp_Structure_End
   */
  si.num = ++cmp_structure_num;
  si.size = (cmp_structure_size += item_size);
  si.expr = *e;
  return Arr_Push(cmp_structure_exprs, & si);
}

/* DESCRIPTION: This function ends the creation of the current structure.
 */
Task Cmp_Structure_End(Expr *new_struct) {
  StructItem *si, *first;
  int only_backgnd, only_foregnd;
  Int num = cmp_structure_num, size = cmp_structure_size,
      type = cmp_structure_type;

  assert(cmp_structure_exprs != NULL);

  if ( num < 1 ) {
    MSG_ERROR("Trying to create an empty structure.");
    return Failed;
  }

  si = Arr_LastItemPtr(cmp_structure_exprs, StructItem);
  assert( (si->num == num) && (si->size == size) );
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
     Cmp_Imm_Add(type, Arr_FirstItemPtr(cmp_structure_data, void), size);
    if ( imm_addr < 0 ) return Failed;

  } else {
    /* Alloco lo spazio dove mettero' la struttura */
    Expr_Container_New(new_struct, type, CONTAINER_LREG_AUTO);
    Expr_Alloc(new_struct);

    /* Copio il background della struttura, se presente */
    if ( ! only_foregnd ) {
      Int struct_addr;
      /* Trasferisco il background nel segmento dati */
      struct_addr = Cmp_Data_Add(type,
       Arr_FirstItemPtr(cmp_structure_data, void), size);
      if ( struct_addr < 0 ) return Failed;
      Cmp_Assemble(ASM_MOV_OO, CAT_LREG, 0, CAT_GREG, 0);
      Cmp_Assemble(ASM_LEA_OO, CAT_LREG, 0, CAT_PTR, struct_addr);
      Cmp_Assemble(ASM_MOV_II, CAT_LREG, 0, CAT_IMM, size);
      Cmp_Assemble(ASM_MCOPY_OO, new_struct->categ, new_struct->value.i,
       CAT_LREG, 0);
    }

    TASK( Cmp__Structure_Foregnd(new_struct, first, num, only_foregnd) );
  }

  /* Expr are destroyed in Cmp__Structure_Foregnd */
  //TASK( Cmp__Structure_Free(first, cmp_structure_num) );

  if ( cmp_structure_num + 1 > Arr_NumItem(cmp_structure_exprs) ) {
    TASK( Arr_MDec(cmp_structure_exprs, cmp_structure_num) );
    si = Arr_LastItemPtr(cmp_structure_exprs, StructItem);
    cmp_structure_type = si->expr.type;
    TASK( Arr_Dec(cmp_structure_exprs) );
    si = Arr_LastItemPtr(cmp_structure_exprs, StructItem);
    cmp_structure_num = si->num;
    cmp_structure_size = si->size;
    return Success;

  } else {
    assert(cmp_structure_num + 1 == Arr_NumItem(cmp_structure_exprs));
    cmp_structure_num = TYPE_NONE;
    cmp_structure_num = 0;
    cmp_structure_size = 0;
    return Arr_Clear(cmp_structure_exprs);
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

  /* Se non e' gia' stato fatto, creo l'array che conterra'
   * "l'immagine di sfondo" della struttura (cioe' la sua parte immediata)
   */
  if ( cmp_structure_data == NULL ) {
    cmp_structure_data = Array_New(sizeof(char), CMP_TYPICAL_STRUC_SIZE);
    if ( cmp_structure_data == NULL ) return Failed;
  } else {
    TASK( Arr_Clear(cmp_structure_data) );
  }

  /* Mi assicuro che riesca a contenere tutta la struttura
   * (cosi' evito inutili riallocazioni)if ( Cmp_Structure_Add(& $1) ) MY_ERR
   */
  TASK( Arr_BigEnough(cmp_structure_data, size) );

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
        TASK( Arr_MPush(cmp_structure_data, & e->value, size_of_type[t]) );

      } else {
        char *imm_addr;
        imm_addr = Arr_ItemPtr(cmp_structure_data, char, e->addr);
        TASK( Arr_MPush(cmp_structure_data, imm_addr, arg_size) );
      }

    } else {
      *only_backgnd = 0;
      TASK( Arr_Append_Blank(cmp_structure_data, si->size) );
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
      TASK( Cmp_Expr_Move(& e_dest, e_src) );
    } else {
      TASK( Cmp_Expr_Destroy_Tmp(e_src) );
    }

    arg_size_old = si->size;
    ++si;
  }

  return Success;
}

/* DESCRIPTION: This function frees the expression that were collected
 *  to build the structure.
 */
static Task Cmp__Structure_Free(StructItem *first, Int num) {
  Int i;
  for(i = 0; i < num; i++) {
    TASK( Cmp_Expr_Destroy_Tmp(& first->expr) );
    ++first;
  }
  return Success;
}

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
      int n1, n2;
      Expr new_struc, member1, member2, member2_copy;
      Expr_Container_New(& new_struc, type1, CONTAINER_LREG_AUTO);
      Expr_Alloc(& new_struc);
      member1 = new_struc;
      member2 = *e;

      TASK(Cmp_Structure_Get(& member1, & n1));
      TASK(Cmp_Structure_Get(& member2, & n2));
      while (n1 > 0) {
        member2_copy = member2;
#ifdef DEBUG_STRUCT_EXPANSION
        printf("Espando il membro: '%s' --> '%s'\n",
               Tym_Type_Names(member2.type), Tym_Type_Names(member1.type));
#endif
        TASK( Cmp_Expr_Expand(member1.type, & member2_copy) );
#ifdef DEBUG_STRUCT_EXPANSION
        printf("Dopo l'espansione: type='%s', resolved='%s'\n",
               Tym_Type_Names(member2_copy.type),
               Tym_Type_Names(member2_copy.resolved));
#endif
        TASK( Cmp_Expr_Move(& member1, & member2_copy) );

        TASK( Cmp_Structure_Get(& member1, & n1) );
        TASK( Cmp_Structure_Get(& member2, & n2) );
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

/*  An example of use (see also Tym_Structure_Get):
 *  Suppose that the Expr structure is a structure,
 *     Expr member = structure; int n;
 *     do {TASK( Cmp_Structure_Get(& member, & n) );} while (n > 0);
 *  During the loop, member will contain each of the members of the structure:
 *  the first, the second, ..., the last.
 *  The first time Cmp_Structure_Get is called, it sets n=[number of members].
 *  When it is called again, it decreases n, until it is equal to 0.
 * NOTE: *member shouldn't change between the calls to Cmp_Structure_Get!
 *****************************************************************FULL EXAMPLE:
 *   Expr member = structure; int n;
 *   TASK(Cmp_Structure_Get(& member, & n));
 *   while (n > 0) {
 *     ... // <-- here we can use the Expression member
 *     TASK(Cmp_Structure_Get(& member, & n));
 *   };
 */
Task Cmp_Structure_Get(Expr *member, int *n) {
  Int member_type = member->type;
  TASK( Tym_Structure_Get(& member_type) );

  if (TS_Kind(cmp->ts, member->type) == TS_KIND_STRUCTURE) {
    Expr first;

    /* (sotto) Manca parte dell'implementazione! */
    assert(member->categ == CAT_LREG || member->categ == CAT_GREG);

    if ( (*n = Tym_Struct_Get_Num_Items(member->type)) == 0 ) return Success;
    assert( *n > 0 );

    first.categ = CAT_PTR;
    first.is.imm = 0; first.is.value = 1; first.is.typed = 1;
    first.is.ignore = 1; first.is.target = 1;
    first.is.allocd = 0; /* <-- importanti!!! */
    first.is.release = 0;
    first.addr = member->value.i;
    first.type = member_type;
    first.resolved = Tym_Type_Resolve_All(member_type);
    first.is.gaddr = (member->categ == CAT_GREG) ? 1 : 0;
    first.value.i = 0;
    *member = first;
    return Success;

  } else {
    assert( *n > 0 );
    --(*n);
    /* (n == 0) if and only if (member_type == TYPE_NONE) */
    if ( (*n == 0) != (member_type == TYPE_NONE) ) {
      MSG_FATAL("Cmp_Structure_Get: errore interno: *n = %d, member_type = %d",
                *n, member_type);
      return Failed;
    }
    if (n == 0) return Success;
    member->value.i += Tym_Type_Size(member->resolved);
    member->type = member_type;
    member->resolved = Tym_Type_Resolve_All(member_type);
  }
  return Success;
}


