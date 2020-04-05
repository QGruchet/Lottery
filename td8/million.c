#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include "se_fichier.h"
#include "se_fichier.c"

#define SIZE_TAB_ANSWER 5
#define CLIENT_WIN 1
#define FALSE_COMBINAISON 0
#define LOTTERY_CLOSE 2


//////////////////////STRUCTURE DE DONNEES///////////////////////////
typedef struct client{
	int* client_nombre; //tableaux contenant les nombres choisi de l'user
	int client_ID;	//contient l'identifiants de l'user
} client_info;

typedef struct serveur{
	int status; //(1 si ouvert 0 sinon)
} serveur_info;

int million_client(client_info client);

///////////////////LECTURE DU FICHIER REPONSE////////////////////////// 

//Stocke les numeros gagnant dans un tableau
int* lire_numero_gagnant(char* name_file){
	SE_FICHIER file;
	int* nombre_gagnant = malloc(sizeof(int) * 5);
	if(nombre_gagnant == NULL){
		printf("erreur alloc\n");
		exit( EXIT_FAILURE );
	}
	int tmp;

	//On ouvrir le fichier en argument ici lottery-test.cfg
	file = SE_ouverture(name_file, O_RDONLY);
	if(file.descripteur == -1){
		printf("Erreur lors de l'ouverture du fichier%s\n", name_file);
	}
	//On lit la premiere ligne qui ne nous interesse pas on le place dans un variable tmp qui ne sera pas utiliser apres
	SE_lectureEntier(file, &tmp);

	//On lit la deuxieme ligne (qui nous interesse) et le stocke dans le tab nombres gagnants 
	for (int i = 0; i < 5; ++i)
	{
		SE_lectureEntier(file, &nombre_gagnant[i]);
		//printf("%d\n", nombre_gagnant[i]);   //test d'affichage des nombres gagnant
	}

	SE_fermeture(file);

	//free(nombre_gagnant);

	return nombre_gagnant;
}

//Stocke les gains ainsi que le nombres de nombres pour les gagner dans un tableau
int* gain(){
	SE_FICHIER file;
	int* gain = malloc(sizeof(int) * 8);
	if(gain == NULL) printf("erreur alloc\n");
	int* tmp = malloc(sizeof(int) * 5);
	if(tmp == NULL) printf("erreur alloc\n");

	file = SE_ouverture("lottery-test.cfg", O_RDONLY);
	//On lit la 1er et 2e ligne qui nous interesse pas on le stocke dans un tmp non utilise apres
	for(int i = 0; i < 6; ++i){
		SE_lectureEntier(file, &tmp[i]);
	}
	//on lit le reste du fichier càd le nombre de chiffre correct pour remporter des sous puis le montant de ces gains
	for (int i = 0; i < 9; ++i)
	{
		SE_lectureEntier(file, &gain[i]);
		//printf("%d\n", gain[i]);
	}

	SE_fermeture(file);

	free(tmp);

	return gain;
}

//Permet de verifier combien de nombres gagnant le clients a et retourne ce nombre
int win(int* nombre_gagnant, int* client_nombre){
	int compt = 0;

	for(int i = 0; i < 5; ++i){
		if(nombre_gagnant[i] == client_nombre[i]){
			compt++;
		}
	}
	//printf("compt f = %d\n", compt);
	return compt;
}

/////////////////////SERVEUR////////////////////////////
void envoi_gain(int* reponse_lottery, int* reponse_client){
	int fd_sent;
	int* gain_lottery = malloc(sizeof(int) * 5);
	if(gain_lottery == NULL) printf("erreur alloc\n");
	gain_lottery = gain();
	int compt;
	compt = win(reponse_lottery, reponse_client);

	unlink("/tmp/tube_gain");
	if(mkfifo("/tmp/tube_gain", 0666) == -1){
		printf("Erreur envoi des gains : %s\n", strerror(errno));
	}

	if((fd_sent = open("/tmp/tube_gain", O_WRONLY)) == -1){
		printf("Erreur envoi des gains: %s\n", strerror(errno));
	}

	int pos;
	 switch(compt) {
	 	case 5:
	 		pos = 1;
		 	break;
		case 4:
			pos = 3;
		 	break;
		case 3:
			pos = 5;
		 	break;
		case 2:
			pos = 7;
		 	break;
		case 1:
			pos = 9;
		 	break;
		default:
			exit(-1);
	 }

	write(fd_sent, &gain_lottery[pos], sizeof(int));
 	close(fd_sent);

	unlink("/tmp/tube_gain");
}

void million_serveur(char *name_file){
	int fd_serv;
	int fin;
	int* reponse_client = malloc(sizeof(int) * 5);
	if(reponse_client == NULL) printf("erreur alloc\n");

	int* reponse_lottery = malloc(sizeof(int) * 5);
	if(reponse_lottery == NULL) printf("erreur alloc\n");
	serveur_info serveur;


	reponse_lottery = lire_numero_gagnant(name_file); 
	serveur.status = 1;
	if(mkfifo("/tmp/tube_reponse", 0666) == -1){
		printf("Erreur 1 dans serveur: %s\n", strerror(errno));
		}

	/** if( var ) <=> var == 1
	  *	if( !var ) <=> var == 0
	  */
	while(serveur.status){ 
		//On creer le tube permettant d'envoyer les reponses
		

		//On ouvre le tube permettant d'acceder au reponse
		if((fd_serv = open("/tmp/tube_reponse", O_RDONLY)) == -1){
			printf("Erreur 2 dans serveur: %s\n", strerror(errno));
		}
		//On lit et affiche ces reponse
		for (int i = 0; i < 5; ++i)
		{	
			read(fd_serv, &reponse_client[i], sizeof(int));
		}
		for(int i = 0; i < 5; i++){
			printf("%de nombre recu : %d\n", i+1, reponse_client[i]);
		}
		close(fd_serv);
		
		sleep(1);
		
		envoi_gain(reponse_lottery, reponse_client);

		fin = win(reponse_lottery, reponse_client);


		
		if(fin == 5){
			serveur.status = 0;
		}
	}
	unlink("/tmp/tube_reponse");
}	

//////////////////////CLIENT///////////////////////////
void recevoir_gain(){
	int fd_receive;
	int gain;
	sleep(1);
	if((fd_receive = open("/tmp/tube_gain", O_RDONLY)) == -1){
		printf("Erreur reception des gains: %s\n", strerror(errno));
	}
	else{
		read(fd_receive, &gain, sizeof(int));
		printf("Vous avez gagné %d €\n", gain);
	}

	close(fd_receive);
	unlink("/tmp/tube_gain");

}

int million_client(client_info client){
	int fd_client;
	//Puis on l'ouvre
	sleep(1);
	
	if((fd_client = open("/tmp/tube_reponse", O_WRONLY)) == -1){
		printf("Erreur dans client %s\n", strerror(errno));
	}

	//On ecrit dans le tube, les reponse de l'user
	for (int i = 0; i < 5; ++i){

		write(fd_client, &client.client_nombre[i], sizeof(int));
	}

	close(fd_client);

	sleep(1);
	recevoir_gain();

	return 0;

}


/////////////////////////INT MAIN////////////////////////////
int main(int argc, char* argv[]){

	//En fonction de l'argument donné, on biffurque ou sur serveur ou sur client
	if(strcmp(argv[1], "server") == 0){
		char* name_file = argv[2];
		printf("Initialisation du serveur\n");
		million_serveur(name_file);
	}

	if(strcmp(argv[1], "client") == 0){
		client_info client;
		printf("Initialisation du client \n");
		client.client_nombre = malloc(sizeof(int) * 5);
		if(client.client_nombre == NULL) printf("Erreur alloc\n");
		//Permet de lire les nombres donne en arguments
		for (int i = 2; i < argc; ++i)
		{
			client.client_nombre[i-2] = atoi(argv[i]);
		}
		million_client(client);
	}

	return 0;
}