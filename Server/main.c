/* -*- Mode: C; -*- */
/*
 * main.c
 * Copyleft (C) 2012 Chouaib BOUKHRIS <chouaibboukhris@gmail.com>
 *                   Lamia   LAHMAM   <lahmam_lamia@hotmail.com>
 *                   Amine   LABRIJI  <labriji73@hotmail.com>
 *
 * test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */




#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "server.h"
#include "client.h"


/*==============================================================================================================*/
/* Initialisation de la structure WSADATA dans le cas de Windows*/
static void init(void) {
    #ifdef WIN32
        WSADATA wsa;
        int err = WSAStartup(MAKEWORD(2, 2), &wsa);
        if(err < 0) {
            puts("WSAStartup failed !");
            exit(EXIT_FAILURE);
        }
    #endif
}
/*==============================================================================================================*/



/*==============================================================================================================*/
/* Décharger ws2_32.dll de la mémoire si OS = Windows */
static void end(void) {
    #ifdef WIN32
        WSACleanup();
    #endif
}
/*==============================================================================================================*/



/*==============================================================================================================*/
/* Ecriture dans fichier log */
static void write_log(char *Buffer) {

    FILE *f_Log = fopen("chatRoom.log", "a+b");

    char Date[256];
    time_t timestamp = time(NULL);



    fputs("/*==============================================================================================================*/\n\n"
            , f_Log);

    fputs("\t Date: ", f_Log);

    /* Préparer la variable Date  */
    strftime(Date, sizeof(Date), "%A %d %B %Y - %X.\n",
             localtime(&timestamp));

    fputs(Date, f_Log);
    fputs(Buffer, f_Log);
    fputs("\n\n", f_Log);

    fclose(f_Log);
}
/*==============================================================================================================*/



/*==============================================================================================================*/
/* Fonction Principale du traitement du Serveur */
static void chat_Room(int PORT) {

    int res;


    /* Socket et contexte d'adressage du serveur */
    SOCKET sock = init_connection(PORT);
    char Buffer[BUF_SIZE];

    /* l'indice du tableau */
    int actual = 0;
    int max = sock;

    /* Une matrice de tous les clients */
    Client clients[MAX_CLIENTS];

    /* Création de l'ensemble de lecture */
    fd_set readfs;

    /* Log */
    printf("Patientez pendant que les client se connecte sur le port %d ...\n\n", PORT);
    printf("Pour Arrêté le serveur appuyer sur [Entrée] ...\n");
    strcpy (Buffer, "Patientez pendant que les clients se connecte sur le port ");
    char tmp[256];
    sprintf(tmp,"%d\n",PORT);
    strcat(Buffer, tmp);
    write_log(Buffer);
    bzero((char *) &Buffer, sizeof(Buffer));


    while(1) {

        int i = 0;

        /**
        * On vide l'ensemble de lecture
        * et on lui ajoute la socket serveur
        **/
        FD_ZERO(&readfs);

        /* Ajouter STDIN_FILENO */
        FD_SET(STDIN_FILENO, &readfs);

        /* Ajouter les connections sockets */
        FD_SET(sock, &readfs);

        /* ajouter une socket pour chaque client */
        for(i = 0; i < actual; i++) {
            FD_SET(clients[i].sock, &readfs);
        }

        if(select(max + 1, &readfs, NULL, NULL, NULL) == -1) {
            perror("select()");
            exit(errno);
        }

        /* Si y a quelque chose dans l'entrée standard : Clavier */
        if(FD_ISSET(STDIN_FILENO, &readfs)) {
            /* Arrêter le processus quand on tape sur le clavier */
            sprintf(Buffer,"Serveur arrêté\n");
            write_log(Buffer);
            break;
        }
        else if(FD_ISSET(sock, &readfs)) {

            /* Nouveau client */
            SOCKADDR_IN csin = { 0 };
            size_t sinsize = sizeof(csin);

            int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);

            /* Garder une trace de chaque client */
            sprintf(Buffer,"Un client se connecte avec la socket %d depuis %s:%d\n",
                            csock, inet_ntoa(csin.sin_addr), htons(csin.sin_port));
            write_log(Buffer);


            /* Vider le Buffer */
            bzero((char *) &Buffer, sizeof(Buffer));

            if(csock == SOCKET_ERROR) {
                perror("accept()");
                continue;
            }


            /* Après la connexion, le client envoie son nickName */
            do{
                bzero((char *) &Buffer, sizeof(Buffer));

                if(read_client(csock, Buffer) != -1){
                    Client client;
                    client.sock = csock;
                    strcpy (client.nickName, Buffer);
                    /* Verifier la validité du nickName */
                    res = chat_join(client, actual);
                }
                else {
                    res = -1;
                    break;
                }
            }while(res != 0);

            /* Si le message n'a pas eté ressu corréctement */
            if(res == -1) {
                /* Déconnecté */
                continue;
            }


            /* Si connection accépté */

            /* Quelle est la nouvelle valeur maximale de fd? */
            max = csock > max ? csock : max;

            /* Ajouter les connections sockets Client*/
            FD_SET(csock, &readfs);

            Client c = { csock };
            strncpy(c.nickName, Buffer, BUF_SIZE - 1);
            clients[actual] = c;

            /* Incrémenté l'indice du tableau d'un pas */
            actual++;

            /*
            * Envoyer une notification a tout les clients pour les
            * informer de l'arriver d'un nouveau chateur
            */
            strncpy(Buffer, "\033[4;32m", BUF_SIZE - 1);
            strncat(Buffer, c.nickName, BUF_SIZE - strlen(Buffer) - 1);
            strncat(Buffer, " s'est connecté !\033[0m", BUF_SIZE - strlen(Buffer) - 1);

            /* Forwarder le message a tout le monde */
            chat(clients, c, actual, Buffer, 1);

        }
        else {
            int i = 0;
            for(i = 0; i < actual; i++) {
                /* Un client parle */
                if(FD_ISSET(clients[i].sock, &readfs)) {

                    Client client = clients[i];

                    int c = read_client(clients[i].sock, Buffer);

                    /* La déconnexion du client */
                    if(c == 0) {
                        /* Garder une trace de chaque client qui se déconnecte */
                        sprintf(Buffer,"Un client s'est déconnecté avec la socket %d\n",
                            clients[i].sock);
                        write_log(Buffer);


                        closesocket(clients[i].sock);
                        chat_leave(clients, i, &actual);
                        strncpy(Buffer, "\033[4;31m", BUF_SIZE - 1);
                        strncat(Buffer, client.nickName, BUF_SIZE - strlen(Buffer) - 1);
                        strncat(Buffer, " s'est déconnecté !\033[0m", BUF_SIZE - strlen(Buffer) - 1);

                        /* Forwarder le message a tout le monde */
                        chat(clients, client, actual, Buffer, 1);



                    }
                    else {
                        chat(clients, client, actual, Buffer, 0);
                    }
                    break;
                }
            }
        }
    }
    system("rm clients.bin");
    clear_clients(clients, actual);
    end_connection(sock);
}
/*==============================================================================================================*/



/*==============================================================================================================*/
/* Fonction qui ferme les sockets Client pour términer la session serveur correctement */
static void clear_clients(Client *clients, int actual) {
    int i = 0;

    for(i = 0; i < actual; i++) {
        closesocket(clients[i].sock);
    }
}
/*==============================================================================================================*/



/*==============================================================================================================*/
/* Enlever un client du Tableau Client */
static void chat_leave(Client *clients, int to_remove, int *actual) {

    /* Ouvrire le fichier client en effacant son ancien contenu */
    FILE *f_Client = fopen("clients.bin", "w+b");

    /**
    * void *memmove (void *dest, const void *src, size_t n);
    *
    * La fonction memmove() copie n octets de la zone de mémoire
    * src vers la zone dest. Les deux zones peuvent se chevaucher.
    **/
    /* On enlève le client dans le tableau */
    memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));

    /* Nombre de Clients - 1 */
    (*actual)--;

    /* Mise ajour de la liste des nickName dans le fichier clients */
    fwrite(clients, (*actual) * sizeof(Client), 1, f_Client);

    fclose(f_Client);

}
/*==============================================================================================================*/



/*==============================================================================================================*/
/* Envoyer un message a tout le monde sauf l'expiditeur */
static void chat(Client *clients, Client sender, int actual, const char *Buffer, int from_server) {

    int i = 0;
    char message[BUF_SIZE];


    for(i = 0; i < actual; i++) {
        /* Vider le Buffer */
        bzero((char *) &message, sizeof(message));
        message[0] = 0;

        /* En envoie pas de message a l'expiditeur */
        if(sender.sock != clients[i].sock) {

            if(from_server == 0) {

                sprintf(message, "\033[01;3%dm",(rand() % 6) + 1);
                strncat(message, sender.nickName, sizeof message - strlen(message) - 1);
                strncat(message, " :\033[0m \033[01;37m", sizeof message - strlen(message) - 1);
            }

            strncat(message, Buffer, sizeof message - strlen(message) - 1);
            strncat(message, "\033[0m ", sizeof message - strlen(message) - 1);
            write_client(clients[i].sock, message);
        }
    }
}
/*==============================================================================================================*/



/*==============================================================================================================*/
/* Initialisé la connection de la socket serveur */
static int init_connection(int PORT) {

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = { 0 };
    char Buffer[BUF_SIZE];


    if(sock == INVALID_SOCKET) {
        perror("socket()");
        exit(errno);
    }
    else {
        sprintf(Buffer, "La socket %d est maintenant ouverte en mode TCP/IP\n", sock);
        write_log(Buffer);
    }


    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR) {
        perror("bind()");
        exit(errno);
    }

    if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR) {
        perror("listen()");
        exit(errno);
    }
    else {
        sprintf(Buffer, "Listage du port %d...\n", PORT);
        write_log(Buffer);
    }

    return sock;
}
/*==============================================================================================================*/



/*==============================================================================================================*/
/* Fermeture de la socket Serveur */
static void end_connection(int sock) {
   closesocket(sock);
}
/*==============================================================================================================*/



/*==============================================================================================================*/
/* Fonction de Lecture depuis la socket Client */
static int read_client(SOCKET sock, char *Buffer) {

    int n = 0;

    if((n = recv(sock, Buffer, BUF_SIZE - 1, 0)) < 0) {
        perror("recv()");
        /* En cas d'erreur recv déconnecter le client */
        n = 0;
    }

    Buffer[n] = 0;

    return n;
}
/*==============================================================================================================*/



/*==============================================================================================================*/
/* Fonction d'ecriture sur la socket Client */
static void write_client(SOCKET sock, const char *Buffer) {

    if(send(sock, Buffer, strlen(Buffer), 0) < 0) {
        perror("send() !!!!!");
        exit(errno);
    }
}
/*==============================================================================================================*/



/*======================================================================================================================================*/
/* Fonction qui verifie si un nickname existe ou non dans notre BDD */
static int check_nickname (Client client) {

    FILE *f_Client = fopen("clients.bin", "a+b");

    Client tmp_Client;

    int Bool = 0;
    int f_Curs = 0;
    long f_Pos;

    /* On récupére la valeur de la fin du fichier */
    fseek(f_Client, 0, SEEK_END);
    f_Pos = ftell(f_Client);
    /* Retour au debut du fichier */
    rewind(f_Client);



    /* Lire le ficier binaire jusqu'a la fin */
    while(f_Pos != ftell(f_Client)){

        fread(&tmp_Client, sizeof(Client), 1, f_Client);

        if(tmp_Client.nickName != NULL){
            if(strcmp(tmp_Client.nickName, client.nickName) == 0){
                Bool = -1;
                break;
            }
        }

        /* Retour au debut du fichier */
        rewind(f_Client);
        /* Deplacement de N Client en avant */
        fseek(f_Client, f_Curs * sizeof(Client), SEEK_CUR);

        ++f_Curs;

    }

    if(Bool == 0){
        fwrite(&client, sizeof(Client), 1, f_Client);
    }

    fclose(f_Client);
    return Bool;
}
/*======================================================================================================================================*/




/*======================================================================================================================================*/
/**
* La procédure chat_join prend en parametres le User.
* Elle consulte ensuite la liste des chatteurs en ligne qui est enregistrée dans un fichier.
* Deux cas sont envisageables, si le nickname existe dans cette liste le serveur envoie au client
* une invitation à saisir un autre nickname sinon l’application enregistre le nouveau chatteur en
* ajoutant son nickname à la fin du fichier et en lui envoyant un message de bienvenue dans le salon
* de chat et la liste des chatteurs en ligne. Dans les deux cas le serveur envoie la réponse vers la
* socket du client .L’adresse du client est récupérée à partir de sa socket d’émission.
**/
static int chat_join(Client client, int actual) {

    /* Buffer de communication */
	char Buffer[BUF_SIZE];


    if(check_nickname(client) == 0) {

        //write_client(client.sock, Buffer);

        chat_list(client, actual);

        return 0;
    }

    else {
        sprintf(Buffer, "KO");

        write_client(client.sock, Buffer);
        return -1;
    }
}
/*======================================================================================================================================*/



/*======================================================================================================================================*/
/* Envoie La liste des Clients connecter  */
static void chat_list(Client client, int actual) {

    FILE *f_Client = fopen("clients.bin", "rb");
    int i;

    Client tmp_Client[actual];
    char Buffer[BUF_SIZE] = "";

    fread(&tmp_Client, actual *sizeof(Client), 1, f_Client);

    sprintf(Buffer, "\tBienvenue <span color='lightblue'><i><b>%s</b></i></span> dans le salon de chat ...\n\n\t    Pour Quitter le chat tapper <span color='red'>[EXIT]</span>\n\n",
                client.nickName);

    strncat(Buffer, "<span  color='lightgreen'><b><i><u>La liste des chateurs en ligne :</u></i></b></span>\n\n", sizeof Buffer - strlen(Buffer) - 1);

    if(actual > 0){
        for(i=0; i < actual; i++){
            strncat(Buffer, "<b>", sizeof Buffer - strlen(Buffer) - 1);
            strncat(Buffer, tmp_Client[i].nickName, sizeof Buffer - strlen(Buffer) - 1);
            strncat(Buffer, "</b>\n", sizeof Buffer - strlen(Buffer) - 1);
        }
    }
    else
        strncat(Buffer, "<span  color='red'>Aucun Client n'est en ligne...!</span>\n\n", sizeof Buffer - strlen(Buffer) - 1);



    write_client(client.sock, Buffer);

    fclose(f_Client);
}

/*======================================================================================================================================*/



/*==============================================================================================================*/
/* Fonction Principale Main */
int main(int argc, char **argv) {

    int PORT;
    char Buffer[BUF_SIZE];

    char Date[256];
    time_t timestamp = time(NULL);

    srand(time(NULL));


    printf("Starting MSQL_ChatRoom ( http://github.com/choubisoft/MSQL_ChatRoom ) at ");

    /* Préparer la variable Date  */
    strftime(Date, sizeof(Date), "%A %d %B %Y - %X.\n",
             localtime(&timestamp));
    printf("%s\n\n", Date);

    sprintf(Buffer,"Starting MSQL_ChatRoom ...");
    write_log(Buffer);

    #if defined(linux)
        printf("\033[2J\033[0;0H");
    #elif defined (WIN32)
        clrscr();
    #endif
    /*================================================================================================================================*/
    if (argc < 2 || argc >2) {
        printf("ERREUR, fausse manipulation des arguments\n");

	    printf("\tUsage : %s [port]\n", argv[0]);

        exit(1);
    }
    else {
        PORT = atoi(argv[1]);
    }


   init();

   chat_Room(PORT);

   end();

   return EXIT_SUCCESS;
}
