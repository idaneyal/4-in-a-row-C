#ifndef CLIENT_MAIN_H
#define CLIENT_MAIN_H

#include "Threads.h"
#include "LogFile.h"
#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define NUM_OF_CLIENT 2



BOOL Client_Main(int Port, char LogFile_name[], char InputFileName[]);
void IntilizeWinsockClient(SOCKET *NewSocket);
void IntilizeParam(InfoStruct *Param, SOCKET SocketWithServer, FILE *LogFile);
BOOL ClientSendMessage(InfoStruct *Param, BOOL *IsExitMessage);
BOOL ClientRecieveMessageHandler(InfoStruct *Param, char *Line);
BOOL ClientSendMessageHandler(InfoStruct *Param, char *MessageString);
void PrintReceiveMessage(char Line[], InfoStruct *Param);
BOOL UsernameAccepted(InfoStruct *Param, char Line[]);
void UsernameDeclined();
void PlayDeclined(char Line[]);
void ClientTurnSwitch(char Name[], char Line[]);
BOOL ClientNewUserRequestMessage(InfoStruct *Param, char MessageString[]);
BOOL ClientPrepareSendMessage(InfoStruct *Param, char MessageArr[][MAX_STRING_LEN], int count);
BOOL ClientPlayRequest(InfoStruct *Param, char MessageArr[][MAX_STRING_LEN]);
BOOL ReadGameFile(Buffer **HeadList, InfoStruct *Param);
void ClientGameEnded(char Line[]);
void ClientCloseAllThreads();
void CloseClient(InfoStruct *Param);
BOOL IsFileMode();
BOOL AoutomaticUserPlayer(InfoStruct *Param, Buffer **HeadList);
#endif // !CLIENT_MAIN_H
