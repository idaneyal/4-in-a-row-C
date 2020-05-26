
/*
Description:	This modul is in charge of the messages tranmission between the server and the clients. here there are functions that both the server
				and the clients uses
*/

#include "Message.h"
#include "Client_Main.h"
#include "Server_Main.h"


/*
Description: This function recieve a string through TCP protocol

Arguments:	char* OutputBuffer	- the string which recieve the message
			int BytesToReceive	- the number of byted which the message contain
			SOCKET sd			- the socket of the communication

Algorithm:	while there are stiil bytes to recieve
				reciev more bytes 
				if an error ocrred return
				add the new bytes to the previous bytes

Returns:	TRNS_FAILED			- the transmission failed
			TRNS_DISCONNECTED	- the communication is lost
			TRNS_SUCCEEDED		- the transmission succeed
*/

TransferResult_t ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd)
{
	char* CurPlacePtr = OutputBuffer;
	int BytesJustTransferred;
	int RemainingBytesToReceive = BytesToReceive;
	while (RemainingBytesToReceive > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesJustTransferred = recv(sd, CurPlacePtr, RemainingBytesToReceive, 0);

		if (BytesJustTransferred == SOCKET_ERROR)
		{
			return TRNS_FAILED;
		}
		else if (BytesJustTransferred == 0)
			return TRNS_DISCONNECTED; // recv() returns zero if connection was gracefully disconnected.

		RemainingBytesToReceive -= BytesJustTransferred;
		CurPlacePtr += BytesJustTransferred; // <ISP> pointer arithmetic
	}
	return TRNS_SUCCEEDED;
}

/*
Description:	This function is in charge of reciving messages from the server/user

Arguments:		InfoStruct *param - the struct which contain all the information needed to the
				connection and sychrinize between the differents threads

Algorithm:		infinite while loop
					recieve the first message which contain the length of the coming string
					create a string which will contain the recieved message
					if the message recieved without errors
						transfer it to the recieved message buffer
					else
						return FALSE

Returns:		The function return FALSe if an error occured during the procces of reciving the message, else return TRUE
*/

BOOL RecieveMessage(InfoStruct *param)
{
	int TotalStringSizeInBytes;
	TransferResult_t RecvRes;
	char *RecievedLine = NULL;
	while (1) {
		RecvRes = ReceiveBuffer((char *)(&TotalStringSizeInBytes),
			(int)(sizeof(TotalStringSizeInBytes)), // 4 bytes
			param->PairSocket);
		if (RecvRes != TRNS_SUCCEEDED) {
			if (param->OwnerNumber == PLAYER_1 || param->OwnerNumber == PLAYER_2) {
				fprintf(param->LogFile, "Server disconnected. Exiting.");
				ClientCloseAllThreads();
			}
			return FALSE;
		}
		RecievedLine = (char*)malloc(TotalStringSizeInBytes * sizeof(char));
		if (RecievedLine == NULL) {
			printf("And error occured during memory allocation. Exiting.\n");
			fprintf(param->LogFile, "Custom message:And error occured during memory allocation. Exiting.\n");
			fclose(param->LogFile);
			exit(-1);
		}
		RecvRes = ReceiveBuffer(RecievedLine, TotalStringSizeInBytes
			, param->PairSocket);

		if (RecvRes == TRNS_SUCCEEDED) {
			if (!RecivedMessageHandler(param, RecievedLine)) {
				if (param->OwnerNumber == PLAYER_1 || param->OwnerNumber == PLAYER_2) {
					ClientCloseAllThreads();
				}
				return FALSE;
			}

		}
		else {
			free(RecievedLine);
			printf("Server disconnected. Exiting.\n");
			if (param->OwnerNumber != SERVER_NUMBER) {
				fprintf(param->LogFile, "Server disconnected. Exiting.");
				ClientCloseAllThreads();
			}
			return FALSE;
		}
	}

	return TRUE;
}

/*
Description:	This function transfer the recieved message into the buffer

Arguments:		InfoStruct *Param		- the param struct of the user
				char *Line				- the recieved line

Algorithm:		if Recieved message is TRUE
					intilize the mutex and semaphores to the value of the recieved message buffer
				else
					intilize the mutex and semaphores to the value of the send message buffer
				create the new buffer struct
				check if can insert into the buffer
				insert the new struct to buffer
				close the buffer's guard

Returns:		The function return FALSE if an error occured during it run, else it return TRUE
*/

BOOL TransferMessageToBuffer(InfoStruct *Param, char *Line)
{
	Buffer *NewBuffer = NULL;
	DWORD waitcode = 0;
	Buffer **TempBufferHead = NULL;
	Owner Number;
	HANDLE *Mutex, *FullSemaphore, *EmptySemaphore;
	TempBufferHead = &(Param->SendBuffer);
	Mutex = &(Param->SendMutex);
	FullSemaphore = &(Param->SendFullSemaphore);
	EmptySemaphore = &(Param->SendEmptySemaphore);
	Number = Param->OwnerNumber;
	if (!CreateNewBuffer(&NewBuffer, Line)) {
		fprintf(Param->LogFile, "Custom message: An error occured during memory allocation of buffer\n");
		fclose(Param->LogFile);
		exit(-1);
	}
	if (!OpenInsertBufferGuards(Mutex, EmptySemaphore))
		return FALSE;
	InsertToBuffer(TempBufferHead, NewBuffer);//critical code
	if (!CloseInsertBufferGuards(Mutex, FullSemaphore)) {
		return FALSE;
	}
	return TRUE;
}

/*
Description:	This function is in charge of getting permission to access the buffer

Arguments:		HANDLE *Mutex			- a pointer  to the mutex of the buffer
				HANDLE *EmptySemaphore	- a pointer to the EmptySemaphore of the buffer

Returns:		The function return TRUE if no error occured during it operation, otherwise return FALSE
*/

BOOL OpenInsertBufferGuards(HANDLE *Mutex, HANDLE *EmptySemaphore)
{
	DWORD waitcode = 0;
	waitcode = WaitForSingleObject(*EmptySemaphore, INFINITE);
	if (waitcode != WAIT_OBJECT_0) {
		printf("An error occured during the waiting for the Empty semaphore\nExiting program...\n");
		return FALSE;
	}
	waitcode = WaitForSingleObject(*Mutex, INFINITE);
	if (waitcode != WAIT_OBJECT_0) {
		printf("An Error occured during waiting to the Recieved buffer mutex.\n Exiting Thread\n");
		return FALSE;
	}
	return TRUE;
}

/*
Description:	This function create a new Buffer struct with the relevant values

Arguments:		Buffer **NewBuffer	- a pointer to the new buffer
				char *Line			- the line which the buffer will contain

returns:		The function return FALSE if an error occured during the memory allocation of the buffer or it component
*/

BOOL CreateNewBuffer(Buffer **NewBuffer, char *Line)
{
	int size = (int)strlen(Line) + 1;
	*NewBuffer = (Buffer*)malloc(sizeof(Buffer));
	if (*NewBuffer == NULL) {
		printf("An error occured during the creation of the new buffer member\n. Exiting program...");
		return FALSE;
	}
	(*NewBuffer)->Line = (char*)malloc(size * sizeof(char));
	if ((*NewBuffer)->Line == NULL) {
		free(*NewBuffer);
		printf("An error occured during the creation of the new buffer member\n. Exiting program...");
		return FALSE;
	}
	strcpy((*NewBuffer)->Line, Line);
	(*NewBuffer)->next = NULL;
	return TRUE;
}

/*
Description:	This function is in charge of relesing the buffer so other theads could use it

Arguments:		HANDLE *Mutex			- the mutex of the buffer
				HANDLE *FullSemaphore	- the FullSemaphore of the buffer

Returns:		The function return TRUE if no error occured during it operation, otherwise return FALSE
*/

BOOL CloseInsertBufferGuards(HANDLE *Mutex, HANDLE *FullSemaphore)
{
	if (ReleaseMutex(*Mutex) == FALSE)
	{
		printf("An Error occured during releasing to the buffer mutex.\n Exiting Thread\n");
		return FALSE;
	}
	if (ReleaseSemaphore(*FullSemaphore, 1, NULL) == FALSE)
	{
		printf("An Error occured during releasing to the buffer's full semaphore.\n Exiting Thread\n");
		return FALSE;
	}
	return TRUE;
}

/*
Description:	This function is in charge of insert a message to the buffer

Arguments:		Buffer **HeadList	- a pointer to the head of the buffer
				Buffer *NewMember	- a pointer to the new message
*/

void InsertToBuffer(Buffer **HeadList, Buffer *NewMember)
{
	Buffer *temp = *HeadList;
	if (temp == NULL) {
		*HeadList = NewMember;
		return;
	}
	while (temp->next != NULL) {
		temp = temp->next;
	}
	temp->next = NewMember;
}

/*
Description:	This function is in charge of sending messages through TCP protocol

Arguments:		const char* Buffer	- a pointer to the message which will be send
				int BytesToSend		- the number of bytes which we will send
				SOCKET sd			- the communication socket

Algorithm:		while there are still bytes to send
					send the next bytes

Returns:		TRNS_FAILED - the transmission of the message faild
				TRNS_SUCCEEDED - the transmission of the message succedd
*/

TransferResult_t SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd)
{
	const char* CurPlacePtr = Buffer;
	int BytesTransferred;
	int RemainingBytesToSend = BytesToSend;

	while (RemainingBytesToSend > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesTransferred = send(sd, CurPlacePtr, RemainingBytesToSend, 0);
		if (BytesTransferred == SOCKET_ERROR)
		{
			printf("send() failed, error %d\n", WSAGetLastError());
			return TRNS_FAILED;
		}

		RemainingBytesToSend -= BytesTransferred;
		CurPlacePtr += BytesTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}

/*
Description:	This function is in charge of sending messages to the server/user

Arguments:		InfoStruct *Param	- a pointer to a Infostruct which contain all the data of the server/user

Algorihtm:		if it is a user
					send the first message to the server
				infinite while loop
					fetch a message
					send the message length
					send the message itself

Returns:		the function return FALSE if an error occured during it operation, else it return TRUE(the function run forever unless an error occured so it will never return the value TRUE)
*/

BOOL SendMessageFunction(InfoStruct *param)
{
	int TotalStringSizeInBytes;
	TransferResult_t SendRes;
	Buffer *NewMessage = NULL;
	while (1) {
		if (!FetchMessageFromBuffer(param, &NewMessage)) {
			return FALSE;
		}
		if (NewMessage == NULL)
			continue;
		TotalStringSizeInBytes = (int)(strlen(NewMessage->Line) + 1); // terminating zero also sent	
		SendRes = SendBuffer(
			(const char *)(&TotalStringSizeInBytes),
			(int)(sizeof(TotalStringSizeInBytes)), // sizeof(int) 
			param->PairSocket);
		if (SendRes != TRNS_SUCCEEDED) {
			if (param->OwnerNumber != SERVER_NUMBER)
				fprintf(param->LogFile, "Server disconnected. Exiting.");
			return SendRes;
		}
		SendRes = SendBuffer(
			(const char *)(NewMessage->Line),
			(int)(TotalStringSizeInBytes),
			param->PairSocket);
		free(NewMessage->Line);
		free(NewMessage);
	}
	return TRUE;
}

/*
Description:	This function fetch a message from a buffer

Arguments:		InfoStruct *Param		- a pointer to a InfoStruct which contain all the data of the server/user
				Buffer **NewMessage		- a pointer which will point on the message we want to send

Algorihtm:		Choose which buffer is used and which mutex and semaphores to use
				wait for the buffer to be free to use
				extract the first message from the buffer
				relese the buffer's mutex and semaphores

Returns:		the function return FALSE if an error occured during it operation, else it return TRUE
*/

BOOL FetchMessageFromBuffer(InfoStruct *Param, Buffer **NewMessage)
{
	HANDLE *Mutex = NULL, *FullSemaphore = NULL, *EmptySemaphore = NULL;
	Buffer **CurrentBuffer;
	CurrentBuffer = &(Param->SendBuffer);
	Mutex = &(Param->SendMutex);
	FullSemaphore = &(Param->SendFullSemaphore);
	EmptySemaphore = &(Param->SendEmptySemaphore);
	if (!OpenExtractBufferGuards(Mutex, FullSemaphore))
		return FALSE;
	if (!ExtractMessageFromBuffer(CurrentBuffer, NewMessage))
		return FALSE;
	if (!CloseExtractBufferGuards(Mutex, EmptySemaphore))
		return FALSE;
	return TRUE;
}

/*
Description:	This function check with the mutex and full semaphore if it is possible to extract from the buffer

Arguments:		HANDLE *Mutex			- a pointer to the buffer's mutex
				HANDLE *FullSemaphore	- a pointer to the buffer's full semaphore


Returns:		the function return FALSE if an error occured during it operation, else it return TRUE
*/

BOOL OpenExtractBufferGuards(HANDLE *Mutex, HANDLE *FullSemaphore)
{
	DWORD waitcode = 0;
	waitcode = WaitForSingleObject(*FullSemaphore, INFINITE);
	if (waitcode != WAIT_OBJECT_0) {
		printf("An Error occured during waiting to the Full Semaphore.\n Exiting Thread\n");
		return FALSE;
	}
	waitcode = WaitForSingleObject(*Mutex, INFINITE);
	if (waitcode != WAIT_OBJECT_0) {
		printf("An Error occured during waiting to the buffer mutex.\n Exiting Thread\n");
		return FALSE;
	}
	return TRUE;
}

/*
Description:	This function release the mutex and empty semaphore after a message was extract from the buffer

Arguments:		HANDLE *Mutex			- a pointer to the buffer's mutex
				HANDLE *EmptySemaphore	- a pointer to the buffer's empty semaphore

Returns:		the function return FALSE if an error occured during it operation, else it return TRUE
*/

BOOL CloseExtractBufferGuards(HANDLE *Mutex, HANDLE *EmptySemaphore)
{
	if (ReleaseMutex(*Mutex) == FALSE)
	{
		printf("An Error occured during releasing to the buffer mutex.\n Exiting Thread\n");
		return FALSE;
	}
	if (ReleaseSemaphore(*EmptySemaphore, 1, NULL) == FALSE)
	{
		printf("An Error occured during releasing to the buffer's empty semaphore.\n Exiting Thread\n");
		return FALSE;
	}
	return TRUE;
}

/*
Description:	This function extract the first message from the buffer

Arguments:		Buffer **SourceBuffer	- a pointer to the buffer which we want to extract from it
				Buffer **NewMessage		- a pointer which will point on the requested message from the buffer

Returns:		the function return FALSE if an error occured during it operation, else it return TRUE
*/

BOOL ExtractMessageFromBuffer(Buffer **SourceBuffer, Buffer **NewMessage)
{
	if (SourceBuffer == NULL || *SourceBuffer == NULL) {
		printf("Source Buffer is NULL.\nExiting program...\n");
		return FALSE;
	}
	*NewMessage = *SourceBuffer;
	(*SourceBuffer) = (*SourceBuffer)->next;
	return TRUE;
}


/*
Description:	This function is in charge of te recieved messages

Arguments:		InfoStruct *Param	- a pointer to InfoStruct of client\server
				char *Line			- the message string

Algorithm:		if it is the server
					call server message handler
				else
					call the client message handler

Returns:		The function return TRUE if no error occured during it operation, otherwise return FALSE
*/

BOOL RecivedMessageHandler(InfoStruct *Param, char *Line)
{
	if (Param->OwnerNumber == SERVER_NUMBER) {
		return ServerMessageHandler(Param, Line);
	}
	else {
		ReciveFromServer_Log(Param, Line);
		return ClientRecieveMessageHandler(Param, Line);
	}
	return FALSE;
}

/*
Description:	This function split a line by specific parameter

Arguments:		char stringArr[][MAX_STRING_LEN]	- an array of strings which will contain the message after spliting
				char StringToSplit[]				- the original split
				char delimiter[]					- the parameter to tplit the line with

Algorithm:		for loop
					if the delimiter doesnt occured any more in the string
						break;
					copy the splited string to it place in the stringArr
					continue

Returns:		The function return the number of strings which we splited
*/

int split_line(char stringArr[][MAX_STRING_LEN], char StringToSplit[], char delimiter[])
{
	int count = 0;
	char *pos;
	char *bb = StringToSplit;
	char copy[MAX_STRING_LEN];
	for (count = 0; count < MAX_STRING_LEN - 1; count++)
	{
		if ((pos = strstr(bb, delimiter)) == NULL)
			break;
		*pos = '\0';
		strcpy(copy, bb);
		strcpy(stringArr[count], copy);
		bb = pos + strlen(delimiter);
	}
	if ((pos = strstr(bb, "\n")) != NULL)
	{
		*pos = '\0';
	}
	strcpy(copy, bb);
	strcpy(stringArr[count], copy);
	return (count + 1);
}

/*
Description:	This function copy a string by passing it the pointer of the start and end
				to the chars we want to copy

Arguments:		char *start	- the start of the string we want to copy
				char *End	- the end of the string we want to copy
				char temp[]	- a string which we copy the string in to

*/

void CopyPartOfString(char *start, char *End, char temp[])
{
	int i = 0;
	while (start != End && *start != '\0') {
		temp[i] = *start;
		start++;
		i++;
	}
	temp[i] = '\0';
}

/*
Description:	This function is in charge of translating the messages

Arguments:		char OriginalLine[MAX_STRING_LEN]	- the orogonal message
				char NewLine[MAX_STRING_LEN]		- the translated message

Algorithm:		find the first occurence of ':' in the message
				get rid of the ';' chars in the string

Returns:
*/

void TranslateMessageNew(char OriginalLine[MAX_STRING_LEN], char NewLine[MAX_STRING_LEN])
{
	char *EndPos = NULL, temp[MAX_STRING_LEN];
	char *StartPos = strstr(OriginalLine, ":");
	strcpy(NewLine, "");
	if (StartPos == NULL) {
		StartPos = OriginalLine;
	}
	else {
		StartPos++;
	}
	while ((EndPos = strstr(StartPos, ";")) != NULL) {
		CopyPartOfString(StartPos, EndPos, temp);
		strcat(NewLine, temp);
		StartPos = EndPos + 1;
	}
	strcat(NewLine, StartPos);
}

/*
Description:	This function is reciving string from the stdin pointer

Arguments:		char Line[]	- the line which contain the recieved string
				int Size	- the size of the Line

*/

void GetString(char Line[], int Size)
{
	char tav;
	int i = 0;
	while ((tav = getchar()) != '\n' && i < Size) {
		Line[i] = tav;
		i++;
	}
	Line[i] = '\0';
}