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
#include "processusDepart.h"

void seralizationOfArray(struct accueilClient *tab, char *message, int tailleTab, int id)
{
    char str[20];
    sprintf(str, "%d", id);
    strcat(message, str);
    strcat(message, ",");
    for (int i = 0; i < tailleTab; i++)
    {
        strcat(message, tab[i].adresseIP);
        strcat(message, ",");
        sprintf(str, "%d", tab[i].port);
        strcat(message, str);
        strcat(message, ",");
        sprintf(str, "%d", tab[i].idSite);
        strcat(message, str);
        strcat(message, ",");
    }
}

void fonctionDepart(int nbClientAttendus, int port)
{

    int nbRecus = 0;
    int tailleMatrice = (int)sqrt((float)nbClientAttendus); // taille matrice c'est la racine du nombre de sites attendus (faut que ce soit un carré)
    int tailleQuorum = tailleMatrice + tailleMatrice - 2;   // taille quorum (-1 car on renvoie pas le site lui même)

    // Tableau à deux dimensions pour accueillir tous les sites
    struct accueilClient tabDD[tailleMatrice][tailleMatrice];

    int socketServeur = socket(AF_INET, SOCK_STREAM, 0);
    if (socketServeur == -1)
    {
        perror("socket() serveur");
        exit(-1);
    }

    struct sockaddr_in mon_address;
    mon_address.sin_family = AF_INET;
    mon_address.sin_addr.s_addr = INADDR_ANY;
    mon_address.sin_port = htons(port);

    if (bind(socketServeur, (struct sockaddr *)&mon_address, sizeof(mon_address)) == -1)
    {
        perror("bind()");
        close(socketServeur);
        exit(1);
    }

    if (listen(socketServeur, nbClientAttendus) == -1)
    {
        perror("listen()");
        close(socketServeur);
        exit(1);
    }

    struct sockaddr_in adClient;
    socklen_t lgAdClient = sizeof(struct sockaddr_in);

    int cmptLignes = 0;
    int cmptColonnes = 0;
    while (nbRecus != nbClientAttendus)
    {
        printf("Attente d'un client !\n");

        int socketClient = accept(socketServeur, (struct sockaddr *)&adClient, &lgAdClient);
        if (socketClient == -1)
        {
            perror("accept()");
            close(socketServeur);
            exit(1);
        }

        // récupération du numéro de port du client
        int portClient = 0;
        int rec = recv(socketClient, &portClient, sizeof(portClient), 0);
        if (rec == -1)
        {
            perror("recv()");
            close(socketClient);
            close(socketServeur);
            exit(1);
        }

        // récupération de l'adresse ip du client
        char *adresseClient;
        socklen_t len = sizeof(adClient);
        if (getsockname(socketClient, (struct sockaddr *)&adClient, &len) != -1)
            adresseClient = inet_ntoa(adClient.sin_addr);

        printf("Nouveau client ! \nSon IP : %s, son port : %d, sa socket : %d.\n", adresseClient, portClient, socketClient);

        nbRecus++;

        if (cmptColonnes - (tailleMatrice) == 0)
        {
            cmptLignes++;
            cmptColonnes = 0;
        }
        struct accueilClient clientMat;
        clientMat.idSite = nbRecus;
        clientMat.socket = socketClient;
        clientMat.port = portClient;
        clientMat.adresseIP = adresseClient;
        tabDD[cmptLignes][cmptColonnes] = clientMat;
        cmptColonnes++;
    }

    printf("\nLa matrice est remplie : \n");
    for (int i = 0; i < tailleMatrice; i++)
    {
        for (int k = 0; k < tailleMatrice; k++)
        {
            printf("Ligne %d Colonne %d contient le site numéro %d d'IP : %s et de port : %d\n", i, k, tabDD[i][k].idSite, tabDD[i][k].adresseIP, tabDD[i][k].port);
        }
    }

    printf("\nTous les clients sont présents. Création des quorums !\n\n");

    // J'ai tous les clients. Je fais tourner l'algo qui construit les quorums
    // Pour chaque client je construis la liste de son quorum et je lui envoie pour qu'il la stocke et puisse l'utiliser

    // Boucle qui calcule chaque quorum et le place dans tabQuorum
    for (int i = 0; i < tailleMatrice; i++)
    {
        for (int k = 0; k < tailleMatrice; k++)
        {
            struct accueilClient tabQuorum[tailleQuorum];
            int cmpt = 0; // compteur pour remplir le tableau tabQuorum
            for (int ord = 0; ord < tailleMatrice; ord++)
            {
                if (ord != i)
                {
                    tabQuorum[cmpt] = tabDD[ord][k]; // la colonne ne bouge pas je recup toutes les lignes
                    cmpt++;
                }
            }

            for (int abs = 0; abs < tailleMatrice; abs++)
            {
                if (abs != k)
                {
                    tabQuorum[cmpt] = tabDD[i][abs]; // la ligne ne bouge pas je recup toutes les colonnes
                    cmpt++;
                }
            }

            char msg[500] = "";

            struct accueilClient clientEnvoi = tabDD[i][k];

            // envoi de la taille du quorum pour la construction de la liste du côté client
            int snd = send(clientEnvoi.socket, &tailleQuorum, sizeof(tailleQuorum), 0);
            if (snd <= 0)
            {
                perror("send()\n");
                close(clientEnvoi.socket);
                exit(1);
            }

            // serialisation du tableau
            seralizationOfArray(tabQuorum, msg, tailleQuorum, tabDD[i][k].idSite);
            printf("Message qui va être envoyé au site d'id %d :\n %s\n", clientEnvoi.idSite, msg);

            // envoi du quorum
            snd = send(clientEnvoi.socket, msg, sizeof(msg), 0);
            if (snd <= 0)
            {
                perror("send()\n");
                close(clientEnvoi.socket);
                exit(1);
            }

        } // fin boucle k
    }     // fin boucle i

    //fermeture des sockets des clients
    for (int i = 0; i < tailleMatrice; i++)
    {
        for (int k = 0; k < tailleMatrice; k++)
        {
            close(tabDD[i][k].socket);
        }
    }

    //fermeture de la socket principale
    close(socketServeur);

}

int main(int argc, char const *argv[])
{

    if (argc < 3)
    {
        printf("Veuillez entrer : %s 'nombre de clients attendus' 'port utilisé pour ce processus'\n", argv[0]);
        exit(1);
    }

    printf("Je suis le processus de départ !\n");
    printf("Nombre de clients attendus : %s\n", argv[1]);
    printf("Port utilisé par ce processus : %s\n\n", argv[2]);
    fonctionDepart(atoi(argv[1]), atoi(argv[2]));
}