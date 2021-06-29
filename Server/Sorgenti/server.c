#include "manager.h"

int ID_PARTITA = 1; /* Id iniziale */

void handler(int signum);

int main(int arg_num, char **arg) {
	char client_name[MAXLEN], buffer_evento[LEN_EVENTO];
	int port = DEFAULT_PORT;
	pthread_t tid;
	struct sockaddr_in address;
	struct sockaddr_in client_address;
	struct sockaddr_in *client_addr;
	socklen_t client_address_len = sizeof(client_address);
	int on = 1;
	int client_socket;
	logEvent("[SERVER] [START] Tentativo di avvio.", timeStamp());
	/* SEGNALI */
	if(signal(SIGABRT, handler) == SIG_ERR)
	{
		perror("[SERVER] [ERROR] Impostazione handler per SIGABRT.");
		logEvent("[SERVER] [ERROR] Impostazione handler per SIGABRT.", timeStamp());
		exit(-1);
	}
	if(signal(SIGINT, handler) == SIG_ERR)
		writeLogServer("[ERROR] Impostazione handler per SIGINT.");
	if(signal(SIGUSR1, handler) == SIG_ERR)
		writeLogServer("[ERROR] Impostazione handler per SIGUSR1.");
	/* Controllo il numero di parametri */
	if (arg_num > 2)
	{
		strcpy(buffer_evento, "[SERVER] [ERROR] Numero parametri errato.");
		write(STDERR_FILENO, buffer_evento, strlen(buffer_evento));
		logEvent(buffer_evento, timeStamp());
		raise(SIGABRT);
	}
	else if (arg_num == 2)
	{
		/* Casting della porta da stringa a valore numerico */
		if ((port = atoi(arg[1])) == 0) {
			strcpy(buffer_evento, "[SERVER] [ERROR] Porta non valida.");
			write(STDERR_FILENO, buffer_evento, strlen(buffer_evento));
			logEvent(buffer_evento, timeStamp());
			raise(SIGABRT);
		}
		if (port < MIN_PORT)
		{
			strcpy(buffer_evento, "[SERVER] [ERROR] Porta riservata al sistema.");
			write(STDERR_FILENO, buffer_evento, strlen(buffer_evento));
			logEvent(buffer_evento, timeStamp());
			raise(SIGABRT);
		}
	}
	/* Impostazioni indirizzo */
	address.sin_family = AF_INET;/* Progetto */
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&(address.sin_zero), '\0', 8);
	/* Inizializzazione socket */
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		writeLogServer("[ERROR] Creazione socket.");
	/*Inizializzazione server*/
	if (setsockopt (server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) == -1)
		writeLogServer("[ERROR] Set socket options.");
	if ((bind(server_socket, (struct sockaddr *)&address, sizeof(address))) == -1)
		writeLogServer("[ERROR] Bind socket.");
	if ((listen(server_socket, QUEUE_LENGTH)) == -1)
		writeLogServer("[ERROR] Listen socket.");
	/* CREAZIONE MUTEX */
	if (pthread_mutex_init(&mutex_partita, NULL))
		writeLogServer("[ERROR] Creazione del mutex_partita.");
	if (pthread_mutex_init(&mutex_credenziali, NULL))
		writeLogServer("[ERROR] Creazione del mutex_credenziali.");
	if (pthread_mutex_init(&mutex_log, NULL))
		writeLogServer("[ERROR] Creazione del mutex_log.");
	if (pthread_mutex_init(&mutex_informazioni, NULL))
		writeLogServer("[ERROR] Creazione del mutex_informazioni.");
	/* THREAD CHE GESTISCE LA PARTITA */
	GS = (struct settings *)malloc(sizeof(struct settings));
	if (pthread_create(&tid, NULL, initializer, (void *)GS) != 0)
		writeLogServer("[ERROR] Creazione del thread initializer.");
	sprintf(buffer_evento, "[SERVER] [START] Server avviato con successo sulla porta %d.", port);
	logEvent(buffer_evento, timeStamp());
	sprintf(buffer_evento, "[SERVER] [START] Server in attesa di connessioni.");
	logEvent(buffer_evento, timeStamp());

	/* LOOP CHE ACCETTA LE CHIAMATE */
	while (1) {
		/* Inizializzo thread client */
		int *thread_connection = (int *)malloc(sizeof(int));
		if ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len)) == -1)
			writeLogServer("[ERROR] Accept client.");
		client_addr = (struct sockaddr_in *)&client_address;	/* Salvo l'indirizzo in client_addr */
		inet_ntop(AF_INET, &(client_addr->sin_addr), client_name, sizeof(client_name));		/* Converto in stringa */
		*thread_connection = client_socket;
		sprintf(buffer_evento, "[SERVER] [CONNECTION] Effettuata connessione col client n. %d di indirizzo %s.", *((int *) thread_connection) - 3, client_name);
		logEvent(buffer_evento, timeStamp());
		if (pthread_create(&tid, NULL, clientManagement, (void *)thread_connection) != 0)
			writeLogServer("[ERROR] Creazione del thread manager.");
	}
	return 0;
}

void handler(int signum)
{
	char buffer_evento[MAXLEN];
	switch(signum)
	{
		case SIGINT:
			sprintf(buffer_evento, "\n[SERVER] [END] Ricezione del segnale SIGINT.\n[SERVER] [END] Terminazione del server.");
			break;
		case SIGABRT:
			sprintf(buffer_evento, "\n[SERVER] [END] Ricezione del segnale SIGABRT.\n[SERVER] [END] Terminazione del server.");
			break;
		case SIGUSR1:
			fflush(stdout);
			/*Rilascio i mutex eventualmente bloccati dal thread */
			/* per consentire al server di continuare il suo lavoro */
			pthread_mutex_unlock(&mutex_partita);
			pthread_mutex_unlock(&mutex_credenziali);
			pthread_mutex_unlock(&mutex_log);
			pthread_mutex_unlock(&mutex_informazioni);
			pthread_exit(0);
			return;
		default:
			sprintf(buffer_evento, "\n[SERVER] [END] Ricezione del segnale di errore %d.\n[SERVER] [END] Terminazione del server.", signum);
			break;
	}
	/* Deallocazione delle strutture contenenti le informazioni */
	utenti = deleteList(utenti);
	/* Chiusura socket server */
	close(server_socket);
	/* Distruzione mutex */
	pthread_mutex_destroy(&mutex_partita);
	pthread_mutex_destroy(&mutex_credenziali);
	pthread_mutex_destroy(&mutex_log);
	pthread_mutex_destroy(&mutex_informazioni);
	/* Chiusura thread per la gestione della matrice: anch'esso aiuterà con le deallocazioni */
	ID_PARTITA = -1;
	/* Chiusura file di log */
	close(log_file);
	logEvent(buffer_evento, timeStamp());
	write(STDERR_FILENO, buffer_evento, strlen(buffer_evento));
	/* Messaggio in output su standard error del server */
	sprintf(buffer_evento, "\n[SERVER] [END] Controlla il file di log alla data \n'%s%s%s' per ulteriori informazioni.\n\n", CYAN, timeStamp(), NORMAL);
	write(STDERR_FILENO, buffer_evento, strlen(buffer_evento));
	exit(0);
}
