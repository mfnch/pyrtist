/*****************************************************************************
 * The following part of this file handles structures.                       *
 *****************************************************************************/

static Intg cmp_structure_type, cmp_structure_num, cmp_structure_size;
static Array *cmp_structure_exprs;
static Array *cmp_structure_data;

typedef struct {
  Intg       num;  /* Numero dell'elemento */
  Intg       size; /* Posizione dell'elemento nella struttura */
  Expression expr; /* Espressione contenente il valore dell'elemento */
} StructItem;

static Task Cmp__Structure_Free(StructItem *first, Intg num);
static Task Cmp__Structure_Backgnd(StructItem *first, Intg num,
 Intg size, int *only_backgnd, int *only_foregnd);
static Task Cmp__Structure_Foregnd(Expression *new_struct, StructItem *first,
 Intg num, int only_foregnd);

/* DESCRIPTION: This function starts the creation of a new structure.
 * NOTE: It is possible to build a new structure inside another.
 */
Task Cmp_Structure_Begin(void) {
  StructItem header;
  MSG_LOCATION("Cmp_Structure_Begin");

  if ( cmp_structure_exprs == NULL ) {
    cmp_structure_exprs = Array_New(sizeof(StructItem), CMP_TYPICAL_STRUC_DIM);
    if ( cmp_structure_exprs == NULL ) return Failed;
    cmp_structure_type = TYPE_NONE;
  }

  header.expr.type = cmp_structure_type;
  TASK( Arr_Push(cmp_structure_exprs, & header) );
  cmp_structure_type = TYPE_NONE;
  cmp_structure_num = 0;
  cmp_structure_size = 0;
  return Success;
}

/* DESCRIPTION: This function adds a new elements to the structure
 *  which is currently being constructed.
 */
Task Cmp_Structure_Add(Expression *e) {
  StructItem si;
  register Intg t, item_size;

  assert(e->is.typed && e->is.value);
  item_size = Tym_Type_Size(e->type);
  if ( item_size < 1 ) {
    assert(item_size == 0);
    MSG_ERROR("Impossibile aggiungere alla struttura"
     "un oggetto di dimensione 0 (tipo '%s')", Tym_Type_Name(e->type));
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
Task Cmp_Structure_End(Expression *new_struct) {
  StructItem *si, *first;
  int only_backgnd, only_foregnd;
  Intg num = cmp_structure_num, size = cmp_structure_size,
   type = cmp_structure_type;

  assert(cmp_structure_exprs != NULL);

  if ( num < 1 ) {
    MSG_ERROR("Tentativo di creare una struttura vuota.");
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
    register Intg imm_addr;
    /* La struttura e' interamente immediata */
    new_struct->categ = CAT_IMM;
    new_struct->type = type;
    new_struct->addr = imm_addr =
     Cmp_Imm_Add(type, Arr_FirstItemPtr(cmp_structure_data, void), size);
    if ( imm_addr < 0 ) return Failed;

  } else {
    /* Alloco lo spazio dove mettero' la struttura */
    TASK( Cmp_Expr_Create(new_struct, type, 1) );

    /* Copio il background della struttura, se presente */
    if ( ! only_foregnd ) {
      register Intg struct_addr;
      /* Trasferisco il background nel segmento dati */
      struct_addr = Cmp_Data_Add(type,
       Arr_FirstItemPtr(cmp_structure_data, void), size);
      if ( struct_addr < 0 ) return Failed;
      VM_Assemble(ASM_MOV_OO, CAT_LREG, 0, CAT_GREG, 0);
      VM_Assemble(ASM_LEA_OO, CAT_LREG, 0, CAT_PTR, struct_addr);
      VM_Assemble(ASM_MOV_II, CAT_LREG, 0, CAT_IMM, size);
      VM_Assemble(ASM_MCOPY_OO, new_struct->categ, new_struct->value.i,
       CAT_LREG, 0);
    }

    TASK( Cmp__Structure_Foregnd(new_struct, first, num, only_foregnd) );
  }

  /* Expression are destroyed in Cmp__Structure_Foregnd */
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
static Task Cmp__Structure_Backgnd(StructItem *first, Intg num, Intg size,
 int *only_backgnd, int *only_foregnd) {
  register Intg i, arg_size, arg_size_old;
  register StructItem *si;

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
    register Expression *e = & si->expr;

    arg_size = si->size - arg_size_old;
    arg_size_old = si->size;

    if ( e->categ == CAT_IMM ) {
      /* Remember that e->type is already resolved (see Cmp_Structure_Add)*/
      register Intg t = e->type;
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
      TASK( Arr_Empty(cmp_structure_data, si->size) );
    }

    ++si;
  }

  return Success;
}

/* DESCRIPTION: This function creates the non-immediate part of the structure.
 *  It fills all the members of the structure which have not a definite value
 *  at compile-time.
 */
static Task Cmp__Structure_Foregnd(
 Expression *new_struct, StructItem *first, Intg num, int only_foregnd) {

  register Intg i, arg_size, arg_size_old;
  register StructItem *si;
  Expression e_dest;

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
    register Expression *e_src = & si->expr;

    if ( (e_src->categ != CAT_IMM) || only_foregnd ) {
      arg_size = si->size - arg_size_old;
      e_dest.type = e_src->type;
      e_dest.resolved = e_src->resolved;
      e_dest.value.i = arg_size_old;
      TASK( Cmp_Expr_Move(& e_dest, e_src) );
    } else {
      TASK( Cmp_Expr_Destroy(e_src) );
    }

    arg_size_old = si->size;
    ++si;
  }

  return Success;
}

/* DESCRIPTION: This function frees the expression that were collected
 *  to build the structure.
 */
static Task Cmp__Structure_Free(StructItem *first, Intg num) {
  register Intg i;
  for(i = 0; i < num; i++) {
    TASK( Cmp_Expr_Destroy(& first->expr) );
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

Task Cmp_Expr_Expand(Intg species, Expression *e) {
  int first;
  TypeDesc *td1, *td2;
  register Intg type1, type2;

  assert( e->is.typed && e->is.value );

  type1 = species;
  type2 = e->type;

  first = 1;

#ifdef DEBUG_SPECIES_EXPANSION
  printf("Espando: '%s' --> '%s'\n",
   Tym_Type_Names(e->type), Tym_Type_Names(species));
#endif

  for(;;) {
    if ( type1 == type2 ) return Success;
    type1 = Tym_Type_Resolve_Alias(type1);
    type2 = Tym_Type_Resolve_All(type2);
    if ( type1 == type2 ) return Success;

    td1 = Tym_Type_Get(type1);
    td2 = Tym_Type_Get(type2);
    if ( (td1 == NULL) || (td2 == NULL) ) {
      MSG_ERROR("Tipi errati in Cmp_Expr_Expand!");
      return Failed;
    }

    switch( td1->tot ) {
     case TOT_INSTANCE: /* type1 != type2 !!! */
      MSG_ERROR("Tipo non ammesso per la conversione della specie.");
      return Failed;

     case TOT_PTR_TO:
      MSG_ERROR("Non ancora implementato!"); return Failed;
      /*if (td2->tot == TOT_PTR_TO) break;
      return 0;*/

     case TOT_ARRAY_OF:
      MSG_ERROR("Non ancora implementato!"); return Failed;
      /*if (td2->tot != TOT_ARRAY_OF) return 0;
      {
        register Intg td1s = td1->arr_size, td2s = td2->arr_size;
        if (td1s == td2s) break;
        if ((td1s < 1) && first) break;
        return 0;
      }*/

     case TOT_SPECIE: {
        register Intg t = td1->target, great_type = td1->greater;
        do {
          td1 = Tym_Type_Get(t);
          if ( td1 == NULL ) {
            MSG_ERROR("Array dei tipi danneggiata!");
            return Failed;
          }
          if ( Tym_Compare_Types(t, type2, NULL) ) {
            TASK( Cmp_Expr_Expand(t, e) );
            return Cmp_Conversion(e->type, great_type, e);
          }
          t = td1->greater;
        } while( td1->greater != TYPE_NONE );
      }
      MSG_ERROR("Impossibile espandere la specie!");
      return Failed;

     case TOT_PROCEDURE:
      MSG_ERROR("Non ancora implementato!"); return Failed;
      /*if (td2->tot != TOT_PROCEDURE) return 0;
      return (
           Tym_Compare_Types(td1->parent, td2->parent)
        && Tym_Compare_Types(td1->target, td2->target) );*/

     case TOT_STRUCTURE: {
       int need_expansion = 0;

      /* Prima di eseguire la conversione verifico che si possa
       * effettivamente fare!
       */
      if ( ! Tym_Compare_Types(type1, type2, & need_expansion) ) {
        MSG_ERROR("Cmp_Expr_Expand: "
         "Espansione fra tipi non compatibili!");
        return Failed;
      }

      /* Ora dobbiamo espandere la struttura, ma solo se necessario! */
      if ( ! need_expansion ) return Success;

      /* E' necessario: bisogna creare una nuova struttura per contenere
       * il risultato dell'espansione
       */
      {
        int n1, n2;
        Expression new_struc, member1, member2, member2_copy;
        TASK( Cmp_Expr_Create(& new_struc, type1, 1) );
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
           Tym_Type_Names(member2_copy.type), Tym_Type_Names(member2_copy.resolved));
#endif
          TASK( Cmp_Expr_Move(& member1, & member2_copy) );

          TASK( Cmp_Structure_Get(& member1, & n1) );
          TASK( Cmp_Structure_Get(& member2, & n2) );
        };

        TASK( Cmp_Expr_Destroy(e) );
        *e = new_struc;
      }
      return Success;
      }

     default:
      MSG_ERROR("Cmp_Expr_Expand non implementata fino in fondo!");
      return 0;
    }

    first = 0;
    type1 = td1->target;
    type2 = td2->target;
  }

  return Failed;
}

/* DESCRIPTION: An example of use (see also Tym_Structure_Get):
 *  Suppose that the Expression structure is a structure,
 *     Expression member = structure; int n;
 *     do {TASK( Cmp_Structure_Get(& member, & n) );} while (n > 0);
 *  During the loop, member will contain each of the members of the structure:
 *  the first, the second, ..., the last.
 *  The first time Cmp_Structure_Get is called, it sets n=[number of members].
 *  When it is called again, it decreases n, until it is equal to 0.
 * NOTE: *member shouldn't change between the calls to Cmp_Structure_Get!
 *****************************************************************FULL EXAMPLE:
 *   Expression member = structure; int n;
 *   TASK(Cmp_Structure_Get(& member, & n));
 *   while (n > 0) {
 *     ... // <-- here we can use the Expression member
 *     TASK(Cmp_Structure_Get(& member, & n));
 *   };
 */
Task Cmp_Structure_Get(Expression *member, int *n) {
  Intg member_type = member->type;
  TASK( Tym_Structure_Get(& member_type) );
  /* Now tym_recent_typedesc is the descriptor of member->resolved */

  if ( tym_recent_typedesc->tot == TOT_STRUCTURE ) {
    Expression first;

    /* (sotto) Manca parte dell'implementazione! */
    assert( (member->categ == CAT_LREG) || (member->categ == CAT_GREG) );

    if ( (*n = tym_recent_typedesc->st_size) == 0 ) return Success;
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
    assert( n > 0 );
    --(*n);
    /* (n == 0) if and only if (member_type == TYPE_NONE) */
    if ( (*n == 0) != (member_type == TYPE_NONE) ) {
      MSG_ERROR("Cmp_Structure_Get: errore interno!");
      return Failed;
    }
    if ( n == 0 ) return Success;
    member->type = member_type;
    member->value.i += tym_recent_typedesc->size;
    member->resolved = Tym_Type_Resolve_All(member_type);
  }
  return Success;
}


