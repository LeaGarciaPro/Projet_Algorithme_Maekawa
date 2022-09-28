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
#include "structListe.h"
#include "structVarPartageeThread.h"
#include "calcul.h"

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1

void fonctionReception(void *p)
{

    struct params *args = (struct params *)p;
    struct varPartageeThread *varPart = args->varPartagee;

    int socketServeur = args->socket;

    // Mise en écoute de la socket
    if (listen(socketServeur, varPart->tailleQuorum) == SOCKET_ERROR)
    {
        perror("listen()");
        pthread_exit(NULL);
    }

    char container[30];

    struct sockaddr_in adClient;
    socklen_t lgAdClient = sizeof(struct sockaddr_in);

    unsigned long long int estamp = 0;

    // boucle de réception de messages
    while (1)
    {

        int socketClient = accept(socketServeur, (struct sockaddr *)&adClient, &lgAdClient);
        if (socketClient == INVALID_SOCKET)
        {
            perror("accept()");
            pthread_exit(NULL);
        }

        int rec = recv(socketClient, container, sizeof(container), 0);
        if (rec == -1)
        {
            perror("recv()");
            pthread_exit(NULL);
        }

        // partie pour décrypter le message
        char *msg = strtok(container, ":");

        struct site siteEnvoyeur;
        siteEnvoyeur.idSite = -1;
        siteEnvoyeur.adresseIP = "-1";
        siteEnvoyeur.port = -1;

        char *token = strtok(NULL, ":");
        if (token != NULL)
        {
            siteEnvoyeur = getSiteFromID(atoi(token), varPart->quorum, varPart->tailleQuorum);
        }
        token = strtok(NULL, ":");
        if (token != NULL)
        {
            estamp = strtoull(token, NULL, 10);
        }

        if (strcmp(msg, "DEMANDE") == 0)
        {
            printf("\nNouveau message : DEMANDE \n");
            printf("Demande de rentrer en section critique du site de port : %d \n", siteEnvoyeur.port);
            
            // si le jeton = 1, j'ai pas donné mon accord à personne je peux lui donner
            if(varPart->jetonDaccord && varPart->liste->premier == NULL)
            {
                varPart->jetonDaccord = 0;
                // Je garde en mémoire le site qui est en section critique pour lui envoyer un SONDAGE si il y a besoin
                varPart->idSiteEnSectionCritique = siteEnvoyeur.idSite;
                repondreAUnSite("ACCORD", siteEnvoyeur);
                varPart->prioCurrentProcessus = estamp;
                printf("Envoi du jeton d'accord au site : %d \n", siteEnvoyeur.port);

                close(socketClient);
            }
            // je ne possède pas le jeton et la demande n'est pas prioritaire
            else if(varPart->prioCurrentProcessus <= estamp)
            {

                printf("Priorité processus qui demande : %llu\n", estamp);
                printf("Priorité processus bientôt en SC : %llu\n", varPart->prioCurrentProcessus);
                // insertion en queue de liste normale car pas la priorité
                if (estamp > varPart->maDemande)
                {
                    insertion(varPart->liste, siteEnvoyeur.idSite);
                }
                else
                {
                // insertion en tete de liste car priorité sur ma demande, elle a été formulée avant
                    insererTete(varPart->liste, siteEnvoyeur.idSite);
                }
                repondreAUnSite("ATTENTE", siteEnvoyeur);
                printf("J'envoie un message d'attente\n");
                close(socketClient);
            }
            // je ne possède pas le jeton et la demande est prioritaire sur celui en SC
            else if(varPart->prioCurrentProcessus > estamp)
            {

                printf("Priorité processus qui demande : %llu\n", estamp);
                printf("Priorité processus bientôt en SC : %llu\n", varPart->prioCurrentProcessus);

                if (varPart->idSiteEnSectionCritique != varPart->monId)
                {
                    char msg[100];
                    snprintf(msg, 100, "SOND:%d", varPart->monId);
                    repondreAUnSite(msg, getSiteFromID(varPart->idSiteEnSectionCritique, varPart->quorum, varPart->tailleQuorum));
                    // J'insère ce site prioritaire en tête de liste en espérant pouvoir lui envoyer mon accord si je le récupère
                    insererTete(varPart->liste, siteEnvoyeur.idSite);
                    printf("J'envoie un message de sondage au site %d en faveur du site %d\n", varPart->idSiteEnSectionCritique, siteEnvoyeur.idSite);
                    close(socketClient);
                }
                else
                {
                    // C'est moi bientôt en SC et je dois récupérer mon propre jeton pour le donner au processus plus prioritaire que moi
                    // ont la demande vient à peine d'arriver (après que j'ai fait ma demande)
                    // Si il arrive c'est qu'il ne m'avait pas encore donné son accord
                    varPart->jetonDaccord = 0;
                    printf("Je récupère mon jeton d'accord pour le donner au site qui me le demande: %d\n", siteEnvoyeur.idSite);    
                    varPart->idSiteEnSectionCritique = siteEnvoyeur.idSite;
                    varPart->prioCurrentProcessus = estamp;
                    repondreAUnSite("ACCORD", siteEnvoyeur);
                    close(socketClient);
                }
            }
        }
        else if (strcmp(msg, "ACCORD") == 0)
        {
            printf("\nNouveau message : ACCORD\n");

            varPart->v.nbAccords++;
            printf("Nombre d'accords obtenus : %d\n", varPart->v.nbAccords);

            calcul(1);

            int lock = pthread_mutex_lock(&(varPart->v.lock));
            if (lock != 0)
            {
                perror("lock() :");
                exit(1);
            }

            // Dès que tous les accords sont arrivés, je ne rends plus la section critique.
            // On considère qu'elle va être utilisée donc même si c'était pas la demande la plus prioritaire, on la laisse
            // Par contre, si elle a un accord en attente, elle doit rendre l'accord !

            if ((varPart->v.nbAccords == varPart->tailleQuorum) && varPart->jetonDaccord)
            {
                // Je vais rentrer en section critique, je ne donne donc plus mon propre jeton !
                varPart->jetonDaccord = 0;
                // Taille quorum + 1 jeton = je suis en SC
                varPart->v.nbAccords++;
                int broad = pthread_cond_signal(&(varPart->v.entreeSC));
                if (broad != 0)
                {
                    perror("signal() :");
                    exit(1);
                }
            }

            int unlock = pthread_mutex_unlock(&(varPart->v.lock));
            if (unlock != 0)
            {
                perror("unlock() :");
                exit(1);
            }

            close(socketClient);
        }
        else if (strcmp(msg, "ATTENTE") == 0)
        {
            printf("\nNouveau message : ATTENTE\n");
            printf("Je suis bloqué par un site pour l'instant\n");
            close(socketClient);
        }
        else if (strcmp(msg, "SOND") == 0)
        {
            printf("\nNouveau message : SONDAGE\n");

            if (!(varPart->v.nbAccords == varPart->tailleQuorum + 1))
            {
                char message[100];
                snprintf(message, 100, "REST:%d", varPart->monId);
                repondreAUnSite(message, siteEnvoyeur);
                printf("Je ne ne suis pas encore entré en SC, je rends le jeton d'accord !\n");
                // Je rends l'accord à celui qui me le demande
                varPart->v.nbAccords--;
                printf("Nombre d'accords reçus : %d\n", varPart->v.nbAccords);
                close(socketClient);
            }
            else
            {
                printf("JE SUIS DEJA EN SECTION CRITIQUE !\n");
                close(socketClient);
            }
        }
        else if (strcmp(msg, "REST") == 0)
        {
            printf("\nNouveau message : RESTITUTION\n");

            printf("Jeton d'accord récupéré, j'envoie un accord au site plus prioritaire\n");

            // J'envoie mon accord au site + prioritaire qui a été mis en tête de liste
            printf("Ma file d'attente : ");
            afficherListe(varPart->liste);
            repondreAUnSite("ACCORD", getSiteFromID(varPart->liste->premier->nombre, varPart->quorum, varPart->tailleQuorum));
            suppressionTete(varPart->liste);
            // Le site qui vient de me rendre son jeton devra être desservit donc je le met en tête de ma liste
            insererTete(varPart->liste, siteEnvoyeur.idSite);
            printf("Ma file d'attente : ");
            afficherListe(varPart->liste);
            varPart->prioCurrentProcessus = estamp;
            varPart->idSiteEnSectionCritique = siteEnvoyeur.idSite;
            close(socketClient);
        }
        else if (strcmp(msg, "LIBERATION") == 0)
        {
            printf("\nNouveau message : LIBERATION\n");

            afficherListe(varPart->liste);

            if (varPart->liste->premier != NULL && varPart->liste->premier->nombre == varPart->monId)
            {
                varPart->jetonDaccord = 1;
                varPart->idSiteEnSectionCritique = varPart->monId;
                varPart->prioCurrentProcessus = varPart->maDemande;
                printf("Je récupère mon jeton d'accord et je le garde car je suis le suivant dans la file\n");
                close(socketClient);
            }
            else if (varPart->liste->premier != NULL)
            {
                printf("J'envoie un message d'accord au premier site en attente dans ma file \n");
                repondreAUnSite("ACCORD", getSiteFromID(varPart->liste->premier->nombre, varPart->quorum, varPart->tailleQuorum));
                varPart->idSiteEnSectionCritique = varPart->liste->premier->nombre;
                suppressionTete(varPart->liste);
                close(socketClient);
            }
            else
            {
                // dans le cas où la liste d'attente est vide, je récupère mon jeton
                varPart->jetonDaccord = 1;
                // Si j'attendais mon jeton, et que j'ai tous les accords, je vais pouvoir rentrer en SC
                printf("\nListe d'attente vide, je récupère mon jeton d'accord\n");
                close(socketClient);
            }

            afficherListe(varPart->liste);
        }
        else
        {
            perror("\nMessage inconnu");
            printf("Le message en question : %s\n", msg);
            close(socketClient);
            pthread_exit(NULL);
        }
    }
}