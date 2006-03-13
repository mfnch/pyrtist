/* compiler.h - Autore: Franchin Matteo - dicembre 2004
 *
 * Questo file contiene le dichiarazioni per typeman.c, symbol.c, compiler.c
 */

typedef struct {
  struct {
    unsigned int imm     : 1; /* l'espressione e' immediata? */
    unsigned int value   : 1; /* Possiede un valore determinato? */
    unsigned int typed   : 1; /* Possiede tipo? */
    unsigned int ignore  : 1; /* Va ignorata o passata alla box? */
    unsigned int target  : 1; /* si puo' assegnare un valore all'espressione?*/
    unsigned int gaddr   : 1; /* addr e' un registro globale (o locale)? */
    unsigned int allocd  : 1; /* l'oggetto e' stato allocato? (va liberato?) */
    unsigned int release : 1; /* il registro va rilasciato automaticamente? */
  } is;

  Intg    addr;
  Intg    type;     /* The type of the expression */
  Intg    resolved; /* = Tym_Type_Resolve_All(type) */
  AsmArg  categ;
  union {
    Intg  i;
    Real  r;
    Point p;
    Intg  reg;
    Name  nm;
  } value;

} Expression;

struct Operation {
  struct {
    unsigned int intrinsic   : 1; /* E' una operazione intrinseca? */
    unsigned int commutative : 1; /* E' commutativa? */
    unsigned int immediate   : 1; /* Ammette l'operazione immediata? */
    unsigned int assignment  : 1; /* E' una operazione di assegnazione? */
    unsigned int link        : 1; /* E' una operazione di tipo "link"? */
  } is;
  Intg type1, type2, type_rs;	/* Tipi dei due argomenti */
  union {
    UInt asm_code;  /* Codice assembly dell'istruzione associata */
    Intg module;    /* Modulo caricato nella VM per eseguire l'operazione */
  }; /* <-- questa e' una union senza nome! Non e' ISO C purtroppo! */
  struct Operation *next;     /* Prossima operazione dello stesso operatore */
};

typedef struct Operation Operation;

struct Operator {
  unsigned int can_define : 1; /* E' un operatore di definizione? */
  char *name; /* Token che rappresenta l'operatore */
  /* Operazioni privilegiate, cioe' con collegamenti diretti */
  Operation *opn[3][CMP_PRIVILEGED];
  /* Operazioni non privilegiate, cioe' con collegamenti in catena */
  Operation *opn_chain;
};

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

/* Tipi possibili per un simbolo */
typedef enum { CONSTANT, VARIABLE, FUNCTION, BOX } SymbolType;

/* Questa struttura descrive tutte le proprieta' di un simbolo
 * NOTA: brother serve a costruire la catena dei simboli di una box.
 *  Quando una box viene chiusa tale catena viene utilizzata per cancellarne
 *  i simboli.
 */
struct symbol {
	/* Quantita' di competenza delle funzioni del tipo Sym_Symbol_... */
	char			*name;		/* Nome del simbolo */
	UInt			leng;		/* Lunghezza del nome */
	struct symbol	*next;		/* Simbolo seguente nella hash-table */
	struct symbol	*previous;	/* Simbolo precedente nella hash-table */

	/* Quantita' che descrivono il simbolo e lo relazionano agli altri */
	struct {
	  unsigned int is_explicit : 1;
	} symattr;					/* Attributi del simbolo */
	SymbolType		symtype;	/* Tipo di simbolo */
	Expression		value;		/* Valore dell'simbolo */
	struct symbol	*child; 	/* Catena dei simboli figli */
	struct symbol	*brother;	/* Prossimo simbolo nella catena (vedi sopra) */

	/* Questa union distingue i dati di simboli espliciti dagli impliciti */
	union {
		UInt	exp;	/* Caso esplicito: il genitore e' un esempio di box */
		Intg	imp;	/* Caso implicito: il genitore e' un tipo di box */
	} parent;
};

typedef struct symbol Symbol;

/* Questa struttura descrive un esempio di box */
typedef struct {
  Intg        ID;       /* Livello della box, cioe' numero che la identifica */
  Intg        type;     /* Tipo originario della box */
  Expression  value;    /* Valore della box */
  Symbol      *child;   /* Catena dei simboli "figli" della box */
} Box;

/* Enumero i tipi di tipo */
typedef enum {
  TOT_INSTANCE,
  TOT_PTR_TO,
  TOT_ARRAY_OF,
  TOT_SPECIE,
  TOT_PROCEDURE,
  TOT_ALIAS_OF,
  TOT_STRUCTURE
} TypeOfType;

/* Stringhe corrispondenti ai tipi di tipi */
#define TOT_DESCRIPTIONS { \
 "instance of ", "pointer to", "array of ", "set of types", "procedure " }

/* Enumeration of special procedures */
enum {
  PROC_OPEN    = -1,
  PROC_CLOSE   = -2,
  PROC_PAUSE   = -3,
  PROC_REOPEN  = -4,
  PROC_RECLOSE = -5,
  PROC_REPAUSE = -6,
  PROC_COPY    = -7,
  PROC_DESTROY = -8,
  PROC_SPECIAL_NUM = 9
};

extern char *tym_special_name[PROC_SPECIAL_NUM];

/* Struttura usata per descrivere i tipi di dati */
typedef struct {
  TypeOfType tot;
  Intg parent;     /* Specie a cui appartiene il tipo */
  Intg greater;    /* Tipo in cui puo' essere convertito */
  Intg size;       /* Spazio occupato in memoria dal tipo */
  Intg target;     /* Per costruire puntatori a target, array di target, etc */
  Intg procedure;  /* Prima procedura corrispondente al tipo */
  union{
    Intg asm_mod;  /* Numero di modulo associato alla procedura */
    Intg arr_size; /* Numero di elementi dell'array */
    Intg st_size;  /* Numero di elementi della struttura */
    Intg sp_size;  /* Numero di elementi della specie */
    Symbol *sym;   /* Simbolo associato al tipo (NULL = non ce n'e'!)*/
  };
  char *name;      /* Nome del tipo */
} TypeDesc;

/* Definisco il tipo SymbolAction. Se dichiaro:
 *  SymbolAction action;
 * action sara' una funzione in grado di operare o analizzare su un simbolo
 * (tipo Symbol). Funzioni di questo tipo vengono chiamate da quelle funzioni
 * (come ad esempio Sym_Symbol_Find) che scorrono la lista dei simboli,
 * in modo da poter eseguire un'operazione o controllo sul simbolo trovato.
 * Il valore restituito e' un UInt che informa la funzione chiamante dell'esito
 * dell'operazione/controllo. I valori sono:
 *   0 tutto Ok, e' possibile passare al prossimo simbolo;
 *   1 tutto Ok, ma non occorre continuare e passare al prossimo simbolo;
 *   2 qualcosa non e' andato correttamente, meglio che la funzione chiamante
 *     esca immediatamente.
 */
typedef UInt SymbolAction(Symbol *s);

/* Tipo legato all'uso di Cmp_Operation_Find */
typedef struct {
  unsigned int commute :1;
  unsigned int expand1 :1;
  unsigned int expand2 :1;
  Intg exp_type1;
  Intg exp_type2;
} OpnInfo;

/* This type is used to specify a container (see the macros CONTAINER_...) */
typedef struct {
  int type_of_container;
  int which_one;
} Container;

/* These macros are used in functions such as Cmp_Expr_Container_New
 * or Cmp_Expr_Container_Change to specify the container for an expression.
 */

/* The container is immediate (contains directly the value
 * of the integer, real, etc.)
 */
#define CONTAINER_IMM (& (Container) {0, 0})

/* In the function Cmp_Expr_Container_Change this will behaves similar
 * to CONTAINER_LREG_AUTO, but with one difference. If the expression
 * is already contained in a local register:
 *  - macro CONTAINER_LREG_AUTO: will keep the expression
 *    in its current container;
 *  - macro CONTAINER_LREG_FORCE: will choose another local register
 *    and use this as the new container.
 */
#define CONTAINER_LREG_FORCE (& (Container) {1, -2})

/* A local register automatically chosen (and reserved) */
#define CONTAINER_LREG_AUTO (& (Container) {1, -1})

/* A well defined local register */
#define CONTAINER_LREG(num) (& (Container) {1, num > 0 ? num : 0})

/* A well defined local variable */
#define CONTAINER_LVAR(num) (& (Container) {2, num > 0 ? num : 0})

/* A local variable automatically chosen (and reserved) */
#define CONTAINER_LVAR_AUTO (& (Container) {2, -1})

/* A well defined global register */
#define CONTAINER_GREG(num) (& (Container) {3, num > 0 ? num : 0})

/* A well defined global variable */
#define CONTAINER_GVAR(num) (& (Container) {4, num > 0 ? num : 0})

extern struct cmp_opr_struct cmp_opr;
extern AsmOut *Cmp_Curr_Output;
extern Intg cmp_box_level;

/* Variabili definite in 'typeman.c'*/
extern int tym_must_expand;
extern Intg tym_recent_type;
extern TypeDesc *tym_recent_typedesc;

/* Important builtin types */
extern Intg type_Point, type_RealNum, type_IntgNum, type_CharNum;

Intg Tym_Type_Next(void);
TypeDesc *Tym_Type_New(Name *nm);
Intg Tym_Type_Newer(void);
TypeDesc *Tym_Type_Get(Intg t);
Intg Tym_Type_Size(Intg t);
const char *Tym_Type_Name(Intg t);
char *Tym_Type_Names(Intg t);
Intg Tym__Box_Abstract_New(Name *nm, Intg size, Intg aliased_type);
Symbol *Tym_Symbol_Of_Type(Intg type);
Task Tym_Box_Abstract_Member_New(Intg parent, Name *nm, Intg type);
Task Tym_Box_Abstract_Delete(Intg type);
void Tym_Box_Abstract_Print(FILE *stream, Intg type);
Intg Tym_Build_Array_Of(Intg num, Intg type);
Intg Tym_Build_Pointer_To(Intg type);
Intg Tym_Build_Alias_Of(Name *nm, Intg type);
int Tym_Compare_Types(Intg type1, Intg type2, int *need_expansion);
Intg Tym_Type_Resolve(Intg type, int not_alias, int not_species);
#define Tym_Type_Resolve_Alias(type) Tym_Type_Resolve(type, 0, 1)
#define Tym_Type_Resolve_Species(type) Tym_Type_Resolve(type, 1, 0)
#define Tym_Type_Resolve_All(type) Tym_Type_Resolve(type, 0, 0)
Task Tym_Delete_Type(Intg type);
Intg Tym_Build_Procedure(Intg proc, Intg of_type, Intg asm_module);
Intg Tym_Search_Procedure(Intg proc, Intg of_type, Intg *containing_species);
void Tym_Print_Procedure(FILE *stream, Intg of_type);
Task Tym_Build_Specie(Intg *specie, Intg type);
Task Tym_Build_Structure(Intg *strc, Intg type);
Task Tym_Structure_Get(Intg *type);

#define Tym_Box_Abstract_New(nm) Tym__Box_Abstract_New(nm, (Intg) 0, TYPE_NONE)
#define Tym_Box_Abstract_New_Alias(nm, type) \
 Tym__Box_Abstract_New(nm, (Intg) -1, type)
#define Tym_Intrinsic_New(nm, size) Tym__Box_Abstract_New(nm, size, TYPE_NONE)

/* Procedure definite in 'symbol.c'*/
UInt Sym__Hash(char *name, UInt leng);
Symbol *Sym_Symbol_Find(Name *nm, SymbolAction action);
#ifdef DEBUG
void Sym_Symbol_Diagnostic(FILE *stream);
#endif
Symbol *Sym_Symbol_New(Name *nm);
void Sym_Symbol_Delete(Symbol *s);
Task Cmp_Box_Instance_Begin(Expression *e);
Task Cmp_Box_Instance_End(void);
Intg Box_Search_Opened(Intg type, Intg depth);
Box *Box_Get(Intg depth);
Symbol *Sym_Implicit_Find(Intg parent, Name *nm);
Symbol *Sym_Implicit_New(Intg parent, Name *nm);
#define EXACT_DEPTH 0
#define NO_EXACT_DEPTH 1
Symbol *Sym_Find_Explicit(Name *nm, Intg depth, int mode);
Symbol *Sym_New_Explicit(Name *nm, Intg depth);

/* Procedure definite in 'compiler.c'*/
Operator *Cmp_Operator_New(char *token);
Operation *Cmp_Operation_Add(Operator *opr, Intg type1, Intg type2, Intg typer);
Operation *Cmp_Operation_Find(Operator *opr,
 Intg type1, Intg type2, Intg typer, OpnInfo *oi);
Task Cmp_Conversion(Intg type1, Intg type2, Expression *e);
Task Cmp_Conversion_Exec(Expression *e, Intg type_dest, Operation *c_opn);
Expression *Cmp_Member_Intrinsic(Expression *e, Name *m);
Expression *Cmp_Member_Get(Expression *e, Name *m);
Expression *Cmp_Operator_Exec(Operator *opr, Expression *e1, Expression *e2);
Expression *Cmp_Operation_Exec(Operation *opn, Expression *e1, Expression *e2);
void Cmp_Expr_Print(FILE *out, Expression *e);
Task Cmp_Expr_Container_New(Expression *e, Intg type, Container *c);
Task Cmp_Expr_LReg(Expression *e, Intg t, int zero);
Task Cmp_Free(Expression *expr);
Task Cmp_Expr_To_X(Expression *expr, AsmArg categ, Intg reg, int and_free);
Expression *Cmp__Expr_To_LReg(Expression *expr, int force);
Task Cmp_Expr_To_Ptr(Expression *expr, AsmArg categ, Intg reg, int and_free);
Task Cmp_Expr_Create(Expression *e, Intg type, int temporary);
Task Cmp_Expr_Destroy(Expression *e);
Task Cmp_Expr_Copy(Expression *e_dest, Expression *e_src);
Task Cmp_Expr_Move(Expression *e_dest, Expression *e_src);
Task Cmp_Def_C_Procedure(Intg procedure, Intg of_type, Task (*C_func)(void));

Expression *Cmp_Expr_Reg0_To_LReg(Intg t);
Task Cmp_Expr_O_To_OReg(Expression *e);
Task Cmp_Complete_Ptr_1(Expression *e);
Task Cmp_Complete_Ptr_2(Expression *e1, Expression *e2);
Task Cmp_Define_Builtins();
void Cmp_Expr_New_Imm_Char(Expression *e, Char c);
void Cmp_Expr_New_Imm_Intg(Expression *e, Intg i);
void Cmp_Expr_New_Imm_Real(Expression *e, Real r);
Task Cmp_Expr_Target_New(Expression *e, Intg type);
Task Cmp_Expr_Target_Delete(Expression *e);
void Cmp_Expr_New_Imm_Point(Expression *e, Point *p);
Task Cmp_Data_Init(void);
Intg Cmp_Data_Add(Intg type, void *data, Intg size);
void Cmp_Data_Destroy(void);
Task Cmp_Data_Prepare(void);
Task Cmp_Data_Display(FILE *stream);
Task Cmp_Imm_Init(void);
Intg Cmp_Imm_Add(Intg type, void *data, Intg size);
void Cmp_Imm_Destroy(void);
Task Cmp_String_New(Expression *e, Name *str, int free_str);
#define Cmp_String_New_And_Free(e, str) Cmp_String_New(e, str, 1)
Task Cmp_Procedure_Search
 (Intg procedure, Intg prefix, Box **box, Intg *prototype, Intg *asm_module);
Task Cmp_Procedure(Expression *e, Intg prefix);
Task Cmp_Structure_Begin(void);
Task Cmp_Structure_Add(Expression *e);
Task Cmp_Structure_End(Expression *new_struct);
Task Cmp_Structure_Get(Expression *member, int *n);
Task Cmp_Expr_Expand(Intg species, Expression *e);
Task Cmp_Convert(Intg type, Expression *e);

#define Cmp_Expr_To_LReg(expr) Cmp__Expr_To_LReg(expr, 0)
#define Cmp_Expr_Force_To_LReg(expr) Cmp__Expr_To_LReg(expr, 1)
#define Cmp_Expr_To_Reg0(expr) Cmp__Expr_To_LReg(expr, 0, 1)
#define Cmp_Expr_Force_To_Reg0(expr) Cmp__Expr_To_LReg(expr, 1, 1)
#define Cmp_Operation_Find2(o, t1, t2) Cmp_Operation_Find(o, t1, t2, TYPE_NONE)
#define Cmp_Expr_Reg0(e, t) Cmp_Expr_LReg(e, t, 1)

Task Lib_Define_All(void);
