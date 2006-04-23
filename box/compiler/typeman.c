/* typeman.c - Autore: Franchin Matteo - dicembre 2004
 *
 * This file contains the functions for managing the user defined types.
 * (typeman stands for TYPE-MANager!)
 */

/*#define DEBUG*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
#include "str.h"
#include "virtmach.h"
#include "registers.h"
#include "compiler.h"

/* #define DEBUG_COMPARE_TYPES */

/******************************************************************************
 * Le procedure che seguono servono a gestire i tipi definiti dall'utente.    *
 ******************************************************************************/
TypeDesc *tym_recent_typedesc;
Intg tym_recent_type;
char *tym_special_name[PROC_SPECIAL_NUM] = {
  "",
  "{open}", "{close}", "{pause}",
  "{reopen}", "{reclose}", "{repause}",
  "{copy}", "{destroy}"
};

static Array *tym_type_list = NULL;
static int tym_num_name = -1;
static char *tym_name[TYM_NUM_TYPE_NAMES];

/* DESCRIPTION: This function returns the integer index that will
 *  be associated to the new type, that will be created by the next call
 *  to the function Tym_Type_New.
 */
Intg Tym_Type_Next(void) {
  if ( tym_type_list == NULL ) return 0;
  return Arr_NumItem(tym_type_list);
}

/* DESCRIZIONE: Inserisce un nuovo tipo nella lista dei tipi e restituisce
 *  il puntatore dell'elemento inserito all'interno della lista (o NULL
 *  in caso di errore).
 * NOTE: It updates tym_recent_type and tym_recent_typedesc.
 */
TypeDesc *Tym_Type_New(Name *nm) {
  TypeDesc td;

  /* Se l'array dei tipi non e' stato inizializzato, lo faccio ora */
  if ( tym_type_list == NULL ) {
    tym_type_list = Array_New(sizeof(TypeDesc), CMP_TYPICAL_NUM_TYPES);
    if ( tym_type_list == NULL ) return NULL;
  }

  td.name = Str_Dup(nm->text, nm->length);
  td.parent = TYPE_NONE;
  td.greater = TYPE_NONE;
  td.procedure = TYPE_NONE;
  td.sym = NULL;

  /* Inserisce il nuovo tipo nella lista */
  if IS_FAILED(Arr_Push(tym_type_list, & td)) return NULL;

  tym_recent_type = Arr_NumItem(tym_type_list) - 1;
  return (tym_recent_typedesc = Arr_LastItemPtr(tym_type_list, TypeDesc));
}

/* DESCRIPTION: Returns the last type created by the function Cmp_Type_New
 *  (or -1 if none was created!).
 */
Intg Tym_Type_Newer(void) {
  MSG_LOCATION("Tym_Type_Newer");

  if ( tym_type_list != NULL ) return (Arr_NumItem(tym_type_list) - 1);
  MSG_ERROR("Lista dei tipi non inizializzata!");
  return TYPE_NONE;
}

/* DESCRIPTION: Returns the descriptor of the type t.
 * NOTE: It updates tym_recent_type and tym_recent_typedesc.
 */
TypeDesc *Tym_Type_Get(Intg t) {
  MSG_LOCATION("Tym_Type_Get");

  if ( (t < 0) || (t >= Arr_NumItem(tym_type_list)) ) {
    MSG_ERROR("Tipo sconosciuto!");
    return NULL;
  }

  tym_recent_type = t;
  return (tym_recent_typedesc = Arr_ItemPtr(tym_type_list, TypeDesc, t+1));
}

/* DESCRIPTION: Returns the size of an object of type t
 *  (or -1 in case of errors).
 */
Intg Tym_Type_Size(Intg t) {
  TypeDesc *td;
  MSG_LOCATION("Tym_Type_Size");

  if ( (t < 0) || (t >= Arr_NumItem(tym_type_list)) ) return -1;
  td = Arr_ItemPtr(tym_type_list, TypeDesc, t+1);
  return td->size;
}

/* DESCRIPTION: Restituisce il nome del tipo t.
 */
Task Tym__Type_Name(Intg t, Array *name) {
  TypeDesc *td;
  MSG_LOCATION("Tym__Type_Name");

  if (t < 0) {
    register Intg tt = -t;
    if ( tt < PROC_SPECIAL_NUM ) {
      char *nm = tym_special_name[tt];
      return Arr_MPush(name, nm, strlen(nm));
    }

    MSG_ERROR("Tipo sconosciuto!");
    return Arr_MPush(name, "Unknown", 7);
  }

  if (t >= Arr_NumItem(tym_type_list)) {
    MSG_ERROR("Tipo sconosciuto!");
    return Arr_MPush(name, "Unknown", 7);
  }

  td = Arr_ItemPtr(tym_type_list, TypeDesc, t+1);

  switch (td->tot) {
   case TOT_INSTANCE:
    return Arr_MPush(name, td->name, strlen(td->name));

   case TOT_PTR_TO:
    if IS_FAILED( Arr_MPush(name, "puntatore a ", 12) ) return Failed;
    return Tym__Type_Name(td->target, name);

   case TOT_ARRAY_OF: {
      char str[128];
      register Intg s = td->arr_size;
      if ( s > 0 ) sprintf(str, "array("SIntg") di ", s);
              else sprintf(str, "array() di ");
      if IS_FAILED( Arr_MPush(name, str, strlen(str)) ) return Failed;
    }
    return Tym__Type_Name(td->target, name);

   case TOT_PROCEDURE:
    TASK( Arr_MPush(name, "procedura ", 10) );
    TASK( Tym__Type_Name(td->target, name)  );
    TASK( Arr_MPush(name, " di ", 4)        );
    TASK( Tym__Type_Name(td->parent, name)  );
    return Success;

   case TOT_SPECIE: {
      register Intg t = td->target;
      register int not_first = 0;
      TASK( Arr_MPush(name, "specie(", 7) );
      do {
        td = Tym_Type_Get(t);
        if ( td == NULL ) {
          MSG_ERROR("Tym__Type_Name: Array dei tipi danneggiata(1)!");
          return Failed;
        }
        if ( td->tot != TOT_ALIAS_OF ) {
          MSG_ERROR("Tym__Type_Name: Array dei tipi danneggiata(2)!");
          return Failed;
        }
        if ( not_first ) {TASK( Arr_MPush(name, "->", 2) );}
        TASK( Tym__Type_Name(td->target, name) );
        t = td->greater;
        not_first = 1;
      } while( td->greater != TYPE_NONE );
      TASK( Arr_Push(name, ")") );
      return Success;
    }

   case TOT_ALIAS_OF:
    if ( td->name == NULL )
      return Tym__Type_Name(td->target, name);
    else
      return Arr_MPush(name, td->name, strlen(td->name));

   case TOT_STRUCTURE: {
      register Intg t = td->target;
      register int not_first = 0;
      TASK( Arr_MPush(name, "struttura(", 10) );
      do {
        td = Tym_Type_Get(t);
        if ( td == NULL ) {
          MSG_ERROR("Tym__Type_Name: Array dei tipi danneggiata(3)!");
          return Failed;
        }
        if ( td->tot != TOT_ALIAS_OF ) {
          MSG_ERROR("Tym__Type_Name: Array dei tipi danneggiata(4)!");
          return Failed;
        }
        if ( not_first ) {TASK( Arr_MPush(name, ", ", 2) );}
        TASK( Tym__Type_Name(td->target, name) );
        t = td->greater;
        not_first = 1;
      } while( td->greater != TYPE_NONE );
      TASK( Arr_Push(name, ")") );
      return Success;
    }

   default:
    MSG_ERROR("Array dei tipi danneggiata!");
    return Arr_MPush(name, "Unknown", 7);
  }
}

/* DESCRIPTION: Returns the name of the type t.
 * NOTE: This functions builds the name into an array-object (type: Array)
 *  and then returns the pointer to that array.
 *  There is no need to free the string returned: the same array is used
 *  for all the calls to Tym_Type_Name.
 *  As a consequence to that behaviour YOU CANNOT USE Tym_Type_Name MORE
 *  THAN ONCE INSIDE ONE SINGLE printf STATEMENT.
 *  For this purpose use Tym_Type_Names.
 */
const char *Tym_Type_Name(Intg t) {
  static Array *name = NULL;
  const char ending_char = '\0';

  /* Set up or clear the array of chars where the name of type
   * will be composed.
   */
  if ( name == NULL ) {
    name = Array_New(sizeof(char), TYM_TYPICAL_TYPE_NAME_SIZE);
    if ( name == NULL ) return NULL;

  } else {
     if IS_FAILED( Arr_Clear(name) ) return NULL;
  }

  /* The name will be composed by the recursive function Tym__Type_Name */
  if IS_FAILED( Tym__Type_Name(t, name) ) return NULL;
  if IS_FAILED( Arr_Push(name, (void *) & ending_char) ) return NULL;

  return Arr_FirstItemPtr(name, char);
}

/* DESCRIPTION: Returns the name of the type t.
 *  This function is similar to Tym_Type_Names, but can used more than
 *  once inside a printf statement. For example:
 *   printf("Type 1: '%s' -- Type 2: '%s' -- Type 3: '%s'\n",
 *    Tym_Type_Names(type1), Tym_Type_Names(type2), Tym_Type_Names(type3));
 */
char *Tym_Type_Names(Intg t) {
  register char *str;
  if ( tym_num_name < 0 ) {
    register int i;
    for(i = 0; i < TYM_NUM_TYPE_NAMES; i++) tym_name[i] = NULL;
    tym_num_name = 0;
  }
  free(tym_name[tym_num_name]);
  str = strdup(Tym_Type_Name(t));
  tym_name[tym_num_name] = str;
  tym_num_name = (tym_num_name + 1) % TYM_NUM_TYPE_NAMES;
  return str;
}

Intg Cmp_Align(Intg addr) {
  return addr;
}

/* This function defines a new type and a corresponding new symbol.
 * It behaves in two distinct ways: if 'parent != TYPE_NONE', it defines
 * a type 'new_type' which will be member of 'parent',
 * if 'parent == TYPE_NONE', the new type will be an independent
 * (explicit) type. There is another important thing to say:
 * if 'size < 0' the new type will be simply an alias of the type given
 * inside 'aliased_type'

 The new type is returned inside *new_type.

 * It creates a new type associated to the new box and returns it.
 * NOTE: In case of error returns TYPE_NONE.
 *  If size < 0 an alias of aliased_type will be created.
 * Tym_Def_Type_With_Name
 */
Task Tym_Def_Type(Intg *new_type,
 Intg parent, Name *nm, Intg size, Intg aliased_type) {
  Symbol *s;
  TypeDesc *td;
  Intg type;
  MSG_LOCATION("Tym_Def_Type");

  /* First of all I create the symbol with name *nm */
  if ( parent == TYPE_NONE ) {
    s = Sym_Explicit_New(nm, 0);
    if ( s == NULL ) return Failed;

  } else {
    TASK( Sym_Implicit_New(& s, parent, nm) );
  }

  /* Now I create a new type for the box */
  if ( size < 0 ) {
    type = Tym_Def_Alias_Of(nm, aliased_type);
    if ( type == TYPE_NONE ) return Failed;
    if ( (td = Tym_Type_Get(type)) == NULL ) return Failed;

  } else {
    td = Tym_Type_New(nm);
    type = Tym_Type_Newer();
    if ( (td == NULL) || (type < 0) ) return Failed;
    td->tot = TOT_INSTANCE;
    td->size = size;
    td->target = TYPE_NONE;
  }
  td->sym = s;

  /* I set all the remaining values of the structure s */
  s->value.type = type;
  s->value.resolved = Tym_Type_Resolve_All(type);
  s->symattr.is_explicit = 1;
  s->symtype = VARIABLE;
  s->value.is.value = 0;
  s->value.is.typed = 1;
  *new_type = type;
  return Success;
}

/* DESCRIPTION: Returns the symbol associated with the type 'type';
 */
Symbol *Tym_Symbol_Of_Type(Intg type) {
  TypeDesc *td;
  td = Tym_Type_Get(type);
  if ( td == NULL ) return NULL;
  return td->sym;
}

/* DESCRIPTION: This function creates a new member for the abstract box
 *  associated with the type parent.
 */
Task Tym_Def_Member(Intg parent, Name *nm, Intg type) {
  Symbol *s, *ps;
  TypeDesc *td, *ptd;

  /* I create a new implicit symbol with name nm and I get the type and symbol
   * descriptors for the new symbol and its parent.
   */
  TASK( Sym_Implicit_New(& s, parent, nm) );
  td = Tym_Type_Get(type);
  ptd = Tym_Type_Get(parent);
  if ( (s == NULL) || (td == NULL) || (ptd == NULL) ) return Failed;
  ps = ptd->sym;
  if ( ps == NULL ) return Failed;

  s->symtype = VARIABLE;
  s->value.type = type;
  s->value.resolved = Tym_Type_Resolve_All(type);
  s->value.is.value = 0;
  s->value.addr = ptd->size;

  ptd->size = Cmp_Align(ptd->size + td->size);
  return Success;
}

/* DESCRIPTION: Deletes the type 'type' and all its corresponding box.
 */
Task Tym_Undef_Type(Intg type) {
  Symbol *s, *ps;
  TypeDesc *td, *ptd;

  MSG_ERROR("Non ancora implementato!");
  return Failed;

  ptd = Tym_Type_Get(type);
  if (ptd == NULL) return Failed;
  ps = ptd->sym;
  if ( ps == NULL ) return Failed;

  /* Loop over the symbols of the box */
  for ( s = ps->child; s != NULL; s = s->brother ) {
    td = Tym_Type_Get(s->value.type);
    if ( td == NULL ) return Failed;
  }
  return Success;
}

/* DESCRIPTION: This function is written for debugging: it prints the structure
 *  of the abstract box 'type'.
 */
void Tym_Print_Structure(FILE *stream, Intg type) {
  Symbol *s, *ps;
  TypeDesc *td, *ptd;

  ptd = Tym_Type_Get(type);
  if (ptd == NULL) return;
  ps = ptd->sym;
  if ( ps == NULL ) return;

  fprintf(stream, "Printing the structure of the box '%s':\n", ptd->name);

  /* Loop over the symbols of the box */
  for ( s = ps->child; s != NULL; s = s->brother ) {
    char *name;
    td = Tym_Type_Get(s->value.type);
    name = Str_Dup(s->name, s->leng);
    if ( (name != NULL) && (td != NULL) ) {
      fprintf(stream, "Address "SIntg
       ":\titem with name '%s' of type '%s' and dimension "SIntg"\n",
       s->value.addr, name, Tym_Type_Name(s->value.type), td->size);
      free(name);
    } else {
      fprintf(stream, "Problem in Tym_Print_Structure!\n");
    }
  }
}

/* DESCRIZIONE: Crea una nuova specie di tipi. Una specie di tipi e'
 *  un raggruppamento di tipi di dato (che a loro volta possono essere
 *  specie di tipi). name e leng sono nome e lunghezza del nome della specie
 *  che si vuole creare. La lista types e' una lista di liste di tipi.
 *  Per convenzione uso TYPE_NONE per terminare le liste, ad esempio:
 *  int types[] = {
 *   TYPE_CHAR, TYPE_INTG, TYPE_NONE,
 *   TYPE_REAL, TYPE_DREAL, TYPE_NONE,
 *   TYPE_NONE
 *  };
 * E' una lista composta da 2 liste di tipi:
 *  1 --> (TYPE_CHAR, TYPE_INTG)
 *  2 --> (TYPE_REAL, TYPE_DREAL)
 * La lista che compare per prima ha una "priorita'" maggiore.
 * I tipi appartenenti alla stessa lista hanno stessa priorita'.
 * Tale priorita' viene usata nella conversione implicita dei tipi,
 * cioe' quando si vuole compilare un'operazione come 1 * 3.4 =
 * intero * reale (a questo servono le specie di tipi).
 *
Task Tym_Specie_New(Name *nm, Intg *types)
{
        Intg t, first, greater;
        int error = 0, new_num;
        TypeDesc *new, *td;

        new = Tym_Type_New(nm);
        if ( new == NULL ) return Failed;
        new_num = Arr_NumItem(tym_type_list) - 1;
        new->tot = TOT_SPECIE2;
        new->parent = TYPE_NONE;
        new->greater = TYPE_NONE;

        greater = TYPE_NONE;
        for ( t = *types; t != TYPE_NONE; t = *(++types) ) {
                first = t;
                for ( ; t != TYPE_NONE; t = *(++types) ) {
                        td = Tym_Type_Get(t);
                        if ( td == NULL) return Failed;

                        if ( (td->parent == TYPE_NONE) && (td->greater == TYPE_NONE) ) {
                                td->parent = new_num;
                                td->greater = greater;

                        } else {
                                MSG_ERROR( "Il tipo '%s' appartiene gia' alla specie '%s'",
                                 td->name, Tym_Type_Name(td->parent) );
                                error = 1;
                        }
                }
                greater = first;
        }
        new->target = first;

        if ( ! error ) return Success;
        return Failed;
}*/

/* DESCRIZIONE: Ottiene la specie di ordine order del tipo type
 *  (restituisce TYPE_NONE se tale specie non esiste).
 * NOTA: Si ha la seguente proprieta':
 *  Tym_Specie_Of(Tym_Specie_Of(t, a), b) = Tym_Specie_Of(t, a + b)
 *
Intg Tym_Specie_Of(Intg type, int order)
{
        int i;
        Intg t = type;
        TypeDesc *td;

        for (i = 0; i < order; i++) {
                td = Tym_Type_Get(t);
                if ( td == NULL ) return TYPE_NONE;
                if ( (t = td->parent) == TYPE_NONE ) return TYPE_NONE;
        }
        return t;
}*/

/* DESCRIZIONE: Ottiene l'estensione di ordine order del tipo type
 *  (restituisce TYPE_NONE se tale estensione non esiste).
 * NOTA: Si ha la seguente proprieta':
 *  Tym_Extension_Of(Tym_Extension_Of(t, a), b) = Tym_Extension_Of(t, a + b)
 *
Intg Tym_Extension_Of(Intg type, int order)
{
        int i;
        Intg t = type;
        TypeDesc *td;

        for (i = 0; i < order; i++) {
again:
                td = Tym_Type_Get(t);
                if ( td == NULL ) return TYPE_NONE;
                if ( td->tot == TOT_SPECIE2 ) {
                         * "espando" la specie dal piu' piccolo
                         * al piu' grande dei suoi elementi.
                         *
                        t = td->target;
                        goto again;

                } else {
                        t = td->greater;
                        if ( t != TYPE_NONE ) continue;
                        td = Tym_Type_Get(td->parent);
                        if ( td == NULL ) return TYPE_NONE;
                        t = td->greater;
                        goto again;
                }
        }
        return t;
}*/

/******************************************************************************
 * Le procedure che seguono servono a creare tipi composti di dati            *
 * (array di ..., puntatori a..., etc).                                       *
 ******************************************************************************/

/* DESCRIPTION: This function creates a new type of data using the type "type".
 *  The new data type is "array of num object of kind type".
 *  If num < 1 the function assumes that the size of the array is unspecified.
 * NOTE: The new type will be returned or TYPE_NONE in case of errors.
 * NOTE 2: It updates tym_recent_type and tym_recent_typedesc.
 */
Intg Tym_Def_Array_Of(Intg num, Intg type) {
  Intg size;
  TypeDesc *td;
  MSG_LOCATION("Tym_Def_Array_Of");

  if ( (td = Tym_Type_Get(type)) == NULL ) return TYPE_NONE;
  /* Faccio i controlli sul tipo type */
  switch (td->tot) {
   case TOT_INSTANCE: case TOT_PTR_TO: case TOT_ARRAY_OF: break;
   default:
    MSG_ERROR("Impossibile creare una array di '%s'.",
     Tym_Type_Name(type));
    return TYPE_NONE;
  }
  size = td->size;
  if ( size < 1 ) {
    MSG_ERROR("Impossibile creare una array di dimensione indefinita.");
    return TYPE_NONE;
  }

  /* Creo il nuovo tipo */
  if ( (td = Tym_Type_New( Name_Empty() )) == NULL ) return TYPE_NONE;
  td->arr_size = num;
  td->size = ( (size > 0) && (num > 0) ) ? size * num : 0;
  td->tot = TOT_ARRAY_OF;
  td->target = type;
  return tym_recent_type;
}

/* DESCRIPTION: This function creates a new type of data using the type "type".
 *  The new data type is "pointer to object of kind type".
 * NOTE: The new type will be returned or TYPE_NONE in case of errors.
 * NOTE 2: It updates tym_recent_type and tym_recent_typedesc.
 */
Intg Tym_Def_Pointer_To(Intg type) {
  TypeDesc *td;
  MSG_LOCATION("Tym_Def_Pointer_To");

  if ( (td = Tym_Type_Get(type)) == NULL ) return TYPE_NONE;
  /* Faccio i controlli sul tipo type */
  switch (td->tot) {
   case TOT_INSTANCE: case TOT_PTR_TO: case TOT_ARRAY_OF: break;
   default:
    MSG_ERROR("Impossibile creare un puntatore a '%s'.",
     Tym_Type_Name(type));
    return TYPE_NONE;
  }
  if ( td->size < 1 ) {
    MSG_ERROR("Tentativo di creare un puntatore ad un tipo di size 0 ('%s').",
     Tym_Type_Name(type));
    return TYPE_NONE;
  }

  /* Creo il nuovo tipo */
  if ( (td = Tym_Type_New( Name_Empty() )) == NULL ) return TYPE_NONE;
  td->size = SIZEOF_PTR;
  td->tot = TOT_PTR_TO;
  td->target = type;
  return tym_recent_type;
}

/* DESCRIPTION: This function creates a new type of data using the type "type".
 *  The new data type is "alias of type".
 * NOTE: The new type will be returned or TYPE_NONE in case of errors.
 * NOTE 2: It updates tym_recent_type and tym_recent_typedesc.
 */
Intg Tym_Def_Alias_Of(Name *nm, Intg type) {
  TypeDesc *td;
  Intg size;
  MSG_LOCATION("Tym_Def_Alias_Of");

  if ( nm == NULL ) nm = Name_Empty();

  /* DOVREI RISOLVERE GLI ALIAS??? */
  if ( (td = Tym_Type_Get(type)) == NULL ) return TYPE_NONE;
  /* Faccio i controlli sul tipo type */
  switch (td->tot) {
   case TOT_INSTANCE: case TOT_PTR_TO: case TOT_ARRAY_OF:
   case TOT_SPECIE: case TOT_ALIAS_OF: case TOT_STRUCTURE:
     break;
   default:
    MSG_ERROR("Impossibile creare uno pseudonimo di '%s'.",
     Tym_Type_Name(type));
    return TYPE_NONE;
  }
  size = td->size;

  /* Creo il nuovo tipo */
  if ( (td = Tym_Type_New( nm )) == NULL ) return TYPE_NONE;
  td->size = size;
  td->tot = TOT_ALIAS_OF;
  td->target = type;
  return tym_recent_type;
}

/* DESCRIPTION: This function returns 1 if the type type1 is equal to
 *  (or 'contains') the type type2 and returns 0 otherwise.
 *  In OTHER WORDS this function give an answer to the question:
 *  "an object of type 'type2' belong to the cathegory of objects
 *  of type 'type1'?".
 * EXAMPLES: array[] of Int contains array[5] of Int
 *            specie (Real > Int > Char) contains Int
 * NOTE: If type2 needs to be 'expanded' into type1 and need_expansion != NULL,
 *  then *need_expansion will be setted to 1. In other cases *need_expansion
 *  will be leaved untouched.
 */
int Tym_Compare_Types(Intg type1, Intg type2, int *need_expansion) {
  int first, dummy;
  TypeDesc *td1, *td2;

#ifdef DEBUG_COMPARE_TYPES
  printf( "Type_Compare_Types(%s, %s)\n",
   Tym_Type_Names(type1), Tym_Type_Names(type2) );
#endif

  need_expansion = (need_expansion == NULL) ? & dummy : need_expansion;

  first = 1;
  for(;;) {
    if ( type1 == type2 ) return 1;
    type1 = Tym_Type_Resolve_Alias(type1);
    type2 = Tym_Type_Resolve_All(type2);
    if ( type1 == type2 ) return 1;
    td1 = Tym_Type_Get(type1);
    td2 = Tym_Type_Get(type2);
    if ( (td1 == NULL) || (td2 == NULL) ) {
      MSG_ERROR("Tipi errati in Tym_Compare_Types!");
      return 0;
    }

    switch( td1->tot ) {
     case TOT_INSTANCE: /* type1 != type2 !!! */
      return 0;

     case TOT_PTR_TO:
      if (td2->tot == TOT_PTR_TO) break;
      return 0;

     case TOT_ARRAY_OF:
      if (td2->tot != TOT_ARRAY_OF) return 0;
      {
        register Intg td1s = td1->arr_size, td2s = td2->arr_size;
        if (td1s == td2s) break;
        if ((td1s < 1) && first) break;
        return 0;
      }

     case TOT_SPECIE: {
        register Intg t = td1->target;
        do {
          td1 = Tym_Type_Get(t);
          if ( td1 == NULL ) {
            MSG_ERROR("Array dei tipi danneggiata!");
            return 0;
          }
          if ( Tym_Compare_Types(t, type2, need_expansion) ) {
            *need_expansion |= (td1->greater != TYPE_NONE);
            return 1;
          }
          t = td1->greater;
        } while( td1->greater != TYPE_NONE );
      }
      return 0;

     case TOT_PROCEDURE:
      if (td2->tot != TOT_PROCEDURE) return 0;
      return (
           Tym_Compare_Types(td1->parent, td2->parent, need_expansion)
        && Tym_Compare_Types(td1->target, td2->target, need_expansion) );

     case TOT_STRUCTURE: {
        register Intg t1 = td1->target, t2 = td2->target;
        if (td2->tot != TOT_STRUCTURE) return 0;
        do {
#ifdef DEBUG_COMPARE_TYPES
          printf("Structure are equals? testing '%s' <--> '%s': ",
           Tym_Type_Names(t1), Tym_Type_Names(t2));
#endif
          td1 = Tym_Type_Get(t1);
          td2 = Tym_Type_Get(t2);
          if ( (td1 == NULL) || (td2 == NULL) ) {
            MSG_ERROR("Array dei tipi danneggiata(2)!");
            return 0;
          }
          if ( ! Tym_Compare_Types(t1, t2, need_expansion) ) {return 0;}
#ifdef DEBUG_COMPARE_TYPES
          if (! *need_expansion) printf("no ");
          printf("expansion needed!\n");
#endif
          t1 = td1->greater;
          t2 = td2->greater;
        } while( (t1 != TYPE_NONE) && (t2 != TYPE_NONE) );
        if ( t1 == t2 ) return 1; /* t1 == t2 == TYPE_NONE */
        return 0;
      }

     default:
      MSG_ERROR("Tym_Compare_Types non implementata fino in fondo!");
      return 0;
    }

    first = 0;
    type1 = td1->target;
    type2 = td2->target;
  }

  return 0;
}

/* DESCRIPTION: This function resolves the alias of types.
 *  It returns the final target of the alias.
 * NOTE: If there is something wrong, this functions simply ignore it
 *  and returns type.
 */
Intg Tym_Type_Resolve(Intg type, int not_alias, int not_species) {
  TypeDesc *td;
  MSG_LOCATION("Tym_Type_Resolve");

  while( type >= 0 ) {
    td = Tym_Type_Get(type);
    if ( td == NULL ) return type;
    switch( td->tot ) {
     case TOT_ALIAS_OF:
      if ( not_alias ) return type;
      type = td->target;
      continue;

     case TOT_SPECIE:
      if ( not_species ) return type;
      type = td->greater;
      continue;
     default:
      return type;
    }
  }
  return type;
}

/* DESCRIPTION: This function deletes the type type (and its chain
 *  of type-descriptors).
 */
Task Tym_Delete_Type(Intg type) {
  MSG_LOCATION("Tym_Delete_Type");
  MSG_ERROR("'Tym_Delete_Type' <-- non ancora implementato!");
  return Failed;
}

/* DESCRIPTION: This function define the procedure proc@of_type and associates
 *  the module asm_module to the procedure.
 */
Intg Tym_Def_Procedure(Intg proc, Intg of_type, Intg asm_module) {
  TypeDesc *td, *p;
  Intg new_procedure;
  MSG_LOCATION("Tym_Def_Procedure");

  if ( (p = Tym_Type_Get(of_type)) == NULL ) return TYPE_NONE;
  switch (p->tot) {
   case TOT_INSTANCE: case TOT_PTR_TO: case TOT_ARRAY_OF: case TOT_ALIAS_OF:
     break;
   default:
    MSG_ERROR("Impossibile creare una procedura di tipo '%s'.",
     Tym_Type_Name(of_type));
    return TYPE_NONE;
  }

  /* Creo il nuovo tipo */
  if ( (td = Tym_Type_New( Name_Empty() )) == NULL ) return TYPE_NONE;
  td->tot = TOT_PROCEDURE;
  td->size = 0;
  td->parent = of_type;
  td->target = proc;
  td->asm_mod = asm_module;
  td->procedure = p->procedure;
  new_procedure = tym_recent_type;
  /* The previous call to Tym_Type_New may had required to realloc
   * the array of types, so we must re-get the address of p.
   */
  if ( (p = Tym_Type_Get(of_type)) == NULL ) return TYPE_NONE;
  p->procedure = new_procedure;
  return new_procedure;
}

/* DESCRIPTION: This function searches for the existence of the procedure
 *  proc@of_type.
 * NOTE: This function doesn't produce any error messages, when a procedure
 *  has not been found (it simply returns TYPE_NONE!).
 * NOTE: Box can handle procedures like the following: (Scalar, Scalar)@Print
 *  When the compiler finds something like: Print[ (1, 1.2) ] it should:
 *   1) convert 1 --> 1.0 (convert Int --> Real)
 *   2) build the structure (1.0, 1.2)
 *   3) call (Scalar, Scalar)@Print and pass it the new created structure.
 *  These operations are not always necessary. This is the case
 *  of the following example: Print[ (1.0, 1.2) ]
 *  In this case the compiler can pass the structure to the procedure
 *  without conversions.
 *  Tym_Search_Procedure can recognize if conversions should be performed:
 *  it sets *containing_species = species, if this happens. species is
 *  the species which 'contains' the type proc ( '(Scalar, Scalar)'
 *  in the previous example). In other cases *containing_species = TYPE_NONE.
 */
Intg Tym_Search_Procedure(Intg proc, Intg of_type, Intg *containing_species) {
  TypeDesc *td, *td0;
  Intg procedure, dummy;
  int need_expansion = 0;
  MSG_LOCATION("Tym_Search_Procedure");

  if ( (td0 = td = Tym_Type_Get(of_type)) == NULL ) return TYPE_NONE;
  if ( containing_species == NULL ) containing_species = & dummy;

  if ( proc < 0 ) {
    *containing_species = TYPE_NONE;
    while ( (procedure = td->procedure) != TYPE_NONE ) {
      td = Tym_Type_Get(procedure);

      if (td == NULL) {
        MSG_ERROR("Array dei tipi danneggiata!");
        return TYPE_NONE;

      } else if ( (td->tot != TOT_PROCEDURE) || (td->parent != of_type) ) {
        int status = (td->tot != TOT_PROCEDURE) | (td->parent != of_type) << 1;
        MSG_ERROR("Array dei tipi danneggiata: stato "SIntg, status);
        return TYPE_NONE;
      }

      if ( td->target == proc ) return procedure;
    }

  } else {
    while ( (procedure = td->procedure) != TYPE_NONE ) {
      td = Tym_Type_Get(procedure);

      if (td == NULL) {
        MSG_ERROR("Array dei tipi danneggiata!");
        return TYPE_NONE;

      } else if ( (td->tot != TOT_PROCEDURE) || (td->parent != of_type) ) {
        int status = (td->tot != TOT_PROCEDURE) | (td->parent != of_type) << 1;
        MSG_ERROR("Array dei tipi danneggiata: stato "SIntg, status);
        return TYPE_NONE;
      }

      if ( td->target >= 0 ) {
        if (Tym_Compare_Types(td->target, proc, & need_expansion) ) {
          *containing_species = (need_expansion) ? td->target : TYPE_NONE;
          return procedure;
        }
      }
    }
  }

/*  if ( td0->tot == TOT_ALIAS_OF )
    return Tym_Search_Procedure(proc, td0->target);*/

  return TYPE_NONE;
}

/* DESCRIPTION: This function prints all the procedures belonging
 *  to the type of_type.
 */
void Tym_Print_Procedure(FILE *stream, Intg of_type) {
  TypeDesc *td;
  Intg procedure;

  if ( (td = Tym_Type_Get(of_type)) == NULL ) return;

  fprintf(stream, "Printing all the procedures of '%s':\n",
   Tym_Type_Name(of_type));

  while ( (procedure = td->procedure) != TYPE_NONE ) {
    printf(" procedure '%s'\n", Tym_Type_Name(procedure));
    td = Tym_Type_Get(procedure);

    if (td == NULL) {
      MSG_ERROR("Array of types is corrupted!"); return;

    } else if ( (td->tot != TOT_PROCEDURE) || (td->parent != of_type) ) {
      int status = (td->tot != TOT_PROCEDURE) | (td->parent != of_type) << 1;
      MSG_ERROR("Array of types is corrupted! status = "SIntg, status); return;
    }
  }
}

/* DESCRIPTION: This function defines a new specie of types.
 *  You can define a new specie as in the following example:
 *   EXAMPLE: (Real < Intg < Char)
 *            new_specie = TYPE_NONE;
 *            Tym_Def_Specie(*new_specie, TYPE_REAL);
 *            Tym_Def_Specie(*new_specie, TYPE_INTG);
 *            Tym_Def_Specie(*new_specie, TYPE_CHAR);
 * NOTE: Species are used to do automatic conversions. For instance:
 *  consider the function cosine: Cos[x]. This is a real valued function
 *  of the real variable x. What happens if you write Cos[1]?
 *  The answer is rather simple: 1 is an integer, so the compiler will search
 *  the procedure Intg@Cos. This is not what one would expect.
 *  A better behaviour is: conversion of 1 into 1.0 (real value)
 *  and execution of the procedure Real@Cos.
 *  To accomplish what has just been described, one should define a species
 *  of types in the following way: Scalar = (Real < Intg < Char).
 *  then one should define Scalar@Cos. Now when the compiler finds Cos[x]
 *  where x is a Real, an Intg or a Char, it converts x into a Real
 *  and then executes Real@Cos.
 *  A Scalar is essentially a Real with only one difference: you can assign
 *  to a Scalar a Real value, an Intg value or a Char value. The compiler
 *  will chose the right conversion for you!
 */
Task Tym_Def_Specie(Intg *specie, Intg type) {
  if ( *specie == TYPE_NONE ) {
    register TypeDesc *td;
    Intg great_type, great_size, specie_type;

    /* Creo un alias di type */
    great_type = Tym_Def_Alias_Of(Name_Empty(), type);
    if ( great_type == TYPE_NONE ) return Failed;
    td = tym_recent_typedesc;
    specie_type = Tym_Type_Next();
    great_size = td->size;
    td->parent = specie_type;
    td->greater = TYPE_NONE;

    /* Creo la specie */
    td = Tym_Type_New( Name_Empty() );
    if ( td == NULL ) return Failed;
    td->tot = TOT_SPECIE;
    td->size = great_size;
    td->sp_size = 1;  /* La specie per ora contiene un solo elemento! */
    td->target = great_type;  /* lower type   */
    td->greater = great_type; /* greater type */

    *specie = specie_type;
    return Success;

  } else {
    register TypeDesc *td;
    Intg lower_type, greater_type;

    /* Trovo il descrittore della specie */
    td = Tym_Type_Get(*specie);
    if ( td == NULL ) return Failed;
    assert( td->tot == TOT_SPECIE );
    ++(td->sp_size);
    greater_type = td->target;
    td->target = Tym_Type_Next();

    /* Creo il prossimo elemento della specie */
    lower_type = Tym_Def_Alias_Of(Name_Empty(), type);
    if ( lower_type == TYPE_NONE ) return Failed;
    td = tym_recent_typedesc;
    td->parent = *specie;
    td->greater = greater_type;
    return Success;
  }
}

/* DESCRIPTION: This function defines a new structure of types.
 *  You can define a new structure in a way very similar to the
 *  one used to define species.
 */
Task Tym_Def_Structure(Intg *strc, Intg type) {
  if ( *strc == TYPE_NONE ) {
    register TypeDesc *td;
    Intg first_type, strc_size, strc_type;

    /* Creo un alias di type */
    first_type = Tym_Def_Alias_Of(Name_Empty(), type);
    if ( first_type == TYPE_NONE ) return Failed;
    td = tym_recent_typedesc;
    strc_type = Tym_Type_Next();
    strc_size = td->size;
    td->parent = strc_type;
    td->greater = TYPE_NONE;

    /* Creo la struttura */
    td = Tym_Type_New( Name_Empty() );
    if ( td == NULL ) return Failed;
    td->tot = TOT_STRUCTURE;
    td->size = strc_size;
    td->st_size = 1;  /* La struttura per ora contiene un solo elemento! */
    td->target = first_type;  /* first element of the structure */
    td->greater = first_type; /*  last element of the structure */

    *strc = strc_type;
    return Success;

  } else {
    register TypeDesc *strc_td, *new_td, *last_td;
    register Intg new;

    /* Trovo il descrittore della struttura */
    strc_td = Tym_Type_Get(*strc);
    if ( strc_td == NULL ) return Failed;
    assert( strc_td->tot == TOT_STRUCTURE );

    /* Creo il prossimo elemento della struttura */
    new = Tym_Def_Alias_Of(Name_Empty(), type);
    if ( new == TYPE_NONE ) return Failed;
    new_td = tym_recent_typedesc;
    new_td->parent = *strc;
    new_td->greater = TYPE_NONE;

    /* Concateno questo nuovo elemento agli altri */
    last_td = Tym_Type_Get(strc_td->greater);
    if ( last_td == NULL ) return Failed;
    last_td->greater = new;
    strc_td->greater = new;
    ++(strc_td->st_size);
    strc_td->size += new_td->size;
    return Success;
  }
}

/* DESCRIPTION: We explain this function with an example:
 *  Suppose you want to obtain the list of types which made up
 *  the structure s = ('c', 0.5, 23). First of all you call:
 *   Intg s_copy = s, *t = & s_copy;
 *   TASK( Tym_Structure_Get(t));
 *  This call will put the type of the first member of s into *t:
 *  so, if s is really a structure, the function will assign *t = TYPE_CHAR
 *  and will return 'Success'. If s is not a structure it will return 'Failed'
 *  Now, if you want the second member, you should call again:
 *   TASK( Tym_Structure_Get(t));
 *  In this case, after this call: *t = TYPE_REAL. When there are no more
 *  members,the function will put: *t = TYPE_NONE.
 * NOTE: you shouldn't change *type between one call and another.
 *  This code - for example - IS TO BE AVOIDED:
 *   Intg s_copy = s, *t = & s_copy;
 *   TASK( Tym_Structure_Get(t));
 *   *t = Tym_Type_Resolve_All(*t);  <-- E R R O R !
 *   TASK( Tym_Structure_Get(t));
 * NOTE 2: It updates: tym_recent_type = *type, tym_recent_typedesc = ...
 * NOTE 3: FULL EXAMPLE:
 *   Intg member_type = structure_type;
 *   TASK(Tym_Structure_Get(& member_type));
 *   while (member_type != TYPE_NONE) {
 *     ... // <-- here we can use member_type
 *     TASK(Tym_Structure_Get(& member_type));
 *   };
 */
Task Tym_Structure_Get(Intg *type) {
  TypeDesc *td;

  /* Trovo il descrittore della struttura */
  assert( *type != TYPE_NONE );
  td = Tym_Type_Get(*type);
  if ( td == NULL ) return Failed;

  if ( td->tot == TOT_STRUCTURE ) {
    *type = td->target;
    return Success;

  } else {
    assert( td->tot == TOT_ALIAS_OF );
    *type = td->greater;
    return Success;
  }
}


