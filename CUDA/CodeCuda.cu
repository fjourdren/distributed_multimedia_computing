#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#define initTimer struct timeval tv1, tv2; struct timezone tz
#define startTimer gettimeofday(&tv1, &tz)
#define stopTimer gettimeofday(&tv2, &tz)
#define tpsCalcul (tv2.tv_sec-tv1.tv_sec)*1000000L + (tv2.tv_usec-tv1.tv_usec)


#define MAX_DIM_GRID 65535
#define MAX_DIM_BLOCK 1024


#define MAX_CHAINE 100

#define MIN(a, b) 	(a < b ? a : b)
#define MAX(a, b) 	(a > b ? a : b)

#define CALLOC(ptr, nr, type) 		if (!(ptr = (type *) calloc((size_t)(nr), sizeof(type)))) {		\
						printf("Erreur lors de l'allocation memoire \n") ; 		\
						exit (-1);							\
					} 


#define FOPEN(fich,fichier,sens) 	if ((fich=fopen(fichier,sens)) == NULL) { 				\
						printf("Probleme d'ouverture du fichier %s\n",fichier);		\
						exit(-1);							\
					} 

#define MAX_VALEUR 	255
#define MIN_VALEUR 	0

#define NBPOINTSPARLIGNES 15

#define false 0
#define true 1
#define boolean int

long tailleVecteur ;

/* KERNEL CUDA */

__global__ void rehaussement_contraste_gpu(int *vec, int *res, int min, float coef ,long N) {
	long i = (long)blockIdx.x * (long)blockDim.x + (long)threadIdx.x;
	res[i] = (vec[i] - min) * coef;
}

void rehaussement_contraste_cpu(int *vec, int *res, int min, float coef ,long N) 
{
	long i ;
	for (i=0 ; i < N ; i ++) {
		res[i] = (vec[i] - min) * coef;
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Erreur, manque un argument\n");
		exit(0);
	}
	/*========================================================================*/
	/* Declaration de variables et allocation memoire */
	/*========================================================================*/

	int i, j, n;
	
	int LE_MIN = MAX_VALEUR;
	int LE_MAX = MIN_VALEUR;
	
	float ETALEMENT = 0.0;
	
	int **image;
	int **resultat;
	int X, Y, x, y;

	int P;
	
	FILE *Src, *Dst;

	char SrcFile[MAX_CHAINE];
	char DstFile[MAX_CHAINE];
	
	char ligne[MAX_CHAINE];
	
	boolean inverse = false;
	
	char *Chemin;
	
	/*========================================================================*/
	/* Recuperation des parametres						*/
	/*========================================================================*/

	sscanf(argv[1],"%s", SrcFile);
	
	sprintf(DstFile,"%s.new",SrcFile);
	
	/*========================================================================*/
	/* Recuperation de l'endroit ou l'on travail				*/
	/*========================================================================*/

	CALLOC(Chemin, MAX_CHAINE, char);
	Chemin = getenv("PWD");
	printf("Repertoire de travail : %s \n\n",Chemin);
	

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
			
	tailleVecteur = X * Y;
	
	x = 0;
	y = 0;
	
	
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
	/* Code CUDA --> Calcul de chaque nouvelle valeur de pixel */ 
	/*========================================================================*/
	long blocksize = 1;	

	// GPUmode, if 1 -> use cuda & gpu else use cpu
	int gpumode = 1;

	int *vecteur;
	int *resultatContraste;
	int *cudaVec;
	int *cudaRes;

	initTimer;

	long size = sizeof(int)*tailleVecteur;

	vecteur = (int *)malloc(size);
	resultatContraste = (int *)malloc(size);

	if (vecteur == NULL) {
		printf("Allocation memoire qui pose probleme (vecteur) \n");
	}
	if (resultatContraste == NULL) {
		printf("Allocation memoire qui pose probleme (resultatContraste) \n");
	}

	// DONE: init vec and res
	long i_vec = 0 ;
	for (i = 0 ; i < Y ; i++) {
		for (j = 0 ; j < X ; j++) {
			vecteur[i_vec] = image[i][j];
			resultatContraste[i_vec] = 0;
			i_vec++;
		}
	}

	if (gpumode==1){
		printf("Using gpu\n");

		if (cudaMalloc((void **)&cudaVec, size) == cudaErrorMemoryAllocation) {
			printf("Allocation memoire qui pose probleme (cudaVec) \n");
		}
		if (cudaMalloc((void **)&cudaRes, size)  == cudaErrorMemoryAllocation) {
			printf("Allocation memoire qui pose probleme (cudaRes) \n");
		}

		long dimBlock = blocksize;
		long dimGrid = tailleVecteur/blocksize;
		if ((tailleVecteur % blocksize) != 0) {
			dimGrid++;
		}


		int res = cudaMemcpy(&cudaVec[0], &vecteur[0], size, cudaMemcpyHostToDevice);

		// printf("Copy CPU -> GPU %d \n",res);
	startTimer;
		rehaussement_contraste_gpu<<<dimGrid, dimBlock>>>(cudaVec, cudaRes, LE_MIN, ETALEMENT, tailleVecteur);
		// DONE: Wait for GPU to finish before accessing on host
		cudaDeviceSynchronize();
	stopTimer;

		cudaMemcpy(&resultatContraste[0], &cudaRes[0], size, cudaMemcpyDeviceToHost);


		/* Test bon fonctionnement */

		bool ok = true;
		int indice = -1;
		int valtest = -1;
		for (i_vec= 0 ; i_vec < tailleVecteur ; i_vec++) {	
			
			valtest = (vecteur[i_vec] - LE_MIN) * ETALEMENT;
			if (resultatContraste[i_vec] != valtest) {
				// printf("Resultat GPU %d Resultat CPU %d \n", resultatContraste[i_vec], valtest);
				ok = false;
				if (indice ==-1) {
					indice = i_vec;
				}
			}
		}
		printf("------ ");
		printf("dimGrid %ld dimBlock %ld ",dimGrid, dimBlock);
		if (ok) {
			printf("Resultat ok\n");
		} else {
			printf("resultatContraste NON ok (%d)\n", indice);
		}
		
		
		cudaFree(cudaVec);
		cudaFree(cudaRes);
		/*========================================================================*/
		/* Fin Code CUDA */
		/*========================================================================*/

	}
	else
	{
		printf("Using cpu\n");
	startTimer;
		rehaussement_contraste_cpu(vecteur, resultatContraste, LE_MIN, ETALEMENT, tailleVecteur);
	stopTimer;

	}

	printf("chrono %ld \n", tpsCalcul);
	
	/*========================================================================*/
	/* Sauvegarde de l'image dans le fichier resultat			*/
	/*========================================================================*/
	
	n = 0;
	long cpt;
	for (cpt = 0 ; cpt < tailleVecteur ; cpt++) {
		// printf("%d \n", resultatContraste[cpt]);
		
		fprintf(Dst,"%3d ",resultatContraste[cpt]);
		n++;
		if (n == NBPOINTSPARLIGNES) {
			n = 0;
			fprintf(Dst, "\n");
		}
	}

	fprintf(Dst,"\n");
	fclose(Dst);


	/*========================================================================*/
	/* Fin du programme principal	*/
	/*========================================================================*/
	
	exit(0); 
}




