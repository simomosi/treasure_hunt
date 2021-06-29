#include "manager.h"

/* Funzione di debug che stampa la matrice anche sul server */
void stampaMatrice(char **matrice, int altezza, int ampiezza, int entry, int player_x, int player_y)
{
	int x, y;
	printf("\n\n   ");
	for (x = 0; x < ampiezza + 1; x++)
		printf("%s%2d%s ", BLUE, x, NORMAL);
	printf("\n   ");
	for (x = 0; x < altezza; x++)
	{
		printf("%s%2d%s", BLUE, x+1, NORMAL);
		for (y = 0; y < ampiezza; y++) {
			switch(matrice[x][y]) {
				case 'f':
					printf(" %s #%s", GREY, NORMAL);
					break;
				case 'p':
					printf(" %s @%s", CYAN, NORMAL);
					break;
				case 'o':
					printf(" %s X%s", RED, NORMAL);
					break;
				default:
					printf(" %s $%s", YELLOW, NORMAL);
					break;
			}
		}
		printf("\n   ");
	}
}

/* Ripristina i dati prima dell'uscita del giocatore; stampa l'errore sul file di log */
void lastWish(char *testo, int x, int y, int client_match, char *username, int client_socket)
{
	writeLogClient(testo, client_socket);
	playersReduction(client_match, username, 1, client_socket);
	freeCell(x, y, client_match, client_socket);
	close(client_socket);
	raise(SIGUSR1);
}

/* Riduce il numero di giocatori */
void playersReduction(int client_match, char *username, int flag, int client_socket)
{
	if (pthread_mutex_lock(&mutex_informazioni))
	{
		writeLogClient("[ERROR] Lock informazioni_partita.", client_socket);
		raise(SIGUSR1);
	}
	redPlayer(search(elenco, client_match));
	if (flag)
		utenti = deleteNode(utenti, username);
	if (pthread_mutex_unlock(&mutex_informazioni))
	{
		writeLogClient("[ERROR] Unlock informazioni_partita.", client_socket);
		raise(SIGUSR1);
	}
}

/* Riduce il numero di tesori ancora presenti in campo */
void treasuresReduction(int client_match, int client_socket)
{
	if (client_match > -1 && client_match == ID_PARTITA) /* Prima di uscire rendo libera la posizione occupata dal client */
	{
		if (pthread_mutex_lock(&mutex_informazioni))
		{
			writeLogClient("[ERROR] Lock mutex_informazioni.", client_socket);
			raise(SIGUSR1);
		}
		GS->treasures--;
		if (pthread_mutex_unlock(&mutex_informazioni))
		{
			writeLogClient("[ERROR] Unlock mutex_informazioni.", client_socket);
			raise(SIGUSR1);
		}
	}
}

/* Libera l'attuale cella del giocatore */
void freeCell(int x, int y, int client_match, int client_socket)
{
	if (client_match > -1 && client_match == ID_PARTITA) /* Prima di uscire rendo libera la posizione occupata dal client */
	{
		if (pthread_mutex_lock(&mutex_informazioni))
		{
			writeLogClient("[ERROR] Lock mutex_informazioni.", client_socket);
			raise(SIGUSR1);
		}
		GS->matrix[x][y] = 'f';
		if (pthread_mutex_unlock(&mutex_informazioni))
		{
			writeLogClient("[ERROR] Unlock mutex_informazioni.", client_socket);
			raise(SIGUSR1);
		}
	}
}

/* Invia al client le informazioni sui tesori trovati */
void sendInfo(int client_socket, int client_match, struct partita *elenco)
{
	char buffer[MAXLEN];
	struct partita *tmp = search(elenco, client_match);
	if (tmp)
	{
		sprintf(buffer, "\nId partita:\t%s%3d%s\nTesori totali:\t%s%3d%s\nTesori trovati:\t%s%3d%s\nTesori rimasti:\t%s%3d%s\nGiocatori:\t%s%3d%s\n",	
			MAGENTA, tmp->id, NORMAL, YELLOW, tmp->totali, NORMAL, GREEN, tmp->trovati, NORMAL, 
			YELLOW, tmp->totali -  tmp->trovati,  NORMAL, RED, tmp->players, NORMAL); /* 99 caratteri */
		if (write(client_socket, buffer, strlen(buffer)) < 1)
		{
			writeLogClient("[ERROR] Trasferimento informazioni.", client_socket);
			raise(SIGUSR1);
		}
		if (tmp->trovati < 1)
		{
			sprintf(buffer, "Nessun tesoro è stato ancora trovato!\n");
			if (write(client_socket, buffer, strlen(buffer)) < 1)
			{
				writeLogClient("[ERROR] Trasferimento informazioni.", client_socket);
				raise(SIGUSR1);
			}
		}
		else
		{
			int i;
			for(i = 0; i < tmp->trovati; i++)
			{
				sprintf(buffer, "Tesoro di coordinate (%s%2d%s;%s%2d%s) trovato dopo %s%3d%s second%c dall'utente (n.%s%d%s) '%s%s%s'.\n",
					CYAN, tmp->found->x[i], NORMAL, CYAN, tmp->found->y[i], NORMAL, GREEN, tmp->found->tempo[i], NORMAL, 
					tmp->found->tempo[i] == 1 ? 'o' : 'i', MAGENTA, tmp->found->identity[i], NORMAL, MAGENTA,   tmp->found->username[i], NORMAL);
				if (write(client_socket, buffer, strlen(buffer)) < 1)
				{
					writeLogClient("[ERROR] Trasferimento informazioni.", client_socket);
					raise(SIGUSR1);
				}
			}
		}
	}
	/* Terminazione stringa*/
	buffer[0] = '#';
	if (write (client_socket, buffer, sizeof(char)) < 1)
	{
		writeLogClient("[ERROR] Trasferimento informazioni.", client_socket);
		raise(SIGUSR1);
	}
	stampaListaBuffer(client_socket, utenti);
	return;
}

/* Stampa la lista degli utenti connessi al server */
void stampaListaBuffer(int client_socket, struct userlist *utenti)
{
	char buffer[MAXLENLOG];
	if (!utenti) {
		sprintf(buffer, "#");
		if (write(client_socket, buffer, strlen(buffer)) < 1)
		{
			writeLogClient("[ERROR] Trasferimento lista utenti.", client_socket);
			raise(SIGUSR1);
		}
	}
	else
	{
		if (utenti->next)
			sprintf(buffer, "%s ~ ", utenti->username);
		else
			sprintf(buffer, "%s", utenti->username);
		if (write(client_socket, buffer, strlen(buffer)) < 1)
		{
			writeLogClient("[ERROR] Trasferimento lista utenti.", client_socket);
			raise(SIGUSR1);
		}
		stampaListaBuffer(client_socket, utenti->next);
	}
}

/* Contiene una funzione di crittografia stabilita dal programmatore (è facilmente sostituibile) */
char *crittografia(char *password)
{
	return encryptpw(password);
}

/* Funzione di crittografia */
char *encryptpw(char *password) {
	int i;
	char *encrypted_pw;
	char salt[] = "$1$lsosmpll$";
	encrypted_pw = crypt(password, salt);
	for (i = 0; i < strlen(encrypted_pw); i++)
		encrypted_pw[i] = encrypted_pw[i + 12];
	encrypted_pw[i] = '\0';
	return encrypted_pw;
}

/* Thread dedicato interamente alla gestione del client */
void *clientManagement(void *client_socket)
{
	int nread, scelta_login, login_flag = 0, i, j, flag = 0, entry = -1, answer, tempo_restante, tempo_limite = TIMELIMIT, x, y, client_match = -1;
	char buffer = '?', password[MAXLENLOG - 1], userpass[MAXLEN], *encrypted_pw, nuova = '?', buffer_evento[LEN_EVENTO], vincitore[MAXLENLOG];
	char username[MAXLENLOG - 1] = {"\0"};
	struct partita *winner;
	do
	{
		/* Login o Signup */
		if ( read(*(int *)client_socket, &scelta_login, sizeof(int)) < 1)
			lastWish("[ERROR] Connessione interrotta durante il login.", x, y, client_match, username, *(int *)client_socket);
		/* USERNAME */
		if ((nread = read(*(int *)client_socket, userpass, MAXLEN)) < 1)
			lastWish("[ERROR] Connessione interrotta durante il login.", x, y, client_match, username, *(int *)client_socket);
		userpass[nread] = '\0';
		for (i = 0, j = 0; i < strlen(userpass); i++)
		{
			if (userpass[i] == ':') {
				username[i] = '\0';
				flag = 1;
				i++;
			}
			if (!flag)
				username[i] = userpass[i];
			else
				password[j++] = userpass[i];
		}
		password[j] = '\0';
		encrypted_pw = crittografia(password);
		flag = 0;
		login_flag = checkLogin(scelta_login, username, encrypted_pw, *(int *)client_socket);
		if (login_flag)
		{
			if (pthread_mutex_lock(&mutex_informazioni))
				lastWish("[ERROR] Lock informazioni.", x, y, client_match, username, *(int *)client_socket);
			utenti = insert(utenti, username, &login_flag);
			if (pthread_mutex_unlock(&mutex_informazioni))
				lastWish("[ERROR] Unlock informazioni.", x, y, client_match, username, *(int *)client_socket);
		}
		if (write(*(int *)client_socket, &login_flag, sizeof(int)) < 1)
			lastWish("[ERROR] Comunicazione col client.", x, y, client_match, username, *(int *)client_socket);
	} while (login_flag < 1);
	/* Gestisco la richiesta solo se il login avrà successo*/
	if (login_flag)
	{
		sprintf(buffer_evento, "[THREAD_CLIENT_%d] Client loggato col nome %s.", *((int *)client_socket) - 3, username);
		logEvent(buffer_evento, timeStamp());
		while (1)
		{
			if (DEBUG)
				stampaMatrice(GS->matrix, GS->height, GS->width, 0, 0, 0);
			/* Invio ID partita */
			if ( write(*(int *)client_socket, &ID_PARTITA, sizeof(int)) < 1 )
				lastWish("[ERROR] Invio ID al client.", x, y, client_match, username, *(int *)client_socket);
			/* Ricevo richiesta se si vuole affrontare la comunicazione della posizione iniziale */
			if ( read(*(int *)client_socket, &nuova, sizeof(char))  < 1 )
				lastWish("[ERROR] Richiesta invio della posizione iniziale.", x, y, client_match, username, *(int *)client_socket);
			/* Il client deve comunicare la nuova posizione */
			if (nuova == 'y')
			{
				/* Comunico al client le dimensioni del campo da gioco */
				if ( write(*(int *)client_socket, &GS->height, sizeof(int))  < 1 )
					lastWish("[ERROR] Invio altezza al client.", x, y, client_match, username, *(int *)client_socket);
				if ( write(*(int *)client_socket, &GS->width, sizeof(int))  < 1 )
					lastWish("[ERROR] Invio ampiezza al client.", x, y, client_match, username, *(int *)client_socket);
				/* Comunico al client il tempo limite */
				if ( write(*(int *)client_socket, &tempo_limite, sizeof(int))  < 1 )
					lastWish("[ERROR] Invio tempo limite al client.", x, y, client_match, username, *(int *)client_socket);
				if (client_match != -1 && client_match != ID_PARTITA)
				{
					winner = search(elenco, client_match);
					if (winner)
						sprintf(vincitore, "\nIl vincitore è #%s!\n#", winner->vincitore);
					else
						sprintf(vincitore, "#\n#");
					if ( write(*(int *)client_socket, vincitore, strlen(vincitore)) < 1 )
						lastWish("[ERRORE] Invio vincitore al client.", x, y, client_match, username, *(int *)client_socket);
				}
				entry = -1;
				/* Loop fino al ritrovamento di una posizione di partenza valida */
				while (entry < 0)
				{
					/* Il server riceve le coordinate dal client */
					if ( read(*(int *)client_socket, &x, sizeof(int))  < 1 )
						lastWish("[ERROR] Ricezione coordinate dal client.", x, y, client_match, username, *(int *)client_socket);
					if ( read(*(int *)client_socket, &y, sizeof(int))  < 1 )
						lastWish("[ERROR] Ricezione coordinate dal client.", x, y, client_match, username, *(int *)client_socket);
					/* Check posizione (x;y) sul campo da gioco */
					if (x < GS->height && y < GS->width)
					{
						if (GS->matrix[x][y] == 'o')		/*Ostacolo*/
							entry = -1;
						else if (GS->matrix[x][y] == 'p')	/*Giocatore avversario*/
							entry = -2;
						else if (GS->matrix[x][y] == 't') 	/*Tesoro (posizione lecita)*/
						{
							client_match = ID_PARTITA;
							newTreasure(search(elenco, client_match), x, y, username, *(int *)client_socket - 3,
							 ((int)time(NULL) - (int)tempo_partenza), *(int *)client_socket);
							entry = 2;
							treasuresReduction(client_match, *(int *)client_socket);
						}
						else {						/*Posizione libera*/
							client_match = ID_PARTITA;
							entry = 1;
						}
					}
					if ( write(*(int *)client_socket, &entry, sizeof(int))  < 1 )
						lastWish("[ERROR] Invio stato posizione iniziale al client.", x, y, client_match, username, *(int *)client_socket);
					if (entry > 0)
					{
						if ( write(*(int *)client_socket, &GS->totTreasures, sizeof(int))  < 1 )
							lastWish("[ERROR] Errore invio numero di tesori totali al client", x, y, client_match, username, *(int *)client_socket);
						/* La partita riceve un nuovo giocatore, incremento il numero di partecipanti */
						addPlayer(search(elenco, ID_PARTITA), *(int *)client_socket);
					}
				}
				nuova = '?';
			}
			/* Trasmissione di varie informazioni */
			if ( write(*(int *)client_socket, &GS->treasures, sizeof(int))  < 1 )
				lastWish("[ERROR] Invio numero di tesori al client.", x, y, client_match, username, *(int *)client_socket);
			tempo_restante = TIMELIMIT - ((int)time(NULL) - (int)tempo_partenza);
			if (tempo_restante < 0)	/*L'intervallo di aggiornamento avviene ogni secondo */
				tempo_restante = 0; /*quindi tempo_restante potrebbe assumere un valore negativo nel caso limite */
			if ( write(*(int *)client_socket, &tempo_restante, sizeof(int))  < 1 )
				lastWish("[ERROR] Errore invio tempo restante al client", x, y, client_match, username, *(int *)client_socket);
			/* Prima di proseguire, controllo che il client si muova nel mondo giusto */
			if ( read(*(int *)client_socket, &client_match, sizeof(int))  < 1 )
				lastWish("[ERROR] Ricezione id partita del client.", x, y, client_match, username, *(int *)client_socket);
			if ((entry == 1 || entry == 2) && client_match == ID_PARTITA) {
				if (pthread_mutex_lock(&mutex_partita))
					lastWish("[ERROR] Lock partita.", x, y, client_match, username, *(int *)client_socket);
				GS->matrix[x][y] = 'p';
				if (pthread_mutex_unlock(&mutex_partita))
					lastWish("[ERROR] Unlock partita.", x, y, client_match, username, *(int *)client_socket);
			}
			if ( write(*(int *)client_socket, &ID_PARTITA, sizeof(int))  < 1 )
				lastWish("[ERROR] Invio ID partita in corso al client.", x, y, client_match, username, *(int *)client_socket);
			if (ID_PARTITA != client_match) /* Il client crede di essere sulla vecchia (ed inesistente) matrice: */
			{
				/* Il giocatore in ogni caso abbandonerà la vecchia partita */
				if (client_match > -1)
					/* Prima di uscire riduco il numero di giocatori della partita del giocatore */
					playersReduction(client_match, username, 0, *(int *)client_socket);
				continue;		/* sia client che server ripartono da capo in modo da ripristinare l'integrità dei dati */
			}
			else
			{
				/* Ricevo il comando dal client */
				if (read(*(int *)client_socket, &buffer, sizeof(char)) < 1 )
					lastWish("[ERROR] Invio ID partita in corso al client.", x, y, client_match, username, *(int *)client_socket);
				else
				{
					/* Gestione del comando ricevuto */
					if (buffer == 'q')	/*Il client vuole disconnettersi */
						break;
					else if (buffer != 'i') /* Il client vuole compiere un movimento */
					{
						if (client_match != ID_PARTITA)
							answer = 0;
						else
						{
							if (pthread_mutex_lock(&mutex_partita))
								lastWish("[ERROR] Lock partita.", x, y, client_match, username, *(int *)client_socket);
							answer = move(GS->matrix, buffer, &x, &y);
							if (pthread_mutex_unlock(&mutex_partita))
								lastWish("[ERROR] Unlock partita.", x, y, client_match, username, *(int *)client_socket);
						}
					}
					else
						answer = 0;
					/* Se il client ha trovato un tesoro */
					if (answer == 2)
					{
						newTreasure(search(elenco, client_match), x, y, username, *(int *)client_socket - 3,
							((int)time(NULL) - (int)tempo_partenza), *(int *)client_socket);
						treasuresReduction(client_match, *(int *)client_socket);
					}
					/* Invio l'esito del movimento al client */
					if ( write(*(int *)client_socket, &answer, sizeof(int))  < 1 )
						lastWish("[ERROR] Invio esito del movimento al client.", x, y, client_match, username, *(int *)client_socket);
				  	if (buffer == 'i')
				  		sendInfo(*(int *)client_socket, client_match, elenco);
				}
			}
		}
	}
	lastWish("[END] Client disconnesso.", x, y, client_match, username, *(int *)client_socket);
	pthread_exit(0);
}

/* Controlla che le credenziali inviate dal client siano valide per il login/signup */
int checkLogin(int scelta_login, char *username, char *password, int client_socket)
{
	char buffer_user[MAXLENLOG], buffer_pwd[MAXLENLOG], str_out[MAXLENLOG * 2 + 2];
	int i, j, trovato_user = 0, trovato_pass = 0, ret = 0;
	ssize_t letti = 1;
	/* Lock del file per tutta la durata del login (username e pwd già inseriti, */
	/* non c'è il rischio che l'utente blocchi il file restando in idle) */
	if (pthread_mutex_lock(&mutex_credenziali))
	{
		writeLogClient("[ERROR] Lock mutex per le credenziali.", client_socket);
		raise(SIGUSR1);
	}
	/* Apro il file per il login */
	if ((file_login = open("credenziali.txt", O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) < 0)
	{
		writeLogClient("[ERROR] Apertura file delle credenziali.", client_socket);
		raise(SIGUSR1);
	}
	/* Il file è formattato in questo modo: " username:password " */
	do
	{
		/* Leggo l'username fino ai due punti */
		for(i = 0; i < MAXLENLOG-1; i++)
		{

			if ((letti = read(file_login, &buffer_user[i], sizeof(char))) < 1)
			{
				/* Se il file è vuoto i controlli successivi falliranno */
				if (letti == 0)
					break;
				writeLogClient("[ERROR] Lettura username da file delle credenziali.", client_socket);
				raise(SIGUSR1);
			}
			if (buffer_user[i] == ':')
				break;
		}
		buffer_user[i] = '\0';
		trovato_user = (strcmp(username, buffer_user) == 0);
		/* Leggo la password fino al '\n' */
		if (letti)
		{
			for(j = 0; j < MAXLEN-1; j++)
			{
				if ((letti = read(file_login, &buffer_pwd[j], sizeof(char) )) < 0)
				{
					writeLogClient("[ERROR] Lettura password da file delle credenziali.", client_socket);
					raise(SIGUSR1);
				}
				else if (!letti || buffer_pwd[j] == '\n')
					break;
			}
			buffer_pwd[j] = '\0';
			trovato_pass = (strcmp(password, buffer_pwd) == 0);
			/* Se l'utente vuole loggare ed ha inserito username e password corretti */
			if (scelta_login == 1 && trovato_pass && trovato_user)
			{
				ret = 1;
			}
		}
	} while (letti > 0 && !trovato_user); /*Esco se ho raggiunto la fine del file oppure ho raggiunto l'username (unico!) interessato */
	/* Se l'utente vuole registarsi e l'username inserito NON è già registrato */
	if (scelta_login == 2 && !trovato_user)
	{
		if ((letti = sprintf(str_out, "%s:%s\n", username, password)) < 0)
		{
			writeLogClient("[ERROR] Formattazione file delle credenziali.", client_socket);
			raise(SIGUSR1);
		}
		/* Inserisco la stringa "username:password" nel file delle credenziali */
		if (write(file_login, str_out, strlen(str_out)) < 0)
		{
			writeLogClient("[ERROR] Scrittura nel file delle credenziali.", client_socket);
			raise(SIGUSR1);
		}
		else	/* Nessun errore */
			ret = 1;
	}
	close(file_login);
	if (pthread_mutex_unlock(&mutex_credenziali))
	{
		writeLogClient("[ERROR] Unlock file delle credenziali.", client_socket);
		raise(SIGUSR1);
	}
	return ret;
}

/* Movimento sulla matrice */
int move(char **matrix, char buffer, int *x, int *y)
{
	int esito = 0, new_x = 0, new_y = 0;
	/* Calcolo le nuove coordinate */
	switch (buffer)
	{
		case 'n':
			new_x =  -1;
			break;
		case 's':
			new_x = 1;
			break;
		case 'e':
			new_y =  1;
			break;
		case 'o':
			new_y = -1;
			break;
	}
	/* Considero la nuova posizione */
	switch ( matrix[*x + new_x][*y + new_y] )
	{
		/* Cella libera: movimento permesso */
		case 'f':
			esito = 1;
			matrix[*x][*y] = 'f';
			*x += new_x;
			*y += new_y;
			matrix[*x][*y] = 'p';
			break;
		/* Cella con ostacolo: movimento NON permesso */
		case 'o':
			esito = -1;
			break;
		/* Cella con tesoro: movimento permesso */
		case 't':
			esito = 2;
			matrix[*x][*y] = 'f';
			*x += new_x;
			*y += new_y;
			matrix[*x][*y] = 'p';
			break;
		/* Cella con giocatore: movimento NON permesso */
		case 'p':
			esito = -2;
			break;
	}
	return esito;
}
