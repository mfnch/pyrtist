/* Autore: Franchin Matteo - 8 maggio 2004
 *
 *  File header associato al codice contenuto nel file "str.c"
 */

Task Str_Eq(char *a, char *b);
Task Str_Eq2(char *s1, UInt l1, char *s2, UInt l2);
Task Str_CaseEq2(char *s1, UInt l1, char *s2, UInt l2);
char *Str_DupLow(char *s, UInt leng);
char *Str_Dup(char *s, UInt leng);
char *Str_Cut(const char *s, UInt maxleng, Intg start);
char *Str__Cut(const char *s, UInt leng, UInt maxleng, Intg start);
unsigned char oct_digit(unsigned char c, int *status);
unsigned char hex_digit(unsigned char c, int *status);
Task Str_ToChar(char *s, Intg l, char *c);
Task Str_ToIntg(char *s, UInt l, Intg *i);
Task Str_ToReal(char *s, UInt l, Real *r);
char *Str_ToString(char *s, Intg l, Intg *new_length);
Name *Name_Empty(void);
const char *Name_To_Str(Name *n);
void Name_Free(Name *n);
Name *Name_Dup(Name *n);
Task Name_Cat(Name *nm, Name *nm1, Name *nm2, int free_args);
#define Name_Cat_And_Free(nm, nm1, nm2) Name_Cat(nm, nm1, nm2, 1)

#ifndef HAVE_STRNDUP
char *strndup(const char *s, int n);
#endif
