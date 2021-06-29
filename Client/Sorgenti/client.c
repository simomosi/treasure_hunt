#include "client_interface.h"

int main(int arg_num, char **arg) {
	int port, tempo_restante, tempo_limite;
	char ipAddress[MAX_IP], username[MAXLENLOG], informazioni, vincitore;
	struct sockaddr_in address;
	int current_match, real_match, x, y, entry = -1;
	int altezza, ampiezza, tesori_totali, tesori_rimasti, tesori_trovati;
	char scelta = '?', colore_tempo[10];
	char **matrice = NULL, comando = '?';
	/* Segnali */
	if(signal(SIGABRT, handler) == SIG_ERR)
	{
		perror("Errore impostazione handler per SIGABRT");
		exit(-1);
	}
	if(signal(SIGINT, handler) == SIG_ERR)
		errorExit("Errore impostazione handler per SIGINT");
	/* Impostazioni server */
	setup(ipAddress, &port, address);
	/* Login al server, salvo l'username per eventuale utilizzo */
	serverLog(username);
	/* PARTITA */
	current_match = -1;
	while(1)
	{
		/* Leggo ID Partita attuale */
		if (read(client_socket, &real_match, sizeof(int)) < 1)
			errorExit("Errore lettura ID partita");
		/* Verifico che il client stia nell'ID Partita corretta */
		if (current_match != real_match)
		{
			/* Libero la matrice se esiste già da partita precedente */
			if (matrice)
				freeMatrice(matrice, altezza);
			/* Imposto scelta = 'y' per ricevere nuove info di gioco */
			scelta = 'y';
			/* Comunico al server che il client deve aggionarsi */
			if (write(client_socket, &scelta, sizeof(char)) < 1)
				errorExit("Errore scrittura scelta");
			/* Acquisizione nuove dimensioni del campo da gioco */
			if (read(client_socket, &altezza, sizeof(int)) < 1)
				errorExit("Errore lettura altezza");
			if (read(client_socket, &ampiezza, sizeof(int)) < 1)
				errorExit("Errore lettura ampiezza");
			if (read(client_socket, &tempo_limite, sizeof(int)) < 1)
				errorExit("Errore lettura tempo limite");
			/* Reset del contatore di tesori trovati */
			tesori_trovati = 0;
			if (current_match != -1)	/* E' stata iniziata una nuova partita */
			{
				printf("\nPartita %s%d%s terminata.", RED, current_match, NORMAL);
				if (tesori_rimasti < 1)
					printf("\nTutti i tesori sono stati trovati!");
				else
					printf("\nIl tempo è scaduto!");

				while (read(client_socket, &vincitore, sizeof(char)) > 0 && vincitore != '#')
					printf("%c", vincitore);
				printf("%s", CYAN);
				while (read(client_socket, &vincitore, sizeof(char)) > 0 && vincitore != '#')
					printf("%c", vincitore);
				printf("%s", NORMAL);

				printf("\nVuoi joinare la partita %s%d%s ? (y/n) ", GREEN, real_match, NORMAL);
				scelta = acquireAnswer();
				entry = -1;
			}
			/* Aggiorno ID partita corrente */
			current_match = real_match;
			if (scelta == 'y') 	/* Comunico al server che devo entrare in gioco */
			{
				/* Allocazione matrice */
				matrice = allocaMatrice(matrice, altezza, ampiezza);
				/* Offuscamento mappa */
				offuscaMatrice(matrice, altezza, ampiezza);
				/* Entrata in gioco dell'utente */
				entry = parachute(matrice, &x, &y, &tesori_trovati, altezza, ampiezza);
				comando = '?';
				if (entry > 0)
					if (read(client_socket, &tesori_totali, sizeof(int)) < 1)
						errorExit("Errore lettura tesori totali");
			}
			else				/* Il client vuole abbandonare il gioco */
				break;
		}
		else 	/* Comunico al server che NON devo entrare in gioco */
		{
			scelta = 'n';
			if (write(client_socket, &scelta, sizeof(char)) < 1)
				errorExit("Errore scrittura scelta");
		}
		if (read(client_socket, &tesori_rimasti, sizeof(int)) < 1)
			errorExit("Errore lettura tesori rimasti");
		if (read(client_socket, &tempo_restante, sizeof(int)) < 1)
			errorExit("Errore lettura tempo restante");
		/* Colore del tempo */
		if (tempo_restante > tempo_limite/2)
			strcpy(colore_tempo, GREEN);
		else if (tempo_restante > tempo_limite/4)
			strcpy(colore_tempo, YELLOW);
		else
			strcpy(colore_tempo, RED);
		/* HUD */
		if (comando != 'i')
		{
			if (entry == 2)
				printf("\nHai trovato un %sTESORO%s!\n", YELLOW, NORMAL);
			else if (entry == -1)
				printf("\nTi sei scontrato con un %sOSTACOLO%s!\n", RED, NORMAL);
			else if (entry == -2)
				printf("\nTi sei scontrato con un %sGIOCATORE%s!\n", GREEN, NORMAL);
			else
				printf("\nSei su una cella vuota!\n");
			printf("\nID Partita: %s%d%s", MAGENTA, current_match, NORMAL);
			printf("\tNome giocatore: %s%s%s", CYAN, username, NORMAL);
			printf("\nTesori: %s%d%s/%s%d%s", YELLOW, tesori_rimasti, NORMAL, YELLOW, tesori_totali, NORMAL);
			printf("\tTrovati: %s%d%s", YELLOW, tesori_trovati, NORMAL);
			printf("\tTempo: %s%d%s", colore_tempo, tempo_restante, NORMAL);
				stampaMatrice(matrice, altezza, ampiezza, entry, x, y);
		}
		if (write(client_socket, &current_match, sizeof(int)) < 1)
			errorExit("Errore scrittura id corrente");
		/* Acquisisco e controllo l'input dell'utente */
		clientInput(&comando, altezza, ampiezza, x, y);
		clearTerm();
		if (read(client_socket, &real_match, sizeof(int)) < 1)
			errorExit("Errore lettura id reale");
		if (real_match == current_match)
		{
			if (write(client_socket, &comando, sizeof(char)) < 1)
				errorExit("Errore scrittura comando");
		}
		else
			continue;
		if (read(client_socket, &entry, sizeof(int)) < 1)
			errorExit("Errore lettura movimento");
		/* Se ha richiesto informazioni, resta in ascolto*/
		if (comando == 'i')
		{
			entry = 0;
			while( read(client_socket, &informazioni, sizeof(char)) > 0 && informazioni != '#')
				printf("%c", informazioni);
			printf("\n");
			printf("%sUtenti conessi al server%s: ", CYAN, NORMAL);
			while (read(client_socket, &informazioni, sizeof(char)) > 0 && informazioni != '#')
				printf("%c", informazioni);
			printf(".\n");
		}
		else if (entry != 0)	/* Il client ha scelto di muoversi */
		{
			update_matrix(matrice, &x, &y, comando, entry);
			if (entry == 2)
				tesori_trovati++;
		}
	}
	printf("\nClient disconnesso. Bye bye! \n");
	/* Gestione client */
	close(client_socket);
	exit(0);
}
