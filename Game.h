#ifndef GAME_H
#define GAME_h

#include <stdio.h>
#include <Windows.h>


#define RED_PLAYER 1
#define YELLOW_PLAYER 2

#define BOARD_HEIGHT 6
#define BOARD_WIDTH  7

#define BLACK  15
#define RED    204
#define YELLOW 238

#define BOARD_STRING_LENGTH 100

void PrintBoard(int board[][BOARD_WIDTH]);
BOOL IsStepValid(int Column);
int game_main();
BOOL BoardViewMessage(char **MessageString);
BOOL ThereIsAWinner(int clm);
void DisplayBoard(char BoardString[]);
BOOL CheckIfTie();
void UpdateBoard(int column, int Player);
void IntilizeBoard();

#endif // !GAME_h