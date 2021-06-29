#include "winner.h"

/* PROTOTIPI */

/* Alloca la matrice */
void matrixAlloc(struct settings *GS);

/* Inserisce i valori di default nella struttura dati (che li contiene per modularita') */
void init(struct settings *GS);

/* Azzera la matrice e reimposta random i trofei e gli ostacoli */
void sow(struct settings *GS);

/* Dealloca la matrice */
void freeMatrix(struct settings * GS);

/* Funzione per la gestione di errori */
void errorExit(char *error);

/* E' il thread che si occupa dell'intera gestione della matrice */
void *initializer(void *GS);