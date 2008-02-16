/* messages.h - Autore: Franchin Matteo - maggio 2004
 *
 * File header per usare le funzioni definite in 'messages.c'
 */

typedef void (*MsgAction)(UInt *errlev);

void Msg_PrintF(UInt level, const char *fmt, ...);
void Msg_Init(UInt ignore_level, UInt act_level, UInt ctxt_always, MsgAction fn);
UInt Msg_Context_Num(void);
void Msg_Context_Exit(Intg n);
void Msg_Context_Return(UInt n);
UInt Msg_Num(Intg t);
void Msg_Num_Reset_All(void);
extern void (*Msg_Exit_Now)(char *msg);

#ifdef DEBUG
#define MSG_LOCATION(fname) fprintf(stderr, fname " function entered!\n");
#else
#define MSG_LOCATION(fname)
#endif

/* Le seguenti macro servono a segnalare errori o messaggi,
 * sono "variadic macros" cioe' possono avere un numero imprecisato di argomenti
 * (questa caratteristica e' supportata purtroppo solo dal C99)
 */
/* Macro per segnalare un errore */
#define MSG_ADVICE(...) Msg_PrintF(1, __VA_ARGS__)
#define MSG_WARNING(...) Msg_PrintF(4, __VA_ARGS__)
#define MSG_ERROR(...) Msg_PrintF(8, __VA_ARGS__)
#define MSG_FATAL(...) Msg_PrintF(12, __VA_ARGS__)

/* Macro per entrare in un nuovo contesto */
#define Msg_Context_Enter(...) Msg_PrintF(0, __VA_ARGS__)

#define MSG_NUM_ADVICES 0
#define MSG_NUM_WARNINGS 1
#define MSG_NUM_ERRORS 2
#define MSG_NUM_FATALS 3
#define MSG_NUM_ALL -1
#define MSG_NUM_WARNERRFAT -2
#define MSG_NUM_ERRFAT -3
