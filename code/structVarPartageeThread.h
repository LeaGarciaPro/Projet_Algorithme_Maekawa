#include "structListe.h"

struct site
{
  int idSite;
  int port;
  char *adresseIP;
};

struct variablePartagee
{
  pthread_mutex_t lock;
  pthread_cond_t entreeSC;
  int nbAccords;
};

struct varPartageeThread
{
  int tailleQuorum;
  struct site *quorum;
  int jetonDaccord;
  unsigned long long int prioCurrentProcessus;
  unsigned long long int maDemande;
  Liste *liste;
  int idSiteEnSectionCritique;
  int port;
  struct variablePartagee v;
  int monId;
};

struct params
{
  int socket;
  struct varPartageeThread *varPartagee;
};

struct site getSiteFromID(int id, struct site *tab, int taille);
void repondreAUnSite(char message[100], struct site site);
void calculV2(int a);