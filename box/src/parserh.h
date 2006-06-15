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

typedef struct {
  unsigned int old_box : 1;
  unsigned int no_syntax_err : 1;
} ParserAttr ;

extern ParserAttr parser_attr;

Task Parser_Init(UInt maxinc, char *f);
Task Parser_Finish(void);
Task Prs_Operator(Operator *opr, Expression *rs, Expression *a, Expression *b);
Task Prs_Name_To_Expr(Name *nm, Expression *e, Intg suffix);
Task Prs_Member_To_Expr(Name *nm, Expression *e, Intg suffix);
Task Prs_Suffix(Intg *rs, Intg suffix, Name *nm);
Expression *Prs_Def_And_Assign(Name *nm, Expression *e);
Task Prs_Array_Of_X(Expression *array, Expression *num, Expression *x);
Task Prs_Alias_Of_X(Expression *alias, Expression *x);
Task Prs_Species_New(Expression *species, Expression *first, Expression *second);
Task Prs_Species_Add(Expression *species, Expression *old, Expression *type);
Task Prs_Struct_New(Expression *strc, Expression *first, Expression *second);
Task Prs_Struct_Add(Expression *strc, Expression *old, Expression *type);
Task Prs_Procedure_Special(int *found, int type, int auto_define);
Task Prs_Rule_Typed_Eq_Typed(Expression *rs, Expression *typed1, Expression *typed2);
Task Prs_Rule_Valued_Eq_Typed(Expression *rs,
 Expression *valued, Expression *typed);
Task Prs_Rule_Typed_Eq_Valued(Expression *typed, Expression *valued);
Task Prs_Rule_Valued_Eq_Valued(Expression *rs,
 Expression *valued1, Expression *valued2);
int yyparse(void);
