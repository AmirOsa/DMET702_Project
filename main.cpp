// ===============================
// DMET 502 . Team El 3ennab
// ===============================

// ========================
// CHOOSE WHICH LEVEL TO RUN
// ========================
//#define RUN_LEVEL_2  // Comment this to run Level 1, uncomment to run Level 2
// ========================

#pragma warning(disable : 2381)   // ignore 'exit' redefinition from old GLUT vs stdlib

#include <math.h>
#include <string>

#define GLUT_DISABLE_ATEXIT_HACK
#include "glut.h"
#include "Model_3DS.h"
#include "TextureBuilder.h"
#include "GLTexture.h"
#include <stdlib.h>
#include <time.h>

// ===============================
// SHARED UTILITIES (used by both levels)
// ===============================

enum CameraMode { FIRST_PERSON, THIRD_PERSON };
enum GameState { GAME_PLAYING, GAME_WON, GAME_LOST };

// Simple Axis Aligned Bounding Box on XZ plane
struct AABB {
    float x, z;   // center position in XZ plane
    float halfW;  // half width on X
    float halfD;  // half depth on Z
    bool  active; // for collectibles and checkpoint
};

// 2D text rendering for HUD (shared)
void drawText2D(float x, float y, const char* text) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING); // HUD should not be lit

    glRasterPos2f(x, y);
    for (int i = 0; text[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
    }

    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Random helper functions (shared)
float randRange(float minVal, float maxVal) {
    return minVal + (maxVal - minVal) * (rand() / (float)RAND_MAX);
}

// Collision check (shared)
bool checkAABBCollision(const AABB& a, const AABB& b) {
    bool overlapX = fabs(a.x - b.x) <= (a.halfW + b.halfW);
    bool overlapZ = fabs(a.z - b.z) <= (a.halfD + b.halfD);
    return overlapX && overlapZ;
}

// ===============================
// LEVEL 1 CODE (MOHANAD'S WORK - COMPLETE AND UNCHANGED)
// ===============================
#ifndef RUN_LEVEL_2

// ===============================
// Global game state - Level 1
// ===============================

// Player state
float playerX = 0.0f;      // side movement along the street (left/right)
float playerZ = 0.0f;      // forward movement (runner direction, negative Z)
float playerY = 0.0f;      // height

// Street boundaries (adjust to match your street width)
float streetMinX = -10.0f;
float streetMaxX = 10.0f;

// Street Z range (towards negative Z)
const float streetStartZ = 5.0f;      // near camera
const float streetEndZ = -1000.0f;  // far end

// Camera
CameraMode cameraMode = THIRD_PERSON;

// Camera parameters
float eyeHeight = 2.0f;  // height of player's eyes
float thirdPersonDist = 8.0f;  // how far camera is behind player
float thirdPersonHeight = 4.0f; // how high camera is above player

// Score
int score = 0;

// Limits
const int MAX_CARS = 50;
const int MAX_TRASHCANS = 50;
const int MAX_COLLECTIBLES = 200;

// Arrays
AABB cars[MAX_CARS];
int  numCars = 0;

AABB trashCans[MAX_TRASHCANS];
int  numTrashCans = 0;

AABB collectibles[MAX_COLLECTIBLES];
int  numCollectibles = 0;

// Checkpoint (No2'et El Tafteesh)
AABB checkpoint;

// Lighting . street lamp (Level 1)
GLfloat lampBasePos[4] = { 0.0f, 5.0f, -30.0f, 1.0f }; // base position
float   lampRotateAngle = 0.0f;  // rotation for animation
float   lampIntensity = 1.0f;  // light intensity

// Time for delta time computations
int prevTimeMs = 0;

// Movement step
float moveStep = 0.5f;

// Difficulty level 0 = easy, 1 = medium, 2 = hard
int difficultyLevel = 0;

bool checkpointSpawned20s = false;
bool checkpointSpawned3s = false;

// Game state and timer
GameState gameState = GAME_PLAYING;

// Timer, 60 seconds for Level 1
const int gameDurationMs = 60000;   // 60 * 1000
int       gameStartTimeMs = 0;      // when level started in ms
int       remainingTimeMs = 60000;  // remaining time in ms

// Models . Person B will fill paths and textures
Model_3DS playerModel;
Model_3DS carModel;
Model_3DS trashModel;
Model_3DS collectibleModel;
Model_3DS checkpointModel;
Model_3DS buildingModel;
Model_3DS lampModel;

// ===============================
// Helper functions - Level 1 specific
// ===============================

AABB getPlayerAABB() {
    AABB p;
    p.x = playerX;
    p.z = playerZ;
    p.halfW = 1.0f;  // approximate player width
    p.halfD = 1.0f;  // approximate player depth
    p.active = true;
    return p;
}

void handleObstacleCollision(const AABB& obstacle) {
    // 1. Deduct 10 from score
    score -= 10;
    if (score < 0) score = 0; // do not go below 0

    // 2. Simple knock-back translation for the player, push slightly backward along +Z
    playerZ += 2.0f;

    // 3. Small side push away from the obstacle center
    float sidePush = 1.0f;
    if (playerX >= obstacle.x)
        playerX += sidePush;
    else
        playerX -= sidePush;

    // 4. Clamp inside street after the push
    if (playerX < streetMinX) playerX = streetMinX;
    if (playerX > streetMaxX) playerX = streetMaxX;
}

void spawnExtraObstacles(int extraCars, int extraTrash) {
    // Add extra cars into any free slots
    while (extraCars > 0 && numCars < MAX_CARS) {
        AABB& c = cars[numCars];
        c.x = randRange(streetMinX + 2.0f, streetMaxX - 2.0f);
        c.z = randRange(streetEndZ + 30.0f, -10.0f);
        c.halfW = 2.0f;
        c.halfD = 4.0f;
        c.active = true;
        numCars++;
        extraCars--;
    }

    // Add extra trash cans
    while (extraTrash > 0 && numTrashCans < MAX_TRASHCANS) {
        AABB& t = trashCans[numTrashCans];
        t.x = randRange(streetMinX + 1.0f, streetMaxX - 1.0f);
        t.z = randRange(streetEndZ + 30.0f, -5.0f);
        t.halfW = 0.8f;
        t.halfD = 0.8f;
        t.active = true;
        numTrashCans++;
        extraTrash--;
    }
}

// This sets up Level 1 layout . Person A can tweak positions later
void setupLevel1() {
    // Reset player and score
    playerX = 0.0f;
    playerZ = 0.0f;
    playerY = 0.0f;
    score = 0;
    moveStep = 0.5f;
    difficultyLevel = 0;
    checkpoint.active = false;
    checkpointSpawned20s = false;
    checkpointSpawned3s = false;

    // ----- Cars -----
    numCars = 50;  // you can change this up to MAX_CARS

    float carMinZ = streetEndZ + 20.0f; // e.g. -180
    float carMaxZ = -10.0f;             // closer to player

    for (int i = 0; i < numCars; ++i) {
        cars[i].x = randRange(streetMinX + 2.0f, streetMaxX - 2.0f);
        cars[i].z = randRange(carMinZ, carMaxZ);
        cars[i].halfW = 2.0f;
        cars[i].halfD = 4.0f;
        cars[i].active = true;
    }

    // ----- Trash cans -----
    numTrashCans = 30;  // up to MAX_TRASHCANS

    float trashMinZ = streetEndZ + 30.0f;
    float trashMaxZ = -5.0f;

    for (int i = 0; i < numTrashCans; ++i) {
        trashCans[i].x = randRange(streetMinX + 1.0f, streetMaxX - 1.0f);
        trashCans[i].z = randRange(trashMinZ, trashMaxZ);
        trashCans[i].halfW = 0.8f;
        trashCans[i].halfD = 0.8f;
        trashCans[i].active = true;
    }

    // ----- Collectibles (3ennabeyat) -----
    numCollectibles = 100;  // up to MAX_COLLECTIBLES

    float colMinZ = streetEndZ + 15.0f;
    float colMaxZ = -5.0f;

    for (int i = 0; i < numCollectibles; ++i) {
        collectibles[i].x = randRange(streetMinX + 1.0f, streetMaxX - 1.0f);
        collectibles[i].z = randRange(colMinZ, colMaxZ);
        collectibles[i].halfW = 0.5f;
        collectibles[i].halfD = 0.5f;
        collectibles[i].active = true;
    }

    // ----- Checkpoint at far end of street -----
    checkpoint.x = 0.0f;
    checkpoint.z = streetEndZ + 10.0f; // e.g. -190
    checkpoint.halfW = 4.0f;
    checkpoint.halfD = 1.0f;
    checkpoint.active = false;              // appears only in last 20 seconds
}

// Camera setup for first person and third person
void setupCamera() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // For Level 1 runner, player faces along negative Z
    float lookDirX = 0.0f;
    float lookDirZ = -1.0f;

    if (cameraMode == FIRST_PERSON) {
        gluLookAt(
            playerX, playerY + eyeHeight, playerZ,
            playerX + lookDirX, playerY + eyeHeight, playerZ + lookDirZ,
            0, 1, 0
        );
    }
    else { // THIRD_PERSON
        float camX = playerX - lookDirX * thirdPersonDist;
        float camY = playerY + thirdPersonHeight;
        float camZ = playerZ - lookDirZ * thirdPersonDist;

        gluLookAt(
            camX, camY, camZ,
            playerX, playerY + eyeHeight, playerZ,
            0, 1, 0
        );
    }
}

void drawScore() {
    char buffer[64];
    sprintf(buffer, "Score: %d", score);
    drawText2D(-0.95f, 0.9f, buffer);
}

void drawTimer() {
    int seconds = remainingTimeMs / 1000;
    if (seconds < 0) seconds = 0;

    char buffer[64];
    sprintf(buffer, "Time: %02d", seconds);

    // Place timer on the top right
    drawText2D(0.6f, 0.9f, buffer);
}

void drawGameStatus() {
    if (gameState == GAME_WON) {
        drawText2D(-0.3f, 0.0f, "Level 1 complete !");
    }
    else if (gameState == GAME_LOST) {
        drawText2D(-0.35f, 0.0f, "TIME UP! Game Over");
    }
}

// ===============================
// Drawing the world (Person A layout)
// ===============================

void drawStreet() {
    // Simple ground plane . Person B can later add textures
    glDisable(GL_TEXTURE_2D); // keep it simple for now

    glColor3f(0.2f, 0.2f, 0.2f); // asphalt

    glBegin(GL_QUADS);
    glVertex3f(-15.0f, 0.0f, streetStartZ);
    glVertex3f(15.0f, 0.0f, streetStartZ);
    glVertex3f(15.0f, 0.0f, streetEndZ);
    glVertex3f(-15.0f, 0.0f, streetEndZ);
    glEnd();

    // Side sidewalks
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    // left sidewalk
    glVertex3f(-15.0f, 0.0f, streetStartZ);
    glVertex3f(-10.0f, 0.0f, streetStartZ);
    glVertex3f(-10.0f, 0.0f, streetEndZ);
    glVertex3f(-15.0f, 0.0f, streetEndZ);
    // right sidewalk
    glVertex3f(10.0f, 0.0f, streetStartZ);
    glVertex3f(15.0f, 0.0f, streetStartZ);
    glVertex3f(15.0f, 0.0f, streetEndZ);
    glVertex3f(10.0f, 0.0f, streetEndZ);
    glEnd();

    glColor3f(1, 1, 1); // reset color
}

void drawBuildings() {
    glColor3f(0.25f, 0.25f, 0.3f);

    for (float z = streetStartZ; z > streetEndZ; z -= 60.0f) {
        // LEFT SIDE BUILDING
        glPushMatrix();
        glTranslatef(-12.5f, 5.0f, z);     // Y = 5 because height = ~10
        glScalef(4.0f, 10.0f, 6.0f);      // width, height, depth
        glutSolidCube(1.0f);              // cube of size 1 scaled into a real building
        glPopMatrix();

        // RIGHT SIDE BUILDING
        glPushMatrix();
        glTranslatef(12.5f, 5.0f, z);
        glScalef(4.0f, 10.0f, 6.0f);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    glColor3f(1, 1, 1);
}

void drawCars() {
    for (int i = 0; i < numCars; i++) {
        const AABB& c = cars[i];
        if (!c.active) continue;

        glPushMatrix();
        glTranslatef(c.x, 0.0f, c.z);
        // Person B: adjust scale and orientation to fit car model
        // glScalef(...);
        // carModel.Draw();
        glutSolidCube(4.0); // placeholder visual for testing
        glPopMatrix();
    }
}

void drawTrashCans() {
    for (int i = 0; i < numTrashCans; i++) {
        const AABB& t = trashCans[i];
        if (!t.active) continue;

        glPushMatrix();
        glTranslatef(t.x, 0.0f, t.z);
        // trashModel.Draw(); // Person B
        glutSolidCube(1.5); // placeholder
        glPopMatrix();
    }
}

void drawCollectibles() {
    for (int i = 0; i < numCollectibles; i++) {
        const AABB& c = collectibles[i];
        if (!c.active) continue;

        glPushMatrix();
        glTranslatef(c.x, 0.5f, c.z);
        // 3ennabeya model . Person B
        // collectibleModel.Draw();
        glutSolidSphere(0.7, 16, 16); // placeholder
        glPopMatrix();
    }
}

void drawCheckpoint() {
    if (!checkpoint.active) return;

    glPushMatrix();
    glTranslatef(checkpoint.x, 0.0f, checkpoint.z);
    // checkpointModel.Draw(); // Person B
    glutSolidCube(3.0); // placeholder
    glPopMatrix();
}

void drawLamps() {
    const float lampX = 10.5f;
    const float scale = 0.85f;
    const float lampY = 0.15f;   // lift lamp a bit above the ground

    for (float z = streetStartZ; z > streetEndZ; z -= 60.0f) {
        float midZ = z - 30.0f;

        // LEFT lamp
        glPushMatrix();
        glTranslatef(-lampX, lampY, midZ);    // was 0.0f
        glScalef(scale, scale, scale);
        lampModel.Draw();
        glPopMatrix();

        // RIGHT lamp
        glPushMatrix();
        glTranslatef(lampX, lampY, midZ);     // was 0.0f
        glRotatef(-180.0f, 0, 1, 0);
        glScalef(scale, scale, scale);
        lampModel.Draw();
        glPopMatrix();
    }
}


void drawPlayer() {
    glPushMatrix();
    glTranslatef(playerX, playerY, playerZ);
    // playerModel.Draw(); // Person B
    glutSolidCube(2.0); // placeholder
    glPopMatrix();
}

// ===============================
// Lighting - Level 1 specific
// ===============================

void applyLampLight() {
    glEnable(GL_LIGHTING);

    // Enable up to 8 lights
    for (int i = 0; i < 8; ++i) {
        glEnable(GL_LIGHT0 + i);
    }

    // Global ambient for the whole scene (dark)
    GLfloat globalAmbient[] = { 0.05f, 0.05f, 0.05f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    // Warm street-lamp colour
    GLfloat ambient[]  = { 0.05f, 0.05f, 0.03f, 1.0f };
    GLfloat diffuse[]  = { 1.2f, 1.2f, 1.0f, 1.0f };   // a bit bright
    GLfloat specular[] = { 1.0f, 1.0f, 0.9f, 1.0f };

    for (int i = 0; i < 8; ++i) {
        GLenum L = GL_LIGHT0 + i;
        glLightfv(L, GL_AMBIENT,  ambient);
        glLightfv(L, GL_DIFFUSE,  diffuse);
        glLightfv(L, GL_SPECULAR, specular);

        // Same attenuation for all lamps
        glLightf(L, GL_CONSTANT_ATTENUATION,  0.5f);
        glLightf(L, GL_LINEAR_ATTENUATION,    0.02f);
        glLightf(L, GL_QUADRATIC_ATTENUATION, 0.008f);
    }
}


void updateLamp(float deltaTime) {
    (void)deltaTime; // we don't need it now

    const float lampX      = 10.5f;   // same as in drawLamps()
    const float lampHeight = 6.0f;    // approximate lamp head height
    const float spacing    = 60.0f;   // distance between building rows
    const float offsetZ    = 30.0f;   // lamps are between buildings

    // How far from the player a lamp can be and still get a real light
    const float lightRangeZ = 180.0f;  // lamps within +/- 180 on Z get lit

    int lightIndex = 0; // 0..7 → GL_LIGHT0..GL_LIGHT7

    for (float z = streetStartZ; z > streetEndZ; z -= spacing) {
        float midZ = z - offsetZ; // same Z as drawLamps()

        // Only attach lights to lamps near the player
        if (fabsf(midZ - playerZ) <= lightRangeZ && lightIndex < 8) {
            // LEFT lamp
            GLfloat posL[] = { -lampX, lampHeight, midZ, 1.0f };
            glLightfv(GL_LIGHT0 + lightIndex, GL_POSITION, posL);
            lightIndex++;

            if (lightIndex >= 8) break;

            // RIGHT lamp
            GLfloat posR[] = {  lampX, lampHeight, midZ, 1.0f };
            glLightfv(GL_LIGHT0 + lightIndex, GL_POSITION, posR);
            lightIndex++;
            if (lightIndex >= 8) break;
        }
    }

    // Any remaining lights that weren't used: move them far away so they don't affect scene
    for (; lightIndex < 8; ++lightIndex) {
        GLfloat offPos[] = { 0.0f, 10000.0f, 0.0f, 0.0f }; // directional far away
        glLightfv(GL_LIGHT0 + lightIndex, GL_POSITION, offPos);
    }
}



// ===============================
// GLUT callbacks - Level 1
// ===============================

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setupCamera();
    applyLampLight();
    updateLamp(0.0f);

    drawStreet();
    drawBuildings();
    drawLamps();
    drawCars();
    drawTrashCans();
    drawCollectibles();
    drawCheckpoint();
    drawPlayer();

    drawScore();
    drawTimer();
    drawGameStatus();

    glutSwapBuffers();
}

void idle() {
    // Compute current time and delta time
    int   currentMs = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (currentMs - prevTimeMs) / 1000.0f;
    prevTimeMs = currentMs;

    // If game is finished, only keep animating visuals if we want, no more logic
   

    // Update remaining time
    int elapsedSinceStart = currentMs - gameStartTimeMs;
    remainingTimeMs = gameDurationMs - elapsedSinceStart;
    if (remainingTimeMs <= 0) {
        remainingTimeMs = 0;
        gameState = GAME_LOST;   // time up
        glutPostRedisplay();
        return;
    }

    // ---------- Checkpoint logic ----------

    // 1st chance . somewhere ahead in last 20 seconds
    if (!checkpointSpawned20s && remainingTimeMs <= 20000) {
        checkpoint.x = 0.0f;
        checkpoint.z = playerZ - 60.0f;   // 60 units ahead of player
        checkpoint.halfW = 4.0f;
        checkpoint.halfD = 1.0f;
        checkpoint.active = true;
        checkpointSpawned20s = true;
    }

    // If checkpoint is active and player has passed it without touching it, deactivate it
    if (checkpoint.active && playerZ < checkpoint.z - 5.0f && gameState == GAME_PLAYING) {
        checkpoint.active = false;
    }

    // 2nd chance . last 3 seconds if still not won
    if (!checkpointSpawned3s && gameState == GAME_PLAYING && remainingTimeMs <= 3000) {
        checkpoint.x = 0.0f;
        checkpoint.z = playerZ - 40.0f;   // closer this time
        checkpoint.halfW = 4.0f;
        checkpoint.halfD = 1.0f;
        checkpoint.active = true;
        checkpointSpawned3s = true;
    }

    // Clamp player inside street
    if (playerX < streetMinX) playerX = streetMinX;
    if (playerX > streetMaxX) playerX = streetMaxX;

    // Update lamp animation and its light position
    updateLamp(deltaTime);

    // Collision checks
    AABB playerBox = getPlayerAABB();

    // Player ↔ obstacles . cars
    for (int i = 0; i < numCars; i++) {
        if (cars[i].active && checkAABBCollision(playerBox, cars[i])) {
            handleObstacleCollision(cars[i]);
            // Avoid multiple penalties in the same frame
            break;
        }
    }

    // Player ↔ obstacles . trash cans
    for (int i = 0; i < numTrashCans; i++) {
        if (trashCans[i].active && checkAABBCollision(playerBox, trashCans[i])) {
            handleObstacleCollision(trashCans[i]);
            // Avoid multiple penalties in the same frame
            break;
        }
    }

    // Player ↔ collectibles
    for (int i = 0; i < numCollectibles; i++) {
        if (collectibles[i].active && checkAABBCollision(playerBox, collectibles[i])) {
            collectibles[i].active = false;  // collected logically
            score += 10;
            // Person B: trigger 3ennabeya scaling, rotation and sound here
        }
    }

    // Player ↔ checkpoint
    if (checkpoint.active && checkAABBCollision(playerBox, checkpoint)) {
        checkpoint.active = false;
        gameState = GAME_WON;
        // Later: trigger Level 2 here
        // Example: call a function Level2_Init() when implemented
    }

    // ----- Dynamic difficulty based on score -----
    if (difficultyLevel == 0 && score >= 50) {
        difficultyLevel = 1;
        moveStep = 0.7f;          // slightly faster movement
        spawnExtraObstacles(5, 5);       // more cars and trash
    }

    if (difficultyLevel == 1 && score >= 100) {
        difficultyLevel = 2;
        moveStep = 0.9f;          // even faster
        spawnExtraObstacles(10, 10);
    }

    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    if (gameState != GAME_PLAYING && key != 27) {
        return;
    }

    switch (key) {
    case 'a':
    case 'A':
        playerX -= moveStep;
        break;

    case 'd':
    case 'D':
        playerX += moveStep;
        break;

    case 'w':
    case 'W':
        playerZ -= moveStep;    // move forward along -Z
        break;

    case 's':
    case 'S':
        playerZ += moveStep;    // move backward along +Z
        break;

        // 1 = first person camera
    case '1':
        cameraMode = FIRST_PERSON;
        break;

        // 3 = third person camera
    case '3':
        cameraMode = THIRD_PERSON;
        break;

    case 27: // ESC
        exit(0);
        break;
    }

    glutPostRedisplay();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    float aspect = (float)w / (float)h;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 200.0);

    glMatrixMode(GL_MODELVIEW);
}

// ===============================
// Initialization - Level 1
// ===============================

void loadModels() {
    lampModel.Load("models/StreetLamp.3ds");

    // Person B: fill correct paths to .3ds files and handle textures
    // Example:
    // playerModel.Load("models/Player.3ds");
    // carModel.Load("models/Car.3ds");
    // trashModel.Load("models/TrashCan.3ds");
    // collectibleModel.Load("models/3ennabeya.3ds");
    // checkpointModel.Load("models/Checkpoint.3ds");
    // buildingModel.Load("models/Building.3ds");
    // lampModel.Load("models/Lamp.3ds");
}

void initGL() {
    // Seed random once
    srand((unsigned int)time(NULL));
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f); // dark sky
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    applyLampLight();
    updateLamp(0.0f); 


    setupLevel1();
    loadModels();

    prevTimeMs = glutGet(GLUT_ELAPSED_TIME);
    gameStartTimeMs = prevTimeMs;
    remainingTimeMs = gameDurationMs;
    gameState = GAME_PLAYING;
}

#endif // RUN_LEVEL_2 - End of Level 1 code

// ===================================================================
// LEVEL 2 CODE STARTS HERE - Amir's Work
// ===================================================================
#ifdef RUN_LEVEL_2

// ===============================
// Level 2 Specific Structures - Amir's Work
// ===============================

// Level 2 AABB with Y coordinate for flying - Amir's Work
struct AABB_L2 {
    float x, y, z;   // 3D position
    float halfW, halfD, halfH; // 3D dimensions
    bool active;
};

// ===============================
// Level 2 Global Variables - Amir's Work
// ===============================

// Player state for flying - Amir's Work
float playerX_L2 = 0.0f;      // side movement
float playerY_L2 = 25.0f;     // flying height (Y-axis)
float playerZ_L2 = 0.0f;      // forward movement (flying direction, negative Z)

// Flying boundaries - Amir's Work
const float riverWidth = 80.0f;
const float riverMinX = -riverWidth / 2;
const float riverMaxX = riverWidth / 2;
const float minHeight = 10.0f;
const float maxHeight = 50.0f;

// Camera - Amir's Work
CameraMode cameraMode_L2 = THIRD_PERSON;
float thirdPersonDist_L2 = 15.0f;  // Camera distance behind player
float thirdPersonHeight_L2 = 8.0f; // Camera height above player

// Score - Amir's Work
int score_L2 = 0;

// Flying collectibles (3ennabeyat) - Amir's Work
const int MAX_FLYING_COLLECTIBLES = 50;
AABB_L2 flyingCollectibles[MAX_FLYING_COLLECTIBLES];
int numFlyingCollectibles = 0;

// Obstacles (birds) - Amir's Work
const int MAX_BIRDS = 20;
AABB_L2 birds[MAX_BIRDS];
int numBirds = 0;

// Target (Dr. Beram) - Amir's Work
AABB_L2 drBeram;

// Lighting for sunset - Amir's Work
float sunColor[3] = { 1.0f, 0.9f, 0.0f }; // Yellow (start)
float sunsetProgress = 0.0f; // 0.0 = start, 1.0 = sunset complete

// Time and movement - Amir's Work
int prevTimeMs_L2 = 0;
float moveStep_L2 = 1.2f; // Flying speed
float verticalSpeed = 0.0f; // For up/down movement

// Game state - Amir's Work
GameState gameState_L2 = GAME_PLAYING;
const int level2DurationMs = 90000; // 90 seconds for flying level
int level2StartTimeMs = 0;
int remainingTime_L2 = level2DurationMs;

// Models (to be loaded by Person B) - Amir's Work
Model_3DS playerModel_L2;
Model_3DS birdModel;
Model_3DS collectibleModel_L2;
Model_3DS drBeramModel;

// ===============================
// Level 2 Helper Functions - Amir's Work
// ===============================

// Check collision in 3D - Amir's Work
bool checkAABBCollision3D(const AABB_L2& a, const AABB_L2& b) {
    bool overlapX = fabs(a.x - b.x) <= (a.halfW + b.halfW);
    bool overlapY = fabs(a.y - b.y) <= (a.halfH + b.halfH);
    bool overlapZ = fabs(a.z - b.z) <= (a.halfD + b.halfD);
    return overlapX && overlapY && overlapZ;
}

// Get player's 3D bounding box - Amir's Work
AABB_L2 getPlayerAABB_L2() {
    AABB_L2 p;
    p.x = playerX_L2;
    p.y = playerY_L2;
    p.z = playerZ_L2;
    p.halfW = 1.5f;   // Player width
    p.halfH = 1.5f;   // Player height
    p.halfD = 1.5f;   // Player depth
    p.active = true;
    return p;
}

// Handle collision with bird - Amir's Work
void handleBirdCollision() {
    // Deduct points
    score_L2 -= 15;
    if (score_L2 < 0) score_L2 = 0;

    // Push back and down
    playerZ_L2 += 3.0f;
    playerY_L2 -= 2.0f;

    // Clamp position
    if (playerY_L2 < minHeight) playerY_L2 = minHeight;
    if (playerY_L2 > maxHeight) playerY_L2 = maxHeight;
}

// Random float in range - Amir's Work (using shared randRange)
float randFloat(float minVal, float maxVal) {
    return randRange(minVal, maxVal);
}

// Setup Level 2 - Flying over Nile - Amir's Work
void setupLevel2() {
    // Reset player
    playerX_L2 = 0.0f;
    playerY_L2 = 25.0f;
    playerZ_L2 = 0.0f;
    score_L2 = 0;
    sunsetProgress = 0.0f;
    sunColor[0] = 1.0f; // Red
    sunColor[1] = 0.9f; // Green
    sunColor[2] = 0.0f; // Blue

    // Setup flying collectibles (3ennabeyat) - Amir's Work
    numFlyingCollectibles = 40;
    for (int i = 0; i < numFlyingCollectibles; i++) {
        flyingCollectibles[i].x = randFloat(riverMinX + 5, riverMaxX - 5);
        flyingCollectibles[i].y = randFloat(15, 40); // Different heights
        flyingCollectibles[i].z = -50.0f - (i * 25.0f); // Spread along Z
        flyingCollectibles[i].halfW = 1.0f;
        flyingCollectibles[i].halfH = 1.0f;
        flyingCollectibles[i].halfD = 1.0f;
        flyingCollectibles[i].active = true;
    }

    // Setup birds (obstacles) - Amir's Work
    numBirds = 15;
    for (int i = 0; i < numBirds; i++) {
        birds[i].x = randFloat(riverMinX + 10, riverMaxX - 10);
        birds[i].y = randFloat(20, 35);
        birds[i].z = -80.0f - (i * 40.0f);
        birds[i].halfW = 2.0f;
        birds[i].halfH = 1.0f;
        birds[i].halfD = 3.0f;
        birds[i].active = true;
    }

    // Setup Dr. Beram (target) - Amir's Work
    drBeram.x = 0.0f;
    drBeram.y = 30.0f;
    drBeram.z = -800.0f; // Far ahead
    drBeram.halfW = 2.0f;
    drBeram.halfH = 3.0f;
    drBeram.halfD = 1.0f;
    drBeram.active = true;
}

// Camera for flying level - Amir's Work
void setupCameraLevel2() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (cameraMode_L2 == THIRD_PERSON) {
        // Third person: behind and above - Amir's Work
        float camX = playerX_L2;
        float camY = playerY_L2 + thirdPersonHeight_L2;
        float camZ = playerZ_L2 + thirdPersonDist_L2;

        gluLookAt(camX, camY, camZ,
            playerX_L2, playerY_L2, playerZ_L2 - 20.0f,
            0, 1, 0);
    }
    else {
        // First person (optional) - Amir's Work
        gluLookAt(playerX_L2, playerY_L2 + 2.0f, playerZ_L2,
            playerX_L2, playerY_L2 + 2.0f, playerZ_L2 - 20.0f,
            0, 1, 0);
    }
}

// Draw sky with sunset gradient - Amir's Work
void drawSky() {
    glDisable(GL_LIGHTING);

    // Sky gradient - Amir's Work
    glBegin(GL_QUADS);
    // Top color (darker blue/orange) - Amir's Work
    glColor3f(0.1f, 0.2f, 0.8f - sunsetProgress * 0.4f);
    glVertex3f(-500, 200, -1500);
    glVertex3f(500, 200, -1500);

    // Bottom color (lighter, more orange) - Amir's Work
    glColor3f(0.6f + sunsetProgress * 0.3f,
        0.3f + sunsetProgress * 0.5f,
        0.1f);
    glVertex3f(500, -50, 500);
    glVertex3f(-500, -50, 500);
    glEnd();

    glEnable(GL_LIGHTING);
}

// Draw Nile River - Amir's Work
void drawNileRiver() {
    glColor3f(0.0f, 0.3f, 0.6f); // Nile blue - Amir's Work

    glBegin(GL_QUADS);
    glVertex3f(-200, 0, -1500);
    glVertex3f(200, 0, -1500);
    glVertex3f(200, 0, 500);
    glVertex3f(-200, 0, 500);
    glEnd();

    // River banks - Amir's Work
    glColor3f(0.4f, 0.3f, 0.1f); // Brown banks - Amir's Work
    glBegin(GL_QUADS);
    // Left bank - Amir's Work
    glVertex3f(-250, 0, -1500);
    glVertex3f(-200, 0, -1500);
    glVertex3f(-200, 0, 500);
    glVertex3f(-250, 0, 500);
    // Right bank - Amir's Work
    glVertex3f(200, 0, -1500);
    glVertex3f(250, 0, -1500);
    glVertex3f(250, 0, 500);
    glVertex3f(200, 0, 500);
    glEnd();
}

// Draw flying collectibles (3ennabeyat) - Amir's Work
void drawFlyingCollectibles() {
    glColor3f(1.0f, 0.0f, 0.0f); // Red for 3ennab - Amir's Work

    for (int i = 0; i < numFlyingCollectibles; i++) {
        if (!flyingCollectibles[i].active) continue;

        glPushMatrix();
        glTranslatef(flyingCollectibles[i].x,
            flyingCollectibles[i].y,
            flyingCollectibles[i].z);
        glutSolidSphere(1.5, 16, 16); // Placeholder - Amir's Work
        glPopMatrix();
    }
}

// Draw birds - Amir's Work
void drawBirds() {
    glColor3f(0.5f, 0.5f, 0.5f); // Gray birds - Amir's Work

    for (int i = 0; i < numBirds; i++) {
        if (!birds[i].active) continue;

        glPushMatrix();
        glTranslatef(birds[i].x, birds[i].y, birds[i].z);

        // Simple bird shape (two spheres) - Amir's Work
        glutSolidSphere(1.5, 8, 8); // Body - Amir's Work
        glPushMatrix();
        glTranslatef(0, 0, -2.0f);
        glutSolidSphere(0.8, 8, 8); // Head - Amir's Work
        glPopMatrix();

        glPopMatrix();
    }
}

// Draw Dr. Beram - Amir's Work
void drawDrBeram() {
    if (!drBeram.active) return;

    glPushMatrix();
    glTranslatef(drBeram.x, drBeram.y, drBeram.z);
    glColor3f(0.0f, 1.0f, 0.0f); // Green for visibility - Amir's Work
    glutSolidCube(4.0); // Placeholder - Amir's Work
    glPopMatrix();
}

// Draw flying player - Amir's Work
void drawPlayerLevel2() {
    glPushMatrix();
    glTranslatef(playerX_L2, playerY_L2, playerZ_L2);

    // Simple flying superhero shape - Amir's Work
    glColor3f(1.0f, 0.5f, 0.0f); // Orange hero - Amir's Work

    // Body - Amir's Work
    glPushMatrix();
    glScalef(1.5f, 3.0f, 1.0f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Head - Amir's Work
    glPushMatrix();
    glTranslatef(0, 2.2f, 0);
    glutSolidSphere(0.8, 16, 16);
    glPopMatrix();

    // Wings (simple triangles) - Amir's Work
    glDisable(GL_LIGHTING);
    glColor3f(0.8f, 0.8f, 0.0f);
    glBegin(GL_TRIANGLES);
    // Left wing - Amir's Work
    glVertex3f(-2.5f, 0, 0);
    glVertex3f(-4.0f, 0, -2.0f);
    glVertex3f(-2.5f, 0, -2.0f);
    // Right wing - Amir's Work
    glVertex3f(2.5f, 0, 0);
    glVertex3f(4.0f, 0, -2.0f);
    glVertex3f(2.5f, 0, -2.0f);
    glEnd();
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

// Setup sunset lighting - Amir's Work
void setupSunLight() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Sun color changes from yellow to orange/red - Amir's Work
    GLfloat lightDiffuse[] = { sunColor[0], sunColor[1], sunColor[2], 1.0f };
    GLfloat lightAmbient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Sun position (high and to the side) - Amir's Work
    GLfloat lightPosition[] = { 100.0f, 150.0f, -300.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
}

// Update sunset progress - Amir's Work
void updateSunset(float deltaTime) {
    // Gradually progress sunset - Amir's Work
    sunsetProgress += deltaTime * 0.02f; // Adjust speed as needed
    if (sunsetProgress > 1.0f) sunsetProgress = 1.0f;

    // Interpolate from yellow (1, 0.9, 0) to orange/red (1, 0.3, 0) - Amir's Work
    sunColor[0] = 1.0f;                    // Red stays high
    sunColor[1] = 0.9f - (sunsetProgress * 0.6f); // Green decreases
    sunColor[2] = 0.0f;                    // Blue stays 0
}

// Draw HUD for Level 2 - Amir's Work
void drawScoreLevel2() {
    char buffer[64];
    sprintf(buffer, "Score: %d", score_L2);
    drawText2D(-0.95f, 0.9f, buffer);
}

void drawTimerLevel2() {
    int seconds = remainingTime_L2 / 1000;
    if (seconds < 0) seconds = 0;
    char buffer[64];
    sprintf(buffer, "Time: %02d", seconds);
    drawText2D(0.6f, 0.9f, buffer);
}

void drawHeightIndicator() {
    char buffer[64];
    sprintf(buffer, "Height: %.0f", playerY_L2);
    drawText2D(-0.95f, 0.8f, buffer);
}

void drawGameStatusLevel2() {
    if (gameState_L2 == GAME_WON) {
        drawText2D(-0.3f, 0.0f, "Rescue Complete! Dr. Beram Saved!");
    }
    else if (gameState_L2 == GAME_LOST) {
        drawText2D(-0.35f, 0.0f, "Mission Failed!");
    }
}

// ===============================
// Level 2 GLUT Callbacks - Amir's Work
// ===============================

void displayLevel2() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setupCameraLevel2();
    setupSunLight();

    drawSky();
    drawNileRiver();
    drawFlyingCollectibles();
    drawBirds();
    drawDrBeram();
    drawPlayerLevel2();

    drawScoreLevel2();
    drawTimerLevel2();
    drawHeightIndicator();
    drawGameStatusLevel2();

    glutSwapBuffers();
}

void idleLevel2() {
    int currentMs = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (currentMs - prevTimeMs_L2) / 1000.0f;
    prevTimeMs_L2 = currentMs;

    if (gameState_L2 != GAME_PLAYING) {
        // Still update sunset animation - Amir's Work
        updateSunset(deltaTime);
        glutPostRedisplay();
        return;
    }

    // Update timer - Amir's Work
    int elapsed = currentMs - level2StartTimeMs;
    remainingTime_L2 = level2DurationMs - elapsed;
    if (remainingTime_L2 <= 0) {
        remainingTime_L2 = 0;
        gameState_L2 = GAME_LOST;
        glutPostRedisplay();
        return;
    }

    // Update sunset - Amir's Work
    updateSunset(deltaTime);

    // Clamp player position - Amir's Work
    if (playerX_L2 < riverMinX) playerX_L2 = riverMinX;
    if (playerX_L2 > riverMaxX) playerX_L2 = riverMaxX;
    if (playerY_L2 < minHeight) playerY_L2 = minHeight;
    if (playerY_L2 > maxHeight) playerY_L2 = maxHeight;

    // Apply gravity/slight downward drift - Amir's Work
    playerY_L2 += verticalSpeed * deltaTime;
    verticalSpeed -= 2.0f * deltaTime; // Gentle downward acceleration

    // Collision detection - Amir's Work
    AABB_L2 playerBox = getPlayerAABB_L2();

    // Check collectibles - Amir's Work
    for (int i = 0; i < numFlyingCollectibles; i++) {
        if (flyingCollectibles[i].active && checkAABBCollision3D(playerBox, flyingCollectibles[i])) {
            flyingCollectibles[i].active = false;
            score_L2 += 20; // More points for flying collectibles
            // TODO: Add collection animation and sound - Amir's Work
        }
    }

    // Check birds - Amir's Work
    for (int i = 0; i < numBirds; i++) {
        if (birds[i].active && checkAABBCollision3D(playerBox, birds[i])) {
            handleBirdCollision();
            birds[i].active = false; // Bird disappears
            break;
        }
    }

    // Check Dr. Beram - Amir's Work
    if (drBeram.active && checkAABBCollision3D(playerBox, drBeram)) {
        drBeram.active = false;
        gameState_L2 = GAME_WON;
        // TODO: Add rescue animation - Amir's Work
    }

    // Move birds (simple animation) - Amir's Work
    for (int i = 0; i < numBirds; i++) {
        if (birds[i].active) {
            birds[i].z += 3.0f * deltaTime; // Birds move toward player
            if (birds[i].z > playerZ_L2 + 50.0f) {
                // Reset bird behind player - Amir's Work
                birds[i].z = playerZ_L2 - 200.0f;
                birds[i].x = randFloat(riverMinX + 10, riverMaxX - 10);
                birds[i].y = randFloat(20, 35);
            }
        }
    }

    glutPostRedisplay();
}

void keyboardLevel2(unsigned char key, int x, int y) {
    if (gameState_L2 != GAME_PLAYING && key != 27) return;

    switch (key) {
    case 'a': case 'A': // Left - Amir's Work
        playerX_L2 -= moveStep_L2 * 2.0f;
        break;
    case 'd': case 'D': // Right - Amir's Work
        playerX_L2 += moveStep_L2 * 2.0f;
        break;
    case 'w': case 'W': // Forward - Amir's Work
        playerZ_L2 -= moveStep_L2 * 2.0f;
        break;
    case 's': case 'S': // Backward - Amir's Work
        playerZ_L2 += moveStep_L2;
        break;
    case ' ': // Space for up - Amir's Work
        verticalSpeed = 5.0f;
        break;
    case 'c': case 'C': // Down - Amir's Work
        verticalSpeed = -5.0f;
        break;
    case '1': // First person - Amir's Work
        cameraMode_L2 = FIRST_PERSON;
        break;
    case '3': // Third person - Amir's Work
        cameraMode_L2 = THIRD_PERSON;
        break;
    case 27: // ESC - Amir's Work
        exit(0);
        break;
    }

    glutPostRedisplay();
}

void reshapeLevel2(int w, int h) {
    if (h == 0) h = 1;
    float aspect = (float)w / (float)h;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 2000.0); // Larger far plane for flying

    glMatrixMode(GL_MODELVIEW);
}

void loadModelsLevel2() {
    // Person B will implement - Amir's Work
    // playerModel_L2.Load("models/FlyingHero.3ds");
    // birdModel.Load("models/Bird.3ds");
    // collectibleModel_L2.Load("models/Flying3ennab.3ds");
    // drBeramModel.Load("models/DrBeram.3ds");
}

void initGLLevel2() {
    srand((unsigned int)time(NULL));
    glClearColor(0.1f, 0.2f, 0.4f, 1.0f); // Sky blue - Amir's Work
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    setupSunLight();
    setupLevel2();
    loadModelsLevel2();

    prevTimeMs_L2 = glutGet(GLUT_ELAPSED_TIME);
    level2StartTimeMs = prevTimeMs_L2;
    remainingTime_L2 = level2DurationMs;
    gameState_L2 = GAME_PLAYING;
}

#endif // RUN_LEVEL_2 - End of Level 2 code

// ===============================
// SINGLE MAIN FUNCTION (shared by both levels)
// ===============================

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 600);

#ifdef RUN_LEVEL_2
    glutCreateWindow("El Ragol El 3ennab . Level 2 (Flying over Nile)");
    initGLLevel2();
    glutDisplayFunc(displayLevel2);
    glutIdleFunc(idleLevel2);
    glutKeyboardFunc(keyboardLevel2);
    glutReshapeFunc(reshapeLevel2);
#else
    glutCreateWindow("El Ragol El 3ennab . Level 1");
    initGL();
    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);
#endif

    glutMainLoop();
    return 0;
}