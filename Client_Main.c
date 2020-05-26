/*
Description:	This module contain all the client function and handlers
*/

#include "Client_Main.h"

static int FirstString = 0;
static BOOL ClientGameHasEnded = FALSE;
static HANDLE hThread[3];

static BOOL CanSendMove = FALSE;
static BOOL IsInFileMode = FALSE;
static FILE *AutomaticFile = NULL;
static BOOL GameHasStart = FALSE;

/*
Description:	This function is the client intilize function

Parameters:		int Port				- the port of the server
				char LogFile_name[]		- the Logfile path and name
				char InputFileName[]	- the input file of inhuman mode path and name

Algorithm:		open LogFile
				if in inhuman mode
					open the input file
					set IsInFileMode to be TRUE
				intilize the winsock of client
				connect to server
				intilize the parameter struct to the threads
				create the thresds
				wait for at least one of them to end
				terminate all threads and close the client
				


Retrun:			this function return TRUE if the intilization happend withou problems, otherwise return FALSE
*/
BOOL Client_Main(int Port, char LogFile_name[], char InputFileName[]) 
{
	SOCKADDR_IN clientService;
	InfoStruct Param;	
	SOCKET ClientSocket;
	int i = 0;
	FILE *LogFile = NULL;
	FILE *Check = NULL;
	LogFile = OpenLogFile(LogFile_name);
	if (InputFileName != NULL) {
		Check = fopen(InputFileName, "r");
		if (Check == NULL) {
			printf("Input file does not exist. Exiting program...\n");
			exit(-1);
		}
		AutomaticFile = Check;
		IsInFileMode = TRUE;
	}
	IntilizeWinsockClient(&ClientSocket);
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(SERVER_IP); //Setting the IP address to connect to
	clientService.sin_port = htons(Port); //Setting the port to connect to.
	if (connect(ClientSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
		fprintf(LogFile, "Failed connecting to server on 127.0.0.1:%d. Exiting\n", Port);
		printf("Failed to connect.\n");
		WSACleanup();
		return FALSE;
	}
	else {
		fprintf(LogFile,"Connected to server on 127.0.0.1:%d\n", Port);
	}
	/*
		In this code, two integers are used to keep track of the number of bytes that are sent and received.
		The send and recv functions both return an integer value of the number of bytes sent or received,
		respectively, or an error. Each function also takes the same parameters:
		the active socket, a char buffer, the number of bytes to send or receive, and any flags to use.

	*/
	IntilizeParam(&Param, ClientSocket, LogFile);
	if (!CreateMutexAndSemaphore(&Param)) {
		return FALSE;;
	}
	hThread[0] = CreateThread(
		NULL,
		0,
		RecieveThread,
		&Param,
		0,
		NULL);
	hThread[1] = CreateThread(
		NULL,
		0,
		SendThread,
		&Param,
		0,
		NULL);
	hThread[2] = CreateThread(
		NULL,
		0,
		ClientUserInterface,
		&Param,
		0,
		NULL);

	WaitForMultipleObjects(3, hThread, FALSE, INFINITE);
	TerminateThread(hThread[0], 0x555);
	TerminateThread(hThread[1], 0x555);
	TerminateThread(hThread[2], 0x555);
	fclose(LogFile);
	WSACleanup();
	return TRUE;
}

/*
Description:	This function intilize the client socket

Parameters:		SOCKET *NewSocket	- a pointer to the client socket

*/

void IntilizeWinsockClient(SOCKET *NewSocket)
{
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		printf("Error at WSAStartup()\n");
		exit(-1);
	}
	*NewSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*NewSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		exit(-1);
	}
}

/*
Description:	This function initialize information about client in its InfoStruct

Arguments:		InfoStruct *Param     - a pointer to InfoStruct
                SOCKET SocketWithServe- socket 
				FILE *LogFile         - a pointer to lof file

*/
void IntilizeParam(InfoStruct *Param, SOCKET SocketWithServer, FILE *LogFile)
{
	Param->OwnerNumber = PLAYER_2;
	Param->PairSocket = SocketWithServer;
	Param->SendBuffer = NULL;
	Param->Other = NULL;
	Param->LogFile = LogFile;
	Param->ClientFile = NULL;
}

/*
Description:	This function accept the comand from client and prepare it to sent to server

Arguments:      FILE *LogFile         - a pointer to lof file		
                Buffer **NewMessage   - a pointer to a buffer to the new command 
				Owner OwnerNumber     - owner number from the Infostruct
				BOOL *IsExitMessage   - a pointer to a flag - if exit command hsa recieved

Algorihtm:      afetr accept the command call ClientSendMessageHandler function to handle the command type

Returns:		return TRUE for success and False for fail.
*/
BOOL ClientSendMessage(InfoStruct *Param, BOOL *IsExitMessage)
{
	char MessageString[MAX_STRING_LEN];
	GetString(MessageString, MAX_STRING_LEN - 1);
	if (strcmp(MessageString, "exit") == 0|| ClientGameHasEnded==TRUE) {
		CloseClient(Param);
		*IsExitMessage = TRUE;
		return TRUE;
	}	
	return ClientSendMessageHandler(Param,MessageString);
}


/*
Description:	This function handle messages recieved from server to client

Arguments:      InfoStruct *Param - a pointer to InfoStruct of client
                char *Line - msg from server to client

Algorihtm:      search for type of msg and call appropriate handle functions

Returns:		return TRUE for success and False for fail.
*/
BOOL ClientRecieveMessageHandler(InfoStruct *Param, char *Line)
{
	if (strstr(Line, "NEW_USER_ACCEPTED") != NULL) {
		return UsernameAccepted(Param, Line);
	}
	else if (strstr(Line, "NEW_USER_DECLINED") != NULL) {
		UsernameDeclined();
		return FALSE;
	}
	else if (strstr(Line, "PLAY_DECLINED") != NULL) {
		PlayDeclined(Line);
		return TRUE;
	}
	else if (strstr(Line, "PLAY_ACCEPTED") != NULL) {
		printf("Well played\n");
		return TRUE;
	}
	else if (strstr(Line, "GAME_STARTED") != NULL) {
		printf("Game is on!\n");
		GameHasStart = TRUE;
		return TRUE;
	}
	else if (strstr(Line, "BOARD_VIEW") != NULL) {
		DisplayBoard(Line);
		return TRUE;
	}
	else if (strstr(Line, "TURN_SWITCH") != NULL) {
		ClientTurnSwitch(Param->NameOfUser, Line);
		return TRUE;
	}
	else if (strstr(Line, "RECIEVE_MESSAGE") != NULL) {
		PrintReceiveMessage(Line, Param);
		return TRUE;
	}
	else if (strstr(Line, "GAME_ENDED") != NULL) {
		ClientGameEnded(Line);
		ClientGameHasEnded = TRUE;
		return TRUE;
	}
	else {
		printf("The message is: %s\n", Line);
		return TRUE;
	}
	return FALSE;
}

/*
Description:	This function handle messages to from client to server

Arguments:      InfoStruct *Param		- a infostruct which contain all the data of the client
                char *MessageString		- a string which is the command from client to server

Algorihtm:      search for type of command and call appropriate handle functions

Returns:		return TRUE for success and False for fail.
*/
BOOL ClientSendMessageHandler(InfoStruct *Param, char *MessageString)
{
	int count = 0, i;
	char FullMsg[MAX_STRING_LEN];
	char MessageArr[MAX_STRING_LEN / 2][MAX_STRING_LEN];
	//USERNEAM_REQUEST
	if (FirstString == 0) {
		FirstString = 1;
		return ClientNewUserRequestMessage(Param, MessageString);
	}
	count = split_line(MessageArr, MessageString, " ");
	if (strcmp(MessageArr[0], "play") == 0) {
		return ClientPlayRequest(Param, MessageArr);
	}
	else if (strcmp(MessageArr[0], "message") == 0) {
		return ClientPrepareSendMessage(Param, MessageArr, count);
	} 
	//handle exit
	else {
		printf("Error: Illegal command\n");
		SendToServer_Log(Param, "Error: Illegal command");
		//*NewMessage = NULL;
		return TRUE;
	}
	return FALSE;
}

/*
Description:	This function handle USER NAME ACCEPTED msg from server 

Arguments:      InfoStruct *Param   - a pointer to InfoStruct of client
				char Line[]			- a msg from server

Returns:		return TRUE for success and False for fail.
*/
BOOL UsernameAccepted(InfoStruct *Param, char Line[])
{
	char *pos = NULL;
	if ((pos = strstr(Line, ":")) == NULL) {
		return FALSE;
	}
	pos++;
	if (*pos == '1') {
		Param->OwnerNumber = PLAYER_1;
	}
	else if (*pos == '2') {
		Param->OwnerNumber = PLAYER_2;
	}
	else {
		printf("invalid parameter for \"ACCEPTED_USER_N\"\nExiting program...\n");
		return FALSE;
	}
	return TRUE;
}

/*
Description:	This function handle print of USER NAME DECLINE msg from server

*/
void UsernameDeclined()
{
	FirstString = 0;
	printf("Request to join was refused\n");
}

/*
Description:	This function handle print of PLAY DECLINE msg from server

Arguments:      char Line[] - msg from server

*/
void PlayDeclined(char Line[])
{
	char ErrorMessage[MAX_STRING_LEN];
	TranslateMessageNew(Line, ErrorMessage);
	printf("Error: %s\n", ErrorMessage);
	if (strstr(ErrorMessage, "turn") != NULL) {
		CanSendMove = TRUE;
	}
}

/*
Description:	This function handle the client recieved message

Arguments:      InfoStruct *Param - a pointer to InfoStruct of client
				char *Line - msg from server to client

Algorihtm:      translate the message
				print it
*/

void PrintReceiveMessage(char Line[], InfoStruct *Param)
{
	int count = 0, i;
	char MessageString[MAX_STRING_LEN], MessageData[MAX_STRING_LEN / 2][MAX_STRING_LEN];
	Line = strstr(Line, ":");
	if (Line == NULL) {
		return FALSE;
	}
	Line++;
	count = split_line(MessageData, Line, ";");
	strcpy(MessageString, "");
	for (i = 0; i < count; i++) {
		strcat(MessageString, MessageData[i]);
		if (i == 0) {
			strcat(MessageString, ":");
		}
	}
	printf("%s\n", MessageString);
}

/*
Description:	This function prepare the a message NEW_USER_REQUEST befor sending to server

Arguments:		InfoStruct *Param - a pointer to InfoStruct of client
				char MessageString[]	- a string which contain the message

Returns:		return True for success of creating a new buffer of False to fail.
*/
BOOL ClientNewUserRequestMessage(InfoStruct *Param, char MessageString[])
{
	char FullMsg[MAX_STRING_LEN];
	strcpy(FullMsg, "NEW_USER_REQUEST:");
	strcat(FullMsg, MessageString);
	SendToServer_Log(Param, FullMsg);
	return TransferMessageToBuffer(Param, FullMsg);
}

/*
Description:	This function prepare the msg of PLAY REQUEST befor send to server

Arguments:		InfoStruct *Param					- a pointer to InfoStruct of client
				char MessageArr[][MAX_STRING_LEN]	- an array of strings

Returns:		return True for success of creating a new buffer of False to fail.
*/
BOOL ClientPlayRequest(InfoStruct *Param, char MessageArr[][MAX_STRING_LEN])
{
	char FullMsg[MAX_STRING_LEN];
	strcpy(FullMsg, "PLAY_REQUEST:");
	strcat(FullMsg, MessageArr[1]);
	SendToServer_Log(Param, FullMsg);
	return TransferMessageToBuffer(Param, FullMsg);
}

/*
Description:	This function prepare the a message from one client to onther to be send to server
                at the wanted format:  word; ;word etc'

Arguments:		InfoStruct *Param					- a pointer to InfoStruct of client
				char MessageArr[][MAX_STRING_LEN]	- an array of strings
				int count							- the number of strings in MessageArr

Algorihtm:      concatenate all strings from the MessageArr in the wanted format

Returns:		return True for success of creating a new buffer of False to fail.
*/
BOOL ClientPrepareSendMessage(InfoStruct *Param, char MessageArr[][MAX_STRING_LEN], int count)
{
	char FullMsg[MAX_STRING_LEN];
	int i;
	strcpy(FullMsg, "SEND_MESSAGE:");
	for (i = 1; i < count; i++) {
		strcat(FullMsg, MessageArr[i]);
		if (i != count - 1)
			strcat(FullMsg, "; ;");
	}
	SendToServer_Log(Param, FullMsg);
	return TransferMessageToBuffer(Param, FullMsg);
}

/*
Description:	This function print whos turn to play

Arguments:		char Line[] - a msg from server
				char Name[] - the name of the player

*/
void ClientTurnSwitch(char Name[],char Line[])
{
	char *pos = strstr(Line, ":");
	pos++;
	printf("%s's turn\n", pos);
	if (strcmp(pos, Name) == 0) {
		CanSendMove = TRUE;
	}
}

/*
Description:	This function print the END_GAME message

Arguments:		char Line[] - a msg from server

*/

void ClientGameEnded(char Line[])
{
	char *pos = strstr(Line, ":");
	pos++;
	if (strcmp(pos, "TIE") == 0) {
		printf("Game ended. Evrobidy wins!.\n");
	}
	else {
		printf("Game ended. The winner is %s!\n", pos);
	}
}

/*
Description:	This function is in charge of reading the input file when the client is in file mode

Arguments:		Buffer **HeadList	- a pointer to the command lines linked list
				InfoStruct *Param	- a pointer to InfoStruct of client

Algorithm:		while there are still lines in the file
					get rid of '\n' and '\r' if they occured in the line
					if it is the first line
						make NEW_USER_REQUEST message
					else
						split the line
						if the line is play
							create PLAY_REQUEST message
						if the line is message
							create SEND_MESSAGE
						if the line is exit
							create exit message
						else
							create illegal message
						add the message to the end of the list

*/

BOOL ReadGameFile(Buffer **HeadList, InfoStruct *Param)
{
	char Line[MAX_STRING_LEN + 1], TempLine[MAX_STRING_LEN + 1];
	char StringArr[MAX_STRING_LEN / 2][MAX_STRING_LEN], *pos=NULL;
	Buffer *temp = NULL, *Current = NULL;
	int count = 0;
	while (fgets(Line, MAX_STRING_LEN, AutomaticFile) != NULL) {
		if (Line[strlen(Line) - 1] == '\n')
			Line[strlen(Line) - 1] = '\0';
		if(Line[strlen(Line) - 1] == '\r')
			Line[strlen(Line) - 1] = '\0';
		if (count == 0) {
			strcpy(Param->NameOfUser, Line);
			sprintf(Line, "NEW_USER_REQUEST:%s", Param->NameOfUser);
			count++;
		}
		else {
			count = split_line(StringArr, Line, " ");
			if (strstr(Line, "play") != NULL) {
				strcpy(Line, "PLAY_REQUEST");
				strcat(Line, ":");
				strcat(Line, StringArr[1]);
			}
			else if ((strstr(Line, "message")) != NULL) {
				strcpy(Line, "SEND_MESSAGE:");
				for (int i = 1; i < count; i++) {
					strcat(Line, StringArr[i]);
					if (i != count - 1)
						strcat(Line, "; ;");
				}
			}
			else if (strcmp(Line, "exit") == 0) {

			}
			else {
				strcpy(Line, "Error Illegal Command");
			}
		}
		if (!CreateNewBuffer(&temp, Line)) {
			fprintf(Param->LogFile, "Custom message: An error occured during the creation"
				"of the new buffer member\n. Exiting program...\n");
			fclose(Param->LogFile);
			FreeBuffer(HeadList);
			fclose(AutomaticFile);
			exit(-1);
		}
		Current = *HeadList;
		while (Current != NULL) {
			if (Current->next == NULL)
				break;
			Current = Current->next;
		}
		count = 1;
		if (Current == NULL){
			*HeadList = temp;
		}
		else
			Current->next = temp;
		temp = NULL;
	}
	fclose(AutomaticFile);
	return TRUE;
}

/*
Description:	This function is in charge of reading the File Client mode actions

Arguments:		InfoStruct *Param	- a pointer to InfoStruct of client
				Buffer **HeadList	- a pointer to the head of the command list

Algorithm:		while there are still lines in the list
					if line is exit
						return TRUE
					if the line is a message
						check if game has strated (use bussy wait until it start)
					if the line is a play
						check if the player can make a move(it turn and he didn't make already a move)
						bussy wait until he can play
					if the line is Error
						print the error
						continue to next message
					transfer the message to the send buffer						
					

Returns:		return TRUE if no error occured during the runing of the function, else return FALSE

*/
BOOL AoutomaticUserPlayer(InfoStruct *Param, Buffer **HeadList)
{
	Buffer *Current = NULL;
	while (*HeadList != NULL) {
		Current = *HeadList;
		(*HeadList) = (*HeadList)->next;
		if (strcmp(Current->Line, "exit") == 0)
			return TRUE;
		if (strstr(Current->Line, "SEND_MESSAGE") != NULL) {
			while (GameHasStart == FALSE) {
				//bussy wait
			}
		}
		if (strstr(Current->Line, "PLAY_REQUEST") != NULL) {
			while (CanSendMove == FALSE) {
				//bussywait
			}
			CanSendMove = FALSE;
		}
		if (strcmp(Current->Line, "Error Illegal Command")==0) {
			printf("%s\n", Current->Line);
			free(Current->Line);
			free(Current);
			continue;
		}
		if (!TransferMessageToBuffer(Param, Current->Line)) {
			FreeBuffer(HeadList);
			return FALSE;
		}		
	}
	return TRUE;
}

/*
Description:	this function close the param InfoStruct's Handles and free the buffer list

Arguments:		InfoStruct *Param - a pointer to InfoStruct of client

*/

void CloseClient(InfoStruct *Param)
{
	CloseHandle(Param->SendMutex);
	FreeBuffer(&(Param->SendBuffer));
	CloseHandle(Param->SendFullSemaphore);
	CloseHandle(Param->SendEmptySemaphore);
	closesocket(Param->PairSocket);
}

/*
Description:	This function close all the client threads in order to finish the program
*/

void ClientCloseAllThreads()
{
	TerminateThread(hThread[0], 0);
	TerminateThread(hThread[1], 0);
	TerminateThread(hThread[2], 0);
}
/*
Description:	This function return True if the client is in file mode or FALSE if it is in human mode
*/

BOOL IsFileMode()
{
	return IsInFileMode;
}
/*
Description:	This function return the client input file pointer if he is in File mode
*/

FILE *InputFilePointer()
{
	return AutomaticFile;
}
