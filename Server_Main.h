#ifndef SERVER_MAIN_H
#define SERVER_MAIN_H

#include "Threads.h"
#include "LogFile.h"
#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define NUM_OF_CLIENT 2




void Server_Main(int ServerPort,char LogFile_Name[]);
BOOL IntilizeWinsockServer(WSADATA *wsaData);
BOOL BindSocket(SOCKET MainSocket, SOCKADDR_IN *service);
int FindFirstUnusedThreadSlot(HANDLE ThreadHandles[]);
void WaitForClients(SOCKET *MainSocket, HANDLE ThreadsArr[], FILE *LogFile);
void CleanupWorkerThreads(HANDLE ThreadArr[], SOCKET ClientSocket[]);
void IntilizeInfoStruct(InfoStruct Info[], SOCKET ClientSocket, int Index, FILE *LogFile);

BOOL ServerMessageHandler(InfoStruct *Param, char *Line);
BOOL TransferMessageToOtherClient(InfoStruct *Param, char *Line);;
BOOL ServerNewUserRequest(InfoStruct *Param, char Line[]);
BOOL ServerDeclineMessage(InfoStruct *Param);
BOOL ServerNewUserAccepted(InfoStruct *Param);
BOOL ServerGameStartMessage(InfoStruct *Param, char Line[]);
BOOL ServerPlayRequest(InfoStruct *Param, char Line[]);
BOOL ServerInvalidMoveMessage(InfoStruct *Param, BOOL IsTurn);
BOOL ServerSendMessage(InfoStruct *Param, char Line[]);
BOOL ServerTurnSwitchMessage(InfoStruct *Param, char Line[]);
void UpdateTurn();
BOOL ServerCheckIfWinner(InfoStruct *Param, int Column);
BOOL ServerCheckIfTie(InfoStruct *Param);
Owner IsPlayerTurn(char Name[]);


void ClearParam(InfoStruct *Param);
void EndSession(InfoStruct *Param);

#endif // !SERVER_MAIN_H