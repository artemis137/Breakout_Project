/*
 * Arduino Breakout 
 *
 * This example for the Arduino screen reads the values 
 * of a joystick to move a rectangle on the x axis. The 
 * platform can intersect with a ball causing it to bounce.
 * The ball then intersects with colored blocks. When one is
 * hit, it no longer exists and the ball can hit the one's
 * behind it. This goes on till they lose all their lives,
 * or until there are no more bricks. You start the movement
 * of the ball by pressing the joystick. Also, you press the
 * joystick to restart the game once you win or lose.
 
 * Created by Meaghan Neill December 2013
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>   
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SD.h>
#include <stdlib.h>

#include "breakout.h"

/* 
 * Display pins:
 * Standard U of A library settings, assuming Atmel Mega SPI pins 
*/
#define SD_CS    5  // Chip select line for SD card
#define TFT_CS   6  // Chip select line for TFT display
#define TFT_DC   7  // Data/command line for TFT
#define TFT_RST  8  // Reset line for TFT (or connect to +5V)

/* Joystick pins and constants: */
#define joystick_vert 1 // Analog input A1 - vertical
#define joystick_button 9 // Digital input pin 9 for the button
int8_t joy_button;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

/* Variables for the position of the ball and paddle */
int paddleY, paddleX;
int oldPaddleX, oldPaddleY;
int ballX, ballY;
int oldBallX, oldBallY;
int ballDirectionX = 2;
int ballDirectionY = 5;

/* Variables for starting numbers of points and lives */
int points = 0;
int oldpoints = 0;
int lives = 3;
int oldlives = 3;

/* Create the variables for the number of bricks and
 * initialize the struct
 */
const int rows = 6;
const int columns = 6;
BrickPosition brick_position[columns][rows];

/* Declare functions used in the game  */
void initializeBricks(BrickPosition brick_position[columns][rows]);
void drawPaddle();
void drawBricks();
void lifeandScore();
void lifeCheck();
void scoreCheck();
void startPhase();
void moveBall();
boolean inPaddle(int ballX, int ballY, int paddleX, int paddleY, int paddleWidth, int paddleHeight);
boolean inBrick (int ballX, int ballY, int paddleX, int paddleY, int paddleWidth, int paddleHeight);

void setup() 
{
  /* Initialize the Serial */
  Serial.begin(9600);

  /* Initalize the TFT */
  tft.initR(INITR_BLACKTAB);

  /* Initialize Joystick */
  pinMode(joystick_button, INPUT);
  digitalWrite(joystick_button, HIGH);
  joy_button = digitalRead(joystick_button);
  
  /* Make the entire screen black */
  tft.fillScreen(ST7735_BLACK);
  
  /* Initialize the bricks and their positions */
  initializeBricks(brick_position);
}

void loop()
{
  /* Create, move, and draw the paddle */
  drawPaddle();
  
  /* Create, remove, and draw bricks */
  drawBricks();
  
  /* Create and check lives and scores */
  lifeandScore();

  /* Check if the mall is at a "starting phase". 
   * The "starting phase" will be  either the
   * beginning of the game, or when the player 
   * has lost a life. Press the joystick button 
   * to start. */
  startPhase();
  
  /* Create, remove, and draw the ball as it moves 
   * around the screen and hits the bricks. Also checks
   * if the ball hits the paddle or the bricks. */
  moveBall();
}

 /* Intialization of 2-d array called brick_position
  * using a nested for loop */
void initializeBricks(BrickPosition brick_position[columns][rows])
{
  for (int i=0; i < columns; i++)
    {
      for (int j = 0; j < rows; j++)
	{
	  brick_position[i][j].brickX = (1 + (21 * i));
	  brick_position[i][j].brickY = (15 + (6 * j));
	  brick_position[i][j].brickExists = true;
	}
    }
}

/* This function determines the paddle position and draws it
 * It does this by looking at the old one, erases it, and 
 * looks at the new one, and draws it */
void drawPaddle()
{
  /* Map the paddle's location to the joystick */
  paddleX = map(analogRead(joystick_vert), 0, 1024, 0, tft.width()-20);
  paddleY = 155;
  
  /* Set the fill color to black and erase the previous position
   * of the paddle if different from present */
  if (oldPaddleX != paddleX)
    {
      tft.fillRect(oldPaddleX, paddleY, 20, 5, ST7735_BLACK);
    }
  
  /* Draw Paddle */
  tft.fillRect(paddleX, paddleY, 20, 5, ST7735_WHITE);
  oldPaddleX = paddleX;
}

/* Draw the bricks in the positions that were previous initialized. 
 * As the ball hits each brick, it no longer "exists" and therefore
 * turns black and is no longer recongized. */
void drawBricks()
{
  for (int i=0; i<columns; i++)
    {
      for (int j = 0; j < rows; j++)
	{
	  if (brick_position[i][j].brickExists == true)
	    {	
	      tft.fillRect(brick_position[i][j].brickX, brick_position[i][j].brickY, 20, 5, ST7735_BLUE);
	    }
	  else
	    {
	      tft.fillRect(brick_position[i][j].brickX, brick_position[i][j].brickY, 20, 5, ST7735_BLACK);
	    }
	}
    }
}

/* Checks to see if there a change in the number of lives or score.
 * If there is, it removes the old one and reprints the new one on
 * the top of the screen. If you have no more lives, changes to a
 * screen stating so and you lose. If you have no more bricks, it
 * changes to a screen telling you that you have won the game. */
void lifeandscorePrint()
{
  /* Checks to see if you have lost any lives. */
  checkLives();

  /* Checks to see if you have hit any bricks and gained points */
  checkScore();

  /* Print Create the game score and lives onto the screen */
  tft.fillRect(0,10,128,1,ST7735_WHITE);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(1, 1);
  tft.print("Points:");
  tft.setCursor(79, 1);
  tft.print("Lives: ");
  tft.setCursor(45,1);
  tft.print(points);
  tft.setCursor(117,1);
  tft.print(lives);
}

/* This is what starts the beginning screen. It controls when the ball
 * first leaves the paddle when you start the game. It also controls
 * when the ball leaves the paddle if you lose a life. To do this, press
 * the joystick. */
void startPhase()
{
  while (joy_button == HIGH)
    {
      drawPaddle();
      drawBall();
      delay(100);

      /* When you press the joystick, the ball starts to move */
      if (digitalRead(joystick_button) == LOW)
	{
	  tft.fillCircle(ballX,ballY,2,ST7735_BLACK);
	}
      /* joy_button stays LOW so it does not enter into this loop
       * until you either restart the game or lose a life  */
      joy_button = LOW;
    }
}

/* Draws and removes the ball on the screen with each movement */
void drawBall()
{
  /* If it is in a starting phase, reset the ball to the middle of the screen */
  if (joy_button == HIGH)
    {
      ballX = tft.width/2;
      ballY = tft.height()-8;
    }
  /* Erase the ball's previous position */
  if (oldBallX != ballX || oldBallY != ballY)
    {
      tft.fillCircle(oldBallX, oldBallY, 2, ST7735_BLACK);
    }
  /* Draw the ball's current position */
  tft.fillCircle(ballX, ballY, 2, ST7735_WHITE);

  oldBallX = ballX;
  oldBallY = ballY;
}

/* This function determines the ball's position on the screen */
void moveBall()
{
  /* Checks if the ball goes offscreen, and if it does, reverses 
   * the direction of the ball. */
  ballOffscreen();

  /* Check if the ball and the paddle occupy the same space on the screen 
   * If they do, reverse the ball. */
  if (inPaddle(ballX, ballY, paddleX, paddleY, 20, 5))
    {
      ballDirectionX = ballDirectionX;
      ballDirectionY = -ballDirectionY;
    }
  /* Check if the ball and the bricks occupy the same space on the screen 
   * If they do, reverse the ball and erase the brick. Also, add a point for
   * each brick erased. If there is no more bricks, go to a win screen. */
  for (int i=0; i<columns ; i++)
    {
      for (int j=0; j < rows; j++)
	{
	  if (inBrick(ballX, ballY, brick_position[i][j].brickX, brick_position[i][j].brickY, 20, 5) 
	      && brick_position[i][j].brickExists)
	    {
	      ballDirectionX = ballDirectionX;
	      ballDirectionY = -ballDirectionY;
	      brick_position[i][j].brickExists = false;
	      oldpoints = points;
	      points++;
	      if (points == rows*columns)
		{
		  joy_button = HIGH;
		}
	    }
	}
    }

  /* Update the ball's position */
  ballX += ballDirectionX;
  ballY += ballDirectionY;
  
  /* Draw and erase the ball when it moves */
  drawBall();
}

/* Checks to see if you have lost any lives. If you have, it changes
 * the amount of lives to be display. Also if you lose all your lives
 * you lose the game and it changes to that screen. */
void lifeCheck()
{
  if (oldlives != lives)
    {
      tft.setTextColor(ST7735_BLACK);
      tft.setCursor(117,1);
      tft.print(oldlives);
      oldlives = lives;
    }
  while (lives == 0)
    {
      if (joy_button == HIGH)
	{
	  joy_button = LOW;
	  tft.fillScreen(ST7735_BLACK);
	  tft.setTextColor(ST7735_RED);
	  tft.setTextSize(2);
	  tft.setCursor(22, tft.height()/2-35);
	  tft.print("No more");
	  tft.setCursor(30, tft.height()/2-15);
	  tft.print("lives.");
	  tft.setCursor(5, tft.height()/2+5);
	  tft.print("Try again.");
	  tft.setCursor(25, tft.height()/2+30);
	  tft.setTextSize(1);
	  tft.print("Final Score:");
	  tft.print(points);
	}
    }
}

/* Checks to see if you have gained points. If you have, it changes
 * the amount of points to be display. Also if there are no bricks
 * to gain points from, you win the game and it changes to a screen
 * stating so. */
void scoreCheck()
{
  if (oldpoints != points)
    {
      tft.setTextColor(ST7735_BLACK);
      tft.setCursor(45,1);
      tft.print(oldpoints);
      oldpoints = points;
    }
  while (points == columns*rows)
    {
      if (joy_button == HIGH)
	{
	  joy_button = LOW;
	  tft.fillScreen(ST7735_BLACK);
	  tft.setTextColor(ST7735_RED);
	  tft.setTextSize(2);
	  tft.setCursor(20, tft.height()/2-20);
	  tft.print("You Win!");
	  tft.setCursor(25, tft.height()/2+5);
	  tft.setTextSize(1);
	  tft.print("Final Score:");
	  tft.print(points);
	}
    }
}

/* Checks to see if the ball is off screen. If it is off the screen on 
 * the left, right, or top, it reverses the ball. If it goes off the
 * screen on the bottom, you lose a life and start over with the ball
 * on the paddle. */
void ballOffscreen()
{
  if (ballX > tft.width()-1 || ballX < 0)
    {
      ballDirectionX = -ballDirectionX;
    }
  if (ballY < 16)
    {
      ballDirectionY = -ballDirectionY;
    }
  if (ballY > tft.height())
    {
      joy_button = HIGH;
      oldlives = lives;
      lives--;
    }
}

/* This function checks the position of the ball to see if it intersects with the paddle */
boolean inPaddle(int ballX, int ballY, int paddleX, int paddleY, int paddleWidth, int paddleHeight)
{
  boolean result = false;
 
  if ((ballX>= paddleX && ballX <= (paddleX + paddleWidth)) && 
      (ballY >= paddleY && ballY<= (paddleHeight + paddle)))
    {
      result = true;
    }
  return result;
}

/* This function checks the position of the ball to see if it intersects with a brick */
boolean inBrick (int ballX, int ballY, int paddleX, int paddleY, int paddleWidth, int paddleHeight)
{
  boolean result = false;
  
  if ((x>= rectX && x<= (rectX + rectWidth)) && 
      (y >= rectY && y<= (rectHeight + rectY)))
    {
      result = true;
    }
  return result;
}
