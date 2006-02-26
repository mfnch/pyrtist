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
	YY_BUFFER_STATE buffer;
} TokIncludeData;

/* Variabili gestite in questo file */
UInt tok_linenum = 1;
static UInt tok_cur_include_level = 1, tok_max_include_level;
static Array *tok_include_list = NULL;

%}

SPACES	[ \t]*
DIGIT	[0-9]
NAME	[a-z][a-z0-9_]*
ANAME	[a-z_]+

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
"\n"		{return TOK_NEWLINE;}

 /* Il carattere di continuazione riga mangia tutto ci che segue
  * fino alla nuova riga e annulla il carattere newline
  */
"_"{SPACES}"\n"				{++tok_linenum;}
"_"{SPACES}"'"[^\n]*"\n"	{++tok_linenum;}

 /* Scarta i commenti (i commenti iniziano col carattere '
  * e terminano alla fine della riga; per commenti su piu' righe
  * bisogna mettere un ' all'inizio di ogni riga di commento)
  */
"'"[^\n]*	{}

 /* Ignora gli spazi */
{SPACES}	{}


"end"		{return TOK_END;}


 /* Numero di tipo intero */
{DIGIT}+	{
	
	char *s;

	yylval.val.type = TVAL_IMMINTG;
	yylval.val.num.intg = atoi(s = strndup(yytext, yyleng));
	free(s);
	return TOK_NUM;
 }

 /* Numero di tipo float */
{DIGIT}+("."{DIGIT}+)?([e][+-]?{DIGIT}+)?	{

	char *s;

	yylval.val.type = TVAL_IMMFLT;
	yylval.val.num.flt = atof(s = strndup(yytext, yyleng));
	free(s);
	return TOK_NUM;
 }

 /* Stringa: una stringa e' un'insieme di caratteri racchiusi
  * tra due caratteri ". Nel nostro programma l'utilizzo e la gestione
  * delle stringhe sono molto limitati: le stringhe possono
  * solo essere passate come argomento alle funzioni, alle istruzioni, etc
  */
["][^"\n]*["]	{
	yylval.nm.leng = yyleng - 2;
	yylval.nm.ptr = strndup(yytext+1, yyleng-2);
	return TOK_STRING;
 }

 /* Nome semplice: puo' essere una variabile esplicita o una sessione
  * (non sub-sessione)
  */
{NAME} {
	Symbol *s;

	MSG_LOCATION("yylex");

	s = Sym_Find_Simple_Name(yytext, yyleng);
	if ( s == NULL ) {
		/* Il nome non corrisponde ad un simbolo gia' definito:
		 * lo restituisco cosi' com'e'!
		 */
		yylval.nm.ptr = yytext;
		yylval.nm.leng = yyleng;
		return TOK_NAME;
	}

	if ( s->symattr.is_explicit ) {
		if ( s->symtype == CONSTANT ) {

		} else if ( s->symtype == VARIABLE ) { 

		} else {
			MSG_ERROR("Errore interno: simbolo esplicito che non e'"
			 "una variabile o una costante!");
			return TOK_ERR;
		}

	} else {
		if ( s->symtype == SESSION ) {
		} else if ( s->symtype == FUNCTION ) { 
		} else {
			MSG_ERROR("Errore interno: simbolo implicito che non e'"
			 "una funzione o una sessione!");
			return TOK_ERR;
		}
	}
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
 * NOTA: Restituisce 1 in caso di successo, 0 altrimenti.
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
	ERRORFUNC("Tok_Finish");
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

	ERRORFUNC("Tok_Include_Begin");

	if ( tok_cur_include_level >= tok_max_include_level ) {
		MSG_ERROR("Impossibile includere \"%s\": troppi file inclusi!", f);
		return Failed;
	}

	incfile = fopen( f, "rt" );

	if ( incfile == NULL ) {
		MSG_ERROR("Impossibile aprire il file \"%f\"!", f);
		return Failed;
	}

	cur.linenum = tok_linenum;
	cur.msgcontext = Msg_Context_Num();
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

		/* Cancella il buffer corrente */
		yy_delete_buffer( YY_CURRENT_BUFFER );
		/* Imposta il nuovo buffer all'ultimo inserito nello "stack" */
		cur = Arr_LastItem( tok_include_list, TokIncludeData );
		tok_linenum = cur.linenum;
		yy_switch_to_buffer( cur.buffer );
		/* Cancella l'ultimo elemento dello stack */
		(void) Arr_Dec( tok_include_list );
		--tok_cur_include_level;
		MSG_ADVICE( "Esco dal file incluso. Trovati %lu errori e %lu warnings.\n",
		 Msg_Num(MSG_NUM_ERRFAT), Msg_Num(MSG_NUM_WARNINGS) );
		Msg_Context_Return( cur.msgcontext );

		return 0;

	} else
		return 1;
}
