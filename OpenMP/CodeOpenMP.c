/*==============================================================================*/
/* Programme 	: CodeSequentiel.c						*/
/* Auteur 	: Daniel CHILLET						*/
/* Date 	: Decembre 2011							*/
/* 										*/
/*==============================================================================*/


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <omp.h>


// définition pour les temps de calcul (sans préemptions OS)
#define initClock    clock_t start_t, end_t, total_t;
#define beginClock start_t = clock()
#define endClock end_t = clock()
#define tpsClock (double)(end_t - start_t) / CLOCKS_PER_SEC
initClock;


#define MAX_CHAINE 100

#define MAX_HOSTS 100



#define CALLOC(ptr, nr, type) 		if (!(ptr = (type *) calloc((size_t)(nr), sizeof(type)))) {		\
						printf("Erreur lors de l'allocation memoire \n") ; 		\
						exit (-1);							\
					} 


#define FOPEN(fich,fichier,sens) 	if ((fich=fopen(fichier,sens)) == NULL) { 				\
						printf("Probleme d'ouverture du fichier %s\n",fichier);		\
						exit(-1);							\
					} 
				
#define MIN(a, b) 	(a < b ? a : b)
#define MAX(a, b) 	(a > b ? a : b)

#define MAX_VALEUR 	255
#define MIN_VALEUR 	0

#define NBPOINTSPARLIGNES 15

#define false 0
#define true 1
#define boolean int

int main(argc, argv)
int argc;
char *argv[];
{
	/*========================================================================*/
	/* Declaration de variables et allocation memoire */
	/*========================================================================*/

	int i, j, n;
	int info ;
	int nbhost, nbtaches, nbarch;
	int msgtype ;
	int who;
	
	int LE_MIN = MAX_VALEUR;
	int LE_MAX = MIN_VALEUR;
	
	float ETALEMENT = 0.0;
	
	int **image;
	int **resultat;
	int X, Y, x, y;
	int TailleImage;

	int NbResultats, quelle_ligne, lignes;
	int *la_ligne;
	
	int P;
	
	FILE *Src, *Dst;

	char SrcFile[MAX_CHAINE];
	char DstFile[MAX_CHAINE];
	
	char ligne[MAX_CHAINE];
	
	int NumLigne, NumTache;
	int ReponsesRecues;
	
	boolean fin ;
	boolean inverse = false;
	
	char *Chemin;
	char *CheminTache;
	
	/*========================================================================*/
	/* Recuperation des parametres						*/
	/*========================================================================*/

	sscanf(argv[1],"%s", SrcFile);
	
	sprintf(DstFile,"%s.new",SrcFile);
	
	/*========================================================================*/
	/* Recuperation de l'endroit ou l'on travail				*/
	/*========================================================================*/

	CALLOC(Chemin, MAX_CHAINE, char);
	CALLOC(CheminTache, MAX_CHAINE, char);
	Chemin = getenv("PWD");
	printf("Repertoire de travail : %s \n\n",Chemin);


	/*========================================================================*/
	/* Affichage des information OpenMP						*/
	/*========================================================================*/
	int nthreads, tid, procs, maxt, inpar, dynamic, nested;
	/* Start parallel region */
	#pragma omp parallel private(nthreads, tid)
	{
		/* Obtain thread number */
		tid = omp_get_thread_num();
		/* Only master thread does this */ 
		if (tid == 0) {
			printf("Thread %d getting environment info...\n", tid);	
			/* Get environment information */
			procs = omp_get_num_procs();
			nthreads = omp_get_num_threads();
			maxt = omp_get_max_threads();
			inpar = omp_in_parallel();
			dynamic = omp_get_dynamic();	
			nested = omp_get_nested();
			/* Print environment information */
			printf("Number of processors = %d\n", procs);	
			printf("Number of threads = %d\n", nthreads);
			printf("Max threads = %d\n", maxt);	
			printf("In parallel? = %d\n", inpar);
			printf("Dynamic threads enabled? = %d\n", dynamic);
			printf("Nested parallelism supported? = %d\n", nested);	
		}
	}

	/*========================================================================*/
	/* Ouverture des fichiers						*/
	/*========================================================================*/

	printf("Operations sur les fichiers\n");

	FOPEN(Src, SrcFile, "r");
	printf("\t Fichier source ouvert (%s) \n",SrcFile);
		
	FOPEN(Dst, DstFile, "w");
	printf("\t Fichier destination ouvert (%s) \n",DstFile);
	
	/*========================================================================*/
	/* On effectue la lecture du fichier source */
	/*========================================================================*/
	
	printf("\t Lecture entete du fichier source ");
	
	for (i = 0 ; i < 2 ; i++) {
		fgets(ligne, MAX_CHAINE, Src);	
		fprintf(Dst,"%s", ligne);
	}	

	fscanf(Src," %d %d\n",&X, &Y);
	fprintf(Dst," %d %d\n", X, Y);
	
	fgets(ligne, MAX_CHAINE, Src);	/* Lecture du 255 	*/
	fprintf(Dst,"%s", ligne);
	
	printf(": OK \n");
	
	/*========================================================================*/
	/* Allocation memoire pour l'image source et l'image resultat 		*/
	/*========================================================================*/
	
	CALLOC(image, Y+1, int *);
	CALLOC(resultat, Y+1, int *);
	for (i=0;i<Y;i++) {
		CALLOC(image[i], X+1, int);
		CALLOC(resultat[i], X+1, int);
		for (j=0;j<X;j++) {
			image[i][j] = 0;
			resultat[i][j] = 0;
		}
	}
	printf("\t\t Initialisation de l'image [%d ; %d] : Ok \n", X, Y);
			
	TailleImage = X * Y;
	
	x = 0;
	y = 0;
	
	lignes = 0;
	
	/*========================================================================*/
	/* Lecture du fichier pour remplir l'image source 			*/
	/*========================================================================*/
	
	while (! feof(Src)) {
		n = fscanf(Src,"%d",&P);
		image[y][x] = P;	
		LE_MIN = MIN(LE_MIN, P);
		LE_MAX = MAX(LE_MAX, P);
		x ++;
		if (n == EOF || (x == X && y == Y-1)) {
			break;
		}
		if (x == X) {
			x = 0 ;
			y++;
		}
	}
	fclose(Src);
	printf("\t Lecture du fichier image : Ok \n\n");
	
	/*========================================================================*/
	/* Calcul du facteur d'etalement					*/
	/*========================================================================*/
	
	if (inverse) {
		ETALEMENT = 0.2;	
	} else {
		ETALEMENT = (float)(MAX_VALEUR - MIN_VALEUR) / (float)(LE_MAX - LE_MIN);	
	}
	
	
	
	/*========================================================================*/
	/* Calcul de cahque nouvelle valeur de pixel							*/
	/*========================================================================*/
	
	// début chrono
	beginClock;

    omp_set_num_threads(nthreads);
	#pragma omp parallel for
	for (i = 0 ; i < Y ; i++) {
		for (j = 0 ; j < X ; j++) {
			resultat[i][j] = ((image[i][j] - LE_MIN) * ETALEMENT);
		}
	}

	// Fin chrono
	endClock;

	// affichage du chrono
	printf("chrono %f ", tpsClock);

	/*========================================================================*/
	/* Sauvegarde de l'image dans le fichier resultat			*/
	/*========================================================================*/
	
	n = 0;
	for (i = 0 ; i < Y ; i++) {
		for (j = 0 ; j < X ; j++) {
			
			fprintf(Dst,"%3d ",resultat[i][j]);
			n++;
			if (n == NBPOINTSPARLIGNES) {
				n = 0;
				fprintf(Dst, "\n");
			}
		}
	}
				
	fprintf(Dst,"\n");
	fclose(Dst);
	
	printf("\n");

	/*========================================================================*/
	/* Fin du programme principal	*/
	/*========================================================================*/
	
	exit(0); 
	
}
