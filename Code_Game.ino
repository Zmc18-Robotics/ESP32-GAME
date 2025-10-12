#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// Pin definitions for ILI9341
#define TFT_CS   5
#define TFT_DC   4
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_RST  22
#define TFT_MISO 19

// Pin definitions for Touch Screen (SPI XPT2046)
#define TOUCH_CS 14
// No IRQ pin used

// Pin definitions for Joystick
#define JOYSTICK_X_PIN 32    // Pin analog untuk X axis joystick
#define JOYSTICK_Y_PIN 33    // Pin analog untuk Y axis joystick
#define JOYSTICK_BTN_PIN 25  // Pin digital untuk tombol joystick

// Pin for LED backlight if needed
#define LED_PIN 2

// Pin definition for Buzzer
#define BUZZER_PIN 16

// Sound constants
#define MENU_SELECT_TONE 1000
#define SNAKE_MOVE_TONE 220
#define FOOD_EATEN_TONE 880
#define GAME_MOVE_TONE 220
#define COLLISION_TONE 880

// Joystick thresholds and values
#define JOYSTICK_THRESHOLD 1000  // Threshold untuk menentukan gerakan joystick
#define JOYSTICK_MIDPOINT 2048   // Nilai tengah joystick (berdasarkan resolusi 12-bit ADC ESP32)

// Screen dimensions in PORTRAIT orientation
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// Game constants
#define SNAKE_BLOCK_SIZE 10
#define GAME_AREA_TOP 30
#define MAX_SNAKE_LENGTH 200
#define INITIAL_SNAKE_LENGTH 3
#define GAME_SPEED_INITIAL 200 // Milliseconds between updates (lower = faster)
#define GAME_SPEED_MIN 80 // Maximum speed

// Special food types
#define FOOD_NORMAL 0
#define FOOD_GOLDEN 1
#define FOOD_BLUE 2

// Special food constants
#define GOLDEN_APPLE_DURATION 10000 // 10 seconds in milliseconds
#define GOLDEN_APPLE_SPEED_MULTIPLIER 3

// Special food constants - Updated blue apple chance
#define BLUE_APPLE_CHANCE 60 // 60% chance
#define SPECIAL_APPLE_DURATION 10000 // 10 seconds before disappearing
#define BLINK_INTERVAL 250 // Blink interval in milliseconds

// Mario star theme melody (simplified)
#define NOTE_LENGTH 150

// Add this with your other color definitions if needed
#define ILI9341_ORANGE 0xFD20

// Game state variables
int foodType = FOOD_NORMAL;
unsigned long specialEffectEndTime = 0;
bool hasSpecialEffect = false;
int applesEaten = 0;
int lastTailX = -1, lastTailY = -1;
bool newFoodPlaced = true;
bool scoreChanged = true;
unsigned long lastToneEndTime = 0;
int currentTone = 0;
const int MAX_NOTES = 15;
int starThemeNotes[MAX_NOTES] = {
  523, 523, 523, 
  784, 784, 784, 
  659, 659, 659, 
  880, 880, 880, 
  587, 587, 587
};
int starThemeNoteDurations[MAX_NOTES] = {
  150, 150, 250,  // First three notes
  150, 150, 250,  // Next three notes
  150, 150, 250,  // Next three notes
  150, 150, 250,  // Next three notes
  150, 150, 350   // Last three notes
};
int currentNote = 0;
unsigned long nextNoteTime = 0;
bool playingStarTheme = false;
int activeEffectFoodType = FOOD_NORMAL;

// Add these global variables
unsigned long specialAppleSpawnTime = 0;
bool specialAppleVisible = true;
unsigned long lastBlinkTime = 0;
bool specialAppleActive = false;
int normalFoodX, normalFoodY;

// Game state
enum GameState {
  MENU,
  GAME1_READY, // Snake Game Ready State
  GAME1,       // Snake Game
  GAME2,       // Breakout Game
  GAME3,       // Future Game 3
  GAME4,       // Future Game 4
  GAME5        // Future Game 5
};

// Direction enum
enum Direction {
  UP,
  RIGHT,
  DOWN,
  LEFT,
  NONE
};

// Initialize the TFT display and touch screen (without IRQ)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS); // Initialize without IRQ pin

// Game variables
GameState currentState = MENU;
int menuSelection = 0; // Track menu selection for button control
unsigned long lastInputTime = 0; // For input debouncing
unsigned long lastGameUpdateTime = 0; // For game speed control
bool gamePaused = false;
int score = 0;

// Snake specific variables
int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength;
Direction snakeDirection;
Direction lastDirection;
int foodX, foodY;
int gameSpeed;

// Joystick state variables
int joystickX, joystickY;
bool joystickButtonPressed = false;
bool lastJoystickButtonState = false;


//BREAKOUT
//----
//HERE
// Breakout game specific variables
#define PADDLE_WIDTH 40
#define PADDLE_HEIGHT 10
#define BALL_SIZE 6
#define BRICK_ROWS 4
#define BRICK_COLUMNS 6
#define BRICK_WIDTH 35
#define BRICK_HEIGHT 15
#define BRICK_GAP 3
#define GAME_AREA_TOP 30

// Tambahkan definisi konstanta ini di dekat konstantan game lainnya
#define MULTI_HIT_BRICK_ROW 0
#define MULTI_HIT_BRICK_HEALTH 3
#define SPECIAL_BRICK_COLOR ILI9341_BLUE

// Special brick types
#define BRICK_NORMAL 0
#define BRICK_MULTI_HIT 1
#define BRICK_EXTEND_PADDLE 2
#define BRICK_SPLIT_BALL 3
#define BRICK_EXTRA_LIFE 4

// Special brick appearance
#define EXTEND_PADDLE_COLOR ILI9341_MAGENTA
#define SPLIT_BALL_COLOR ILI9341_CYAN
#define EXTRA_LIFE_COLOR ILI9341_GREEN

// Special effect durations
#define PADDLE_EXTENSION_DURATION 10000 // 10 seconds in milliseconds

// Game level variables
int currentLevel = 1;
bool paddleExtended = false;
unsigned long paddleExtensionStartTime = 0;
int originalPaddleWidth = PADDLE_WIDTH;
int extendedPaddleWidth = PADDLE_WIDTH * 2;

// Multi-ball variables
#define MAX_BALLS 4
int ballCount = 1;
int ballsX[MAX_BALLS], ballsY[MAX_BALLS];
int ballsSpeedX[MAX_BALLS], ballsSpeedY[MAX_BALLS];
bool ballActive[MAX_BALLS];
bool multiHitBlocksReset = true;

// Brick type storage
int brickType[BRICK_ROWS][BRICK_COLUMNS];

int extraLivesInLevel = 0;
const int MAX_EXTRA_LIVES_PER_LEVEL = 2;
unsigned long effectMessageTime = 0;
const int EFFECT_MESSAGE_DURATION = 1000; // 1 second

int brickHealth[BRICK_ROWS+1][BRICK_COLUMNS]; // +1 untuk row balok biru tambahan

int paddleX;
int ballX, ballY;
int ballSpeedX, ballSpeedY;
bool bricks[BRICK_ROWS][BRICK_COLUMNS];
int lives;

//SNAKE
// Function declarations
void showMenu();
void drawButton(int x, int y, int w, int h, const char* label, bool selected);
void updateMenuSelection();
void handleMenuInput();
void initSnakeGame();
void drawSnakeGame();
void updateSnakeGame();
void handleReadyState();
void runSnakeGame();
void showMessage(const char* message);
void readJoystick();
void placeFood();
bool checkCollision(int x, int y);
void redrawEntireSnake();

// Breakout game functions
void initBreakoutGame();
void drawBreakoutGame();
void updateBreakoutGame();
void runBreakoutGame();

// Function to play button press tone
void playButtonTone() {
  tone(BUZZER_PIN, MENU_SELECT_TONE, 100);
}

// Function to play snake movement tone
void playMoveTone() {
  playToneNonBlocking(SNAKE_MOVE_TONE, 50);
}

void playCollisionTone() {
  tone(BUZZER_PIN, COLLISION_TONE, 150);
}

// Function to play food eaten tone
void playFoodTone() {
  tone(BUZZER_PIN, FOOD_EATEN_TONE, 150);
}

// Function to play game over melody
void playGameOverTone() {
  // Descending tones for game over
  int gameOverMelody[] = {660, 622, 587, 523, 494, 440, 415, 392};
  int noteDurations[] = {200, 200, 200, 200, 200, 200, 200, 400};
  
  for (int i = 0; i < 8; i++) {
    tone(BUZZER_PIN, gameOverMelody[i], noteDurations[i]);
    delay(noteDurations[i] + 50); // Add a small delay between notes
  }
  noTone(BUZZER_PIN);
}

// Function to play win melody
void playWinTone() {
  // Ascending then descending melody for winning
  int winMelody[] = {523, 587, 659, 698, 784, 880, 988, 880, 784, 698, 659, 587, 523};
  int noteDurations[] = {150, 150, 150, 150, 150, 150, 300, 150, 150, 150, 150, 150, 300};
  
  for (int i = 0; i < 13; i++) {
    tone(BUZZER_PIN, winMelody[i], noteDurations[i]);
    delay(noteDurations[i] + 30); // Add a small delay between notes
  }
  noTone(BUZZER_PIN);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup...");
  
  // Set up LED backlight if needed
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Turn backlight on
  
  // Initialize joystick button pin
  pinMode(JOYSTICK_BTN_PIN, INPUT_PULLUP);
  
  // Initialize buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Setup untuk ESP32 ADC
  analogReadResolution(12);  // Set resolusi ADC ke 12 bit (0-4095)
  
  // Initialize TFT display
  tft.begin();
  tft.setRotation(0); // Vertical orientation (portrait)
  
  // Initialize touch screen
  touch.begin();
  
  // Initialize random seed
  randomSeed(analogRead(A0));
  
  // Initial screen
  showMenu();
}

void loop() {
  switch (currentState) {
    case MENU:
      handleMenuInput();
      break;
    case GAME1_READY:
      handleReadyState();
      break;
    case GAME1:
      runSnakeGame();
      break;
    case GAME2:
      runBreakoutGame();
      break;
    case GAME3:
      // Future game implementation
      showMessage("Game 3 Coming Soon!");
      delay(2000);
      showMenu();
      break;
    case GAME4:
      // Future game implementation
      showMessage("Game 4 Coming Soon!");
      delay(2000);
      showMenu();
      break;
    case GAME5:
      // Future game implementation
      showMessage("Game 5 Coming Soon!");
      delay(2000);
      showMenu();
      break;
  }

    // Check the status of any playing tones
  checkToneStatus();
}

void readJoystick() {
  // Read joystick X and Y values
  joystickX = analogRead(JOYSTICK_X_PIN);
  joystickY = analogRead(JOYSTICK_Y_PIN);
  
  // Read joystick button state
  bool currentButtonState = digitalRead(JOYSTICK_BTN_PIN) == LOW;
  
  // Detect button press (transition from not pressed to pressed)
  joystickButtonPressed = currentButtonState && !lastJoystickButtonState;
  
  // Update the last button state for the next read
  lastJoystickButtonState = currentButtonState;
  
  // Debug output (uncomment if needed)
  // Serial.printf("Joystick: X=%d, Y=%d, Button=%d\n", joystickX, joystickY, joystickButtonPressed);
}

void showMenu() {
  currentState = MENU;
  menuSelection = 0; // Reset menu selection
  
  // Clear the screen
  tft.fillScreen(ILI9341_BLACK);
  
  // Draw title
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(40, 20);
  tft.println("ESP32 GAMES");
  
  // Draw all game buttons
  drawButton(40, 60, 160, 30, "Game 1: Snake", menuSelection == 0);
  drawButton(40, 100, 160, 30, "Game 2: Breakout", menuSelection == 1);
  drawButton(40, 140, 160, 30, "Game 3", menuSelection == 2);
  drawButton(40, 180, 160, 30, "Game 4", menuSelection == 3);
  drawButton(40, 220, 160, 30, "Game 5", menuSelection == 4);
  
  // Add button control instructions
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(30, 270);
  tft.println("Use Joystick up/down to navigate");
  tft.setCursor(30, 285);
  tft.println("Press joystick button to select");
  
  // Debug menu display
  Serial.println("Menu displayed");
}

void drawButton(int x, int y, int w, int h, const char* label, bool selected) {
  uint16_t bgColor = selected ? ILI9341_GREEN : ILI9341_BLUE;
  
  tft.fillRoundRect(x, y, w, h, 8, bgColor);
  tft.drawRoundRect(x, y, w, h, 8, ILI9341_WHITE);
  
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  
  int textWidth = strlen(label) * 6;
  int textX = x + (w - textWidth) / 2;
  int textY = y + (h - 8) / 2;
  
  tft.setCursor(textX, textY);
  tft.print(label);
}

void updateMenuSelection() {
  // Highlight the selected option
  drawButton(40, 60, 160, 30, "Game 1: Snake", menuSelection == 0);
  drawButton(40, 100, 160, 30, "Game 2: Breakout", menuSelection == 1);
  drawButton(40, 140, 160, 30, "Game 3", menuSelection == 2);
  drawButton(40, 180, 160, 30, "Game 4", menuSelection == 3);
  drawButton(40, 220, 160, 30, "Game 5", menuSelection == 4);
  
  // Debug menu selection
  Serial.print("Menu selection updated: ");
  Serial.println(menuSelection);
}

void handleMenuInput() {
  // Debounce input
  if (millis() - lastInputTime < 200) {
    return;
  }
  
  // Poll for touch input
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    
    // Map touch coordinates to screen orientation
    // Adjusted mapping for portrait mode
    int y = map(p.y, 0, 4096, 0, tft.height());
    int x = map(p.x, 0, 4096, tft.width(), 0); // Invert X axis for portrait
    
    Serial.print("Touch at: x=");
    Serial.print(x);
    Serial.print(", y=");
    Serial.println(y);
    
    // Check which game button was pressed
    if (x >= 40 && x <= 200) {
      if (y >= 60 && y <= 90) {
        menuSelection = 0;
        playButtonTone();
        initSnakeGame(); // Ubah ini - panggil initSnakeGame() bukan langsung set state
        lastInputTime = millis();
        return;
      } else if (y >= 100 && y <= 130) {
        menuSelection = 1;
        playButtonTone();
        initBreakoutGame();
        lastInputTime = millis();
        return;
      } else if (y >= 140 && y <= 170) {
        menuSelection = 2;
        playButtonTone();
        currentState = GAME3;
        lastInputTime = millis();
        return;
      } else if (y >= 180 && y <= 210) {
        menuSelection = 3;
        playButtonTone();
        currentState = GAME4;
        lastInputTime = millis();
        return;
      } else if (y >= 220 && y <= 250) {
        menuSelection = 4;
        playButtonTone();
        currentState = GAME5;
        lastInputTime = millis();
        return;
      }
    }
  }
  
  // Read joystick
  readJoystick();
  
  // Menu navigation (up/down) - INVERTED CONTROLS FOR MENU
  if (joystickY > JOYSTICK_MIDPOINT + JOYSTICK_THRESHOLD) {
    // Joystick pushed down - move selection up
    if (menuSelection > 0) {
      menuSelection--;
      playMoveTone();
      updateMenuSelection();
      lastInputTime = millis();
    }
  } 
  else if (joystickY < JOYSTICK_MIDPOINT - JOYSTICK_THRESHOLD) {
    // Joystick pushed up - move selection down
    if (menuSelection < 4) {
      menuSelection++;
      playMoveTone();
      updateMenuSelection();
      lastInputTime = millis();
    }
  }
  
  // Check joystick button for selection
  if (joystickButtonPressed) {
    // Button pressed
    playButtonTone();
    Serial.print("Selected game: ");
    Serial.println(menuSelection + 1);
    
    switch (menuSelection) {
      case 0:
        initSnakeGame(); // Ubah ini - panggil initSnakeGame() bukan langsung set state
        break;
      case 1:
        initBreakoutGame();
        break;
      case 2:
        currentState = GAME3;
        break;
      case 3:
        currentState = GAME4;
        break;
      case 4:
        currentState = GAME5;
        break;
    }
    
    lastInputTime = millis();
  }
  
  delay(20); // Short delay to prevent CPU hogging
}

void showMessage(const char* message) {
  // Display a centered message on the screen
  tft.fillRect(30, 140, 180, 40, ILI9341_BLUE);
  tft.drawRect(30, 140, 180, 40, ILI9341_WHITE);
  
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  
  int textWidth = strlen(message) * 12;
  int textX = (SCREEN_WIDTH - textWidth) / 2;
  
  tft.setCursor(textX, 155);
  tft.print(message);
}

void initSnakeGame() {
  // Set to ready state first
  currentState = GAME1_READY;
  Serial.println("Snake game initialized");
  
  // Initialize game variables
  score = 0;
  snakeLength = INITIAL_SNAKE_LENGTH;
  snakeDirection = NONE; // Start with no direction
  lastDirection = NONE;
  gameSpeed = GAME_SPEED_INITIAL;
  gamePaused = false;
  foodType = FOOD_NORMAL;
  hasSpecialEffect = false;
  specialEffectEndTime = 0;
  applesEaten = 0;

  // Reset drawing flags
  lastTailX = -1;
  lastTailY = -1;
  newFoodPlaced = true;
  scoreChanged = true;
  
  // Clear the screen
  tft.fillScreen(ILI9341_BLACK);
  
  // Draw game area border
  tft.drawRect(0, GAME_AREA_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - GAME_AREA_TOP, ILI9341_WHITE);
  
  // Draw score
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.print("Score: 0");
  
  // Initialize snake in the middle of the screen
  int centerX = (SCREEN_WIDTH / SNAKE_BLOCK_SIZE) / 2;
  int centerY = ((SCREEN_HEIGHT - GAME_AREA_TOP) / SNAKE_BLOCK_SIZE) / 2 + (GAME_AREA_TOP / SNAKE_BLOCK_SIZE);
  
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = (centerX - i) * SNAKE_BLOCK_SIZE;
    snakeY[i] = centerY * SNAKE_BLOCK_SIZE;
  }
  
  // Place initial food
  placeFood();
  
  // Draw initial snake and food
  drawSnakeGame();
  
  // Add message to press button
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(20, SCREEN_HEIGHT - 20);
  tft.print("Move joystick to start");
  
  lastGameUpdateTime = millis();
  lastInputTime = millis();
}

void placeFood() {
  bool validPosition = false;
  
  // Add these static variables for tracking milestones
  static bool missedMilestone = false;
  static int lastMilestoneCheck = 0;
  
  // Check for missed milestones even if we have an active effect
  if (!missedMilestone && (score >= 100 && lastMilestoneCheck < 100 || 
                          score >= 300 && lastMilestoneCheck < 300 || 
                          score >= 500 && lastMilestoneCheck < 500 || 
                          score >= 700 && lastMilestoneCheck < 700 || 
                          score >= 900 && lastMilestoneCheck < 900 || 
                          score >= 1100 && lastMilestoneCheck < 1100)) {
    // We've crossed a milestone that needs a golden apple
    missedMilestone = true;
    lastMilestoneCheck = score;
  }
  
  // Determine food type based on state
  if (!hasSpecialEffect || missedMilestone) {
    // Always prioritize missed milestones for golden apples
    if (missedMilestone) {
      foodType = FOOD_GOLDEN;
      specialAppleActive = true;
      specialAppleSpawnTime = millis();
      specialAppleVisible = true;
      missedMilestone = false; // Reset the flag
      Serial.println("Placing Golden Apple at score milestone!");
    } else if (score == 100 || score == 300 || score == 500 || score == 700 || score == 900 || score == 1100) {
      // Golden apple at specific score milestones
      foodType = FOOD_GOLDEN;
      specialAppleActive = true;
      specialAppleSpawnTime = millis();
      specialAppleVisible = true;
      lastMilestoneCheck = score; // Update milestone check
      Serial.println("Placing Golden Apple at score milestone!");
    } else if (applesEaten > 0 && applesEaten % 7 == 0) {
      // 50% chance for blue apple every 7 regular apples eaten
      if (random(100) < 50) {
        foodType = FOOD_BLUE;
        specialAppleActive = true;
        specialAppleSpawnTime = millis();
        specialAppleVisible = true;
        Serial.println("Placing Blue Apple!");
      } else {
        foodType = FOOD_NORMAL;
        specialAppleActive = false;
      }
    } else {
      foodType = FOOD_NORMAL;
      specialAppleActive = false;
    }
  } else {
    // If special effect is active and no missed milestone, always place normal food
    foodType = FOOD_NORMAL;
    specialAppleActive = false;
    Serial.println("Special effect active - placing normal food");
  }
  
  // Try to find a position not occupied by the snake
  while (!validPosition) {
    // Random position within game area
    int foodPosX = random(1, (SCREEN_WIDTH / SNAKE_BLOCK_SIZE) - 1) * SNAKE_BLOCK_SIZE;
    int foodPosY = random((GAME_AREA_TOP / SNAKE_BLOCK_SIZE) + 1, 
                   ((SCREEN_HEIGHT - GAME_AREA_TOP) / SNAKE_BLOCK_SIZE) - 1) * SNAKE_BLOCK_SIZE;
    
    validPosition = true;
    
    // Check if food is on any part of the snake
    for (int i = 0; i < snakeLength; i++) {
      if (foodPosX == snakeX[i] && foodPosY == snakeY[i]) {
        validPosition = false;
        break;
      }
    }
    
    if (validPosition) {
      if (specialAppleActive) {
        // Place normal food
        normalFoodX = foodPosX;
        normalFoodY = foodPosY;
        
        // Find another valid position for special apple
        bool specialPositionValid = false;
        while (!specialPositionValid) {
          foodX = random(1, (SCREEN_WIDTH / SNAKE_BLOCK_SIZE) - 1) * SNAKE_BLOCK_SIZE;
          foodY = random((GAME_AREA_TOP / SNAKE_BLOCK_SIZE) + 1, 
                         ((SCREEN_HEIGHT - GAME_AREA_TOP) / SNAKE_BLOCK_SIZE) - 1) * SNAKE_BLOCK_SIZE;
          
          specialPositionValid = true;
          
          // Check if special apple is on any part of the snake
          for (int i = 0; i < snakeLength; i++) {
            if (foodX == snakeX[i] && foodY == snakeY[i]) {
              specialPositionValid = false;
              break;
            }
          }
          
          // Check if special apple overlaps with normal food
          if (foodX == normalFoodX && foodY == normalFoodY) {
            specialPositionValid = false;
          }
        }
      } else {
        // Just place normal food
        foodX = foodPosX;
        foodY = foodPosY;
      }
    }
  }
  
  // Draw normal food
  if (specialAppleActive) {
    tft.fillRect(normalFoodX, normalFoodY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, ILI9341_RED);
  } else {
    tft.fillRect(foodX, foodY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, ILI9341_RED);
  }
  
  // Draw special food if active
  if (specialAppleActive) {
    uint16_t foodColor = (foodType == FOOD_GOLDEN) ? ILI9341_YELLOW : ILI9341_BLUE;
    tft.fillRect(foodX, foodY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, foodColor);
  }

  newFoodPlaced = true;
}

void drawSnakeGame() {
  // Clear only the last tail segment instead of the entire game area
  // Only erase if lastTailX is valid and we're not in a growth frame
  if (lastTailX >= 0 && lastTailY >= 0) {
    tft.fillRect(lastTailX, lastTailY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, ILI9341_BLACK);
  }
  
  uint16_t bodyColor = ILI9341_GREEN;
  uint16_t headColor = ILI9341_CYAN;
  
  // Use activeEffectFoodType for determining snake color
  if (hasSpecialEffect) {
    if (activeEffectFoodType == FOOD_GOLDEN) {
      bodyColor = ILI9341_YELLOW;
      headColor = ILI9341_ORANGE;
      
      // Debug output
      Serial.println("Drawing snake with golden effect!");
    }
    // Removed the blue apple color change effect
  }
  
  // Draw the snake - ALL segments to ensure no gaps
  // Draw head
  tft.fillRect(snakeX[0], snakeY[0], SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, headColor);
  
  // Draw all body segments
  for (int i = 1; i < snakeLength; i++) {
    tft.fillRect(snakeX[i], snakeY[i], SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, bodyColor);
  }
  
  // Handle special apple blinking and timeout
  if (specialAppleActive) {
    unsigned long currentTime = millis();
    
    // Check if special apple should time out
    if (currentTime - specialAppleSpawnTime > SPECIAL_APPLE_DURATION) {
      // Time's up, remove special apple
      tft.fillRect(foodX, foodY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, ILI9341_BLACK);
      specialAppleActive = false;
      
      // Keep only normal food
      if (foodType != FOOD_NORMAL) {
        foodType = FOOD_NORMAL;
        foodX = normalFoodX;
        foodY = normalFoodY;
      }
      
      Serial.println("Special apple timed out");
    } else {
      // Handle blinking effect as timeout approaches
      unsigned long timeLeft = SPECIAL_APPLE_DURATION - (currentTime - specialAppleSpawnTime);
      
      // Start blinking when less than 3 seconds remain
      if (timeLeft < 3000) {
        // Blink the special food
        if (currentTime - lastBlinkTime > BLINK_INTERVAL) {
          specialAppleVisible = !specialAppleVisible;
          lastBlinkTime = currentTime;
          
          if (specialAppleVisible) {
            uint16_t foodColor = (foodType == FOOD_GOLDEN) ? ILI9341_YELLOW : ILI9341_BLUE;
            tft.fillRect(foodX, foodY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, foodColor);
          } else {
            tft.fillRect(foodX, foodY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, ILI9341_BLACK);
          }
        }
      }
    }
  }
  
  // Draw food only if it's a new one or reappearing after blinking
  if (newFoodPlaced) {
    // Draw normal food
    if (specialAppleActive) {
      tft.fillRect(normalFoodX, normalFoodY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, ILI9341_RED);
      
      // Draw special food
      if (specialAppleVisible) {
        uint16_t foodColor = (foodType == FOOD_GOLDEN) ? ILI9341_YELLOW : ILI9341_BLUE;
        tft.fillRect(foodX, foodY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, foodColor);
      }
    } else {
      tft.fillRect(foodX, foodY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, ILI9341_RED);
    }
    newFoodPlaced = false;
  }
  
  // Update score only when it changes
  if (scoreChanged) {
    tft.fillRect(0, 0, 100, 20, ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.print("Score: ");
    tft.print(score);
    scoreChanged = false;
  }
  
  // Update special effect timer
  if (hasSpecialEffect) {
    static int lastDisplayedTime = -1;
    int remainingTime = (specialEffectEndTime - millis()) / 1000;
    
    if (remainingTime != lastDisplayedTime) {
      tft.fillRect(SCREEN_WIDTH - 60, 5, 60, 10, ILI9341_BLACK);
      
      if (remainingTime > 0) {
        tft.setCursor(SCREEN_WIDTH - 60, 5);
        
        // Use activeEffectFoodType instead of foodType
        if (activeEffectFoodType == FOOD_GOLDEN) {
          tft.setTextColor(ILI9341_YELLOW);
          tft.print("SPEED: ");
          Serial.print("Remaining golden effect: ");
          Serial.println(remainingTime);
        } else if (activeEffectFoodType == FOOD_BLUE) {
          tft.setTextColor(ILI9341_BLUE);
          tft.print("Bonus");
          tft.setCursor(SCREEN_WIDTH - 60, 15); // Add second line position
          tft.print("+20 skor");
        }
        
        // Only show countdown timer for golden apple effect
        if (activeEffectFoodType == FOOD_GOLDEN) {
          tft.print(remainingTime);
          tft.print("s");
        }
        lastDisplayedTime = remainingTime;
      }
    }
  }
}

bool checkCollision(int x, int y) {
  // Check for wall collision - add a small buffer zone
  if (x < 1 || y < GAME_AREA_TOP + 1 || 
      x >= SCREEN_WIDTH - 1 || y >= SCREEN_HEIGHT - 1) {
    return true;
  }
  
  // Check for self collision (skip head)
  for (int i = 1; i < snakeLength; i++) {
    if (x == snakeX[i] && y == snakeY[i]) {
      return true;
    }
  }
  
  return false;
}

void handleReadyState() {
  // Read joystick input
  readJoystick();
  
  // Joystick detection with proper thresholds
  bool joystickUp = joystickY < (JOYSTICK_MIDPOINT - JOYSTICK_THRESHOLD);
  bool joystickDown = joystickY > (JOYSTICK_MIDPOINT + JOYSTICK_THRESHOLD);
  bool joystickLeft = joystickX < (JOYSTICK_MIDPOINT - JOYSTICK_THRESHOLD);
  bool joystickRight = joystickX > (JOYSTICK_MIDPOINT + JOYSTICK_THRESHOLD);
  
  // Check if any direction was chosen
  Direction newDirection = NONE;
  
  // INVERTED JOYSTICK CONTROLS - opposite from physical movement
  if (joystickDown) {  // Joystick pushed down, snake goes up
    newDirection = UP;
    Serial.println("Joystick DOWN - Snake moving UP");
  } 
  else if (joystickUp) {  // Joystick pushed up, snake goes down
    newDirection = DOWN;
    Serial.println("Joystick UP - Snake moving DOWN");
  }
  else if (joystickRight) {  // Joystick pushed right, snake goes left
    newDirection = LEFT;
    Serial.println("Joystick RIGHT - Snake moving LEFT");
  }
  else if (joystickLeft) {  // Joystick pushed left, snake goes right
    newDirection = RIGHT;
    Serial.println("Joystick LEFT - Snake moving RIGHT");
  }
  
  // Transition to game state if a direction was chosen
  if (newDirection != NONE) {
    // Clear the instruction text
    tft.fillRect(20, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 40, 10, ILI9341_BLACK);
    
    // Set initial direction
    snakeDirection = newDirection;
    lastDirection = newDirection;
    
    // Transition to active game state
    currentState = GAME1;
    Serial.println("Game started");
  }

  // In handleReadyState() function, near the end:
  if (newDirection != NONE) {
    // Clear the instruction text
    tft.fillRect(20, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 40, 10, ILI9341_BLACK);
    
    playButtonTone(); // Add this line
    
    // Set initial direction
    snakeDirection = newDirection;
    lastDirection = newDirection;
    
    // Transition to active game state
    currentState = GAME1;
    Serial.println("Game started");
  }
}

void runSnakeGame() {
  // Only update the game at the defined speed
  unsigned long currentTime = millis();
  
  // Calculate actual game speed based on special effects
  int effectiveGameSpeed = gameSpeed;
  if (hasSpecialEffect) {
    if (activeEffectFoodType == FOOD_GOLDEN) {
      // Golden apple - 3x speed (faster) - berarti delay menjadi 1/3 dari normal
      effectiveGameSpeed = gameSpeed / GOLDEN_APPLE_SPEED_MULTIPLIER;
    } 
    // Removed blue apple speed effect (was making game slower)
  }
  
  // Check if it's time to update based on the effective game speed
  if (currentTime - lastGameUpdateTime < effectiveGameSpeed) {
    return; // Belum waktunya update, keluar dari fungsi
  }
  
  // Read joystick for controls
  readJoystick();
  
  // Check for pause with joystick button
  if (joystickButtonPressed) {
    gamePaused = !gamePaused;
    
    if (gamePaused) {
      // Show pause message
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(2);
      tft.setCursor(SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT/2);
      tft.print("PAUSED");
    } else {
      // Clear pause message
      tft.fillRect(SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT/2, 100, 20, ILI9341_BLACK);
      
      // Redraw the game elements
      drawSnakeGame();
    }
    
    lastInputTime = millis();
    return;
  }
  
  // Don't update if paused
  if (gamePaused) {
    return;
  }
  
  // Joystick detection with proper thresholds
  bool joystickUp = joystickY < (JOYSTICK_MIDPOINT - JOYSTICK_THRESHOLD);
  bool joystickDown = joystickY > (JOYSTICK_MIDPOINT + JOYSTICK_THRESHOLD);
  bool joystickLeft = joystickX < (JOYSTICK_MIDPOINT - JOYSTICK_THRESHOLD);
  bool joystickRight = joystickX > (JOYSTICK_MIDPOINT + JOYSTICK_THRESHOLD);
  
  // INVERTED CONTROLS - prevent 180 degree turns
  if (joystickDown && lastDirection != DOWN) {  // Joystick down = snake up
    snakeDirection = UP;
  } 
  else if (joystickUp && lastDirection != UP) {  // Joystick up = snake down
    snakeDirection = DOWN;
  }
  else if (joystickRight && lastDirection != RIGHT) {  // Joystick right = snake left
    snakeDirection = LEFT;
  }
  else if (joystickLeft && lastDirection != LEFT) {  // Joystick left = snake right
    snakeDirection = RIGHT;
  }
  
  // Save current direction for next update
  lastDirection = snakeDirection;
  
  // Don't proceed if no direction set (shouldn't happen in GAME1 state)
  if (snakeDirection == NONE) {
    return;
  }
  
  // Calculate new head position
  int newHeadX = snakeX[0];
  int newHeadY = snakeY[0];
  
  switch (snakeDirection) {
    case UP:
      newHeadY -= SNAKE_BLOCK_SIZE;
      break;
    case DOWN:
      newHeadY += SNAKE_BLOCK_SIZE;
      break;
    case LEFT:
      newHeadX -= SNAKE_BLOCK_SIZE;
      break;
    case RIGHT:
      newHeadX += SNAKE_BLOCK_SIZE;
      break;
    default:
      // No movement if no direction
      return;
  }
  
  // Check for collision
  if (checkCollision(newHeadX, newHeadY)) {
    // Game over
    playGameOverTone();
    showMessage("Game Over!");
    delay(2000);
    showMenu();
    return;
  }
  
  // Check win condition
  if (snakeLength >= MAX_SNAKE_LENGTH) {
    // Win condition - snake reached maximum length
    playWinTone();
    showMessage("You Win!");
    delay(2000);
    showMenu();
    return;
  }
  
  // Save the last tail position before moving the snake
  lastTailX = snakeX[snakeLength-1];
  lastTailY = snakeY[snakeLength-1];
  
  // Move the snake body (from tail to head)
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i-1];
    snakeY[i] = snakeY[i-1];
  }
  
  // Set new head position
  snakeX[0] = newHeadX;
  snakeY[0] = newHeadY;
  
  static unsigned long lastSoundTime = 0;

  // Play movement sound with delay
  if (millis() - lastSoundTime >= 200) { // 0.2 second delay
    playMoveTone();
    lastSoundTime = millis();
  }

  // Check if special apple should time out
  if (specialAppleActive && millis() - specialAppleSpawnTime > SPECIAL_APPLE_DURATION) {
    // Time's up, remove special apple
    tft.fillRect(foodX, foodY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, ILI9341_BLACK);
    specialAppleActive = false;
    
    // Keep only normal food
    if (foodType != FOOD_NORMAL) {
      foodType = FOOD_NORMAL;
      foodX = normalFoodX;
      foodY = normalFoodY;
    }
    
    Serial.println("Special apple timed out");
  }

  // Check for food collision
  bool ateFoodFlag = false;
  bool ateSpecialFlag = false;
  bool ateNormalFlag = false;
  
  // Check collision with special food
  if (specialAppleActive && newHeadX == foodX && newHeadY == foodY) {
    // Play food eaten sound
    playFoodTone();

    // Handle special food effects
    if (foodType == FOOD_GOLDEN) {
      // Golden apple - Speed up
      hasSpecialEffect = true;
      activeEffectFoodType = FOOD_GOLDEN; // Save effect type
      specialEffectEndTime = millis() + GOLDEN_APPLE_DURATION;
      // Play star theme immediately when golden apple is eaten
      playStarTheme();
      score += 20;
      Serial.println("Golden apple eaten! Speed boost active!");
    } else if (foodType == FOOD_BLUE) {
      // Blue apple - Now worth more points but no speed effect and no color change
      hasSpecialEffect = true;
      activeEffectFoodType = FOOD_BLUE; // Save effect type only for timer display
      specialEffectEndTime = millis() + GOLDEN_APPLE_DURATION; // Use same duration
      // Play star theme for blue apple too
      playStarTheme();
      // Add extra points for blue apple (20 instead of 10)
      score += 10; // Additional 10 points (total will be 20)
      Serial.println("Blue apple eaten! Bonus points!");
    }
    
    ateFoodFlag = true;
    ateSpecialFlag = true;
    
    // Mark special apple as gone
    specialAppleActive = false;
  }
  // Check collision with normal food
  else if ((newHeadX == foodX && newHeadY == foodY) || 
          (specialAppleActive && newHeadX == normalFoodX && newHeadY == normalFoodY)) {
    // Play food eaten sound
    playFoodTone();
    
    // Count normal apples
    applesEaten++;
    
    ateFoodFlag = true;
    
    // If we ate the normal food while special is still active
    if (specialAppleActive && newHeadX == normalFoodX && newHeadY == normalFoodY) {
      ateNormalFlag = true;
    }
  }
  
  // Handle food eaten
  if (ateFoodFlag) {
    // Grow snake
    if (snakeLength < MAX_SNAKE_LENGTH) {
      // Add new segment at the current tail position
      snakeX[snakeLength] = snakeX[snakeLength-1];
      snakeY[snakeLength] = snakeY[snakeLength-1];
      snakeLength++;
    }
    
    // Increase score (blue apple already got +10 above)
    score += 10;
    scoreChanged = true;
    
    // Increase game speed (decrease delay) for normal apples
    if (gameSpeed > GAME_SPEED_MIN) {
      gameSpeed -= 5;
    }
    
    // If we ate the normal food but special is still active
    if (ateNormalFlag) {
      // Find a new position for normal food only
      bool validPosition = false;
      
      while (!validPosition) {
        int newFoodX = random(1, (SCREEN_WIDTH / SNAKE_BLOCK_SIZE) - 1) * SNAKE_BLOCK_SIZE;
        int newFoodY = random((GAME_AREA_TOP / SNAKE_BLOCK_SIZE) + 1, 
                         ((SCREEN_HEIGHT - GAME_AREA_TOP) / SNAKE_BLOCK_SIZE) - 1) * SNAKE_BLOCK_SIZE;
        
        validPosition = true;
        
        // Check if food overlaps with snake
        for (int i = 0; i < snakeLength; i++) {
          if (newFoodX == snakeX[i] && newFoodY == snakeY[i]) {
            validPosition = false;
            break;
          }
        }
        
        // Check if food overlaps with special apple
        if (newFoodX == foodX && newFoodY == foodY) {
          validPosition = false;
        }
        
        if (validPosition) {
          normalFoodX = newFoodX;
          normalFoodY = newFoodY;
          
          // Draw new normal food
          tft.fillRect(normalFoodX, normalFoodY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, ILI9341_RED);
        }
      }
    } 
    // If we ate the special food or both are gone
    else {
      // Place new food normally
      placeFood();
    }
    
    newFoodPlaced = true;
    
    // Make sure old tail is fully erased
    tft.fillRect(lastTailX, lastTailY, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, ILI9341_BLACK);

    // Tell the drawing code not to erase the tail when growing
    lastTailX = -1;
    lastTailY = -1;
  }

  // Check if special effect has expired
  if (hasSpecialEffect && millis() > specialEffectEndTime) {
    hasSpecialEffect = false;
    activeEffectFoodType = FOOD_NORMAL; // Reset food effect to normal
    
    // Clear special effect indicator when expired
    tft.fillRect(SCREEN_WIDTH - 60, 5, 60, 20, ILI9341_BLACK); // Increased height to 20 to cover both lines
    
    // Only redraw entire snake if it was a golden apple (to remove yellow color)
    if (activeEffectFoodType == FOOD_GOLDEN) {
      redrawEntireSnake();
    }
    
    Serial.println("Special effect ended");
  }

  // Update the game display
  drawSnakeGame();
  
    // Update the star theme
  updateStarTheme();  // Add this line here at the end

  // Update the time of the last game update at the end
  lastGameUpdateTime = millis();
}

void playStarTheme() {
  // Start playing the star theme
  playingStarTheme = true;
  currentNote = 0;
  nextNoteTime = millis();
}

void playToneNonBlocking(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
  lastToneEndTime = millis() + duration;
  currentTone = frequency;
}

void checkToneStatus() {
  if (currentTone > 0 && millis() > lastToneEndTime) {
    noTone(BUZZER_PIN);
    currentTone = 0;
  }
}

void clearGameArea() {
    // Hanya bersihkan area permainan, bukan seluruh layar
    tft.fillRect(1, GAME_AREA_TOP + 1, SCREEN_WIDTH - 2, SCREEN_HEIGHT - GAME_AREA_TOP - 2, ILI9341_BLACK);
}

void updateStarTheme() {
  if (!playingStarTheme || millis() < nextNoteTime) {
    return;
  }
  
  // Play the current note
  playToneNonBlocking(starThemeNotes[currentNote], starThemeNoteDurations[currentNote]);
  
  // Move to the next note
  nextNoteTime = millis() + starThemeNoteDurations[currentNote] + 30; // Small delay between notes
  currentNote++;
  
  // Check if we've played all notes
  if (currentNote >= MAX_NOTES) {
    playingStarTheme = false;
  }
}

void redrawEntireSnake() {
  // Draw the snake - ALL segments to ensure no gaps
  // Draw head
  tft.fillRect(snakeX[0], snakeY[0], SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, ILI9341_CYAN);
  
  // Draw all body segments with normal color
  for (int i = 1; i < snakeLength; i++) {
    tft.fillRect(snakeX[i], snakeY[i], SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, ILI9341_GREEN);
  }
}

// Breakout game implementation
void initBreakoutGame() {
  currentState = GAME2;
  Serial.println("Breakout game initialized");
  
  // Initialize game variables
  if (currentLevel == 1) {
    // Only reset score on first level
    score = 0;
    lives = 3;
  }
  
  gameSpeed = 20; // milliseconds between updates
  gamePaused = false;
  paddleExtended = false;
  
  // Initialize paddle position
  paddleX = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
  
  // Initialize ball position and reset to single ball
  ballCount = 1;
  for (int i = 0; i < MAX_BALLS; i++) {
    ballsX[i] = SCREEN_WIDTH / 2;
    ballsY[i] = SCREEN_HEIGHT / 2;
    ballsSpeedX[i] = (i % 2 == 0) ? 2 : -2; // Alternate directions for split balls
    ballsSpeedY[i] = -3;
    ballActive[i] = (i == 0); // Only first ball is active initially
  }
  
  // Copy first ball values to main ball variables for compatibility
  ballX = ballsX[0];
  ballY = ballsY[0];
  ballSpeedX = ballsSpeedX[0];
  ballSpeedY = ballsSpeedY[0];
  
  // Initialize bricks with random special types
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLUMNS; c++) {
      bricks[r][c] = true; // All bricks initially visible
      
      // Determine brick type
      if (r == MULTI_HIT_BRICK_ROW && multiHitBlocksReset) {
        brickType[r][c] = BRICK_MULTI_HIT;
        brickHealth[r][c] = MULTI_HIT_BRICK_HEALTH;
      } else {
        // Random chance for special bricks
        int rnd = random(100);
        if (rnd < 5) { // 5% chance for extend paddle
          brickType[r][c] = BRICK_EXTEND_PADDLE;
          brickHealth[r][c] = 1;
        } else if (rnd < 10) { // 5% chance for split ball
          brickType[r][c] = BRICK_SPLIT_BALL;
          brickHealth[r][c] = 1;
        } else if (rnd < 15 && random(100) < 40 && extraLivesInLevel < MAX_EXTRA_LIVES_PER_LEVEL) {
          brickType[r][c] = BRICK_EXTRA_LIFE;
          brickHealth[r][c] = 1;
          extraLivesInLevel++;
        } else {
          brickType[r][c] = BRICK_NORMAL;
          brickHealth[r][c] = 1;
        }
      }
    }
  }
  
  // Clear the screen
  tft.fillScreen(ILI9341_BLACK);
  
  // Draw level indicator
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(SCREEN_WIDTH - 60, 5);
  tft.print("Level: ");
  tft.print(currentLevel);
  
  // Draw game area border
  tft.drawRect(0, GAME_AREA_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - GAME_AREA_TOP, ILI9341_WHITE);
  
  // Draw initial game state
  drawBreakoutGame();
  
  extraLivesInLevel = 0;
  multiHitBlocksReset = true;
  effectMessageTime = millis() - EFFECT_MESSAGE_DURATION; // Initialize to expired
  lastGameUpdateTime = millis();
  lastInputTime = millis();
}

//draw Breakout Game
void drawBreakoutGame() {
  // Clear the game area (except borders)
  tft.fillRect(1, GAME_AREA_TOP + 1, SCREEN_WIDTH - 2, SCREEN_HEIGHT - GAME_AREA_TOP - 2, ILI9341_BLACK);
  
  // Draw score and lives
  tft.fillRect(0, 0, SCREEN_WIDTH, GAME_AREA_TOP, ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.print("Score: ");
  tft.print(score);
  
  tft.setCursor(5, 20);
  tft.print("Lives: ");
  tft.print(lives);
  
  // Draw level indicator
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(SCREEN_WIDTH - 60, 5);
  tft.print("Level: ");
  tft.print(currentLevel);
  
  // Draw all bricks
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLUMNS; c++) {
      if (bricks[r][c] || (r == MULTI_HIT_BRICK_ROW && brickHealth[r][c] > 0)) {
        // Calculate brick position
        int brickX = c * (BRICK_WIDTH + BRICK_GAP) + 5;
        int brickY = r * (BRICK_HEIGHT + BRICK_GAP) + GAME_AREA_TOP + 10;
        
        // Different colors based on brick type
        uint16_t brickColor;
        switch (brickType[r][c]) {
          case BRICK_MULTI_HIT:
            brickColor = SPECIAL_BRICK_COLOR; // Blue for multi-hit
            break;
          case BRICK_EXTEND_PADDLE:
            brickColor = EXTEND_PADDLE_COLOR; // Magenta for paddle extension
            break;
          case BRICK_SPLIT_BALL:
            brickColor = SPLIT_BALL_COLOR; // Cyan for ball splitting
            break;
          case BRICK_EXTRA_LIFE:
            brickColor = EXTRA_LIFE_COLOR; // Green for extra life
            break;
          default:
            // Regular bricks with different colors by row
            switch (r) {
              case 0: brickColor = ILI9341_RED; break;
              case 1: brickColor = ILI9341_ORANGE; break;
              case 2: brickColor = ILI9341_YELLOW; break;
              case 3: brickColor = ILI9341_GREEN; break;
              default: brickColor = ILI9341_BLUE;
            }
        }
        
        // Draw the brick
        tft.fillRect(brickX, brickY, BRICK_WIDTH, BRICK_HEIGHT, brickColor);
        
        // Add special symbols for special bricks
        if (brickType[r][c] == BRICK_EXTEND_PADDLE) {
          // Draw wide rectangle symbol for paddle extension
          tft.fillRect(brickX + 5, brickY + BRICK_HEIGHT/2 - 1, BRICK_WIDTH - 10, 3, ILI9341_WHITE);
        } else if (brickType[r][c] == BRICK_SPLIT_BALL) {
          // Draw two small circles for ball splitting
          tft.fillCircle(brickX + BRICK_WIDTH/3, brickY + BRICK_HEIGHT/2, 2, ILI9341_WHITE);
          tft.fillCircle(brickX + 2*BRICK_WIDTH/3, brickY + BRICK_HEIGHT/2, 2, ILI9341_WHITE);
        } else if (brickType[r][c] == BRICK_EXTRA_LIFE) {
          // Draw + symbol for extra life
          tft.fillRect(brickX + BRICK_WIDTH/2 - 1, brickY + 3, 3, BRICK_HEIGHT - 6, ILI9341_WHITE);
          tft.fillRect(brickX + 3, brickY + BRICK_HEIGHT/2 - 1, BRICK_WIDTH - 6, 3, ILI9341_WHITE);
        } else if (brickType[r][c] == BRICK_MULTI_HIT) {
          // Show remaining health with small white dots
          for (int h = 0; h < brickHealth[r][c]; h++) {
            tft.fillCircle(brickX + 5 + h * 8, brickY + BRICK_HEIGHT/2, 2, ILI9341_WHITE);
          }
        }
      }
    }
  }
  
  // Draw paddle (normal or extended)
  int currentPaddleWidth = paddleExtended ? extendedPaddleWidth : originalPaddleWidth;
  tft.fillRect(paddleX, SCREEN_HEIGHT - 20, currentPaddleWidth, PADDLE_HEIGHT, ILI9341_CYAN);
  
  // Draw all active balls
  for (int i = 0; i < MAX_BALLS; i++) {
    if (ballActive[i]) {
      tft.fillCircle(ballsX[i], ballsY[i], BALL_SIZE, ILI9341_WHITE);
    }
  }
}

void updateBreakoutGame() {
  // Check if paddle extension has expired
  if (paddleExtended && millis() - paddleExtensionStartTime > PADDLE_EXTENSION_DURATION) {
    paddleExtended = false;
    // Adjust paddle position if it would overflow screen after shrinking
    if (paddleX + originalPaddleWidth > SCREEN_WIDTH) {
      paddleX = SCREEN_WIDTH - originalPaddleWidth;
    }
    
    // Clear message
    tft.fillRect(60, 20, 180, 10, ILI9341_BLACK);
  }

  // Clear effect messages after duration
  if (millis() - effectMessageTime > EFFECT_MESSAGE_DURATION) {
    tft.fillRect(60, 20, 180, 10, ILI9341_BLACK); // Clear message area
  }
  
  // Get current paddle width
  int currentPaddleWidth = paddleExtended ? extendedPaddleWidth : originalPaddleWidth;
  
  // Adjust ball speed based on score
  float speedMultiplier = 1.0;
  
  if (score >= 150) {
    speedMultiplier = 2.0; // Double speed at 150 points
  } else if (score >= 100) {
    speedMultiplier = 1.5; // 50% faster at 100 points
  } else if (score >= 50) {
    speedMultiplier = 1.2; // 20% faster at 50 points
  }
  
  // Track if any ball is still active
  bool anyBallActive = false;
  
  // Update all active balls
  for (int i = 0; i < MAX_BALLS; i++) {
    if (ballActive[i]) {
      anyBallActive = true;
      
      // Erase previous ball position
      tft.fillCircle(ballsX[i], ballsY[i], BALL_SIZE, ILI9341_BLACK);
      
      // Update ball position with speed multiplier
      ballsX[i] += ballsSpeedX[i] * speedMultiplier;
      ballsY[i] += ballsSpeedY[i] * speedMultiplier;
      
      // Wall collision detection and handling - X axis
      if (ballsX[i] <= BALL_SIZE) {
        ballsX[i] = BALL_SIZE + 1; // Move ball slightly away from wall
        ballsSpeedX[i] = abs(ballsSpeedX[i]); // Ensure speed is positive (moving right)
        playCollisionTone();
      } else if (ballsX[i] >= SCREEN_WIDTH - BALL_SIZE) {
        ballsX[i] = SCREEN_WIDTH - BALL_SIZE - 1; // Move ball slightly away from wall
        ballsSpeedX[i] = -abs(ballsSpeedX[i]); // Ensure speed is negative (moving left)
        playCollisionTone();
      }
      
      // Wall collision detection and handling - Top edge only
      if (ballsY[i] <= GAME_AREA_TOP + BALL_SIZE) {
        ballsY[i] = GAME_AREA_TOP + BALL_SIZE + 1; // Move ball slightly away from wall
        ballsSpeedY[i] = abs(ballsSpeedY[i]); // Ensure speed is positive (moving down)
        playCollisionTone();
      }
      
      // Check for paddle collision
      if (ballsY[i] >= SCREEN_HEIGHT - 20 - BALL_SIZE && 
          ballsY[i] <= SCREEN_HEIGHT - 20 + PADDLE_HEIGHT &&
          ballsX[i] >= paddleX && ballsX[i] <= paddleX + currentPaddleWidth) {
        
        // Bounce the ball
        ballsY[i] = SCREEN_HEIGHT - 20 - BALL_SIZE - 1; // Move ball above paddle to prevent sticking
        ballsSpeedY[i] = -abs(ballsSpeedY[i]); // Ensure ball moves upward
        
        // Adjust angle based on where ball hit the paddle
        int hitPosition = ballsX[i] - paddleX;
        int paddleCenter = currentPaddleWidth / 2;
        int offset = hitPosition - paddleCenter;
        
        // Adjust X speed based on hit position
        ballsSpeedX[i] = offset / 5; // Divide by 5 to keep speed reasonable
        
        // Ensure minimum horizontal movement
        if (abs(ballsSpeedX[i]) < 1) {
          ballsSpeedX[i] = (ballsSpeedX[i] >= 0) ? 1 : -1;
        }
        
        playCollisionTone();
      }
      
      // Check for brick collisions
      bool brickHit = false;

      for (int r = 0; r < BRICK_ROWS && !brickHit; r++) {
        for (int c = 0; c < BRICK_COLUMNS && !brickHit; c++) {
          if ((r == MULTI_HIT_BRICK_ROW && brickHealth[r][c] > 0) || bricks[r][c]) {
            // Calculate brick position
            int brickX = c * (BRICK_WIDTH + BRICK_GAP) + 5;
            int brickY = r * (BRICK_HEIGHT + BRICK_GAP) + GAME_AREA_TOP + 10;
            
            // Check if ball collides with this brick
            if (ballsX[i] >= brickX - BALL_SIZE && ballsX[i] <= brickX + BRICK_WIDTH + BALL_SIZE &&
                ballsY[i] >= brickY - BALL_SIZE && ballsY[i] <= brickY + BRICK_HEIGHT + BALL_SIZE) {
              brickHit = true;
              
              // Determine collision side more accurately
              bool hitTop = (ballsY[i] - ballsSpeedY[i] <= brickY - BALL_SIZE) && (ballsSpeedY[i] > 0);
              bool hitBottom = (ballsY[i] - ballsSpeedY[i] >= brickY + BRICK_HEIGHT + BALL_SIZE) && (ballsSpeedY[i] < 0);
              bool hitLeft = (ballsX[i] - ballsSpeedX[i] <= brickX - BALL_SIZE) && (ballsSpeedX[i] > 0);
              bool hitRight = (ballsX[i] - ballsSpeedX[i] >= brickX + BRICK_WIDTH + BALL_SIZE) && (ballsSpeedX[i] < 0);
              
              // Respond based on collision side
              if (hitTop) {
                ballsY[i] = brickY - BALL_SIZE - 1;
                ballsSpeedY[i] = -abs(ballsSpeedY[i]);
              } else if (hitBottom) {
                ballsY[i] = brickY + BRICK_HEIGHT + BALL_SIZE + 1;
                ballsSpeedY[i] = abs(ballsSpeedY[i]);
              } else if (hitLeft) {
                ballsX[i] = brickX - BALL_SIZE - 1;
                ballsSpeedX[i] = -abs(ballsSpeedX[i]);
              } else if (hitRight) {
                ballsX[i] = brickX + BRICK_WIDTH + BALL_SIZE + 1;
                ballsSpeedX[i] = abs(ballsSpeedX[i]);
              } else {
                // If collision side is ambiguous, default to Y direction change
                // This prevents the ball getting stuck
                ballsSpeedY[i] = -ballsSpeedY[i];
                
                // Ensure minimum vertical speed to prevent very slow movement
                if (abs(ballsSpeedY[i]) < 2) {
                  ballsSpeedY[i] = (ballsSpeedY[i] >= 0) ? 2 : -2;
                }
              }
              
              // Process based on brick type
              if (brickType[r][c] == BRICK_MULTI_HIT) {
                // Decrease brick health
                brickHealth[r][c]--;
                
                // If brick still has health, redraw it
                if (brickHealth[r][c] > 0) {
                  // Redraw the brick with updated health indicator
                  tft.fillRect(brickX, brickY, BRICK_WIDTH, BRICK_HEIGHT, SPECIAL_BRICK_COLOR);
                  for (int h = 0; h < brickHealth[r][c]; h++) {
                    tft.fillCircle(brickX + 5 + h * 8, brickY + BRICK_HEIGHT/2, 2, ILI9341_WHITE);
                  }
                } else {
                  // Brick destroyed, erase it
                  tft.fillRect(brickX, brickY, BRICK_WIDTH, BRICK_HEIGHT, ILI9341_BLACK);
                }
              } else {
                // Process special brick effects
                if (brickType[r][c] == BRICK_EXTEND_PADDLE) {
                  // Extend paddle for 10 seconds
                  paddleExtended = true;
                  paddleExtensionStartTime = millis();
                  
                  // Show effect message
                  tft.setTextColor(EXTEND_PADDLE_COLOR);
                  tft.setTextSize(1);
                  tft.setCursor(60, 20);
                  tft.print("Paddle Extended!");
                  effectMessageTime = millis();
                } 
                else if (brickType[r][c] == BRICK_SPLIT_BALL) {
                  // Split ball into up to 3 total balls
                  int newBallsNeeded = min(3 - ballCount, MAX_BALLS - ballCount);
                  
                  if (newBallsNeeded > 0) {
                    // Find inactive ball slots
                    int ballsAdded = 0;
                    for (int b = 0; b < MAX_BALLS && ballsAdded < newBallsNeeded; b++) {
                      if (!ballActive[b]) {
                        // Activate new ball
                        ballActive[b] = true;
                        ballsX[b] = ballsX[i];
                        ballsY[b] = ballsY[i];
                        
                        // Different direction for each new ball
                        if (ballsAdded == 0) {
                          ballsSpeedX[b] = -ballsSpeedX[i]; // Opposite X direction
                          ballsSpeedY[b] = ballsSpeedY[i];
                        } else {
                          ballsSpeedX[b] = ballsSpeedX[i] / 2; // Angled directions
                          ballsSpeedY[b] = -abs(ballsSpeedY[i]); // Always up initially
                        }
                        
                        ballCount++;
                        ballsAdded++;
                      }
                    }
                    
                    // Show effect message
                    tft.setTextColor(SPLIT_BALL_COLOR);
                    tft.setTextSize(1);
                    tft.setCursor(60, 20);
                    tft.print("Ball Split x");
                    tft.print(ballsAdded + 1);
                    effectMessageTime = millis();
                  }
                }
                else if (brickType[r][c] == BRICK_EXTRA_LIFE) {
                  // Give extra life
                  lives++;
                  
                  // Update lives display
                  tft.fillRect(5, 20, 50, 10, ILI9341_BLACK);
                  tft.setTextColor(ILI9341_WHITE);
                  tft.setTextSize(1);
                  tft.setCursor(5, 20);
                  tft.print("Lives: ");
                  tft.print(lives);
                  
                  // Show effect message
                  tft.setTextColor(EXTRA_LIFE_COLOR);
                  tft.setTextSize(1);
                  tft.setCursor(60, 20);
                  tft.print("Extra Life!");
                  effectMessageTime = millis();
                }
                
                // Regular brick destruction
                score += 10;
                tft.fillRect(brickX, brickY, BRICK_WIDTH, BRICK_HEIGHT, ILI9341_BLACK);
                bricks[r][c] = false;
                
                // Update score display
                tft.fillRect(5, 5, 50, 10, ILI9341_BLACK);
                tft.setTextColor(ILI9341_WHITE);
                tft.setTextSize(1);
                tft.setCursor(5, 5);
                tft.print("Score: ");
                tft.print(score);
              }
              
              playCollisionTone();
            }
          }
        }
      }
      
      // Check if ball falls below paddle
      if (ballsY[i] >= SCREEN_HEIGHT - BALL_SIZE) {
        // Deactivate this ball
        ballActive[i] = false;
        ballCount--;
        
        // Clear this ball
        tft.fillCircle(ballsX[i], ballsY[i], BALL_SIZE, ILI9341_BLACK);
      }
      
      // Draw ball at new position if still active
      if (ballActive[i]) {
        tft.fillCircle(ballsX[i], ballsY[i], BALL_SIZE, ILI9341_WHITE);
      }
    }
  }
  
  // If all balls are lost
  if (!anyBallActive) {
    // Lose a life
    lives--;
    
    // Update lives display
    tft.fillRect(0, 15, 100, 15, ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(5, 20);
    tft.print("Lives: ");
    tft.print(lives);
    
    if (lives <= 0) {
      // Game over
      playGameOverTone();
      showMessage("Game Over!");
      delay(2000);
      currentLevel = 1; // Reset level for next game
      showMenu();
      return;
    } else {
      // Reset ball position
      ballCount = 1;
      for (int i = 0; i < MAX_BALLS; i++) {
        ballsX[i] = SCREEN_WIDTH / 2;
        ballsY[i] = SCREEN_HEIGHT / 2;
        ballsSpeedX[i] = (i % 2 == 0) ? 2 : -2;
        ballsSpeedY[i] = -3;
        ballActive[i] = (i == 0); // Only first ball is active initially
      }
      
      // Copy to main ball variables for compatibility
      ballX = ballsX[0];
      ballY = ballsY[0];
      ballSpeedX = ballsSpeedX[0];
      ballSpeedY = ballsSpeedY[0];
      
      // Reset paddle position and state
      paddleExtended = false;
      paddleX = (SCREEN_WIDTH - originalPaddleWidth) / 2;
      
      // Show "Ready" message
      showMessage("Ready!");
      delay(1000);
      
      // Clear message and redraw game
      drawBreakoutGame();
    }
  }
  
  // Check if all bricks are cleared
  bool allBricksCleared = true;
  
  // Check all regular bricks
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLUMNS; c++) {
      // For multi-hit bricks, check health rather than the bricks array
      if (r == MULTI_HIT_BRICK_ROW) {
        if (brickHealth[r][c] > 0) {
          allBricksCleared = false;
          break;
        }
      } else {
        // For regular bricks, check the bricks array
        if (bricks[r][c]) {
          allBricksCleared = false;
          break;
        }
      }
    }
    
    // Exit early if we found remaining bricks
    if (!allBricksCleared) {
      break;
    }
  }
  
  // Debug output - uncomment to see brick status
  // Serial.print("All bricks cleared status: ");
  // Serial.println(allBricksCleared);
  
  if (allBricksCleared) {
    // Debug message
    Serial.println("All bricks cleared! Advancing to next level");
    
    // Level completed
    playWinTone();
    
    // Show level complete message
    char levelMessage[30];
    sprintf(levelMessage, "Level %d Complete!", currentLevel);
    showMessage(levelMessage);
    delay(1500);
    
    // Advance to next level
    currentLevel++;
    
    // Reset game for next level
    multiHitBlocksReset = true;
    extraLivesInLevel = 0;
    
    showMessage("Get Ready!");
    delay(1000);
    
    // Initialize next level
    initBreakoutGame();
    return; // Important to exit the function after level change
  }
}

// Replace the runBreakoutGame function with this improved version
void runBreakoutGame() {
  // Read joystick for controls
  readJoystick();
  
  // Check for pause with joystick button
  if (joystickButtonPressed) {
    gamePaused = !gamePaused;
    
    if (gamePaused) {
      // Show pause message
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(2);
      tft.setCursor(SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT/2);
      tft.print("PAUSED");
    } else {
      // Clear pause message
      tft.fillRect(SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT/2, 100, 20, ILI9341_BLACK);
      
      // Redraw the game elements
      drawBreakoutGame();
    }
    
    lastInputTime = millis();
    return;
  }
  
  // Don't update if paused
  if (gamePaused) {
    return;
  }
  
  // Get current paddle width based on extended state
  int currentPaddleWidth = paddleExtended ? extendedPaddleWidth : originalPaddleWidth;
  
  // Store previous paddle position to check if it actually moved
  int previousPaddleX = paddleX;
  
  // Use smaller step size for smoother movement
  int paddleStep = 2; 
  
  // Add a small dead zone to prevent jittering when joystick is near center
  int deadZone = 300;
  
  // Map joystick value to a paddle speed (0-3)
  int joystickDelta = 0;
  
  // Adjust paddle position based on joystick (left/right) with smoother control
  if (joystickX < JOYSTICK_MIDPOINT - deadZone) {
    // Calculate how far from center (for variable speed)
    int distance = JOYSTICK_MIDPOINT - joystickX;
    if (distance > JOYSTICK_THRESHOLD) {
      // Map the distance to a speed between 1-3
      joystickDelta = map(distance, deadZone, JOYSTICK_MIDPOINT, 1, 3);
      // Move paddle right (inverted control)
      paddleX = min(SCREEN_WIDTH - currentPaddleWidth, paddleX + joystickDelta);
    }
  } else if (joystickX > JOYSTICK_MIDPOINT + deadZone) {
    // Calculate how far from center (for variable speed)
    int distance = joystickX - JOYSTICK_MIDPOINT;
    if (distance > JOYSTICK_THRESHOLD) {
      // Map the distance to a speed between 1-3
      joystickDelta = map(distance, deadZone, JOYSTICK_MIDPOINT, 1, 3);
      // Move paddle left (inverted control)
      paddleX = max(0, paddleX - joystickDelta);
    }
  }
  
  // Only redraw the paddle if its position changed
  if (paddleX != previousPaddleX) {
    // Completely erase the old paddle to prevent artifacts
    tft.fillRect(previousPaddleX, SCREEN_HEIGHT - 20, currentPaddleWidth, PADDLE_HEIGHT, ILI9341_BLACK);
    
    // Draw the paddle at its new position
    tft.fillRect(paddleX, SCREEN_HEIGHT - 20, currentPaddleWidth, PADDLE_HEIGHT, ILI9341_CYAN);
  }
  
  // Only update the game at the defined speed
  if (millis() - lastGameUpdateTime >= gameSpeed) {
    updateBreakoutGame();
    lastGameUpdateTime = millis();
  }
  
  // Small delay to prevent too rapid updates
  delay(5);
}
