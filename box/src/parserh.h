/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* $Id$ */

/*
 * Questo file contiene le dichiarazioni delle funzioni del compilatore,
 * che servono per inizializzare ed avviare il compilatore.
 */

#ifndef _PARSERH_H
#  define _PARSERH_H

#  include "typesys.h"

typedef struct {
  unsigned int old_box : 1;
  unsigned int no_syntax_err : 1;
} ParserAttr;


/*****************************************************************************
 * Data types used during parsing of structures                              *
 *****************************************************************************/
typedef struct {
  char *name;
  Type type;
} StrucMember;

typedef struct {
  Type type;
  Type previous;
} Struc;

/*****************************************************************************/

extern ParserAttr parser_attr;

Task Parser_Init(const char *f);
Task Parser_Finish(void);
Task Prs_Operator(Operator *opr, Expr *rs, Expr *a, Expr *b);
Task Prs_Name_To_Expr(Name *nm, Expr *e, Int suffix);
Task Prs_Member_To_Expr(Name *nm, Expr *e, Int suffix);
Task Prs_Suffix(Int *rs, Int suffix, Name *nm);
Expr *Prs_Def_And_Assign(Name *nm, Expr *e);
Task Prs_Array_Of_X(Expr *array, Expr *num, Expr *x);
Task Prs_Alias_Of_X(Expr *alias, Expr *x);
Task Prs_Species_New(Expr *species, Expr *first, Expr *second);
Task Prs_Species_Add(Expr *species, Expr *old, Expr *type);
Task Prs_Struct_New(Expr *strc, Expr *first, Expr *second);
Task Prs_Struct_Add(Expr *strc, Expr *old, Expr *type);
Task Prs_Rule_Typed_Eq_Typed(Expr *rs, Expr *typed1, Expr *typed2);
Task Prs_Rule_Valued_Eq_Typed(Expr *rs, Expr *valued, Expr *typed);
Task Prs_Rule_Typed_Eq_Valued(Expr *typed, Expr *valued);
Task Prs_Rule_Valued_Eq_Valued(Expr *rs, Expr *valued1, Expr *valued2);
int yyparse(void);

#endif
