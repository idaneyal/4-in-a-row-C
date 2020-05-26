#ifndef MESSAGE_H
#define MESSAGE_H


#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include "Game.h"

#pragma comment(lib, "ws2_32.lib")

#define MAX_STRING_LEN 201 // need to find the size

typedef enum {
	TRNS_FAILED,
	TRNS_DISCONNECTED,
	TRNS_SUCCEEDED
} TransferResult_t;

typedef enum {
	PLAYER_1,
	PLAYER_2,
	SERVER_NUMBER,
	NOT_ACTIVE
}Owner;

typedef struct _Buffer {
	char *Line;
	struct _Buffer *next;
}Buffer;

typedef struct _InfoStruct
{
	Owner   OwnerNumber;
	SOCKET  PairSocket;
	Buffer  *SendBuffer;
	HANDLE  SendMutex;
	HANDLE  SendFullSemaphore;
	HANDLE  SendEmptySemaphore;
	struct  _InfoStruct *Other;
	char    NameOfUser[MAX_STRING_LEN];
	FILE    *LogFile;
	FILE	*ClientFile;
}InfoStruct;

TransferResult_t ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd);
BOOL RecieveMessage(InfoStruct *param);
BOOL TransferMessageToBuffer(InfoStruct *Param, char *Line);
BOOL OpenInsertBufferGuards(HANDLE *Mutex, HANDLE *EmptySemaphore);
BOOL CreateNewBuffer(Buffer **NewBuffer, char *Line);
BOOL CloseInsertBufferGuards(HANDLE *Mutex, HANDLE *FullSemaphore);
void InsertToBuffer(Buffer **HeadList, Buffer *NewMember);
TransferResult_t SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd);
BOOL SendMessageFunction(InfoStruct *param);
BOOL FetchMessageFromBuffer(InfoStruct *Param, Buffer **NewMessage);
BOOL RecivedMessageHandler(InfoStruct *Param, char *Line);
BOOL OpenExtractBufferGuards(HANDLE *Mutex, HANDLE *FullSemaphore);
BOOL CloseExtractBufferGuards(HANDLE *Mutex, HANDLE *EmptySemaphore);
BOOL ExtractMessageFromBuffer(Buffer **SourceBuffer, Buffer **NewMessage);


int split_line(char stringArr[][MAX_STRING_LEN], char StringToSplit[], char delimiter[]);



void TranslateMessageNew(char OriginalLine[MAX_STRING_LEN], char NewLine[MAX_STRING_LEN]);
void CopyPartOfString(char *start, char *End, char temp[]);
void GetString(char Line[], int Size);
#endif // !MESSAGE_H
