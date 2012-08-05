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

#include <box/types.h>
#include <box/types_priv.h>
#include <box/messages.h>


/* Get the iterator over the combinations of the given identifier type. */
BoxBool BoxType_Get_Combinations(BoxType ident, BoxTypeIter *iter) {
  if (ident->type_class == BOXTYPECLASS_IDENT) {
    BoxTypeIdent *td = BoxType_Get_Data(ident);
    iter->current_node = td->combs.node.next;
    return BOXBOOL_TRUE;
  }

  return BOXBOOL_FALSE;
}

/* Define a combination 'child'@'parent' and associate an action to it. */
BoxBool
BoxType_Define_Combination(BoxType parent, BoxCombType type, BoxType child,
                           BoxCallable *callable) {
  if (parent->type_class == BOXTYPECLASS_IDENT) {
    BoxTypeIdent *pd = BoxType_Get_Data(parent);

    /* Create the node. */
    BoxType comb_node;
    BoxTypeCombNode *cn = BoxType_Alloc(& comb_node, BOXTYPECLASS_COMB_NODE);
    cn->comb_type = type;
    cn->child = BoxType_Link(child);
    cn->callable = callable;

    BoxTypeNode_Prepend_Node(& pd->combs.node, comb_node);
    return BOXBOOL_TRUE;

  } else {
    MSG_FATAL("Parent is not an identifier type.");
    return BOXBOOL_FALSE;
  }
}

/* Find the procedure 'left'@'right' */
BoxType
BoxType_Find_Combination(BoxType parent, BoxCombType type, BoxType child,
                         BoxTypeCmp *expand) {
  BoxTypeIter ti;
  if (BoxType_Get_Combinations(parent, & ti)) {
    BoxType t;
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

BoxType
BoxType_Find_Combination_With_Id(BoxType parent, BoxCombType type,
                                 BoxTypeId child_id, BoxTypeCmp *expand) {
  /* Quick hack: we should do this better!
   * This code relies on BoxType_Find_Combination not trying to link or
   * unlink the child type.
   */
  BoxTypeBundle child;
  child.header.type_class = BOXTYPECLASS_PRIMARY;
  child.data.primary.id = child_id;
  return BoxType_Find_Combination(parent, type, (BoxType) & child, expand);
}

/* Get details about a combination found with BoxType_Find_Combination. */
BoxBool
BoxType_Get_Combination_Info(BoxType comb, BoxType *child,
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
