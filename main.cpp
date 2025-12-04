// ===============================
// DMET 502 . Team El 3ennab
// Level 1 . Person A (Environment + Mechanics)
// ===============================

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
// Global game state
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
enum CameraMode { FIRST_PERSON, THIRD_PERSON };
CameraMode cameraMode = THIRD_PERSON;

// Camera parameters
float eyeHeight = 2.0f;  // height of player's eyes
float thirdPersonDist = 8.0f;  // how far camera is behind player
float thirdPersonHeight = 4.0f; // how high camera is above player

// Score
int score = 0;

// Simple Axis Aligned Bounding Box on XZ plane
struct AABB {
    float x, z;   // center position in XZ plane
    float halfW;  // half width on X
    float halfD;  // half depth on Z
    bool  active; // for collectibles and checkpoint
};

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

// ===============================
// Game state and timer
// ===============================

enum GameState { GAME_PLAYING, GAME_WON, GAME_LOST };
GameState gameState = GAME_PLAYING;

// Timer, 60 seconds for Level 1
const int gameDurationMs = 60000;   // 60 * 1000
int       gameStartTimeMs = 0;      // when level started in ms
int       remainingTimeMs = 60000;  // remaining time in ms

// ===============================
// Models . Person B will fill paths and textures
// ===============================

Model_3DS playerModel;
Model_3DS carModel;
Model_3DS trashModel;
Model_3DS collectibleModel;
Model_3DS checkpointModel;
Model_3DS buildingModel;
Model_3DS lampModel;

// ===============================
// Helper functions
// ===============================

bool checkAABBCollision(const AABB& a, const AABB& b) {
    bool overlapX = fabs(a.x - b.x) <= (a.halfW + b.halfW);
    bool overlapZ = fabs(a.z - b.z) <= (a.halfD + b.halfD);
    return overlapX && overlapZ;
}

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

float randRange(float minVal, float maxVal) {
    return minVal + (maxVal - minVal) * (rand() / (float)RAND_MAX);
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

// 2D text rendering for HUD
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

void drawLamp() {
    glPushMatrix();

    // Place the lamp post at ground level at the same X,Z as the light base
    glTranslatef(lampBasePos[0], 0.0f, lampBasePos[2]);

    // Lamp pole
    glPushMatrix();
    glTranslatef(0.0f, 3.0f, 0.0f);     // center at y = 3
    glScalef(0.2f, 6.0f, 0.2f);         // tall thin pole
    glutSolidCube(1.0f);                // placeholder pole
    glPopMatrix();

    // Lamp head
    glPushMatrix();
    glTranslatef(0.0f, 6.5f, 0.0f);     // head above the pole
    glutSolidSphere(0.5, 16, 16);       // placeholder lamp head
    glPopMatrix();

    glPopMatrix();
}

void drawPlayer() {
    glPushMatrix();
    glTranslatef(playerX, playerY, playerZ);
    // playerModel.Draw(); // Person B
    glutSolidCube(2.0); // placeholder
    glPopMatrix();
}

// ===============================
// Lighting
// ===============================

void applyLampLight() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat diffuse[] = { lampIntensity, lampIntensity, lampIntensity, 1.0f };
    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    // Position will be updated each frame in updateLamp()
}

void updateLamp(float deltaTime) {
    // Rotate lamp around a small circle for animation
    lampRotateAngle += 20.0f * deltaTime;
    if (lampRotateAngle > 360.0f) lampRotateAngle -= 360.0f;

    float radius = 2.0f;
    GLfloat pos[4] = {
        lampBasePos[0] + radius * cosf(lampRotateAngle * 3.14159f / 180.0f),
        lampBasePos[1],
        lampBasePos[2] + radius * sinf(lampRotateAngle * 3.14159f / 180.0f),
        1.0f
    };

    glLightfv(GL_LIGHT0, GL_POSITION, pos);
}

// ===============================
// GLUT callbacks
// ===============================

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setupCamera();
    applyLampLight();

    drawStreet();
    drawBuildings();
    drawLamp();
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
    if (gameState != GAME_PLAYING) {
        updateLamp(deltaTime);
        glutPostRedisplay();
        return;
    }

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
// Initialization
// ===============================

void loadModels() {
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

    setupLevel1();
    loadModels();

    prevTimeMs = glutGet(GLUT_ELAPSED_TIME);
    gameStartTimeMs = prevTimeMs;
    remainingTimeMs = gameDurationMs;
    gameState = GAME_PLAYING;
}

// ===============================
// main
// ===============================

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("El Ragol El 3ennab . Level 1");

    initGL();

    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);

    glutMainLoop();
    return 0;
}
