#include "interface.h"

/* Alloca il campo da gioco */
void matrixAlloc(struct settings *GS)
{
	int i;
	GS->matrix = (char **)malloc(GS->height * sizeof(char *));
	for (i = 0; i < GS->height; i++)
		GS->matrix[i] = (char *)malloc(GS->width * sizeof(char));
}

/* Imposta i valori per la nuova partita */
void init(struct settings *GS)
{
	int min, range;
	GS->height = (rand()%RANGEMATR) + MINMATR;
	GS->width = (rand()%RANGEMATR) + MINMATR;
	min = GS->height < GS->width ? GS->height : GS->width;
	range = ((min + 1) / 2) * (min - 1);
	GS->obstacles = (rand()%range);
	GS->treasures = (rand()%((GS->height * GS->width) / 4)) + 5;
	GS->totTreasures = GS->treasures;
	GS->timeLimit = TIMELIMIT;
	matrixAlloc(GS);
	return;
}

/* "Popolamento" della matrice */
void sow(struct settings *GS)
{
	int x, y, tesori, ostacoli, min;
	/* Inizialmente, tutte le celle sono libere */
	for (x = 0; x < GS->height; x++)
		for (y = 0; y < GS->width; y++)
			GS->matrix[x][y] = 'f';
	/* OSTACOLI ASSICURATI */
	min = GS->height < GS->width ? GS->height : GS->width;
	for (ostacoli = 0; ostacoli < min; ostacoli++)
	{
		x = rand()%GS->height;
		y = rand()%GS->width;
		if (GS->matrix[x][y] == 'f') /*L'azzeramento non prevede slot occupati da giocatori*/
		{
			if (x - 1 >= 0 && y - 1 >= 0 && GS->matrix[x - 1][y - 1] == 'o')
				ostacoli--;
			else if (x + 1 < GS->height && y - 1 >= 0 && GS->matrix[x + 1][y - 1] == 'o')
				ostacoli--;
			else if (x - 1 >= 0 && y + 1 < GS->width && GS->matrix[x - 1][y + 1] == 'o')
				ostacoli--;
			else if (x + 1 < GS->height && y + 1 < GS->width && GS->matrix[x + 1][y + 1] == 'o')
				ostacoli--;
			else if (x == 0 && GS->matrix[GS->height - 1][y] == 'o')
				ostacoli--;
			else if (x == GS->width - 1 && GS->matrix[0][y] == 'o')
				ostacoli--;
			else if (y == 0 && GS->matrix[x][GS->height - 1] == 'o')
				ostacoli--;
			else if (y == GS->height - 1 && GS->matrix[x][0] == 'o')
				ostacoli--;
			else
				GS->matrix[x][y] = 'o';
		}
		else
			ostacoli--;
	}
	/* OSTACOLI OPZIONALI */
	for (ostacoli = 0; ostacoli < GS->obstacles; ostacoli++)
	{
		x = rand()%GS->height;
		y = rand()%GS->width;
		if (GS->matrix[x][y] == 'f') /*L'azzeramento non prevede slot occupati da giocatori*/
		{
			if (x - 1 >= 0 && y - 1 >= 0 && GS->matrix[x - 1][y - 1] == 'o');
			else if (x + 1 < GS->height && y - 1 >= 0 && GS->matrix[x + 1][y - 1] == 'o');
			else if (x - 1 >= 0 && y + 1 < GS->width && GS->matrix[x - 1][y + 1] == 'o');
			else if (x + 1 < GS->height && y + 1 < GS->width && GS->matrix[x + 1][y + 1] == 'o');
			else if (x == 0 && GS->matrix[GS->height - 1][y] == 'o');
			else if (x == GS->width - 1 && GS->matrix[0][y] == 'o');
			else if (y == 0 && GS->matrix[x][GS->height - 1] == 'o');
			else if (y == GS->height - 1 && GS->matrix[x][0] == 'o');
			else
				GS->matrix[x][y] = 'o';
		}
	}
	/* TESORI */
	for (tesori = 0; tesori < GS->treasures; tesori++)
	{
		x = rand()%GS->height;
		y = rand()%GS->width;
		if (GS->matrix[x][y] == 'f') /*L'azzeramento non prevede slot occupati da giocatori*/
		{
			GS->matrix[x][y] = 't';
		}
		else
			tesori--;
	}
	return;
}

/* Deallocazione della matrice */
void freeMatrix(struct settings * GS)
{
	int x;
	for (x = 0; x < GS->height; x++)
		free(GS->matrix[x]);
	free(GS->matrix);
	return;
}

/* Funzione per la gestione di errori */
void errorExit(char *error)
{
	perror(error);
	raise(SIGABRT);
}

/* Thread di gestione del campo da gioco */
void *initializer(void *GS)
{
	struct partita *tmp;
	srand(time(NULL));
	init((struct settings *) GS);
	sow((struct settings *) GS);
	elenco = NULL;
	while (ID_PARTITA != -1)
	{
		tempo_partenza = time(NULL);
		/* Aggiungo una nuova partita all'elenco delle informazioni DOPO aver rimosso le info ormai inutili*/
		if (pthread_mutex_lock(&mutex_informazioni))
			errorExit("Errore lock informazioni partita");
		/* elenco = garbageCollector(elenco); */
		elenco = appendGame(elenco, newGame(ID_PARTITA, ((struct settings *) GS)->treasures));
		if (pthread_mutex_unlock(&mutex_informazioni))
			errorExit("Errore unlock informazioni partita");
		/* Ciclo di attesa fino al termine della partita */
		while( ((int)time(NULL) - tempo_partenza <= TIMELIMIT) && ((struct settings *) GS)->treasures > 0)
		{
			usleep(0.5);
		}
		if (pthread_mutex_lock(&mutex_informazioni))
			errorExit("Errore lock informazioni in thread initializer");
		/* Settaggio del vincitore */
		tmp = search(elenco, ID_PARTITA);
		getWinner(tmp);
		if (pthread_mutex_lock(&mutex_partita))
			errorExit("Errore lock partita in thread initializer");
		/* Nuovo campo da gioco; nuovo id partita */
		freeMatrix((struct settings *) GS);
		init((struct settings *) GS);
		sow((struct settings *) GS);
		ID_PARTITA = (ID_PARTITA%999)+1;
		if (pthread_mutex_unlock(&mutex_partita))
			errorExit("Errore unlock partita in thread initializer");
		if (pthread_mutex_unlock(&mutex_informazioni))
			errorExit("Errore unlock informazioni in thread initializer");
	}
	/* Qui il server ha richiesto la terminazione */
	if (pthread_mutex_lock(&mutex_partita))
		errorExit("Errore lock partita");
	freeMatrix((struct settings *) GS);
	if (pthread_mutex_unlock(&mutex_partita))
		errorExit("Errore unlock partita");
	free(GS);
	elenco = garbageDestroyer(elenco);
	pthread_exit(0);
}
