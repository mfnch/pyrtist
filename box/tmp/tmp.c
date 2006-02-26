
#include "types.h"

/* Tipi possibili per un simbolo */
enum SymbolType { CONSTANT, VARIABLE, FUNCTION, SESSION  };

/* Tipi possibili per un dato (costante o variabile) */
enum DataType { VOID, INTEGER, FLOATING, POINT, STRING, OBJECT };

/* Sotto-Tipo di dato (specifica il tipo di oggetto) */
typedef unsigned long DataSub;

/* Numero di sessione */
typedef unsigned long SessionID;

/* Questa struttura descrive tutte le proprieta' di un simbolo
 * NOTA: brother serve a costruire la catena dei simboli di una sessione.
 *  Quando una sessione termina tale catena viene utilizzata per cancellarne
 *  i simboli.
 */
struct symbol {
  char          *name;       /* Nome del simbolo */
  UInt          leng;        /* Lunghezza del nome */
  SymbolType    symtype;     /* Tipo di simbolo */
  DataType      datatype;    /* Tipo di dato associato al simbolo */
  UInt          datasubtype; /* Sotto-tipo di dato associato al simbolo */
  SymbolAttr    symattr;     /* Attributi del simbolo */
  SymbolValue   symval;
  SessionID     parent;     /* Numero di sessione a cui appartiene il simb. */
  struct symbol *brother;   /* Prossimo simbolo nella catena (vedi sopra) */
  struct symbol *next;      /* Simbolo seguente nella hash-table */
  struct symbol *previous;  /* Simbolo precedente nella hash-table */
};

typedef struct symbol Symbol;

/* Questa struttura descrive una sessione: i simboli ad essa associati
 * e le sessioni figlie
 */
struct session {
	char		*name;		/* Nome del simbolo */
	UInt		leng;		/* Lunghezza del nome */
	DataType	rettype;	/* Tipo di dato restituito */
	UInt		retsubtype;	/* Sotto-tipo del dato restituito */

	struct session	*child;	/* Funzioni e sessioni "figlie" */
	struct symbol	*data;	/* Dati relativi alla sessione (lista di simboli) */
}

typedef struct session Session;


Symbol lib_graphic_data[] = {
	{"window", 6, OBJECT, },
	{"", 0}
};

Session lib_graphic_subs[] = {
	{"flist", 5, OBJECT, ITSELF, NULL, flist_sub},
	{"plist", 5, OBJECT, ITSELF, NULL, plist_sub},
	{"line", 4, OBJECT, OBJ_PLIST, NULL, line_sub},
	{"", 0}
};

Session lib_graphic = {"graphic", 7, VOID,, lib_graphic_sub, lib_graphic_data };
