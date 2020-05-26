/*
Authors:	Idan Eyal		
			Liran Mazliach	

Project:	This project simulate a 4 in a row game with a server and two clients

Decription:	This modul choose if the game will run at server or client mod
*/
#include "Server_Main.h"
#include "Client_Main.h"
#include "Game.h"

int main(int argc, char *argv[])
{
	int Port = atoi(argv[3]);
	if (strcmp(argv[1], "server") == 0) {
		Server_Main(Port, argv[2]);
	}
	else {
		if (argc == 6) {
			Client_Main(Port, argv[2], argv[5]);
		}
		else {
			Client_Main(Port, argv[2], NULL);
		}
	}
	return 0;
}