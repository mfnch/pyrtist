/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
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

/* $Id$ */

/* 8 maggio 2004 - Semplici operazioni su stringhe. */

/* Just to use strndup! */
#define _GNU_SOURCE

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "config.h"
#include "types.h"
#include "messages.h"
#include "mem.h"
#include "str.h"

/* DESCRIZIONE: Questa funzione confronta due stringhe senza distinguere
 *  tra maiuscole e minuscole e restituisce Succesfull se le stringhe
 *  sono uguali, Failed se sono differenti
 */
Task Str_Eq(char *a, char *b) {
  while (*a != '\0') {
    if ( tolower(*(a++)) != tolower(*(b++)) )
      return Failed;
  }
  if (*b == '\0') return Success;
  return Failed;
}

/* DESCRIZIONE: Controlla se la prima stringa (s1 di lunghezza l1)
 *  coincide con la seconda (s2 di lunghezza l2).
 * NOTA: La seconda stringa deve essere scritta con caratteri minuscoli,
 *  mentre nella prima possono essere presenti indifferentemente lettere
 *  minuscole o maiuscole; in questo modo l'uguaglianza viene stabilita
 *  a meno di differenze maiuscolo/minuscolo. Se le due stringhe coincidono
 *  restituisce Success, altrimenti restituisce Failed.
 */
Task Str_CaseEq2(char *s1, UInt l1, char *s2, UInt l2) {
  if (l1 != l2) return Failed;

  for (; l1 > 0; l1--)
    if (tolower(*(s1++)) != *(s2++)) return Failed;

  return Success;
}

/* DESCRIZIONE: Controlla se la prima stringa (s1 di lunghezza l1)
 *  coincide con la seconda (s2 di lunghezza l2).
 * NOTA: Distingue maiuscole da minuscole.
 */
Task Str_Eq2(char *s1, UInt l1, char *s2, UInt l2) {
  if (l1 != l2) return Failed;
  for (; l1 > 0; l1--)
    if ( *(s1++) != *(s2++) ) return Failed;
  return Success;
}

/* DESCRIZIONE: Copia in minuscolo la stringa s (di lunghezza leng)
 *  e crea cosi' una nuova stringa di cui restituisce il puntatore.
 * NOTA: Restituisce NULL se non c'e' abbastanza memoria per creare
 *  la nuova stringa.
 */
char *Str_DupLow(char *s, UInt leng)
{
  char *ns, *nc;
  ns = (char *) Mem_Alloc(leng);
  for (nc = ns; leng > 0; leng--) *(nc++) = tolower(*(s++));
  return ns;
}

/* DESCRIZIONE: Copia la stringa s (di lunghezza leng) e crea una stringa
 *  asciiz, cioe' terminante con '\0'.
 * NOTA: Restituisce NULL se non c'e' abbastanza memoria per creare
 *  la nuova stringa.
 */
char *Str_Dup(const char *s, UInt leng) {
  char *ns, *nc;

  if (s == NULL || leng < 1)
    return (char *) Mem_Strdup("");
  ns = (char *) Mem_Alloc(leng + 1);
  for (nc = ns; leng > 0; leng--) *(nc++) = *(s++);
  *nc = '\0';
  return ns;
}

/* DESCRIZIONE: Taglia la stringa s, qualora la sua lunghezza superi maxleng.
 *  Se ad esempio s = "questa stringa e' troppo lunga" e maxleng = 20,
 *  Str_Cut restituira' "quest...troppo lunga" (start = 25).
 *  start specifica quale parte della stringa deve essere tagliata, in modo
 *  da farla stare in maxleng caratteri.
 *  start va da 0 a 100: indica la posizione (in percentuale) da dove iniziare
 *  a tagliare la stringa.
 * NOTA: In ogni caso la stringa restituita viene allocata da Str_Cut
 *  e sara' quindi necessario eventualmente usare free(...) per eliminarla.
 */
char *Str_Cut(const char *s, UInt maxleng, Int start) {
  return Str__Cut(s, strlen(s), maxleng, start);
}

/* DESCRIZIONE: Come Str_Cut, ma la lunghezza della stringa viene specificata
 *  direttamente attraverso il parametro leng.
 */
char *Str__Cut(const char *s, UInt leng, UInt maxleng, Int start) {
  if ( leng <= maxleng )
    return Mem_Strdup(s);

  else {
    char *rets, *c;
    rets = (char *) Mem_Alloc(maxleng + 1);

    if ( start < 0 ) start = 0;
      else if ( start > 100 ) start = 100;

    start = (maxleng * start) / 100;
    if ( start > 0 )
        (void) strncpy(rets, s, start);

    c = rets + start;
    start = maxleng - start - 3;
    if ( start < 1 ) {
      for( start = 3 + start; start > 0; start-- )
        *(c++) = '.';

      *c = '\0';
      return rets;
    }

    *(c++) = '.'; *(c++) = '.'; *(c++) = '.';
    (void) strncpy(c, s + (leng - start), start);

    rets[maxleng] = '\0';
    return rets;
  }
}

/* DESCRIPTION: This function reads one octal digit, whose ASCII code
 *  is in the character-constant c. In case of errors it sets *status = 1.
 */
unsigned char oct_digit(unsigned char c, int *status) {
  if ( (c >= '0') && (c <= '7') ) return c - '0';
  *status |= 1;
  return '\0';
}

/* DESCRIPTION: This function reads one hexadecimal digit, whose ASCII code
 *  is in the character-constant c. In case of errors it sets *status = 1.
 */
unsigned char hex_digit(unsigned char c, int *status) {
  c = tolower(c);
  if (c >= '0' && c <= '9') return c - '0';
   else if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
  *status |= 1;
  return '\0';
}

/* DESCRIPTION: This function reads the first (simple or escaped) character
 *  of the string s, whose length is l. It returns the lenght of the read
 *  character in *f (1 if it was a simple character, 1 to 4 if it was
 *  an escaped character). The function returns also the read character in *c
 *  (whose value is suitably translated into one single byte).
 * NOTE: This function is used by Str_ToChar and by Str_ToString.
 */
static Task Str__ToChar(char *s, Int l, Int *f, char *c) {
  Name nm = {l, s};

  if (l < 1) goto err_empty;

  if ( s[0] != '\\' ) {
    *c = s[0]; *f = 1; return Success;

  } else {
    register unsigned char s1 = s[1];

    if (l < 2) goto err_miss;
    if ( s1 == 'x' ) {
      register unsigned char d, d2;
      int err = 0;

      if ( l < 3 ) goto err_miss;
      d = hex_digit(s[2], & err);
      if ( err ) goto err_hex_digit;

      if ( l > 3 ) {
        d2 = hex_digit(s[3], & err);
        if (! err) {*f = 3; *c = (d << 4) | d2; return Success;}
      }
      *f = 2; *c = d; return Success;

    } else if ( (s1 >= '0') && (s1 <= '9') ) {
      register unsigned int d, d2;
      int err = 0;

      d = oct_digit(s1, & err);
      if ( err ) goto err_oct_digit;
      if ( l > 2 ) {
        d2 = oct_digit(s[2], & err);
        if (! err) {
          d = (d << 3) | d2;
          if ( l > 3 ) {
            d2 = oct_digit(s[3], & err);
            if (! err) {
              *f = 4; *c = d = (d << 3) | d2;
              if (d <= 255) return Success;
              goto err_overflow;
            }
          }
          *f = 3; *c = d; return Success;
        }
      }
      *f = 2; *c = d; return Success;

    } else {
      *f = 2;
      switch(s1) {
       case 'a':  *c = '\a'; return Success;
       case 'b':  *c = '\b'; return Success;
       case 'f':  *c = '\f'; return Success;
       case 'n':  *c = '\n'; return Success;
       case 'r':  *c = '\r'; return Success;
       case 't':  *c = '\t'; return Success;
       case 'v':  *c = '\v'; return Success;
       case '\\': *c = '\\'; return Success;
       case '\?': *c = '\?'; return Success;
       case '\'': *c = '\''; return Success;
       case '\"': *c = '\"'; return Success;
       default:
         MSG_ERROR("'%N' <- Wrong escape sequence.", & nm);
         return Failed;
      }
    }
  }

err_empty:
  MSG_ERROR("'' <- Missing character.");
  return Failed;
err_miss:
  MSG_ERROR("'%N' <- Unexpected end for the escape sequence.", & nm);
  return Failed;
err_hex_digit:
  nm.length = 3;
  MSG_ERROR("'%N' <- Wrong hexadecimal digit.", & nm);
  return Failed;
err_oct_digit:
  nm.length = 2;
  MSG_ERROR("'%N' <- Wrong ocatal digit", & nm);
  return Failed;
err_overflow:
  nm.length = 4;
  MSG_ERROR("'%N' <- This octal number is greater than 255.", & nm);
  return Failed;
}

/* DESCRIPTION: This function scans the string s (whose length is l)
 *  and converts it into a Char.
 */
Task Str_ToChar(char *s, Int l, char *c) {
  Int f;

  if IS_FAILED( Str__ToChar(s, l, & f, c) ) return Failed;
  if ( f == l )
    return Success;
  else {
    Name nm = {l, s};
    MSG_ERROR("'%N' <- Too many characters.", & nm);
    return Failed;
  }
}

/* DESCRIPTION: This function scans the C-type string s (whose length is l)
 *  and converts it into a string (array of char), which will be copied
 *  into out (this function uses malloc to allocate the string of output).
 */
char *Str_ToString(char *s, Int l, Int *new_length) {
  Int f, nl = 1; /* <-- incluso il '\0' di terminazione stringa */
  char *c, *out;

  c = out = (char *) Mem_Alloc(l+1);
  while (l > 0) {
    if IS_FAILED( Str__ToChar(s, l, & f, c) ) return NULL;
    ++c;
    ++nl;
    s += f;
    l -= f;
  }
  *c = '\0';
  if ( new_length != NULL ) *new_length = nl;
  return out;
}

/* DESCRIZIONE: Converte il numero da formato stringa a formato numerico.
 *  s e' il puntatore alla stringa, l la lunghezza. Il numero convertito verra'
 *  memorizzato in *i.
 */
Task Str_ToInt(char *s, UInt l, Int *i) {
  char sc[sizeof(Int)*5 + 1], *endptr;

  if ( l >= (sizeof(Int)*5 + 1) ) {
    MSG_ERROR("Intero troppo grande!");
    return Failed;
  }

  /* Copio la stringa in modo da poterla terminare con '\0' */
  strncpy(sc, s, l);
  sc[l] = '\0';

  errno = 0;
  *i = strtoint(sc, & endptr, 10);
  if ( errno == 0 ) return Success;

  MSG_ERROR("L'intero e' troppo grande!");
  return Failed;
}

/** Return the numerical value of a charecter when interpreted
 * as an hexadecimal digit. Returns -1 if the character is not an hex digit.
 * NOTE: I may use something like:
 *  return (digit >= '0' && digit <= '9') ?
 *         digit - '0' :
 *         ((digit >= 'a' && digit <= 'f') ? digit - 'a' : -1);
 * But this makes assumptions on the order of the character codes.
 * I feel unsure about that.
 */
static int Hex_Digit(char digit) {
  switch(tolower(digit)) {
  case '0': return 0;
  case '1': return 1;
  case '2': return 2;
  case '3': return 3;
  case '4': return 4;
  case '5': return 5;
  case '6': return 6;
  case '7': return 7;
  case '8': return 8;
  case '9': return 9;
  case 'a': return 10;
  case 'b': return 11;
  case 'c': return 12;
  case 'd': return 13;
  case 'e': return 14;
  case 'f': return 15;
  default: return -1;
  }
}

Task Str_Hex_To_Int(char *s, UInt l, Int *out) {
  char *c = s;
  UInt i, n = 0;

  for (i = 0; i < l; i++) {
    UInt digit = Hex_Digit(*(c++)),
         m = n << 4;
    if (m < n) {
      MSG_WARNING("Hexadecimal number is out of bounds!");
      return Success;
    }
    if (digit < 0) {
      MSG_ERROR("Bad digit in hexadecimal number!");
      return Failed;
    }
    n = m | digit;
  }
  *out = (Int) n;
  return Success;
}

/* Converte il numero da formato stringa a formato numerico.
 *  s e' il puntatore alla stringa, l la lunghezza. Il numero convertito verra'
 *  memorizzato in *r.
 */
Task Str_ToReal(char *s, UInt l, Real *r) {
  if ( l < 64 ) {
    char sc[64];

    /* Copio la stringa in modo da poterla terminare con '\0' */
    strncpy(sc, s, l);
    sc[l] = '\0';

    errno = 0;
    *r = strtoreal(sc, NULL);
    if ( errno == 0 ) return Success;

  } else {
    char *sc, *endptr;
    sc = (char *) Mem_Alloc(sizeof(char)*(l+1));

    /* Copio la stringa in modo da poterla terminare con '\0' */
    strncpy(sc, s, l);
    sc[l] = '\0';

    errno = 0;
    *r = strtoreal(sc, & endptr);
    Mem_Free(sc);
    if ( errno == 0 ) return Success;
  }

  MSG_ERROR("Il numero reale sta fuori dai limiti consentiti!");
  return Failed;
}


/* DESCRIZIONE: Restituisce un nome nullo (lunghezza pari a 0)
 */
Name *Name_Empty(void) {
  static Name nm = {(UInt) 0, (char *) NULL}; return & nm;
}

/* This function converts the string corresponding to the structure of type
 * Name into a normal C-string (NUL-terminated).
 * The string is allocated by this function, but should not be freed directly
 * by the user. For this purpose call: (void) Name_Str((Name) {0, NULL}).
 * After the user calls 'Name_Str' he must use the string, before the next
 * call to this same function is made. For this reason the statement:
 *   printf("Two Name-s: '%s' and '%s'\n", Name_Str(nm1), Name_Str(nm2));
 * I S   W R O N G!!!
 */
const char *Name_Str(Name *n) {
  static char *asciiz = NULL;
  Mem_Free(asciiz);
  if (n->length == 0) return "";
  asciiz = (char *) Mem_Alloc(n->length + 1);
  asciiz[n->length] = '\0';
  return strncpy(asciiz, n->text, n->length);
}

/* This function converts the string corresponding to the structure of type
 * Name into a normal C-string (NUL-terminated).
 */
char *Name_To_Str(Name *n) {
  char *asciiz = NULL;
  if (n->length == 0) return Mem_Strdup("");
  asciiz = (char *) Mem_Alloc(n->length + 1);
  asciiz[n->length] = '\0';
  return strncpy(asciiz, n->text, n->length);
}

void Name_From_Str(Name *dest, char *src) {
  dest->text = src;
  dest->length = strlen(src);
}

/* DESCRIZIONE: Rimuove la stringa associata a *n (pone n --> "")
 */
void Name_Free(Name *n) {Mem_Free(n->text); n->text = NULL; n->length = 0;}

/* DESCRIZIONE: Duplica la stringa *n.
 */
Name *Name_Dup(Name *n) {
  static Name rs;

  if ( n > 0 ) {
    rs.length = n->length;
    rs.text = strndup(n->text, n->length);
    if ( rs.text == NULL ) {MSG_FATAL("Memoria esaurita!");}
    return & rs;
  }
  rs.text = NULL;
  rs.length = 0;
  return & rs;
}

Task Name_Cat(Name *nm, Name *nm1, Name *nm2, int free_args) {
  register int l1 = nm1->length, l2 = nm2->length, l;
  register char *dest;

  if ( (l1 < 1) || (l2 < 1) ) goto strange_cases;
  if ( nm1->text[l1-1] == '\0' ) --l1;
  if ( nm2->text[l2-1] == '\0' ) --l2;
  l = l1 + l2;

  nm->text = dest = (char *) Mem_Alloc( nm->length = l + 1 );
  if ( l1 > 0 ) strncpy(dest, nm1->text, l1);
  if ( l2 > 0 ) strncpy(dest + l1, nm2->text, l2);
  *(dest + l) = '\0';
  if ( free_args ) {Name_Free(nm1); Name_Free(nm2);}
  return Success;

strange_cases: {
    Name *tmp = nm1;
    if ( l1 < 1 ) {tmp = nm2;}
    if ( free_args ) {
      *nm = *tmp;
      tmp->text = NULL;
      tmp->length = 0;
      return Success;
    } else {
      *nm = *Name_Dup(tmp);
      if ( nm->length > 0 ) return Success;
      return Failed;
    }
  }
}

void *Mem_Dup(const void *src, unsigned int length) {
  void *copy;
  if (length < 1) return NULL;
  copy = (void *) Mem_Alloc(length);
  memcpy(copy, src, length);
  return copy;
}

/** Similar to Mem_Dup, but allocate extra space */
void *Mem_Dup_Larger(const void *src, Int src_size, Int dest_size) {
  void *copy;
  assert(dest_size >= src_size && src_size >= 0);
  if (dest_size < 1) return NULL;
  copy = (void *) Mem_Alloc(dest_size);
  if (copy == NULL) return NULL;
  memcpy(copy, src, src_size);
  return copy;
}

#ifndef HAVE_STRNDUP
char *strndup(const char *s, int n) {
  int i, l;
  char *ret, *r;

  l = strlen(s);
  if ( l < n ) n = l;
  ret = r = (char *) malloc(n + 1);
  if (r == NULL) return NULL;
  for(i = 0; i < n; i++) *(r++) = *(s++);
  *r = '\0';
  return ret;
}
#endif
