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
    // Save current matrices
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Draw HUD on top of everything
    GLboolean depthEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
    glDisable(GL_DEPTH_TEST);

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);   // white text

    glRasterPos2f(x, y);
    for (int i = 0; text[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
    }

    glEnable(GL_LIGHTING);
    if (depthEnabled) glEnable(GL_DEPTH_TEST);

    glPopMatrix();                  // modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();                  // projection
    glMatrixMode(GL_MODELVIEW);
}


// Random helper functions (shared)
float randRange(float minVal, float maxVal) {
    return minVal + (maxVal - minVal) * (rand() / (float)RAND_MAX);
}

// Collision check (shared)
bool checkAABBCollision(const AABB& a, const AABB& b) {
    if (!a.active || !b.active) return false;
    float dx = fabs(a.x - b.x);
    float dz = fabs(a.z - b.z);
    bool overlapX = dx <= (a.halfW + b.halfW);
    bool overlapZ = dz <= (a.halfD + b.halfD);
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
// Collider size (hitbox) for each trash can
// Make the box a bit tighter so it matches the cylinder better
const float TRASH_COLLIDER_HALF_W = 0.9f;
const float TRASH_COLLIDER_HALF_D = 0.9f;

// model pivot is to the right of the cylinder center → move model left
const float TRASH_MODEL_OFFSET_X = -6.0f;
const float TRASH_MODEL_OFFSET_Z = 0.0f;

// Collider size (hitbox) for each car
const float CAR_COLLIDER_HALF_W = 5.15f;  // wider on X (left–right)
const float CAR_COLLIDER_HALF_D = 4.0f;  // front–back depth (was already 4)

// Player collider size (XZ only)
const float PLAYER_COLLIDER_HALF_W = 1.2f;  // left–right radius
const float PLAYER_COLLIDER_HALF_D = 0.2f;  // front–back radius


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

// Ground texture
GLTexture groundTexture;

// ===============================
// Helper functions - Level 1 specific
// ===============================
// ==== Looping buildings (max 3 per side = 6 total) ====
const int   NUM_BUILDINGS_PER_SIDE = 3;
const float BUILDING_SPACING_Z = 80.0f;  // distance between buildings on Z
const float BUILDING_RECYCLE_Z = 40.0f;   // how far behind player before we recycle

float leftBuildingZ[NUM_BUILDINGS_PER_SIDE];
float rightBuildingZ[NUM_BUILDINGS_PER_SIDE];

AABB getPlayerAABB() {
    AABB p;
    p.x = playerX;
    p.z = playerZ;
    p.halfW = PLAYER_COLLIDER_HALF_W;
    p.halfD = PLAYER_COLLIDER_HALF_D;
    p.active = true;
    return p;
}

AABB getTrashWorldAABB(const AABB& src) {
    // collider center IS the logical center in the world
    return src;
}



// Test if a player at (testX, testZ) would collide with any car or trash can
bool collidesWithAnyObstacleAt(float testX, float testZ) {
    AABB testBox;
    testBox.x = testX;
    testBox.z = testZ;
    testBox.halfW = PLAYER_COLLIDER_HALF_W;
    testBox.halfD = PLAYER_COLLIDER_HALF_D;
    testBox.active = true;

    // Check against cars
    for (int i = 0; i < numCars; ++i) {
        if (!cars[i].active) continue;
        if (checkAABBCollision(testBox, cars[i])) {
            return true;
        }
    }

    // Check against trash cans
    for (int i = 0; i < numTrashCans; ++i) {
        if (!trashCans[i].active) continue;
        AABB trashBox = getTrashWorldAABB(trashCans[i]);
        if (checkAABBCollision(testBox, trashBox)) {
            return true;
        }
    }

    return false;
}

void handleObstacleCollision(const AABB& obstacle) {
    // 1. Score penalty
    score -= 10;
    if (score < 0) score = 0;

    // 2. Compute overlap between player and obstacle
    AABB playerBox = getPlayerAABB();

    float dx = playerBox.x - obstacle.x;
    float dz = playerBox.z - obstacle.z;

    float overlapX = (playerBox.halfW + obstacle.halfW) - fabsf(dx);
    float overlapZ = (playerBox.halfD + obstacle.halfD) - fabsf(dz);

    // If for some reason there is no overlap, do nothing
    if (overlapX <= 0.0f || overlapZ <= 0.0f) {
        return;
    }

    // Extra amount to make it look like a bounce, not just unclipping
    const float bounceExtra = 0.3f;

    if (overlapX < overlapZ) {
        // Smaller overlap on X, push sideways away from obstacle
        float dirX = (dx >= 0.0f) ? 1.0f : -1.0f;
        playerX += dirX * (overlapX + bounceExtra);
    }
    else {
        // Smaller overlap on Z, push along the runner direction
        float dirZ = (dz >= 0.0f) ? 1.0f : -1.0f;
        playerZ += dirZ * (overlapZ + bounceExtra);
    }

    // A tiny extra push backwards along +Z so the black cube clearly bounces back
    playerZ += 0.5f;

    // 3. Clamp inside street
    if (playerX < streetMinX) playerX = streetMinX;
    if (playerX > streetMaxX) playerX = streetMaxX;
}


void spawnExtraObstacles(int extraCars, int extraTrash) {
    // Add extra cars into any free slots
    while (extraCars > 0 && numCars < MAX_CARS) {
        AABB& c = cars[numCars];
        c.x = randRange(streetMinX + 2.0f, streetMaxX - 2.0f);
        c.z = randRange(streetEndZ + 30.0f, -10.0f);
        c.halfW = CAR_COLLIDER_HALF_W;
        c.halfD = CAR_COLLIDER_HALF_D;
        c.active = true;
        numCars++;
        extraCars--;
    }

    // Add extra trash cans in the street where player can reach them
    while (extraTrash > 0 && numTrashCans < MAX_TRASHCANS) {
        AABB& t = trashCans[numTrashCans];
        // Place in street (inside player boundaries)
        bool useLeft = (numTrashCans % 2 == 0);
        t.x = useLeft ? -9.0f : 9.0f;  // Inside street boundaries
        // Place near buildings (closer to player)
        // Store collision box at base position
        t.z = randRange(-50.0f, -10.0f);
        t.halfW = TRASH_COLLIDER_HALF_W;
        t.halfD = TRASH_COLLIDER_HALF_D;
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
    numCars = 5;  // you can change this up to MAX_CARS

    float carMinZ = streetEndZ + 20.0f; // e.g. -180
    float carMaxZ = -10.0f;

    // ==== Init looping buildings ====
// Player moves in -Z, so "in front" means more negative Z.
    float firstZ = playerZ - 40.0f;  // first building row in front

    for (int i = 0; i < NUM_BUILDINGS_PER_SIDE; ++i) {
        float z = firstZ - i * BUILDING_SPACING_Z;
        leftBuildingZ[i] = z;
        rightBuildingZ[i] = z;
    }
    // closer to player

    for (int i = 0; i < numCars; ++i) {
        cars[i].x = randRange(streetMinX + 2.0f, streetMaxX - 2.0f);
        cars[i].z = randRange(carMinZ, carMaxZ);
        cars[i].halfW = CAR_COLLIDER_HALF_W;
        cars[i].halfD = CAR_COLLIDER_HALF_D;
        cars[i].active = true;
    }


    // ----- Trash cans -----
    // Place trash cans in the street where player can collide with them
    // Buildings are at z positions: streetStartZ, streetStartZ-60, streetStartZ-120, etc.
    // Place trash cans slightly in front of each building (closer to camera = higher Z)
    // Position them at the edge of the street so player can reach them
    const float leftTrashX = -9.0f;   // Inside street boundary (streetMinX = -10.0f)
    const float rightTrashX = 9.0f;   // Inside street boundary (streetMaxX = 10.0f)
    const float buildingSpacing = 60.0f;       // Buildings are spaced 60 units apart
    const float trashOffsetZ = 2.0f;
    const float trashSpacing = 40.0f;
    // Place trash cans 2 units in front of buildings

    numTrashCans = 0;

    for (float z = -30.0f; z > streetEndZ && numTrashCans < MAX_TRASHCANS; z -= trashSpacing) {
        // Left side trash can
        if (numTrashCans < MAX_TRASHCANS) {
            trashCans[numTrashCans].x = leftTrashX;
            trashCans[numTrashCans].z = z;
            trashCans[numTrashCans].halfW = TRASH_COLLIDER_HALF_W;
            trashCans[numTrashCans].halfD = TRASH_COLLIDER_HALF_D;
            trashCans[numTrashCans].active = true;
            numTrashCans++;
        }

        // Right side trash can
        if (numTrashCans < MAX_TRASHCANS) {
            trashCans[numTrashCans].x = rightTrashX;
            trashCans[numTrashCans].z = z;
            trashCans[numTrashCans].halfW = TRASH_COLLIDER_HALF_W;
            trashCans[numTrashCans].halfD = TRASH_COLLIDER_HALF_D;
            trashCans[numTrashCans].active = true;
            numTrashCans++;
        }
    }
    // Add debug function to see where collision boxes are vs where models are

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

void debugDrawTrashAtOrigin() {
    glPushMatrix();

    // collider centre at origin
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);     // white cube for collider
    glutWireCube(1.0f);
    glEnable(GL_LIGHTING);

    // now draw the model shifted by the offset
    glTranslatef(TRASH_MODEL_OFFSET_X, 0.0f, TRASH_MODEL_OFFSET_Z);
    glScalef(0.006f, 0.006f, 0.006f);
    trashModel.Draw();

    glPopMatrix();
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
    // Force our own clean state for the ground
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    if (groundTexture.texture[0] != 0) {
        groundTexture.Use(); // Bind the ground texture

        // Make texture completely control the color (ignore vertex color)
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        const float groundExtentX = 200.0f;
        const float groundStartZ = 100.0f;
        const float groundEndZ = -1500.0f;

        const float textureTileSize = 10.0f;

        float startX = -groundExtentX;
        float endX = groundExtentX;
        float startZ = groundStartZ;
        float endZ = groundEndZ;

        float texStartX = startX / textureTileSize;
        float texEndX = endX / textureTileSize;
        float texStartZ = startZ / textureTileSize;
        float texEndZ = endZ / textureTileSize;

        glBegin(GL_QUADS);
        glTexCoord2f(texStartX, texStartZ); glVertex3f(startX, 0.0f, startZ);
        glTexCoord2f(texEndX, texStartZ); glVertex3f(endX, 0.0f, startZ);
        glTexCoord2f(texEndX, texEndZ);   glVertex3f(endX, 0.0f, endZ);
        glTexCoord2f(texStartX, texEndZ);   glVertex3f(startX, 0.0f, endZ);
        glEnd();
    }
    else {
        // fallback if texture not loaded
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.2f, 0.2f, 0.2f);
        const float groundExtentX = 200.0f;
        const float groundStartZ = 100.0f;
        const float groundEndZ = -1500.0f;
        glBegin(GL_QUADS);
        glVertex3f(-groundExtentX, 0.0f, groundStartZ);
        glVertex3f(groundExtentX, 0.0f, groundStartZ);
        glVertex3f(groundExtentX, 0.0f, groundEndZ);
        glVertex3f(-groundExtentX, 0.0f, groundEndZ);
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}


void drawBuildings() {
    glColor3f(1.0f, 1.0f, 1.0f);

    const float buildingScale = 0.23f;
    const float leftX = -30.0f;
    const float rightX = 30.0f;
    const float groundY = 0.0f;

    for (int i = 0; i < NUM_BUILDINGS_PER_SIDE; ++i) {
        float zL = leftBuildingZ[i];
        float zR = rightBuildingZ[i];

        // LEFT building
        glPushMatrix();
        glTranslatef(leftX, groundY, zL);
        glScalef(buildingScale, buildingScale, buildingScale);
        buildingModel.Draw();
        glPopMatrix();

        // RIGHT building
        glPushMatrix();
        glTranslatef(rightX, groundY, zR);
        glRotatef(180.0f, 0, 1, 0);
        glScalef(buildingScale, buildingScale, buildingScale);
        buildingModel.Draw();
        glPopMatrix();
    }
}



void drawCars() {
    for (int i = 0; i < numCars; i++) {
        const AABB& c = cars[i];
        if (!c.active) continue;

        glPushMatrix();

        // Position the car on the ground
        // Y = 0.0f means “place origin at ground level”
        // If wheels sink, increase Y slightly to 0.3f or 0.5f
        glTranslatef(c.x, 0.3f, c.z);

        // Rotate car so front faces -Z (adjust as needed)
        // Uncomment one of these if car is sideways:
        // glRotatef(90, 0, 1, 0);
        // glRotatef(-90, 0, 1, 0);
        // glRotatef(180, 0, 1, 0);

        // Correct scale — much smaller
        glScalef(0.00015f, 0.00015f, 0.00015f);


        // Draw car
        carModel.Draw();

        glPopMatrix();
    }
}


void drawTrashCans() {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);

    for (int i = 0; i < numTrashCans; i++) {
        const AABB& t = trashCans[i];
        if (!t.active) continue;

        glPushMatrix();
        // collider centre is (t.x, t.z) → shift mesh by same offset
        glTranslatef(t.x + TRASH_MODEL_OFFSET_X,
            0.0f,
            t.z + TRASH_MODEL_OFFSET_Z);
        glScalef(0.006f, 0.006f, 0.006f);
        trashModel.Draw();
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
    const float lampX = 16.0f;  // Increased from 11.0f to 12.0f for more spacing
    const float scale = 0.95f;
    const float lampY = -0.5f;   // lift lamp a bit above the ground

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

    // put player at same X,Z as the collision box
    glTranslatef(playerX, 0.0f, playerZ);

    // face along -Z (runner direction)
    glRotatef(180.0f, 0, 1, 0);

    // adjust this until size feels right
    glScalef(1.0f, 1.0f, 1.0f);   // try 0.02, then tweak up/down

    // use normal lighting & textures
    playerModel.Draw();

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

    // Warm street-lamp colour (yellowish/orange tint, not pure white)
    GLfloat ambient[] = { 0.1f, 0.08f, 0.05f, 1.0f };   // Warm ambient
    GLfloat diffuse[] = { 1.0f, 0.85f, 0.6f, 1.0f };    // Warm yellow/orange light (not white)
    GLfloat specular[] = { 0.9f, 0.8f, 0.7f, 1.0f };     // Warm specular

    for (int i = 0; i < 8; ++i) {
        GLenum L = GL_LIGHT0 + i;
        glLightfv(L, GL_AMBIENT, ambient);
        glLightfv(L, GL_DIFFUSE, diffuse);
        glLightfv(L, GL_SPECULAR, specular);

        // Same attenuation for all lamps
        glLightf(L, GL_CONSTANT_ATTENUATION, 0.5f);
        glLightf(L, GL_LINEAR_ATTENUATION, 0.02f);
        glLightf(L, GL_QUADRATIC_ATTENUATION, 0.008f);
    }
}


void updateLamp(float deltaTime) {
    (void)deltaTime; // we don't need it now

    const float lampX = 12.0f;   // same as in drawLamps() - increased spacing
    const float lampHeight = 6.0f;    // approximate lamp head height
    const float spacing = 60.0f;   // distance between building rows
    const float offsetZ = 30.0f;   // lamps are between buildings

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
            GLfloat posR[] = { lampX, lampHeight, midZ, 1.0f };
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
void drawTrashCollisionBoxes() {
    glDisable(GL_LIGHTING);
    glLineWidth(3.0f);
    glColor3f(1.0f, 0.0f, 1.0f); // bright magenta so it's obvious

    for (int i = 0; i < numTrashCans; i++) {
        if (!trashCans[i].active) continue;

        AABB box = getTrashWorldAABB(trashCans[i]);

        glPushMatrix();
        glTranslatef(box.x, 0.5f, box.z);
        glScalef(box.halfW * 2.0f, 1.0f, box.halfD * 2.0f);
        glutWireCube(1.0);
        glPopMatrix();
    }

    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}


void drawPlayerCollisionBox() {
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 1.0f, 0.0f); // green

    AABB p = getPlayerAABB();

    glPushMatrix();
    glTranslatef(p.x, 0.5f, p.z);
    glScalef(p.halfW * 2.0f, 1.0f, p.halfD * 2.0f);
    glutWireCube(1.0);
    glPopMatrix();

    glEnable(GL_LIGHTING);
}
void drawCarCollisionBoxes() {
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 0.0f, 1.0f); // blue

    for (int i = 0; i < numCars; ++i) {
        const AABB& c = cars[i];
        if (!c.active) continue;

        glPushMatrix();
        glTranslatef(c.x, 0.5f, c.z);
        glScalef(c.halfW * 2.0f, 1.0f, c.halfD * 2.0f);
        glutWireCube(1.0);
        glPopMatrix();
    }

    glEnable(GL_LIGHTING);
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
    //debugDrawTrashAtOrigin();
    //drawTrashCollisionBoxes();
    //drawCarCollisionBoxes();
    drawPlayerCollisionBox();
    drawCollectibles();
    drawCheckpoint();
    drawPlayer();


    drawScore();
    drawTimer();
    drawGameStatus();

    glutSwapBuffers();
}

void idle() {
    // 1. Time and delta time
    int   currentMs = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (currentMs - prevTimeMs) / 1000.0f;
    prevTimeMs = currentMs;

    // 2. Always animate lamps (even if game is over)
    updateLamp(deltaTime);

    // 3. Game logic only while playing
    if (gameState == GAME_PLAYING) {

        // 3.1 Timer / time up
        int elapsedSinceStart = currentMs - gameStartTimeMs;
        remainingTimeMs = gameDurationMs - elapsedSinceStart;

        if (remainingTimeMs <= 0) {
            remainingTimeMs = 0;
            gameState = GAME_LOST;   // time up
            glutPostRedisplay();
            return;
        }

        // 3.2 Checkpoint logic

        // First checkpoint . appears in last 20 seconds
        if (!checkpointSpawned20s && remainingTimeMs <= 20000) {
            checkpoint.x = 0.0f;
            checkpoint.z = playerZ - 60.0f;   // 60 units ahead of player
            checkpoint.halfW = 4.0f;
            checkpoint.halfD = 1.0f;
            checkpoint.active = true;
            checkpointSpawned20s = true;
        }

        // If checkpoint is active and player passed it without touching it, disable it
        if (checkpoint.active && playerZ < checkpoint.z - 5.0f) {
            checkpoint.active = false;
        }

        // Second chance . last 3 seconds
        if (!checkpointSpawned3s && remainingTimeMs <= 3000) {
            checkpoint.x = 0.0f;
            checkpoint.z = playerZ - 40.0f;   // closer this time
            checkpoint.halfW = 4.0f;
            checkpoint.halfD = 1.0f;
            checkpoint.active = true;
            checkpointSpawned3s = true;
        }

        // 3.3 Clamp player inside street
        if (playerX < streetMinX) playerX = streetMinX;
        if (playerX > streetMaxX) playerX = streetMaxX;

        // 3.4 Collision checks
        AABB playerBox = getPlayerAABB();

        // Player ↔ cars
        for (int i = 0; i < numCars; ++i) {
            if (!cars[i].active) continue;

            if (checkAABBCollision(playerBox, cars[i])) {
                handleObstacleCollision(cars[i]);
                // Rebuild player box after we moved the player
                playerBox = getPlayerAABB();
                break; // only handle one car per frame
            }
        }


        
        // Player ↔ trash cans
        for (int i = 0; i < numTrashCans; ++i) {
            if (!trashCans[i].active) continue;

            AABB trashBox = getTrashWorldAABB(trashCans[i]);  // <-- apply offset

            if (checkAABBCollision(playerBox, trashBox)) {
                handleObstacleCollision(trashBox);            // <-- use world-aligned box
                playerBox = getPlayerAABB();                  // rebuild after bounce
                break;
            }
        }



        // Player ↔ collectibles
        for (int i = 0; i < numCollectibles; ++i) {
            if (!collectibles[i].active) continue;

            if (checkAABBCollision(playerBox, collectibles[i])) {
                collectibles[i].active = false;
                score += 10;
                // TODO: 3ennabeya animation / sound
            }
        }

        // Player ↔ checkpoint
        if (checkpoint.active && checkAABBCollision(playerBox, checkpoint)) {
            checkpoint.active = false;
            gameState = GAME_WON;
            // TODO: trigger Level 2
        }

        // 3.5 Dynamic difficulty
        if (difficultyLevel == 0 && score >= 50) {
            difficultyLevel = 1;
            moveStep = 0.7f;
            spawnExtraObstacles(5, 5);
        }

        if (difficultyLevel == 1 && score >= 100) {
            difficultyLevel = 2;
            moveStep = 0.9f;
            spawnExtraObstacles(10, 10);
        }
    }

    // 4. Recycle buildings (works in both playing & not playing states)
    for (int i = 0; i < NUM_BUILDINGS_PER_SIDE; ++i) {
        // LEFT side
        if (leftBuildingZ[i] > playerZ + BUILDING_RECYCLE_Z) {
            float minZ = leftBuildingZ[0];
            for (int j = 1; j < NUM_BUILDINGS_PER_SIDE; ++j) {
                if (leftBuildingZ[j] < minZ)
                    minZ = leftBuildingZ[j];
            }
            leftBuildingZ[i] = minZ - BUILDING_SPACING_Z;
        }

        // RIGHT side
        if (rightBuildingZ[i] > playerZ + BUILDING_RECYCLE_Z) {
            float minZ = rightBuildingZ[0];
            for (int j = 1; j < NUM_BUILDINGS_PER_SIDE; ++j) {
                if (rightBuildingZ[j] < minZ)
                    minZ = rightBuildingZ[j];
            }
            rightBuildingZ[i] = minZ - BUILDING_SPACING_Z;
        }
    }

    // 5. Request redraw
    glutPostRedisplay();
}




void keyboard(unsigned char key, int x, int y) {
    if (gameState != GAME_PLAYING && key != 27) {
        return;
    }

    float newX = playerX;
    float newZ = playerZ;

    switch (key) {
    case 'a':
    case 'A':
        newX -= moveStep;
        break;

    case 'd':
    case 'D':
        newX += moveStep;
        break;

    case 'w':
    case 'W':
        newZ -= moveStep;    // move forward along -Z
        break;

    case 's':
    case 'S':
        newZ += moveStep;    // move backward along +Z
        break;

    case '1':
        cameraMode = FIRST_PERSON;
        glutPostRedisplay();
        return;

    case '3':
        cameraMode = THIRD_PERSON;
        glutPostRedisplay();
        return;

    case 27: // ESC
        exit(0);
        return;
    }

    // Clamp inside the street only on X
    if (newX < streetMinX) newX = streetMinX;
    if (newX > streetMaxX) newX = streetMaxX;

    // Just apply the movement.
    // Collisions (score change + knock-back) are handled in idle().
    playerX = newX;
    playerZ = newZ;

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
    trashModel.Load("models/Urn3.3ds");
    carModel.Load("models/carKiaPicantoN240910.3ds");
    playerModel.Load("models/Player.3ds");
    // ===== Player texture =====
    if (playerModel.numMaterials > 0) {
        char playerTexPath[256];
        // If you saved as BMP:
        strcpy_s(playerTexPath, sizeof(playerTexPath),
            "textures/Ch24_1001_Diffuse.bmp");
        // If you kept PNG, use .png here instead.

        // Apply same diffuse texture to first material
        playerModel.Materials[0].tex.Load(playerTexPath);
        playerModel.Materials[0].textured = true;

        // If the model has more materials and looks half-white
        // you can loop and assign the same texture to all:
        /*
        for (int i = 0; i < playerModel.numMaterials; ++i) {
            playerModel.Materials[i].tex.Load(playerTexPath);
            playerModel.Materials[i].textured = true;
        }
        */
    }

    buildingModel.Load("models/cottage.3ds");

    // Load ground texture
    char groundTexturePath[256];
    strcpy_s(groundTexturePath, sizeof(groundTexturePath), "textures/ground1.bmp");
    groundTexture.Load(groundTexturePath);

    // Set texture wrapping to repeat so it tiles across the ground
    // This must be done after loading the texture and OpenGL context is ready
    if (groundTexture.texture[0] != 0) {
        glBindTexture(GL_TEXTURE_2D, groundTexture.texture[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // Apply textures to trash model materials
    // Load textures from textures directory and apply to model materials
    if (trashModel.numMaterials > 0) {
        // Load textures - create mutable strings since GLTexture::Load() modifies them
        // Using relative paths from the executable's working directory
        char texturePath1[256];
        char texturePath2[256];
        strcpy_s(texturePath1, sizeof(texturePath1), "textures/4_1_9_d.bmp");
        strcpy_s(texturePath2, sizeof(texturePath2), "textures/ConcrMet.bmp");

        // Load and apply first texture (4_1_9_d.bmp) to first material
        // Note: GLTexture::Load() converts path to lowercase internally
        trashModel.Materials[0].tex.Load(texturePath1);
        trashModel.Materials[0].textured = true;

        // Apply second texture (ConcrMet.bmp) to second material if available
        if (trashModel.numMaterials > 1) {
            trashModel.Materials[1].tex.Load(texturePath2);
            trashModel.Materials[1].textured = true;
        }
        // If model has only one material, it will use the first texture
        // Some 3DS models may have multiple objects sharing materials
    }

    // Person B: fill correct paths to .3ds files and handle textures
    // Example:
    // playerModel.Load("models/Player.3ds");
    // carModel.Load("models/Car.3ds");
    // collectibleModel.Load("models/3ennabeya.3ds");
    // checkpointModel.Load("models/Checkpoint.3ds");
    // buildingModel.Load("models/Building.3ds");
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
// LEVEL 2 CODE STARTS HERE - Amir's Work (FIXED)
// ===================================================================
#ifdef RUN_LEVEL_2

// ===============================
// Level 2 Specific Structures
// ===============================

// Level 2 AABB with Y coordinate for flying
struct AABB_L2 {
    float x, y, z;   // 3D position
    float halfW, halfD, halfH; // 3D dimensions
    bool active;
};

// ===============================
// Level 2 Global Variables
// ===============================

// Player state for flying
float playerX_L2 = 0.0f;      // side movement
float playerY_L2 = 25.0f;     // flying height (Y-axis)
float playerZ_L2 = 0.0f;      // forward movement (flying direction, negative Z)
float playerSpeed_L2 = 15.0f; // Forward speed

// Flying boundaries
const float riverWidth = 100.0f;
const float riverMinX = -riverWidth / 2;
const float riverMaxX = riverWidth / 2;
const float minHeight = 10.0f;
const float maxHeight = 50.0f;
const float riverStartZ = 100.0f;   // Start point
const float riverEndZ = -1500.0f;   // End point

// Camera
CameraMode cameraMode_L2 = THIRD_PERSON;
float thirdPersonDist_L2 = 15.0f;  // Camera distance behind player
float thirdPersonHeight_L2 = 8.0f; // Camera height above player

// Score
int score_L2 = 0;

// Flying collectibles (3ennabeyat)
const int MAX_FLYING_COLLECTIBLES = 50;
AABB_L2 flyingCollectibles[MAX_FLYING_COLLECTIBLES];
int numFlyingCollectibles = 0;

// Obstacles (birds)
const int MAX_BIRDS = 15;
AABB_L2 birds[MAX_BIRDS];
int numBirds = 0;

// Target (Dr. Beram)
AABB_L2 drBeram;
bool drBeramRescued = false;
float rescueAnimationTime = 0.0f;

// Lighting for sunset
float sunColor[3] = { 1.0f, 0.9f, 0.0f }; // Yellow (start)
float sunsetProgress = 0.0f; // 0.0 = start, 1.0 = sunset complete
bool sunsetActive = true;

// Time and movement
int prevTimeMs_L2 = 0;
float moveStep_L2 = 4.0f; // Horizontal movement speed
float verticalSpeed = 0.0f; // For up/down movement
float liftForce = 15.0f;   // Upward force when ascending

// Game state
GameState gameState_L2 = GAME_PLAYING;
const int level2DurationMs = 120000; // 120 seconds for flying level
int level2StartTimeMs = 0;
int remainingTime_L2 = level2DurationMs;

// Models - Use placeholders since 3DS files don't exist
// Model_3DS playerModel_L2; // Comment out - using glut placeholders
// Model_3DS birdModel;
// Model_3DS collectibleModel_L2;
// Model_3DS drBeramModel;

// Animation states
float collectibleRotation = 0.0f;
float birdFlapAnimation = 0.0f;
float heroWingFlap = 0.0f;

// Collectible collection animation
struct CollectibleAnim {
    int index;
    float scale;
    float rotation;
    float alpha;
    bool active;
};
const int MAX_ANIM_COLLECTIBLES = 10;
CollectibleAnim collectibleAnimations[MAX_ANIM_COLLECTIBLES];

// Model loading flags
bool modelsLoaded = false;

// Sky texture - ADDED
GLTexture skyTexture;

// ===============================
// Level 2 Key State Tracking (NEW)
// ===============================
bool keyStates[256] = { false }; // Track all key states

// ===============================
// Level 2 Helper Functions
// ===============================

// Check collision in 3D
bool checkAABBCollision3D(const AABB_L2& a, const AABB_L2& b) {
    if (!a.active || !b.active) return false;

    bool overlapX = fabs(a.x - b.x) <= (a.halfW + b.halfW);
    bool overlapY = fabs(a.y - b.y) <= (a.halfH + b.halfH);
    bool overlapZ = fabs(a.z - b.z) <= (a.halfD + b.halfD);
    return overlapX && overlapY && overlapZ;
}

// Get player's 3D bounding box
AABB_L2 getPlayerAABB_L2() {
    AABB_L2 p;
    p.x = playerX_L2;
    p.y = playerY_L2;
    p.z = playerZ_L2;
    p.halfW = 2.0f;   // Player width
    p.halfH = 2.0f;   // Player height
    p.halfD = 3.0f;   // Player depth
    p.active = true;
    return p;
}

// Handle collision with bird
void handleBirdCollision(int birdIndex) {
    // Deduct points
    score_L2 -= 15;
    if (score_L2 < 0) score_L2 = 0;

    // Push back and down
    playerZ_L2 += 10.0f;
    playerY_L2 -= 5.0f;
    verticalSpeed = 0.0f; // Strong downward push

    // Deactivate bird
    birds[birdIndex].active = false;

    // Clamp position
    if (playerY_L2 < minHeight) playerY_L2 = minHeight;
    if (playerY_L2 > maxHeight) playerY_L2 = maxHeight;
}

// Add collectible animation
void addCollectibleAnimation(int index, float x, float y, float z) {
    for (int i = 0; i < MAX_ANIM_COLLECTIBLES; i++) {
        if (!collectibleAnimations[i].active) {
            collectibleAnimations[i].active = true;
            collectibleAnimations[i].scale = 1.0f;
            collectibleAnimations[i].rotation = 0.0f;
            collectibleAnimations[i].alpha = 1.0f;
            collectibleAnimations[i].index = index;
            break;
        }
    }
}

// Setup Level 2 - Flying over Nile
void setupLevel2() {
    // Reset player
    playerX_L2 = 0.0f;
    playerY_L2 = 25.0f;
    playerZ_L2 = riverStartZ;
    playerSpeed_L2 = 20.0f;
    score_L2 = 0;
    sunsetProgress = 0.0f;
    sunsetActive = true;
    drBeramRescued = false;
    rescueAnimationTime = 0.0f;

    // Reset sunset color to yellow
    sunColor[0] = 1.0f; // Red
    sunColor[1] = 0.9f; // Green
    sunColor[2] = 0.0f; // Blue

    // Clear animations
    for (int i = 0; i < MAX_ANIM_COLLECTIBLES; i++) {
        collectibleAnimations[i].active = false;
    }

    // Setup flying collectibles (3ennabeyat)
    numFlyingCollectibles = 40;
    for (int i = 0; i < numFlyingCollectibles; i++) {
        flyingCollectibles[i].x = randRange(riverMinX + 5, riverMaxX - 5);
        flyingCollectibles[i].y = randRange(15, 40); // Different heights
        flyingCollectibles[i].z = riverStartZ - 50.0f - (i * 35.0f); // Spread along Z
        flyingCollectibles[i].halfW = 1.5f;
        flyingCollectibles[i].halfH = 1.5f;
        flyingCollectibles[i].halfD = 1.5f;
        flyingCollectibles[i].active = true;
    }

    // Setup birds (obstacles)
    numBirds = 12;
    for (int i = 0; i < numBirds; i++) {
        birds[i].x = randRange(riverMinX + 10, riverMaxX - 10);
        birds[i].y = randRange(20, 35);
        birds[i].z = riverStartZ - 80.0f - (i * 60.0f);
        birds[i].halfW = 3.0f;
        birds[i].halfH = 2.0f;
        birds[i].halfD = 4.0f;
        birds[i].active = true;
    }

    // Setup Dr. Beram (target) - at the end of the river
    drBeram.x = 0.0f;
    drBeram.y = 30.0f;
    drBeram.z = riverEndZ + 50.0f; // Near the end
    drBeram.halfW = 3.0f;
    drBeram.halfH = 4.0f;
    drBeram.halfD = 2.0f;
    drBeram.active = true;
}

// Camera for flying level - FIXED to properly follow player
void setupCameraLevel2() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (cameraMode_L2 == FIRST_PERSON) {
        // First person: from player's viewpoint
        gluLookAt(
            playerX_L2, playerY_L2 + 2.0f, playerZ_L2,        // Eye position
            playerX_L2, playerY_L2 + 2.0f, playerZ_L2 - 50.0f, // Look at point ahead
            0, 1, 0                                          // Up vector
        );
    }
    else { // THIRD_PERSON
        // Third person: behind and above, always looking at player
        float camX = playerX_L2;
        float camY = playerY_L2 + thirdPersonHeight_L2;
        float camZ = playerZ_L2 + thirdPersonDist_L2;

        // Look at point slightly ahead of player for better view
        float lookAtX = playerX_L2;
        float lookAtY = playerY_L2 + 2.0f;
        float lookAtZ = playerZ_L2 - 10.0f;

        gluLookAt(
            camX, camY, camZ,        // Camera position (behind and above)
            lookAtX, lookAtY, lookAtZ, // Look at player
            0, 1, 0                  // Up vector
        );
    }
}

// Draw sky with sunset gradient and texture - UPDATED for front screen only
void drawSky() {
    glDisable(GL_LIGHTING);

    // Use the sky texture if loaded
    if (skyTexture.texture[0] != 0) {
        glEnable(GL_TEXTURE_2D);
        skyTexture.Use(); // Bind the sky texture

        // Use modulate to blend texture with sunset colors
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        // Apply sunset tint (more orange/red as sunset progresses)
        glColor4f(1.0f,
            1.0f - sunsetProgress * 0.5f,  // Less green
            1.0f - sunsetProgress * 0.7f,  // Less blue
            1.0f);
    }
    else {
        // Fallback to solid color gradient if texture not loaded
        glDisable(GL_TEXTURE_2D);

        // Simple gradient front screen
        glBegin(GL_QUADS);
        // Bottom-left (horizon - orange)
        glColor3f(0.8f + sunsetProgress * 0.2f,
            0.3f + sunsetProgress * 0.4f,
            0.1f);
        glVertex3f(-1500, 0, -2500);

        // Bottom-right
        glColor3f(0.8f + sunsetProgress * 0.2f,
            0.3f + sunsetProgress * 0.4f,
            0.1f);
        glVertex3f(1500, 0, -2500);

        // Top-right (sky - blue/orange)
        glColor3f(0.1f,
            0.2f + sunsetProgress * 0.3f,
            0.8f - sunsetProgress * 0.7f);
        glVertex3f(1500, 800, -2500);

        // Top-left
        glColor3f(0.1f,
            0.2f + sunsetProgress * 0.3f,
            0.8f - sunsetProgress * 0.7f);
        glVertex3f(-1500, 800, -2500);
        glEnd();

        glEnable(GL_LIGHTING);
        return;
    }

    // Draw a large front screen (billboard) far in the distance
    // This creates a "skybox front" effect
    float screenWidth = 2000.0f;   // Very wide
    float screenHeight = 800.0f;   // Very tall
    float screenDepth = -2500.0f;  // Far away

    glBegin(GL_QUADS);

    // Bottom-left corner (near horizon)
    // Texture coordinates: bottom of texture at horizon
    glTexCoord2f(0.0f, 0.7f);
    glVertex3f(-screenWidth, 0, screenDepth);

    // Bottom-right corner
    glTexCoord2f(1.0f, 0.7f);
    glVertex3f(screenWidth, 0, screenDepth);

    // Top-right corner (sky)
    // Texture coordinates: top of texture at top of screen
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(screenWidth, screenHeight, screenDepth);

    // Top-left corner
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-screenWidth, screenHeight, screenDepth);

    glEnd();

    // Draw sun on the front screen (as part of the sky texture enhancement)
    if (sunsetProgress < 0.8f) {
        glDisable(GL_TEXTURE_2D);

        // Sun position based on sunset progress
        float sunSize = 25.0f - sunsetProgress * 12.0f;
        float sunY = 400.0f - sunsetProgress * 300.0f; // Sun goes down
        float sunX = 500.0f - sunsetProgress * 400.0f; // Sun moves left

        // Draw sun as a glowing sphere
        glPushMatrix();
        glTranslatef(sunX, sunY, screenDepth + 5.0f); // Slightly in front of sky

        // Main sun disc
        glColor3f(sunColor[0], sunColor[1], sunColor[2]);
        glutSolidSphere(sunSize, 32, 32);

        // Sun glow/halo (larger, semi-transparent)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(sunColor[0], sunColor[1], sunColor[2], 0.3f);
        glutSolidSphere(sunSize * 1.5f, 24, 24);
        glDisable(GL_BLEND);

        glPopMatrix();
    }

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

// Draw Nile River with texture - IMPROVED
void drawNileRiver() {
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    // River water color with sunset reflection
    glColor3f(0.1f, 0.3f + sunsetProgress * 0.3f, 0.6f);

    glBegin(GL_QUADS);
    // River surface
    glVertex3f(-200, 0, riverEndZ);
    glVertex3f(200, 0, riverEndZ);
    glVertex3f(200, 0, riverStartZ + 100);
    glVertex3f(-200, 0, riverStartZ + 100);
    glEnd();

    // River banks
    glColor3f(0.5f, 0.4f, 0.2f);
    glBegin(GL_QUADS);
    // Left bank
    glVertex3f(-250, 0, riverEndZ);
    glVertex3f(-200, 0, riverEndZ);
    glVertex3f(-200, 0, riverStartZ + 100);
    glVertex3f(-250, 0, riverStartZ + 100);
    // Right bank
    glVertex3f(200, 0, riverEndZ);
    glVertex3f(250, 0, riverEndZ);
    glVertex3f(250, 0, riverStartZ + 100);
    glVertex3f(200, 0, riverStartZ + 100);
    glEnd();

    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
}

// Draw flying collectibles (3ennabeyat) with animation
void drawFlyingCollectibles() {
    collectibleRotation += 2.0f;
    if (collectibleRotation > 360.0f) collectibleRotation -= 360.0f;

    glColor3f(1.0f, 0.0f, 0.0f); // Red for 3ennab
    glEnable(GL_COLOR_MATERIAL);

    for (int i = 0; i < numFlyingCollectibles; i++) {
        if (!flyingCollectibles[i].active) continue;

        glPushMatrix();
        glTranslatef(flyingCollectibles[i].x,
            flyingCollectibles[i].y,
            flyingCollectibles[i].z);

        // Rotating animation
        glRotatef(collectibleRotation, 0, 1, 0);

        // Draw as a shiny red gem
        glutSolidSphere(2.0, 16, 16);

        // Add a glow effect
        glDisable(GL_LIGHTING);
        glColor4f(1.0f, 0.3f, 0.3f, 0.5f);
        glutSolidSphere(2.5, 12, 12);
        glEnable(GL_LIGHTING);

        glPopMatrix();
    }

    // Draw collection animations
    glDisable(GL_LIGHTING);
    for (int i = 0; i < MAX_ANIM_COLLECTIBLES; i++) {
        if (collectibleAnimations[i].active) {
            int idx = collectibleAnimations[i].index;
            if (idx >= 0 && idx < numFlyingCollectibles) {
                glPushMatrix();
                glTranslatef(flyingCollectibles[idx].x,
                    flyingCollectibles[idx].y,
                    flyingCollectibles[idx].z);

                // Scale down and fade out animation
                collectibleAnimations[i].scale *= 0.9f;
                collectibleAnimations[i].rotation += 10.0f;
                collectibleAnimations[i].alpha *= 0.8f;

                glRotatef(collectibleAnimations[i].rotation, 0, 1, 0);
                glScalef(collectibleAnimations[i].scale,
                    collectibleAnimations[i].scale,
                    collectibleAnimations[i].scale);

                glColor4f(1.0f, 0.0f, 0.0f, collectibleAnimations[i].alpha);
                glutSolidSphere(2.0, 12, 12);

                glPopMatrix();

                if (collectibleAnimations[i].alpha < 0.1f) {
                    collectibleAnimations[i].active = false;
                }
            }
        }
    }
    glEnable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
}

// Draw birds with flapping animation
void drawBirds() {
    birdFlapAnimation += 5.0f;
    if (birdFlapAnimation > 360.0f) birdFlapAnimation -= 360.0f;

    float flapAngle = sin(birdFlapAnimation * 3.14159f / 180.0f) * 20.0f;

    glColor3f(0.3f, 0.3f, 0.3f); // Gray birds
    glEnable(GL_COLOR_MATERIAL);

    for (int i = 0; i < numBirds; i++) {
        if (!birds[i].active) continue;

        glPushMatrix();
        glTranslatef(birds[i].x, birds[i].y, birds[i].z);

        // Flapping animation
        glRotatef(flapAngle, 1, 0, 0);

        // Body
        glutSolidSphere(2.0, 10, 10);

        // Head
        glPushMatrix();
        glTranslatef(0, 0.5f, -2.5f);
        glutSolidSphere(1.0, 8, 8);
        glPopMatrix();

        // Wings
        glDisable(GL_LIGHTING);
        glColor3f(0.2f, 0.2f, 0.2f);
        glBegin(GL_TRIANGLES);
        // Left wing
        glVertex3f(-3.0f, 0, 0);
        glVertex3f(-5.0f, 0, -3.0f);
        glVertex3f(-3.0f, 0, -2.0f);
        // Right wing
        glVertex3f(3.0f, 0, 0);
        glVertex3f(5.0f, 0, -3.0f);
        glVertex3f(3.0f, 0, -2.0f);
        glEnd();
        glEnable(GL_LIGHTING);

        glPopMatrix();
    }

    glDisable(GL_COLOR_MATERIAL);
}

// Draw Dr. Beram with rescue animation
void drawDrBeram() {
    if (!drBeram.active && !drBeramRescued) return;

    glPushMatrix();

    if (drBeramRescued) {
        // Rescue animation: float up with player
        float animY = drBeram.y + rescueAnimationTime * 5.0f;
        glTranslatef(playerX_L2, playerY_L2 + 5.0f + animY, playerZ_L2 - 5.0f);
        glColor3f(0.0f, 1.0f, 0.0f); // Bright green when rescued
    }
    else {
        glTranslatef(drBeram.x, drBeram.y, drBeram.z);
        glColor3f(0.0f, 0.8f, 0.0f); // Green for visibility
    }

    // Draw as a person shape
    // Body
    glPushMatrix();
    glScalef(1.5f, 4.0f, 1.0f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Head
    glPushMatrix();
    glTranslatef(0, 2.5f, 0);
    glutSolidSphere(1.0, 12, 12);
    glPopMatrix();

    // Arms
    glBegin(GL_LINES);
    glVertex3f(0, 1.5f, 0);
    glVertex3f(2.0f, 1.5f, 0);
    glVertex3f(0, 1.5f, 0);
    glVertex3f(-2.0f, 1.5f, 0);
    glEnd();

    // Legs
    glBegin(GL_LINES);
    glVertex3f(0, -1.5f, 0);
    glVertex3f(1.0f, -3.0f, 0);
    glVertex3f(0, -1.5f, 0);
    glVertex3f(-1.0f, -3.0f, 0);
    glEnd();

    glPopMatrix();
}

// Draw flying player with wing flap animation
void drawPlayerLevel2() {
    heroWingFlap += 10.0f;
    if (heroWingFlap > 360.0f) heroWingFlap -= 360.0f;

    float wingFlap = sin(heroWingFlap * 3.14159f / 180.0f) * 15.0f;

    glPushMatrix();
    glTranslatef(playerX_L2, playerY_L2, playerZ_L2);

    // Rotate based on movement
    float tiltAngle = (playerX_L2 - riverMinX) / (riverMaxX - riverMinX) * 30.0f - 15.0f;
    glRotatef(tiltAngle, 0, 0, 1);

    // Superhero color
    glColor3f(1.0f, 0.5f, 0.0f); // Orange hero
    glEnable(GL_COLOR_MATERIAL);

    // Body
    glPushMatrix();
    glScalef(2.0f, 3.0f, 1.5f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Head
    glPushMatrix();
    glTranslatef(0, 2.2f, 0);
    glutSolidSphere(1.0, 16, 16);
    glPopMatrix();

    // Cape (flowing back)
    glDisable(GL_LIGHTING);
    glColor3f(0.8f, 0.0f, 0.0f); // Red cape
    glBegin(GL_TRIANGLE_STRIP);
    glVertex3f(0, 1.0f, 0.5f);
    glVertex3f(0, -1.0f, 0.5f);
    glVertex3f(-1.0f, 0.5f, -1.0f);
    glVertex3f(-1.0f, -1.5f, -1.0f);
    glVertex3f(1.0f, 0.5f, -1.0f);
    glVertex3f(1.0f, -1.5f, -1.0f);
    glEnd();
    glEnable(GL_LIGHTING);

    // Wings with flapping animation
    glDisable(GL_LIGHTING);
    glColor3f(0.9f, 0.9f, 0.1f); // Yellow wings

    // Left wing
    glPushMatrix();
    glRotatef(wingFlap, 1, 0, 0);
    glBegin(GL_TRIANGLES);
    glVertex3f(-1.5f, 0, 0);
    glVertex3f(-4.0f, 0, -3.0f);
    glVertex3f(-1.5f, 0, -2.0f);
    glEnd();
    glPopMatrix();

    // Right wing
    glPushMatrix();
    glRotatef(-wingFlap, 1, 0, 0);
    glBegin(GL_TRIANGLES);
    glVertex3f(1.5f, 0, 0);
    glVertex3f(4.0f, 0, -3.0f);
    glVertex3f(1.5f, 0, -2.0f);
    glEnd();
    glPopMatrix();

    glEnable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);

    glPopMatrix();
}

// Setup sunset lighting - IMPROVED with visible animation
void setupSunLight() {
    glEnable(GL_LIGHTING);

    // Main sunlight (LIGHT0)
    glEnable(GL_LIGHT0);

    // Sun color based on sunset progress
    GLfloat lightDiffuse[] = { sunColor[0], sunColor[1], sunColor[2], 1.0f };
    GLfloat lightAmbient[] = {
        0.2f + sunsetProgress * 0.3f,
        0.2f + sunsetProgress * 0.2f,
        0.1f + sunsetProgress * 0.1f,
        1.0f
    };
    GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Sun position (moves with sunset)
    float sunX = 200.0f - sunsetProgress * 300.0f;
    float sunY = 150.0f - sunsetProgress * 100.0f;
    GLfloat lightPosition[] = { sunX, sunY, -500.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    // Fill light (LIGHT1) for better illumination
    glEnable(GL_LIGHT1);
    GLfloat fillAmbient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat fillDiffuse[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    GLfloat fillPosition[] = { 0.0f, 100.0f, 0.0f, 1.0f };

    glLightfv(GL_LIGHT1, GL_AMBIENT, fillAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, fillDiffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, fillPosition);
}

// Update sunset progress - IMPROVED
void updateSunset(float deltaTime) {
    if (!sunsetActive) return;

    // Gradually progress sunset based on time and player position
    float progressByTime = (float)(level2DurationMs - remainingTime_L2) / level2DurationMs;
    float progressByDistance = (riverStartZ - playerZ_L2) / (riverStartZ - riverEndZ);

    // Combine both factors
    sunsetProgress = 0.7f * progressByTime + 0.3f * progressByDistance;

    if (sunsetProgress > 1.0f) sunsetProgress = 1.0f;

    // Smooth color transition from yellow to orange/red
    sunColor[0] = 1.0f;                    // Red stays high
    sunColor[1] = 0.9f - (sunsetProgress * 0.7f); // Green decreases
    sunColor[2] = 0.1f - (sunsetProgress * 0.1f); // Blue decreases slightly
}

// Draw HUD for Level 2
void drawScoreLevel2() {
    char buffer[64];
    sprintf(buffer, "Score: %d", score_L2);
    drawText2D(-0.95f, 0.9f, buffer);
}

void drawTimerLevel2() {
    int seconds = remainingTime_L2 / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;

    char buffer[64];
    sprintf(buffer, "Time: %02d:%02d", minutes, seconds);
    drawText2D(0.6f, 0.9f, buffer);
}

void drawHeightIndicator() {
    char buffer[64];
    sprintf(buffer, "Height: %.0f", playerY_L2);
    drawText2D(-0.95f, 0.8f, buffer);
}

void drawSpeedIndicator() {
    char buffer[64];
    sprintf(buffer, "Speed: %.0f", playerSpeed_L2);
    drawText2D(-0.95f, 0.7f, buffer);
}

void drawCameraMode() {
    const char* mode = (cameraMode_L2 == FIRST_PERSON) ? "1st Person" : "3rd Person";
    char buffer[64];
    sprintf(buffer, "Camera: %s", mode);
    drawText2D(0.6f, 0.8f, buffer);
}

void drawControlsInfo() {
    drawText2D(-0.95f, -0.9f, "Controls: A/D=Left/Right, W/S=Up/Down, Arrows=Speed");
    drawText2D(-0.95f, -0.95f, "Up=Speed+, Down=Speed-, 1/3=Camera, ESC=Exit");
}

void drawGameStatusLevel2() {
    if (gameState_L2 == GAME_WON) {
        glDisable(GL_LIGHTING);
        drawText2D(-0.35f, 0.2f, "MISSION ACCOMPLISHED!");
        drawText2D(-0.3f, 0.1f, "Dr. Beram Rescued!");
        drawText2D(-0.25f, 0.0f, "Final Score: ");

        char scoreBuffer[32];
        sprintf(scoreBuffer, "%d", score_L2);
        drawText2D(0.05f, 0.0f, scoreBuffer);

        drawText2D(-0.4f, -0.1f, "Press ESC to exit");
        glEnable(GL_LIGHTING);
    }
    else if (gameState_L2 == GAME_LOST) {
        glDisable(GL_LIGHTING);
        drawText2D(-0.3f, 0.1f, "MISSION FAILED!");
        drawText2D(-0.35f, 0.0f, "Time's up or crashed!");
        drawText2D(-0.4f, -0.1f, "Press ESC to exit");
        glEnable(GL_LIGHTING);
    }
}

// ===============================
// Level 2 GLUT Callbacks - UPDATED for smooth movement
// ===============================

void keyboardLevel2(unsigned char key, int x, int y) {
    if (gameState_L2 != GAME_PLAYING) {
        if (key == 27) exit(0); // ESC to exit
        return;
    }

    switch (key) {
    case 'a': case 'A': // Left - PRESS
        keyStates['a'] = true;
        break;
    case 'd': case 'D': // Right - PRESS
        keyStates['d'] = true;
        break;
    case 'w': case 'W': // Speed up - PRESS
        keyStates['w'] = true;
        break;
    case 's': case 'S': // Slow down - PRESS
        keyStates['s'] = true;
        break;
    case ' ': // Space for up - PRESS
        keyStates[' '] = true;
        break;
    case 'c': case 'C': // Down - PRESS
        keyStates['c'] = true;
        break;
    case '1': // First person
        cameraMode_L2 = FIRST_PERSON;
        break;
    case '3': // Third person
        cameraMode_L2 = THIRD_PERSON;
        break;
    case 27: // ESC
        exit(0);
        break;
    }

    glutPostRedisplay();
}

void keyboardUpLevel2(unsigned char key, int x, int y) {
    // Handle key releases for smooth movement
    switch (key) {
    case 'a': case 'A': // Left - RELEASE
        keyStates['a'] = false;
        break;
    case 'd': case 'D': // Right - RELEASE
        keyStates['d'] = false;
        break;
    case 'w': case 'W': // Speed up - RELEASE
        keyStates['w'] = false;
        break;
    case 's': case 'S': // Slow down - RELEASE
        keyStates['s'] = false;
        break;
    case ' ': // Space for up - RELEASE
        keyStates[' '] = false;
        break;
    case 'c': case 'C': // Down - RELEASE
        keyStates['c'] = false;
        break;
    }
}

void specialKeysLevel2(int key, int x, int y) {
    if (gameState_L2 != GAME_PLAYING) return;

    switch (key) {
    case GLUT_KEY_UP: // Up arrow - PRESS
        keyStates['w'] = true;
        break;
    case GLUT_KEY_DOWN: // Down arrow - PRESS
        keyStates['s'] = true;
        break;
    case GLUT_KEY_LEFT: // Left arrow - PRESS
        keyStates['a'] = true;
        break;
    case GLUT_KEY_RIGHT: // Right arrow - PRESS
        keyStates['d'] = true;
        break;
    }
}

void specialUpLevel2(int key, int x, int y) {
    // Handle special key releases
    switch (key) {
    case GLUT_KEY_UP: // Up arrow - RELEASE
        keyStates['w'] = false;
        break;
    case GLUT_KEY_DOWN: // Down arrow - RELEASE
        keyStates['s'] = false;
        break;
    case GLUT_KEY_LEFT: // Left arrow - RELEASE
        keyStates['a'] = false;
        break;
    case GLUT_KEY_RIGHT: // Right arrow - RELEASE
        keyStates['d'] = false;
        break;
    }
}

void idleLevel2() {
    int currentMs = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (currentMs - prevTimeMs_L2) / 1000.0f;
    prevTimeMs_L2 = currentMs;

    if (gameState_L2 != GAME_PLAYING) {
        // Update rescue animation if won
        if (gameState_L2 == GAME_WON) {
            rescueAnimationTime += deltaTime;
        }
        // Still update sunset animation
        updateSunset(deltaTime);
        glutPostRedisplay();
        return;
    }

    // ========== CONTINUOUS MOVEMENT HANDLING ==========

    // Horizontal movement (A/D or Left/Right arrows)
    float horizontalMove = 0.0f;
    if (keyStates['a']) {
        horizontalMove -= moveStep_L2 * deltaTime * 20.0f;
    }
    if (keyStates['d']) {
        horizontalMove += moveStep_L2 * deltaTime * 20.0f;
    }

    // Apply horizontal movement
    if (horizontalMove != 0.0f) {
        playerX_L2 += horizontalMove;
    }

    // Vertical movement (W/S) - Key controlled only, no gravity
    if (keyStates['w']) {
        verticalSpeed = liftForce; // W for UP
    }
    else if (keyStates['s']) {
        verticalSpeed = -liftForce; // S for DOWN
    }
    else {
        verticalSpeed = 0.0f; // No movement when no keys pressed (no gravity)
    }

    // Speed control (Up/Down arrows) - SLOWER
    if (keyStates[GLUT_KEY_UP]) { // Up arrow for speed up
        playerSpeed_L2 += 3.0f * deltaTime; // Smooth acceleration
        if (playerSpeed_L2 > 30.0f) playerSpeed_L2 = 30.0f;
    }
    if (keyStates[GLUT_KEY_DOWN]) { // Down arrow for slow down
        playerSpeed_L2 -= 3.0f * deltaTime; // Smooth deceleration
        if (playerSpeed_L2 < 8.0f) playerSpeed_L2 = 8.0f;
    }

    // ========== REST OF THE GAME LOGIC ==========

    // Update timer
    int elapsed = currentMs - level2StartTimeMs;
    remainingTime_L2 = level2DurationMs - elapsed;
    if (remainingTime_L2 <= 0) {
        remainingTime_L2 = 0;
        gameState_L2 = GAME_LOST;
        glutPostRedisplay();
        return;
    }

    // Update sunset animation
    updateSunset(deltaTime);

    // Auto-forward movement (always fly forward)
    playerZ_L2 -= playerSpeed_L2 * deltaTime;

    // Apply vertical movement
    playerY_L2 += verticalSpeed * deltaTime;

    // Clamp player position
    if (playerX_L2 < riverMinX) {
        playerX_L2 = riverMinX;
    }
    else if (playerX_L2 > riverMaxX) {
        playerX_L2 = riverMaxX;
    }

    if (playerY_L2 < minHeight) {
        playerY_L2 = minHeight;
        verticalSpeed = 0; // Stop falling when hit ground
    }
    else if (playerY_L2 > maxHeight) {
        playerY_L2 = maxHeight;
        verticalSpeed = 0; // Stop rising when hit ceiling
    }

    // Check if player reached end without rescuing
    if (playerZ_L2 < riverEndZ && !drBeramRescued) {
        gameState_L2 = GAME_LOST; // Failed to rescue in time
        glutPostRedisplay();
        return;
    }

    // Collision detection
    AABB_L2 playerBox = getPlayerAABB_L2();

    // Check collectibles
    for (int i = 0; i < numFlyingCollectibles; i++) {
        if (flyingCollectibles[i].active && checkAABBCollision3D(playerBox, flyingCollectibles[i])) {
            flyingCollectibles[i].active = false;
            score_L2 += 20; // More points for flying collectibles
            addCollectibleAnimation(i, flyingCollectibles[i].x, flyingCollectibles[i].y, flyingCollectibles[i].z);
        }
    }

    // Check birds
    for (int i = 0; i < numBirds; i++) {
        if (birds[i].active && checkAABBCollision3D(playerBox, birds[i])) {
            handleBirdCollision(i);
            break;
        }
    }

    // Check Dr. Beram
    if (drBeram.active && checkAABBCollision3D(playerBox, drBeram)) {
        drBeram.active = false;
        drBeramRescued = true;
        gameState_L2 = GAME_WON;
        score_L2 += 500; // Big bonus for rescue
    }

    // Move birds (simple animation) - birds fly toward player
    for (int i = 0; i < numBirds; i++) {
        if (birds[i].active) {
            // Birds move in a sine wave pattern
            birds[i].x += sin(currentMs * 0.001f + i) * 0.5f;
            birds[i].y += sin(currentMs * 0.002f + i) * 0.3f;
            birds[i].z += 15.0f * deltaTime; // Birds move toward player

            // If bird passes player, reset it behind
            if (birds[i].z > playerZ_L2 + 100.0f) {
                birds[i].z = playerZ_L2 - 300.0f;
                birds[i].x = randRange(riverMinX + 10, riverMaxX - 10);
                birds[i].y = randRange(20, 35);
            }
        }
    }

    glutPostRedisplay();
}

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

    // HUD elements
    glDisable(GL_LIGHTING);
    drawScoreLevel2();
    drawTimerLevel2();
    drawHeightIndicator();
    drawSpeedIndicator();
    drawCameraMode();
    drawControlsInfo();
    drawGameStatusLevel2();
    glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

void reshapeLevel2(int w, int h) {
    if (h == 0) h = 1;
    float aspect = (float)w / (float)h;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 3000.0); // Larger far plane for flying

    glMatrixMode(GL_MODELVIEW);
}

// Safe model loading that won't crash if files don't exist
void loadModelsLevel2() {
    // COMMENTED OUT - Using glut placeholders instead
    // The Model_3DS::Load() function is causing file access assertions

    // Note: Person B should provide actual 3DS files later
    // For now, we'll use glutSolidSphere/Cube placeholders

    modelsLoaded = false; // Flag to indicate we're using placeholders

    // Load sky texture - ADDED
    char skyTexturePath[256];
    strcpy_s(skyTexturePath, sizeof(skyTexturePath), "textures/blu-sky-3.bmp");
    skyTexture.Load(skyTexturePath);

    // Set texture wrapping to repeat so it tiles across the sky
    if (skyTexture.texture[0] != 0) {
        glBindTexture(GL_TEXTURE_2D, skyTexture.texture[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}

void initGLLevel2() {
    srand((unsigned int)time(NULL));
    glClearColor(0.1f, 0.2f, 0.4f, 1.0f); // Sky blue
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    setupSunLight();
    setupLevel2();

    // Call the function to load sky texture
    loadModelsLevel2();  // UNCOMMENTED - Now it only loads texture, not 3DS models

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
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(1024, 768);

#ifdef RUN_LEVEL_2
    glutCreateWindow("El Ragol El 3ennab - Level 2 (Flying over Nile)");
    initGLLevel2();
    glutDisplayFunc(displayLevel2);
    glutIdleFunc(idleLevel2);
    glutKeyboardFunc(keyboardLevel2);
    glutKeyboardUpFunc(keyboardUpLevel2);        // NEW: For smooth key release handling
    glutSpecialFunc(specialKeysLevel2);
    glutSpecialUpFunc(specialUpLevel2);          // NEW: For smooth arrow key release handling
    glutReshapeFunc(reshapeLevel2);
#else
    glutCreateWindow("El Ragol El 3ennab - Level 1");
    initGL();
    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);
#endif

    glutMainLoop();
    return 0;
}