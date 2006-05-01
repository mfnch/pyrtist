/* parser.h - Autore: Franchin Matteo
 *
 * Questo file contiene le dichiarazioni delle funzioni del compilatore,
 * che servono per inizializzare ed avviare il compilatore.
 */

Task Parser_Init(UInt maxinc, char *f);
Task Parser_Finish(void);
Expression *Prs_Def_And_Assign(Name *nm, Expression *e);
Task Prs_Array_Of_X(Expression *array, Expression *num, Expression *x);
Task Prs_Alias_Of_X(Expression *alias, Expression *x);
Task Prs_Species_New(Expression *species, Expression *first, Expression *second);
Task Prs_Species_Add(Expression *species, Expression *old, Expression *type);
Task Prs_Struct_New(Expression *strc, Expression *first, Expression *second);
Task Prs_Struct_Add(Expression *strc, Expression *old, Expression *type);
Task Prs_Procedure_Special(int *found, int type,
 int fresh_object, int auto_define);
Task Prs_Rule_Typed_Eq_Typed(Expression *rs, Expression *typed1, Expression *typed2);
Task Prs_Rule_Valued_Eq_Typed(Expression *rs,
 Expression *valued, Expression *typed);
Task Prs_Rule_Typed_Eq_Valued(Expression *typed, Expression *valued);
Task Prs_Rule_Valued_Eq_Valued(Expression *rs,
 Expression *valued1, Expression *valued2);
int yyparse(void);
