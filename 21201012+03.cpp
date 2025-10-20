// ---------------------- Header Files & Constants ----------------------
// Includes OpenGL/GLUT libraries for graphics rendering
// Math library for trigonometric functions (sin, cos, circles)
// Standard libraries for file I/O, vectors, time functions
// Defines M_PI constant for mathematical calculations

#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------- Game State Management ----------------------
// Enum defines all possible game screens/states
// Variables track current state, previous state for resume functionality
// Window dimensions, score, lives, high score tracking
// Game time counter and frame counter for animations

enum GameState { MENU, PLAYING, PAUSED, GAMEOVER, WIN, HELP, HIGHSCORE };
GameState gameState = MENU;
GameState previousState = MENU;

int windowWidth = 800, windowHeight = 800;
int score = 0;
int lives = 3;
int highScore = 0;
int gameTime = 0;
int frameCount = 0;

// ---------------------- Pacman Structure ----------------------
// Stores Pacman's position (x, y coordinates)
// Direction vectors (dirX, dirY) for movement
// Speed value controls how fast Pacman moves per frame

struct Pacman {
    float x, y;
    int dirX, dirY;
    float speed;
} pacman;

// ---------------------- Ghost Structure & AI ----------------------
// Each ghost has position, speed, RGB color values
// Name for identification
// Behavior type determines AI pattern (chase, ambush, patrol, random)
// Special timer for behavior timing
// isActive flag to enable/disable ghost

struct Ghost {
    float x, y;
    float speed;
    float r, g, b;
    std::string name;
    int behavior; // 0=chase, 1=ambush, 2=patrol, 3=random
    float specialTimer;
    bool isActive;
};

std::vector<Ghost> ghosts;

// ---------------------- Power-up System ----------------------
// Power-ups at specific positions with different types
// Type 0: Invincibility (eat ghosts)
// Type 1: Freeze (stop ghosts)
// Type 2: Speed boost
// Active flag and duration timer for each power-up

struct PowerUp {
    float x, y;
    int type; // 0=invincible, 1=freeze, 2=speed
    bool active;
    float duration;
};

std::vector<PowerUp> powerUps;
float powerUpTimer = 0;
int activePowerUp = -1;

// ---------------------- Game Board/Grid ----------------------
// 20x20 grid system for the maze
// Cell values: 0=empty, 1=pellet, 2=wall, 3=power-up
// Total pellets counter to check win condition

const int ROWS = 20;
const int COLS = 20;
int board[ROWS][COLS];
int totalPellets = 0;

// ---------------------- Text Rendering Functions ----------------------
// Draws text at specified coordinates using GLUT bitmap fonts
// Two sizes: regular (18pt) and small (12pt)
// Used for menus, score display, instructions

void drawText(float x, float y, const char *text) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text);
        text++;
    }
}

void drawTextSmall(float x, float y, const char *text) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *text);
        text++;
    }
}

// ---------------------- Utility Function ----------------------
// Converts integer to string for display purposes
// Used for score, lives, time display

std::string intToString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// ---------------------- High Score Persistence ----------------------
// Loads high score from file on game start
// Saves high score to file when game ends if beaten
// File: "highscore.txt" stores the best score

void loadHighScore() {
    std::ifstream file("highscore.txt");
    if (file.is_open()) {
        file >> highScore;
        file.close();
    } else {
        highScore = 0;
    }
}

void saveHighScore() {
    if (score > highScore) {
        highScore = score;
        std::ofstream file("highscore.txt");
        if (file.is_open()) {
            file << highScore;
            file.close();
        }
    }
}

// ---------------------- Board Initialization ----------------------
// Creates the maze layout with walls around borders
// Adds internal cross-shaped wall pattern
// Places pellets in all empty spaces
// Clears starting positions for Pacman and ghosts
// Places 4 power-ups in corners

void initBoard() {
    totalPellets = 0;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (i == 0 || j == 0 || i == ROWS-1 || j == COLS-1) {
                board[i][j] = 2; // wall
            } else if ((i == 10 && j >= 8 && j <= 12) || (j == 10 && i >= 8 && i <= 12)) {
                board[i][j] = 2; // internal walls
            } else {
                board[i][j] = 1; // pellet
                totalPellets++;
            }
        }
    }
    // Clear start positions
    board[1][1] = 0;
    board[ROWS-2][COLS-2] = 0;
    board[ROWS-2][1] = 0;
    board[1][COLS-2] = 0;
    board[10][10] = 0;

    // Add power-ups
    board[3][3] = 3;
    board[3][COLS-4] = 3;
    board[ROWS-4][3] = 3;
    board[ROWS-4][COLS-4] = 3;
}

// ---------------------- Ghost Initialization ----------------------
// Creates 4 ghosts with unique personalities:
// Blinky (Red): Aggressive direct chaser, fastest
// Pinky (Pink): Ambusher, tries to cut you off
// Inky (Cyan): Uses corner strategy relative to Blinky
// Clyde (Orange): Random/unpredictable movement, slowest
// Each starts at different corner position

void initGhosts() {
    ghosts.clear();

    // Blinky (Red) - Direct chaser
    Ghost blinky;
    blinky.x = COLS-2; blinky.y = ROWS-2;
    blinky.speed = 0.04f;
    blinky.r = 1.0f; blinky.g = 0.0f; blinky.b = 0.0f;
    blinky.name = "Blinky";
    blinky.behavior = 0;
    blinky.specialTimer = 0;
    blinky.isActive = true;
    ghosts.push_back(blinky);

    // Pinky (Pink) - Ambusher
    Ghost pinky;
    pinky.x = 1; pinky.y = ROWS-2;
    pinky.speed = 0.035f;
    pinky.r = 1.0f; pinky.g = 0.4f; pinky.b = 0.7f;
    pinky.name = "Pinky";
    pinky.behavior = 1;
    pinky.specialTimer = 0;
    pinky.isActive = true;
    ghosts.push_back(pinky);

    // Inky (Cyan) - Patrol/Corner
    Ghost inky;
    inky.x = COLS-2; inky.y = 1;
    inky.speed = 0.038f;
    inky.r = 0.0f; inky.g = 1.0f; inky.b = 1.0f;
    inky.name = "Inky";
    inky.behavior = 2;
    inky.specialTimer = 0;
    inky.isActive = true;
    ghosts.push_back(inky);

    // Clyde (Orange) - Random
    Ghost clyde;
    clyde.x = 10; clyde.y = 10;
    clyde.speed = 0.03f;
    clyde.r = 1.0f; clyde.g = 0.6f; clyde.b = 0.0f;
    clyde.name = "Clyde";
    clyde.behavior = 3;
    clyde.specialTimer = 0;
    clyde.isActive = true;
    ghosts.push_back(clyde);
}

// ---------------------- Power-up Initialization ----------------------
// Creates 4 power-ups at corner positions
// Mix of invincibility, freeze, and speed power-ups
// All start as active and available to collect

void initPowerUps() {
    powerUps.clear();

    PowerUp p1 = {3, 3, 0, true, 0}; // invincible
    PowerUp p2 = {COLS-4, 3, 1, true, 0}; // freeze
    PowerUp p3 = {3, ROWS-4, 2, true, 0}; // speed
    PowerUp p4 = {COLS-4, ROWS-4, 0, true, 0}; // invincible

    powerUps.push_back(p1);
    powerUps.push_back(p2);
    powerUps.push_back(p3);
    powerUps.push_back(p4);
}

// ---------------------- Pacman Rendering ----------------------
// Draws Pacman as a circular shape with mouth opening
// Uses 36 segments for smooth circle
// Changes color to cyan when invincible power-up is active
// Otherwise draws in golden yellow color
// Mouth angle creates the classic Pacman shape

void drawPacman() {
    const int SEG = 36;
    const float radius = 0.5f;
    const float mouthAngle = 40.0f * M_PI / 180.0f;

    if (activePowerUp == 0) {
        glColor3f(0.0f, 1.0f, 1.0f); // Cyan when invincible
    } else {
        glColor3f(1.0f, 0.84f, 0.0f); // Golden yellow
    }

    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(pacman.x + 0.5f, pacman.y + 0.5f);
        for (int i = 0; i <= SEG; ++i) {
            float theta = i * 2.0f * (float)M_PI / SEG;
            if (theta > mouthAngle && theta < (2.0f * M_PI - mouthAngle)) {
                glVertex2f(pacman.x + 0.5f + radius * std::cos(theta),
                           pacman.y + 0.5f + radius * std::sin(theta));
            }
        }
    glEnd();
}

// ---------------------- Ghost Rendering ----------------------
// Draws each ghost as a circle with two white eyes
// Ghost color changes to dark blue when frozen (power-up active)
// Otherwise displays in their unique color (red, pink, cyan, orange)
// Eyes are two small white circles for visual character

void drawGhost(Ghost &ghost) {
    const int SEG = 20;
    const float radius = 0.5f;

    if (activePowerUp == 1) {
        glColor3f(0.3f, 0.3f, 0.5f); // Darker when frozen
    } else {
        glColor3f(ghost.r, ghost.g, ghost.b);
    }

    glBegin(GL_POLYGON);
    for (int i = 0; i < SEG; i++) {
        float theta = i * 2.0f * M_PI / SEG;
        glVertex2f(ghost.x + 0.5f + radius * std::cos(theta),
                   ghost.y + 0.5f + radius * std::sin(theta));
    }
    glEnd();

    // Eyes
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 10; i++) {
        float theta = i * 2.0f * M_PI / 10;
        glVertex2f(ghost.x + 0.3f + 0.15f * std::cos(theta),
                   ghost.y + 0.7f + 0.15f * std::sin(theta));
    }
    glEnd();

    glBegin(GL_POLYGON);
    for (int i = 0; i < 10; i++) {
        float theta = i * 2.0f * M_PI / 10;
        glVertex2f(ghost.x + 0.7f + 0.15f * std::cos(theta),
                   ghost.y + 0.7f + 0.15f * std::sin(theta));
    }
    glEnd();
}

// ---------------------- Board/Maze Rendering ----------------------
// Draws all board elements based on grid values:
// Pellets: Small yellow squares that Pacman collects
// Walls: Purple rectangles forming the maze
// Power-ups: Magenta circles at special positions
// Loops through entire 20x20 grid and draws each cell type

void drawBoard() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j] == 1) { // pellet
                glColor3f(1.0f, 0.9f, 0.4f);
                glBegin(GL_POLYGON);
                    glVertex2f(j+0.4f, i+0.4f);
                    glVertex2f(j+0.6f, i+0.4f);
                    glVertex2f(j+0.6f, i+0.6f);
                    glVertex2f(j+0.4f, i+0.6f);
                glEnd();
            } else if (board[i][j] == 2) { // wall
                glColor3f(0.2f, 0.0f, 0.6f);
                glBegin(GL_POLYGON);
                    glVertex2f(j, i);
                    glVertex2f(j+1, i);
                    glVertex2f(j+1, i+1);
                    glVertex2f(j, i+1);
                glEnd();
            } else if (board[i][j] == 3) { // power-up
                glColor3f(1.0f, 0.0f, 1.0f); // Magenta
                glBegin(GL_POLYGON);
                for (int k = 0; k < 20; k++) {
                    float theta = k * 2.0f * M_PI / 20;
                    glVertex2f(j + 0.5f + 0.3f * std::cos(theta),
                               i + 0.5f + 0.3f * std::sin(theta));
                }
                glEnd();
            }
        }
    }
}

// ---------------------- Win Condition Check ----------------------
// Scans entire board to check if any pellets remain
// Returns true when all pellets are eaten (win condition)
// Returns false if any pellet still exists

bool allPelletsEaten() {
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            if (board[i][j] == 1) return false;
    return true;
}

// ---------------------- Game Reset Function ----------------------
// Reinitializes all game components to starting state
// Resets board, ghosts, power-ups
// Repositions Pacman to starting position (1,1)
// Resets score, lives, timers
// Returns to menu screen

void resetGame() {
    initBoard();
    initGhosts();
    initPowerUps();
    pacman.x = 1; pacman.y = 1; pacman.dirX = 0; pacman.dirY = 0;
    pacman.speed = 0.1f;
    score = 0;
    lives = 3;
    gameTime = 0;
    frameCount = 0;
    powerUpTimer = 0;
    activePowerUp = -1;
    gameState = MENU;
}

// ---------------------- Ghost AI & Movement Logic ----------------------
// Implements 4 different AI behaviors:
// Behavior 0 (Blinky): Direct chase - targets Pacman's current position
// Behavior 1 (Pinky): Ambush - targets 4 cells ahead of Pacman
// Behavior 2 (Inky): Corner - uses Blinky's position to flank
// Behavior 3 (Clyde): Random - picks random targets periodically
// Ghosts freeze when freeze power-up is active
// Ghost speed gradually increases over time for difficulty
// Collision detection with walls prevents ghost movement through barriers

void updateGhost(Ghost &ghost) {
    if (activePowerUp == 1) return; // Frozen

    ghost.specialTimer += 0.016f;

    // Increase speed over time
    if (gameTime % 30 == 0 && gameTime > 0) {
        ghost.speed += 0.001f;
    }

    float targetX = pacman.x;
    float targetY = pacman.y;

    // Different behaviors
    if (ghost.behavior == 0) { // Blinky - Direct chase
        targetX = pacman.x;
        targetY = pacman.y;
    } else if (ghost.behavior == 1) { // Pinky - Ambush (ahead of pacman)
        targetX = pacman.x + pacman.dirX * 4;
        targetY = pacman.y + pacman.dirY * 4;
    } else if (ghost.behavior == 2) { // Inky - Try to corner
        targetX = pacman.x + (pacman.x - ghosts[0].x);
        targetY = pacman.y + (pacman.y - ghosts[0].y);
    } else if (ghost.behavior == 3) { // Clyde - Random movement
        if ((int)ghost.specialTimer % 5 == 0) {
            targetX = rand() % COLS;
            targetY = rand() % ROWS;
        }
    }

    // Move toward target
    float dx = targetX - ghost.x;
    float dy = targetY - ghost.y;
    float dist = std::sqrt(dx*dx + dy*dy);

    if (dist > 0) {
        float nextX = ghost.x + (dx/dist) * ghost.speed;
        float nextY = ghost.y + (dy/dist) * ghost.speed;

        if (board[(int)nextY][(int)nextX] != 2) {
            ghost.x = nextX;
            ghost.y = nextY;
        }
    }
}

// ---------------------- Main Game Update Loop ----------------------
// Only runs when game state is PLAYING
// Frame counter and time tracking (60 FPS)
// Pacman movement with wall collision detection
// Pellet collection and scoring (+10 points per pellet)
// Power-up collection and activation (+50 points)
// Power-up timer countdown (5 second duration)
// Speed boost application/removal for speed power-up
// Updates all ghost positions using AI
// Collision detection between Pacman and ghosts:
//   - With invincibility: Ghost respawns, +100 points
//   - Without: Lose life, reset positions, check game over
// Win condition check when all pellets eaten

void updateGame() {
    if (gameState != PLAYING) return;

    frameCount++;
    if (frameCount % 60 == 0) {
        gameTime++;
    }

    // Move Pacman
    float nextX = pacman.x + pacman.dirX * pacman.speed;
    float nextY = pacman.y + pacman.dirY * pacman.speed;
    if (board[(int)nextY][(int)nextX] != 2) {
        pacman.x = nextX;
        pacman.y = nextY;
    }

    // Eat pellet
    if (board[(int)pacman.y][(int)pacman.x] == 1) {
        board[(int)pacman.y][(int)pacman.x] = 0;
        score += 10;
    }

    // Collect power-up
    if (board[(int)pacman.y][(int)pacman.x] == 3) {
        board[(int)pacman.y][(int)pacman.x] = 0;
        for (size_t i = 0; i < powerUps.size(); i++) {
            if ((int)powerUps[i].x == (int)pacman.x && (int)powerUps[i].y == (int)pacman.y && powerUps[i].active) {
                activePowerUp = powerUps[i].type;
                powerUpTimer = 5.0f; // 5 seconds
                powerUps[i].active = false;
                score += 50;

                if (activePowerUp == 2) {
                    pacman.speed = 0.15f;
                }
                break;
            }
        }
    }

    // Update power-up timer
    if (powerUpTimer > 0) {
        powerUpTimer -= 0.016f;
        if (powerUpTimer <= 0) {
            activePowerUp = -1;
            pacman.speed = 0.1f;
        }
    }

    // Move Ghosts
    for (size_t i = 0; i < ghosts.size(); i++) {
        updateGhost(ghosts[i]);
    }

    // Collision check
    for (size_t i = 0; i < ghosts.size(); i++) {
        if (std::abs(pacman.x - ghosts[i].x) < 0.6 && std::abs(pacman.y - ghosts[i].y) < 0.6) {
            if (activePowerUp == 0) {
                // Invincible - ghost respawns
                ghosts[i].x = 10;
                ghosts[i].y = 10;
                score += 100;
            } else {
                // Lose life
                lives--;
                pacman.x = 1; pacman.y = 1;
                initGhosts();
                if (lives <= 0) {
                    gameState = GAMEOVER;
                    saveHighScore();
                }
            }
        }
    }

    // Win check
    if (allPelletsEaten()) {
        gameState = WIN;
        saveHighScore();
    }
}

// ---------------------- Display/Rendering Function ----------------------
// Main rendering function called every frame
// Sets dark blue background color
// Renders different screens based on game state:
// MENU: Title, navigation options (Start, Resume, Help, High Score, Exit)
// HELP: Complete instructions, controls, ghost behaviors, power-up explanations
// HIGHSCORE: Displays best score from file
// PLAYING/PAUSED: Game board, Pacman, ghosts, HUD (score, time, lives)
//   Shows active power-up name, pause overlay
// GAMEOVER: Final score, time, menu option
// WIN: Congratulations, final score, new high score notification
// Double buffering used for smooth rendering

void display() {
    glClearColor(0.05f, 0.05f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.0f, 1.0f, 1.0f);

    if (gameState == MENU) {
        drawText(6.5f, 14.0f, "PACMAN GAME");
        drawText(6.0f, 11.0f, "Press SPACE to Start");
        drawText(6.5f, 10.0f, "Press R to Resume");
        drawText(6.5f, 9.0f,  "Press H for Help");
        drawText(5.5f, 8.0f,  "Press S for High Score");
        drawText(6.5f, 7.0f,  "Press ESC to Exit");
    }
    else if (gameState == HELP) {
        drawText(7.0f, 16.0f, "HOW TO PLAY");
        drawTextSmall(3.0f, 14.0f, "CONTROLS:");
        drawTextSmall(3.0f, 13.0f, "W/A/S/D - Move Up/Left/Down/Right");
        drawTextSmall(3.0f, 12.0f, "P - Pause, M - Menu, ESC - Exit");

        drawTextSmall(3.0f, 10.5f, "GHOSTS:");
        drawTextSmall(3.0f, 9.5f, "Blinky (Red) - Chases you directly");
        drawTextSmall(3.0f, 8.8f, "Pinky (Pink) - Ambushes ahead");
        drawTextSmall(3.0f, 8.1f, "Inky (Cyan) - Tries to corner you");
        drawTextSmall(3.0f, 7.4f, "Clyde (Orange) - Random movement");

        drawTextSmall(3.0f, 6.2f, "POWER-UPS (Magenta circles):");
        drawTextSmall(3.0f, 5.5f, "Invincible - Eat ghosts!");
        drawTextSmall(3.0f, 4.8f, "Freeze - Stops ghosts");
        drawTextSmall(3.0f, 4.1f, "Speed - Move faster");

        drawText(6.5f, 2.0f, "Press M for Menu");
    }
    else if (gameState == HIGHSCORE) {
        drawText(6.5f, 12.0f, "HIGH SCORE");
        std::string hs = "Best Score: " + intToString(highScore);
        drawText(6.0f, 10.0f, hs.c_str());
        drawText(6.5f, 8.0f, "Press M for Menu");
    }
    else if (gameState == PLAYING || gameState == PAUSED) {
        drawBoard();
        drawPacman();
        for (size_t i = 0; i < ghosts.size(); i++) {
            drawGhost(ghosts[i]);
        }

        std::string scoreText = "Score: " + intToString(score);
        std::string livesText = "Lives: " + intToString(lives);
        std::string timeText = "Time: " + intToString(gameTime) + "s";
        drawTextSmall(0.5f, 19.5f, scoreText.c_str());
        drawTextSmall(7.0f, 19.5f, timeText.c_str());
        drawTextSmall(14.0f, 19.5f, livesText.c_str());

        if (activePowerUp >= 0) {
            std::string powerText = "POWER: ";
            if (activePowerUp == 0) powerText += "INVINCIBLE!";
            else if (activePowerUp == 1) powerText += "FREEZE!";
            else if (activePowerUp == 2) powerText += "SPEED!";
            drawTextSmall(6.0f, 0.5f, powerText.c_str());
        }

        if (gameState == PAUSED) {
            drawText(5.5f, 10.0f, "PAUSED - Press P to Resume");
        }
    }
    else if (gameState == GAMEOVER) {
        drawText(7.0f, 13.0f, "GAME OVER!");
        std::string finalScore = "Final Score: " + intToString(score);
        std::string finalTime = "Time: " + intToString(gameTime) + " seconds";
        drawText(6.0f, 11.0f, finalScore.c_str());
        drawText(6.0f, 10.0f, finalTime.c_str());
        drawText(6.5f, 8.0f, "Press M for Menu");
    }
    else if (gameState == WIN) {
        drawText(7.5f, 13.0f, "YOU WIN!");
        std::string finalScore = "Final Score: " + intToString(score);
        std::string finalTime = "Time: " + intToString(gameTime) + " seconds";
        drawText(6.0f, 11.0f, finalScore.c_str());
        drawText(6.0f, 10.0f, finalTime.c_str());
        if (score > highScore) {
            drawText(5.5f, 9.0f, "NEW HIGH SCORE!");
        }
        drawText(6.5f, 7.0f, "Press M for Menu");
    }

    glutSwapBuffers();
}

// ---------------------- Keyboard Input Handler ----------------------
// Processes all keyboard inputs for game control
// ESC: Exit game immediately
// SPACE: Start new game from menu
// R: Resume game if paused
// H: Open help screen from menu
// S: Open high score screen (also Down movement in-game)
// M: Return to menu from any screen
// P: Pause/unpause during gameplay
// W/A/S/D: Movement controls (Up/Left/Down/Right)
// Movement only active during PLAYING state

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: exit(0); break; // ESC
        case ' ': // SPACE
            if (gameState == MENU) {
                resetGame();
                gameState = PLAYING;
            }
            break;
        case 'r': case 'R':
            if (gameState == MENU && previousState == PAUSED) {
                gameState = PLAYING;
            }
            break;
        case 'h': case 'H':
            if (gameState == MENU) {
                gameState = HELP;
            }
            break;
        case 's': case 'S':
            if (gameState == MENU) {
                gameState = HIGHSCORE;
            } else if (gameState == PLAYING) {
                pacman.dirX = 0; pacman.dirY = -1;
            }
            break;
        case 'm': case 'M':
            if (gameState != PLAYING) {
                gameState = MENU;
            }
            break;
        case 'p': case 'P':
            if (gameState == PLAYING) {
                previousState = PAUSED;
                gameState = PAUSED;
            } else if (gameState == PAUSED) {
                gameState = PLAYING;
            }
            break;
        case 'w': case 'W':
            if (gameState == PLAYING) {
                pacman.dirX = 0; pacman.dirY = 1;
            }
            break;
        case 'a': case 'A':
            if (gameState == PLAYING) {
                pacman.dirX = -1; pacman.dirY = 0;
            }
            break;
        case 'd': case 'D':
            if (gameState == PLAYING) {
                pacman.dirX = 1; pacman.dirY = 0;
            }
            break;

            }
}

// ---------------------- Timer Callback Function ----------------------
// Called repeatedly at 60 FPS (every 16.67ms)
// Updates game logic by calling updateGame()
// Triggers screen redraw with glutPostRedisplay()
// Reschedules itself to maintain constant frame rate
// This creates the game loop for smooth animation

void timer(int) {
    updateGame();
    glutPostRedisplay();
    glutTimerFunc(1000/60, timer, 0);
}

// ---------------------- Main Entry Point ----------------------
// Initializes random number generator for ghost AI
// Sets up GLUT window and OpenGL context
// Configures double buffering for smooth graphics
// Creates 800x800 pixel window with title
// Sets up 2D orthographic projection (0-20 range for game grid)
// Loads high score from file
// Resets game to initial state
// Registers callback functions:
//   - display() for rendering
//   - keyboard() for input
//   - timer() for game loop
// Starts GLUT main loop (runs until exit)

int main(int argc, char** argv) {
    srand(time(0));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Pacman Game - Complete Edition");

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, COLS, 0, ROWS);

    loadHighScore();
    resetGame();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
