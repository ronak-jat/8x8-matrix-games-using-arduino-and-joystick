#include <LedControl.h>

// ----------------- PIN CONFIG -----------------
#define DIN 12
#define CS  11
#define CLK 10
LedControl lc = LedControl(DIN, CLK, CS, 1);

#define VRX A0
#define VRY A1
#define SW  2

// ----------------- SNAKE STRUCT -----------------
struct Snake {
  int head[2];        // row, col
  int body[64][2];    // body segments
  int len;
  int dir[2];         // movement direction
} snake;

// ----------------- FOOD -----------------
int food[2];
bool bigFood = false;
int foodCounter = 0;

// ----------------- GAME CONTROL -----------------
bool gameOver = false;
unsigned long lastMoveTime = 0;
int moveDelay = 300;          // ms between moves

// ----------------- WALL STAY TIMER -----------------
unsigned long wallStayTimer = 0;
int wallCellRow = -1, wallCellCol = -1;

// ----------------- FRAMEBUFFER -----------------
uint8_t frame[8];

// ----------------- HELPERS -----------------
void clearFrame() {
  for (int i = 0; i < 8; ++i) frame[i] = 0;
}
void setPixel(int r, int c) {
  if (r < 0 || r >= 8 || c < 0 || c >= 8) return;
  frame[r] |= (1 << c);
}
void renderFrame() {
  for (int r = 0; r < 8; ++r) {
    lc.setRow(0, r, frame[r]);
  }
}
bool coordOnSnake(int r, int c) {
  if (snake.head[0] == r && snake.head[1] == c) return true;
  for (int i = 0; i < snake.len; ++i) {
    if (snake.body[i][0] == r && snake.body[i][1] == c) return true;
  }
  return false;
}

// ----------------- FOOD -----------------
void spawnFood() {
  if (!bigFood) {
    int tries = 0;
    do {
      food[0] = random(0, 8);
      food[1] = random(0, 8);
      tries++;
    } while (coordOnSnake(food[0], food[1]) && tries < 200);
  } else {
    int tries = 0;
    do {
      food[0] = random(0, 7);
      food[1] = random(0, 7);
      tries++;
    } while ((coordOnSnake(food[0], food[1]) ||
              coordOnSnake(food[0] + 1, food[1]) ||
              coordOnSnake(food[0], food[1] + 1) ||
              coordOnSnake(food[0] + 1, food[1] + 1)) && tries < 200);
  }
}

// ----------------- GAME LOGIC -----------------
void resetGame() {
  snake.head[0] = 4;
  snake.head[1] = 4;
  snake.len = 1;
  snake.dir[0] = 0;
  snake.dir[1] = 1;
  for (int i = 0; i < 64; ++i) snake.body[i][0] = snake.body[i][1] = -1;
  foodCounter = 0;
  bigFood = false;
  wallStayTimer = millis();
  wallCellRow = snake.head[0];
  wallCellCol = snake.head[1];
  gameOver = false;
  spawnFood();
}

// ----------------- READ JOYSTICK -----------------
void readJoystick() {
  int xVal = analogRead(VRX);
  int yVal = analogRead(VRY);
  // LEFT (previously right)
  if(xVal > 920 && snake.dir[1] == 0) {
    snake.dir[0] = 0;
    snake.dir[1] = -1;
  } 
  // RIGHT (previously left)
  else if(xVal < 100 && snake.dir[1] == 0) {
    snake.dir[0] = 0;
    snake.dir[1] = 1;
  } 
  // UP (joystick physically up → row decreases)
  else if(yVal > 920 && snake.dir[0] == 0) { // no change here
    snake.dir[0] = -1;
    snake.dir[1] = 0;
  } 
  // DOWN (joystick physically down → row increases)
  else if(yVal < 100 && snake.dir[0] == 0) { // no change here
    snake.dir[0] = 1;
    snake.dir[1] = 0;
  }
}



// ----------------- WALL STAY CHECK -----------------
void checkWallStay() {
    int hx = snake.head[0];
    int hy = snake.head[1];

    if(hx == 0 || hx == 7 || hy == 0 || hy == 7){
        if(hx == wallCellRow && hy == wallCellCol){
            if(millis() - wallStayTimer >= 500){
                gameOver = true;
            }
        } else {
            wallCellRow = hx;
            wallCellCol = hy;
            wallStayTimer = millis();
        }
    } else {
        wallCellRow = -1;
        wallCellCol = -1;
        wallStayTimer = millis();
    }
}

// ----------------- UPDATE SNAKE -----------------
bool updateSnake() {
  int nextRow = snake.head[0] + snake.dir[0];
  int nextCol = snake.head[1] + snake.dir[1];

  if(nextRow >=0 && nextRow <=7 && nextCol >=0 && nextCol <=7){
    for(int i=snake.len; i>0; i--){
      snake.body[i][0] = snake.body[i-1][0];
      snake.body[i][1] = snake.body[i-1][1];
    }
    snake.body[0][0] = snake.head[0];
    snake.body[0][1] = snake.head[1];

    snake.head[0] = nextRow;
    snake.head[1] = nextCol;

    // Self collision
    for(int i=0;i<snake.len;i++){
      if(snake.head[0] == snake.body[i][0] && snake.head[1]==snake.body[i][1]){
        return false;
      }
    }

    // Food logic
    if(!bigFood){
      if(snake.head[0] == food[0] && snake.head[1]==food[1]){
        snake.len++;
        foodCounter++;
        if(foodCounter>=5){
          bigFood = true;
        }
        spawnFood();
      }
    } else {
      bool ate=false;
      if((snake.head[0]==food[0] && snake.head[1]==food[1])||
         (snake.head[0]==food[0]+1 && snake.head[1]==food[1])||
         (snake.head[0]==food[0] && snake.head[1]==food[1]+1)||
         (snake.head[0]==food[0]+1 && snake.head[1]==food[1]+1)) ate=true;
      if(ate){
        snake.len++;
        foodCounter=0;
        bigFood=false;
        spawnFood();
      }
    }
  }

  checkWallStay();
  return true;
}

// ----------------- SETUP -----------------
void setup() {
  lc.shutdown(0,false);
  lc.setIntensity(0,6);
  lc.clearDisplay(0);

  pinMode(SW, INPUT_PULLUP);
  randomSeed(analogRead(A2)^analogRead(A3)^micros());
  resetGame();
}

// ----------------- LOOP -----------------
void loop(){
  readJoystick();

  if(!gameOver){
    if(millis()-lastMoveTime > moveDelay){
      lastMoveTime = millis();
      if(!updateSnake()) gameOver=true;
    }

    clearFrame();

    // Draw food
    if(!bigFood) setPixel(food[0],food[1]);
    else {
      setPixel(food[0],food[1]);
      setPixel(food[0]+1,food[1]);
      setPixel(food[0],food[1]+1);
      setPixel(food[0]+1,food[1]+1);
    }

    // Draw body + head
    for(int i=0;i<snake.len;i++)
      if(snake.body[i][0]>=0) setPixel(snake.body[i][0],snake.body[i][1]);
    setPixel(snake.head[0],snake.head[1]);

    renderFrame();
  } else {
    // Game over blink effect
    clearFrame();
    renderFrame();
    delay(200);

    clearFrame();
    if(!bigFood) setPixel(food[0],food[1]);
    else {
      setPixel(food[0],food[1]);
      setPixel(food[0]+1,food[1]);
      setPixel(food[0],food[1]+1);
      setPixel(food[0]+1,food[1]+1);
    }
    for(int i=0;i<snake.len;i++)
      if(snake.body[i][0]>=0) setPixel(snake.body[i][0],snake.body[i][1]);
    setPixel(snake.head[0],snake.head[1]);
    renderFrame();

    // Restart on joystick press
    unsigned long end = millis()+200;
    while(millis()<end){
      if(digitalRead(SW)==LOW){
        delay(30);
        if(digitalRead(SW)==LOW){ resetGame(); return;}
      }
    }
    if(digitalRead(SW)==LOW){
      delay(30);
      if(digitalRead(SW)==LOW) resetGame();
    }
  }
}
