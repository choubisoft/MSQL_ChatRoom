#!/bin/bash

# * -*- Mode: bash; -*- */
# *
# * clientexec.sh
# * Copyleft (C) 2012 Chouaib BOUKHRIS <chouaibboukhris@gmail.com>
# *
# * test is free software: you can redistribute it and/or modify it
# * under the terms of the GNU General Public License as published by the
# * Free Software Foundation, either version 3 of the License, or
# * (at your option) any later version.
# *
# * test is distributed in the hope that it will be useful, but
# * WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# * See the GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License along
# * with this program.  If not, see <http://www.gnu.org/licenses/>.




clear

# Test de validité IPv4 de l'adresse entrée (expression régulière)
function isIPv4 {
	if [ $# = 1 ] 
	then
 		printf $1 | grep -Eq '^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-4]|2[0-4][0-9]|[01]?[1-9][0-9]?)$'
 		return $?
	else
 		return 2
	fi
}




#On crée le formulaire en stockant les valeurs de sortie dans $servinfo :/
servinfo=`zenity --forms \
    --title="Chat Room Connection" \
    --text="Saisissez les informations de connexion" \
    --add-entry="Adresse IP" \
    --add-entry="Port" \
    --separator="|"`

#Si on clique sur le bouton Annuler
if [ "$?" -eq 1 ]; then
    #On quitte le script
    exit
fi



#Sinon on continue
#On peut récupérer les valeurs des différents champs de cette façon :
serv_IP=$(printf $servinfo | cut -d "|" -f1)  # IPv4
serv_PORT=$(printf $servinfo | cut -d "|" -f2) # Port


if (isIPv4 $serv_IP)
then 
	if [[ $serv_PORT = +([0-9]) ]] ;
	then
		(
		echo "10" ; sleep 1
		echo "# Vérification de l'adresse IP" ; sleep 1
		echo "20" ; sleep 1
		echo "# Vérification du numéro de PORT" ; sleep 1
		echo "50" ; sleep 1
		echo "# Connexion au Serveur... $serv_IP:$serv_PORT" ; sleep 1
		echo "75" ; sleep 1
		echo "" ; sleep 1
		echo "100" ; sleep 1
		) |
		zenity --width=400 --progress \
  			--title="Lancement du client Chat Room v 1.2" \
 			--text="Analyse des Donnée..." \
  			--percentage=0

		if [ "$?" = -1 ] ; then
  			zenity --error \
    			--text="Connexion annulée."
    			exit -1;
		fi
		
		./client $serv_IP $serv_PORT
	else
		zenity --error --text " Numéro de PORT erroné !"
		exit -1;
	fi
else 
	zenity --error --text "Adresse IP erroné !"
	exit -1;
fi
