#ifndef LOG_FILE_H
#define LOG_FILE_H
#include "Message.h"

FILE *OpenLogFile(char Path[]);  //open log file according to path/ name of file.
void ReciveFromServer_Log(InfoStruct *Param, char MSG[]);
void SendToServer_Log(InfoStruct *Param, char MSG[]);

#endif //LOG_FILE_H 