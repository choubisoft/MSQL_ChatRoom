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

#include "client.h"



/*================================================================================================================================*/
/* Initialisation de la connection */
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
/*================================================================================================================================*/



/*======================================================================================================================================*/
/* Décharger ws2_32.dll de la mémoire si OS = Windows */
static void end(void) {
    #ifdef WIN32
        WSACleanup();
    #endif
}
/*======================================================================================================================================*/



/*======================================================================================================================================*/
/* Fonction Principale du traitement du Serveur */
static void chat_Room(const char *address, const char *PORT) {

    SOCKET sock = init_connection(address, PORT);
    char Buffer[BUF_SIZE];

    /* Création de l'ensemble de lecture */
    fd_set readfs;

    int i = 0;
    /* Envoie du nickName */
    do{
        /* Envoie du NikName */

        if(i == 0)
            printf("\033[22;35m Saisissez Votre NickName : \033[0m");
        else if(i < 3){
            #if defined(linux)
                printf("\033[2J\033[0;0H");
            #elif defined (WIN32)
                clrscr();
            #endif
            printf("\033[01;31m NickName déja existant, ressaisissez un nouveau : \033[0m");
        }
        else{
            #if defined(linux)
                printf("\033[2J\033[0;0H");
            #elif defined (WIN32)
                clrscr();
            #endif
            system("zenity --warning --text=\"Al7or belghamza ol3abed bedebza !!!! \"");
            printf("\033[22;31m\n NickName déja existant, ressaisissez un nouveau : \033[0m");
        }

        gets(Buffer);
        /*color("0");*/

        write_server(sock, Buffer);

        int n = read_server(sock, Buffer);
        /* Serveur deconnecté pour une raison ou une autre */
        if(n == 0) {
            system("zenity --error --text=\"Le serveur s'est déconnecté ! \"");
            //printf("\033[22;31m Le serveur s'est déconnecté !\033[0m\n");
            break;
        }
        ++i;
    }while(strcmp(Buffer,"KO") == 0);


    char tmp[BUF_SIZE];
    sprintf(tmp ,"zenity --info --text=\"");

    strncat(tmp, Buffer, BUF_SIZE - strlen(Buffer) - 1);
    strncat(tmp, "\"", BUF_SIZE - strlen(Buffer) - 1);

    system(tmp);
    //printf("%s", Buffer);


    #if defined(linux)
        printf("\033[2J\033[0;0H");
    #elif defined (WIN32)
        clrscr();
    #endif



    while(1) {

      /**
        * On vide l'ensemble de lecture
        * et on lui ajoute la socket serveur
        **/
        FD_ZERO(&readfs);

      /* Ajouter STDIN_FILENO */
        FD_SET(STDIN_FILENO, &readfs);

        /* Ajouter les connections sockets */
        FD_SET(sock, &readfs);

        if(select(sock + 1, &readfs, NULL, NULL, NULL) == -1) {
            perror("select()");
            exit(errno);
        }

        /* something from standard input : i.e keyboard */
        if(FD_ISSET(STDIN_FILENO, &readfs)) {
            fgets(Buffer, BUF_SIZE - 1, stdin);
            {
                char *p = NULL;
                p = strstr(Buffer, "\n");
                if(p != NULL) {
                    *p = 0;
                }
                else {
                    /* fclean */
                    Buffer[BUF_SIZE - 1] = 0;
                }
            }
            if(strcmp(Buffer, "EXIT") == 0)
               goto Fin;

            write_server(sock, Buffer);
        }
        else if(FD_ISSET(sock, &readfs)) {

            int n = read_server(sock, Buffer);
            /* Serveur deconnecté pour une raison ou une autre */
            if(n == 0) {
                system("zenity --warning --text=\"Le serveur s'est déconnecté !\"");
                //printf("\033[22;31m Le serveur s'est déconnecté !\033[0m\n");
                break;
            }
            puts(Buffer);
        }
    }

    Fin:
        end_connection(sock);
}
/*======================================================================================================================================*/



/*======================================================================================================================================*/
/* Initialisé la connection de la socket Client */
static int init_connection(const char *address, const char *PORT) {

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = { 0 };
    struct hostent *hostinfo;


    if(sock == INVALID_SOCKET) {
        perror("socket()");
        exit(errno);
    }


    hostinfo = gethostbyname(address);

    if (hostinfo == NULL) {
        fprintf (stderr, "\033[22;31mHôte inconnu %s...\033[0m\n", address);
        exit(EXIT_FAILURE);
    }

    sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
    sin.sin_port = htons(atoi(PORT));
    sin.sin_family = AF_INET;

    if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        perror("connect()");
        exit(errno);
    }
    else
        printf("\033[22;32mConnexion à %s sur le port %d\033[0m\n\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));

    return sock;
}
/*======================================================================================================================================*/



/*======================================================================================================================================*/
/* Fermeture de la socket Serveur */
static void end_connection(int sock) {
    closesocket(sock);
}
/*======================================================================================================================================*/



/*======================================================================================================================================*/
/* Lire depuis Le serveur */
static int read_server(SOCKET sock, char *Buffer) {

    int n = 0;

    if((n = recv(sock, Buffer, BUF_SIZE - 1, 0)) < 0) {
        perror("recv()");
        exit(errno);
    }

    Buffer[n] = 0;

    return n;
}
/*======================================================================================================================================*/



/*======================================================================================================================================*/
/* Ecrire sur la socket Serveur */
static void write_server(SOCKET sock, const char *Buffer) {

    if(send(sock, Buffer, strlen(Buffer), 0) < 0) {
        perror("send()");
        exit(errno);
    }
}
/*======================================================================================================================================*/




/*======================================================================================================================================*/
/* Fonction Principale Main */
int main(int argc, char **argv) {

    /* On Vérifie si les argument sont Correcte */
    if (argc < 3 || argc >3) {
    	printf("ERREUR, fausse manipulation des arguments\n");

        printf("\tUsage : %s [address] [port]\n", argv[0]);

        return EXIT_FAILURE;
     }


    #if defined(linux)
        printf("\033[2J\033[0;0H");
    #elif defined (WIN32)
        clrscr();
    #endif


    init();

    chat_Room(argv[1], argv[2]);

    end();

    return EXIT_SUCCESS;
}
