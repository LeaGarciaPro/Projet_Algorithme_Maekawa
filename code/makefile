all: proc site clean

site: fonctionsClient.o fonctionsServeur.o site.o structListe.o structVarPartageeThread.o
		gcc -o site fonctionsClient.o fonctionsServeur.o site.o structListe.o structVarPartageeThread.o calculC.o -lpthread

proc: processusDepart.o
		gcc -o proc processusDepart.o -lm

fonctionsClient.o: fonctionsClient.c fonctionsClient.h
		gcc -o fonctionsClient.o -c fonctionsClient.c -Wall

fonctionsServeur.o: fonctionsServeur.c fonctionsServeur.h
		gcc -o fonctionsServeur.o -c fonctionsServeur.c -Wall

site.o: site.c
		gcc -o site.o -c site.c -Wall

structListe.o: structListe.c structListe.h
		gcc -o structListe.o -c structListe.c -Wall

processusDepart.o: processusDepart.c processusDepart.h
		gcc -o processusDepart.o -c processusDepart.c -Wall

structVarPartageeThread.o: structVarPartageeThread.c structVarPartageeThread.h
		gcc -o structVarPartageeThread.o -c structVarPartageeThread.c -Wall

clean:
	rm -f fonctionsClient.o fonctionsServeur.o site.o structListe.o processusDepart.o structVarPartageeThread.o