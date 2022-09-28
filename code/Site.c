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
#include "fonctionsClient.h"
#include "fonctionsServeur.h"
#include "structVarPartageeThread.h"

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1

int main(int argc, char const *argv[])
{

    if (argc < 3)
    {
        printf("Veuillez entrer : %s 'ip du serveur' 'port du serveur'\n", argv[0]);
        exit(1);
    }

    int portChoosed = 0;

    int sockServeur = socket(AF_INET, SOCK_STREAM, 0);
    if (sockServeur == -1)
    {
        perror("socket() serveur");
        exit(-1);
    }

    // lancement du serveur pour récuperer le port
    struct sockaddr_in mon_address;
    mon_address.sin_family = AF_INET;
    mon_address.sin_addr.s_addr = 0;
    mon_address.sin_addr.s_addr = INADDR_ANY;
    mon_address.sin_port = 0;

    // nommage de la socket
    if (bind(sockServeur, (struct sockaddr *)&mon_address, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
    {
        perror("bind()");
        pthread_exit(NULL);
    }
    // récupération du port
    socklen_t len = sizeof(mon_address);
    if (getsockname(sockServeur, (struct sockaddr *)&mon_address, &len) != -1)
    {
        portChoosed = ntohs(mon_address.sin_port);
    }

    int sockClient = socket(AF_INET, SOCK_STREAM, 0);
    if (sockClient == -1)
    {
        perror("socket() client");
        exit(-1);
    }

    // connexion au processus de départ pour avoir son quorum
    int socketServeurMere = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adrServ;
    adrServ.sin_addr.s_addr = inet_addr(argv[1]);
    adrServ.sin_family = AF_INET;
    int lgAd = sizeof(struct sockaddr_in);
    adrServ.sin_port = htons(atoi(argv[2]));
    int connexion = connect(socketServeurMere, (struct sockaddr *)&adrServ, lgAd);
    if (connexion == -1)
    {
        perror("connect()");
        pthread_exit(NULL);
    }

    // envoi de son port qui est en écoute
    int snd = send(socketServeurMere, &portChoosed, sizeof(portChoosed), 0);
    if (snd <= 0)
    {
        perror("send()\n");
        pthread_exit(NULL);
    }
    printf("En attente de l'attribution d'un quorum. \n");

    // je reçois la taille de mon quorum
    int tailleQuorum = 0;
    int rec = recv(socketServeurMere, &tailleQuorum, sizeof(tailleQuorum), 0);
    if (rec == -1)
    {
        perror("recv()");
        close(socketServeurMere);
        exit(1);
    }

    // je reçois mon quorum
    char msg[500];
    rec = recv(socketServeurMere, msg, sizeof(msg), 0);
    if (rec == -1)
    {
        perror("recv()");
        close(socketServeurMere);
        exit(1);
    }

    struct site quorum[tailleQuorum];

    char *ptr = strtok(msg, ",");
    int monID = atoi(ptr);
    ptr = strtok(NULL, ",");
    for (int i = 0; i < tailleQuorum; i++)
    {
        quorum[i].adresseIP = ptr;
        ptr = strtok(NULL, ",");
        quorum[i].port = atoi(ptr);
        ptr = strtok(NULL, ",");
        quorum[i].idSite = atoi(ptr);
        ptr = strtok(NULL, ",");
    }

    printf("Je suis le site %d", monID);
    printf("\nMon quorum est composé des sites : ");
    for (int i = 0; i < tailleQuorum - 1; i++)
    {
        printf("%d, ", quorum[i].idSite);
    }
    printf("%d", quorum[tailleQuorum - 1].idSite);

    printf("\n");

    // mise en place de la variable partagée
    struct varPartageeThread varPartagee;
    varPartagee.jetonDaccord = 1;
    varPartagee.prioCurrentProcessus = 0;
    varPartagee.port = portChoosed;
    varPartagee.monId = monID;
    varPartagee.quorum = quorum;
    varPartagee.liste = initialisationListe();
    varPartagee.tailleQuorum = tailleQuorum;
    varPartagee.idSiteEnSectionCritique = 0;
    varPartagee.maDemande = 0;

    // mise en place de la variable conditionnelle et du mutex
    pthread_mutex_init(&(varPartagee.v.lock), NULL);
    pthread_cond_init(&(varPartagee.v.entreeSC), NULL);
    varPartagee.v.nbAccords = 0;

    struct params paramServeur;
    paramServeur.socket = sockServeur;
    paramServeur.varPartagee = &varPartagee;

    struct params paramClient;
    paramClient.socket = sockClient;
    paramClient.varPartagee = &varPartagee;

    pthread_t threadReception;
    pthread_t threadDemande;

    pthread_create(&threadReception, NULL, (void *)fonctionReception, &paramServeur);
    pthread_create(&threadDemande, NULL, (void *)fonctionDemande, &paramClient);

    pthread_join(threadReception, NULL);
    pthread_join(threadDemande, NULL);

    close(socketServeurMere);
    close(sockServeur);
    close(sockClient);

    printf("Fin du programme principal\n");

    return 0;
}