/*
Description:	This modul contain all the game related function, both the checks if a step is valid and the print of the board
*/
#include "Game.h"

static int ServerBoard[BOARD_HEIGHT][BOARD_WIDTH] = { 0 };

int game_main()
{
	int board[BOARD_HEIGHT][BOARD_WIDTH] = { 0 };
	HANDLE  consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	//here's an example of setting 2 slots
	board[5][3] = RED_PLAYER;
	board[4][3] = YELLOW_PLAYER;
	board[4][2] = RED_PLAYER;
	board[3][1] = RED_PLAYER;
	board[2][0] = RED_PLAYER;
	board[5][0] = YELLOW_PLAYER;
	board[4][0] = YELLOW_PLAYER;
	board[3][0] = YELLOW_PLAYER;
	//board[3][1] = YELLOW_PLAYER;
	board[4][1] = YELLOW_PLAYER;
	board[5][1] = RED_PLAYER;
	board[5][2] = RED_PLAYER;
	//This handle allows us to change the console's color

	PrintBoard(board);
	printf("there is a winner? = %d\n", ThereIsAWinner(board, 0));
	CloseHandle(consoleHandle);
	getchar();
	return 0;
}

/***********************************************************
* This function prints the board, and uses O as the holes.
* The disks are presented by red or yellow backgrounds.
* Input: A 2D array representing the board and the console handle
* Output: Prints the board, no return value
************************************************************/

void PrintBoard(int board[][BOARD_WIDTH])
{
	int row, column;
	HANDLE  consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	//Draw the board
	for (row = 0; row < BOARD_HEIGHT; row++)
	{
		for (column = 0; column < BOARD_WIDTH; column++)
		{
			printf("| ");
			if (board[row][column] == RED_PLAYER)
				SetConsoleTextAttribute(consoleHandle, RED);

			else if (board[row][column] == YELLOW_PLAYER)
				SetConsoleTextAttribute(consoleHandle, YELLOW);

			printf("O");

			SetConsoleTextAttribute(consoleHandle, BLACK);
			printf(" ");
		}
		printf("\n");
		//Draw dividing line between the rows
		for (column = 0; column < BOARD_WIDTH; column++) {
			printf("----");
		}
		printf("\n");
	}
}

/*
Description:	This function Check if a step is valid

Parameters:		int board[][]	- An int double array which represent the game's board
				int Column		- The step's column

Algorithm:		if column is not a valid value
					return FALSE
				if the requested Column is already full
					return FALSE
				return TRUE

Retrun:			this function return TRUE if the step is valid and FALSE if it isn't
*/

BOOL IsStepValid(int Column)
{
	if (Column >= BOARD_WIDTH || Column < 0) {
		return FALSE;
	}
	if (ServerBoard[0][Column] != 0) {
		return FALSE;
	}
	return TRUE;
}

/*
Description:	This function Check if the last move made the player to win the game

Parameters:		int Clm		- The step's column

Algorithm:		find the  row of the the last step
				check if there is a vertical 4 in a row
				check if there is a horizontal win
				check if there is diagonal win

Retrun:			this function return TRUE if the there is a winner and FALSE otherwise
*/

BOOL ThereIsAWinner(int clm)
{
	int row = 0, i = 0, j = 0, current = 0, count = 1;
	//find the row
	for (i = BOARD_HEIGHT - 1; i > -1; i--)
	{
		if (ServerBoard[i][clm] != RED_PLAYER && ServerBoard[i][clm] != YELLOW_PLAYER)
		{
			row = i + 1;
			break;
		}
	}
	current = ServerBoard[row][clm];
	//vertical
	if (row <= 2 && ServerBoard[row + 1][clm] == current && ServerBoard[row + 2][clm] == current && ServerBoard[row + 3][clm] == current)
		return 1;
	else {
		for (i = clm + 1; i < BOARD_WIDTH; i++) //horizontal
		{
			if (ServerBoard[row][i] != current)
				break;
			count++;
		}
		for (i = clm - 1; i >= 0; i--) //horizontal
		{
			if (ServerBoard[row][i] != current)
				break;
			count++;
		}
		if (count >= 4)
			return 1;

		count = 1;
		//diagonal
		for (i = row + 1, j = clm + 1; i < BOARD_HEIGHT && j < BOARD_WIDTH; i++, j++)
		{
			if (ServerBoard[i][j] != current)
				break;
			count++;
		}
		for (i = row - 1, j = clm - 1; i >= BOARD_HEIGHT && j >= BOARD_WIDTH; i--, j--)
		{
			if (ServerBoard[i][j] != current)
				break;
			count++;
		}
		if (count >= 4)
			return 1;
		for (i = row + 1, j = clm - 1; i < BOARD_HEIGHT && j >= 0; i++, j--)
		{
			if (ServerBoard[i][j] != current)
				break;
			count++;
		}
		for (i = row - 1, j = clm + 1; i >= 0 && j < BOARD_WIDTH; i--, j++)
		{
			if (ServerBoard[i][j] != current)
				break;
			count++;
		}
		if (count >= 4)
			return 1;
	}
	return 0;

}

/*
Description:	This function is in charge of creating the BOARD_VIEW message

Arguments:		char **MessageString	- a pointer to the message string

Algorithm:		create memory allocation to the message
				for each row
					for each column
						translate the value from int to string
						add the string to the previous string with ';' at the end of it
				switch the last ';' with '\0'
*/

BOOL BoardViewMessage(char **MessageString)
{
	int i, j;
	char temp[5];
	*MessageString = (char*)malloc(BOARD_STRING_LENGTH * sizeof(char));
	if (*MessageString == NULL) {
		printf("An error ocuured during the memory allocation of BOARD_VIEW MESSAGE."
			"Exiting program...\n");
		return FALSE;
	}
	strcpy(*MessageString, "BOARD_VIEW:");
	for (i = 0; i < BOARD_HEIGHT; i++) {
		for (j = 0; j < BOARD_WIDTH; j++) {
			sprintf(temp, "%d;", ServerBoard[i][j]);
			strcat(*MessageString, temp);
		}
	}
	(*MessageString)[strlen(*MessageString) - 1] = '\0';
	return TRUE;
}

/*
Description:	This function translate the BOARD_VIEW message into a double size array

Parameters:		int boardStirng[]	-  the BOARD_VIEW message

Algorithm:		for loop i
					for loop j
						translate the int to it place in the board

*/
void DisplayBoard(char BoardString[])
{
	int board[BOARD_HEIGHT][BOARD_WIDTH] = { 0 };
	char *pos = strstr(BoardString, ":");
	int i, j, value = 0;
	pos++;
	for (i = 0; i < BOARD_HEIGHT; i++) {
		for (j = 0; j < BOARD_WIDTH; j++) {
			while (*pos != '\0' && !isdigit(*pos)) {
				pos++;
			}
			if (*pos == '\0') {
				return;
			}
			board[i][j] = (int)(*pos - '0');
			pos++;
		}
	}
	PrintBoard(board);
}

/*
Description:	This function Check if there is a tie

Algorithm:		for each box in the game
					if there is a value 0 (no player chose this box)
						return FALSE
				return TRUE;

Retrun:			this function return TRUE if the step is valid and FALSE if it isn't
*/

BOOL CheckIfTie()
{
	int i, j;
	for (i = 0; i < BOARD_HEIGHT; i++) {
		for (j = 0; j < BOARD_WIDTH; j++) {
			if (ServerBoard[i][j] == 0) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

/*
Description:	This function update the board after a player play a step

Parameters:		int Player	- the value of the player chips
				int Column	- The step's column

Algorithm:		find the first place in the column which is 0 and place there the player value
*/

void UpdateBoard(int column, int Player)
{
	for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
		if (ServerBoard[i][column] == 0) {
			ServerBoard[i][column] = Player;
			return;
		}
	}
}

/*
Description:	This function clear the board before a game is start


Algorithm:		for each place in the array
					set the value of board to be 0

*/

void IntilizeBoard()
{
	int i, j;
	for (i = 0; i < BOARD_HEIGHT; i++) {
		for (j = 0; j < BOARD_WIDTH; j++) {
			ServerBoard[i][j] = 0;
		}
	}
}