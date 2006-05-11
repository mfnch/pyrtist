/* tokenizer.h - Autore: Franchin Matteo - maggio 2004
 *
 * File header per usare le funzioni definite in 'tokenizer.lex.c'
 */

int yywrap(void);
int yylex(void);
Task Tok_Init(UInt maxinc, char *f);
Task Tok_Finish(void);
Task Tok_Include_Begin(char *f);
UInt Tok_Include_End(void);
void Tok_Unput(int c);
