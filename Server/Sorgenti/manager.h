#include "interface.h"

/* Colori */
#define RED "\x1b[31;1m"
#define GREEN "\x1b[32;1m"
#define YELLOW "\x1b[33;1m"
#define BLUE "\x1b[34;1m"
#define MAGENTA "\x1b[35;1m"
#define CYAN "\x1b[36;1m"
#define GREY "\x1b[30;1m"
#define NORMAL "\x1b[0;0m"

/* PROTOTIPI */

/* Funzione di debug che stampa la matrice anche sul server */
void stampaMatrice(char **matrice, int altezza, int ampiezza, int entry, int player_x, int player_y);

/* Ripristina i dati prima dell'uscita del giocatore; stampa l'errore sul file di log */
void lastWish(char *testo, int x, int y, int client_match, char *username, int client_socket);

/* Riduce il numero di giocatori */
void playersReduction(int client_match, char *username, int flag, int client_socket);

/* Riduce il numero di tesori ancora presenti in campo */
void treasuresReduction(int client_match, int client_socket);

/* Libera l'attuale cella del giocatore */
void freeCell(int x, int y, int client_match, int client_socket);

/* Invia al client le informazioni sui tesori trovati */
void sendInfo(int client_socket, int client_match, struct partita *elenco);

/* Stampa la lista degli utenti connessi al server */
void stampaListaBuffer(int client_socket, struct userlist *utenti);

/* Contiene una funzione di crittografia stabilita dal programmatore (è facilmente sostituibile) */
char *crittografia(char *password);

/* Funzione di crittografia */
char *encryptpw(char *password);

/* Thread dedicato interamente alla gestione del client */
void *clientManagement(void *client_socket);

/* Controlla che le credenziali inviate dal client siano valide per il login/signup */
int checkLogin(int scelta_login, char *username, char *password, int client_socket);

/* Gestisce il movimento del client sul campo */
int move(char **matrix, char buffer, int *x, int *y);
