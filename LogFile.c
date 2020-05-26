/*
Description:	This modul contain the function which related to the log file
*/
#include "LogFile.h"

/*
Description:	This function open a file 

Arguments:		char Path[]	- a path to file, or filename

Algorihtm:		use fopen and exit() if couldnt open file.

Returns:		the function return a pointer to LogFile. 
*/
FILE *OpenLogFile(char Path[])
{
	FILE *fp;
	fp = fopen(Path, "w");
	if (fp == NULL)
	{
		printf("error with open logfile\n");  //rewrite error msg
		exit(-1);
	}
	return (fp);
}

/*
Description:	This function write msg's from server to client to log file.

Arguments:		FILE *LogFile - A pointer to client's Log file
                char  MSG[]   - Message from server
 
Algorihtm:		concatenate server's message to "Received from server:"
                handle BOARD VIEW msg to be send without the board itself.

Returns:		the function return a pointer to LogFile.
*/
void ReciveFromServer_Log(InfoStruct *Param, char MSG[])
{
	char FullMSG[MAX_STRING_LEN];
	strcpy(FullMSG, "Received from server: "); 
	if (strstr(MSG, "BOARD_VIEW") != NULL) {
		strcat(FullMSG, "BOARD_VIEW\n");
		fprintf(Param->LogFile, FullMSG);
		return;
		//return LogFile;
	}
	strcat(FullMSG, MSG);
	strcat(FullMSG, "\n");
	fprintf(Param->LogFile, FullMSG);
	//return LogFile;
}

/*
Description:	This function write msg's from client to server to log file.

Arguments:		FILE *LogFile - A pointer to client's Log file
				char  MSG[]   - Message to server

Algorihtm:		concatenate client's message to "Send to server:"
	
Returns:		the function return a pointer to LogFile.
*/
void SendToServer_Log(InfoStruct *Param, char MSG[])
{
	char FullMSG[MAX_STRING_LEN];
	strcpy(FullMSG, "Send to server: ");
	strcat(FullMSG, MSG);
	strcat(FullMSG, "\n");
	fprintf(Param->LogFile, FullMSG);
	//return LogFile;
}
