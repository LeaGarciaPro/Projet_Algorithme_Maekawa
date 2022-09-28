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
#include <math.h>

#include "calcul.h"
#include "structVarPartageeThread.h"

struct site getSiteFromID(int id, struct site *tab, int taille)
{
    for (int i = 0; i < taille; i++)
    {
        if (tab[i].idSite == id)
        {
            return tab[i];
        }
    }

    // Pour ne pas avoir d'erreur à la compilation, on renvoie une structure d'erreur avec le message d'erreur
    struct site erreur;
    erreur.idSite = -1;
    erreur.port = -1;
    erreur.adresseIP = "-1";
    printf("Aucun site avec cet id n'est présent dans le quorum\n");
    return erreur;
}

// méthode qui permet d'établir une connexion avec la partie serveur d'un site demandeur
void repondreAUnSite(char *message, struct site site)
{

    int socketSite = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adrServ;
    adrServ.sin_addr.s_addr = inet_addr(site.adresseIP);
    adrServ.sin_family = AF_INET;
    int lgAd = sizeof(struct sockaddr_in);
    adrServ.sin_port = htons(site.port);

    int connexion = connect(socketSite, (struct sockaddr *)&adrServ, lgAd);
    if (connexion < 0)
    {
        perror("connexion()\n");
        pthread_exit(NULL);
    }
    int snd = send(socketSite, message, sizeof(message), 0);
    if (snd <= 0)
    {
        perror("send()\n");
        pthread_exit(NULL);
    }
    close(socketSite);
}

void calculV2(int a)
{

    for (int i = 0; i < a; i++)
    {
        printf(".\n");
        calcul(1);
    }
}