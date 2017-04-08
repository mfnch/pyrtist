 /* File contenente le regole mediante cui lex produrra' il tokenizer.
  */

 /* include e' una start-condition che serve per prelevare il nome del file
  * in una direttiva include
  */
%x include

%{

/* Dati che servono per la gestione della direttiva "include" */
typedef struct {
  UInt linenum;
  UInt msgcontext;
  UInt num_errs, num_warns;
  YY_BUFFER_STATE buffer;
} TokIncludeData;

/* Variabili gestite in questo file */
UInt tok_linenum = 1;
static UInt tok_cur_include_level = 1, tok_max_include_level;
static Array *tok_include_list = NULL;

%}

SPACES  [ \t]*
DIGIT   [0-9]
NAME    [A-Za-z][A-Za-z0-9_]*
LNAME   [a-z][A-Za-z0-9_]*
UNAME   [A-Z][A-Za-z0-9_]*

%%

include BEGIN(include);

 /* Elimina gli spazi bianchi dopo la direttiva include */
<include>[ \t]*
 /* Cio' che segue include dopo gli eventuali spazi bianchi,
  * e' il nome del file: non sono consentiti commenti
  * o caratteri di continuazione riga sulla stessa riga di una include
  */
<include>["][^"\n]*["] {
  char *f = Str_Dup(yytext+1, yyleng-2);

  BEGIN(INITIAL);

  printf(">>> File: \"%s\"\n", f);

  if ( ! Tok_Include_Begin( f ) ) {
    free(f); return TOK_ERR;
  } else
    free(f);
}
 /* Tutto il resto da' errori! */
<include>. {
  BEGIN(INITIAL);
  return TOK_ERR;
}

 /* Se sto leggendo un file di include, quando arrivo alla sua fine
  * torno a leggere il precedente.
  */
<<EOF>> {
  if ( Tok_Include_End() ) {yyterminate();}
}

 /* Un istruzione deve stare tutta su una riga.
  * Se questo diventa problematico e' possibile usare il carattere
  * di continuazione riga _
  */
"\n"    {return TOK_NEWLINE;}

 /* Il carattere di continuazione riga mangia tutto ci che segue
  * fino alla nuova riga e annulla il carattere newline
  */
"_"{SPACES}"\n"       {++tok_linenum;}
"_"{SPACES}"'"[^\n]*"\n"  {++tok_linenum;}

 /* Scarta i commenti (i commenti iniziano col carattere '
  * e terminano alla fine della riga; per commenti su piu' righe
  * bisogna mettere un ' all'inizio di ogni riga di commento)
  */
"//"[^\n]*  {}

 /* Ignora gli spazi */
{SPACES}    {}

"end"       {return TOK_END;}

 /* Operatori di assegnazione */
"+="  { return TOK_APLUS; }
"-="  { return TOK_AMINUS; }
"*="  { return TOK_ATIMES; }
"%="  { return TOK_AREM; }
"/="  { return TOK_ADIV; }
"<<=" { return TOK_ASHL; }
">>=" { return TOK_ASHR; }
"&="  { return TOK_ABAND; }
"^="  { return TOK_ABXOR; }
"|="  { return TOK_ABOR; }

 /* Opertaori di incremento/decremento */
"++"  { return TOK_INC; }
"--"  { return TOK_DEC; }

 /* Opertaori di incremento/decremento */
"<<"  { return TOK_SHL; }
">>"  { return TOK_SHR; }

 /* Operatori di confronto */
"=="  { return TOK_EQ; }
"!="  { return TOK_NE; }
"<="  { return TOK_LE; }
">="  { return TOK_GE; }

"&&"  { return TOK_LAND; }
"||"  { return TOK_LOR; }

 /* Altri operatori */
"**"  { return TOK_POW; }

 /* Numero intero di tipo carattere */
[']([^'\n]|\\')*['] {
  Char c;

  if IS_FAILED( Str_ToChar(yytext+1, yyleng-2, & c) )
    { parser_attr.no_syntax_err = 1; return TOK_ERR;}
  Cmp_Expr_New_Imm_Char( & yylval.Ex, (Intg) c );
  return TOK_EXPR;
 }

 /* Numero di tipo intero */
{DIGIT}+ {
  Intg i;

  if IS_FAILED( Str_ToIntg(yytext, yyleng, & i) )
    { parser_attr.no_syntax_err = 1; return TOK_ERR;}
  Cmp_Expr_New_Imm_Intg( & yylval.Ex, i );
  return TOK_EXPR;
 }

 /* Numero di tipo float */
{DIGIT}+("."{DIGIT}+)?([e][+-]?{DIGIT}+)? {
  Real r;

  if IS_FAILED( Str_ToReal(yytext, yyleng, & r) )
    { parser_attr.no_syntax_err = 1; return TOK_ERR;}
  Cmp_Expr_New_Imm_Real( & yylval.Ex, r );
  return TOK_EXPR;
 }

 /* Stringa: una stringa e' un'insieme di caratteri racchiusi
  * tra due caratteri ".
  */
["]([^"\n]|\\\")*["] {
  char *s;
  Intg nl;

  s = Str_ToString(yytext+1, yyleng-2, & nl);
  if ( s == NULL ) return TOK_ERR;
  yylval.Nm.length = nl;
  yylval.Nm.text = s;
  return TOK_STRING;
 }

 /* Nome di variabile */
{LNAME} {
  Name nm = {yyleng, yytext};
  yylval.Nm = nm;
  return TOK_LNAME;
 }

 /* Nome di variabile */
{UNAME} {
  Name nm = {yyleng, yytext};
  yylval.Nm = nm;
  return TOK_UNAME;
 }

 /* Membro di struttura */
[.]{LNAME} {
  yylval.Nm.text = yytext + 1;
  yylval.Nm.length = yyleng - 1;
  return TOK_LMEMBER;
 }

 /* Membro di struttura */
[.]{UNAME} {
  yylval.Nm.text = yytext + 1;
  yylval.Nm.length = yyleng - 1;
  return TOK_UMEMBER;
 }

 /* Tutto il resto viene passato cosi' com'e' carattere per carattere al parser
  */
. {return yytext[0];}

%%

/* wrap function */
int yywrap(void) {
  return 1;
}

/* DESCRIZIONE: Prepara le funzioni di analisi lessicale del compilatore.
 *  Se f != NULL il compilatore partira' come se la prima istruzione
 *  del programma fosse una "include nomefile" dove nomefile e' la stringa
 *  a cui punta f.
 *  maxinc indica il numero massimo di file di include che possono essere
 *  aperti contemporaneamente.
 */
Task Tok_Init(UInt maxinc, char *f)
{
  tok_max_include_level = maxinc;

  /* La seguente variabile indica quanti file, aperti con include "nomefile",
   * non sono ancora stati chiusi
   */
  tok_cur_include_level = 0;

  /* Creo una lista dove salvare i buffer, ogni qual volta
   * incontro una include
   */
  Arr_Destroy(tok_include_list);
  tok_include_list = Array_New(sizeof(TokIncludeData), TOK_TYPICAL_MAX_INCLUDE);
  if ( tok_include_list == NULL ) return Failed;

  /* Se f == "nomefile" (e quindi f != NULL), eseguo una:
   * include "nomefile"
   */
  if ( f == NULL ) return Success;

  return Tok_Include_Begin(f);
}

/* DESCRIZIONE: Conclude l'analisi lessicale del compilatore.
 */
Task Tok_Finish(void)
{
  MSG_LOCATION("Tok_Finish");
  Arr_Destroy( tok_include_list );
  return Success;
}

/* DESCRIZIONE: Esegue materialmente la direttiva include,
 *  aprendo il file di nome f in lettura e deviando l'analisi lessicale
 *  su di esso. Bastera' poi eseguire tok_include_end() per ritornare al punto
 *  in cui la direttiva include aveva interrotto la lettura.
 */
Task Tok_Include_Begin(char *f)
{
  FILE *incfile;
  TokIncludeData cur;

  MSG_LOCATION("Tok_Include_Begin");

  if ( tok_cur_include_level >= tok_max_include_level ) {
    MSG_ERROR("Impossibile includere \"%s\": troppi file inclusi!", f);
    return Failed;
  }

  incfile = fopen( f, "rt" );

  if ( incfile == NULL ) {
    MSG_ERROR("Impossibile aprire il file \"%s\"!", f);
    return Failed;
  }

  cur.linenum = tok_linenum;
  cur.msgcontext = Msg_Context_Num();
  cur.num_errs = Msg_Num( MSG_NUM_ERRFAT );
  cur.num_warns = Msg_Num( MSG_NUM_WARNINGS );
  cur.buffer = YY_CURRENT_BUFFER;
  if IS_FAILED( Arr_Push(tok_include_list, & cur) ) return Failed;

  ++tok_cur_include_level;

  tok_linenum = 1;
  yy_switch_to_buffer(
   yy_create_buffer((yyin = incfile), YY_BUF_SIZE) );

  Msg_Context_Enter("File incluso \"%s\":\n", f);

  return Success;
}

/* DESCRIZIONE: Conclude la lettura di un file incluso con "include",
 *  restituendo 1 se non ci sono piu' file da leggere, 0 se bisogna riprendere
 *  la lettura dall'istruzione seguente la "include".
 */
UInt Tok_Include_End(void)
{
  if ( tok_cur_include_level > 0 ) {
    TokIncludeData cur;
    UInt num_errs, num_warns;

    /* Cancella il buffer corrente */
    yy_delete_buffer( YY_CURRENT_BUFFER );
    /* Imposta il nuovo buffer all'ultimo inserito nello "stack" */
    cur = Arr_LastItem( tok_include_list, TokIncludeData );
    tok_linenum = cur.linenum;
    yy_switch_to_buffer( cur.buffer );
    /* Cancella l'ultimo elemento dello stack */
    (void) Arr_Dec( tok_include_list );
    --tok_cur_include_level;
    /* Calcolo il numero di errori nel file incluso (+tutti i sotto-inclusi!) */
    num_errs = Msg_Num(MSG_NUM_ERRFAT) - cur.num_errs;
    num_warns = Msg_Num(MSG_NUM_WARNINGS) - cur.num_warns;
    MSG_ADVICE( "Esco dal file incluso. Trovati " SUInt " errori e "
     SUInt " warnings.",
     num_errs, num_warns );
    Msg_Context_Return( cur.msgcontext );

    return 0;

  } else
    return 1;
}