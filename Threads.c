
/*
Description:	This modul contain all the function that related to theards. it also contain the client and server main threads
*/

#include "Threads.h"
#include "Client_Main.h"
#include "Server_Main.h"

/*
Description:	This function create a thread

Parameters:		LPTHREAD_START_ROUTINE p_start_routine - a function which will be open by the thread
				LPDWORD* p_thread_id - a pointer to the thread id
				LPVOID *param - a pointer to the parameter which the thread will pass to the function it open

Retrun:			this function return the handle of the thread
*/

HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine, LPVOID *param)
{
	HANDLE thread_handle;
	LPDWORD p_thread_id;
	InfoStruct *lp;
	if (NULL == p_start_routine)
	{
		printf("An error occured during the creation of the thread\n"
			"Exiting program...\n");
		lp = (InfoStruct*)param;
		fprintf(lp->LogFile, "Custom message: An error occured during the creation of the thread\n"
			"Exiting program...\n");
		fclose(lp->LogFile);
		exit(-1);
	}
	thread_handle = CreateThread(NULL, 0, p_start_routine, param, 0, &p_thread_id);
	if (thread_handle == NULL)
	{
		printf("An error occured during the creation of the thread\n"
			"Exiting program...\n");
		lp = (InfoStruct*)param;
		fprintf(lp->LogFile, "Custom message: An error occured during the creation of the thread\n"
			"Exiting program...\n");
		fclose(lp->LogFile);
		exit(-1);
	}
	return thread_handle;
}

/*
Description:	This function create mutex

Arguments:		HANDLE *MutexHandle	- a pointer ti the mutex

Returns:		The function return TRUE if no error occured during it operation, otherwise return FALSE
*/

BOOL CreateMutexSimple(HANDLE *MutexHandle)
{
	*MutexHandle = CreateMutex(NULL, FALSE, NULL);
	if (*MutexHandle == NULL) {
		printf("An error occured during Createmutex \n"
			"Exiting program....\n");
		return FALSE;
	}
	return TRUE;
}

/*
Description:	This function open the semaphores of the buffer

Arguments:		int Size			- the size of the buffer
				LPCSTR EmptyCells	- the name of the EmptyCells semaphore
				LPCSTR FullCells	- the name of the FullCells semaphore
				HANDLE *Full		- a pointer to the full semaphore
				HANDLE *Empty		- a pointer to the Empty semaphore

Returns:		The function return FAIL if an error occured during the Creation of the semaphors
				The function return SUCCESS if it function worked without problems
*/

BOOL CreateSemaphoreSimple(HANDLE *Full, HANDLE *Empty)
{
	*Empty = CreateSemaphore(NULL, BUFFER_SIZE, BUFFER_SIZE, NULL);
	*Full = CreateSemaphore(NULL, 0, BUFFER_SIZE, NULL);
	if (*Empty == NULL || *Full == NULL)
	{
		printf("An Error occured during the creation of the buffer semaphores and mutex\n"
			"Exiting program...\n");
		return FALSE;
	}
	return TRUE;
}
/*

Description:	This function is in charge of creating mutex and semaphores for the param InfoStruct

Arguments:		InfoStruct *Param - a pointer to InfoStruct of server\Client
*/

BOOL CreateMutexAndSemaphore(InfoStruct *param)
{
	if (!CreateSemaphoreSimple(&(param->SendFullSemaphore), &(param->SendEmptySemaphore))) {
		printf("An error occured during the creation of the mutex and semaphores. Exiting program...\n");
		fprintf(param->LogFile, "Custom message: An error occured during the creation of the mutex and semaphores."
			"Exiting program...\n");
		fclose(param->LogFile);
		exit(-1);
	}
	if (!CreateMutexSimple(&(param->SendMutex))) {
		printf("An error occured during the creation of the mutex and semaphores. Exiting program...\n");
		fprintf(param->LogFile, "Custom message: An error occured during the creation of the mutex and semaphores."
			"Exiting program...\n");
		fclose(param->LogFile);
		exit(-1);
	}
	return TRUE;
}

/*
Description:	This is the send message thread for both the client and the server

Arguments:		InfoStruct *Param - a pointer to InfoStruct of server\client

Returns:		This functio return 0 if it run without problem and WAIT_FAILED if it was forced to close
*/

DWORD WINAPI SendThread(InfoStruct *param)
{
	if (!SendMessageFunction(param)) {
		if (param->OwnerNumber == SERVER_NUMBER) {
			EndSession(param);
		}
		else if (param->OwnerNumber == PLAYER_1 || param->OwnerNumber == PLAYER_2) {
			ClientCloseAllThreads();
		}
		return WAIT_FAILED;
	}
	return 0;
}

/*
Description:	This is the recieve message thread for both the client and the server

Arguments:		InfoStruct *Param - a pointer to InfoStruct of server\client

Returns:		This functio return 0 if it run without problem and WAIT_FAILED if it was forced to close
*/

DWORD WINAPI RecieveThread(InfoStruct *param)
{
	if (!RecieveMessage(param)) {
		if (param->OwnerNumber == SERVER_NUMBER) {
			EndSession(param);//server mod
		}
		else if (param->OwnerNumber == PLAYER_1 || param->OwnerNumber == PLAYER_2) {
			ClientCloseAllThreads();//client mod
		}
		return WAIT_FAILED;
	}
	return 0;
}

/*
Description:	This is the User interface thread for the client

Arguments:		InfoStruct *Param - a pointer to InfoStruct of client

Algorithm:		if it is file mode
					read the input file
					enter the file mod manager
					if an error occured
						clear all and force exit to program
				else
					enter human play mod
Returns:		This functio return 0 if it run without problem and WAIT_FAILED if it was forced to close
*/

DWORD WINAPI ClientUserInterface(InfoStruct *param)
{
	BOOL IsExit = FALSE;
	Buffer *CommandList = NULL;
	if (IsFileMode() == TRUE) {
		ReadGameFile(&CommandList, param);
		if (!AoutomaticUserPlayer(param,&CommandList)) {
			FreeBuffer(&CommandList);
			ClientCloseAllThreads();
			return WAIT_FAILED;
		}
		return 0;
	}
	while (!IsExit) {
		if (!ClientSendMessage(param, &IsExit)) {
			ClientCloseAllThreads();
			return WAIT_FAILED;
		}
	}
	return 0;
}

/*
Description:	This is the server main thread to communicatio with a single client

Arguments:		InfoStruct *Param - a pointer to InfoStruct of server

Algorithm:		create send and recieve threads
				wait for at least one  of them to end
				shut down all other threads of communication with the client
				varify that the communication thread with the other client also end it operation

Returns:		This function return 0 
*/

DWORD WINAPI ServerThreads(InfoStruct *param)
{
	HANDLE ThreadsArr[2];
	DWORD waitcode;
	CreateMutexAndSemaphore(param);
	ThreadsArr[0] = CreateThreadSimple(RecieveThread, (LPVOID)param);
	ThreadsArr[1] = CreateThreadSimple(SendThread, (LPVOID)param);
	waitcode = WaitForMultipleObjects(2, ThreadsArr, FALSE, INFINITE);
	for (int i = 0; i < 2; i++) {
		TerminateThread(ThreadsArr[i], WAIT_FAILED);
		CloseHandle(ThreadsArr[i]);
	}
	if(param->OwnerNumber==SERVER_NUMBER)
		EndSession(param);
	fprintf(param->LogFile, "Player disconnected. Ending Communication\n"); //print to server log file
	return 0;
}

/*
Description:	This function create mutexes for the send and recieve buffer of each client

Arguments:		InfoStruct *param	- a pointer to InfoStruct which will be send as parameter to a client thread

Returns:		the function return FALSE if an error occured during the creation of the mutex, otherwise it will return TRUE
*/

BOOL CreateMutexToInfoStruct(InfoStruct *param)
{
	param->SendMutex = CreateMutex(NULL, FALSE, NULL);
	if (param->SendMutex == NULL) {
		printf("An error occured during the creation of the mutex.\n Exiting program...");
		return FALSE;
	}
	return TRUE;
}

/*
Description:	This function close the mutex and semaphores of a server or client

Arguments:		InfoStruct *Param - a pointer to InfoStruct of server\server
*/

void CloseMutesAndSemaphore(InfoStruct *Param)
{
	CloseHandle(Param->SendEmptySemaphore);
	CloseHandle(Param->SendFullSemaphore);
	CloseHandle(Param->SendMutex);
}

/*
Description:	This function free a linked list of Buffer struct

Arguments:		Buffer **Head	- a pointer to the head of the list

*/

void FreeBuffer(Buffer **Head)
{
	Buffer *temp = NULL;
	while (*Head != NULL) {
		temp = *Head;
		*Head = (*Head)->next;
		free(temp);
	}
}
