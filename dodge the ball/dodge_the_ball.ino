#include <LedControl.h>

// MAX7219 pins: DIN, CLK, CS
LedControl lc = LedControl(12, 10, 11, 1);

#define MATRIX_SIZE 8
#define MAX_BALLS 10
#define PLAYER_Y (MATRIX_SIZE - 2)   // since player is 2x2 block

// Joystick pins
const int JOY_X = A0;
const int JOY_Y = A1;
const int JOY_BTN = 2;

struct Ball {
  int x, y;
  bool active;
  unsigned long lastMove;
};

Ball balls[MAX_BALLS];

// Player position (top-left of 2x2 block)
int playerX = MATRIX_SIZE / 2;
unsigned long lastBallSpawn = 0;
unsigned long lastMoveTime = 0;

const int MOVE_INTERVAL = 150;   // player move speed
const int SPAWN_INTERVAL = 800;  // ball spawn interval
const int BALL_SPEED = 400;      // ball fall speed

bool gameRunning = false;

// -------------------------------------------------
void setup() {
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);
  pinMode(JOY_BTN, INPUT_PULLUP);   // joystick button

  randomSeed(analogRead(0)); // for random spawning
}

// -------------------------------------------------
void loop() {
  if (!gameRunning) {
    // Show "waiting" screen until button is pressed
    showStartScreen();
    if (digitalRead(JOY_BTN) == LOW) {
      delay(300); // debounce
      startGame();
    }
  } else {
    playGame();
  }
}

// -------------------------------------------------
// Start the game
void startGame() {
  lc.clearDisplay(0);
  playerX = MATRIX_SIZE / 2;
  for (int i = 0; i < MAX_BALLS; i++) {
    balls[i].active = false;
  }
  lastBallSpawn = millis();
  lastMoveTime = millis();
  gameRunning = true;
}

// -------------------------------------------------
// Main game logic
void playGame() {
  unsigned long currentMillis = millis();

  // Handle player movement
  if (currentMillis - lastMoveTime > MOVE_INTERVAL) {
    int xVal = analogRead(JOY_X);
    if (xVal < 300 && playerX > 0) playerX--;
    if (xVal > 700 && playerX < MATRIX_SIZE - 2) playerX++;  // leave space for 2x2 block
    lastMoveTime = currentMillis;
  }

  // Spawn exactly ONE ball
  if (currentMillis - lastBallSpawn > SPAWN_INTERVAL) {
    spawnSingleBall();
    lastBallSpawn = currentMillis;
  }

  // Move balls
  for (int i = 0; i < MAX_BALLS; i++) {
    if (balls[i].active && currentMillis - balls[i].lastMove > BALL_SPEED) {
      balls[i].y++;
      balls[i].lastMove = currentMillis;

      // Collision check with 2x2 player block
      if (balls[i].y >= PLAYER_Y && balls[i].y <= PLAYER_Y + 1) {
        if (balls[i].x >= playerX && balls[i].x <= playerX + 1) {
          gameOver();
          return;
        }
      }

      // Deactivate if off-screen
      if (balls[i].y >= MATRIX_SIZE) {
        balls[i].active = false;
      }
    }
  }

  // Draw everything
  drawFrame();
}

// -------------------------------------------------
// Spawn a single ball at random column
void spawnSingleBall() {
  int col = random(MATRIX_SIZE);

  for (int i = 0; i < MAX_BALLS; i++) {
    if (!balls[i].active) {
      balls[i].x = col;
      balls[i].y = 1;   // start just below the always-lit row
      balls[i].active = true;
      balls[i].lastMove = millis();
      break;
    }
  }
}

// -------------------------------------------------
// Draw player + balls + top boundary
void drawFrame() {
  lc.clearDisplay(0);

  // Draw always-lit top row
  for (int x = 0; x < MATRIX_SIZE; x++) {
    lc.setLed(0, 0, x, true);
  }

  // Draw balls
  for (int i = 0; i < MAX_BALLS; i++) {
    if (balls[i].active) {
      lc.setLed(0, balls[i].y, balls[i].x, true);
    }
  }

  // Draw player (2x2 block)
  lc.setLed(0, PLAYER_Y, playerX, true);
  lc.setLed(0, PLAYER_Y, playerX + 1, true);
  lc.setLed(0, PLAYER_Y + 1, playerX, true);
  lc.setLed(0, PLAYER_Y + 1, playerX + 1, true);
}

// -------------------------------------------------
// Game over animation
void gameOver() {
  for (int t = 0; t < 3; t++) {
    lc.clearDisplay(0);
    delay(200);
    for (int y = 0; y < MATRIX_SIZE; y++) {
      for (int x = 0; x < MATRIX_SIZE; x++) {
        lc.setLed(0, y, x, true);
      }
    }
    delay(200);
  }
  lc.clearDisplay(0);
  gameRunning = false;  // back to start screen
}

// -------------------------------------------------
// Start screen (flashing top row)
void showStartScreen() {
  static unsigned long lastFlash = 0;
  static bool on = false;

  if (millis() - lastFlash > 500) {
    on = !on;
    lc.clearDisplay(0);
    if (on) {
      for (int x = 0; x < MATRIX_SIZE; x++) {
        lc.setLed(0, 0, x, true);
      }
    }
    lastFlash = millis();
  }
}
