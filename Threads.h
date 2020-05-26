#ifndef THREADS_H
#define THREADS_H


#include "Message.h"


#define BUFFER_SIZE 20


HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine, LPVOID *param);
BOOL CreateSemaphoreSimple(HANDLE *Full, HANDLE *Empty);
BOOL CreateMutexSimple(HANDLE *MutexHandle);
BOOL CreateMutexAndSemaphore(InfoStruct *param);
DWORD WINAPI SendThread(InfoStruct *param);
DWORD WINAPI RecieveThread(InfoStruct *param);
DWORD WINAPI ServerThreads(InfoStruct *param);
DWORD WINAPI ClientUserInterface(InfoStruct *param);
BOOL CreateMutexToInfoStruct(InfoStruct *param);
void CloseMutesAndSemaphore(InfoStruct *Param);
void FreeBuffer(Buffer **Head);

#endif // !THREADS_H
