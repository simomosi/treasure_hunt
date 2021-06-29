#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 	/* read & write */
#include <string.h>
#include <pthread.h>
#include <netinet/ip.h> /* struct sockaddr_in & in_addr (netinet/in.h?) */
#include <arpa/inet.h> 	/* inet_ntop */
#include <sys/types.h> 	/* open */
#include <sys/stat.h> 	/* open */
#include <fcntl.h> 		/* open */
#include <signal.h> 	/* segnali*/
#include <crypt.h>		/* crittografia */

#define DEBUG 0			/* Stampa la matrice nel server: 0 disattivato, non-0 attivato*/

#ifndef _ID_PARTITA
#define _ID_PARTITA

extern int ID_PARTITA;

#endif

/* Valori standard */
#define DEFAULT_PORT 8004
#define MIN_PORT 1024	/* Le altre porte sono riservate al sistema */
#define QUEUE_LENGTH 64	/*Coda di attesa del server*/

#define MAXLEN 512
#define MAXLENLOG 16	/* Lunghezza massima di username/password */

#define MINMATR 8
#define RANGEMATR 8 	/* MAX = MIN * RANGE */

#define LEN_EVENTO 256	/* Array per le informazioni da stampare su stderr/file di log */

#define TIMESIZE 21 	/* Lunghezza stringa per il timestamp */

#define TIMELIMIT 60 	/* Tempo limite in secondi */

/* Variabili globali per l'handler di segnali / gestore matrice / gestione login */
int server_socket;
int log_file;
int file_login;
pthread_mutex_t mutex_partita, mutex_credenziali, mutex_log, mutex_informazioni;
time_t tempo_partenza;	/* Tempo di inizio della partita in corso */

/* Struttura contenente i valori*/
struct settings {
	char **matrix;		/*Campo di gioco*/
	int height, width, treasures, totTreasures, obstacles, timeLimit;	/*Impostazioni campo*/
};

struct settings *GS;

/*	Legenda matrice:
 *	Slot libero = f; slot con tesoro = t; slot con ostacolo = o; slot con giocatore = p;
 */

/* Struttura degli utenti connessi al server */
struct userlist {
	char *username;
	struct userlist *next;
	struct userlist *prev;
};

struct userlist *utenti;

/* Struttura con le informazioni sulla partita in corso */
struct partita
{
	int id;						/* Id partita */
	int players;				/* Numero giocatori ancora attivi */
	int totPlayers;				/* Numero giocatori totali connessi */
	int totali;					/* Numero di tesori totali */
	int trovati;				/* Numero di tesori trovati */
	struct tesori *found;		/* Struttura con le informazioni sui tesori trovati */
	char* vincitore;			/* Username del vincitore */
	struct partita *next;		/* Puntatore alla prossima partita */
	struct partita *prev;		/* Puntatore alla precedente partita */
};

struct partita *elenco;

/* Struttura con le informazioni sui tesori trovati */
struct tesori
{
	int *x;						/* Array coordinata x dei tesori trovati */
	int *y;						/* Array coordinata y dei tesori trovati */
	char **username;			/* Array di username dei tesori trovati */
	int *identity;				/* Array di id degli utenti */
	int *tempo;					/* Array che segna i secondi al tempo del ritrovamento dei tesori */
};

/* PROTOTIPI */

/* Alloca una struttura con l'username */
struct userlist *newNode(char *username);

/* Inserisce l'username in una lista ordinata se non è presente. In caso di errore, entry = -1 */
struct userlist *insert(struct userlist *utenti, char *username, int *entry);

/* Elimina il nodo con chiave username */
struct userlist *deleteNode(struct userlist *utenti, char *username);

/* Elimina l'intera lista di utenti */
struct userlist *deleteList(struct userlist *utenti);

/* Alloca una struttura con le info della nuova partita */
struct partita *newGame(int id, int num_tesori);

/* Aggiunge una struttura con le info della nuova partita alla lista di partite in corso */
struct partita *appendGame(struct partita *current, struct partita *nuova);

/* Dealloca una struttura con le info della nuova partita*/
struct partita *deleteGame(struct partita *current);

/* Ritorna il puntatore ad una struttura con l'id corrispondente, null altrimenti */
struct partita *search(struct partita *current, int id);

/* Ritorna il numero di giocatori della partita */
int getPlayer(struct partita *current);

/* Incrementa il numero di giocatori di una partita e ne ritorna il valore */
int addPlayer(struct partita *current, int client_socket);

/* Riduce il numero di giocatori di una partita e ne ritorna il valore */
int redPlayer(struct partita *current);

/* Aggiunge il nuovo tesoro alle informazioni della partita e ritorna il numero di tesori trovati */
int newTreasure(struct partita *current, int x, int y, char *username, int identity, int tempo, int client_socket);

/* Elimina tutte le partite terminate con nessun giocatore rimasto */
struct partita *garbageCollector(struct partita *current);

/* Elimina tutte le partite */
struct partita *garbageDestroyer(struct partita *current);

/* Restituisce il nome del vincitore della partita */
void getWinner(struct partita *current);

/* Scrive su file di log una stringa opportunamente formattata: Timestamp + [THREAD_CLIENT_#] + testo */
void writeLogClient(char *testo, int client_socket);

/* Come writeLogClient ma dal lato server (con inclusione di raise(sigabrt) e di perror(errore) ) */
void writeLogServer(char *testo);

/* Log del sistema sul file 'log_file'.txt*/
void logEvent(char * stringa, char * timeStamp);

/* Restituisce una stringa con le informazioni su data ed ora corrente*/
char *timeStamp();
