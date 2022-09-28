#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "structVarPartageeThread.h"
#include "calcul.h"

#include <time.h>

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1

void fonctionDemande(void *p)
{

    struct params *args = (struct params *)p;
    struct varPartageeThread *varPart = args->varPartagee;

    while (1)
    {
        char str[100];
        int texte = 0;

        printf("\nEntrez 'demande' pour entrer en SC ou 'retard' : \n\n");
        scanf("%s", str);

        while (!(strcmp(str, "demande") == 0 || (strcmp(str, "retard") == 0 && varPart->jetonDaccord)))
        {
            if (!(strcmp(str, "demande") == 0 || (strcmp(str, "retard") == 0)))
            {
                printf("\nErreur, entrez 'demande' pour entrer en SC ou 'retard' :\n\n");
                scanf("%s", str);
            }
            else
            {
                // cas où on aurait donné le jeton d'accord, malgré une demande antérieure sur ce site présente à ce moment là = impossible
                printf("\n\nATTENTION : Cas impossible en réalité, jeton d'accord donné à un site signifie qu'aucune demande antérieure n'avait été faite sur ce site\n");
                printf("\nEntrez 'demande' pour rentrer en section critique : \n");
                scanf("%s", str);
            }
        }

        if (strcmp(str, "demande") == 0 || (strcmp(str, "retard") == 0 && varPart->jetonDaccord))
        {
            if (strcmp(str, "demande") == 0)
            {
                printf("Demande envoyée !\n");
                texte = 1;
                insertion(varPart->liste, varPart->monId);
                // si j'ai mon jeton, sinon c'est celui à qui je le donne qui est en SC
                varPart->idSiteEnSectionCritique = varPart->monId;
            }
            else if (strcmp(str, "retard") == 0 && varPart->jetonDaccord)
            {
                texte = 2;
                insererTete(varPart->liste, varPart->monId);
                printf("Simulation d'une demande antérieure qui n'a pas été traitée, envoyée !\n");
                // J'ai forcément mon jeton sinon je l'aurai pas donné
            }

            char timeS[100] = ":";

            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            sprintf(timeS, ":%02d%02d%02d%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

            if (texte == 1 && varPart->jetonDaccord)
            {
                varPart->prioCurrentProcessus = atoll(timeS + 1);
                varPart->maDemande = atoll(timeS + 1);
            }
            else
            {
                varPart->prioCurrentProcessus = atoll("20201214111217");
            }

            //Un calcul est fait lors de 'demande', permet d'entrer 'retard' sur un autre terminal avant que tous les jetons ne soient donnés
            if (texte != 2)
            {
                calcul(3);
            }

            // Envoi des messages de demandes avec la date de ma demande ou date en retard
            for (int i = 0; i < varPart->tailleQuorum; i++)
            {
                int socketClient = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in adrServ;
                adrServ.sin_addr.s_addr = inet_addr(varPart->quorum[i].adresseIP);
                adrServ.sin_family = AF_INET;
                int lgAd = sizeof(struct sockaddr_in);
                adrServ.sin_port = htons(varPart->quorum[i].port);

                // construction du message "MESSAGE:port:date"
                char msg[100] = "DEMANDE:";
                char id[20];
                sprintf(id, "%d", varPart->monId);
                strcat(msg, id);

                if (texte == 1)
                {
                    strcat(msg, timeS);
                }
                else
                {
                    strcat(msg, ":20201214111217");
                }

                int connexion = connect(socketClient, (struct sockaddr *)&adrServ, lgAd);
                if (connexion == -1)
                {
                    perror("connect()");
                    pthread_exit(NULL);
                }

                int snd = send(socketClient, msg, sizeof(msg), 0);
                if (snd <= 0)
                {
                    perror("send()\n");
                    pthread_exit(NULL);
                }

                close(socketClient);
            }

            int lock = pthread_mutex_lock(&(varPart->v.lock));
            if (lock != 0)
            {
                perror("lock() : ");
                exit(1);
            }

            int wait = pthread_cond_wait(&(varPart->v.entreeSC), &(varPart->v.lock));
            if (wait != 0)
            {
                perror("wait() :");
                exit(1);
            }

            // calcul qui permet de voir quand est-ce que l'on est vraiment en section critique
            printf("La file d'attente : ");
            afficherListe(varPart->liste);
            printf("\nJ'accède à la section critique ici\n");
            calculV2(5);
            printf("Je sors de la section critique\n\n");
            varPart->v.nbAccords = 0;

            if (varPart->liste->premier != NULL)
            {
                if (varPart->liste->premier->nombre == varPart->monId)
                {
                    suppressionTete(varPart->liste);
                }
            }

            int unlock = pthread_mutex_unlock(&(varPart->v.lock));
            if (unlock != 0)
            {
                perror("unlock() :");
                exit(1);
            }

            printf("Je libère les sites de mon quorum\n");

            // envoi des messages de libération
            for (int i = 0; i < varPart->tailleQuorum; i++)
            {
                int socketClient = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in adrServ;
                adrServ.sin_addr.s_addr = inet_addr(varPart->quorum[i].adresseIP);
                adrServ.sin_family = AF_INET;
                int lgAd = sizeof(struct sockaddr_in);
                adrServ.sin_port = htons(varPart->quorum[i].port);

                // construction du message "MESSAGE:port"
                char msg[30] = "LIBERATION:";
                char id[20];
                sprintf(id, "%d", varPart->monId);
                strcat(msg, id);
                strcat(msg, timeS);

                int connexion = connect(socketClient, (struct sockaddr *)&adrServ, lgAd);
                if (connexion == -1)
                {
                    perror("connect()");
                    pthread_exit(NULL);
                }

                int snd = send(socketClient, msg, sizeof(msg), 0);
                if (snd <= 0)
                {
                    perror("send() !\n");
                    pthread_exit(NULL);
                }

                close(socketClient);
            }

            printf("Ma file d'attente : ");
            afficherListe(varPart->liste);

            if (varPart->liste->premier != NULL)
            {
                varPart->jetonDaccord = 0;
                printf("J'envoie un message d'accord au premier site en attente dans ma file \n");
                repondreAUnSite("ACCORD", getSiteFromID(varPart->liste->premier->nombre, varPart->quorum, varPart->tailleQuorum));
                suppressionTete(varPart->liste);
            }
            else
            {
                varPart->jetonDaccord = 1;
                printf("Liste d'attente vide, je garde mon jeton d'accord\n");
            }

            printf("Ma file d'attente : ");
            afficherListe(varPart->liste);
        }
    }
}