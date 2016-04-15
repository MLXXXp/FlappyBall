/*
  FlappyBall (aka FloatyBall) for the Arduboy
  Written by Chris Martinez, 3/5/2014
  Modified by Scott Allen, April 2016
*/

#include <Arduboy.h>
#include "bitmaps.h"

Arduboy arduboy;

// Things that make the game work the way it does
#define PIPE_ARRAY_SIZE 4  // At current settings only 3 sets of pipes can be onscreen at once
#define PIPE_GAP_MAX 32        // Maximum pipe gap
#define PIPE_GAP_MIN 20        // Minimum pipe gap
#define PIPE_GAP_REDUCE 7      // Number of points scored to reduce gap size
#define PIPE_WIDTH 12
#define PIPE_CAP_WIDTH 2
#define PIPE_CAP_HEIGHT 3      // Caps push back into the pipe, it's not added length
#define PIPE_MIN_HEIGHT 6      // Higher values center the gaps more
#define PIPE_GEN_FRAMES 32     // How many frames until a new pipe is generated
#define BALL_MOVE_FRAMES 2     // How many frames until the ball is moved
#define BALL_RADIUS 4
#define BALL_X 32              // Floaty's X Axis
#define JUMP_HEIGHT -4         // Jumping is negative because 0 is up

// Storage Vars
byte gameState = 0;
unsigned int gameScore = 0;
unsigned int gameHighScore = 0;
char pipes[2][PIPE_ARRAY_SIZE]; // Row 0 for x values, row 1 for gap location
byte pipeGap = PIPE_GAP_MAX;    // Height of gap between pipes to fly through
byte pipeReduceCount = PIPE_GAP_REDUCE; // Score tracker for pipe gap reduction
char ballY = 32;                // Floaty's height
char ballVY = 0;                // Floaty's vertical velocity
char ballFlapper = BALL_RADIUS; // Floaty's wing length
char gameScoreX = 0;
char gameScoreY = 0;
byte gameScoreRiser = 0;

// Sounds
const byte PROGMEM bing [] = {
0x90,0x30, 0,107, 0x80, 0x90,0x60, 1,244, 0x80, 0xf0};
const byte PROGMEM intro [] = {
0x90,72, 1,244, 0x80, 0x90,60, 1,244, 0x80, 0x90,64, 1,244, 0x80, 0x90,69, 1,244, 0x80, 0x90,67, 
1,244, 0x80, 0x90,60, 1,244, 0x80, 0x90,72, 1,244, 0x80, 0x90,60, 1,244, 0x80, 0x90,64, 1,244, 
0x80, 0x90,69, 1,244, 0x80, 0x90,67, 1,244, 0x80, 0x90,60, 1,244, 0x80, 0x90,69, 1,244, 0x80, 
0x90,57, 1,244, 0x80, 0x90,60, 1,244, 0x80, 0x90,65, 1,244, 0x80, 0x90,62, 1,244, 0x80, 0x90,57, 
1,244, 0x80, 0x90,69, 1,244, 0x80, 0x90,57, 1,244, 0x80, 0x90,60, 1,244, 0x80, 0x90,67, 1,244, 
0x80, 0x90,62, 1,244, 0x80, 0x90,65, 1,244, 0x80, 0x90,72, 7,208, 0x80, 0xf0};
const byte PROGMEM point [] = {
0x90,83, 0,75, 0x80, 0x90,88, 0,225, 0x80, 0xf0};
const byte PROGMEM flap [] = {
0x90,24, 0,125, 0x80, 0xf0};
const byte PROGMEM horns [] = {
0x90,60, 1,44, 0x80, 0x90,50, 0,150, 0x80, 0,150, 0x90,48, 0,150, 0x80, 0x90,55, 2,238, 
0x80, 0x90,62, 3,132, 0x80, 0x90,60, 0,37, 0x80, 0x90,59, 0,37, 0x80, 0x90,57, 0,37, 0x80, 
0x90,55, 0,37, 0x80, 0x90,53, 0,18, 0x80, 0x90,52, 0,18, 0x80, 0x90,50, 0,18, 0x80, 0x90,48, 
0,18, 0x80, 0x90,47, 0,18, 0x80, 0x90,45, 0,18, 0x80, 0x90,43, 0,18, 0x80, 0x90,41, 0,18, 
0x80, 0xf0};
const byte PROGMEM hit [] = { 
0x90,60, 0,31, 0x80, 0x90,61, 0,31, 0x80, 0x90,62, 0,31, 0x80, 0xf0};

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(30);
  arduboy.tunes.playScore (bing);
  delay(1500);
  arduboy.clear();
  arduboy.drawSlowXYBitmap(0,0,floatyball,128,64,1);
  arduboy.display();
  arduboy.tunes.playScore (intro);
  delay(500);
  arduboy.setCursor(18,55);
  arduboy.print("Press Any Button");
  arduboy.display();

  while (!arduboy.buttonsState());

  arduboy.initRandomSeed();
  delay(500);
  for (byte x = 0; x < PIPE_ARRAY_SIZE; x++) { pipes[0][x] = 0; }  // Set all pipes offscreen
}

void loop() {
  if (!arduboy.nextFrame())
    return;

  arduboy.clear();
  if (gameState == 0) {       // If the game is paused
    drawFloor();
    drawFloaty();
    if (arduboy.buttonsState()) { // Wait for a button press
      gameState = 1;          // Then start the game
      ballVY = JUMP_HEIGHT;    // And make Floaty jump
      if (arduboy.tunes.playing()) { arduboy.tunes.stopScore(); }
      arduboy.tunes.playScore (flap);
    }
  }

  if (gameState == 1) {       // If the game is playing
    if (ballVY > 0) {         // If the ball isn't already rising, check for jump
      if (arduboy.pressed(B_BUTTON) || arduboy.pressed(A_BUTTON)) {
        ballVY = JUMP_HEIGHT;  // jump
        if (arduboy.tunes.playing()) { arduboy.tunes.stopScore(); }
        arduboy.tunes.playScore (flap);
      }
    }
    if (arduboy.everyXFrames(PIPE_GEN_FRAMES)) { // Every PIPE_GEN_FRAMES worth of frames
      generatePipe();                  // Generate a pipe
    }
    if (arduboy.everyXFrames(BALL_MOVE_FRAMES)) {
      ballY += ballVY;                // Move the ball according to ballVY
      ballVY++;                       // Increase the fall rate
      if (ballY < BALL_RADIUS) {
        ballY = BALL_RADIUS;          // No clipping the top
        ballVY = 1;                   // Start Falling
      }
    }
    for (byte x = 0; x < PIPE_ARRAY_SIZE; x++) {  // For each pipe array element
      if (pipes[0][x] != 0) {           // If the pipe is active
        pipes[0][x] = pipes[0][x] - 2;  // Then move it left 2px
        if (pipes[0][x] + PIPE_WIDTH < 0) {  // If the pipe's right edge is off screen
          pipes[0][x] = 0;              // Then set it inactive
        }
        if (pipes[0][x] + PIPE_WIDTH == (BALL_X - BALL_RADIUS)) {  // If the pipe passed Floaty
          gameScore++;                  // Increment the score
          pipeReduceCount--;            // Decrement the gap reduction counter
          gameScoreX = BALL_X;                  // Load up the floating text with
          gameScoreY = ballY - BALL_RADIUS - 8; //  current ball x/y values
          gameScoreRiser = 15;          // And set it for 15 frames
          if (arduboy.tunes.playing()) { arduboy.tunes.stopScore(); }
          arduboy.tunes.playScore (point);
        }
      }
    }

    if (gameScoreRiser > 0) {  // If we have floating text
      gameScoreY--;
      if (gameScoreY >= 0) { // If the score will still be on the screen
        arduboy.setCursor(gameScoreX, gameScoreY);
        arduboy.print(gameScore);
        gameScoreX = gameScoreX - 2;
        gameScoreRiser--;
      } else {
        gameScoreRiser = 0;
      }
    }

    if (ballY + BALL_RADIUS > (HEIGHT-1)) {  // If the ball has fallen below the screen
      ballY = (HEIGHT-1) - BALL_RADIUS;      // Don't let the ball go under :O
      gameState = 2;                        // Game over. State is 2.
    }
    // Collision checking
    for (byte x = 0; x < PIPE_ARRAY_SIZE; x++) { // For each pipe array element
      if (pipes[0][x] != 0) {                 // If the pipe is active (not 0)
        if (checkPipe(x)) { gameState = 2; }  // If the check is true, game over
      }
    }

    drawPipes();
    drawFloor();
    drawFloaty();

    // Reduce pipe gaps as the game progresses
    if ((pipeGap > PIPE_GAP_MIN) && (pipeReduceCount <= 0)) {
      pipeGap--;
      pipeReduceCount = PIPE_GAP_REDUCE;  // Restart the countdown
    }
  }

  if (gameState == 2) {  // If the gameState is 2 then we draw a Game Over screen w/ score
    if (gameScore > gameHighScore) { gameHighScore = gameScore; }
    if (arduboy.tunes.playing()) { arduboy.tunes.stopScore(); }
    arduboy.display();              // Make sure final frame is drawn
    arduboy.tunes.playScore (hit);  // Hit sound
    delay(100);                     // Pause for the sound
    while (ballY + BALL_RADIUS < (HEIGHT-1)) {  // While floaty is still airborne
      if (ballVY < 0) { ballVY = 0; } // Stop any upward momentum
      ballY = ballY + ballVY;       // Fall
      ballVY++;                     // Increase falling speed
      if (ballY + BALL_RADIUS > (HEIGHT-1)) { ballY = HEIGHT - BALL_RADIUS; } // Don't fall through the floor plx
      arduboy.clear();
      drawPipes();
      drawFloor();
      drawFloaty();
      arduboy.display();
      while (!arduboy.nextFrame()) {}  // Wait for next frame
    }
    arduboy.tunes.playScore (horns);     // SOUND THE LOSER'S HORN  
    arduboy.drawRect(16,8,96,48, WHITE); // Box border
    arduboy.fillRect(17,9,94,46, BLACK); // Black out the inside
    arduboy.drawSlowXYBitmap(30,12,gameover,72,14,1);
    arduboy.setCursor(56 - getOffset(gameScore),30);
    arduboy.print(gameScore);
    arduboy.setCursor(69,30);
    arduboy.print("Score");

    arduboy.setCursor(56 - getOffset(gameHighScore),42);
    arduboy.print(gameHighScore);
    arduboy.setCursor(69,42);
    arduboy.print("High");

    arduboy.display();
    delay(1500);         // Give some time to stop pressing buttons

    while (!arduboy.buttonsState());

    gameState = 0;       // Then start the game paused
    gameScore = 0;       // Reset score to 0
    gameScoreRiser = 0;  // Clear the floating score
    for (byte x = 0; x < PIPE_ARRAY_SIZE; x++) { pipes[0][x] = 0; }  // set all pipes inactive
    ballY = 32;          // Reset ball to center
    ballVY = 0;          // With zero lift
    pipeGap = PIPE_GAP_MAX; // Reset the pipe gap height
    pipeReduceCount = PIPE_GAP_REDUCE; // Init the pipe gap reduction counter

    delay(250);          // Slight delay so input doesn't break pause
  }

  arduboy.display();  // Finally draw this thang
}

void drawFloor() {
  arduboy.drawLine(0,HEIGHT-1,WIDTH-1,HEIGHT-1,WHITE);
}

void drawFloaty() {
  ballFlapper--;
  if (ballFlapper < 0) { ballFlapper = BALL_RADIUS; }  // Flapper starts at the top of the ball
  arduboy.drawCircle(BALL_X, ballY, BALL_RADIUS, BLACK);  // Black out behind the ball
  arduboy.drawCircle(BALL_X, ballY, BALL_RADIUS, WHITE);  // Draw outline
  arduboy.drawLine(BALL_X, ballY, BALL_X - (BALL_RADIUS+1), ballY - ballFlapper, WHITE);  // Draw wing
  arduboy.drawPixel(BALL_X - (BALL_RADIUS+1), ballY - ballFlapper + 1, WHITE);  // Dot the wing
  arduboy.drawPixel(BALL_X + 1, ballY - 2, WHITE);  // Eye
}

void drawPipes() {
  for (byte x = 0; x < PIPE_ARRAY_SIZE; x++){
    if (pipes[0][x] != 0) {    // Value set to 0 if array element is inactive,
                               // otherwise it is the xvalue of the pipe's left edge
      // Pipes
      arduboy.drawRect(pipes[0][x], -1, PIPE_WIDTH, pipes[1][x], WHITE);
      arduboy.drawRect(pipes[0][x], pipes[1][x] + pipeGap, PIPE_WIDTH, HEIGHT - pipes[1][x] - pipeGap, WHITE);
      // Caps
      arduboy.drawRect(pipes[0][x] - PIPE_CAP_WIDTH, pipes[1][x] - PIPE_CAP_HEIGHT, PIPE_WIDTH + (PIPE_CAP_WIDTH*2), PIPE_CAP_HEIGHT, WHITE);
      arduboy.drawRect(pipes[0][x] - PIPE_CAP_WIDTH, pipes[1][x] + pipeGap, PIPE_WIDTH + (PIPE_CAP_WIDTH*2), PIPE_CAP_HEIGHT, WHITE);
      // Detail lines
      arduboy.drawLine(pipes[0][x]+2, 0, pipes[0][x]+2, pipes[1][x]-5, WHITE);
      arduboy.drawLine(pipes[0][x]+2, pipes[1][x] + pipeGap + 5, pipes[0][x]+2, HEIGHT - 3,WHITE);
    }
  }
}

void generatePipe() {
  for (byte x = 0; x < PIPE_ARRAY_SIZE; x++) {
    if (pipes[0][x] == 0) { // If the element is inactive
      pipes[0][x] = WIDTH;  // Then create it starting right of the screen
      pipes[1][x] = random(PIPE_MIN_HEIGHT, HEIGHT - PIPE_MIN_HEIGHT - pipeGap);
      return;
    }
  }
}

boolean checkPipe(byte x) {  // Collision detection, x is pipe to check
  byte AxA = BALL_X - (BALL_RADIUS-1);  // Hit box for floaty is a square
  byte AxB = BALL_X + (BALL_RADIUS-1);  // If the ball radius increases too much, corners
  byte AyA = ballY - (BALL_RADIUS-1);  // of the hitbox will go outside of floaty's
  byte AyB = ballY + (BALL_RADIUS-1);  // drawing
  byte BxA, BxB, ByA, ByB;
  
  // check top cylinder
  BxA = pipes[0][x];
  BxB = pipes[0][x] + PIPE_WIDTH;
  ByA = 0;
  ByB = pipes[1][x];
  if (AxA < BxB && AxB > BxA && AyA < ByB && AyB > ByA) { return true; } // Collided with top pipe
  
  // check top cap
  BxA = pipes[0][x] - PIPE_CAP_WIDTH;
  BxB = BxA + PIPE_WIDTH + (PIPE_CAP_WIDTH*2);
  ByA = pipes[1][x] - PIPE_CAP_HEIGHT;
  if (AxA < BxB && AxB > BxA && AyA < ByB && AyB > ByA) { return true; } // Collided with top cap
  
  // check bottom cylinder
  BxA = pipes[0][x];
  BxB = pipes[0][x] + PIPE_WIDTH;
  ByA = pipes[1][x] + pipeGap;
  ByB = HEIGHT-1;
  if (AxA < BxB && AxB > BxA && AyA < ByB && AyB > ByA) { return true; } // Collided with bottom pipe
  
  // check bottom cap
  BxA = pipes[0][x] - PIPE_CAP_WIDTH;
  BxB = BxA + PIPE_WIDTH + (PIPE_CAP_WIDTH*2);
  ByB = ByA + PIPE_CAP_HEIGHT;
  if (AxA < BxB && AxB > BxA && AyA < ByB && AyB > ByA) { return true; } // Collided with bottom pipe

  return false; // Nothing hits
}

byte getOffset(byte s) {
  if (s > 9999) { return 20; }
  if (s > 999) { return 15; }
  if (s > 99) { return 10; }
  if (s > 9) { return 5; }
  return 0;
}

