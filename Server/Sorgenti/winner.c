#include "winner.h"

/* Creazione del nuovo nodo userlist contenente l'username */
struct userlist *newNode(char *username)
{
	struct userlist *tmp = (struct userlist *)malloc(sizeof(struct userlist));
	tmp->username = (char *)malloc(strlen(username) * sizeof(char));
	strcpy(tmp->username, username);
	tmp->next = NULL;
	tmp->prev = NULL;
	return tmp;
}

/* Inserimento ordinato del nodo userlist */
struct userlist *insert(struct userlist *utenti, char *username, int *entry)
{
	struct userlist *tmp = utenti;
	if (!utenti)
		tmp = newNode(username);
	else if (!strcmp(username, utenti->username))
		*entry = -1;
	else if (strcmp(username, utenti->username) < 0)
	{
		tmp = newNode(username);
		tmp->prev = utenti->prev;
		tmp->next = utenti;
		utenti->prev = tmp;
	}
	else
		utenti->next = insert(utenti->next, username, entry);
	return tmp;
}

/* Cancellazione nodo userlist */
struct userlist *deleteNode(struct userlist *utenti, char *username)
{
	struct userlist *tmp = NULL;
	if (utenti)
	{
		if (!strcmp(utenti->username, username))
		{
			tmp = utenti;
			if (utenti->next)
				utenti->next->prev = utenti->prev;
			if (utenti->prev)
				utenti->prev->next = utenti->next;
			utenti = utenti->next;
			free(tmp->username);
			free(tmp);
		}
		else if (strcmp(utenti->username, username) < 0)
			utenti->next = deleteNode(utenti->next, username);
	}
	return utenti;
}

/* Cancellazione dell'intera lista userlist */
struct userlist *deleteList(struct userlist *utenti) {
	if (utenti) {
		deleteList(utenti->next);
		utenti->next = NULL;
		free(utenti->username);
		free(utenti);
	}
	return NULL;
}

/* Creazione del nuovo nodo contenente informazioni sulla partita */
struct partita *newGame(int id, int num_tesori)
{
	struct partita *nuova = (struct partita *)malloc(sizeof(struct partita));
	int i;
	nuova->id = id;
	nuova->players = 0;
	nuova->totPlayers = 0;
	nuova->totali = num_tesori;
	nuova->trovati = 0;
	nuova->vincitore = (char *)malloc(sizeof(char));
	nuova->found = (struct tesori *)malloc(sizeof(struct tesori));
	nuova->found->x = (int *)calloc(num_tesori, sizeof(int));
	nuova->found->y = (int *)calloc(num_tesori, sizeof(int));
	nuova->found->tempo = (int *)calloc(num_tesori, sizeof(int));
	nuova->found->identity = (int *)calloc(num_tesori, sizeof(int));
	nuova->found->username = (char **)malloc(num_tesori * sizeof(char *));
	for(i = 0; i < num_tesori; i++)
		nuova->found->username[i] = (char *)malloc((MAXLENLOG-1) * sizeof(char));
	nuova->next = NULL;
	nuova->prev = NULL;
	return nuova;
}

/* Inserimento in lista del nodo partita */ 
struct partita *appendGame(struct partita *current, struct partita *nuova)
{
	if (current)
	{
		current->next = appendGame(current->next, nuova);
		current->next->prev = current;
	}
	else
		current = nuova;
	return current;
}

/* Cancellazione del nodo partita */
struct partita *deleteGame(struct partita *current)
{
	struct partita *tmp = current;
	int i;
	if (current)
	{
		free(current->found->x);
		free(current->found->y);
		free(current->found->identity);
		free(current->found->tempo);
		for(i = 0; i < current->totali; i++)
			free(current->found->username[i]);
		free(current->found->username);
		free(current->found);
		free(current->vincitore);
		tmp = current->next;
		if (tmp)
			tmp->prev = current->prev;
		if (current->prev)
			current->prev->next = tmp;
		free(current);
	}
	return tmp;
}

/* Ricerca del nodo partita tramite ID */
struct partita *search(struct partita *current, int id)
{
	if (id == -1)
		return NULL;
	if (current)
	{
		if (current->id == id)
			return current;
		return search(current->next, id);
	}
	return current;
}

/* Ritorna il numero di giocatori di una partita */
int getPlayer(struct partita *current)
{
	if (current)
		return (current->players);
	return -1;
}

/* Incrementa il numero di giocatori di una partita */
int addPlayer(struct partita *current, int client_socket)
{
	if (current)
	{
		if (pthread_mutex_lock(&mutex_informazioni))
		{
			writeLogClient("[ERROR] Lock informazioni_partita.", client_socket);
			raise(SIGABRT);
		}
		current->totPlayers++;
		current->players++;
		if (pthread_mutex_unlock(&mutex_informazioni))
		{
			writeLogClient("[ERROR] Unlock informazioni_partita.", client_socket);
			raise(SIGABRT);
		}
		return (current->players);
	}
	return -1;
}

/* Riduce il numero di giocatori di una partita */
int redPlayer(struct partita *current)
{
	if (current)
		return --current->players;
	return -1;
}

/* Registra la scoperta di un tesoro */
int newTreasure(struct partita *current, int x, int y, char *username, int identity, int tempo, int client_socket)
{
	if (current)
	{
		if (pthread_mutex_lock(&mutex_informazioni))
		{
			writeLogClient("[ERROR] Lock informazioni_partita.", client_socket);
			raise(SIGABRT);
		}
		current->found->x[current->trovati] = x;
		current->found->y[current->trovati] = y;
		strcpy(current->found->username[current->trovati], username);
		current->found->identity[current->trovati] = identity;
		current->found->tempo[current->trovati] = tempo;
		if (pthread_mutex_unlock(&mutex_informazioni))
		{
			writeLogClient("[ERROR] Unlock informazioni_partita.", client_socket);
			raise(SIGABRT);
		}
		return ++(current->trovati);
	}
	return -1;
}

/* Elimina tutte le partite terminate con nessun giocatore rimasto */
struct partita * garbageCollector(struct partita *current)
{
	struct partita *tmp = current;
	if (current)
	{
		current->next = garbageCollector(current->next);
		if (current->next)
			current->next->prev = current;
		if (current->players < 1)
			tmp = deleteGame(current);
	}
	return tmp;
}

/* Elimina tutte le partite */
struct partita * garbageDestroyer(struct partita *current)
{
	struct partita *tmp = current;
	if (current)
	{
		current->next = garbageCollector(current->next);
		if (current->next)
			current->next->prev = current;
		tmp = deleteGame(current);
	}
	return tmp;
}

/* Ricerca del vincitore della partita */
void getWinner(struct partita *current)
{
	if (current)
	{
		int i, temp, max = 0, index = 0, *tesori_trovati = (int *)calloc(current->totPlayers + 1, sizeof(int));
		/* Scorro l'array con l'id dei giocatori che hanno trovato i tesori */
		for(i = 0; i < current->trovati; i++)
		{
			temp = current->found->identity[i]; /* identità del trovatore del tesoro */
			tesori_trovati[temp]++;	/* l'id parte da 1, in questo modo li posiziono nell'array */
			if (tesori_trovati[temp] > max) /* Salvo di volta in volta il presunto vincitore */
			{
				max = tesori_trovati[temp];
				index = i;
			}
		}
		if (max > 0)
			strcpy(current->vincitore, current->found->username[index]);
		else /* Se nessun tesoro è stato trovato */
			strcpy(current->vincitore, "Nessuno");
		free(tesori_trovati);
	}
}

/* Errore nel thread: formattazione stringa e scrittura nel file di log del server */
void writeLogClient(char *testo, int client_socket)
{
	char buffer_evento[MAXLEN];
	sprintf(buffer_evento, "[THREAD_CLIENT_%d] %s\n[THREAD_CLIENT_%d][END] Terminazione client.", client_socket - 3, testo, client_socket - 3);
	logEvent(buffer_evento, timeStamp());
	return;
}

/* Errore nel server: formattazione stringa e scrittura nel file di log del server */
void writeLogServer(char *testo)
{
	char buffer_evento[MAXLEN];
	sprintf(buffer_evento, "[SERVER] %s\n[SERVER][END] Terminazione server.", testo);
	logEvent(buffer_evento, timeStamp());
	perror(buffer_evento);
	raise(SIGABRT);
	return;
}

/* Scrittura nel file di log del server */
void logEvent(char * stringa, char * timeStamp)
{
	char buffer[MAXLEN];
	pthread_mutex_lock(&mutex_log);
	/* Apertura file di log */
	if ((log_file = open("log.txt", O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) < 0)
	{
		perror("Errore apertura file di log");
		raise(SIGABRT);
	}
	sprintf(buffer, "~ %s\n%s\n\n", timeStamp, stringa);
	write(log_file, buffer, strlen(buffer));
	close(log_file);
	pthread_mutex_unlock(&mutex_log);
	free(timeStamp);
	return;
}

/* Creazione stringa con informazioni sulla data */
char *timeStamp()
{
	char *buffer_tempo = (char *)malloc((TIMESIZE+1)*sizeof(char)); /* Array di destinazione, deallocato in logEvent */
	struct tm *struttura_tempo;		/* Struttura contenente i valori 'spezzati' del timestamp */
	time_t tempo;	/* Dati del tempo corrente */
	char buffer_format[] = "%d/%m/%Y - %X"; /* Formato del timestamp da inserire in strftime*/
	tempo = time(NULL);	/* Acquisizione numero di secondi dall'epoch time (1/1/1970)*/
	struttura_tempo = localtime(&tempo); /* Trasformazione da secondi a data leggibile */
	strftime(buffer_tempo, (size_t) TIMESIZE+1, buffer_format, struttura_tempo); /*Formattazione stringa*/
	return buffer_tempo;
}
