/*==============================================================================*/
/* Programme 	: Tache.c (Escalve)						*/
/* Auteur 	: Daniel CHILLET						*/
/* Date 	: Novermbre 2001						*(
/* 										*/
/* Objectifs	: Une tache qui fait pas grand chose d'autre que d'afficher	*/
/* 		  sur quelle machine elle tourne 				*/
/* 										*/
/* Principe	: Tache est lance par le maitre (LanceTaches) 			*/
/* 										*/
/*==============================================================================*/

#include <stdlib.h>
#include <stdio.h>
#include "pvm3.h"


#define MAX_CHAINE 100


#define MAITRE_ENVOI 	0
#define MAITRE_RECOIT	5

#define ESCLAVE_ENVOI	MAITRE_RECOIT
#define ESCLAVE_RECOIT 	MAITRE_ENVOI

main(argc, argv)
int argc;
char *argv[];
{
	int i;
	int param;
	int indice1 = -1;
	int indice2 = -1;
	int mytid;
	int nbtaches;
	int info1;
	int info2;
	int ti_host;
	int nhost;
	int narch;
	int maitre;
	int msgtype;
	int newvaleur;
	char machine[MAX_CHAINE];
	struct pvmtaskinfo *taskinfo;
	struct pvmhostinfo *hostp;

	// log
	/*FILE *OutputFile;
	char fichierOutput[100];
	sprintf(fichierOutput, "log%d.txt", pvm_mytid());
	OutputFile = fopen(fichierOutput, "w");*/

	wait(2);

	sscanf(argv[1],"%d",&param);
	sscanf(argv[2],"%s",machine);

	taskinfo = calloc(1, sizeof(struct pvmtaskinfo));
	hostp = calloc(1, sizeof(struct pvmhostinfo));

	mytid = pvm_mytid();

	info1 = pvm_tasks(mytid, &nbtaches, &taskinfo);
	for (i=0 ; i < nbtaches ; i++) {
		if (mytid == taskinfo[i].ti_tid) {
			indice1 = i;
		}
	}
	ti_host = taskinfo[indice1].ti_host;


	info2 = pvm_config(&nhost, &narch, &hostp);
	for (i=0 ; i < nhost ; i++) {
		if (ti_host == hostp[i].hi_tid) {
			indice2 = i;
		}
	}
	//printf("Tache %d (%d): je tourne sur la machine %s (%s : %d)\n",param, mytid, hostp[indice2].hi_name, machine, ti_host);


    // mallocs pour les images
    float ETALEMENT;
    int X, LE_MIN, pvm_mytidHost, index;

    int stop = 0;
    int malloced_pixels_arrays = 0;
    int *ligne, *resultat;
    while(stop == 0) {
        /*========================================================================*/
        /* Réception 						                                      */
        /*========================================================================*/
        /** RECEPTION
            - id_parent
            - LE_MIN
            - ETALEMENT
            - X
            - index de la ligne
            - ligne
        */

        msgtype = ESCLAVE_RECOIT;
        pvm_recv(-1, msgtype);

        // réception de stop
        pvm_upkint(&stop, 1, 1);

        // réception de l'ID du parent

        pvm_upkint(&pvm_mytidHost, 1, 1);

        // réception de le_min
        pvm_upkint(&LE_MIN, 1, 1);

        // réception de l'étalement
        pvm_upkfloat(&ETALEMENT, 1, 1);

        // réception de X
        pvm_upkint(&X, 1, 1);

        // build arrays if needed
        if(malloced_pixels_arrays == 0) {
            ligne = malloc(X * sizeof(int));
            resultat = malloc(X * sizeof(int));
            malloced_pixels_arrays = 1;
        }

        // réception de l'index de la ligne
        pvm_upkint(&index, 1, 1);

        // réception de la ligne
        pvm_upkint(&ligne[0], X, 1);


        /*========================================================================*/
        /* Calcul de chaque nouvelle valeur de pixel				  			  */
        /*========================================================================*/
        for (i = 0 ; i < X ; i++) {
            resultat[i] = ((ligne[i] - LE_MIN) * ETALEMENT);
        }

        /*========================================================================*/
        /* Envoi de la nouvelle ligne au maître                                   */
        /*========================================================================*/

        if(stop == 0) {
            // init envoi
            msgtype = ESCLAVE_ENVOI;
            pvm_initsend(PvmDataDefault);

            // construction des variables à envoyer
            pvm_pkint(&mytid, 1, 1);
            pvm_pkint(&index, 1, 1);
            pvm_pkint(resultat, X, 1);

            // envoi
            pvm_send(pvm_mytidHost, msgtype);

            //fprintf(OutputFile, "Ligne num %d | STOP: %d | premier pixel: %d\n", index, stop, resultat[0]);
        }
        
    }

    //fprintf(OutputFile, "Closing task %d\n", pvm_mytidHost);

	pvm_exit();


	//fclose(OutputFile);

	exit(0);

}

