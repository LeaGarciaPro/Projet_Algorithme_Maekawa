#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "structListe.h"

Liste *initialisationListe()
{
    Liste *liste = malloc(sizeof(*liste));
    Element *element = malloc(sizeof(*element));

    if (liste == NULL || element == NULL)
    {
        exit(EXIT_FAILURE);
    }

    liste->premier = NULL;

    return liste;
}

void insertion(Liste *liste, int nvNombre)
{
    /* Création du nouvel en fin de liste   élément */
    Element *nouveau = malloc(sizeof(*nouveau));
    if (liste == NULL || nouveau == NULL)
    {
        exit(EXIT_FAILURE);
    }
    nouveau->nombre = nvNombre;
    nouveau->suivant = NULL;
    /* Insertion de l'élément au début de la liste */

    if (liste->premier == NULL)
    {
        liste->premier = nouveau;
    }
    else
    {
        Element *lastNode = liste->premier;

        while (lastNode->suivant != NULL)
        {
            lastNode = lastNode->suivant;
            printf("%d \n", lastNode->nombre);
        }

        lastNode->suivant = nouveau;
    }
}

void suppressionTete(Liste *liste)
{
    if (liste == NULL)
    {
        exit(EXIT_FAILURE);
    }

    if (liste->premier != NULL)
    {
        Element *aSupprimer = liste->premier;
        liste->premier = liste->premier->suivant;
        free(aSupprimer);
    }
}

void afficherListe(Liste *liste)
{
    if (liste == NULL)
    {
        exit(EXIT_FAILURE);
    }

    Element *actuel = liste->premier;

    while (actuel != NULL)
    {
        printf("%d -> ", actuel->nombre);
        actuel = actuel->suivant;
    }
    printf("NULL\n");
}

void insererTete(Liste *liste, int nvNombre)
{
    Element *nouveau = malloc(sizeof(*nouveau));
    if (liste == NULL || nouveau == NULL)
    {
        exit(EXIT_FAILURE);
    }
    nouveau->nombre = nvNombre;
    Element *deuxieme = liste->premier;
    nouveau->suivant = deuxieme;
    liste->premier = nouveau;

}