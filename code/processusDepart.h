struct accueilClient{
    int idSite; //désigner chaque site de manière unique
    int socket; //socket récupérée avec l'accept qui crée la nouvelle socket connectée à chaque client, sert de communication avec le client
    char* adresseIP;
    int port;
};

void seralizationOfArray(struct accueilClient* tab, char* message, int tailleTab, int id);
void fonctionDepart(int nbClientAttendus, int port);
int main(int argc, char const *argv[]);