/* Autore: Franchin Matteo - gennaio 2004
 *
 * File comune del linguaggio gdl.
 * Definizione delle principali macro usate per il debug
 * (stampa di messaggi di debug).
 */

/* Macro usate il debug */
#ifdef DEBUG
 #define EXIT_ERR(msg)	printf(msg); return 0
 #define EXIT_OK(msg)	printf(msg); return 1
 #define PRNMSG(msg)	printf(msg)
 #define PRNINTG(intg)	printf("%d", intg)
 #define PRNFLT(flt)	printf("%g", flt)
 #define PRNSTR(str)	printf("\"%s\"", str)
#else
 #define EXIT_ERR(msg)	return 0
 #define EXIT_OK(msg)	return 1
 #define PRNMSG(msg)
 #define PRNINTG(intg)
 #define PRNFLT(flt)
 #define PRNSTR(str)
#endif
