/*==============================================================================*/
/* Programme 	: CodeSequentiel.c						*/
/* Auteur 	: Daniel CHILLET						*/
/* Date 	: Decembre 2011							*/
/* 										*/
/*==============================================================================*/




#include <stdlib.h>
#include <stdio.h>
#include "pvm3.h"

#define MAX_CHAINE 100

#define MAX_HOSTS 100



#define RESULT_TAG 1
#define MAX_LENGTH 128

#define MAX_HOSTS 100
#define MAX_CHAINE 100
#define MAX_PARAM 10

#define MAITRE_ENVOI 	0
#define MAITRE_RECOIT	5

#define ESCLAVE_ENVOI	MAITRE_RECOIT
#define ESCLAVE_RECOIT 	MAITRE_ENVOI



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


// HEADERS
void envoiLigne(int destinataire, int stop, int Y, int pvm_mytidHost, int LE_MIN, float ETALEMENT, int X, int index, int *ligne);


// CODE
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


	// PVM
	int pvm_info ;
	int pvm_nhost;
	int pvm_mytidHost;
	int pvm_who, pvm_retour;

	int pvm_narch;
	struct pvmhostinfo *pvm_hostp;
	struct pvmtaskinfo *pvm_taskinfo;
	int pvm_i;
	int pvm_numtaches[MAX_HOSTS];
	int pvm_nbtaches;
	char *pvm_param[MAX_PARAM];

	char *pvm_Chemin[MAX_CHAINE];
	char pvm_Fichier[MAX_CHAINE];

	*pvm_Chemin = getenv("PWD");
/*	sprintf(pvm_Fichier, "%s/Tache",*Chemin);*/
	sprintf(pvm_Fichier, "./Tache");

	pvm_hostp = calloc(1, sizeof(struct pvmhostinfo));
	pvm_taskinfo = calloc(1, sizeof(struct pvmtaskinfo));

	int pvm_iddaemon ;
	int pvm_msgtype;

	int pvm_valeur;


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
		ETALEMENT = (float) (MAX_VALEUR - MIN_VALEUR) / (float)(LE_MAX - LE_MIN);
	}




	/*========================================================================*/
	/* Calcul de chaque nouvelle valeur de pixel							*/
	/*========================================================================*/

	// gestion des noeuds PVM
	pvm_mytidHost = pvm_mytid();


	pvm_info = pvm_tasks(0, &pvm_nbtaches, &pvm_taskinfo);
	printf("\nListe des taches de la Parallel Virtuelle Machine \n");
	for (i=0 ; i < pvm_nbtaches ; i++) {
		if (pvm_taskinfo[i].ti_tid == pvm_mytidHost) {
			printf("\t Tache %d : tourne sur le noeud (ti_host) : %d ; Commentaire TACHE PRINCIPALE \n",
				pvm_taskinfo[i].ti_tid, pvm_taskinfo[i].ti_host);
		} else {
			printf("\t Tache daemon %d : tourne sur le noeud (ti_host) : %d ;\n",
				pvm_taskinfo[i].ti_tid, pvm_taskinfo[i].ti_host);
			pvm_iddaemon = pvm_taskinfo[i].ti_tid;
		}

	}

	info = pvm_config(&pvm_nhost, &pvm_narch, &pvm_hostp);

	printf("Nombre de noeuds dans la Parallel Virtual Machine : %d\n",pvm_nhost);
	printf("Nombre d'architecture dans la Parallel Virtual Machine : %d\n",pvm_narch);

	printf("\nListe des machines de la PVM :  \n");
	for (i=1 ; i < pvm_nhost ; i++) { // on passe à 1 pour prendre que les hôtes distants
		printf("\tNoeud %d : \n", i);
		printf("\t\t hi_tid = %d \n",pvm_hostp[i].hi_tid);
		printf("\t\t hi_name = %s \n",pvm_hostp[i].hi_name);
		printf("\t\t hi_arch = %s \n",pvm_hostp[i].hi_arch);
		printf("\t\t hi_speed = %d \n",pvm_hostp[i].hi_speed);

		pvm_param[0] = calloc(1, MAX_CHAINE);
		pvm_param[1] = calloc(1, MAX_CHAINE);
		sprintf(pvm_param[0],"%d",pvm_i);
		sprintf(pvm_param[1], "%s", pvm_hostp[i].hi_name);
		pvm_param[2] = NULL;

		/* Pour lancer la tache sur un host particulier,
		il faut placer le flag PvmTaskHost
		Sinon, le systeme choisit tout seul le meilleur host
		pour lancer la tache */
      		pvm_nbtaches = pvm_spawn(pvm_Fichier, &pvm_param[0], PvmTaskHost, pvm_hostp[i].hi_name,1, &pvm_numtaches[i]);

		printf("\tLance une tache sur %s : Tache %c %s %s (%d) \n", pvm_hostp[i].hi_name, pvm_numtaches[i], pvm_param[0], pvm_param[1], pvm_nbtaches);
	}

	pvm_info = pvm_tasks(0, &pvm_nbtaches, &pvm_taskinfo);
	printf("\nListe des taches de la Parallel Virtuelle Machine : %d \n", nbtaches);
	for (i=0 ; i < pvm_nbtaches ; i++) {
		if (pvm_taskinfo[i].ti_tid == pvm_iddaemon) {
			printf("\t Tache %d : tache systeme pvmd (Daemon) \n",pvm_taskinfo[i].ti_tid);
		} else if (pvm_taskinfo[i].ti_tid == pvm_mytidHost) {
			printf("\t Tache %d : tourne sur le noeud (ti_host) : %d ; Commentaire TACHE PRINCIPALE \n",
				pvm_taskinfo[i].ti_tid, pvm_taskinfo[i].ti_host);
		} else {
			printf("\t Tache %d : tourne sur le noeud (ti_host) : %d ;\n",
				pvm_taskinfo[i].ti_tid, pvm_taskinfo[i].ti_host);
		}

	}





	// boucle et envoi de chaque ligne
	/*for (i = 0 ; i < Y ; i++) { // récupère ligne par ligne
        envoiLigne(, Y, pvm_mytidHost, LE_MIN, ETALEMENT, X, i, image[i]);



        // init réception
        pvm_msgtype = MAITRE_RECOIT;
        pvm_recv(-1, pvm_msgtype);

        // récupère l'index
        int index_task;
        pvm_upkint(&index_task, 1, 1);

        // récupère la ligne traité
        int *ligne_task = malloc(X * sizeof(int));
        pvm_upkint(&ligne_task[0], X, 1);

        printf("Pixel reçu : %d\n", ligne_task[0]);



        resultat[index_task] = ligne_task;
	}*/



	// envoi de stop à tous les esclaves
	int last_line_sent = 0;
	int n_ligne;

    //for (n_ligne=0; n_ligne < 1; n_ligne++) {
	for (n_ligne=0; n_ligne < pvm_nbtaches; n_ligne++) {
		if(pvm_numtaches[n_ligne] >= 500000 && pvm_numtaches[n_ligne] <= 999999) {
			envoiLigne(pvm_numtaches[n_ligne], 0, Y, pvm_mytidHost, LE_MIN, ETALEMENT, X, last_line_sent, image[last_line_sent]); // envoi de la ligne
			last_line_sent = last_line_sent + 1; // incrémentation de la dernière ligne envoyé
		}
    }

    // réception
    int id_task, index_task;
	int *ligne_task;
    int nb_line_received = 0;
    while(nb_line_received < Y) {
        // réception
        pvm_msgtype = MAITRE_RECOIT;
        pvm_recv(-1, pvm_msgtype);

        // récupère l'id de la tâche
        pvm_upkint(&id_task, 1, 1);

        // récupère l'index
        pvm_upkint(&index_task, 1, 1);

        // récupère la ligne traité
		ligne_task = malloc(X * sizeof(int));
        pvm_upkint(&ligne_task[0], X, 1);

        // ajout de la ligne dans le tableau des résultats
        resultat[index_task] = ligne_task;

        nb_line_received = nb_line_received + 1;

		printf("taks: %d | index: %d, %d < %d \n", id_task, index_task, nb_line_received, Y);




        // vérifie qu'il reste des lignes à traiter
        if(last_line_sent < Y) {
            // envoi d'un nouvelle ligne
            envoiLigne(id_task, 0, Y, pvm_mytidHost, LE_MIN, ETALEMENT, X, last_line_sent, image[last_line_sent]);
            last_line_sent = last_line_sent + 1;
        }
    }


    printf("Stopping\n");
    // stop
    int tache_stopping;
    for (tache_stopping=1; tache_stopping < pvm_nbtaches; tache_stopping++) {
		if(pvm_numtaches[tache_stopping] > 0) {
			envoiLigne(pvm_numtaches[tache_stopping], 1, 0, pvm_mytidHost, 0, 0, 0, 0, NULL);
		}
        
    }





	// FIN BOUCLE TRAITEMENT
	pvm_exit();



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


	// free memory
	int i_delete;
	for(i_delete = 0; i < Y; i++) {
		free(resultat[i_delete]);
	}


	printf("\n");

	/*========================================================================*/
	/* Fin du programme principal	*/
	/*========================================================================*/

	exit(0);

}


void envoiLigne(int destinataire, int stop, int Y, int pvm_mytidHost, int LE_MIN, float ETALEMENT, int X, int index, int *ligne) {
        /** ENVOI
            - stop
            - id_parent
            - LE_MIN
            - ETALEMENT
            - X
            - index de la ligne
            - ligne
        */

        // DEBUT BOUCLE TRAITEMENT
        int pvm_msgtype = MAITRE_ENVOI;
        pvm_initsend(PvmDataDefault);

        // si on est à la dernière ligne, envoi de stop
        pvm_pkint(&stop, 1, 1); // https://manpages.debian.org/jessie/pvm-dev/pvm_pkint.3

        // envoi de l'ID parent
        pvm_pkint(&pvm_mytidHost, 1, 1);

        // envoi de le_min
        pvm_pkint(&LE_MIN, 1, 1);

        // envoi de l'etalement
        pvm_pkfloat(&ETALEMENT, 1, 1);

        // envoi de X
        pvm_pkint(&X, 1, 1);

        // envoi de l'index de la ligne
        pvm_pkint(&index, 1, 1);

        // envoi de la première ligne
        pvm_pkint(ligne, X, 1);

        // envoi de toutes les infos
        pvm_send(destinataire, pvm_msgtype);

		if(stop == 1) {
			printf("Envoie de stop\n");
		} else {
			printf("Envoie ligne %d effectue \n", index);
		}
}
