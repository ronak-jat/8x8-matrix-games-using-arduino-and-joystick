#include <LedControl.h>

// Function declarations (Arduino needs these)
void updateGame();
void handleInput();
void drawFrame();
void newObstacle();
void showGameOver();
void resetGame();

// MAX7219 connections
LedControl lc = LedControl(12, 10, 11, 1);

// Joystick pins
const int VRy = A1;   // vertical axis
const int SW  = 2;    // button

// Bird
int birdRow = 3;       // start middle
const int birdCol = 1; // bird stays static in column 1

// Obstacles
int obstacleCol = 7;
int gapRow;
bool gameOver = false;

// Score and difficulty system
int score = 0;
int baseGameSpeed = 350;        // Starting speed (changed to 350)
int currentGameSpeed = 350;     // Current speed (will decrease = faster)
const int speedIncrease = 50;   // Speed increase every 10 obstacles (50ms faster)
const int obstaclesPerLevel = 5; // Obstacles needed to level up (changed from 10 to 5)

// Timing - SEPARATE input from game speed
unsigned long lastUpdate = 0;
unsigned long lastInputCheck = 0;
const int inputSpeed = 50;      // Check input every 50ms (fast response)

// Gravity system
unsigned long lastGravity = 0;
const int gravityDelay = 500; // ms between gravity pulls (300-800 range)

// Jump system
const int jumpHeight = 1;     // How many rows bird jumps (1-2)
const int gravityPause = 400; // ms gravity is paused after jump (200-600)

// Joystick calibration
int yCenter = 512;
const int DEADZONE = 100;
bool lastUp = false;
bool lastDown = false;  // Add down control

void setup() {
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);
  pinMode(SW, INPUT_PULLUP);
  randomSeed(analogRead(A0));
  
  // Calibrate joystick center
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(VRy);
    delay(5);
  }
  yCenter = sum / 10;
  
  newObstacle();
  
  // Debugging
  Serial.begin(9600);
  Serial.println("=== FLAPPY BIRD CONTROLS ===");
  Serial.println("UP = Jump | DOWN = Manual Down");
  Serial.println("Button = Restart when Game Over");
  Serial.println("============================");
}

void loop() {
  if (gameOver) {
    showGameOver();
    if (digitalRead(SW) == LOW) {
      resetGame();
      delay(200); // debounce
    }
    return;
  }
  
  // Check input FREQUENTLY (every 50ms) - independent of game speed
  if (millis() - lastInputCheck >= inputSpeed) {
    lastInputCheck = millis();
    handleInput();
  }
  
  // Update game logic at current game speed
  if (millis() - lastUpdate >= currentGameSpeed) {
    lastUpdate = millis();
    updateGame();
  }
}

void handleInput() {
  int yVal = analogRead(VRy);
  
  // Manual control - bird only moves when you move joystick
  bool upPressed = (yVal < yCenter - DEADZONE);
  bool downPressed = (yVal > yCenter + DEADZONE);
  
  // Move bird up (once per press) - INSTANT RESPONSE
  if (upPressed && !lastUp) {
    birdRow -= jumpHeight;
    if (birdRow < 0) birdRow = 0;  // Don't go above top
    lastGravity = millis() - gravityDelay + gravityPause; // Pause gravity
    Serial.println("JUMP! Instant response");
    drawFrame(); // Immediate visual update
  }
  
  // Move bird down (once per press) 
  if (downPressed && !lastDown) {
    birdRow++;
    if (birdRow > 7) birdRow = 7;  // Don't go below bottom
    Serial.println("Manual down");
    drawFrame(); // Immediate visual update
  }
  
  // Update previous states
  lastUp = upPressed;
  lastDown = downPressed;
}

void updateGame() {
  // Apply gravity ONLY if enough time passed
  if (millis() - lastGravity >= gravityDelay) {
    birdRow++; // Gravity pulls down
    lastGravity = millis();
    Serial.println("Gravity applied");
  }
  
  // Check boundaries for game over
  if (birdRow <= 0 || birdRow >= 7) {
    gameOver = true;
    return;
  }
  
  // Move obstacle left
  obstacleCol--;
  
  // Collision check
  if (obstacleCol == birdCol) {
    if (birdRow < gapRow || birdRow > gapRow + 2) {
      gameOver = true;
      return;
    } else {
      // Bird successfully passed through obstacle!
      score++;
      Serial.print("SCORE: ");
      Serial.print(score);
      
      // Check if we need to increase difficulty
      if (score % obstaclesPerLevel == 0) {
        currentGameSpeed -= speedIncrease;  // Decrease delay = faster game
        if (currentGameSpeed < 100) currentGameSpeed = 100; // Minimum speed limit
        
        int level = score / obstaclesPerLevel;
        Serial.print(" | LEVEL UP! Level ");
        Serial.print(level);
        Serial.print(" | Speed: ");
        Serial.print(currentGameSpeed);
        Serial.println("ms");
      } else {
        Serial.println("");
      }
    }
  }
  
  // New obstacle
  if (obstacleCol < 0) {
    obstacleCol = 7;
    newObstacle();
  }
  
  drawFrame();
}

void drawFrame() {
  lc.clearDisplay(0);
  
  // Draw bird
  lc.setLed(0, birdRow, birdCol, true);
  
  // Draw obstacle with gap
  for (int r = 0; r < 8; r++) {
    if (r < gapRow || r > gapRow + 2) {
      lc.setLed(0, r, obstacleCol, true);
    }
  }
}

void newObstacle() {
  // Gap positioning (adjust difficulty)
  gapRow = random(1, 4);  // Range 1-3 (easier: 0-4, harder: 2-3)
}

void showGameOver() {
  lc.clearDisplay(0);
  // Blink bird position
  lc.setLed(0, birdRow, birdCol, (millis() / 300) % 2);
  
  // Print final score (only once when game over starts)
  static bool scorePrinted = false;
  if (!scorePrinted) {
    Serial.println("===============================");
    Serial.print("GAME OVER! Final Score: ");
    Serial.print(score);
    Serial.print(" | Level Reached: ");
    Serial.println(score / obstaclesPerLevel + 1);
    Serial.println("Press button to restart!");
    Serial.println("===============================");
    scorePrinted = true;
  }
  
  // Reset flag when restarting
  if (digitalRead(SW) == LOW) {
    scorePrinted = false;
  }
}

void resetGame() {
  birdRow = 3;
  obstacleCol = 7;
  score = 0;                        // Reset score
  currentGameSpeed = baseGameSpeed; // Reset speed
  newObstacle();
  gameOver = false;
  lastGravity = millis(); // Reset gravity timer
  lc.clearDisplay(0);
  Serial.println("=== GAME RESET ===");
  Serial.println("Starting fresh! Good luck!");
}
