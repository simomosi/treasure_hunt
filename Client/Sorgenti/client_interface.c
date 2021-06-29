#include "client_interface.h"

/* Acquisisce un carattere tra {y, Y, n, N} */
char acquireAnswer(){
	char res='?';
	while(scanf(" %c%*[^\n]%*c",&res)<1 || (res != 'n' && res != 'N' && res != 'y' && res != 'Y'))	/*Acquisisce e pulisce il buffer*/
    	printf("%sErrore%s! Inserisci un carattere in {y,Y,n,N}: ", RED, NORMAL);
	return res;
}

/* Acquisisce un intero tra [min, max] */
int acquireRangedIntegerIncl(int max, int min)
{
	int integer, tmp;
	if(min>max) {
    	tmp=min;
    	min=max;
    	max=tmp;
  	}
  	while(scanf("%d",&integer)<1 || integer<min || integer>max) {
    	scanf("%*[^\n]%*c");
    	printf("%sErrore%s! Inserisci un intero in [%d,%d]: ", RED, NORMAL, min, max);
  	}
  	scanf("%*[^\n]%*c");	/*Pulisce il buffer*/
  	return integer;
}

/* Aquisisce una stringa e pulisce il buffer */
char * acquireString(char * str)
{
	scanf(" %15[^\n]%*c",str);
	if (feof(stdin))	/* In caso di input di stringa > 15 char */
	{
		printf("\n%sWarning%s: la stringa inserita verra' troncata perche' contiene piu' di 15 caratteri.\n", YELLOW, NORMAL);
		scanf("%*[^\n]%*c");
	}
	return str;
}

/* Handler segnali */
void handler(int signum)
{
	switch(signum)
	{
		case SIGINT:
	      	printf("\n[CLIENT] [END] Ricezione segnale %sSIGINT%s.", RED, NORMAL);
	      	break;
     	case SIGABRT:
			printf("\n[CLIENT] [END] Ricezione segnale  %sSIGABRT%s.", RED, NORMAL);
			break;
		default:
      		printf("\n[CLIENT] Ricezione segnale di errore %s%d%s.", RED, signum, NORMAL);
      		break;
  	}
  	printf("\nBye bye!\n");
	close(client_socket);
	exit(-1);
}

/* Stampa su stderr e invia un segnale SIGABRT */
void errorExit(char *error)
{
	perror(error);
	raise(SIGABRT);
}

/* Gestisce le impostazioni per la connessione al server */
void setup(char *ipAddress, int *port, struct sockaddr_in address)
{
	char errore[512];
	printf("%sUn server di indirizzo 195.43.185.138:8004 sarà attivo 24h/7.%s\n", GREY, NORMAL);
	printf("Inserisci l'%sindirizzo%s del server: ", CYAN, NORMAL);
	acquireString(ipAddress);
	printf("Inserisci la %sporta%s a cui connettersi: ", CYAN, NORMAL);
	*port = acquireRangedIntegerIncl(1024, 20000);
	fflush(stdout);
	address.sin_family = AF_INET;
	address.sin_port   = htons(*port);
	inet_aton(ipAddress, &address.sin_addr); /* Da stringa ad indirizzo numerico */
	printf("Tentativo di connessione al server %s%s%s sulla porta %s%d%s.\n", YELLOW, ipAddress, NORMAL, YELLOW, *port, NORMAL);
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(client_socket, (struct sockaddr *)&address, sizeof(address)) == -1)
	{
		sprintf(errore, "Errore di connessione al server %s%s%s sulla porta %s%d%s", RED, ipAddress, NORMAL, RED, *port, NORMAL);
		errorExit(errore);
	}
	printf("\nStabilita connessione al server %s%s%s sulla porta %s%d%s.\n", GREEN, ipAddress, NORMAL, GREEN, *port, NORMAL);
}

/* Gestisce l'accesso al gioco (login/signup) */
void serverLog(char *username)
{
	int scelta_log, illegal_char, exists, flag = 1;
	char password[MAXLENLOG], checkpw[MAXLENLOG], coloreLogin[10];
	do {
		printf("\n1. %sLogin%s", GREEN, NORMAL);
		printf("\n2. %sSign Up%s", CYAN, NORMAL);
		printf("\n3. %sExit%s", RED, NORMAL);
		printf("\n  > ");
		scelta_log = acquireRangedIntegerIncl(1, 3);
		switch (scelta_log)
		{
			case 1:
			case 2:
				/* Acquisisco informazione di login o signup */
            	if (scelta_log % 2)
            	{
	            	strcpy(coloreLogin, GREEN);
   				    printf("\n* *%s %s %s* *\n\n", GREEN, "LOGIN", NORMAL);
            	}
            	else
				{
               		strcpy(coloreLogin, CYAN);
               		printf("\n* *%s %s %s* *\n\n", CYAN, "SIGNUP", NORMAL);
            	}
				printf("Inserisci l'%susername%s (max %d caratteri):\n  > ", coloreLogin, NORMAL, MAXLENLOG-1);
				acquireString(username);
				username[strlen(username)] = '\0';
				/* Acquisisco password in maniera nascosta */
				set_input_mode();
				flag = 1;
				while (flag) {
					printf("Inserisci la %spassword%s (max %d caratteri):\n  > ", coloreLogin, NORMAL, MAXLENLOG-2);
					acquirePassword(password);
					printf("\nConferma  la %spassword%s:\n  > ", coloreLogin, NORMAL);
					acquirePassword(checkpw);
					if (strcmp(password, checkpw))
						printf("\nLe %spassword%s non combaciano!\n", RED, NORMAL);
					else
						flag = 0;
				}
				reset_input_mode();
				/* Controllo le stringhe per caratteri illegali */
           		illegal_char = illegalCheck(username, password);
				if (illegal_char)
				{
					printf("%sAttenzione%s! Inserito carattere non valido.\nE' possibile inserire solo caratteri alfanumerici.\n", RED, NORMAL);
					break;
				}
				/* Invio dati al server */
				exists = infoCheck(username, password, scelta_log);
				break;
			case 3:
				printf("\n[CLIENT] Chiusura socket client.\n");
				close(client_socket);
				exit(1);
			default:
				printf("Scelta non valida! Riprova.\n\n");
		}
	} while (scelta_log < 1 || scelta_log > 3 || illegal_char || exists < 1);
}

/* Acquisizione della password nascosta a video */
void acquirePassword(char *password) {
	int check, i = 0;
	do {
		check = i;
		password[i] = getchar();
		if (password[i] == '\n') {
			password[i] = '\0';
			break;
		}
		else if (password[i] == 0x7f) { /* Backspace, per cancellare l'input appena inserito */
			if (check > 0)
				printf("\b \033[D");		/* Cancella l'asterisco e muove il cursore a sinistra */
			if (i > 0)
				password[i--] = '\0';
		}
		else if (i == 14 && password[i] != '\n') {	/* Limite di caratteri superato */
			printf("\nHai superato la soglia di %s%d%s caratteri! Riprova.\n", RED, MAXLENLOG-2, NORMAL);
			for (i = 0; i < 15; i++)	/* Reset della password */
				password[i] = '\0';
			i = 0;
			printf("  > ");
		}
		else {	/*Inserisce degli asterischi per ogni carattere letto */
			putchar('*');
			i++;
		}
	} while (i < 15);
	password[i] = '\0';
}

/* Controlla che nella password non ci siano caratteri NON alfanumerici */
int illegalCheck(char *username, char *password)
{
	int illegal_char = 0, i;
	/* Controllo su username */
	for(i = 0; i < strlen(username); i++)
	{
		if ( username[i] < '0' || ( username[i] > '9' && username[i] < 'A') || (username[i] > 'Z' && username[i] < 'a') || username[i] > 'z' )
		{
			illegal_char = 1;
			break;
		}
	}
	if (!illegal_char)
		/* Controllo su password */
		for(i = 0; i < strlen(password); i++)
		{
			if ( password[i] < '0' || ( password[i] > '9' && password[i] < 'A') || (password[i] > 'Z' && password[i] < 'a') || password[i] > 'z' )
			{
				illegal_char = 1;
				break;
			}
		}
	return illegal_char;
}

/* Controlla che i dati di accesso inseriti siano validi */
int infoCheck(char *username, char *password, int scelta_log)
{
	char userpass[MAXLENLOG * 2 + 1];
	int exists, length;
	sprintf(userpass, "%s:%s", username, password);
	length = strlen(userpass);
	if (write(client_socket, &scelta_log, sizeof(int)) < 1)
		errorExit("Errore durante il login");
	if (write(client_socket, userpass, length) < 1)
		errorExit("Errore durante il login");
	printf("\n\nAttendo validazione dal server...\n");
	if (read(client_socket, &exists, sizeof(int)) < 0)
		errorExit("Errore di comunicazione con server");
	if (exists == 0)
	{
		if (scelta_log == 1)
			printf("Username e/o password errati.\n");
		else
	    	printf("Username già esistente.\n");
	   	printf("Riprova!\n");
	}
	else if (exists == -1)
		printf("Errore! L'utente con tale username è già loggato.\n");
   else {
    	printf("Credenziali confermate!\n");
	  	if (scelta_log == 1)
	    	printf("Login effettuato con successo.\n");
	   	else
	    	printf("Sign up effettuato con successo.\n");
	  	printf("Log col server effettuato. Buon divertimento!\n");
	}
	return exists;
}

/* Funzioni gestione matrice */
void freeMatrice(char **matrice, int altezza)
{
	int i;
	for (i = 0; i < altezza; i++)
		free(matrice[i]);
	free(matrice);
}

char **allocaMatrice(char **matrice, int altezza, int ampiezza)
{
	int i;
	matrice = (char **)malloc(altezza * sizeof(char *));
	for (i = 0; i < altezza; i++)
		matrice[i] = (char *)malloc(ampiezza * sizeof(char));
	return matrice;
}

/* Inizializza la matrice: scrive il carattere '#' */
/* in tutte le celle non ancora visitate dall'utente */
void offuscaMatrice(char **matrice, int altezza, int ampiezza)
{
	int i, j;
	for (i = 0; i < altezza; i++)
		for (j = 0; j < ampiezza; j++)
			matrice[i][j] = '#';
}

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
				case '#':
					printf(" %s%2c%s", GREY, matrice[x][y], NORMAL);
					break;
				case '@':
					if (entry == 2 && x == player_x && y == player_y)
						printf(" %s%2c%s", YELLOW, matrice[x][y], NORMAL);
					else if (x == player_x && y == player_y)
						printf(" %s%2c%s", CYAN, matrice[x][y], NORMAL);
					else
						printf(" %s%2c%s", MAGENTA, matrice[x][y], NORMAL);
					break;
				case 'X':
					printf(" %s%2c%s", RED, matrice[x][y], NORMAL);
					break;
				default:
					printf(" %2c", matrice[x][y]);
					break;
			}
		}
		printf("\n   ");
	}
}

/* Aggiorna la matrice modificando l'ultima cella visitata dal giocatore */
void update_matrix(char **matrice, int *x, int *y, char comando, int entry)
{
	char nuovoSimbolo;
	int new_x = 0, new_y = 0;
	switch (comando)
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
	}
	if (entry > 0)
	{
		matrice[*x][*y] = ' ';
		*x += new_x;
		*y += new_y;
		matrice[*x][*y] = '@';
	}
	else
	{
		if (entry == -1)
			nuovoSimbolo = 'X';
		else
			nuovoSimbolo = '@';
		matrice[*x + new_x][*y + new_y] = nuovoSimbolo;
	}
}

/* Clear del terminale senza chiamate di sistema */
void clearTerm()
{
	printf("\033[2J");
}

/* Entrata in gioco del giocatore */
int parachute(char **matrice, int *x, int *y, int *tesori_trovati, int altezza, int ampiezza)
{
	int entry = -1;
	while (entry < 0)
	{
		printf("\nInserisci la coordinata %sx%s per l'altezza (da 1 a %d): ", CYAN, NORMAL, altezza);
		*x = acquireRangedIntegerIncl(1, altezza);
		printf("\nInserisci la coordinata %sy%s per l'ampiezza (da 1 a %d): ", CYAN, NORMAL, ampiezza);
		*y = acquireRangedIntegerIncl(1, ampiezza);
		(*x)--;	/* La matrice parte da 0 */
		(*y)--;	/* La matrice parte da 0 */
		if (write(client_socket, x, sizeof(int)) < 1)
			errorExit("Errore scrittura coordinata x");
		if (write(client_socket, y, sizeof(int)) < 1)
			errorExit("Errore scrittura coordinata y");
		if (read(client_socket, &entry, sizeof(int)) < 1)
			errorExit("Errore lettura spostamento");
		if (entry == -1)
			printf("\nErrore! Sei caduto su un %sostacolo%s! Riprova.\n", RED, NORMAL);
		else if (entry == -2)
			printf("\nErrore! Sei caduto su un %sgiocatore%s! Riprova.\n", GREEN, NORMAL);
		else if (entry == 2)
		{
			matrice[*x][*y] = '@';
			(*tesori_trovati)++;
		}
		else
		{
			printf("Sei in gioco!\n");
			matrice[*x][*y] = '@';
		}
	}
	if (!feof(stdin))
		scanf("%*c");
	return entry;
}

/* Ripristina l'input dello schermo rendendolo nuovamente visibile */
void reset_input_mode()
{
	/* Reset variabili terminale */
	tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

/* Nasconde l'input dallo schermo */
void set_input_mode()
{
	struct termios tattr;
	/* Salvo variabile del terminale per resettare dopo */
	tcgetattr (STDIN_FILENO, &saved_attributes);
	/* Assicuro il reset all'uscita del programma */
	atexit (reset_input_mode);
	/* Setto le variabili del terminale */
	tcgetattr (STDIN_FILENO, &tattr);
	/* Disabilito ICANON (lettura canonica) e ECHO (echo caratteri in stdout) */
	tattr.c_lflag &= ~(ICANON|ECHO);
	/* Caratteri minimi per lettura non canonica */
	tattr.c_cc[VMIN] = 1;
	/* Tempo di timeout per lettura non canonica */
	tattr.c_cc[VTIME] = 0;
	/* Flush del stdin e setto terminale con attributi di tattr */
	tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}

/* Gestisce l'acquisizione dell'input del giocatore */
void clientInput(char *comando, int altezza, int ampiezza, int x, int y)
{
	int legit = 1, flag = 1;
	int tmp1 = '?', tmp2 = '?', tmp3 = '?';
	set_input_mode();
	do {
		if (flag)
			printf("\nInserisci un comando ('%sh%s' per maggiori informazioni): ", GREEN, NORMAL);
		legit = 0;
  		tmp1 = getchar();
  		if (tmp1 == 27) {	/* Ctrl/Alt + W/A/S/D */
  			tmp2 = getchar();
  			if (tmp2 == 'w')
  				tmp1 = 'n';
  			else if (tmp2 == 's')
  				tmp1 = 's';
  			else if (tmp2 == 'd')
  				tmp1 = 'e';
  			else if (tmp2 == 'a')
  				tmp1 = 'o';
  			else
  				tmp3 = getchar();
  		}
  		if (tmp1 == 27 && tmp2 == 91) {	/* Movimento con le frecce direzionali */
  			switch (tmp3)
			{
				case 65:
					tmp1 = 'n';
					break;
				case 66:
			   		tmp1 = 's';
			   		break;
				case 67:
			   		tmp1 = 'e';
			   		break;
				case 68:
			   		tmp1 = 'o';
			   		break;
			   case '1': /* Ctrl/Alt/Shift + frecce direzionali */
			   	getchar();
			   	getchar();
			   	tmp3 = getchar();
			   	if (tmp3 == 65)
			   		tmp1 = 'n';
			   	else if (tmp3 == 66)
			   		tmp1 = 's';
			   	else if (tmp3 == 67)
			   		tmp1 = 'e';
			   	else if (tmp3 == 68)
			   		tmp1 = 'o';
			   	break;
  			}
  		}
  		*comando = analyze(tmp1);
        switch (*comando)
		{
			case 'n':
			case 's':
			case 'e':
			case 'o':
				legit = legitPosition(*comando, altezza, ampiezza, x, y);
				flag = 0;
				break;
			case 'h':
				clearTerm();
				stampaHelp();
				flag = 1;
				break;
			case 'i':
				legit = 1;
				break;
			case 'q':
				legit = 1;
				break;
			default:
				if (flag)	/* Stampa il messaggio di errore una volta soltanto */
					printf("\nErrore! Il comando inserito è errato!\n");
				flag = 0;
				break;
		}
	} while (!legit);
    reset_input_mode();
}

/* Trasforma i comandi con lo stesso esito in un unico comando da inviare al server */
char analyze(char client_msg)
{
	char comando = '?';
	switch(client_msg) {
		case 'n':
		case 'w':
		case 'W':
			comando = 'n';
			break;
		case 's':
		case 'S':
			comando = 's';
			break;
		case 'e':
		case 'd':
		case 'D':
			comando = 'e';
			break;
		case 'o':
		case 'a':
		case 'A':
			comando = 'o';
			break;
		case 'h':
		case 'H':
			comando = 'h';
			break;
		case 'i':
		case 'I':
			comando = 'i';
			break;
		case 'q':
		case 'Q':
			comando = 'q';
			break;
	}
	return comando;
}

/* Filtra l'input dell'utente, bloccando i movimenti che uscirebbero dai limiti del campo */
int legitPosition(char comando, int altezza, int ampiezza, int x, int y)
{
	int legit = 0;
	switch (comando)
	{
		case 'n':
			if (x - 1 > -1)
				legit = 1;
			break;
		case 's':
			if (x + 1 < altezza)
				legit = 1;
			break;
		case 'e':
			if (y + 1 < ampiezza)
				legit = 1;
			break;
		case 'o':
			if (y - 1 > -1)
				legit = 1;
			break;
	}
	return legit;
}

/* Stampa un menu' che illustra i possibili input validi */
void stampaHelp()
{
	printf("\n\nMovimento verso l'%salto%s: ", RED, NORMAL);
	printf("w, freccia su\n\n");

	printf("Movimento verso il %sbasso%s: ", YELLOW, NORMAL);
	printf("s, freccia giu\n\n");

	printf("Movimento verso %sdestra%s: ", BLUE, NORMAL);
	printf("d, freccia destra\n\n");

	printf("Movimento verso %ssinistra%s: ", GREEN, NORMAL);
	printf("a, freccia sinistra\n\n");

	printf("%sAiuto%s: h\n\n", MAGENTA, NORMAL);

	printf("%sInfo%s: i\n\n", CYAN, NORMAL);

	printf("\x1b[1mEsci\x1b[0m: q\n");
	return;
}
