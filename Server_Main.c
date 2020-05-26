/*
Description:	This module contain all the server functions, include it message handler and function that check and execute the messages of a client
*/
#include "Server_Main.h"

int static NumberOfPlayers = 0;
Owner static PlayerTurn = PLAYER_1;
char static PlayersNames[2][MAX_STRING_LEN];
BOOL static GameHasStarted = FALSE;
BOOL static GameEnded = FALSE;

void Server_Main(int ServerPort, char LogFile_Name[]) //need to get argv[2]- lof file, argv[3] - port num
{
	int Ind;
	SOCKET MainSocket = INVALID_SOCKET;
	unsigned long Address;
	SOCKADDR_IN service;
	HANDLE ThreadsArr[NUM_OF_CLIENT];
	WSADATA wsaData;
	FILE *LogFile = NULL;	
	LogFile = OpenLogFile(LogFile_Name); //path = argv[2] !!!!!!!!!!!!!!!!!!!!!!!!!!!

	if (!IntilizeWinsockServer(&wsaData)) {
		exit(-1);
	}
	MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (MainSocket == INVALID_SOCKET) {
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		goto server_cleanup_1;
	}
	Address = inet_addr(SERVER_IP);
	if (Address == INADDR_NONE) {
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
			SERVER_IP);
		goto server_cleanup_2;
	}
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = Address;
	service.sin_port = htons(ServerPort);
	if (!BindSocket(MainSocket, &service)) {
		goto server_cleanup_2;
	}
	for (Ind = 0; Ind < NUM_OF_CLIENT; Ind++) {
		ThreadsArr[Ind] = NULL;
	}
	printf("Waiting for a client to connect...\n");
	WaitForClients(&MainSocket, ThreadsArr, LogFile);

server_cleanup_2:
	if (closesocket(MainSocket) == SOCKET_ERROR)
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
server_cleanup_1:
	if (WSACleanup() == SOCKET_ERROR)
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
}

BOOL IntilizeWinsockServer(WSADATA *wsaData)
{
	int StartupRes;
	StartupRes = WSAStartup(MAKEWORD(2, 2), wsaData);
	if (StartupRes != NO_ERROR) {
		printf("error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());
		// Tell the user that we could not find a usable WinSock DLL.                                  
		return FALSE;
	}
	return TRUE;
}

BOOL BindSocket(SOCKET MainSocket, SOCKADDR_IN *service)
{
	int bindRes, ListenRes;
	bindRes = bind(MainSocket, (SOCKADDR*)service, sizeof(*service));
	if (bindRes == SOCKET_ERROR) {
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		return FALSE;
	}
	ListenRes = listen(MainSocket, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR) {
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		return FALSE;
	}
	return TRUE;
}

int FindFirstUnusedThreadSlot(HANDLE ThreadHandles[])
{
	int i;
	DWORD Res;
	for (i = 0; i < NUM_OF_CLIENT; i++) {
		if (ThreadHandles[i] == NULL)
			break;
		else {
			Res = WaitForSingleObject(ThreadHandles[i], 0);
			if (Res == WAIT_OBJECT_0) {
				CloseHandle(ThreadHandles[i]);
				ThreadHandles[i] = NULL;
				break;
			}
		}
	}
	return i;
}

void WaitForClients(SOCKET *MainSocket, HANDLE ThreadsArr[], FILE *LogFile)
{
	SOCKET AcceptSocket;
	SOCKET ClientSocket[NUM_OF_CLIENT];
	InfoStruct Client[2];
	int Ind;
	Client[0].OwnerNumber = NOT_ACTIVE;
	Client[1].OwnerNumber = NOT_ACTIVE;
	while (1)
	{
		AcceptSocket = accept(*MainSocket, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET)
		{
			printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
			break;
		}
		printf("Client Connected.\n");
		Ind = FindFirstUnusedThreadSlot(ThreadsArr);
		if (Ind == NUM_OF_CLIENT) //no slot is available
		{
			printf("No slots available for client, dropping the connection.\n");
			closesocket(AcceptSocket); //Closing the socket, dropping the connection.
		}
		else
		{
			ClientSocket[Ind] = AcceptSocket; // shallow copy: don't close 
											  // AcceptSocket, instead close 
											  // ThreadInputs[Ind] when the
											  // time comes.
			IntilizeInfoStruct(Client, ClientSocket[Ind], Ind, LogFile);
			ThreadsArr[Ind] = CreateThreadSimple(ServerThreads, Client + Ind);
			if (ThreadsArr[Ind] == NULL) {
				exit(-1);
			}
		}
	}
	//CleanupWorkerThreads(ThreadArr, ClientSocket);
	if (closesocket(*MainSocket) == SOCKET_ERROR)
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
	if (WSACleanup() == SOCKET_ERROR)
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
}

void CleanupWorkerThreads(HANDLE ThreadArr[], SOCKET ClientSocket[])
{
	int Ind;
	DWORD Res;
	for (Ind = 0; Ind < NUM_OF_CLIENT; Ind++) {
		if (ThreadArr[Ind] != NULL) {
			Res = WaitForSingleObject(ThreadArr[Ind], INFINITE);
			if (Res == WAIT_OBJECT_0) {
				closesocket(ClientSocket[Ind]);
				CloseHandle(ThreadArr[Ind]);
				ThreadArr[Ind] = NULL;
				break;
			}
			else {
				printf("Waiting for thread failed. Ending program\n");
				return;
			}
		}
	}
}

void IntilizeInfoStruct(InfoStruct Info[], SOCKET ClientSocket, int Index, FILE *LogFile)
{
	Info[Index].PairSocket = ClientSocket;
	Info[Index].SendBuffer = NULL;
	Info[Index].OwnerNumber = SERVER_NUMBER;
	Info[Index].LogFile = LogFile;
	if (Index == 0) {
		Info[Index].Other = Info + 1;
		NumberOfPlayers = 0;
		PlayerTurn = PLAYER_1;
		strcpy(PlayersNames[0], "");
		strcpy(PlayersNames[1], "");
		GameHasStarted = FALSE;
		GameEnded = FALSE;
		strcpy(Info[Index + 1].NameOfUser, "");
	}
	else {
		Info[Index].Other = Info;
	}
	NumberOfPlayers = Index + 1;
	IntilizeBoard();
}

BOOL ServerMessageHandler(InfoStruct *Param, char *Line)
{
	if (GameEnded) {
		return FALSE;
	}
	if (strstr(Line, "NEW_USER_REQUEST") != NULL) {
		return ServerNewUserRequest(Param, Line);
	}
	else if (strstr(Line, "PLAY_REQUEST:") != NULL) {
		return ServerPlayRequest(Param, Line);//NotFinished
	}
	else if (strstr(Line, "SEND_MESSAGE:") != NULL) {
		return ServerSendMessage(Param, Line);
	}
	return FALSE;
}

BOOL TransferMessageToOtherClient(InfoStruct *Param, char *Line)
{
	if ((Param->Other)->OwnerNumber == NOT_ACTIVE) {
		return TRUE;
	}
	return TransferMessageToBuffer(Param->Other, Line);
}

BOOL ServerNewUserRequest(InfoStruct *Param, char Line[])
{
	//remove \n
	char *pos;
	pos = strstr(Line, ":");
	char CurrentName[MAX_STRING_LEN], OtherName[MAX_STRING_LEN];
	if (pos == NULL) {
		return FALSE;
	}
	pos++;
	strcpy(CurrentName, pos);
	if (NumberOfPlayers == 2) //not good enough condition 
		strcpy(OtherName, (Param->Other)->NameOfUser);
	if (strcmp(CurrentName, OtherName) == 0) {
		return ServerDeclineMessage(Param);
	}
	strcpy(Param->NameOfUser, CurrentName);
	strcpy(PlayersNames[NumberOfPlayers - 1], CurrentName);
	if (!ServerNewUserAccepted(Param)) {
		return FALSE;
	}

	if (NumberOfPlayers == 2) {
		return ServerGameStartMessage(Param, Line);
	}
	return TRUE;
}

BOOL ServerDeclineMessage(InfoStruct *Param)
{
	char Line[] = "NEW_USER_DECLINED";
	Buffer *NewMessage = NULL;
	if (!TransferMessageToBuffer(Param, Line)) {
		printf("An Error occured during the NEW_USER_DECLINED Message\n");
		return FALSE;
	}
	return TRUE;
}

BOOL ServerNewUserAccepted(InfoStruct *Param)
{
	char Line[MAX_STRING_LEN], ApproveMessage[MAX_STRING_LEN], numofplayers_str[2];
	char Number[5];
	strcpy(Line, "NEW_USER_ACCEPTED:");
	_itoa(NumberOfPlayers, numofplayers_str, 10);
	strcat(Line, numofplayers_str);//the number of the player
	return TransferMessageToBuffer(Param, Line);
}

BOOL ServerGameStartMessage(InfoStruct *Param, char Line[])
{
	//char Line[MAX_STRING_LEN];
	char *BoardMessage = NULL;
	BOOL res = TRUE;
	strcpy(Line, "GAME_STARTED");
	GameHasStarted = TRUE;
	if (!TransferMessageToBuffer(Param, Line)) {
		return FALSE;
	}
	if (!TransferMessageToOtherClient(Param, Line)) {
		return FALSE;
	}
	if (!BoardViewMessage(&BoardMessage)) {
		return FALSE;
	}
	if (!TransferMessageToBuffer(Param, BoardMessage) || !TransferMessageToOtherClient(Param, BoardMessage)) {
		res = FALSE;
	}
	PlayerTurn = PLAYER_1;
	free(BoardMessage);
	res = ServerTurnSwitchMessage(Param, Line);
	return res;
}

BOOL ServerPlayRequest(InfoStruct *Param, char Line[])
{
	int Player = 2, move;
	char *BoardMessage = NULL;
	char *pos = strstr(Line, ":");
	if (!GameHasStarted) {
		return ServerInvalidMoveMessage(Param, FALSE);
	}
	if (pos == NULL) {
		return FALSE;
	}
	pos++;
	move = (int)(*pos - '0');
	if (IsPlayerTurn(Param->NameOfUser)!=PlayerTurn) {
		return ServerInvalidMoveMessage(Param, FALSE);
	}
	if (!IsStepValid(move)) {
		return ServerInvalidMoveMessage(Param, TRUE);
	}
	if (!TransferMessageToBuffer(Param, "PLAY_ACCEPTED")) {
		return FALSE;
	}
	if (IsPlayerTurn(Param->NameOfUser) == PLAYER_1)
		Player = 1;
	UpdateBoard(move, Player);
	if (!CheckIfGameEnded(Param, move)) {
		return FALSE;
	}
	if (GameEnded == TRUE) {
		return TRUE;
	}
	if (!BoardViewMessage(&BoardMessage)) {
		return FALSE;
	}
	if (!TransferMessageToBuffer(Param, BoardMessage) || !TransferMessageToOtherClient(Param, BoardMessage)) {
		return FALSE;
	}
	UpdateTurn();
	return ServerTurnSwitchMessage(Param, Line);
}

BOOL ServerInvalidMoveMessage(InfoStruct *Param, BOOL IsTurn)
{
	if (!GameHasStarted) {
		return TransferMessageToBuffer(Param, "PLAY_DECLINED:Game; ;has; ;not; ;started");
	}
	if (!IsTurn) {
		return TransferMessageToBuffer(Param, "PLAY_DECLINED:Not; ;your; ;turn");
	}
	else {
		return TransferMessageToBuffer(Param, "PLAY_DECLINED:Illegal; ;move");
	}
	return FALSE;
}

BOOL ServerSendMessage(InfoStruct *Param, char Line[])
{
	char *pos = NULL;
	char NewMessage[MAX_STRING_LEN];
	pos = strstr(Line, ":");
	pos++;
	strcpy(NewMessage, "RECIEVE_MESSAGE:");
	strcat(NewMessage, Param->NameOfUser);
	strcat(NewMessage, ";");
	strcat(NewMessage, pos);
	return TransferMessageToOtherClient(Param, NewMessage);
}

BOOL ServerTurnSwitchMessage(InfoStruct *Param, char Line[])
{
	if (PlayerTurn == PLAYER_1) {
		strcpy(Line, "TURN_SWITCH:");
		strcat(Line, PlayersNames[0]);
	}
	else {
		strcpy(Line, "TURN_SWITCH:");
		strcat(Line, PlayersNames[1]);
	}
	if (!TransferMessageToBuffer(Param, Line)) {
		return FALSE;
	}
	if (!TransferMessageToOtherClient(Param, Line)) {
		return FALSE;
	}
	return TRUE;
}

void UpdateTurn()
{
	if (PlayerTurn == PLAYER_1) {
		PlayerTurn = PLAYER_2;
	}
	else {
		PlayerTurn = PLAYER_1;
	}
}

BOOL CheckIfGameEnded(InfoStruct *Param, int Column)
{
	if (!ServerCheckIfWinner(Param, Column)) {
		return FALSE;
	}
	if (GameEnded != TRUE) {
		return ServerCheckIfTie(Param);
	}
	return TRUE;


}

BOOL ServerCheckIfWinner(InfoStruct *Param, int Column)
{
	char Line[MAX_STRING_LEN];
	GameEnded = ThereIsAWinner(Column);
	if (GameEnded == TRUE && NumberOfPlayers==2) {
		sprintf(Line, "GAME_ENDED:%s", Param->NameOfUser);
		if (!TransferMessageToBuffer(Param, Line)) {
			return FALSE;
		}
		return TransferMessageToOtherClient(Param, Line);
	}
	else if (GameEnded == TRUE && NumberOfPlayers != 2) {
		return FALSE;
	}
	return TRUE;
}

BOOL ServerCheckIfTie(InfoStruct *Param)
{
	char Line[MAX_STRING_LEN];
	if (CheckIfTie()) {
		GameEnded = TRUE;
		sprintf(Line, "GAME_ENDED:TIE");
		if (!TransferMessageToBuffer(Param, Line)) {
			return FALSE;
		}
		return TransferMessageToOtherClient(Param, Line);
	}
	return TRUE;
}


Owner IsPlayerTurn(char Name[])
{
	if (strcmp(Name, PlayersNames[0]) == 0) {
		return PLAYER_1;
	}
	return PLAYER_2;
}

void EndSession(InfoStruct *Param)
{
	if (Param->OwnerNumber == SERVER_NUMBER)
		ClearParam(Param);
	if ((Param->Other)->OwnerNumber == SERVER_NUMBER && GameHasStarted == TRUE)
		ClearParam(Param->Other);
}

void ClearParam(InfoStruct *Param)
{
	CloseMutesAndSemaphore(Param);
	FreeBuffer(&(Param->SendBuffer));	
	closesocket(Param->PairSocket);
	Param->OwnerNumber = NOT_ACTIVE;
}

