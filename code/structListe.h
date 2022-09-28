#ifndef STRUC_LISTE_H
#define STRUC_LISTE_H

typedef struct Element Element;
struct Element
{
    int nombre;
    Element *suivant;
};

typedef struct Liste Liste;
struct Liste
{
    Element *premier;
};

Liste *initialisationListe();
void insertion(Liste *liste, int nvNombre);
void suppressionTete(Liste *liste);
void afficherListe(Liste *liste);
void insererTete(Liste *liste, int nvNombre);

#endif