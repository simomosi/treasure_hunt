#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* Thread */
#include <unistd.h>  /* read & write */
#include <string.h>
#include <time.h>
#include <netinet/ip.h> /* struct sockaddr_in & in_addr */
#include <arpa/inet.h> 	/* inet_aton */
#include <signal.h> 	/* segnali*/
#include <netdb.h>
#include <sys/ioctl.h> /* clear terminale */
#include <termios.h>
#include <time.h>

/* VALORI DI DEFAULT */
#define MAX_IP 45
#define MAXLENLOG 16	/* Lunghezza massima dell'username: se modificato, modificare anche acquireString */

/* COLORI */
#define RED "\x1b[31;1m"
#define GREEN "\x1b[32;1m"
#define YELLOW "\x1b[33;1m"
#define BLUE "\x1b[34;1m"
#define MAGENTA "\x1b[35;1m"
#define CYAN "\x1b[36;1m"
#define GREY "\x1b[30;1m"
#define NORMAL "\x1b[0;0m"


/* VARIABILI GLOBALI */
int client_socket;
struct termios saved_attributes;

/* PROTOTIPI */

/* Acquisisce un carattere tra {y, Y, n, N} */
char acquireAnswer();

/* Acquisisce un intero tra [min, max] */
int acquireRangedIntegerIncl(int max, int min);

/* Aquisisce una stringa e pulisce il buffer */
char * acquireString(char * str);

/* Gestione per la cattura dei segnali */
void handler(int signum);

/* Stampa su stderr e invia un segnale SIGABRT */
void errorExit(char *error);

/* Gestisce le impostazioni per la connessione al server */
void setup(char *ipAddress, int *port, struct sockaddr_in address);

/* Gestisce l'accesso al gioco (login/signup) */
void serverLog(char *username);

/* Acquisizione della password nascosta a video */
void acquirePassword(char *password);

/* Controlla che nella password non ci siano caratteri NON alfanumerici */
int illegalCheck(char *username, char *password);

/* Controlla che i dati di accesso inseriti siano validi */
int infoCheck(char *username, char *password, int scelta_log);

/* Dealloca la matrice locale al client*/
void freeMatrice(char **matrice, int altezza);

/* Dealloca la matrice locale al client */
char **allocaMatrice(char **matrice, int altezza, int ampiezza);

/* Inizializza la matrice: scrive il carattere '#' */
/* in tutte le celle non ancora visitate dall'utente */
void offuscaMatrice(char **matrice, int altezza, int ampiezza);

/* Stampa la matrice */
void stampaMatrice(char **matrice, int altezza, int ampiezza, int entry, int player_x, int player_y);

/* Aggiorna la matrice modificando l'ultima cella visitata dal giocatore */
void update_matrix(char **matrice, int *x, int *y, char comando, int entry);

/* Pulisce lo schermo evitando chiamate di sistema */
void clearTerm();

/* Gestisce l'ingresso del giocatore nel campo da gioco */
int parachute(char **matrice, int *x, int *y, int *tesori_trovati, int altezza, int ampiezza);

/* Ripristina l'input dello schermo rendendolo nuovamente visibile */
void reset_input_mode();

/* Nasconde l'input dallo schermo */
void set_input_mode();

/* Gestisce l'acquisizione dell'input del giocatore */
void clientInput(char *comando, int altezza, int ampiezza, int x, int y);

/* Trasforma i comandi con lo stesso esito in un unico comando da inviare al server */
char analyze(char client_msg);

/* Filtra l'input dell'utente, bloccando i movimenti che uscirebbero dai limiti del campo */
int legitPosition(char comando, int altezza, int ampiezza, int x, int y);

/* Stampa un menu' che illustra i possibili input validi */
void stampaHelp();
