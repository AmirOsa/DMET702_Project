// ===============================
// CHOOSE WHICH LEVEL TO RUN
// ===============================
//#define RUN_LEVEL_2  // Comment this to run Level 1, uncomment to run Level 2
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
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// ===============================
// SHARED UTILITIES (used by both levels)
// ===============================
// Shared sky texture for both levels
GLTexture skyTexture;

// Shared sunset parameters.
// Level 2 animates these. Level 1 just uses the default "start" values.
float sunColor[3] = { 1.0f, 0.9f, 0.0f }; // yellowish sun
float sunsetProgress = 0.0f;              // 0..1, 0 = no sunset


void playCollectibleSound() {
    PlaySound(TEXT("sounds/collect.wav"), NULL, SND_FILENAME | SND_ASYNC);
}

void playloseSound() {
    PlaySound(TEXT("sounds/obstacle.wav"), NULL, SND_FILENAME | SND_ASYNC);
}

void playhitSound() {
    PlaySound(TEXT("sounds/lose.wav"), NULL, SND_FILENAME | SND_ASYNC);
}

void playwinSound() {
    PlaySound(TEXT("sounds/win.wav"), NULL, SND_FILENAME | SND_ASYNC);
}

// Load the sky texture once and configure it
void loadSkyTexture() {
    char skyTexturePath[256];
    strcpy_s(skyTexturePath, sizeof(skyTexturePath), "textures/blu-sky-3.bmp");
    skyTexture.Load(skyTexturePath);

    if (skyTexture.texture[0] != 0) {
        glBindTexture(GL_TEXTURE_2D, skyTexture.texture[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}

// Draws the textured sky "billboard" in front of the camera.
// Level 2 will animate sunsetProgress. Level 1 will just see the initial state.
// Draws the textured sky "skybox" that surrounds the entire scene
void drawSky() {
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    if (skyTexture.texture[0] != 0) {
        skyTexture.Use(); // bind texture
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        // Apply sunset tint to the entire sky
        glColor4f(
            1.0f,
            1.0f - sunsetProgress * 0.5f,  // reduce green as sun sets
            1.0f - sunsetProgress * 0.7f,  // reduce blue as sun sets
            1.0f
        );

        // Size of the skybox
        float skySize = 2000.0f;

        // Draw skybox as a cube surrounding everything
        // FRONT face (north)
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-skySize, -skySize, -skySize);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(skySize, -skySize, -skySize);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(skySize, skySize, -skySize);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-skySize, skySize, -skySize);
        glEnd();

        // BACK face (south)
        glBegin(GL_QUADS);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-skySize, -skySize, skySize);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(skySize, -skySize, skySize);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(skySize, skySize, skySize);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-skySize, skySize, skySize);
        glEnd();

        // LEFT face (west)
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-skySize, -skySize, skySize);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-skySize, -skySize, -skySize);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-skySize, skySize, -skySize);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-skySize, skySize, skySize);
        glEnd();

        // RIGHT face (east)
        glBegin(GL_QUADS);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(skySize, -skySize, skySize);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(skySize, -skySize, -skySize);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(skySize, skySize, -skySize);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(skySize, skySize, skySize);
        glEnd();

        // TOP face (sky)
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-skySize, skySize, skySize);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-skySize, skySize, -skySize);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(skySize, skySize, -skySize);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(skySize, skySize, skySize);
        glEnd();

        // BOTTOM face (ground - optional, usually not visible)
        glBegin(GL_QUADS);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-skySize, -skySize, skySize);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(skySize, -skySize, skySize);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(skySize, -skySize, -skySize);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-skySize, -skySize, -skySize);
        glEnd();

    }
    else {
        // Fallback: simple colored skybox if texture is missing
        glDisable(GL_TEXTURE_2D);

        // Draw all 6 faces with gradient colors
        float skySize = 2000.0f;

        // Front (north) - blue
        glBegin(GL_QUADS);
        glColor3f(0.1f, 0.2f, 0.8f - sunsetProgress * 0.7f);
        glVertex3f(-skySize, -skySize, -skySize);
        glVertex3f(skySize, -skySize, -skySize);
        glColor3f(0.6f, 0.7f, 1.0f - sunsetProgress * 0.7f);
        glVertex3f(skySize, skySize, -skySize);
        glVertex3f(-skySize, skySize, -skySize);
        glEnd();

        // Back (south) - darker blue
        glBegin(GL_QUADS);
        glColor3f(0.1f, 0.2f, 0.6f - sunsetProgress * 0.5f);
        glVertex3f(-skySize, -skySize, skySize);
        glVertex3f(skySize, -skySize, skySize);
        glColor3f(0.5f, 0.6f, 0.9f - sunsetProgress * 0.5f);
        glVertex3f(skySize, skySize, skySize);
        glVertex3f(-skySize, skySize, skySize);
        glEnd();

        // Left (west) - purple-blue gradient
        glBegin(GL_QUADS);
        glColor3f(0.2f, 0.1f, 0.7f - sunsetProgress * 0.6f);
        glVertex3f(-skySize, -skySize, skySize);
        glVertex3f(-skySize, -skySize, -skySize);
        glColor3f(0.6f, 0.5f, 1.0f - sunsetProgress * 0.6f);
        glVertex3f(-skySize, skySize, -skySize);
        glVertex3f(-skySize, skySize, skySize);
        glEnd();

        // Right (east) - purple-blue gradient
        glBegin(GL_QUADS);
        glColor3f(0.2f, 0.1f, 0.7f - sunsetProgress * 0.6f);
        glVertex3f(skySize, -skySize, skySize);
        glVertex3f(skySize, -skySize, -skySize);
        glColor3f(0.6f, 0.5f, 1.0f - sunsetProgress * 0.6f);
        glVertex3f(skySize, skySize, -skySize);
        glVertex3f(skySize, skySize, skySize);
        glEnd();

        // Top (sky) - light blue to orange at sunset
        glBegin(GL_QUADS);
        glColor3f(0.6f + sunsetProgress * 0.4f, 0.7f + sunsetProgress * 0.2f, 1.0f);
        glVertex3f(-skySize, skySize, skySize);
        glVertex3f(-skySize, skySize, -skySize);
        glVertex3f(skySize, skySize, -skySize);
        glVertex3f(skySize, skySize, skySize);
        glEnd();
    }

    // Draw the sun as a glowing sphere in the skybox
    if (sunsetProgress < 0.8f) {
        glDisable(GL_TEXTURE_2D);

        float sunSize = 40.0f - sunsetProgress * 20.0f;
        float sunY = 500.0f - sunsetProgress * 300.0f;
        float sunX = 300.0f - sunsetProgress * 500.0f;
        float sunZ = -800.0f; // Position sun in the distance

        glPushMatrix();
        glTranslatef(sunX, sunY, sunZ);

        // main sun disc
        glColor3f(sunColor[0], sunColor[1], sunColor[2]);
        glutSolidSphere(sunSize, 32, 32);

        // glow
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

enum CameraMode { FIRST_PERSON, THIRD_PERSON };
enum GameState { GAME_PLAYING, GAME_WON, GAME_LOST };

// ADD THIS:
enum ScreenMode { SCREEN_WELCOME, SCREEN_LEVEL1, SCREEN_LEVEL2 };

ScreenMode currentScreen = SCREEN_WELCOME;

// Level 2 init state so we only load its models once
bool level2Initialized = false;

// Forward declarations so Level 1 code can start Level 2
void initGLLevel2();
void setupLevel2();

// Forward declarations of Level 2 global state used by Level 1
extern int prevTimeMs_L2;
extern const int level2DurationMs;
extern int level2StartTimeMs;
extern int remainingTime_L2;
extern GameState gameState_L2;



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

// Full-screen fade overlay (alpha 0..1) used when moving to Level 2
void drawFullScreenFade(float alpha) {
    if (alpha <= 0.0f) return;
    if (alpha > 1.0f)  alpha = 1.0f;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    GLboolean depthEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
    glDisable(GL_DEPTH_TEST);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.0f, 0.0f, 0.0f, alpha);

    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    if (depthEnabled) glEnable(GL_DEPTH_TEST);

    glPopMatrix();                 // modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();                 // projection
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
//#ifndef RUN_LEVEL_2

// ===============================
// Global game state - Level 1
// ===============================

// Player state
float playerX = 0.0f;      // side movement along the street (left/right)
float playerZ = 0.0f;      // forward movement (runner direction, negative Z)
float playerY = 0.0f;      // height
// Player facing direction (Y-rotation in degrees)
float playerFacingAngle_L1 = 180.0f;   // 180 = facing forward along -Z

bool showCollisionBoxes = false;   // << turn to true when debugging

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

// Collider size (hitbox) for each collectible
const float COLLECTIBLE_COLLIDER_HALF_W = 0.5f;
const float COLLECTIBLE_COLLIDER_HALF_D = 0.5f;

// The 3DS model pivot is not exactly at the logical center → offset it
// Start with these; you can tweak them by 0.1 / 0.2 until the bottle sits in the cube
const float COLLECTIBLE_MODEL_OFFSET_X = -1.5f;   // +right, -left
const float COLLECTIBLE_MODEL_OFFSET_Z = 0.5f;  // +forward (toward -Z), -back
const float COLLECTIBLE_MODEL_OFFSET_Y = 0.0f;   // use if it floats above/below ground

// Player collider size (XZ only)
const float PLAYER_COLLIDER_HALF_W = 1.5f;  // left–right radius
const float PLAYER_COLLIDER_HALF_D = 2.0f;  // front–back radius


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

// -------- Mouse camera (shared idea, Level 1 uses these) --------
float cameraYaw_L1 = 0.0f;   // degrees, left/right orbit with mouse
float cameraPitch_L1 = 0.0f;   // degrees, up/down orbit with mouse

// Mouse state for dragging
bool mouseDragging_L1 = false;
int  lastMouseX_L1 = 0;
int  lastMouseY_L1 = 0;

// Common constants for both levels
const float MOUSE_SENSITIVITY = 0.25f;   // higher = faster rotation
const float MAX_CAMERA_PITCH = 45.0f;   // clamp up/down

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
// 3ennabeyat animation state
float collectibleRotateAngle = 0.0f;                  // spinning
float collectibleScale[MAX_COLLECTIBLES];             // per-collectible scale
bool  collectibleShrinking[MAX_COLLECTIBLES];         // true when scaling down

// Player spin animation when collecting
bool  playerSpinning = false;
float playerSpinAngle = 0.0f;   // current spin angle in degrees
float playerSpinTime = 0.0f;    // how long we've been spinning (seconds)




// Checkpoint (No2'et El Tafteesh)
AABB checkpoint;

// Make the checkpoint collision cover the whole street width
const float CHECKPOINT_COLLIDER_HALF_W = 20.0f;  // wide, covers -20..+20 (street is -10..+10)
const float CHECKPOINT_COLLIDER_HALF_D = 1.5f;   // thin slice along Z


// Lighting . street lamp (Level 1)
GLfloat lampBasePos[4] = { 0.0f, 5.0f, -30.0f, 1.0f }; // base position
float   lampRotateAngle = 0.0f;  // rotation for animation
float   lampIntensity = 2.0f;  // light intensity
float lampAnimTime = 0.0f;   // time accumulator for moving light animation



// Lighting . street lamp (Level 1)

// Time for delta time computationsFl
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

// ------- Level 1 → Level 2 transition effect -------
bool  transitionToLevel2 = false;     // true while we are fading out / showing message
float transitionTimer = 0.0f;      // how long transition has been running
const float transitionDuration = 2.0f;  // seconds before Level 2 actually starts

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


// Player hit translation (slide) animation when colliding with obstacles
bool  playerHitAnimating = false;
float playerHitTime = 0.0f;       // seconds since hit
float playerHitDistance = 0.0f;   // how far to slide

// NEW: where Z was when the hit started
float playerHitStartZ = 0.0f;


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

// Check if a candidate collectible box touches cars, trash or earlier collectibles
bool collidesWithAnyObstacleOrCollectible(const AABB& box, int uptoCollectIndex) {
    // Cars
    for (int i = 0; i < numCars; ++i) {
        if (!cars[i].active) continue;
        if (checkAABBCollision(box, cars[i])) return true;
    }

    // Trash cans (use world AABB, in case of offsets)
    for (int i = 0; i < numTrashCans; ++i) {
        if (!trashCans[i].active) continue;
        AABB trashBox = getTrashWorldAABB(trashCans[i]);
        if (checkAABBCollision(box, trashBox)) return true;
    }

    // Previously placed collectibles only, from index 0 to uptoCollectIndex-1
    for (int i = 0; i < uptoCollectIndex; ++i) {
        if (!collectibles[i].active) continue;
        if (checkAABBCollision(box, collectibles[i])) return true;
    }

    return false;
}

// Check if a box collides with already placed cars / trash cans
// We only look at the first numCarsToCheck cars and numTrashToCheck trash cans
bool collidesWithCarsAndTrashLimited(const AABB& box,
    int numCarsToCheck,
    int numTrashToCheck) {
    // cars
    for (int i = 0; i < numCarsToCheck; ++i) {
        if (!cars[i].active) continue;
        if (checkAABBCollision(box, cars[i])) return true;
    }

    // trash cans
    for (int i = 0; i < numTrashToCheck; ++i) {
        if (!trashCans[i].active) continue;
        AABB tBox = getTrashWorldAABB(trashCans[i]);
        if (checkAABBCollision(box, tBox)) return true;
    }

    return false;
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
    // Extra cars
    while (extraCars > 0 && numCars < MAX_CARS) {
        bool placed = false;
        int attempts = 0;

        while (!placed && attempts < 50) {
            AABB candidate;
            candidate.x = randRange(streetMinX + 2.0f, streetMaxX - 2.0f);
            candidate.z = randRange(streetEndZ + 30.0f, -10.0f);
            candidate.halfW = CAR_COLLIDER_HALF_W;
            candidate.halfD = CAR_COLLIDER_HALF_D;
            candidate.active = true;

            if (!collidesWithCarsAndTrashLimited(candidate, numCars, numTrashCans)) {
                cars[numCars] = candidate;
                cars[numCars].active = true;
                ++numCars;
                --extraCars;
                placed = true;
            }
            ++attempts;
        }

        if (!placed) break; // no safe spot found
    }

    // Extra trash cans
    while (extraTrash > 0 && numTrashCans < MAX_TRASHCANS) {
        bool placed = false;
        int attempts = 0;

        while (!placed && attempts < 40) {
            AABB candidate;
            candidate.x = randRange(streetMinX + 1.0f, streetMaxX - 1.0f);
            candidate.z = randRange(-50.0f, -10.0f);
            candidate.halfW = TRASH_COLLIDER_HALF_W;
            candidate.halfD = TRASH_COLLIDER_HALF_D;
            candidate.active = true;

            if (!collidesWithCarsAndTrashLimited(candidate, numCars, numTrashCans)) {
                trashCans[numTrashCans] = candidate;
                trashCans[numTrashCans].active = true;
                ++numTrashCans;
                --extraTrash;
                placed = true;
            }
            ++attempts;
        }

        if (!placed) break;
    }
}


// This sets up Level 1 layout . Person A can tweak positions later
void setupLevel1() {
    // Reset player and score
    playerX = 0.0f;
    playerZ = 0.0f;
    playerY = 0.0f;
    score = 0;
    moveStep = 1.0f;
    difficultyLevel = 0;
    checkpoint.active = false;
    checkpointSpawned20s = false;
    checkpointSpawned3s = false;
    playerFacingAngle_L1 = 180.0f;
    cameraYaw_L1 = 0.0f;
    cameraPitch_L1 = 0.0f;



    // Reset animations
    playerSpinning = false;
    playerSpinAngle = 0.0f;
    playerSpinTime = 0.0f;
    playerHitAnimating = false;
    playerHitTime = 0.0f;
    playerHitDistance = 0.0f;

    // Transition state
    transitionToLevel2 = false;
    transitionTimer = 0.0f;

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

    // place cars so they never overlap each other
    for (int i = 0; i < numCars; ++i) {
        bool placed = false;
        int attempts = 0;

        while (!placed && attempts < 50) {
            AABB candidate;
            candidate.x = randRange(streetMinX + 2.0f, streetMaxX - 2.0f);
            candidate.z = randRange(carMinZ, carMaxZ);
            candidate.halfW = CAR_COLLIDER_HALF_W;
            candidate.halfD = CAR_COLLIDER_HALF_D;
            candidate.active = true;

            // only compare with cars that were already placed
            if (!collidesWithCarsAndTrashLimited(candidate, i, 0)) {
                cars[i] = candidate;
                placed = true;
            }
            ++attempts;
        }

        if (!placed) {
            cars[i].active = false; // fail safe
        }
    }


    // ----- Trash cans -----
    // Place trash cans in the street where player can collide with them
    // Buildings are at z positions: streetStartZ, streetStartZ-60, streetStartZ-120, etc.
    // Place trash cans slightly in front of each building (closer to camera = higher Z)
    // Position them at the edge of the street so player can reach them
    // ----- Trash cans -----
// ----- Trash cans -----
// Randomly scatter trash cans across the street (not only edges)
// and do not overlap with cars or other trash cans
    numTrashCans = 0;
    const float trashSpacing = 40.0f;

    for (float z = -30.0f; z > streetEndZ && numTrashCans < MAX_TRASHCANS; z -= trashSpacing) {
        int cansThisRow = 1 + (rand() % 2); // 1 or 2 cans per row

        for (int c = 0; c < cansThisRow && numTrashCans < MAX_TRASHCANS; ++c) {
            bool placed = false;
            int attempts = 0;

            while (!placed && attempts < 40) {
                AABB candidate;
                candidate.x = randRange(streetMinX + 1.0f, streetMaxX - 1.0f);
                candidate.z = z + randRange(-5.0f, 5.0f);
                candidate.halfW = TRASH_COLLIDER_HALF_W;
                candidate.halfD = TRASH_COLLIDER_HALF_D;
                candidate.active = true;

                // compare with all cars and all previously placed trash cans
                if (!collidesWithCarsAndTrashLimited(candidate, numCars, numTrashCans)) {
                    trashCans[numTrashCans] = candidate;
                    trashCans[numTrashCans].active = true;
                    ++numTrashCans;
                    placed = true;
                }

                ++attempts;
            }
            // if not placed after attempts, we just skip this can
        }
    }


    // Add debug function to see where collision boxes are vs where models are

    // ----- Collectibles (3ennabeyat) -----
    numCollectibles = 50;  // up to MAX_COLLECTIBLES

    float colMinZ = streetEndZ + 15.0f;
    float colMaxZ = -5.0f;

    for (int i = 0; i < numCollectibles; ++i) {
        collectibles[i].active = false;   // default, until we find a free spot

        const float halfW = COLLECTIBLE_COLLIDER_HALF_W;
        const float halfD = COLLECTIBLE_COLLIDER_HALF_D;

        bool placed = false;
        int attempts = 0;

        while (!placed && attempts < 50) {   // try several random spots
            AABB candidate;
            candidate.x = randRange(streetMinX + 1.0f, streetMaxX - 1.0f);
            candidate.z = randRange(colMinZ, colMaxZ);
            candidate.halfW = halfW;
            candidate.halfD = halfD;
            candidate.active = true;

            if (!collidesWithAnyObstacleOrCollectible(candidate, i)) {
                collectibles[i] = candidate;
                placed = true;
            }
            attempts++;
        }

        // If we fail to place after many tries, this collectible stays inactive
    }
    // Init collectible animation state
    for (int i = 0; i < numCollectibles; ++i) {
        if (collectibles[i].active) {
            collectibleScale[i] = 1.0f;      // normal size
        }
        else {
            collectibleScale[i] = 0.0f;      // invisible if not placed
        }
        collectibleShrinking[i] = false;
    }



    // ----- Checkpoint at far end of street -----
    checkpoint.x = 0.0f;
    checkpoint.z = streetEndZ + 10.0f; // e.g. -190
    checkpoint.halfW = CHECKPOINT_COLLIDER_HALF_W;
    checkpoint.halfD = CHECKPOINT_COLLIDER_HALF_D;
    checkpoint.active = false;              // appears only in last 20 seconds

}

// Camera setup for first person and third person
void setupCamera() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Base direction: player faces along negative Z
    float baseDirX = 0.0f;
    float baseDirZ = -1.0f;

    if (cameraMode == FIRST_PERSON) {
        // In first person, rotate the view when the player is spinning
        float yawDeg = 0.0f;
        if (playerSpinning) {
            yawDeg = playerSpinAngle;      // same angle used to rotate the model
        }

        const float DEG2RAD = 3.14159265f / 180.0f;
        float yawRad = yawDeg * DEG2RAD;

        // Start from facing -Z and rotate around Y
        float lookDirX = sinf(yawRad);
        float lookDirZ = -cosf(yawRad);

        gluLookAt(
            playerX, playerY + eyeHeight, playerZ,
            playerX + lookDirX, playerY + eyeHeight, playerZ + lookDirZ,
            0, 1, 0
        );
    }
    else { // THIRD_PERSON with mouse orbit
        // Camera orbits around the player at fixed distance
        const float DEG2RAD = 3.14159265f / 180.0f;

        float dist = thirdPersonDist;
        float height = thirdPersonHeight;

        // Spherical distance and base pitch
        float R = sqrtf(dist * dist + height * height);
        float basePitch = atanf(height / dist);

        float yawRad = cameraYaw_L1 * DEG2RAD;
        float pitchRad = basePitch + cameraPitch_L1 * DEG2RAD;

        // Offset from player to camera
        float camOffsetX = R * sinf(yawRad) * cosf(pitchRad);
        float camOffsetY = R * sinf(pitchRad);
        float camOffsetZ = R * cosf(yawRad) * cosf(pitchRad);

        float camX = playerX + camOffsetX;
        float camY = playerY + camOffsetY;
        float camZ = playerZ + camOffsetZ;

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

void drawCheckpointCollisionBox() {
    if (!checkpoint.active) return;

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.0f, 0.0f); // red

    glPushMatrix();
    glTranslatef(checkpoint.x, 0.5f, checkpoint.z);
    glScalef(checkpoint.halfW * 2.0f, 1.0f, checkpoint.halfD * 2.0f);
    glutWireCube(1.0);
    glPopMatrix();

    glEnable(GL_LIGHTING);
}


void debugDrawCollectibleAtOrigin() {
    glPushMatrix();

    // Draw collider cube at origin
    glDisable(GL_LIGHTING);
    glColor3f(1, 1, 0);       // yellow
    glutWireCube(1.0f);       // collider: center at (0,0,0)
    glEnable(GL_LIGHTING);

    // Draw model using the offset you chose
    glTranslatef(
        COLLECTIBLE_MODEL_OFFSET_X,
        COLLECTIBLE_MODEL_OFFSET_Y,
        COLLECTIBLE_MODEL_OFFSET_Z
    );
    float baseScale = 0.5f;
    glScalef(baseScale, baseScale, baseScale);
    collectibleModel.Draw();

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
        drawText2D(-0.35f, -0.1f, "Press R to restart");
    }
    else if (gameState == GAME_LOST) {
        drawText2D(-0.35f, 0.0f, "TIME UP! Game Over");
        drawText2D(-0.35f, -0.1f, "Press R to restart");
    }
}


// ===============================
// Drawing the world (Person A layout)
// ===============================



// >>> ADD THIS BLOCK <<<
const float LAMP_X = 16.0f;   // distance from street center
const float LAMP_SPACING_Z = 60.0f;   // distance between lamp pairs on Z
const float LAMP_OFFSET_Z = 30.0f;   // lamps sit between building rows
const float LAMP_HEIGHT = 6.0f;    // approximate lamp head height
// <<< END OF NEW BLOCK >>>


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
        // Y = 0.0f means "place origin at ground level"
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

        // Skip if invisible and not shrinking
        if (!c.active && !collectibleShrinking[i])
            continue;

        // Skip if fully shrunk
        if (collectibleScale[i] <= 0.0f)
            continue;

        glPushMatrix();

        // Position: collider center + model pivot offset
        glTranslatef(
            c.x + COLLECTIBLE_MODEL_OFFSET_X,
            0.5f + COLLECTIBLE_MODEL_OFFSET_Y,
            c.z + COLLECTIBLE_MODEL_OFFSET_Z
        );

        // ❌ REMOVE ANY ROTATION — NO ROTATION AT ALL

        // Scaling (base size * animation scale)
        float baseScale = 0.5f;      // adjust until size looks good
        float s = baseScale * collectibleScale[i];
        glScalef(s, s, s);

        // ========== DRAW WITH TEXTURES ==========
        // Enable texturing and lighting
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);

        // Set material properties
        GLfloat matAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat matDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat matSpecular[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        GLfloat matShininess[] = { 20.0f };

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);

        // IMPORTANT: Set color to white so textures show correctly
        glColor3f(1.0f, 1.0f, 1.0f);

        // Draw the collectible model with textures
        collectibleModel.Draw();

        // ------- Red glow around collectible -------
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // radius scaled with the object size
        float glowRadius = 1.2f;      // base radius in model space
        glColor4f(1.0f, 0.2f, 0.2f, 0.6f);
        glutSolidSphere(glowRadius, 12, 12);

        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        // -------------------------------------------

        glPopMatrix();
    }
}





void drawCheckpoint() {
    if (!checkpoint.active) return;

    glPushMatrix();

    // Place at checkpoint position
    glTranslatef(checkpoint.x, 5.4f, checkpoint.z);

    // Rotate to face the player if needed
    glRotatef(180.0f, 0, 1, 0);   // you can try removing/changing this if it's backwards

    // Scale the gate – tweak these numbers until it looks good
    glScalef(0.18f, 0.1f, 0.1f);   // try 0.05 / 0.2 etc if it's too big/small

    checkpointModel.Draw();

    glPopMatrix();
}


void drawLamps() {
    const float lampX = LAMP_X;
    const float scale = 0.95f;
    const float lampY = -0.5f;   // lift lamp a bit above the ground

    for (float z = streetStartZ; z > streetEndZ; z -= LAMP_SPACING_Z) {
        float midZ = z - LAMP_OFFSET_Z;


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

// Simple glowing circles on the ground under each lamp
void drawLampLightPools() {
    const float lampX = LAMP_X;
    const float spacing = LAMP_SPACING_Z;
    const float offsetZ = LAMP_OFFSET_Z;

    const float radius = 4.0f;
    const int   segments = 20;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 🔹 same horizontal translation as the light positions
    float slideX = sinf(lampAnimTime * 1.5f) * 1.5f;

    for (float z = streetStartZ; z > streetEndZ; z -= spacing) {
        float midZ = z - offsetZ;

        // Two sides, left and right
        for (int side = -1; side <= 1; side += 2) {
            glPushMatrix();

            // ✅ Lamps fixed at ±lampX, but glow moves: ±lampX + slideX
            glTranslatef(side * lampX + slideX, 0.01f, midZ);

            glBegin(GL_TRIANGLE_FAN);
            // center, bright
            glColor4f(1.0f, 0.95f, 0.7f, 0.6f);
            glVertex3f(0.0f, 0.0f, 0.0f);

            // edge, transparent
            glColor4f(1.0f, 0.95f, 0.7f, 0.0f);
            for (int i = 0; i <= segments; ++i) {
                float angle = (2.0f * 3.14159f * i) / segments;
                float px = cosf(angle) * radius;
                float pz = sinf(angle) * radius;
                glVertex3f(px, 0.0f, pz);
            }
            glEnd();

            glPopMatrix();
        }
    }

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}



void drawPlayer() {
    glPushMatrix();

    // Base position (x, 0, z) - already includes knockback
    glTranslatef(playerX, 0.0f, playerZ);

    // Face current movement direction
    glRotatef(playerFacingAngle_L1, 0, 1, 0);

    // Spin when collecting (adds on top of facing angle)
    if (playerSpinning) {
        glRotatef(playerSpinAngle, 0, 1, 0);
    }


    glScalef(1.5f, 1.5f, 1.5f);
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
    // accumulate time for animation
    lampAnimTime += deltaTime;

    const float lampX = LAMP_X;
    const float lampHeight = LAMP_HEIGHT;
    const float spacing = LAMP_SPACING_Z;
    const float offsetZ = LAMP_OFFSET_Z;

    // How far from the player a lamp can be and still get a real light
    const float lightRangeZ = 180.0f;  // lamps within +/- 180 on Z get lit

    // 🔹 X translation of the LIGHT only (not the models)
    float slideX = sinf(lampAnimTime * 1.5f) * 1.5f;   // speed & range

    int lightIndex = 0; // 0..7 → GL_LIGHT0..GL_LIGHT7

    for (float z = streetStartZ; z > streetEndZ; z -= spacing) {
        float midZ = z - offsetZ; // same Z as drawLamps()

        // Only attach lights to lamps near the player
        if (fabsf(midZ - playerZ) <= lightRangeZ && lightIndex < 8) {
            // LEFT lamp light (sliding horizontally)
            GLfloat posL[] = { -lampX + slideX, lampHeight, midZ, 1.0f };
            glLightfv(GL_LIGHT0 + lightIndex, GL_POSITION, posL);
            lightIndex++;
            if (lightIndex >= 8) break;

            // RIGHT lamp light (sliding horizontally)
            GLfloat posR[] = { lampX + slideX, lampHeight, midZ, 1.0f };
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

// ✅ NEW: collectible collision boxes
void drawCollectibleCollisionBoxes() {
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor3f(1.0f, 1.0f, 0.0f); // yellow

    for (int i = 0; i < numCollectibles; ++i) {
        const AABB& c = collectibles[i];
        if (!c.active) continue; // only draw active ones

        glPushMatrix();
        // same Y as drawCollectibles, center at 0.5
        glTranslatef(c.x, 0.5f, c.z);
        glScalef(c.halfW * 2.0f, 1.0f, c.halfD * 2.0f);
        glutWireCube(1.0);
        glPopMatrix();
    }

    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}
void updateCollectibleAnimations(float deltaTime) {
    const float shrinkSpeed = 3.0f; // how fast scale goes to 0 per second

    for (int i = 0; i < numCollectibles; ++i) {
        if (!collectibleShrinking[i]) continue;

        collectibleScale[i] -= shrinkSpeed * deltaTime;

        if (collectibleScale[i] <= 0.0f) {
            collectibleScale[i] = 0.0f;
            collectibleShrinking[i] = false;   // done, stop drawing later
        }
    }


}
void updatePlayerHit(float deltaTime) {
    if (!playerHitAnimating) return;

    const float HIT_ANIM_DURATION = 0.25f;   // quick slide
    const float MAX_BACK = 1.2f;            // how far to push back along +Z

    playerHitTime += deltaTime;

    // t goes 0 → 1
    float t = playerHitTime / HIT_ANIM_DURATION;
    if (t > 1.0f) t = 1.0f;

    // smooth easing: starts fast → slows down
    float slide = (1.0f - cosf(t * 3.14159f)) * 0.5f;

    // This used to go into playerHitDistance only
    // NOW we apply it to the real Z position
    playerHitDistance = slide * MAX_BACK;
    playerZ = playerHitStartZ + playerHitDistance;

    if (playerHitTime >= HIT_ANIM_DURATION) {
        playerHitAnimating = false;
        playerHitTime = 0.0f;
        playerHitDistance = 0.0f;
    }
}


void updatePlayerSpin(float deltaTime) {
    if (!playerSpinning) return;

    const float spinDuration = 0.5f;   // seconds
    const float spinSpeed = 720.0f; // degrees per second (2 full spins)

    playerSpinTime += deltaTime;
    playerSpinAngle += spinSpeed * deltaTime;

    // stop spinning after duration
    if (playerSpinTime >= spinDuration) {
        playerSpinning = false;
        playerSpinAngle = 0.0f;  // reset angle so player faces forward again
        playerSpinTime = 0.0f;
    }
}



// ===============================
// GLUT callbacks - Level 1
// ===============================

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setupCamera();
    drawSky();
    applyLampLight();
    updateLamp(0.0f);

    drawStreet();
    drawLampLightPools();
    drawBuildings();
    drawLamps();
    drawCars();
    drawTrashCans();
    //debugDrawTrashAtOrigin();
    //debugDrawCollectibleAtOrigin();
    // Only draw collision boxes when debugging
if (showCollisionBoxes) {
    drawTrashCollisionBoxes();
    drawCarCollisionBoxes();
    drawCollectibleCollisionBoxes();
    drawPlayerCollisionBox();
    drawCheckpointCollisionBox();
}

    drawCollectibles();
    drawCheckpoint();
    drawPlayer();


    drawScore();
    drawTimer();
    drawGameStatus();

    // If we touched the checkpoint, show transition message + fade
    if (transitionToLevel2) {
        float t = transitionTimer / transitionDuration;
        if (t > 1.0f) t = 1.0f;

        // Fade to black
        drawFullScreenFade(t);

        // Text on top of fade
        drawText2D(-0.55f, 0.05f, "Checkpoint reached!");
        drawText2D(-0.75f, -0.05f, "Congratulations, moving to Level 2...");
    }

    glutSwapBuffers();
}



void idle() {
    // 1. Time and delta time
    int   currentMs = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (currentMs - prevTimeMs) / 1000.0f;
    prevTimeMs = currentMs;

    // 2. Always animate lamps (even if game is over)
    updateLamp(deltaTime);

    // 2.5 Handle Level 1 → Level 2 transition fade
    if (transitionToLevel2) {
        transitionTimer += deltaTime;

        // Keep small animations alive so scene is not frozen
        updateCollectibleAnimations(deltaTime);
        updatePlayerSpin(deltaTime);
        updatePlayerHit(deltaTime);

        // When fade is done, actually start Level 2
        if (transitionTimer >= transitionDuration) {
            transitionToLevel2 = false;

            currentScreen = SCREEN_LEVEL2;

            if (!level2Initialized) {
                initGLLevel2();
                level2Initialized = true;
            }
            else {
                setupLevel2();
            }

            prevTimeMs_L2 = glutGet(GLUT_ELAPSED_TIME);
            level2StartTimeMs = prevTimeMs_L2;
            remainingTime_L2 = level2DurationMs;
            gameState_L2 = GAME_PLAYING;
        }

        glutPostRedisplay();
        return;
    }

    // 3. Game logic only while playing
    if (gameState == GAME_PLAYING) {

        // 3.1 Timer / time up
        int elapsedSinceStart = currentMs - gameStartTimeMs;
        remainingTimeMs = gameDurationMs - elapsedSinceStart;

        if (remainingTimeMs <= 0) {
            remainingTimeMs = 0;
            gameState = GAME_LOST;   // time up
            // ✅ play lose sound once
            playloseSound();
            glutPostRedisplay();
            return;
        }

        // 3.2 Checkpoint logic

        // First checkpoint . appears in last 20 seconds
        if (!checkpointSpawned20s && remainingTimeMs <= 20000) {
            checkpoint.x = 0.0f;
            checkpoint.z = playerZ - 60.0f;   // 60 units ahead of player
            checkpoint.halfW = CHECKPOINT_COLLIDER_HALF_W;
            checkpoint.halfD = CHECKPOINT_COLLIDER_HALF_D;
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
            checkpoint.halfW = CHECKPOINT_COLLIDER_HALF_W;
            checkpoint.halfD = CHECKPOINT_COLLIDER_HALF_D;
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
                // ✅ play hit sound (car)
                playhitSound();
                // Rebuild player box after we moved the player
                playerBox = getPlayerAABB();
                // Start slide animation
                playerHitAnimating = true;
                playerHitTime = 0.0f;
                playerHitDistance = 0.0f;
                playerHitStartZ = playerZ;
                break; // only handle one car per frame
            }
        }



        // Player ↔ trash cans
        for (int i = 0; i < numTrashCans; ++i) {
            if (!trashCans[i].active) continue;

            AABB trashBox = getTrashWorldAABB(trashCans[i]);  // <-- apply offset


            if (checkAABBCollision(playerBox, trashBox)) {
                handleObstacleCollision(trashBox);
                // ✅ play hit sound (car)
                playhitSound();
                playerBox = getPlayerAABB();

                // Start slide animation - remember Z at start of hit
                playerHitAnimating = true;
                playerHitTime = 0.0f;
                playerHitDistance = 0.0f;
                playerHitStartZ = playerZ;

                break;
            }

        }



        // Player ↔ collectibles
// Player ↔ collectibles
        for (int i = 0; i < numCollectibles; ++i) {
            if (!collectibles[i].active) continue;

            if (checkAABBCollision(playerBox, collectibles[i])) {
                // Stop colliding but start shrink animation
                collectibles[i].active = false;          // no more collisions
                collectibleShrinking[i] = true;          // start scaling down
                score += 10;
                // ✅ play collect sound
               playCollectibleSound();

                // 💫 trigger player spin animation
                playerSpinning = true;
                playerSpinTime = 0.0f;      // restart timer
                playerSpinAngle = 0.0f;     // start from facing forward
                // TODO: sound if you want
            }
        }



        // Player ↔ checkpoint
        if (checkpoint.active && checkAABBCollision(playerBox, checkpoint)) {
            checkpoint.active = false;
            gameState = GAME_WON;
            playwinSound();

            // Start transition effect. actual switch to Level 2 happens in idle()
            transitionToLevel2 = true;
            transitionTimer = 0.0f;

            glutPostRedisplay();
            return;
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
    // 3.6 Update collectible shrinking / rotation animation
    updateCollectibleAnimations(deltaTime);
    // Update player spin animation (after collecting)
    updatePlayerSpin(deltaTime);
    // Update player slide animation (after hitting obstacle)
    updatePlayerHit(deltaTime);



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




void keyboardLevel1(unsigned char key, int x, int y) {
    if (gameState != GAME_PLAYING && key != 27) {
        return;
    }

    float newX = playerX;
    float newZ = playerZ;

    bool moved = false;

    switch (key) {
    case 'a':
    case 'A':
        newX -= moveStep;
        playerFacingAngle_L1 = 270.0f;   // left (-X)
        moved = true;
        break;

    case 'd':
    case 'D':
        newX += moveStep;
        playerFacingAngle_L1 = 90.0f;    // right (+X)
        moved = true;
        break;

    case 'w':
    case 'W':
        newZ -= moveStep;                // forward along -Z
        playerFacingAngle_L1 = 180.0f;   // forward (-Z)
        moved = true;
        break;

    case 's':
    case 'S':
        newZ += moveStep;                // backward along +Z
        playerFacingAngle_L1 = 0.0f;     // back (+Z)
        moved = true;
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


    if (moved) {
        // Reset mouse camera when player moves with keyboard
        cameraYaw_L1 = 0.0f;
        cameraPitch_L1 = 0.0f;
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
    gluPerspective(60.0, aspect, 1.0, 3000.0);


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
    collectibleModel.Load("models/3enabeyat1.3ds");
    checkpointModel.Load("models/gate.3ds");

    // ===== Player texture =====
    if (playerModel.numMaterials > 0) {
        char playerTexPath[256];
        strcpy_s(playerTexPath, sizeof(playerTexPath),
            "textures/Ch24_1001_Diffuse.bmp");

        // Apply same diffuse texture to first material
        playerModel.Materials[0].tex.Load(playerTexPath);
        playerModel.Materials[0].textured = true;

        // Apply to all materials if needed
        for (int i = 1; i < playerModel.numMaterials; ++i) {
            playerModel.Materials[i].tex.Load(playerTexPath);
            playerModel.Materials[i].textured = true;
        }
    }

    // ===== Collectible model textures (Level 1) =====
    if (collectibleModel.numMaterials > 0) {
        printf("Level 1: Loading collectible textures...\n");

        // Load your collectible textures
        char texPath1[256];
        strcpy_s(texPath1, sizeof(texPath1), "textures/3ennabeyat1.bmp");

        char texPath2[256];
        strcpy_s(texPath2, sizeof(texPath2), "textures/3ennabeyat2.bmp");

        printf("Level 1 collectible has %d materials\n", collectibleModel.numMaterials);

        // Apply textures to materials
        for (int i = 0; i < collectibleModel.numMaterials; i++) {
            if (i == 0) {
                // First material gets first texture
                collectibleModel.Materials[i].tex.Load(texPath1);
                collectibleModel.Materials[i].textured = true;
                printf("Level 1: Applied texture 1 to material %d\n", i);
            }
            else if (i == 1 && collectibleModel.numMaterials > 1) {
                // Second material gets second texture if it exists
                collectibleModel.Materials[i].tex.Load(texPath2);
                collectibleModel.Materials[i].textured = true;
                printf("Level 1: Applied texture 2 to material %d\n", i);
            }
            else {
                // Any additional materials default to first texture
                collectibleModel.Materials[i].tex.Load(texPath1);
                collectibleModel.Materials[i].textured = true;
                printf("Level 1: Applied texture 1 to material %d (default)\n", i);
            }
        }
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

    // Load the shared sky texture used in both levels
    loadSkyTexture();
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

//#endif // RUN_LEVEL_2 - End of Level 1 code

// ===================================================================
// LEVEL 2 CODE STARTS HERE - Amir's Work (FIXED VERSION)
// ===================================================================
//#ifdef RUN_LEVEL_2

// ===============================
// Level 2 Specific Structures
// ===============================

// Level 2 AABB with Y coordinate for flying
struct AABB_L2 {
    float x, y, z;   // 3D position
    float halfW, halfD, halfH; // 3D dimensions
    bool active;

    // NEW: Bird patrol variables
    float patrolStartX;   // Left patrol point
    float patrolEndX;     // Right patrol point
    float patrolSpeed;    // Movement speed
    bool movingRight;     // Direction flag
    float patrolY;        // Fixed height for patrol

    // NEW: Vertical bobbing variables
    float verticalBobAmplitude; // How much up/down movement
    float verticalBobSpeed;     // How fast the bobbing is
    float bobPhase;             // Phase offset for each bird
};

// ===============================
// Level 2 Global Variables
// ===============================

// Player state for flying
float playerX_L2 = 0.0f;      // side movement
float playerY_L2 = 25.0f;     // flying height (Y-axis)
float playerZ_L2 = 0.0f;      // forward movement (flying direction, negative Z)
float playerSpeed_L2 = 15.0f; // Forward speed
float playerRollAngle = 0.0f; // Banking angle for visual effect
float playerPitchAngle = 0.0f; // Pitch angle for visual effect
int playerLives = 5;

// Flying boundaries
const float riverWidth = 50.0f;
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
// Camera free-look (mouse) for Level 2
float cameraYaw_L2 = 0.0f;
float cameraPitch_L2 = 0.0f;
bool  mouseDragging_L2 = false;
int   lastMouseX_L2 = 0, lastMouseY_L2 = 0;

// Player facing direction (Y rotation)
float playerFacingAngle_L2 = 180.0f;   // forward along -Z


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
bool sunsetActive = true;

// Animated lights
float lightOrbitAngle = 0.0f;
float lightOrbitRadius = 100.0f;
float lightOrbitHeight = 80.0f;

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

// Win scene after rescuing Dr Beram
bool winSceneActive = false;

// Celebration area position using Level 1 style buildings
// Put it on the concrete land, left of the river
const float WIN_SCENE_CENTER_X = -400.0f;
const float WIN_SCENE_CENTER_Z = -200.0f;
const float WIN_SCENE_PLAYER_Y = 5.0f;

// Level 1 building model reused in Level 2 end scene
Model_3DS buildingModel_L2;


// Debug flag for Level 2 collision boxes
bool debugDrawCollision_L2 = true;   // set false if you do not want boxes

// ===== Level 2 model ↔ collider offsets (tweak these) =====
// We treat playerX_L2, flyingCollectibles[i].x, drBeram.x as
// the center of the collision box. These offsets move the *model*
// so it sits inside the box.

float PLAYER_MODEL_OFFSET_X_L2 = 0.0f;
float PLAYER_MODEL_OFFSET_Y_L2 = -3.0f;   // adjust if player box is too low/high
float PLAYER_MODEL_OFFSET_Z_L2 = 0.0f;

// Collectible (3ennabeyat) offsets in Level 2.
// Model is bigger here, so start with a scaled version of Level 1 offsets.
float COLLECTIBLE_MODEL_OFFSET_X_L2 = -4.5f;  // ≈ -1.5 * 3
float COLLECTIBLE_MODEL_OFFSET_Y_L2 = -4.5f;
float COLLECTIBLE_MODEL_OFFSET_Z_L2 = 1.5f;   // ≈  0.5 * 3

// Dr Beram offsets. Usually pivot is at the feet.
// Move model down so collider center is around his chest.
float DR_BERAM_MODEL_OFFSET_X_L2 = 0.0f;
float DR_BERAM_MODEL_OFFSET_Y_L2 = -7.0f;  // tweak this
float DR_BERAM_MODEL_OFFSET_Z_L2 = 1.0f;


// Models - LOADED LIKE LEVEL 1
Model_3DS playerModel_L2;
Model_3DS collectibleModel_L2;  // DECLARE COLLECTIBLE MODEL
Model_3DS birdModel_L2;  // BIRD MODEL
Model_3DS drBeramModel;

// Ground textures for Level 2
GLTexture grassTexture;      // "Grass.bmp" - for area next to river
GLTexture concrMetTexture;   // "ConcrMet.bmp" - for outer area (concrete/metallic)
GLTexture riverTexture;      // "River.bmp" - for the river water

// Animation states
float collectibleRotation = 0.0f;
float birdFlapAnimation = 0.0f;
float heroWingFlap = 0.0f;
const float FORWARD_TILT_ANGLE = 40.0f; // Degrees to tilt forward (adjust as needed)

// Collectible collection animation
struct CollectibleAnim {
    int index;
    float x, y, z;
    float scale;
    float rotation;
    float alpha;
    bool active;
};
const int MAX_ANIM_COLLECTIBLES = 10;
CollectibleAnim collectibleAnimations[MAX_ANIM_COLLECTIBLES];

// Player animations
bool playerSpinning_L2 = false;
float playerSpinAngle_L2 = 0.0f;
float playerSpinTime_L2 = 0.0f;

// Player hit animation
bool playerHit_L2 = false;
float playerHitTime_L2 = 0.0f;
float playerHitDistance_L2 = 0.0f;
// NEW: Z position when the hit animation starts
float playerHitStartZ_L2 = 0.0f;


// ===============================
// Level 2 Key State Tracking
// ===============================
bool keyStates[256] = { false }; // Track all key states
bool specialKeyStates[256] = { false }; // For special keys


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

// ===============================
// Level 2 debug collision drawing
// ===============================

// Player collision box
void drawPlayerCollisionBox_L2() {
    AABB_L2 p = getPlayerAABB_L2();

    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor3f(0.0f, 1.0f, 0.0f);  // green

    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    glScalef(p.halfW * 2.0f, p.halfH * 2.0f, p.halfD * 2.0f);
    glutWireCube(1.0);
    glPopMatrix();

    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

// Flying collectibles collision boxes
void drawCollectibleCollisionBoxes_L2() {
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor3f(1.0f, 1.0f, 0.0f);  // yellow

    for (int i = 0; i < numFlyingCollectibles; ++i) {
        const AABB_L2& c = flyingCollectibles[i];
        if (!c.active) continue;

        glPushMatrix();
        glTranslatef(c.x, c.y, c.z);
        glScalef(c.halfW * 2.0f, c.halfH * 2.0f, c.halfD * 2.0f);
        glutWireCube(1.0);
        glPopMatrix();
    }

    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

// Bird collision boxes (obstacles)
void drawBirdCollisionBoxes_L2() {
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor3f(1.0f, 0.0f, 1.0f);  // magenta

    for (int i = 0; i < numBirds; ++i) {
        const AABB_L2& b = birds[i];
        if (!b.active) continue;

        glPushMatrix();
        glTranslatef(b.x, b.y, b.z);
        glScalef(b.halfW * 2.0f, b.halfH * 2.0f, b.halfD * 2.0f);
        glutWireCube(1.0);
        glPopMatrix();
    }

    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

// Dr Beram collision box
void drawDrBeramCollisionBox_L2() {
    if (!drBeram.active) return;   // Only draw while he is collidable

    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor3f(0.0f, 1.0f, 1.0f);  // cyan

    glPushMatrix();
    glTranslatef(drBeram.x, drBeram.y, drBeram.z);
    glScalef(drBeram.halfW * 2.0f, drBeram.halfH * 2.0f, drBeram.halfD * 2.0f);
    glutWireCube(1.0);
    glPopMatrix();

    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}


// Handle collision with bird: smooth bounce back, bird stays visible
void handleBirdCollision(int birdIndex) {
    // ✅ hit sound for bird collision
    playhitSound();
    // 1. Lose a life
    playerLives--;
    if (playerLives < 0) playerLives = 0;

    // 2. Lose some score
    score_L2 -= 15;
    if (score_L2 < 0) score_L2 = 0;

    // 3. Compute how much we overlap in Z and push the player back just enough
    AABB_L2 playerBox = getPlayerAABB_L2();
    const AABB_L2& birdBox = birds[birdIndex];

    float dz = playerBox.z - birdBox.z;
    float overlapZ = (playerBox.halfD + birdBox.halfD) - fabsf(dz);

    // Extra distance so we don't immediately collide again
    const float bounceExtra = 2.0f;   // tweak if you want stronger/weaker bounce

    if (overlapZ > 0.0f) {
        // Player always flies forward along -Z, so "bounce back" is +Z
        playerZ_L2 += overlapZ + bounceExtra;
    }

    // Small vertical nudge so it feels like a hit (not a big drop)
    playerY_L2 -= 1.5f;

    // 4. Start hit animation (smooth slide on top of this)
    playerHit_L2 = true;
    playerHitTime_L2 = 0.0f;
    playerHitDistance_L2 = 0.0f;
    // NEW: remember the Z position after the instant bounce
    playerHitStartZ_L2 = playerZ_L2;

    // 5. DO NOT deactivate the bird, it should stay there
    // birds[birdIndex].active = false;

    // 6. Clamp height
    if (playerY_L2 < minHeight) playerY_L2 = minHeight;
    if (playerY_L2 > maxHeight) playerY_L2 = maxHeight;

    // 7. Check for game over
    if (playerLives == 0) {
        gameState_L2 = GAME_LOST;
        // ✅ play lose sound once
    playloseSound();
    }
}


// Add collectible animation
void addCollectibleAnimation(int index, float x, float y, float z) {
    for (int i = 0; i < MAX_ANIM_COLLECTIBLES; i++) {
        if (!collectibleAnimations[i].active) {
            collectibleAnimations[i].active = true;
            collectibleAnimations[i].x = x;
            collectibleAnimations[i].y = y;
            collectibleAnimations[i].z = z;
            collectibleAnimations[i].scale = 1.0f;
            collectibleAnimations[i].rotation = 0.0f;
            collectibleAnimations[i].alpha = 1.0f;
            collectibleAnimations[i].index = index;
            break;
        }
    }

    // Start player spin animation
    playerSpinning_L2 = true;
    playerSpinTime_L2 = 0.0f;
    playerSpinAngle_L2 = 0.0f;
}

// Setup Level 2 - Flying over Nile
void setupLevel2() {
    winSceneActive = false;
    // Reset player
    playerX_L2 = 0.0f;
    playerY_L2 = 25.0f;
    playerZ_L2 = riverStartZ;
    playerSpeed_L2 = 20.0f;
    score_L2 = 0;
    playerLives = 5;
    playerFacingAngle_L2 = 180.0f;
    cameraYaw_L2 = 0.0f;
    cameraPitch_L2 = 0.0f;

    sunsetProgress = 0.05f;  // Start with small progress so sunset is visible immediately
    sunsetActive = true;
    drBeramRescued = false;
    rescueAnimationTime = 0.0f;
    lightOrbitAngle = 0.0f;
    playerRollAngle = 0.0f;
    playerPitchAngle = 0.0f;

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
        flyingCollectibles[i].halfW = 2.5f;
        flyingCollectibles[i].halfH = 2.5f;
        flyingCollectibles[i].halfD = 2.5f;
        flyingCollectibles[i].active = true;
    }

    // Setup birds (obstacles) - UPDATED FOR PATROL WITH DIFFERENT HEIGHTS
    numBirds = 12;
    for (int i = 0; i < numBirds; i++) {
        // RANDOM HEIGHTS between 15 and 45 units
        birds[i].y = randRange(15.0f, 45.0f);
        birds[i].x = randRange(riverMinX + 20, riverMaxX - 20);
        birds[i].z = riverStartZ - 80.0f - (i * 60.0f);
        birds[i].halfW = 5.0f;  // Increased for larger birds
        birds[i].halfH = 4.0f;  // Increased for larger birds
        birds[i].halfD = 6.0f;  // Increased for larger birds
        birds[i].active = true;

        // NEW: Setup patrol behavior with height variation
        float patrolLength = randRange(15.0f, 40.0f); // Random line length
        birds[i].patrolStartX = birds[i].x - patrolLength / 2;
        birds[i].patrolEndX = birds[i].x + patrolLength / 2;
        birds[i].patrolSpeed = randRange(5.0f, 15.0f); // Random speed
        birds[i].movingRight = (rand() % 2 == 0); // Random starting direction
        birds[i].patrolY = birds[i].y; // Store original height

        // NEW: Random vertical patrol amplitude (some birds bob more than others)
        birds[i].verticalBobAmplitude = randRange(0.5f, 3.0f);
        birds[i].verticalBobSpeed = randRange(0.5f, 2.0f);
        birds[i].bobPhase = randRange(0.0f, 360.0f); // Random starting phase
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
        // First person: rotate camera when the flying player is spinning
        float yawDeg = 0.0f;
        if (playerSpinning_L2) {
            yawDeg = playerSpinAngle_L2;
        }

        const float DEG2RAD = 3.14159265f / 180.0f;
        float yawRad = yawDeg * DEG2RAD;

        // Base orientation is along -Z, rotate around Y
        float lookDirX = sinf(yawRad);
        float lookDirZ = -cosf(yawRad);

        float lookDistance = 50.0f;   // how far ahead we look

        gluLookAt(
            playerX_L2, playerY_L2 + 2.0f, playerZ_L2,
            playerX_L2 + lookDirX * lookDistance,
            playerY_L2 + 2.0f,
            playerZ_L2 + lookDirZ * lookDistance,
            0, 1, 0
        );
    }
    else { // THIRD_PERSON with mouse orbit
        const float DEG2RAD = 3.14159265f / 180.0f;

        float dist = thirdPersonDist_L2;
        float height = thirdPersonHeight_L2;

        float R = sqrtf(dist * dist + height * height);
        float basePitch = atanf(height / dist);

        float yawRad = cameraYaw_L2 * DEG2RAD;
        float pitchRad = basePitch + cameraPitch_L2 * DEG2RAD;

        float camOffsetX = R * sinf(yawRad) * cosf(pitchRad);
        float camOffsetY = R * sinf(pitchRad);
        float camOffsetZ = R * cosf(yawRad) * cosf(pitchRad);

        float camX = playerX_L2 + camOffsetX;
        float camY = playerY_L2 + camOffsetY;
        float camZ = playerZ_L2 + camOffsetZ;

        float lookAtX = playerX_L2;
        float lookAtY = playerY_L2 + 2.0f;
        float lookAtZ = playerZ_L2;

        gluLookAt(
            camX, camY, camZ,
            lookAtX, lookAtY, lookAtZ,
            0, 1, 0
        );
    }

}


// Draw Nile River with textured land on both sides
// Grass: immediate area next to river
// Street: everything beyond grass
void drawNileRiver() {
    glEnable(GL_TEXTURE_2D);

    // Define zones
    float riverEdgeLeft = -150.0f;    // Left edge of river
    float riverEdgeRight = 150.0f;    // Right edge of river
    float grassWidth = 75.0f;        // Width of grass strip on each side
    float landFarLeft = -2000.0f;     // Far left edge of land
    float landFarRight = 2000.0f;     // Far right edge of land

    float grassLeftStart = riverEdgeLeft - grassWidth;   // -250
    float grassLeftEnd = riverEdgeLeft;                  // -150
    float grassRightStart = riverEdgeRight;              // 150
    float grassRightEnd = riverEdgeRight + grassWidth;   // 250

    float textureScale = 0.02f;  // Adjust for proper tiling

    // ========== RIVER WATER WITH TEXTURE ==========
    glDisable(GL_LIGHTING);  // Disable lighting for water for better texture visibility

    if (riverTexture.texture[0] != 0) {
        riverTexture.Use();
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        // Apply sunset tint to river texture
        glColor3f(
            1.0f,
            1.0f - sunsetProgress * 0.3f,
            1.0f - sunsetProgress * 0.5f
        );
    }
    else {
        // Fallback color if texture fails to load
        glColor3f(0.1f, 0.3f + sunsetProgress * 0.3f, 0.6f);
    }

    // Calculate texture coordinates based on player position for scrolling effect
    float texOffset = -playerZ_L2 * 0.001f; // Scrolling effect as player flies

    glBegin(GL_QUADS);
    // Bottom-left
    glTexCoord2f(0.0f + texOffset, 0.0f);
    glVertex3f(riverEdgeLeft, 0, riverEndZ);

    // Bottom-right
    glTexCoord2f(3.0f + texOffset, 0.0f); // 3.0 for repeating pattern
    glVertex3f(riverEdgeRight, 0, riverEndZ);

    // Top-right
    glTexCoord2f(3.0f + texOffset, 10.0f); // 10.0 for depth
    glVertex3f(riverEdgeRight, 0, riverStartZ + 100);

    // Top-left
    glTexCoord2f(0.0f + texOffset, 10.0f);
    glVertex3f(riverEdgeLeft, 0, riverStartZ + 100);
    glEnd();

    // ========== GRASS AREA (Grass.bmp) - ONE STRIP NEXT TO RIVER ==========
    if (grassTexture.texture[0] != 0) {
        grassTexture.Use();
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    glColor3f(1.0f, 1.0f, 1.0f); // White for texture

    // LEFT SIDE GRASS (-250 to -150)
    glBegin(GL_QUADS);
    // Bottom-left
    glTexCoord2f(0.0f, (riverEndZ - riverStartZ - 100) * textureScale);
    glVertex3f(grassLeftStart, 0, riverEndZ);
    // Bottom-right
    glTexCoord2f(grassWidth * textureScale, (riverEndZ - riverStartZ - 100) * textureScale);
    glVertex3f(grassLeftEnd, 0, riverEndZ);
    // Top-right
    glTexCoord2f(grassWidth * textureScale, 0.0f);
    glVertex3f(grassLeftEnd, 0, riverStartZ + 100);
    // Top-left
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(grassLeftStart, 0, riverStartZ + 100);
    glEnd();

    // RIGHT SIDE GRASS (150 to 250)
    glBegin(GL_QUADS);
    // Bottom-left
    glTexCoord2f(0.0f, (riverEndZ - riverStartZ - 100) * textureScale);
    glVertex3f(grassRightStart, 0, riverEndZ);
    // Bottom-right
    glTexCoord2f(grassWidth * textureScale, (riverEndZ - riverStartZ - 100) * textureScale);
    glVertex3f(grassRightEnd, 0, riverEndZ);
    // Top-right
    glTexCoord2f(grassWidth * textureScale, 0.0f);
    glVertex3f(grassRightEnd, 0, riverStartZ + 100);
    // Top-left
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(grassRightStart, 0, riverStartZ + 100);
    glEnd();

    // ========== CONCRETE AREA (ConcrMet.bmp) - EVERYTHING ELSE ==========
    if (concrMetTexture.texture[0] != 0) {
        concrMetTexture.Use();
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    glColor3f(1.0f, 1.0f, 1.0f); // White for texture

    // LEFT SIDE STREET (-2000 to -250)
    glBegin(GL_QUADS);
    // Bottom-left
    glTexCoord2f(0.0f, (riverEndZ - riverStartZ - 100) * textureScale);
    glVertex3f(landFarLeft, 0, riverEndZ);
    // Bottom-right
    glTexCoord2f((grassLeftStart - landFarLeft) * textureScale, (riverEndZ - riverStartZ - 100) * textureScale);
    glVertex3f(grassLeftStart, 0, riverEndZ);
    // Top-right
    glTexCoord2f((grassLeftStart - landFarLeft) * textureScale, 0.0f);
    glVertex3f(grassLeftStart, 0, riverStartZ + 100);
    // Top-left
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(landFarLeft, 0, riverStartZ + 100);
    glEnd();

    // RIGHT SIDE STREET (250 to 2000)
    glBegin(GL_QUADS);
    // Bottom-left
    glTexCoord2f(0.0f, (riverEndZ - riverStartZ - 100) * textureScale);
    glVertex3f(grassRightEnd, 0, riverEndZ);
    // Bottom-right
    glTexCoord2f((landFarRight - grassRightEnd) * textureScale, (riverEndZ - riverStartZ - 100) * textureScale);
    glVertex3f(landFarRight, 0, riverEndZ);
    // Top-right
    glTexCoord2f((landFarRight - grassRightEnd) * textureScale, 0.0f);
    glVertex3f(landFarRight, 0, riverStartZ + 100);
    // Top-left
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(grassRightEnd, 0, riverStartZ + 100);
    glEnd();

    // ========== RIVER BANKS (OPTIONAL DARKER STRIP) ==========
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.5f, 0.4f, 0.2f);

    // Left bank (transition between river and grass)
    glBegin(GL_QUADS);
    glVertex3f(-180, 0, riverEndZ);
    glVertex3f(-150, 0, riverEndZ);
    glVertex3f(-150, 0, riverStartZ + 100);
    glVertex3f(-180, 0, riverStartZ + 100);
    glEnd();

    // Right bank
    glBegin(GL_QUADS);
    glVertex3f(150, 0, riverEndZ);
    glVertex3f(180, 0, riverEndZ);
    glVertex3f(180, 0, riverStartZ + 100);
    glVertex3f(150, 0, riverStartZ + 100);
    glEnd();

    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
}

// Small Level 1 style street made of buildings for the win scene
void drawWinSceneBuildings() {
    if (!winSceneActive) return;

    glColor3f(1.0f, 1.0f, 1.0f);

    const float buildingScale = 0.23f;
    const float groundY = 0.0f;

    float leftX = WIN_SCENE_CENTER_X - 30.0f;
    float rightX = WIN_SCENE_CENTER_X + 30.0f;

    float frontZ = WIN_SCENE_CENTER_Z - 20.0f;
    float backZ = WIN_SCENE_CENTER_Z - 80.0f;

    // Front left
    glPushMatrix();
    glTranslatef(leftX, groundY, frontZ);
    glScalef(buildingScale, buildingScale, buildingScale);
    buildingModel_L2.Draw();
    glPopMatrix();

    // Front right, flip like Level 1
    glPushMatrix();
    glTranslatef(rightX, groundY, frontZ);
    glRotatef(180.0f, 0, 1, 0);
    glScalef(buildingScale, buildingScale, buildingScale);
    buildingModel_L2.Draw();
    glPopMatrix();

    // Back left
    glPushMatrix();
    glTranslatef(leftX, groundY, backZ);
    glScalef(buildingScale, buildingScale, buildingScale);
    buildingModel_L2.Draw();
    glPopMatrix();

    // Back right
    glPushMatrix();
    glTranslatef(rightX, groundY, backZ);
    glRotatef(180.0f, 0, 1, 0);
    glScalef(buildingScale, buildingScale, buildingScale);
    buildingModel_L2.Draw();
    glPopMatrix();
}


// Draw flying collectibles (3ennabeyat) with animation - USING REAL TEXTURES
void drawFlyingCollectibles() {
    static float vibrationOffset = 0.0f;
    static float vibrationTimer = 0.0f;

    // Update vibration timer
    vibrationTimer += 0.1f;
    if (vibrationTimer > 360.0f) vibrationTimer -= 360.0f;

    // Calculate gentle up/down vibration
    vibrationOffset = sin(vibrationTimer * 3.14159f / 180.0f) * 1.5f; // 1.5 units up/down

    for (int i = 0; i < numFlyingCollectibles; i++) {
        if (!flyingCollectibles[i].active) continue;

        glPushMatrix();

        // Base position at collider center + model pivot offset
        glTranslatef(
            flyingCollectibles[i].x + COLLECTIBLE_MODEL_OFFSET_X_L2,
            flyingCollectibles[i].y + COLLECTIBLE_MODEL_OFFSET_Y_L2,
            flyingCollectibles[i].z + COLLECTIBLE_MODEL_OFFSET_Z_L2
        );

        // Gentle up/down vibration (different for each collectible)
        float individualVibe = sin((vibrationTimer + i * 20.0f) * 3.14159f / 180.0f) * 0.5f;
        glTranslatef(0.0f, vibrationOffset + individualVibe, 0.0f);

        // Very subtle rotation - just enough to make it interesting
        float subtleRotate = sin((vibrationTimer + i * 10.0f) * 3.14159f / 180.0f) * 3.0f;
        glRotatef(subtleRotate, 0, 1, 0);

        // Small side-to-side wobble
        float wobble = sin((vibrationTimer * 0.7f + i * 15.0f) * 3.14159f / 180.0f) * 0.5f;
        glRotatef(wobble, 1, 0, 0);

        // Draw 3ennabeyat model with REAL TEXTURES
        float baseScale = 1.5f;  // Enlarged size
        glScalef(baseScale, baseScale, baseScale);

        // ========== DRAW TEXTURED MODEL ==========
        // Enable texturing and lighting
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);

        // Set material properties to make textures bright
        GLfloat matAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Full white ambient for bright textures
        GLfloat matDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat matSpecular[] = { 0.3f, 0.3f, 0.3f, 1.0f }; // Low specular to not overpower texture
        GLfloat matShininess[] = { 20.0f };

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);

        // IMPORTANT: Set color to white so textures show correctly
        glColor3f(1.0f, 1.0f, 1.0f);

        // Draw the collectible model (textured)
        collectibleModel_L2.Draw();

        // ------- Red glow around collectible -------
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float glowRadius = 2.0f;   // slightly bigger, because Level 2 collectibles are larger
        glColor4f(1.0f, 0.2f, 0.2f, 0.6f);
        glutSolidSphere(glowRadius, 16, 16);

        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        // -------------------------------------------

        glPopMatrix();
    }


    // Draw collection animations (particle effects when collected)
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < MAX_ANIM_COLLECTIBLES; i++) {
        if (collectibleAnimations[i].active) {
            glPushMatrix();
            glTranslatef(collectibleAnimations[i].x,
                collectibleAnimations[i].y,
                collectibleAnimations[i].z);

            // Add vibration to collection animation too
            float animVibe = sin((vibrationTimer + i) * 3.14159f / 180.0f) * 0.3f;
            glTranslatef(0.0f, animVibe, 0.0f);

            // Scale down and fade out animation
            collectibleAnimations[i].scale *= 0.9f;
            collectibleAnimations[i].rotation += 10.0f;
            collectibleAnimations[i].alpha *= 0.8f;

            glRotatef(collectibleAnimations[i].rotation, 0, 1, 0);
            glScalef(collectibleAnimations[i].scale,
                collectibleAnimations[i].scale,
                collectibleAnimations[i].scale);

            // Draw shrinking collectible with gold color
            glColor4f(1.0f, 0.8f, 0.0f, collectibleAnimations[i].alpha); // Gold color
            glutSolidSphere(3.0, 12, 12);

            // Add particle effect
            glColor4f(1.0f, 1.0f, 0.5f, collectibleAnimations[i].alpha * 0.5f); // Light gold
            glutSolidSphere(3.5, 8, 8);

            glPopMatrix();

            if (collectibleAnimations[i].alpha < 0.1f) {
                collectibleAnimations[i].active = false;
            }
        }
    }

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// Draw birds using the Bird_Level2.3ds model with patrol logic - FIXED FLICKERING
void drawBirds() {
    // Update flapping animation
    static float wingFlapTimer = 0.0f;
    wingFlapTimer += 0.1f;
    if (wingFlapTimer > 360.0f) wingFlapTimer -= 360.0f;

    // ====== SAVE CURRENT STATE ======
    GLboolean depthTest, blendEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTest);
    glGetBooleanv(GL_BLEND, &blendEnabled);

    // Ensure depth test is ON and blending is OFF for birds
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND); // Disable blending to avoid transparency issues

    // ====== CRITICAL FIX: ENABLE POLYGON OFFSET ======
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 3.0f); // Adjust these values as needed

    for (int i = 0; i < numBirds; i++) {
        if (!birds[i].active) continue;

        glPushMatrix();

        // Position the bird with slight Z offset to prevent flickering
        float zOffset = 0.05f * (i % 10); // Small unique offset for each bird
        glTranslatef(birds[i].x, birds[i].y, birds[i].z + zOffset);

        // Face direction of movement (patrol direction)
        if (birds[i].movingRight) {
            glRotatef(90.0f, 0, 1, 0); // Face right (+X)
        }
        else {
            glRotatef(-90.0f, 0, 1, 0); // Face left (-X)
        }

        // Add slight upward tilt for flying look
        glRotatef(-10.0f, 1, 0, 0);

        // Wing flapping animation based on movement
        float flapIntensity = 25.0f;
        float flapSpeed = birds[i].patrolSpeed * 0.3f;

        // Wing flapping - more pronounced animation
        float wingFlap = sin(wingFlapTimer * flapSpeed + i) * flapIntensity;
        glRotatef(wingFlap, 1, 0, 0);

        // ====== BIRD SCALE ======
        float baseScale = 2.0f; // Adjust this value

        // Size variation based on height - higher birds are slightly larger
        float heightRatio = (birds[i].y - minHeight) / (maxHeight - minHeight);
        float sizeVariation = 0.8f + heightRatio * 0.4f;

        // Apply scale
        float finalScale = baseScale * sizeVariation;
        glScalef(finalScale, finalScale, finalScale);

        // ====== IMPORTANT: DISABLE ALPHA TEST AND BLENDING ======
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);

        // Enable texturing and lighting
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);

        // Set material properties
        GLfloat matAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Full white ambient
        GLfloat matDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat matSpecular[] = { 0.5f, 0.5f, 0.5f, 1.0f }; // Reduced specular
        GLfloat matShininess[] = { 30.0f };

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);

        // Force solid color (no transparency)
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Full alpha

        // ====== DRAW BIRD MODEL WITH DEPTH MASK ENABLED ======
        glDepthMask(GL_TRUE); // Ensure depth writing is enabled
        birdModel_L2.Draw();

        glPopMatrix();
    }

    // ====== RESTORE POLYGON OFFSET STATE ======
    glDisable(GL_POLYGON_OFFSET_FILL);

    // ====== RESTORE PREVIOUS STATE ======
    if (!depthTest) glDisable(GL_DEPTH_TEST);
    if (blendEnabled) glEnable(GL_BLEND);
}

// Draw Dr. Beram with rescue animation - USING 3D MODEL
void drawDrBeram() {
    if (!drBeram.active && !drBeramRescued) return;

    glPushMatrix();

    if (!drBeram.active && !drBeramRescued) return;

    glPushMatrix();

    if (winSceneActive && gameState_L2 == GAME_WON) {
        // Final celebration scene. Dr Beram stands near the buildings
        glTranslatef(
            drBeram.x + DR_BERAM_MODEL_OFFSET_X_L2,
            drBeram.y + DR_BERAM_MODEL_OFFSET_Y_L2,
            drBeram.z + DR_BERAM_MODEL_OFFSET_Z_L2
        );

        // Small idle float so he does not look frozen
        float floatOffset = sinf(rescueAnimationTime * 2.0f) * 0.5f;
        glTranslatef(0.0f, floatOffset, 0.0f);

        glRotatef(rescueAnimationTime * 30.0f, 0, 1, 0);
    }
    else if (drBeramRescued) {
        // Old rescue animation. attached to flying player
        float animY = drBeram.y + rescueAnimationTime * 5.0f;
        glTranslatef(playerX_L2, playerY_L2 + 5.0f + animY, playerZ_L2 - 5.0f);

        glRotatef(rescueAnimationTime * 100.0f, 0, 1, 0);

        float rescueScale = 1.0f + rescueAnimationTime * 0.2f;
        glScalef(rescueScale, rescueScale, rescueScale);
    }
    else {
        // Waiting to be rescued in the river scene
        glTranslatef(
            drBeram.x + DR_BERAM_MODEL_OFFSET_X_L2,
            drBeram.y + DR_BERAM_MODEL_OFFSET_Y_L2,
            drBeram.z + DR_BERAM_MODEL_OFFSET_Z_L2
        );

        float floatOffset = sinf(rescueAnimationTime * 2.0f) * 1.5f;
        glTranslatef(0.0f, floatOffset, 0.0f);

        glRotatef(rescueAnimationTime * 50.0f, 0, 1, 0);
    }


    // Save current lighting and texturing state
    GLboolean lightingWasEnabled;
    GLboolean textureWasEnabled;
    glGetBooleanv(GL_LIGHTING, &lightingWasEnabled);
    glGetBooleanv(GL_TEXTURE_2D, &textureWasEnabled);

    // Ensure lighting and texturing are enabled for the model
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Set material properties
    GLfloat matAmbient[] = { 0.6f, 0.6f, 0.6f, 1.0f };
    GLfloat matDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat matSpecular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat matShininess[] = { 30.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);

    // Set color based on state. keep white so texture shows clearly
    if (drBeramRescued) {
        glColor3f(0.0f, 1.0f, 0.0f);   // extra tint when rescued
    }
    else {
        glColor3f(1.0f, 1.0f, 1.0f);   // pure white for texture
    }

    // Draw Dr Beram model with appropriate scale
    float beramScale = 0.2f;
    glScalef(beramScale, beramScale, beramScale);
    drBeramModel.Draw();

    // Restore previous lighting and texturing state
    if (!textureWasEnabled)  glDisable(GL_TEXTURE_2D);
    if (!lightingWasEnabled) glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);


    // Add a glowing effect when rescued
    if (drBeramRescued) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Draw a glowing sphere around Dr. Beram
        glPushMatrix();
        glColor4f(0.0f, 1.0f, 0.0f, 0.3f);
        glutSolidSphere(15.0f, 16, 16);
        glPopMatrix();
        
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    // Restore lighting state
    if (!lightingWasEnabled) {
        glDisable(GL_LIGHTING);
    }
    glDisable(GL_COLOR_MATERIAL);

    glPopMatrix();
}

// Draw flying player with wing flap animation - USING MODEL LIKE LEVEL 1
void drawPlayerLevel2() {
    heroWingFlap += 10.0f;
    if (heroWingFlap > 360.0f) heroWingFlap -= 360.0f;

    glPushMatrix();

    // Base position + model offset
    glTranslatef(
        playerX_L2 + PLAYER_MODEL_OFFSET_X_L2,
        playerY_L2 + PLAYER_MODEL_OFFSET_Y_L2,
        playerZ_L2 + PLAYER_MODEL_OFFSET_Z_L2
    );

    if (!winSceneActive) {
        // Face the current movement direction first
        glRotatef(playerFacingAngle_L2, 0, 1, 0);

        // Spin around Y when collecting (additive)
        if (playerSpinning_L2) {
            glRotatef(playerSpinAngle_L2, 0, 1, 0);
        }

        // Flying pose (tilt)
// Positive X rotation = nose down (lean forward)
        glRotatef(FORWARD_TILT_ANGLE, 1, 0, 0);
        glRotatef(playerRollAngle, 0, 0, 1);
        glRotatef(playerPitchAngle, 1, 0, 0);

    }
    else {
        // Win scene. stand mostly upright, slight idle rotate
        glRotatef(180.0f, 0, 1, 0);
        glRotatef(sinf(rescueAnimationTime) * 3.0f, 0, 1, 0);
    }



    // ====== INCREASED PLAYER SCALE ======
    // Original: glScalef(1.5f, 1.5f, 1.5f);
    // Try these values:
    float playerScale = 2.0f;  // CHANGED FROM 1.5f TO 2.0f (33% larger)
    glScalef(playerScale, playerScale, playerScale);

    // Save and restore lighting state
    GLboolean lightingWasEnabled;
    glGetBooleanv(GL_LIGHTING, &lightingWasEnabled);

    // Ensure lighting is ON before drawing the model
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Set material properties to make model bright
    GLfloat matAmbient[] = { 0.7f, 0.7f, 0.7f, 1.0f };
    GLfloat matDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat matSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat matShininess[] = { 50.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);

    // IMPORTANT: Set color to white to ensure texture shows correctly
    glColor3f(1.0f, 1.0f, 1.0f);

    // Draw player model
    playerModel_L2.Draw();

    // Restore lighting state
    if (!lightingWasEnabled) {
        glDisable(GL_LIGHTING);
    }

    glDisable(GL_COLOR_MATERIAL);

    glPopMatrix();
}

// Setup sunset lighting with animated lights
void setupSunLight() {
    glEnable(GL_LIGHTING);

    // Main sunlight (LIGHT0) - sunset colors with intensity changes
    glEnable(GL_LIGHT0);

    // Calculate intensity reduction as sun sets - VERY DRAMATIC
    // Intensity drops from 1.0 to 0.1 (90% reduction)
    float lightIntensity = 1.0f - (sunsetProgress * 0.9f);
    if (lightIntensity < 0.1f) lightIntensity = 0.1f;

    // Sun color based on sunset progress - keep colors vibrant even as intensity drops
    // This ensures color changes are visible
    GLfloat lightDiffuse[] = { 
        sunColor[0] * lightIntensity, 
        sunColor[1] * lightIntensity, 
        sunColor[2] * lightIntensity, 
        1.0f 
    };
    
    // Ambient light: starts bright, dims significantly as sun sets
    // Also shifts to warmer colors (more red/orange) as sunset progresses
    float ambientIntensity = 0.4f - (sunsetProgress * 0.3f);
    if (ambientIntensity < 0.1f) ambientIntensity = 0.1f;
    
    // Make ambient color shift more dramatic
    GLfloat lightAmbient[] = {
        0.25f + sunsetProgress * 0.4f,  // More red as sun sets (0.25 -> 0.65)
        0.2f + sunsetProgress * 0.2f,   // Less green increase (0.2 -> 0.4)
        0.1f + sunsetProgress * 0.05f,   // Minimal blue (0.1 -> 0.15)
        1.0f
    };
    // Apply intensity to ambient
    lightAmbient[0] *= ambientIntensity;
    lightAmbient[1] *= ambientIntensity;
    lightAmbient[2] *= ambientIntensity;
    
    // Specular also dims with sunset
    GLfloat lightSpecular[] = { 
        lightIntensity, 
        lightIntensity * 0.9f, 
        lightIntensity * 0.7f, 
        1.0f 
    };

    // Sun position (moves with sunset)
    float sunX = 200.0f - sunsetProgress * 300.0f;
    float sunY = 150.0f - sunsetProgress * 100.0f;
    GLfloat lightPosition[] = { sunX, sunY, -500.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    // Fill light (LIGHT1) - also dims as sunset progresses
    glEnable(GL_LIGHT1);
    float fillIntensity = 0.5f - (sunsetProgress * 0.35f);
    if (fillIntensity < 0.1f) fillIntensity = 0.1f;
    
    GLfloat fillAmbient[] = { 
        0.1f * fillIntensity, 
        0.1f * fillIntensity, 
        0.1f * fillIntensity, 
        1.0f 
    };
    GLfloat fillDiffuse[] = { 
        0.4f * fillIntensity, 
        0.4f * fillIntensity, 
        0.4f * fillIntensity, 
        1.0f 
    };
    GLfloat fillPosition[] = { 0.0f, 100.0f, 0.0f, 1.0f };

    glLightfv(GL_LIGHT1, GL_AMBIENT, fillAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, fillDiffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, fillPosition);

    // Animated orbiting light (LIGHT2) - REQUIRED FOR LIGHT ANIMATION
    glEnable(GL_LIGHT2);

    // Calculate orbiting position
    float orbitX = cos(lightOrbitAngle) * lightOrbitRadius;
    float orbitZ = sin(lightOrbitAngle) * lightOrbitRadius;

    GLfloat orbitPos[] = { orbitX, lightOrbitHeight, orbitZ, 1.0f };
    GLfloat orbitColor[] = { 0.8f, 0.8f, 0.3f, 1.0f }; // Yellowish

    glLightfv(GL_LIGHT2, GL_POSITION, orbitPos);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, orbitColor);
    glLightfv(GL_LIGHT2, GL_SPECULAR, orbitColor);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 0.5f);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.02f);
    glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.001f);

    // Additional moving light (LIGHT3) - moves up and down
    glEnable(GL_LIGHT3);

    float bounceY = lightOrbitHeight + sin(lightOrbitAngle * 2.0f) * 20.0f;
    GLfloat bouncePos[] = { -lightOrbitRadius, bounceY, 0.0f, 1.0f };
    GLfloat bounceColor[] = { 0.3f, 0.8f, 0.8f, 1.0f }; // Blueish

    glLightfv(GL_LIGHT3, GL_POSITION, bouncePos);
    glLightfv(GL_LIGHT3, GL_DIFFUSE, bounceColor);
    glLightfv(GL_LIGHT3, GL_SPECULAR, bounceColor);
    glLightf(GL_LIGHT3, GL_CONSTANT_ATTENUATION, 0.6f);
    glLightf(GL_LIGHT3, GL_LINEAR_ATTENUATION, 0.03f);
    glLightf(GL_LIGHT3, GL_QUADRATIC_ATTENUATION, 0.001f);
}

// Update sunset progress
void updateSunset(float deltaTime) {
    if (!sunsetActive) return;

    // Gradually progress sunset based on time and player position
    // Use remainingTime_L2 which is already being updated in idleLevel2
    float progressByTime = (float)(level2DurationMs - remainingTime_L2) / level2DurationMs;
    if (progressByTime < 0.0f) progressByTime = 0.0f;
    if (progressByTime > 1.0f) progressByTime = 1.0f;
    
    float progressByDistance = (riverStartZ - playerZ_L2) / (riverStartZ - riverEndZ);
    if (progressByDistance < 0.0f) progressByDistance = 0.0f;
    if (progressByDistance > 1.0f) progressByDistance = 1.0f;

    // Combine both factors - make sunset progress faster and more visible
    sunsetProgress = 0.6f * progressByTime + 0.4f * progressByDistance;
    
    // Make sunset transition happen faster - compress the timeline
    // This makes changes visible sooner (complete sunset in 80% of level time)
    sunsetProgress = sunsetProgress * 1.25f; // Speed up by 25%
    if (sunsetProgress > 1.0f) sunsetProgress = 1.0f;

    if (sunsetProgress > 1.0f) sunsetProgress = 1.0f;
    if (sunsetProgress < 0.05f) sunsetProgress = 0.05f; // Ensure minimum visibility

    // Enhanced smooth color transition: yellow -> orange -> deep red
    // Make color changes more dramatic and visible
    // Early sunset (0.0-0.4): Yellow to Orange
    if (sunsetProgress < 0.4f) {
        float t = sunsetProgress / 0.4f; // 0 to 1 over first 40%
        sunColor[0] = 1.0f;                    // Red stays at max
        sunColor[1] = 0.9f - (t * 0.5f);       // Green: 0.9 -> 0.4 (more dramatic)
        sunColor[2] = 0.0f;                     // Blue stays at 0
    }
    // Mid sunset (0.4-0.7): Orange to Red-Orange
    else if (sunsetProgress < 0.7f) {
        float t = (sunsetProgress - 0.4f) / 0.3f; // 0 to 1 over 40-70%
        sunColor[0] = 1.0f;                    // Red stays at max
        sunColor[1] = 0.4f - (t * 0.25f);      // Green: 0.4 -> 0.15
        sunColor[2] = 0.0f;                     // Blue stays at 0
    }
    // Late sunset (0.7-1.0): Red-Orange to Deep Red
    else {
        float t = (sunsetProgress - 0.7f) / 0.3f; // 0 to 1 over last 30%
        sunColor[0] = 1.0f;                    // Red stays at max
        sunColor[1] = 0.15f - (t * 0.1f);      // Green: 0.15 -> 0.05 (very red)
        sunColor[2] = t * 0.08f;               // Blue: 0 -> 0.08 (slight purple tint)
    }
}

// Update light animation
void updateLightAnimation(float deltaTime) {
    // Rotate orbiting lights
    lightOrbitAngle += 0.5f * deltaTime; // Rotate lights slowly
    if (lightOrbitAngle > 360.0f) lightOrbitAngle -= 360.0f;
}

// Update birds with back-and-forth patrol movement at different heights
void updateBirds(float deltaTime) {
    static float globalTime = 0.0f;
    globalTime += deltaTime;

    for (int i = 0; i < numBirds; i++) {
        if (!birds[i].active) continue;

        // PATROL MOVEMENT: Move back and forth along X-axis
        if (birds[i].movingRight) {
            birds[i].x += birds[i].patrolSpeed * deltaTime;

            // Check if reached right endpoint
            if (birds[i].x >= birds[i].patrolEndX) {
                birds[i].x = birds[i].patrolEndX; // Clamp
                birds[i].movingRight = false; // Turn around
            }
        }
        else { // Moving left
            birds[i].x -= birds[i].patrolSpeed * deltaTime;

            // Check if reached left endpoint
            if (birds[i].x <= birds[i].patrolStartX) {
                birds[i].x = birds[i].patrolStartX; // Clamp
                birds[i].movingRight = true; // Turn around
            }
        }

        // NEW: Complex vertical bobbing with different patterns
        float verticalMovement = 0.0f;

        // Different bobbing patterns for variety
        switch (i % 3) {
        case 0: // Smooth sine wave
            verticalMovement = sin(globalTime * birds[i].verticalBobSpeed + birds[i].bobPhase) * birds[i].verticalBobAmplitude;
            break;
        case 1: // Faster, smaller bobs
            verticalMovement = sin(globalTime * birds[i].verticalBobSpeed * 1.5f + birds[i].bobPhase) * (birds[i].verticalBobAmplitude * 0.7f);
            break;
        case 2: // Gentle, slow bobs
            verticalMovement = sin(globalTime * birds[i].verticalBobSpeed * 0.7f + birds[i].bobPhase) * (birds[i].verticalBobAmplitude * 1.2f);
            break;
        }

        // Apply vertical movement
        birds[i].y = birds[i].patrolY + verticalMovement;

        // Keep birds within vertical bounds
        if (birds[i].y < minHeight + 5.0f) birds[i].y = minHeight + 5.0f;
        if (birds[i].y > maxHeight - 5.0f) birds[i].y = maxHeight - 5.0f;

        // Keep birds within river bounds
        if (birds[i].x < riverMinX + 10) {
            birds[i].x = riverMinX + 10;
            birds[i].movingRight = true;
        }
        if (birds[i].x > riverMaxX - 10) {
            birds[i].x = riverMaxX - 10;
            birds[i].movingRight = false;
        }
    }
}

// Update player animations
void updatePlayerAnimations(float deltaTime) {
    // Update spin animation
    if (playerSpinning_L2) {
        const float spinDuration = 0.5f;
        const float spinSpeed = 720.0f;

        playerSpinTime_L2 += deltaTime;
        playerSpinAngle_L2 += spinSpeed * deltaTime;

        if (playerSpinTime_L2 >= spinDuration) {
            playerSpinning_L2 = false;
            playerSpinAngle_L2 = 0.0f;
            playerSpinTime_L2 = 0.0f;
        }
    }

    // Update hit animation
    if (playerHit_L2) {
        const float hitDuration = 0.3f;
        const float MAX_BACK = 3.0f;      // same total knockback as before

        playerHitTime_L2 += deltaTime;

        float t = playerHitTime_L2 / hitDuration;
        if (t > 1.0f) t = 1.0f;

        // Smooth slide back (0 → 1)
        float slide = (1.0f - cosf(t * 3.14159f)) * 0.5f;

        // How far we are behind the start point
        playerHitDistance_L2 = slide * MAX_BACK;

        // 🔥 THIS actually moves the real player position
        playerZ_L2 = playerHitStartZ_L2 + playerHitDistance_L2;

        if (playerHitTime_L2 >= hitDuration) {
            playerHit_L2 = false;
            playerHitTime_L2 = 0.0f;
            playerHitDistance_L2 = 0.0f;
        }
    }


    // Smooth banking return
    if (!keyStates['a'] && !keyStates['d']) {
        playerRollAngle *= 0.9f;
    }

    // Smooth pitch return
    if (!keyStates['w'] && !keyStates['s']) {
        playerPitchAngle *= 0.9f;
    }
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
    drawText2D(-0.95f, -0.9f, "Controls: A/D=Left/Right  W/S=Up/Down  Arrows=Speed");
    drawText2D(-0.95f, -0.85f, "5 Lives - Hit birds to lose lives");
    drawText2D(-0.95f, -0.95f, "1=1st Person  3=3rd Person  ESC=Exit");
}

void drawGameStatusLevel2() {
    if (gameState_L2 == GAME_WON) {
        glDisable(GL_LIGHTING);
        drawText2D(-0.35f, 0.2f, "MISSION ACCOMPLISHED!");
        drawText2D(-0.3f, 0.1f, "Dr. Beram Rescued!");
        drawText2D(-0.25f, 0.0f, "Final Score: ");
        drawText2D(-0.25f, -0.1f, "Lives Remaining: ");

        char scoreBuffer[32];
        sprintf(scoreBuffer, "%d", score_L2);
        drawText2D(0.05f, 0.0f, scoreBuffer);

        char livesBuffer[32];
        sprintf(livesBuffer, "%d", playerLives);
        drawText2D(0.05f, -0.1f, livesBuffer);

        drawText2D(-0.4f, -0.2f, "Press ESC to exit");
        glEnable(GL_LIGHTING);
    }
    else if (gameState_L2 == GAME_LOST) {
        glDisable(GL_LIGHTING);
        drawText2D(-0.3f, 0.1f, "MISSION FAILED!");
        drawText2D(-0.35f, 0.0f, "You ran out of lives!");
        drawText2D(-0.4f, -0.1f, "Press ESC to exit");
        glEnable(GL_LIGHTING);
    }
}

// ===============================
// Level 2 GLUT Callbacks
// ===============================

void keyboardLevel2(unsigned char key, int x, int y) {
    // ESC should always exit in Level 2
    if (key == 27) {
        exit(0);
        return;
    }

    // If the mission is finished or failed, ignore other keys
    if (gameState_L2 != GAME_PLAYING) {
        return;
    }

    switch (key) {
        // Camera mode keys for Level 2
    case '1':
        // First person camera for Level 2
        cameraMode_L2 = FIRST_PERSON;
        // Reset mouse orbit so the view is stable
        cameraYaw_L2 = 0.0f;
        cameraPitch_L2 = 0.0f;
        glutPostRedisplay();
        break;

    case '3':
        // Third person camera for Level 2
        cameraMode_L2 = THIRD_PERSON;
        // Reset mouse orbit
        cameraYaw_L2 = 0.0f;
        cameraPitch_L2 = 0.0f;
        glutPostRedisplay();
        break;

        // Movement keys
    case 'a': case 'A': // Left
        keyStates['a'] = true;
        cameraYaw_L2 = 0.0f;
        cameraPitch_L2 = 0.0f;
        break;

    case 'd': case 'D': // Right
        keyStates['d'] = true;
        cameraYaw_L2 = 0.0f;
        cameraPitch_L2 = 0.0f;
        break;

    case 'w': case 'W': // Up
        keyStates['w'] = true;
        cameraYaw_L2 = 0.0f;
        cameraPitch_L2 = 0.0f;
        break;

    case 's': case 'S': // Down
        keyStates['s'] = true;
        cameraYaw_L2 = 0.0f;
        cameraPitch_L2 = 0.0f;
        break;
    }
}


void keyboardUpLevel2(unsigned char key, int x, int y) {
    switch (key) {
    case 'a': case 'A':
        keyStates['a'] = false;
        break;
    case 'd': case 'D':
        keyStates['d'] = false;
        break;
    case 'w': case 'W':
        keyStates['w'] = false;
        break;
    case 's': case 'S':
        keyStates['s'] = false;
        break;
    }
}

void specialKeysLevel2(int key, int x, int y) {
    if (gameState_L2 != GAME_PLAYING) return;

    switch (key) {
    case GLUT_KEY_UP: // Speed up AND move up
        specialKeyStates[GLUT_KEY_UP] = true;
        keyStates['w'] = true;  // ADD THIS LINE
        break;
    case GLUT_KEY_DOWN: // Slow down AND move down
        specialKeyStates[GLUT_KEY_DOWN] = true;
        keyStates['s'] = true;  // ADD THIS LINE
        break;
    case GLUT_KEY_LEFT: // Left arrow
        keyStates['a'] = true;
        break;
    case GLUT_KEY_RIGHT: // Right arrow
        keyStates['d'] = true;
        break;
    }
}

void specialUpLevel2(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP:
        specialKeyStates[GLUT_KEY_UP] = false;
        keyStates['w'] = false;  // ADD THIS LINE
        break;
    case GLUT_KEY_DOWN:
        specialKeyStates[GLUT_KEY_DOWN] = false;
        keyStates['s'] = false;  // ADD THIS LINE
        break;
    case GLUT_KEY_LEFT:
        keyStates['a'] = false;
        break;
    case GLUT_KEY_RIGHT:
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
        // Still update sunset and light animations
        updateSunset(deltaTime);
        updateLightAnimation(deltaTime);
        glutPostRedisplay();
        return;
    }

    // ========== GAME LOGIC ==========

    // Update timer for sunset calculation
    int elapsedMs = currentMs - level2StartTimeMs;
    remainingTime_L2 = level2DurationMs - elapsedMs;
    if (remainingTime_L2 < 0) remainingTime_L2 = 0;

    // Update animations
    updateSunset(deltaTime);
    updateLightAnimation(deltaTime);
    updatePlayerAnimations(deltaTime);
    updateBirds(deltaTime);

    // ========== PLAYER MOVEMENT ==========

    // Auto-forward movement (always fly forward)
    playerZ_L2 -= playerSpeed_L2 * deltaTime;

    // Horizontal & vertical movement + facing direction
    float turnSpeed = 30.0f * deltaTime;
    bool movedDir = false;

    // Vertical movement has priority for facing
    if (keyStates['w']) {
        playerY_L2 += liftForce * deltaTime;
        playerPitchAngle = -10.0f;     // Nose up
        playerFacingAngle_L2 = 180.0f; // forward (-Z)
        movedDir = true;
    }
    else if (keyStates['s']) {
        playerY_L2 -= liftForce * deltaTime;
        playerPitchAngle = 10.0f;      // Nose down
        playerFacingAngle_L2 = 0.0f;   // back (+Z)
        movedDir = true;
    }
    else if (keyStates['a']) {
        playerX_L2 -= turnSpeed;
        playerRollAngle = 15.0f;       // Bank left
        playerFacingAngle_L2 = 270.0f; // left (-X)
        movedDir = true;
    }
    else if (keyStates['d']) {
        playerX_L2 += turnSpeed;
        playerRollAngle = -15.0f;      // Bank right
        playerFacingAngle_L2 = 90.0f;  // right (+X)
        movedDir = true;
    }

    if (!movedDir) {
        // No directional keys. look forward
        playerFacingAngle_L2 = 180.0f;
    }


    // Speed control
    if (specialKeyStates[GLUT_KEY_UP]) {
        playerSpeed_L2 += 5.0f * deltaTime;
        if (playerSpeed_L2 > 40.0f) playerSpeed_L2 = 40.0f;
    }
    if (specialKeyStates[GLUT_KEY_DOWN]) {
        playerSpeed_L2 -= 5.0f * deltaTime;
        if (playerSpeed_L2 < 8.0f) playerSpeed_L2 = 8.0f;
    }

    // Clamp player position
    if (playerX_L2 < riverMinX) playerX_L2 = riverMinX;
    if (playerX_L2 > riverMaxX) playerX_L2 = riverMaxX;
    if (playerY_L2 < minHeight) playerY_L2 = minHeight;
    if (playerY_L2 > maxHeight) playerY_L2 = maxHeight;

    // Check if player reached end without rescuing
    if (playerZ_L2 < riverEndZ && !drBeramRescued) {
        gameState_L2 = GAME_LOST;
        // ✅ play lose sound once
        playloseSound();
        glutPostRedisplay();
        return;
    }

    // ========== COLLISION DETECTION ==========
    AABB_L2 playerBox = getPlayerAABB_L2();

    // Check collectibles
    for (int i = 0; i < numFlyingCollectibles; i++) {
        if (flyingCollectibles[i].active && checkAABBCollision3D(playerBox, flyingCollectibles[i])) {
            flyingCollectibles[i].active = false;
            score_L2 += 20;
            // ✅ play collect sound
            playCollectibleSound();
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
        drBeramRescued = true;
        gameState_L2 = GAME_WON;
        playwinSound();
        score_L2 += 500;

        // Move to the win scene. buildings from Level 1 and sky
        winSceneActive = true;

        // Put Dr Beram in the middle of the street
        drBeram.x = WIN_SCENE_CENTER_X;
        drBeram.y = WIN_SCENE_PLAYER_Y;
        drBeram.z = WIN_SCENE_CENTER_Z;

        // Put player a bit in front of him
        playerX_L2 = WIN_SCENE_CENTER_X;
        playerY_L2 = WIN_SCENE_PLAYER_Y;
        playerZ_L2 = WIN_SCENE_CENTER_Z + 20.0f;

        // Stop flying movement and reset tilt, spin etc
        playerSpeed_L2 = 0.0f;
        playerRollAngle = 0.0f;
        playerPitchAngle = 0.0f;
        playerSpinning_L2 = false;
        playerSpinTime_L2 = 0.0f;
        playerSpinAngle_L2 = 0.0f;

        // keep drBeram.active = true so we can draw him
    }


    glutPostRedisplay();
}

// Draw lives with heart symbols
// Draw lives with heart symbols in 2D HUD space
void drawLivesDisplay() {
    if (playerLives <= 0) return;

    // --- Set up 2D projection like drawText2D ---
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Save and tweak states for HUD
    GLboolean depthEnabled, texEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
    glGetBooleanv(GL_TEXTURE_2D, &texEnabled);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    // lighting is already disabled in displayLevel2() before this call

    glColor3f(1.0f, 0.0f, 0.0f); // red hearts

    // Position hearts on top-left, to the right of "Score:"
    float heartStartX = -0.25f;   // adjust to taste
    float heartY = 0.9f;
    float heartSpacing = 0.08f;
    float heartSize = 0.04f;   // overall heart size

    for (int i = 0; i < playerLives; ++i) {
        float x = heartStartX + i * heartSpacing;
        float y = heartY;

        glBegin(GL_TRIANGLES);
        // left triangle
        glVertex2f(x, y);
        glVertex2f(x - heartSize * 0.5f, y - heartSize);
        glVertex2f(x + heartSize * 0.5f, y - heartSize);
        // right triangle
        glVertex2f(x + heartSize * 0.5f, y - heartSize);
        glVertex2f(x + heartSize, y);
        glVertex2f(x, y);
        glEnd();
    }

    // Restore state
    if (texEnabled)   glEnable(GL_TEXTURE_2D);
    if (depthEnabled) glEnable(GL_DEPTH_TEST);

    glPopMatrix();                // modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();                // projection
    glMatrixMode(GL_MODELVIEW);   // back to normal
}



void displayLevel2() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Set the camera for level 2
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    setupCameraLevel2();

    // 2. Draw the sky *behind everything* and do NOT write/use depth
    glDisable(GL_DEPTH_TEST);
    drawSky();
    glEnable(GL_DEPTH_TEST);

    // 3. Lights for level 2
    setupSunLight();

    // 4. World geometry
    drawNileRiver();

    if (winSceneActive && gameState_L2 == GAME_WON) {
        // End scene. show Level 1 buildings, player and Dr Beram only
        drawWinSceneBuildings();
    }
    else {
        drawFlyingCollectibles();
        drawBirds();
    }

    // 5. Characters
    drawDrBeram();
    drawPlayerLevel2();

    // 6. HUD
    glDisable(GL_LIGHTING);
    drawScoreLevel2();
    drawLivesDisplay();
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
    gluPerspective(60.0, aspect, 1.0, 3000.0);

    glMatrixMode(GL_MODELVIEW);
}

// Load models for Level 2 (like Level 1)
void loadModelsLevel2() {
    printf("Loading Level 2 models...\n");

    // Load player model (same as Level 1)
    playerModel_L2.Load("models/Player.3ds");
    printf("Player model loaded\n");

    // Apply player texture (same as Level 1)
    if (playerModel_L2.numMaterials > 0) {
        char playerTexPath[256];
        strcpy_s(playerTexPath, sizeof(playerTexPath), "textures/Ch24_1001_Diffuse.bmp");
        playerModel_L2.Materials[0].tex.Load(playerTexPath);
        playerModel_L2.Materials[0].textured = true;

        // Apply to all materials
        for (int i = 1; i < playerModel_L2.numMaterials; ++i) {
            playerModel_L2.Materials[i].tex.Load(playerTexPath);
            playerModel_L2.Materials[i].textured = true;
        }
    }

    // Load bird model with texture
    birdModel_L2.Load("models/Bird_Level2.3ds");
    printf("Bird model loaded\n");

    // Apply bird texture
    if (birdModel_L2.numMaterials > 0) {
        char birdTexPath[256];
        strcpy_s(birdTexPath, sizeof(birdTexPath), "textures/Bird_Level2.bmp");
        birdModel_L2.Materials[0].tex.Load(birdTexPath);
        birdModel_L2.Materials[0].textured = true;

        // Apply to all materials
        for (int i = 1; i < birdModel_L2.numMaterials; ++i) {
            birdModel_L2.Materials[i].tex.Load(birdTexPath);
            birdModel_L2.Materials[i].textured = true;
        }

    }

    // ========== LOAD DR. BERAM MODEL ==========
    drBeramModel.Load("models/Beram.3ds");
    printf("Dr. Beram model loaded: %d materials\n", drBeramModel.numMaterials);

    // Apply textures to Dr. Beram model if it has materials
    if (drBeramModel.numMaterials > 0) {
        // You can load specific textures for Dr. Beram if needed
        char beramTexPath[256];
        strcpy_s(beramTexPath, sizeof(beramTexPath), "textures/Ch24_1001_Diffuse.bmp");

        for (int i = 0; i < drBeramModel.numMaterials; i++) {
            drBeramModel.Materials[i].tex.Load(beramTexPath);
            drBeramModel.Materials[i].textured = true;
            printf("Applied texture to Dr. Beram material %d\n", i);
        }
    }

    // ========== LOAD COLLECTIBLE MODEL ==========
    collectibleModel_L2.Load("models/3enabeyat1.3ds");
    printf("Collectible model loaded: %d materials\n", collectibleModel_L2.numMaterials);

    // Apply your real textures to the 3enabeyat model
    if (collectibleModel_L2.numMaterials > 0) {
        // Load your first texture (3ennabeyat1.bmp)
        char texPath1[256];
        strcpy_s(texPath1, sizeof(texPath1), "textures/3ennabeyat1.bmp");

        // Load your second texture (3ennabeyat2.bmp)
        char texPath2[256];
        strcpy_s(texPath2, sizeof(texPath2), "textures/3ennabeyat2.bmp");

        printf("Loading collectible textures...\n");
        printf("Texture 1: %s\n", texPath1);
        printf("Texture 2: %s\n", texPath2);

        // Apply textures to materials
        for (int i = 0; i < collectibleModel_L2.numMaterials; i++) {
            if (i == 0) {
                // First material gets first texture
                collectibleModel_L2.Materials[i].tex.Load(texPath1);
                collectibleModel_L2.Materials[i].textured = true;
                printf("Applied texture 1 to material %d\n", i);
            }
            else if (i == 1 && collectibleModel_L2.numMaterials > 1) {
                // Second material gets second texture if it exists
                collectibleModel_L2.Materials[i].tex.Load(texPath2);
                collectibleModel_L2.Materials[i].textured = true;
                printf("Applied texture 2 to material %d\n", i);
            }
            else {
                // Any additional materials default to first texture
                collectibleModel_L2.Materials[i].tex.Load(texPath1);
                collectibleModel_L2.Materials[i].textured = true;
                printf("Applied texture 1 to material %d (default)\n", i);
            }
        }
    }

    // Load sky texture
    loadSkyTexture();

    // ========== LOAD RIVER TEXTURE ==========
    char riverTexturePath[256];
    strcpy_s(riverTexturePath, sizeof(riverTexturePath), "textures/River.bmp");
    riverTexture.Load(riverTexturePath);
    printf("River texture loaded\n");

    if (riverTexture.texture[0] != 0) {
        glBindTexture(GL_TEXTURE_2D, riverTexture.texture[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // ========== LOAD GROUND TEXTURES FOR LEVEL 2 ==========
    char grassTexturePath[256];
    strcpy_s(grassTexturePath, sizeof(grassTexturePath), "textures/Grass.bmp");
    grassTexture.Load(grassTexturePath);
    printf("Grass texture loaded\n");

    if (grassTexture.texture[0] != 0) {
        glBindTexture(GL_TEXTURE_2D, grassTexture.texture[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    char concrMetTexturePath[256];
    strcpy_s(concrMetTexturePath, sizeof(concrMetTexturePath), "textures/ConcrMet.bmp");
    concrMetTexture.Load(concrMetTexturePath);
    printf("Concrete texture loaded\n");

    if (concrMetTexture.texture[0] != 0) {
        glBindTexture(GL_TEXTURE_2D, concrMetTexture.texture[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // ========== LOAD BUILDING MODEL FOR WIN SCENE ==========
    buildingModel_L2.Load("models/cottage.3ds");
    printf("Win scene building model loaded\n");

    printf("All Level 2 models loaded successfully\n");
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

    // Load models
    loadModelsLevel2();

    setupLevel2();

    prevTimeMs_L2 = glutGet(GLUT_ELAPSED_TIME);
    gameState_L2 = GAME_PLAYING;
}

//#endif // RUN_LEVEL_2 - End of Level 2 code


// ===============================
// WELCOME SCREEN
// ===============================
void displayWelcome() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Plain background color
    glClearColor(0.0f, 0.0f, 0.1f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);

    // Title
    drawText2D(-0.35f, 0.7f, "EL RAGOL EL 3ENNAB");

    drawText2D(-0.5f, 0.5f, "Welcome");
    drawText2D(-0.6f, 0.4f, "Press P to start Level 1");

    // Level 1 instructions
    drawText2D(-0.9f, 0.2f, "Level 1. Street runner");
    drawText2D(-0.9f, 0.1f, "W,S  move forward and backward");
    drawText2D(-0.9f, 0.0f, "A,D  move left and right");
    drawText2D(-0.9f, -0.1f, "1,3  change camera");
    drawText2D(-0.9f, -0.2f, "Collect 3enab, avoid cars and trash cans");
    drawText2D(-0.9f, -0.3f, "Reach the checkpoint gate to go to Level 2");

    // Level 2 instructions
    drawText2D(-0.9f, -0.5f, "Level 2. Flying over the Nile");
    drawText2D(-0.9f, -0.6f, "A,D or Left,Right  move left and right");
    drawText2D(-0.9f, -0.7f, "W,S or Up,Down    move up and down");
    drawText2D(-0.9f, -0.8f, "Avoid birds, collect 3enab, rescue Dr. Beram");

    drawText2D(0.4f, -0.85f, "R. restart from Level 1");
    drawText2D(0.4f, -0.9f, "ESC. quit");


    glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

// ===============================
// MASTER GLUT CALLBACKS
// ===============================
void displayMaster() {
    if (currentScreen == SCREEN_WELCOME) {
        displayWelcome();
    }
    else if (currentScreen == SCREEN_LEVEL1) {
        display();
    }
    else if (currentScreen == SCREEN_LEVEL2) {
        displayLevel2();
    }
}

void idleMaster() {
    if (currentScreen == SCREEN_WELCOME) {
        // Just keep redrawing the welcome screen
        glutPostRedisplay();
        return;
    }

    if (currentScreen == SCREEN_LEVEL1) {
        idle();          // Level 1 logic
    }
    else if (currentScreen == SCREEN_LEVEL2) {
        idleLevel2();    // Level 2 logic
    }
}


void keyboardMaster(unsigned char key, int x, int y) {
    // Global restart: works from any screen
    if (key == 'r' || key == 'R') {
        // Go back to Level 1 from a clean state
        currentScreen = SCREEN_LEVEL1;

        // Reset Level 1 data
        setupLevel1();
        prevTimeMs = glutGet(GLUT_ELAPSED_TIME);
        gameStartTimeMs = prevTimeMs;
        remainingTimeMs = gameDurationMs;
        gameState = GAME_PLAYING;

        // Cancel any running transition
        transitionToLevel2 = false;
        transitionTimer = 0.0f;

        // Reset Level 2 runtime state if it was already created
        if (level2Initialized) {
            setupLevel2();
            prevTimeMs_L2 = glutGet(GLUT_ELAPSED_TIME);
            level2StartTimeMs = prevTimeMs_L2;
            remainingTime_L2 = level2DurationMs;
            gameState_L2 = GAME_PLAYING;
            winSceneActive = false;
        }

        // Clear key state arrays used by Level 2 controls
        for (int i = 0; i < 256; ++i) {
            keyStates[i] = false;
            specialKeyStates[i] = false;
        }

        glutPostRedisplay();
        return;
    }

    // Welcome screen logic
    if (currentScreen == SCREEN_WELCOME) {
        if (key == 'p' || key == 'P') {
            // Start Level 1 from fresh state
            currentScreen = SCREEN_LEVEL1;

            setupLevel1();                          // reset layout and objects
            prevTimeMs = glutGet(GLUT_ELAPSED_TIME);
            gameStartTimeMs = prevTimeMs;
            remainingTimeMs = gameDurationMs;
            gameState = GAME_PLAYING;

            glutPostRedisplay();
        }
        else if (key == 27) {
            exit(0);
        }
        return;
    }

    // In-game logic
    if (currentScreen == SCREEN_LEVEL1) {
        keyboardLevel1(key, x, y);
    }
    else if (currentScreen == SCREEN_LEVEL2) {
        keyboardLevel2(key, x, y);
    }
}



void keyboardUpMaster(unsigned char key, int x, int y) {
    if (currentScreen == SCREEN_LEVEL2) {
        keyboardUpLevel2(key, x, y);
    }
}


void specialKeysMaster(int key, int x, int y) {
    if (currentScreen == SCREEN_LEVEL2) {
        specialKeysLevel2(key, x, y);
    }
}


void specialUpMaster(int key, int x, int y) {
    if (currentScreen == SCREEN_LEVEL2) {
        specialUpLevel2(key, x, y);
    }
}

void reshapeMaster(int w, int h) {
    if (h == 0) h = 1;
    float aspect = (float)w / (float)h;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 3000.0);

    glMatrixMode(GL_MODELVIEW);
}

void mouseMaster(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON)
        return;

    if (state == GLUT_DOWN) {
        if (currentScreen == SCREEN_LEVEL1) {
            mouseDragging_L1 = true;
            lastMouseX_L1 = x;
            lastMouseY_L1 = y;
        }
        else if (currentScreen == SCREEN_LEVEL2) {
            mouseDragging_L2 = true;
            lastMouseX_L2 = x;
            lastMouseY_L2 = y;
        }
    }
    else if (state == GLUT_UP) {
        if (currentScreen == SCREEN_LEVEL1) {
            mouseDragging_L1 = false;
        }
        else if (currentScreen == SCREEN_LEVEL2) {
            mouseDragging_L2 = false;
        }
    }
}

void motionMaster(int x, int y) {
    bool changed = false;

    if (currentScreen == SCREEN_LEVEL1) {
        if (!mouseDragging_L1) return;

        int dx = x - lastMouseX_L1;
        int dy = y - lastMouseY_L1;
        lastMouseX_L1 = x;
        lastMouseY_L1 = y;

        cameraYaw_L1 += dx * MOUSE_SENSITIVITY;
        cameraPitch_L1 += dy * MOUSE_SENSITIVITY;

        if (cameraPitch_L1 > MAX_CAMERA_PITCH) cameraPitch_L1 = MAX_CAMERA_PITCH;
        if (cameraPitch_L1 < -MAX_CAMERA_PITCH) cameraPitch_L1 = -MAX_CAMERA_PITCH;

        changed = true;
    }
    else if (currentScreen == SCREEN_LEVEL2) {
        if (!mouseDragging_L2) return;

        int dx = x - lastMouseX_L2;
        int dy = y - lastMouseY_L2;
        lastMouseX_L2 = x;
        lastMouseY_L2 = y;

        cameraYaw_L2 += dx * MOUSE_SENSITIVITY;
        cameraPitch_L2 += dy * MOUSE_SENSITIVITY;

        if (cameraPitch_L2 > MAX_CAMERA_PITCH) cameraPitch_L2 = MAX_CAMERA_PITCH;
        if (cameraPitch_L2 < -MAX_CAMERA_PITCH) cameraPitch_L2 = -MAX_CAMERA_PITCH;

        changed = true;
    }

    if (changed) {
        glutPostRedisplay();
    }
}



// ===============================
// SINGLE MAIN FUNCTION (shared by both levels)
// ===============================

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(1024, 768);

    glutCreateWindow("El Ragol El 3ennab");

    // Initialise Level 1 GL and load its models.
    // Level 2 GL is loaded on-demand when we first switch to it.
    initGL();

    currentScreen = SCREEN_WELCOME;

    // Master callbacks
    glutDisplayFunc(displayMaster);
    glutIdleFunc(idleMaster);
    glutKeyboardFunc(keyboardMaster);
    glutKeyboardUpFunc(keyboardUpMaster);
    glutSpecialFunc(specialKeysMaster);
    glutSpecialUpFunc(specialUpMaster);
    glutReshapeFunc(reshapeMaster);
    // NEW: mouse camera
    glutMouseFunc(mouseMaster);
    glutMotionFunc(motionMaster);

    glutMainLoop();
    return 0;
}
