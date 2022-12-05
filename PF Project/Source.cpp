﻿#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>
//#include "mygraphics.h"
//#include "yourgraphics.h"
#include "youregraphics.h"

#include <Mmsystem.h>
#include <mciapi.h>
//these two headers are already included in the <Windows.h> header
#pragma comment(lib, "Winmm.lib")

using namespace std;

float dpiRatio = 1;

const int BreakMultiplerMinTime = 1;
const float ball_force = 6;
const float fireballForceInc = ball_force * 0.10;
const int player_speed = 60;
const int playerMoveStep = 20;
const int POWERUP_TIME = 10;

int gameWidth = 1080, gameHeight = 600;
int gameX = 0, gameY = 0;
int consoleRows = 0, consoleCols = 0;

const int TopRowHeight = 80;

const int BricksRowCount = 1, BricksColCount = 3;

#pragma region Structures

struct Color
{
	int R = 0;
	int G = 0;
	int B = 0;
};

struct {
	Color White;
	Color ConsoleWhite;
	Color Black;
	Color Red;
	Color DarkRed;
	Color Green;
	Color Blue;
	Color Purple;
	Color Orange;

	Color Front;
	Color Back;
} COLORS;

struct Player
{
	const int shortWidth = 50;
	const int normalWidth = 120;
	const int longWidth = 200;

	int x;
	int y;

	int width = 120;
	int height = 20;

	int force_x = 0;

	int lives = 3; 

} player;

struct Ball
{
	const int normalWidth = 20;
	const int normalHeight = 20;
	const int fireballSizeInc = 10;

	float x = 200;
	float y = 200;

	int width = normalWidth;
	int height = normalHeight;

	float force_x = 0;
	float force_y = 0;

	bool fireball = false;

	Color color = COLORS.Back;
} ball;

struct
{
	const int shorten = 1;
	const int elongate = 2;
	const int fireball = 3;
	const int life = 4;
} PowerUpTypes;

struct PowerUp
{
	int x = 0;
	int y = 0;

	int width = 20;
	int height = 20;

	int force_y = 4;

	int type;
	bool active = false;
	bool dropping = false;

	int startedTime = 0;

	Color color;
} powerUp;

struct
{
	int score = 0;

	bool started = false;
	bool Over = false;
	bool paused = false;

	bool showStats = false;
	int BricksLeft = BricksColCount * BricksRowCount;

} GameManager;

struct Brick
{
	int x = 0;
	int y = 0;

	int width = 100;
	int height = 50;

	int health = 3;
} bricks[BricksRowCount][BricksColCount];

#pragma endregion

#pragma region Prototypes

void Clear(Color);
void UpdateFontSize(int);
void DrawLives();
void DrawPlayer(bool);

#pragma endregion

#pragma region Library Extensions

void drawRectangle(int x1, int y1, int x2, int y2, Color stroke, Color fill)
{
	drawRectangle(gameX + x1, y1, gameX + x2, y2, stroke.R, stroke.G, stroke.B, fill.R, fill.G, fill.B);
}

void drawRectangle(int x1, int y1, int x2, int y2, Color color)
{
	drawRectangle(gameX + x1, y1, gameX + x2, y2, color, color);
}

void drawEllipse(int x1, int y1, int x2, int y2, Color stroke, Color fill)
{
	drawEllipse(gameX + x1, y1, gameX + x2, y2, stroke.R, stroke.G, stroke.B, fill.R, fill.G, fill.B);
}

void drawEllipse(int x1, int y1, int x2, int y2, Color color)
{
	drawEllipse(gameX + x1, y1, gameX + x2, y2, color, color);
}

bool onKey(char key, char c)
{
	// Making both inputs lower case for easir comparision
	// this will also make it so we wont have to check an input twice for both cases
	if (key >= 'A' && key <= 'Z') key = char((int(key)) + 32);
	if (c >= 'A' && c <= 'Z') c = char((int(c)) + 32);

	return key == c;
}
#pragma endregion

bool onChance(int chance)
{
	int r = rand() % 100;

	if (r < chance) return true;

	return false;
}

int GetBricksLeft()
{
	int count = 0;
	for (int r = 0; r < BricksRowCount; r++)
	{
		for (int c = 0; c < BricksColCount; c++)
		{
			if (bricks[r][c].health > 0) count++;
		}
	}

	return count;
}

#pragma region HighScores

int HighScores[5];

bool AddScore(int score)
{
	for (int i = 0; i < 5; i++)
	{
		if (score > HighScores[i])
		{
			for (int j = 4; j > i; j--)
			{
				int temp = HighScores[j];
				HighScores[j] = HighScores[j - 1];
				HighScores[j - 1] = temp;
			}

			HighScores[i] = score;
			return true;
		}
	}

	return false;
}

void SaveHighScores()
{
	ofstream saveFile;
	saveFile.open("highscores.txt");

	for (int i = 0; i < 5; i++)
	{
		saveFile << HighScores[i] << endl;
	}

	saveFile.close();
}

void LoadHighScores()
{
	ifstream saveFile;
	saveFile.open("highscores.txt");

	for (int i = 0; i < 5; i++)
	{
		saveFile >> HighScores[i];
	}

	saveFile.close();
}

#pragma endregion

#pragma region Colission Detection

// TODO: fix
int LastTimeBrickBroke = 0;
void HandleBrickCollission(Ball& ball)
{
	for (int r = 0; r < BricksRowCount; r++)
	{
		for (int c = 0; c < BricksColCount; c++)
		{
			Brick& brick = bricks[r][c];

			// dont even check for collision if bricks health is zero
			if (brick.health == 0) continue;

			if (
				ball.x + ball.force_x < brick.x + brick.width &&
				ball.x + ball.force_x + ball.width > brick.x &&
				ball.y + ball.force_y < brick.y + brick.height &&
				ball.height + ball.y + ball.force_y > brick.y
				)
			{

				if (abs(ball.x - brick.x) > abs(ball.y - brick.y))
					ball.force_x *= -1;
				else
					ball.force_y *= -1;

				if (ball.fireball)
					brick.health = 0;
				else
					brick.health--;

				Color bColor = COLORS.Back;

				switch (brick.health)
				{
				case 3: bColor = COLORS.Green; break;
				case 2: bColor = COLORS.Blue; break;
				case 1: bColor = COLORS.Red; break;
				default: bColor = COLORS.Back; break;
				}

				Color col = bColor;

				if (brick.health != 0)
				{
					col.R *= 1 / 1.35;
					col.G *= 1 / 1.35;
					col.B *= 1 / 1.35;
				}

				drawRectangle(brick.x, brick.y, brick.x + brick.width, brick.y + brick.height, col, bColor);

				if (brick.health <= 0)
				{
					// Reward a bonus if player broke the brick in 1s of breaking the last one
					if (time(NULL) - LastTimeBrickBroke <= BreakMultiplerMinTime)
						GameManager.score += 10;
					else
						GameManager.score += 5;

					GameManager.BricksLeft--;
					LastTimeBrickBroke = time(NULL);

					// Spawn a new power up - 40% chance it will be dropped
					if (!powerUp.dropping && !powerUp.active && onChance(40))
					{
						powerUp.x = brick.x + brick.width / 2;
						// dropping from last bricks height to avoid reprinting bricks
						powerUp.y = bricks[BricksRowCount - 1][BricksColCount - 1].y + bricks[BricksRowCount - 1][BricksColCount - 1].height;

						if (onChance(5))
						{
							powerUp.type = PowerUpTypes.life;
							powerUp.color = COLORS.Red;
						}
						else if (onChance(15))
							powerUp.type = PowerUpTypes.fireball;
						else if (onChance(80))
							powerUp.type = onChance(50) ? PowerUpTypes.elongate : PowerUpTypes.shorten;

						powerUp.dropping = true;
					}
				}

				// Stop the game if all bricks are broken
				if (GameManager.BricksLeft <= 0) GameManager.Over = true;
			}
		}
	}
}

bool HandlePaddleCollission(Ball& ball)
{
	// detecting collision for the next ball position
	int nextBallx = ball.x + ball.force_x;
	int nextBallY = ball.y + ball.force_y;

	// TODO: check for center
	if (
		nextBallx < player.x + player.width && nextBallx + ball.width > player.x &&
		nextBallY < player.y + player.height && ball.height + nextBallY > player.y
		)
	{
		// Calculate the distance between centers
		int centerDistanceX = (player.x + player.width / 2) - (ball.x + ball.width / 2);
		int centerDistanceY = (player.y + player.height / 2) - (ball.y + ball.height / 2);

		// Calculate the minimum distance to separate along X and Y
		int minXDist = player.width / 2 + ball.width / 2;
		int minYDist = player.height / 2 + ball.height / 2;

		// Calculate the depth of collision for both the X and Y axis
		int depthX = centerDistanceX > 0 ? minXDist - centerDistanceX : -minXDist - centerDistanceX;
		int depthY = centerDistanceY > 0 ? minYDist - centerDistanceY : -minYDist - centerDistanceY;

		if (depthX != 0 && depthY != 0)
		{
			int ballCenter = nextBallx + ball.width / 2;

			float ballDistance = player.x + player.width - ballCenter;

			float ratio = ballDistance / player.width;

			ball.force_x = ball_force * cos((ratio * (180 - 30) + 15) * 3.14 / 180);
			ball.force_y = -ball_force * sin((ratio * (180 - 30) + 15) * 3.14 / 180);


			// TODO: for testing, remove
			//if (abs(depthX) < abs(depthY))
			//{
			//	//ball.force_x *= -1;

			//	int ballCenter = ball.x + ball.width / 2;

			//	if (ballCenter < player.x + player.width / 3)
			//		ball.force_x = -10;
			//	else if (ballCenter > player.x + player.width / 3 && ballCenter < player.x + player.width * 2/3)
			//		ball.force_x = 0;
			//	else if (ballCenter > player.x + player.width * 2 / 3)
			//		ball.force_x = 10;
			//	else ball.force_x *= -1;
			//		
			//}
			//else
			//	ball.force_y *= -1;

		}

		return true;
	}

	return false;
}

void setFireball(bool on)
{
	drawEllipse(ball.x, ball.y, ball.x + ball.width, ball.y + ball.height, COLORS.Back);

	if (on && !ball.fireball && GameManager.started)
	{
		ball.width = ball.normalWidth + ball.fireballSizeInc;
		ball.height = ball.normalHeight + ball.fireballSizeInc;

		ball.fireball = true;
	}
	else if (!on && ball.fireball && GameManager.started)
	{
		ball.width = ball.normalWidth;
		ball.height = ball.normalHeight;

		ball.fireball = false;
	}
}

bool HandlePaddleCollission(PowerUp& powerUp)
{
	if (
		powerUp.x < player.x + player.width && powerUp.x + powerUp.width > player.x &&
		powerUp.y + powerUp.force_y < player.y + player.height && powerUp.height + powerUp.y + powerUp.force_y > player.y
	)
	{
		if (powerUp.type == PowerUpTypes.shorten)
		{
			player.width = player.shortWidth;
		}
		else if (powerUp.type == PowerUpTypes.elongate)
		{
			player.width = player.longWidth;
		}
		else if (powerUp.type == PowerUpTypes.fireball)
		{
			setFireball(true);
		}
		else if (powerUp.type == PowerUpTypes.life && player.lives < 3)
		{
			player.lives++;
			DrawLives();
		}

		powerUp.dropping = false;
		powerUp.active = true;
		powerUp.startedTime = time(NULL);

		DrawPlayer(true);

		return true;
	}

	return false;
}

#pragma endregion

#pragma region Drawing

void Clear(Color bg = COLORS.Back)
{
	gotoxy(0, 0);
	cls();
	delay(100);
	drawRectangle(0, 0, gameWidth, gameHeight, bg);
	delay(200);
}

void DrawPlayer(bool redraw = false)
{
	// TODO: Optimization
	// dont draw again if the player is not moving
	//if (player.force_x == 0) return;

	if(redraw) drawRectangle(0, player.y, gameWidth, player.y + player.height, COLORS.Back);

	// Removing old paddle
	if (player.x + player.width > gameWidth)
	{
		int overflow_right = (player.x + player.width) - gameWidth;
		drawRectangle(0, player.y, overflow_right, player.y + player.height, COLORS.Back);
	}
	else if (player.x < 0)
	{
		int overflow_left = player.x * -1;
		drawRectangle(gameWidth - overflow_left, player.y, gameWidth, player.y + player.height, COLORS.Back);
	}
	
	// Calculating the differnece b/w the area of last and new paddle position and remove that
	if(player.x + player.force_x > player.x)
		drawRectangle(player.x, player.y, player.x + player.force_x + player.width, player.y + player.height, COLORS.Back);
	else 
		drawRectangle(player.x + player.force_x, player.y, player.x + player.width, player.y + player.height, COLORS.Back);


	// Adding force
	// Moving player in steps to make the movement smooth
	if (player.force_x > 0)
	{
		player.x += playerMoveStep;
		player.force_x -= playerMoveStep;

		if (player.force_x < 0) player.force_x = 0;
	}
	else if (player.force_x < 0)
	{
		player.x -= playerMoveStep;
		player.force_x += playerMoveStep;

		if (player.force_x > 0) player.force_x = 0;
	}


	// Calculate how much player is out of one side of the screen and teleport him accordingly
	if (player.x + player.width > gameWidth)
	{
		int overflow_right = (player.x + player.width) - gameWidth;

		drawRectangle(0, player.y, overflow_right, player.y + player.height, COLORS.Front);

		if (overflow_right > player.width)
			player.x = 0;
	}
	else if (player.x < 0)
	{
		int overflow_left = player.x * -1;

		drawRectangle(gameWidth - overflow_left, player.y, gameWidth, player.y + player.height, COLORS.Front);

		if (overflow_left > player.width)
			player.x = gameWidth - player.width;
	}

	drawRectangle(player.x < 0 ? 0 : player.x, player.y, player.x + player.width > gameWidth ? gameWidth : player.x + player.width, player.y + player.height, COLORS.Front);
}

void DrawBall()
{
	HandleBrickCollission(ball);

	HandlePaddleCollission(ball);

	drawEllipse(ball.x, ball.y, ball.x + ball.width, ball.y + ball.height, COLORS.Back);


	// Checking for Wall Collision
	if (ball.x + ball.force_x + ball.width >= gameWidth)
	{
		ball.x = gameWidth - ball.width;
		ball.force_x *= -1;
	}
	if (ball.x <= 0)
	{
		ball.x = 0;
		ball.force_x *= -1;
	}

	if (ball.y + ball.force_y < TopRowHeight)
	{
		ball.y = TopRowHeight + 1;
		ball.force_y *= -1;
	}

	// adding force
	ball.x += ball.force_x + (ball.fireball ? (fireballForceInc * (ball.force_x + 1 / abs(ball.force_x + 1))) : 0);
	ball.y += ball.force_y + (ball.fireball ? (fireballForceInc * (ball.force_y + 1 / abs(ball.force_y + 1))) : 0);

	drawEllipse(ball.x, ball.y, ball.x + ball.width, ball.y + ball.height, ball.fireball ? COLORS.Orange : ball.color);
}

void DrawPowerUp()
{
	if (powerUp.active)
	{
		if (time(NULL) - powerUp.startedTime > POWERUP_TIME)
		{
			powerUp.active = false;

			if (powerUp.type == PowerUpTypes.elongate || powerUp.type == PowerUpTypes.shorten)
				player.width = player.normalWidth;
			else if (powerUp.type == PowerUpTypes.fireball)
				setFireball(false);
			

			DrawPlayer(true);
		}
	}

	if (powerUp.dropping)
	{
		bool hit = HandlePaddleCollission(powerUp);

		drawEllipse(powerUp.x, powerUp.y, powerUp.x + powerUp.width, powerUp.y + powerUp.height, COLORS.Back);
		
		// adding force
		powerUp.y += powerUp.force_y;

		// remove the power up once its out of the screen
		if (powerUp.y > gameHeight + 100)
		{
			powerUp.dropping = false;
		}

		if (!hit) drawEllipse(powerUp.x, powerUp.y, powerUp.x + powerUp.width, powerUp.y + powerUp.height, powerUp.color);
	}
}

void DrawHeart(int x1, int y1, int scale, Color color) {
	// x1 and y1 coordinate of leftmost corner and it takes scale 
	// scale determines size of heart
	if (scale % 2 != 0) 
	{					
		scale = scale / 2 * 2;
	}
	drawLine(x1, y1, x1 + (scale / 2), y1 + (scale / 2), color.R, color.G, color.B);
	drawLine(x1 + scale, y1, x1 + (scale / 2), y1 + (scale / 2), color.R, color.G, color.B);
	drawLine(x1, y1, x1 + scale / 4.0, y1 - scale / 4.0, color.R, color.G, color.B);
	drawLine(x1 + scale / 4.0, y1 - scale / 4.0, x1 + (scale / 2), y1, color.R, color.G, color.B);
	drawLine(x1 + scale, y1, (x1 + scale) - scale / 4.0, y1 - scale / 4.0, color.R, color.G, color.B);
	drawLine((x1 + scale) - scale / 4.0, y1 - scale / 4.0, x1 + (scale / 2), y1, color.R, color.G, color.B);

	for (int i = 2; i <= scale; i++) {
		drawLine(x1 + i, y1, x1 + (scale / 2), y1 + (scale / 2), color.R, color.G, color.B);
	}
	int max = y1 - scale / 4.0;
	for (int i = 2; max <= y1; i++, max++) {
		drawLine(x1, y1, x1 + scale / 4.0, (y1 - scale / 4.0) + i, color.R, color.G, color.B);
		drawLine(x1 + scale, y1, (x1 + scale) - scale / 4.0, (y1 - scale / 4.0) + i, color.R, color.G, color.B);
		drawLine(x1 + scale / 4.0, (y1 - scale / 4.0) + i, x1 + (scale / 2), y1, color.R, color.G, color.B);
		drawLine((x1 + scale) - scale / 4.0, (y1 - scale / 4.0) + i, x1 + (scale / 2), y1, color.R, color.G, color.B);

	}

}

void DrawLives()
{
	for (int i = 1; i <= 3; i++)
	{
		if (i <= player.lives)
			DrawHeart(gameWidth - (50 * i) - 25 - (i * 20), 30, TopRowHeight - 20, COLORS.Red);
		else
			DrawHeart(gameWidth - (50 * i) - 25 - (i * 20), 30, TopRowHeight - 20, Color{ 10,10,10 });
	}
}

int lastScore = -1;
void DrawScore()
{
	// only print new score if the score has changed
	if (lastScore != GameManager.score)
	{
		string a = to_string(GameManager.score);
		gotoxy(0, 0);

		cout << "\033[27m " << a << "\033[27";

		lastScore = GameManager.score;
	}
}

void DrawBricks()
{
	for (int r = 0; r < BricksRowCount; r++)
	{
		for (int c = 0; c < BricksColCount; c++)
		{
			Brick& b = bricks[r][c];

			Color bColor = COLORS.Back;

			switch (b.health)
			{
			case 3: bColor = COLORS.Green; break;
			case 2: bColor = COLORS.Blue; break;
			case 1: bColor = COLORS.Red; break;
			default: bColor = COLORS.Back; break;
			}

			// darkering the color for borders

			Color darkerbColor = bColor;

			if (b.health != 0)
			{
				darkerbColor.R *= 1 / 1.35;
				darkerbColor.G *= 1 / 1.35;
				darkerbColor.B *= 1 / 1.35;
			}

			drawRectangle(b.x, b.y, b.x + b.width, b.y + b.height, darkerbColor, bColor);
		}
	}

	GameManager.BricksLeft = GetBricksLeft();
}

void RedrawGame()
{
	Clear();

	drawRectangle(0, 0, gameWidth, TopRowHeight, COLORS.Front);

	DrawLives();

	DrawPlayer(true);

	lastScore = -1;
	DrawScore();

	DrawBricks();
}
#pragma endregion

#pragma region Saving Game State

bool DoesSaveFileExist()
{
	ifstream saveFile;
	saveFile.open("E:/gamestate.txt");

	return saveFile.is_open();
}

void SaveGameState()
{
	ofstream saveFile;
	saveFile.open("E:/gamestate.txt");

	saveFile << player.x << endl;
	saveFile << player.y << endl;
	saveFile << player.width << endl;
	saveFile << player.height << endl;
	saveFile << player.force_x << endl;
	saveFile << player.lives << endl;

	saveFile << ball.x << endl;
	saveFile << ball.y << endl;
	saveFile << ball.width << endl;
	saveFile << ball.height << endl;
	saveFile << ball.force_x << endl;
	saveFile << ball.force_y << endl;

	saveFile << GameManager.paused << endl;
	saveFile << GameManager.score << endl;
	saveFile << GameManager.showStats << endl;
	saveFile << GameManager.started << endl;

	for (size_t r = 0; r < BricksRowCount; r++)
	{
		for (size_t c = 0; c < BricksColCount; c++)
		{
			Brick b = bricks[r][c];

			saveFile << b.x << endl;
			saveFile << b.y << endl;
			saveFile << b.width << endl;
			saveFile << b.height << endl;
			saveFile << b.health << endl;
		}
	}

	saveFile.close();
}

void LoadGameState()
{
	ifstream saveFile;
	saveFile.open("E:/gamestate.txt");

	saveFile >> player.x;
	saveFile >> player.y;
	saveFile >> player.width;
	saveFile >> player.height;
	saveFile >> player.force_x;
	saveFile >> player.lives;

	saveFile >> ball.x;
	saveFile >> ball.y;
	saveFile >> ball.width;
	saveFile >> ball.height;
	saveFile >> ball.force_x;
	saveFile >> ball.force_y;

	saveFile >> GameManager.paused;
	saveFile >> GameManager.score;
	saveFile >> GameManager.showStats;
	saveFile >> GameManager.started;

	for (size_t r = 0; r < BricksRowCount; r++)
	{
		for (size_t c = 0; c < BricksColCount; c++)
		{
			Brick& b = bricks[r][c];

			saveFile >> b.x;
			saveFile >> b.y;
			saveFile >> b.width;
			saveFile >> b.height;
			saveFile >> b.health;
		}
	}

	RedrawGame();

	saveFile.close();
}

#pragma endregion

#pragma region Initial Set up

void setUpColors()
{
	COLORS.White.R = 236;
	COLORS.White.G = 240;
	COLORS.White.B = 241;

	COLORS.ConsoleWhite.R = 204;
	COLORS.ConsoleWhite.G = 204;
	COLORS.ConsoleWhite.B = 204;

	COLORS.Black.R = 38;
	COLORS.Black.G = 38;
	COLORS.Black.B = 38;

	COLORS.Green.R = 26;
	COLORS.Green.G = 188;
	COLORS.Green.B = 156;

	COLORS.Red.R = 231;
	COLORS.Red.G = 76;
	COLORS.Red.B = 60;

	COLORS.DarkRed.R = 192;
	COLORS.DarkRed.G = 57;
	COLORS.DarkRed.B = 43;

	COLORS.Blue.R = 52;
	COLORS.Blue.G = 152;
	COLORS.Blue.B = 219;

	COLORS.Purple.R = 116;
	COLORS.Purple.G = 94;
	COLORS.Purple.B = 197;

	COLORS.Orange.R = 211;
	COLORS.Orange.G = 84;
	COLORS.Orange.B = 0;

	COLORS.Front = COLORS.Black;
	COLORS.Back = COLORS.White;
}

void setInitPos()
{
	// Player Set up
	player.x = gameWidth / 2 - player.width / 2;
	player.y = gameHeight - player.height - 20;
	
	drawRectangle(0, player.y, gameWidth, player.y + player.height, COLORS.Back);

	// Ball Set up
	setFireball(false);
	ball.y = player.y - ball.height - 20;
	ball.x = gameWidth / 2 - ball.width / 2;

	ball.force_x = 0;
	ball.force_y = 0;

	ball.color = COLORS.Front;

	DrawPlayer(true);

}

#pragma endregion

#pragma region Main Menu

const int menuItemsLength = 3;
string menuItems[menuItemsLength] = { "CONTINUE", "NEW GAME", "EXIT    " };
int currentMenuItem = 0;

bool saveFileExists = false;

void printMenuItems()
{
	int startRow = consoleRows / 2 - 5;

	if (startRow < 0) startRow = 0;

	int a = consoleCols / 4 - strlen("BREAKOUT") / 2 - 1;
	gotoxy(a + 1, startRow + 1);
	cout << "\033[73;27;4;30;47;27mBREAK\033[3mOUT\033[23m\033[24;30;47m" << endl; //4;30;47;27

	for (int i = 0; i < menuItemsLength; i++)
	{
		gotoxy(a , startRow + i + 3);

		if (currentMenuItem == i)
		{
			if (i == 0 && !saveFileExists)
				cout << "\033[27;37;48;2;231;76;60m" << " " << menuItems[i] << " " << "\033[30;47;29m";
			else
				cout << "\033[7m" << " " << menuItems[i] << " " << "\033[27m";
		}
		else
			cout << " " << menuItems[i] << " ";

	}

	gotoxy(a-2, startRow + 4 + 3);
	cout << " \033[7m TAJ && AFK ";
	gotoxy(0, 0);
}

string repeatStr(string s, int times)
{
	string neww;

	for (size_t i = 0; i < times; i++)
	{
		neww += s;
	}

	return neww;
}

void PrintCenterHScores(int row, string start,  string str, string end, int division = 2)
{
	gotoxy(consoleCols / 2, row);

	int a = round((consoleCols / division - str.length() / 2));
	int b = (consoleCols / 2) - a - str.length() + ((consoleCols % 2 == 0) ? 1 : 2);

	cout << repeatStr(" ", a) << start << str << end << repeatStr(" ", b) ;
}

void PrintCenter(int row, string start, string str, string end)
{
	int a = round((consoleCols / 2 - str.length() / 2));

	gotoxy(a, row);

	cout<< start << str << end;
}

void printHighScores()
{
	int startRow = consoleRows / 2 - 3;

	cout << "\033[7m";
	for (size_t i = 0; i <= startRow; i++)
	{
		PrintCenterHScores(i, "", "", "");
	}

	PrintCenterHScores(startRow, "\033[27m", " HIGH SCORES ", "\033[7m", 4);

	PrintCenterHScores(startRow + 1, "", "", "");
	cout << "\033[7m";

	if (HighScores[0] == 0)
	{

		PrintCenterHScores(startRow + 2, "", "NO", "", 4);
		PrintCenterHScores(startRow + 3, "", "SCORES", "", 4);
		PrintCenterHScores(startRow + 4, "", "YET", "", 4);
		PrintCenterHScores(startRow + 5, "", "", "", 4);
		PrintCenterHScores(startRow + 6, "", "PLAY!", "", 4);
	}
	else
	{
		for (int i = 0; i < 5; i++)
		{
			string outputs = to_string(i + 1) + ". " + to_string(HighScores[i]);

			if(HighScores[i] != 0)
				PrintCenterHScores(startRow + i + 2, "", outputs, "", 4);
			else 
				PrintCenterHScores(startRow + i + 2, "", "", "");
		}
	}

	for (size_t i = startRow + 7; i <= consoleRows + 1; i++)
	{
		PrintCenterHScores(i, "", "", "");
	}

}
#pragma endregion

#pragma region Transitions

const float TRANSITION_SPEED = 0.5;

void TransitionCircleExpand(Color Back, Color Front)
{
	for (int i = 0; i < gameWidth / 1.5; i+=4)
	{
		drawEllipse(gameWidth / 2 - i, gameHeight / 2 - i, gameWidth / 2 + i, gameHeight / 2 + i, Front);
	}

	delay(200);

	for (int i = 0; i < gameWidth / 1.5; i+=4)
	{
		drawEllipse(gameWidth / 2 - i, gameHeight / 2 - i, gameWidth / 2 + i, gameHeight / 2 + i, Back);

	}
}

void TransitionRight(Color Back, Color Front)
{
	for (int i = 0; i < gameWidth + gameWidth / 2; i++)
	{
		drawLine(0 + i, 0, 0 + i, gameHeight, Front.R, Front.G, Front.B);

		drawLine(0 + i - gameWidth / 2, 0, 0 + i - gameWidth / 2, gameHeight, Back.R, Back.G, Back.B);
	}
}

void TransitionRightR(Color Back, Color Front)
{
	for (float i = 0; i < gameWidth + gameWidth / 2; i+= TRANSITION_SPEED)
	{
		drawLine(0 + i, 0, 0 + i, gameHeight, Front.R, Front.G, Front.B);
	}

	delay(200);

	for (float i = 0; i < gameWidth + gameWidth / 2; i += TRANSITION_SPEED)
	{
		drawLine(0 + i, 0, 0 + i, gameHeight, Back.R, Back.G, Back.B);
	}

	delay(200);

	for (float i = 0; i < gameWidth + gameWidth / 2; i += TRANSITION_SPEED)
	{
		drawLine(0 + i, 0, 0 + i, gameHeight, COLORS.Back.R, COLORS.Back.G, COLORS.Back.B);
	}
}

void TransitionUp(Color Back, Color Front)
{
	for (int i = 0; i < gameHeight + gameHeight / 2; i++)
	{
		drawLine(0, gameHeight - i, gameWidth, gameHeight - i, Front.R, Front.G, Front.B);
		drawLine(0, gameHeight - i + gameHeight / 2, gameWidth, gameHeight - i + gameHeight / 2, Back.R, Back.G, Back.B);
	}
}

void TransitionDown( Color First, Color Second)
{
	for (float i = 0; i < gameHeight + gameHeight / 2; i+= 0.5)
	{
		drawLine(0, i, gameWidth, i, First.R, First.G, First.B);
	}

	delay(200);

	for (float i = 0; i < gameHeight + gameHeight / 4; i+= 0.5)
	{
		drawLine(0, i, gameWidth, i, Second.R, Second.G, Second.B);
	}

	/*delay(200);

	for (float i = 0; i < gameHeight + gameHeight / 4; i+= 0.5)
	{
		drawLine(0, i, gameWidth, i, COLORS.Back.R, COLORS.Back.G, COLORS.Back.B);
	}*/
}
#pragma endregion

void UpdateFontSize(int rowsCount)
{
	rowsCount++;
	HWND tconsole = GetConsoleWindow();
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	GetCurrentConsoleFontEx(consoleHandle, FALSE, &cfi);
	float a = cfi.dwFontSize.Y / cfi.dwFontSize.X;

	cfi.dwFontSize.Y = ((float)gameHeight / rowsCount) / dpiRatio;


	cfi.FontWeight = FW_BOLD;
	wcscpy_s(cfi.FaceName, L"Consolas");
	SetCurrentConsoleFontEx(consoleHandle, TRUE, &cfi);
	GetCurrentConsoleFontEx(consoleHandle, FALSE, &cfi);
	float b = cfi.dwFontSize.Y / (float)cfi.dwFontSize.X;

	consoleCols = ((float)gameWidth / (float)cfi.dwFontSize.X ) / dpiRatio;
	consoleRows = ((float)gameHeight / cfi.dwFontSize.Y) / dpiRatio;

	delay(500);
}

int main()
{


	// Testing Code, will be removed
	/*HWND tconsole = GetConsoleWindow();
	HANDLE tconsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetProcessDPIAware();
	ShowWindow(tconsole, SW_SHOWMAXIMIZED);
	setUpColors();
	delay(500);

	getWindowDimensions(gameWidth, gameHeight);

	UpdateFontSize(8);

	getWindowDimensions(gameWidth, gameHeight);

	getConsoleWindowDimensions(consoleCols, consoleRows );

	gotoxy(consoleCols, consoleRows);
	cout << "a";

	delay(1000);

	cls();

	for (size_t i = 0; i <= consoleCols; i++)
	{
		gotoxy(i, 0);
		if (i == consoleCols)
			cout << "x";
		else cout << i + 1;
		delay(100);
	}

	for (size_t i = 0; i <= consoleRows; i++)
	{
		gotoxy(0, i);
		if (i == consoleRows)
			cout << "y";
		else cout << i + 1;
		delay(100);
	}

	while (true);*/

	HWND console = GetConsoleWindow();
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	srand(time(NULL));
	system("color 70");

	float dpi = GetDpiForSystem();
	float wdpi = GetDpiForWindow(console);
	
	dpiRatio = wdpi / dpi;

	SetProcessDPIAware();
	ShowWindow(console, SW_SHOWMAXIMIZED);
	//MoveWindow(console, 0, 0, 1000, 800, FALSE);
	//SetConsoleMode(consoleHandle, 
	//	ENABLE_ECHO_INPUT |
	//	ENABLE_INSERT_MODE |
	//	ENABLE_LINE_INPUT |
	//	//ENABLE_MOUSE_INPUT |
	//	ENABLE_PROCESSED_INPUT |
	//	//ENABLE_QUICK_EDIT_MODE |
	//	ENABLE_PROCESSED_OUTPUT |
	//	//ENABLE_WRAP_AT_EOL_OUTPUT |
	//	ENABLE_VIRTUAL_TERMINAL_PROCESSING |
	//	DISABLE_NEWLINE_AUTO_RETURN |
	//	ENABLE_LVB_GRID_WORLDWIDE
	//);

	delay(400);

	saveFileExists = DoesSaveFileExist();

	LoadHighScores();

	// Initialising
	getWindowDimensions(gameWidth, gameHeight);
	getConsoleWindowDimensions(consoleCols, consoleRows);
	showConsoleCursor(false);
	cls();

	setUpColors();

	UpdateFontSize(8);

	// Updating consoles color table with custom colors
	// also adjusting the buffer size to match with font size for 8 rows
	CONSOLE_SCREEN_BUFFER_INFOEX info;
	info.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	GetConsoleScreenBufferInfoEx(consoleHandle, &info);
	info.ColorTable[0] = RGB(COLORS.Black.R, COLORS.Black.G, COLORS.Black.B);
	info.ColorTable[2] = RGB(COLORS.Green.R, COLORS.Green.G, COLORS.Green.B);
	info.ColorTable[4] = RGB(COLORS.Red.R, COLORS.Red.G, COLORS.Red.B);
	info.ColorTable[7] = RGB(COLORS.White.R, COLORS.White.G, COLORS.White.B);

	info.bFullscreenSupported = true;
	info.dwSize.X = consoleCols + 1; 
	info.dwSize.Y = consoleRows + 2;
	info.srWindow.Top = 0;
	info.srWindow.Bottom = consoleRows + 1;
	info.srWindow.Left = 0;
	//info.srWindow.Right = 20;

	SetConsoleScreenBufferInfoEx(consoleHandle, &info);

	#pragma region Main Menu
	
	Clear();

	printHighScores();

	printMenuItems();

	// since the loop runs too fast
	// we handle input when it is detected multiple times
	int holder = 0; 
	
	while (true) // Menu Loop
	{
		char c = getKey();

		// TODO: remove test method
		if (onKey('r', c))
		{
			Clear();
			getWindowDimensions(gameWidth, gameHeight);
			UpdateFontSize(8);
			
			printHighScores();

			printMenuItems();
		}

		if (onKey('s', c) || onKey(KEY_DOWN, c))
		{
			holder++;
			if (holder > 1)
			{
				currentMenuItem++;
				if (currentMenuItem >= menuItemsLength) currentMenuItem = 0;

				printMenuItems();

				holder = 0;
			}

		}
		else if (onKey('w', c) || onKey(KEY_UP, c))
		{
			holder++;
			if (holder > 1)
			{
				currentMenuItem--;
				if (currentMenuItem < 0) currentMenuItem = menuItemsLength - 1;
				printMenuItems();
				holder = 0;
			}
		}
		else if (onKey('\r', c)) // on Enter press
		{
			if (currentMenuItem == 0)
			{
				if (saveFileExists) break;
				else
				{
					int a = consoleCols / 4 - strlen("BREAKOUT") / 2 - 1;
					gotoxy(a, 3);
					cout << "\033[27;37;48;2;231;76;60m" << " " << "NO  SAVE" << " " << "\033[30;47;29m";
				}
			}
			else if (currentMenuItem == 1)
			{
				//TransitionCircleExpand(COLORS.White, COLORS.Black);
				//TransitionUp(COLORS.White, COLORS.Black);
				//TransitionRight(COLORS.Purple, COLORS.Green);
				TransitionRightR(COLORS.Purple, COLORS.Green);
				break;
			}
			else if (currentMenuItem == 2) exit(0);
		}

		delay(10);
	}
	

	#pragma endregion
	

	// TODO: test and udpate
	system("color 07");
	Clear();
	UpdateFontSize(12);
	drawRectangle(0, 0, gameWidth, gameHeight, COLORS.Back);
	drawRectangle(0, 0, gameWidth, TopRowHeight, COLORS.Front);
	delay(100);

	if (currentMenuItem == 0)
	{
		LoadGameState();
	}
	else
	{
		setInitPos();
		DrawLives();

		// Bricks Size Set up
		int brickWidth = gameWidth / BricksColCount;
		int brickHeight = (gameHeight - TopRowHeight) / 2 / BricksRowCount;
		float extraSpace = (((float)gameWidth / (float)BricksColCount) - brickWidth) * BricksColCount;
		
		/*drawRectangle(0, 0, extraSpace / 2, gameHeight, COLORS.Black);
		drawRectangle(gameWidth - extraSpace / 2, 0, gameWidth, gameHeight, COLORS.Black);
		drawRectangle(0, gameHeight - 10, gameWidth, gameHeight, COLORS.Black);*/

		// Setting up intial bricks coordinates
		for (int r = 0; r < BricksRowCount; r++)
		{
			for (int c = 0; c < BricksColCount; c++)
			{
				Brick brick;

				brick.width = brickWidth ;
				brick.height = brickHeight;

				brick.x = c * brick.width + extraSpace / 2;
				brick.y = r * brick.height + TopRowHeight ;

				brick.health = (rand() % 4) ;

				bricks[r][c] = brick;
			}
		}
	
		DrawBricks();
	}


	// Stats set up
	time_t startTime = time(NULL);
	int framecount = 0;
	time_t t = time(NULL);

	holder = 0; 

	// Game Loop
	while (!GameManager.Over)
	{
		char c = getKey();

		if (c == WINDOW_FOCUS_CHANGED)
		{
			if (GameManager.paused)
			{
				delay(200);
				RedrawGame();
				GameManager.paused = false;
			}
			else
				GameManager.paused = true;
		}
		else if (onKey('p', c))
		{
			holder++;

			if (holder > 1)
			{
				GameManager.paused = !GameManager.paused;
				holder = 0;
			}
		}

		if (!GameManager.paused) // Draw only if game is not paused
		{
			DrawBall();

			DrawPlayer();

			DrawPowerUp();

			DrawScore();

			if (ball.y > gameHeight)
			{
				player.lives--;

				DrawLives();
				setInitPos();

				if (player.lives <= 0) GameManager.Over = true;
			}

			if (c == ' ' && !GameManager.started)
			{
				ball.force_x =  ball_force * cos(((rand() % 180 - 40) + 20) * 3.14 / 180);
				ball.force_y = -ball_force * sin(((rand() % 180 - 40) + 20) * 3.14 / 180);

				GameManager.started = true;
			}

			else if (onKey('d', c) || onKey(KEY_RIGHT, c)) player.force_x = player_speed;
			else if (onKey('a', c) || onKey(KEY_LEFT, c)) player.force_x = -player_speed;

			// TODO: For testing, will be removed
			else if (c == 'j') ball.force_x = -3;
			else if (c == 'l') ball.force_x = 3;
			else if (c == 'i') ball.force_y = -3;
			else if (c == 'k') ball.force_y = 3;
		}

		if (c == ';') SaveGameState();
		//else if (c == '\'') LoadGameState();

		// TODO: For testing, will be removed
		else if (c == 'f') setFireball(true);
		else if (c == 'g') setFireball(false);
		else if (c == 'r') RedrawGame();
		else if (c == 't')
		{
			gotoxy(0, 0);
			cout << "test";
		}
		else if (onKey('o', c)) GameManager.showStats = true;
		else if (c == 'c') cls();

		delay(1000 / 120);

#pragma region Statistics
		framecount++;
		if (time(NULL) > t)
		{
			if (GameManager.showStats)
			{
				gotoxy(0, 0);
				cout << "TIM: " << time(NULL) - startTime;

				gotoxy(0, 1);
				cout << "FPS: " << framecount;

				gotoxy(0, 2);
				cout << "FOR: " << ((player.force_x < 0) ? "-" : "+") << abs(player.force_x);

				gotoxy(0, 3);
				cout << "BFX: " << ((ball.force_x < 0) ? "-" : "+") << abs(ball.force_x);

				gotoxy(0, 4);
				cout << "BFY: " << ((ball.force_y < 0) ? "-" : "+") << abs(ball.force_y);

				gotoxy(0, 5);
				cout << "LFT: " << GameManager.BricksLeft;


				/*gotoxy(0, 3);
				cout << "SCO: " << GameManager.score;

				gotoxy(0, 4);
				cout << "LIV: " << player.lives;*/
			}

			framecount = 0;
			t = time(NULL);
		}

#pragma endregion
	}

	TransitionUp(COLORS.Back, COLORS.Purple);

	system("color 70");
	Clear();
	UpdateFontSize(8);
	getConsoleWindowDimensions(consoleCols, consoleRows);

	// Checking wheather player won or lost
	if (player.lives > 0) // player won
	{
		TransitionDown( COLORS.Red, COLORS.Green);

		drawRectangle(0, 0, gameWidth, gameHeight, COLORS.Green);
		system("color 27");

		gotoxy(consoleCols / 2 - strlen(" You Won! ") / 2, consoleRows / 2 - 1);
		cout << "\033[7m You Won! \033[27m" << endl;

		string scoreString = " SCORE " + to_string(GameManager.score) + " ";
		gotoxy(consoleCols / 2 - scoreString.length() / 2, consoleRows / 2 + 1);
		cout << "\033[30;47;29;7m" << scoreString << "\033[27m";

	}
	else // player lost
	{
		TransitionDown( COLORS.Green, COLORS.Red);

		drawRectangle(0, 0, gameWidth, gameHeight, COLORS.Red);

		system("color 47");

		PrintCenter(3, "\033[31;47m", " YOU LOST! ", "");

		string scoreString = " SCORE " + to_string(GameManager.score) + " ";

		if (scoreString.length() % 2 == 0)scoreString += " ";

		PrintCenter(5, "\033[30;47;29;7m", scoreString, "\033[27m");
	}

	// Updating Highscores file if the score is in top 5
	if (AddScore(GameManager.score)) SaveHighScores();

	while (true);

	cls();
	showConsoleCursor(true);

	gotoxy(0, 0);

	return 0;
}