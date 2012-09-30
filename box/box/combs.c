/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

#include <assert.h>

#include <box/types_priv.h>
#include <box/messages.h>
#include <box/obj.h>


/* Get the iterator over the combinations of the given identifier type. */
BoxBool BoxType_Get_Combinations(BoxType *t, BoxTypeIter *iter) {
  if (t->type_class == BOXTYPECLASS_IDENT) {
    BoxTypeIdent *td = BoxType_Get_Data(t);
    iter->current_node = td->combs.node.next;
    return BOXBOOL_TRUE;

  } else if (t->type_class == BOXTYPECLASS_SUBTYPE_NODE) {
    BoxTypeSubtypeNode *td = BoxType_Get_Data(t);
    iter->current_node = td->combs.node.next;
    return BOXBOOL_TRUE;
  }

  return BOXBOOL_FALSE;
}

/**
 * @brief Utility function to retrieve the combinations for a type.
 *
 * @param parent The parent type whose combinations are to be obtained.
 * @return A #BoxTypeNode object representing the linked list of combinations
 *   for the parent type @p parent.
 */
static BoxTypeNode *
My_Get_Combs(BoxType *parent) {
  if (parent->type_class == BOXTYPECLASS_IDENT) {
    BoxTypeIdent *td = BoxType_Get_Data(parent);
    return & td->combs.node;

  } else if (parent->type_class == BOXTYPECLASS_SUBTYPE_NODE) {
    BoxTypeSubtypeNode *td = BoxType_Get_Data(parent);
    return & td->combs.node;

  } else
    return NULL;
}

/* Define a combination 'child'@'parent' and associate a callable to it. */
BoxType *
BoxType_Define_Combination(BoxType *parent, BoxCombType comb_type,
                           BoxType *child, BOXIN BoxCallable *callable) {
  BoxType *comb;
  BoxTypeCombNode *cn;
  BoxTypeNode *combs_of_parent = My_Get_Combs(parent);

  if (!combs_of_parent) {
    BoxSPtr_Unlink(callable);
    MSG_FATAL("Parent is not an identifier type (%d).", parent->type_class);
    return NULL;
  }

  /* Create the node. */
  cn = BoxType_Alloc(& comb, BOXTYPECLASS_COMB_NODE);
  cn->comb_type = comb_type;
  cn->child = BoxType_Link(child);
  cn->callable = callable;
  BoxTypeNode_Prepend_Node(combs_of_parent, comb);
  return comb;
}

/* Undefine the given combination. */
void
BoxType_Undefine_Combination(BoxType *parent, BoxType *comb) {
  BoxTypeNode *combs_of_parent = My_Get_Combs(parent);

  if (!combs_of_parent) {
    MSG_FATAL("Object does not have combinations (type=%d).",
              (int) parent->type_class);
    return;
  }

  /* Create the node. */
  (void) BoxTypeNode_Remove_Node(combs_of_parent, comb);
  (void) BoxType_Unlink(comb);
}

/* Find the non-inherited procedure 'left'@'right'. */
BoxType *
BoxType_Find_Own_Combination(BoxType *parent, BoxCombType type,
                             BoxType *child, BoxTypeCmp *expand) {
  BoxTypeIter ti;
  
  if (parent && child && BoxType_Get_Combinations(parent, & ti)) {
    BoxType *t;
    for (; BoxTypeIter_Get_Next(& ti, & t);) {
      BoxTypeCombNode *node = BoxType_Get_Data(t);
      assert(t->type_class == BOXTYPECLASS_COMB_NODE);
      if (node->comb_type == type) {
        BoxTypeCmp cmp = BoxType_Compare(node->child, child);
        if (cmp != BOXTYPECMP_DIFFERENT) {
          if (expand)
            *expand = cmp;
          return t;
        }
      }
    }
  }

  return NULL;
}

/* Find a (possibly inherited procedure) child@parent of parent. */
BoxType *
BoxType_Find_Combination(BoxType *parent, BoxCombType comb_type,
                         BoxType *child, BoxTypeCmp *expand) {
  BoxType *found_comb, *former_parent;

  if (!(parent && child))
    return NULL;

  do {
    /* Find a combination for the parent type, if found return. */
    found_comb =
      BoxType_Find_Own_Combination(parent, comb_type, child, expand);

    if (found_comb)
      return found_comb;

    /* Remember the parent. */
    former_parent = parent;

    /* If not found, resolve the parent and try again. */
    parent = BoxType_Resolve(parent,
                             (BOXTYPERESOLVE_IDENT | BOXTYPERESOLVE_SPECIES
                              | BOXTYPERESOLVE_RAISED),
                             1);

  } while (parent != former_parent);

  return NULL;
}

BoxType *
BoxType_Find_Combination_With_Id(BoxType *parent, BoxCombType type,
                                 BoxTypeId child_id, BoxTypeCmp *expand) {
  /* Quick hack: we should do this better!
   * This code relies on BoxType_Find_Combination not trying to link or
   * unlink the child type.
   */
  BoxTypeBundle child;
  child.header.type_class = BOXTYPECLASS_PRIMARY;
  child.data.primary.id = child_id;
  return BoxType_Find_Combination(parent, type, (BoxType *) & child, expand);
}

/* Get details about a combination found with BoxType_Find_Combination. */
BoxBool
BoxType_Get_Combination_Info(BoxType *comb, BoxType **child,
                             BoxCallable **callable) {
  if (comb->type_class == BOXTYPECLASS_COMB_NODE) {
    BoxTypeCombNode *td = BoxType_Get_Data(comb);
    if (child)
      *child = td->child;
    if (callable)
      *callable = td->callable;
    return BOXBOOL_TRUE;
  }

  return BOXBOOL_FALSE;
}

/* Generate a call number for calling a combination from bytecode. */
BoxBool
BoxType_Generate_Combination_CallNum(BoxType *comb, BoxVM *vm,
                                     BoxVMCallNum *cn) {
  if (comb->type_class == BOXTYPECLASS_COMB_NODE) {
    BoxTypeCombNode *td = BoxType_Get_Data(comb);
    BoxCallable *new_cb;

    if (!BoxCallable_Request_VM_CallNum(td->callable, vm, cn, & new_cb)) {
      /* Substitute the callable, if necessary. */
      if (new_cb) {
        (void) BoxCallable_Unlink(td->callable);
        td->callable = new_cb;
      }

      return BOXBOOL_TRUE;
    }
  }

  return BOXBOOL_FALSE;
}
