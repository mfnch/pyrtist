/****************************************************************************
 * Copyright (C) 2009 by Matteo Franchin                                    *
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

/** @file operator.h
 * @brief A module to manage compilation of operations.
 *
 * A nice description...
 */

#ifndef _OPERATOR_H
#  define _OPERATOR_H

/** INTERNAL: Called by BoxCmp_Init to initialise the operator table. */
void BoxCmp_Operator_Init(BoxCmp *c);

/** INTERNAL: Called by BoxCmp_Finish to finalise the operator table. */
void BoxCmp_Operator_Finish(BoxCmp *c);

typedef struct {
  unsigned int can_define : 1; /* E' un operatore di definizione? */
  char *name; /* Token che rappresenta l'operatore */
#if 0
  /* Operazioni privilegiate, cioe' con collegamenti diretti */
  Operation *opn[3][CMP_PRIVILEGED];
  /* Operazioni non privilegiate, cioe' con collegamenti in catena */
  Operation *opn_chain;
#endif
} Operator;

#if 0
struct Operation {
  struct {
    unsigned int intrinsic   : 1; /* E' una operazione intrinseca? */
    unsigned int commutative : 1; /* E' commutativa? */
    unsigned int immediate   : 1; /* Ammette l'operazione immediata? */
    unsigned int assignment  : 1; /* E' una operazione di assegnazione? */
    unsigned int link        : 1; /* E' una operazione di tipo "link"? */
  } is;
  Int type1, type2, type_rs;   /* Tipi dei due argomenti */
  union {
    UInt asm_code;  /* Codice assembly dell'istruzione associata */
    Int module;    /* Modulo caricato nella VM per eseguire l'operazione */
  }; /* <-- questa e' una union senza nome! Non e' ISO C purtroppo! */
  struct Operation *next;     /* Prossima operazione dello stesso operatore */
};

typedef struct Operation Operation;

typedef struct Operator Operator;

/* "Collezione" di tutti gli operatori */
struct cmp_opr_struct {
  /* Operatore usato per la conversione fra tipi diversi */
  Operator *converter;

  /* Operatori di assegnazione */
  Operator *assign;
  Operator *aplus;
  Operator *aminus;
  Operator *atimes;
  Operator *adiv;
  Operator *arem;
  Operator *aband;
  Operator *abxor;
  Operator *abor;
  Operator *ashl;
  Operator *ashr;

  /* Operatori di incremento/decremento */
  Operator *inc;
  Operator *dec;

  /* Operatori aritmetici convenzionali */
  Operator *pow;
  Operator *plus;
  Operator *minus;
  Operator *times;
  Operator *div;
  Operator *rem;

  /* Operatori bit-bit */
  Operator *bor;
  Operator *bxor;
  Operator *band;
  Operator *bnot;

  /* Operatori di shift */
  Operator *shl;
  Operator *shr;

  /* Operatori di confronto */
  Operator *eq;
  Operator *ne;
  Operator *gt;
  Operator *ge;
  Operator *lt;
  Operator *le;

  /* Operatori logici */
  Operator *lor;
  Operator *land;
  Operator *lnot;
};
#endif

#endif /* _OPERATOR_H */
