/*
Leveels

Earth venus mecury sun boss

ui start
ui gameplay
earth scene spacescraft enemy

venus scene spacescraft enemy

mercury scene spacescraft enemy

sun boss scene spacescraft enemy
*/


/*

In the code we wrote earlier, drawPolyCircle is a custom helper function (specifically, a C++ lambda function) that I created inside the menu drawing code to save space and keep things organized.
Since legacy OpenGL doesn't have a built-in glDrawCircle() function, you normally have to write a for loop with sin() and cos() every single time you want to draw a round shape.
Because the menu illustration required drawing dozens of circles (for the planets, craters, the rocket window, etc.), writing out the loop every time would have made the code massive and hard to read.
So, I created drawPolyCircle at the top of the function to act as a shortcut. Here is what it looked like:
code
C++
auto drawPolyCircle = [](float cx, float cy, float r, float colR, float colG, float colB, float alpha = 1.0f) {
    glColor4f(colR, colG, colB, alpha);
    glBegin(GL_POLYGON);
    for(int i = 0; i < 360; i += 10) {
        float a = i * PI / 180.0f;
        glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
    }
    glEnd();
};
What it does:
It takes the X and Y position (cx, cy).
It takes the Radius / Size (r).
It takes the RGB Colors (colR, colG, colB).
It sets the color and runs the standard GL_POLYGON math loop to draw the circle.
*/


#ifdef __APPLE__
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
#endif
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
const int WIN_W  = 900;
const int WIN_H  = 600;
const float UI_W = 100.0f;
const float GX0  = 101.0f;
const float GX1  = 900.0f;
const float PI   = 3.14159265f;
// Game State
enum GameState { MENU, PLAYING, LEVEL_COMPLETE, GAME_OVER, VICTORY };
GameState gameState = MENU;

enum Level { EARTH, VENUS, MERCURY, SUN_BOSS };
Level currentLevel = EARTH;
// Data Structures
struct Entity {
    float x, y;
    float vx, vy;
    float w, h;
    bool  active;
    int   health;
    int   type;
};
struct Bullet {
    float x, y;
    float vx, vy;
    bool  active;
    bool  isEnemy;
    float r, g, b;
};
struct Particle {
    float x, y, vx, vy;
    float life, maxLife;
    float r, g, b, size;
};
// --- Scenery Structures ---
struct Meteor {
    float x, y, size, speed, rot, rotSpeed;
    float offsets[8];
};
struct ParallaxStar {
    float x, y, speed, size, bright;
};
struct SpaceRock {
    float x, y, size;
    float points[12][2];
    float rot, rotSpd;
};
struct MercuryPoint {
    float x, y;
    float alpha;
};
// Game Data
Entity   player;
Entity   boss;
std::vector<Entity>   enemies;
std::vector<Bullet>   bullets;
std::vector<Particle> particles;

// Scenery Data
std::vector<Meteor>       meteors;
std::vector<ParallaxStar> pstars;
std::vector<SpaceRock>    spaceRocks;
std::vector<MercuryPoint> mercPoints;

int   score         = 0;
int   colonyHP      = 100;
int   pilotHP       = 100;
int   enemiesKilled = 0;
int   killsNeeded   = 5;
float spawnTimer    = 0.0f;
float spawnInterval = 2.0f;
float gameTime      = 0.0f;
float bulletTimer   = 0.0f;
float droneFireT    = 0.0f;
float bossFireT     = 0.0f;
float levelTransitionTimer = 0.0f;
float warnFlash     = 0.0f;

bool  keys[256]  = {false};
bool  skeys[256] = {false};
// Utility
// --- MATH & COLLISION ---
inline float rnd(float lo, float hi) {
    return lo + (hi - lo) * (rand() / (float)RAND_MAX); }
inline float pseudoRandom(int i)     {
    return (i * 9301 + 49297) % 233280 / 233280.0f; }
inline bool rectHit(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx+bw && ax+aw > bx && ay < by+bh && ay+ah > by;
}

// --- EXPLOSIONS ---
void spawnParticles(float x, float y, int n, float r, float g, float b) {
    for (int i = 0; i < n; i++) {
        float a = rnd(0, 2*PI), s = rnd(0.5f, 4.5f), life = rnd(0.3f, 1.4f);
        // Pushes all particle data (x, y, vx, vy, life, maxLife, r, g, b, size) in one line
        particles.push_back({x, y, cosf(a)*s, sinf(a)*s, life, life, r, g, b, rnd(2.f, 6.f)});
    }
}

// --- BACKGROUND DATA SETUP ---
void initScenery() {
    meteors.clear(); pstars.clear(); spaceRocks.clear(); mercPoints.clear();

    for (int i = 0; i < 25; i++) {
        Meteor m = {
            rnd(-2.f, 2.f),
            rnd(-1.f, 1.f),
            rnd(0.03f, 0.08f),
            rnd(0.5f, 1.0f),
            rnd(0, 360),
            rnd(-1.5f, 1.5f)};
        for (int j = 0; j < 8; j++) m.offsets[j] = rnd(0.7f, 1.3f);
        meteors.push_back(m);
    }

    for (int i = 0; i < 400; i++) {
        float sp = rnd(0.2f, 1.2f);
        pstars.push_back({
                         rnd(0, WIN_W),
                         rnd(0, WIN_H),
                         sp, sp * 1.5f,
                         rnd(0.3f, 1.0f)});
    }

    for (int i = 0; i < 80; i++) {
        SpaceRock r = {
            rnd(0, WIN_W-300),
            rnd(0, WIN_H),
            rnd(20, 75)};
        r.rot = rnd(0, 360); r.rotSpd = rnd(-1.5f, 1.5f);
        for (int j = 0; j < 12; j++) {
            float a = j * 30.f * PI / 180.f, d = r.size * rnd(0.5f, 1.3f);
            r.points[j][0] = d * cosf(a); r.points[j][1] = d * sinf(a);
        }
        spaceRocks.push_back(r);
    }

    for (int i = 0; i < 2500; i++) {
        float a =
        rnd(0, 360) * PI / 180.f,
        r_val = sqrtf(rnd(0, 1.f)) * 0.432f;
        if (cosf(a + PI) > -0.3f) mercPoints.push_back({
                                                       r_val * cosf(a),
                                                       r_val * sinf(a),
                                                       rnd(0, 0.12f) * 0.4f
                                                       });
    }
}
// Level Info Helpers
std::string getLevelName() {
    switch(currentLevel) {
        case EARTH:    return "EARTH SKIES";
        case VENUS:    return "VENUS ASSAULT";
        case MERCURY:  return "MERCURY STRIKE";
        case SUN_BOSS: return "SUN TITAN BOSS";
        default:       return "UNKNOWN";
    }
}
void getLevelColors(float& r, float& g, float& b) {
    switch(currentLevel) {
        case EARTH:
            r = 0.3f; g = 0.7f; b = 1.0f;
            //r = 1.0f; g = 0.0f; b = 0.0f;
            break;
        case VENUS:
            r = 0.9f; g = 0.7f; b = 0.3f;
            break;
        case MERCURY:
            r = 0.6f; g = 0.6f; b = 0.65f;
            break;
        case SUN_BOSS:
            r = 1.0f; g = 0.3f; b = 0.0f;
            break;
    }
}

// Draw: Player Ship
void drawPlayer(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(1.5f, 1.5f, 1.0f);

    // Main hull
    glColor3f(0.1f, 0.05f, 0.15f);
    glBegin(GL_POLYGON);
        glVertex2f( 30,  0);
        glVertex2f( 18,  6);
        glVertex2f(  8, 10);
        glVertex2f( -5, 12);
        glVertex2f(-18,  8);
        glVertex2f(-24,  0);
        glVertex2f(-18, -8);
        glVertex2f( -5,-12);
        glVertex2f(  8,-10);
        glVertex2f( 18, -6);
    glEnd();

    // Armor spikes
    glColor3f(0.25f, 0.05f, 0.05f);
    glBegin(GL_TRIANGLES);
        glVertex2f( 10, 10);
        glVertex2f(  8, 18);
        glVertex2f(  3, 11);
        glVertex2f(  0, 12);
        glVertex2f( -4, 22);
        glVertex2f( -6, 11);
        glVertex2f(-10, 12);
        glVertex2f(-15, 20);
        glVertex2f(-14,  9);
        glVertex2f( 10,-10);
        glVertex2f(  8,-18);
        glVertex2f(  3,-11);
        glVertex2f(  0,-12);
        glVertex2f( -4,-22);
        glVertex2f( -6,-11);
        glVertex2f(-10,-12);
        glVertex2f(-15,-20);
        glVertex2f(-14, -9);
    glEnd();

    // Top wing
    glColor3f(0.35f, 0.05f, 0.05f);
    glBegin(GL_QUADS);
        glVertex2f( -5, 10);
        glVertex2f(-20, 28);
        glVertex2f(-28, 20);
        glVertex2f(-12,  9);
        glVertex2f(-12,  9);
        glVertex2f(-22, 18);
        glVertex2f(-26, 12);
        glVertex2f(-16,  8);
    glEnd();

    // Bottom wing
    glBegin(GL_QUADS);
        glVertex2f( -5,-10);
        glVertex2f(-20,-28);
        glVertex2f(-28,-20);
        glVertex2f(-12, -9);
        glVertex2f(-12, -9);
        glVertex2f(-22,-18);
        glVertex2f(-26,-12);
        glVertex2f(-16, -8);
    glEnd();

    // Mandibles
    glColor3f(0.45f, 0.05f, 0.05f);
    glBegin(GL_TRIANGLES);
        glVertex2f(22,  3);
        glVertex2f(34, 12);
        glVertex2f(28,  2);
        glVertex2f(22, -3);
        glVertex2f(34,-12);
        glVertex2f(28, -2);
        glVertex2f(18,  4);
        glVertex2f(26,  8);
        glVertex2f(20,  3);
        glVertex2f(18, -4);
        glVertex2f(26, -8);
        glVertex2f(20, -3);
    glEnd();

    // Cockpit / evil eye
    glColor3f(1.0f, 0.1f, 0.0f);
    glBegin(GL_TRIANGLES);
        glVertex2f(18,  0);
        glVertex2f( 8,  5);
        glVertex2f( 8, -5);
    glEnd();
    glColor3f(1.0f, 0.6f, 0.1f);
    glBegin(GL_TRIANGLES);
        glVertex2f(15,  0);
        glVertex2f(10,  2);
        glVertex2f(10, -2);
    glEnd();

    // Engine exhaust
    glColor3f(0.8f, 0.2f, 0.0f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-22,  6);
        glVertex2f(-38,  2);
        glVertex2f(-22, -6);
    glEnd();
    glColor3f(1.0f, 0.4f, 0.0f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-22,  4);
        glVertex2f(-34,  0);
        glVertex2f(-22, -4);
    glEnd();
    glColor3f(1.0f, 0.9f, 0.6f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-22,  2);
        glVertex2f(-28,  0);
        glVertex2f(-22, -2);
    glEnd();

    // Fire aura
    float fireAngles[] = {0,45,90,135,180,225,270,315};
    float radius = 38.0f;
    for (int i = 0; i < 8; i++) {
        float rad = fireAngles[i] * PI / 180.0f;
        float cx = cosf(rad)*radius, cy = sinf(rad)*radius;
        float dx = cosf(rad),        dy = sinf(rad);

        glColor3f(1.0f, 0.4f, 0.0f);
        glBegin(GL_TRIANGLES);
            glVertex2f(cx, cy);
            glVertex2f(cx+dx*15.0f, cy+dy*8.0f);
            glVertex2f(cx+dx*12.0f-dy*5.0f, cy+dy*6.0f+dx*5.0f);
            glVertex2f(cx+dx*12.0f+dy*5.0f, cy+dy*6.0f-dx*5.0f);
        glEnd();
        glColor3f(1.0f, 0.8f, 0.2f);
        glBegin(GL_TRIANGLES);
            glVertex2f(cx, cy);
            glVertex2f(cx+dx*10.0f, cy+dy*5.0f);
            glVertex2f(cx+dx*8.0f-dy*3.0f, cy+dy*4.0f+dx*3.0f);
            glVertex2f(cx+dx*8.0f+dy*3.0f, cy+dy*4.0f-dx*3.0f);
        glEnd();
    }

    glPopMatrix();
}


// Draw: Modern Player Ship (Level 2+) venus
void drawModernPlayer(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(1.5f, 1.5f, 1.0f);

    // Engine exhaust (Pulse effect using global gameTime)
    float pulse = 0.8f + 0.2f * sinf(gameTime * 25.0f);

    // Blue plasma trails
    glColor3f(0.0f, 0.6f * pulse, 1.0f);
    glBegin(GL_TRIANGLES);
        // Top thruster
        glVertex2f(-26,  4);
        glVertex2f(-45 - 6*pulse,  4);
        glVertex2f(-26,  1);
        // Bottom thruster
        glVertex2f(-26, -1);
        glVertex2f(-45 - 6*pulse, -4);
        glVertex2f(-26, -4);
    glEnd();

    // Core white-hot exhaust
    glColor3f(0.8f, 0.9f, 1.0f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-26,  3.5f);
        glVertex2f(-35,  2.5f);
        glVertex2f(-26,  1.5f);
        glVertex2f(-26, -1.5f);
        glVertex2f(-35, -2.5f);
        glVertex2f(-26, -3.5f);
    glEnd();

    // Main Fuselage (Sleek White/Silver Aerospace body)
    glColor3f(0.85f, 0.88f, 0.90f);
    glBegin(GL_POLYGON);
        glVertex2f( 32,  0);
        glVertex2f( 15,  7);
        glVertex2f(-12,  8);
        glVertex2f(-24,  5);
        glVertex2f(-24, -5);
        glVertex2f(-12, -8);
        glVertex2f( 15, -7);
    glEnd();

    // Dark grey central armor stripe
    glColor3f(0.25f, 0.28f, 0.32f);
    glBegin(GL_POLYGON);
        glVertex2f( 12,  0);
        glVertex2f( -5,  4);
        glVertex2f(-18,  5);
        glVertex2f(-18, -5);
        glVertex2f( -5, -4);
    glEnd();

    // Swept-back aerodynamic wings
    glColor3f(0.7f, 0.75f, 0.8f);
    glBegin(GL_POLYGON); // Top wing
        glVertex2f( -2,  6);
        glVertex2f(-12, 24);
        glVertex2f(-22, 24);
        glVertex2f(-16,  7);
    glEnd();
    glBegin(GL_POLYGON); // Bottom wing
        glVertex2f( -2, -6);
        glVertex2f(-12,-24);
        glVertex2f(-22,-24);
        glVertex2f(-16, -7);
    glEnd();

    // Neon Cyan Wing Accents (Glowing tech edges)
    glColor3f(0.0f, 0.8f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
        glVertex2f(-5, 9);
        glVertex2f(-13, 23);
        glVertex2f(-21, 23);
    glEnd();
    glBegin(GL_LINE_STRIP);
        glVertex2f(-5, -9);
        glVertex2f(-13, -23);
        glVertex2f(-21, -23);
    glEnd();
    glLineWidth(1.0f);

    // Advanced Cockpit Canopy (Cyan Glass)
    glColor3f(0.0f, 0.7f, 0.9f);
    glBegin(GL_POLYGON);
        glVertex2f( 24,  0);
        glVertex2f( 12,  4);
        glVertex2f(  2,  4);
        glVertex2f(  2, -4);
        glVertex2f( 12, -4);
    glEnd();

    // Cockpit Inner Highlight (Glass glare)
    glColor3f(0.6f, 0.9f, 1.0f);
    glBegin(GL_POLYGON);
        glVertex2f( 20,  0);
        glVertex2f( 10,  2);
        glVertex2f(  4,  2);
        glVertex2f(  4, -2);
        glVertex2f( 10, -2);
    glEnd();

    // Engine housing / Thruster nozzles
    glColor3f(0.15f, 0.15f, 0.18f);
    glBegin(GL_QUADS);
        glVertex2f(-24,  6);
        glVertex2f(-28,  5);
        glVertex2f(-28,  1);
        glVertex2f(-24,  2);

        glVertex2f(-24, -2);
        glVertex2f(-28, -1);
        glVertex2f(-28, -5);
        glVertex2f(-24, -6);
    glEnd();

    glPopMatrix();
}
// Scenery Drawing Helpers
void pushBG(float minX, float maxX, float minY, float maxY) {
    glPushMatrix();
    float scaleX = (WIN_W - GX0) / (maxX - minX);
    float scaleY = WIN_H / (maxY - minY);
    glTranslatef(GX0 - minX * scaleX, -minY * scaleY, 0);
    glScalef(scaleX, scaleY, 1.0f);
}
void popBG() {
    glPopMatrix();
}
void drawCircleBG(float x, float y, float rx, float ry, int segments = 30) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < segments; i++) {
        float theta = 2.0f * PI * float(i) / float(segments);
        glVertex2f(x + rx * cosf(theta), y + ry * sinf(theta));
    }
    glEnd();
}
void drawRectBG(float x, float y, float w, float h, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}
void drawTrapezoidBG(float x, float y, float bottomW, float topW, float h, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
    glVertex2f(x - bottomW/2, y);
    glVertex2f(x + bottomW/2, y);
    glVertex2f(x + topW/2, y + h);
    glVertex2f(x - topW/2, y + h);
    glEnd();
}
void drawTreeBG(float x, float y, float r, float g, float b) {
    drawRectBG(x - 0.01f, y, 0.02f, 0.1f, 0.4f, 0.25f, 0.1f);
    glColor3f(r, g, b);
    drawCircleBG(x, y + 0.12f, 0.07f, 0.09f);
}
void drawCloudBG(float x, float y) {
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircleBG(x, y, 0.08f, 0.05f);
    drawCircleBG(x + 0.07f, y + 0.02f, 0.09f, 0.06f);
    drawCircleBG(x + 0.14f, y, 0.08f, 0.05f);
}

void drawEarthBackground() {
    pushBG(-1.0f, 1.0f, -1.0f, 1.0f);

    // Sky
    drawRectBG(-1.0f, -1.0f, 2.0f, 2.0f, 0.52f, 0.8f, 0.98f);

    float scrollSky = fmodf(gameTime * 0.05f, 2.0f);
    float scrollBG  = fmodf(gameTime * 0.1f, 2.0f);
    float scrollMG  = fmodf(gameTime * 0.2f, 2.0f);
    float scrollFG  = fmodf(gameTime * 0.4f, 2.0f);

    auto drawEarthSkyAndClouds = [&](float offsetX) {
        glPushMatrix();
        glTranslatef(offsetX, 0, 0);
        drawCloudBG(-0.7f, 0.7f);
        drawCloudBG(0.2f, 0.8f);
        drawCloudBG(0.6f, 0.65f);




        glColor3f(0.0f, 0.0f, 0.0f); //birds color
        //glColor3f(0.2f, 0.0f, 0.0f);

        glLineWidth(2.0f); //bird
        for(float i=0; i<3; i++) {
            glBegin(GL_LINE_STRIP);
            glVertex2f(0.7f + i*0.05f, 0.8f + i*0.02f);
            glVertex2f(0.72f + i*0.05f, 0.78f + i*0.02f);
            glVertex2f(0.74f + i*0.05f, 0.8f + i*0.02f);
            glEnd();
        }
        glLineWidth(1.0f);
        glPopMatrix();
    };

    auto drawEarthBGBuildings = [&](float offsetX) {
        glPushMatrix();
        glTranslatef(offsetX, 0, 0);
        drawRectBG(-1.0f, -0.2f, 0.15f, 0.6f, 0.65f, 0.82f, 0.88f);
        drawRectBG(-0.7f, -0.2f, 0.12f, 0.75f, 0.65f, 0.82f, 0.88f);
        drawRectBG(0.3f, -0.2f, 0.18f, 0.65f, 0.65f, 0.82f, 0.88f);
        drawRectBG(0.8f, -0.2f, 0.2f, 0.55f, 0.65f, 0.82f, 0.88f);
        glPopMatrix();
    };

    auto drawEarthMGBuildings = [&](float offsetX) {
        glPushMatrix();
        glTranslatef(offsetX, 0, 0);
        drawRectBG(-0.85f, -0.2f, 0.2f, 0.5f, 0.45f, 0.68f, 0.75f);
        drawRectBG(-0.4f, -0.2f, 0.25f, 0.8f, 0.45f, 0.68f, 0.75f);
        drawRectBG(0.0f, -0.2f, 0.2f, 0.6f, 0.45f, 0.68f, 0.75f);
        drawRectBG(0.55f, -0.2f, 0.15f, 0.7f, 0.45f, 0.68f, 0.75f);
        glPopMatrix();
    };

    auto drawEarthFG = [&](float offsetX) {
        glPushMatrix();
        glTranslatef(offsetX, 0, 0);
        drawRectBG(-0.95f, -0.6f, 0.35f, 0.4f, 0.96f, 0.81f, 0.32f);
        drawTrapezoidBG(-0.775f, -0.2f, 0.4f, 0.3f, 0.1f, 0.9f, 0.35f, 0.15f);
        drawRectBG(-0.55f, -0.6f, 0.3f, 0.35f, 0.98f, 0.95f, 0.8f);
        drawTrapezoidBG(-0.4f, -0.25f, 0.35f, 0.2f, 0.12f, 0.9f, 0.5f, 0.2f);
        drawRectBG(-0.15f, -0.6f, 0.35f, 0.3f, 0.6f, 0.45f, 0.35f);
        drawTrapezoidBG(0.025f, -0.3f, 0.4f, 0.35f, 0.1f, 0.5f, 0.2f, 0.1f);
        drawRectBG(0.35f, -0.6f, 0.35f, 0.45f, 0.96f, 0.81f, 0.32f);
        drawTrapezoidBG(0.525f, -0.15f, 0.4f, 0.3f, 0.1f, 0.9f, 0.35f, 0.15f);

        drawRectBG(-1.0f, -0.7f, 2.0f, 0.1f, 0.45f, 0.8f, 0.4f);
        drawRectBG(-1.0f, -1.0f, 2.0f, 0.3f, 0.3f, 0.3f, 0.3f);
        for(float x = -1.0f; x < 1.0f; x += 0.3f)
            drawRectBG(x, -0.86f, 0.15f, 0.02f, 1.0f, 1.0f, 1.0f);

        drawTreeBG(-0.85f, -0.7f, 0.2f, 0.6f, 0.2f);
        drawTreeBG(-0.4f, -0.7f, 0.9f, 0.4f, 0.2f);
        drawTreeBG(0.1f, -0.7f, 0.9f, 0.8f, 0.2f);
        drawTreeBG(0.6f, -0.7f, 0.8f, 0.2f, 0.2f);
        drawTreeBG(0.9f, -0.7f, 0.2f, 0.6f, 0.2f);
        glPopMatrix();
    };
//left
    drawEarthSkyAndClouds(-scrollSky); drawEarthSkyAndClouds(-scrollSky + 2.0f);
    drawEarthBGBuildings(-scrollBG);   drawEarthBGBuildings(-scrollBG + 2.0f);
    drawEarthMGBuildings(-scrollMG);   drawEarthMGBuildings(-scrollMG + 2.0f);
    drawEarthFG(-scrollFG);            drawEarthFG(-scrollFG + 2.0f);

    popBG();
}

// Draw: Venus Background (Atmospheric Vector Landscape - FLICKER FIXED)
void drawVenusBackground() {
    float t = gameTime;

    // Helper: Draw simple solid circle
    auto drawCircle = [](float cx, float cy, float r, float colR, float colG, float colB, float alpha=1.0f) {
        glColor4f(colR, colG, colB, alpha);
        glBegin(GL_POLYGON);
        for(int i = 0; i < 360; i += 10) {
            float a = i * PI / 180.0f;
            glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
        }
        glEnd();
    };

    // Helper: Draw wavy dunes with vertical gradient (FADE EFFECT)
    auto drawDune = [](float baseY, float amp, float freq, float speed, float time,
                       float topR, float topG, float topB,
                       float botR, float botG, float botB) {
        glBegin(GL_QUAD_STRIP);
        for (float x = GX0 - 20.0f; x <= WIN_W + 40.0f; x += 20.0f) {
                // Extra margin to prevent edge flicker
            float y = baseY + sinf(x * freq + time * speed) * amp;
            glColor3f(topR, topG, topB);
        // Top vertex (bright)
            glVertex2f(x, y);
            glColor3f(botR, botG, botB);
            // Bottom vertex (fades into shadow)
            glVertex2f(x, 0);
        }
        glEnd();
    };

    // Helper: Draw jagged mountain with lit side and shadow side
    auto drawMountain = [](float x, float y, float w, float h, float peakRatio) {
        // Shadow Side (Right) - Deep Dark Purple
        glColor3f(0.07f, 0.03f, 0.15f);
        glBegin(GL_TRIANGLES);
            glVertex2f(x, y);
            glVertex2f(x + w * peakRatio, y + h);
            glVertex2f(x + w * 0.6f, y);
        glEnd();

        // Lit Side (Left) - Vibrant Magenta/Pink
        glColor3f(0.85f, 0.16f, 0.41f);
        glBegin(GL_TRIANGLES);
            glVertex2f(x - w * 0.5f, y);
            glVertex2f(x + w * peakRatio, y + h);
            glVertex2f(x, y);
        glEnd();

        // Extra jagged shadow overlay for vector style
        glColor3f(0.35f, 0.05f, 0.25f);
        glBegin(GL_TRIANGLES);
            glVertex2f(x - w * 0.1f, y);
            glVertex2f(x + w * peakRatio, y + h);
            glVertex2f(x + w * 0.15f, y + h * 0.3f);
        glEnd();
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Sky Gradient (Fades from Deep Purple top to Magenta bottom)
    glBegin(GL_QUADS);
        glColor3f(0.19f, 0.08f, 0.25f);
        // Top Dark Purple
        glVertex2f(GX0, WIN_H);
        glVertex2f(WIN_W, WIN_H);
        glColor3f(0.55f, 0.12f, 0.35f);
        // Bottom Magenta
        glVertex2f(WIN_W, 0);
        glVertex2f(GX0, 0);
    glEnd();

    // 2. Wavy Sky/Nebula Clouds
    glColor4f(0.25f, 0.10f, 0.30f, 0.6f);
    glBegin(GL_POLYGON);
        glVertex2f(GX0, WIN_H);
        for(float x = GX0; x <= WIN_W; x += 20)
            glVertex2f(x, WIN_H - 80 + sinf(x*0.005f + t*0.2f)*40);
        glVertex2f(WIN_W, WIN_H);
    glEnd();

    // 3. Stars (Moving Left seamlessly)
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for(int i = 0; i < 20; i++) {
        // Start from Right edge (WIN_W) and perfectly wrap back around without jumping
        float sx = WIN_W - fmodf(i * 87.0f + t * 5.0f, WIN_W - GX0);
        float sy = 250.0f + fmodf(i * 123.0f, WIN_H - 250.0f);
        float pulse = 0.5f + 0.5f * sinf(t * 3.0f + i);
        glColor4f(1.0f, 0.8f, 0.9f, pulse);
        glVertex2f(sx, sy);
    }
    glEnd();

    // 4. Comets (Falling diagonally)
    for(int i = 0; i < 3; i++) {
        float cx = WIN_W - fmodf(t * (100.0f + i*20.0f) + i*300.0f, WIN_W * 1.5f);
        float cy = WIN_H - fmodf(t * (100.0f + i*20.0f) + i*300.0f, WIN_H * 1.5f);
        if (cx > GX0 && cy > 0) {
            glPushMatrix();
            glTranslatef(cx, cy, 0);
            glRotatef(-45, 0, 0, 1); // Diagonal tilt
            glBegin(GL_POLYGON);
                glColor4f(1.0f, 0.8f, 0.8f, 1.0f);
                glVertex2f(0, 2);
                glColor4f(1.0f, 0.8f, 0.8f, 1.0f);
                glVertex2f(10, 0);
                glColor4f(1.0f, 0.8f, 0.8f, 1.0f);
                glVertex2f(0, -2);
                glColor4f(1.0f, 0.5f, 0.6f, 0.0f);
                glVertex2f(-80, 0); // Tail fade
            glEnd();
            glPopMatrix();
        }
    }

    // 5. Giant Ringed Planet (Top Left)
    float pX = GX0 + 150.0f, pY = WIN_H - 120.0f, pR = 80.0f;
    // Outer Glow
    drawCircle(pX, pY, pR * 1.4f, 0.85f, 0.4f, 0.6f, 0.15f);
    // Planet Base
    drawCircle(pX, pY, pR, 0.85f, 0.4f, 0.6f, 0.9f);
    // Planet Shadow (Crescent shape)
    glColor4f(0.15f, 0.05f, 0.2f, 0.7f);
    glBegin(GL_POLYGON);
        for(int i = 45; i <= 225; i += 10)
            glVertex2f(pX + cosf(i*PI/180.0f)*pR, pY + sinf(i*PI/180.0f)*pR);
        for(int i = 225; i >= 45; i -= 10)
        glVertex2f(pX + 20 + cosf(i*PI/180.0f)*pR*0.8f, pY - 20 + sinf(i*PI/180.0f)*pR*0.8f);
    glEnd();

    // Planet Ring
    glPushMatrix();
    glTranslatef(pX, pY, 0);
    glRotatef(20, 0, 0, 1);
    glColor4f(0.9f, 0.6f, 0.7f, 0.8f);
    glLineWidth(4.0f);
    glBegin(GL_LINE_LOOP);
        for(int i = 0; i < 360; i += 10)
            glVertex2f(cosf(i*PI/180.0f)*160, sinf(i*PI/180.0f)*25);
    glEnd();
    glLineWidth(1.0f);
    glPopMatrix();

    // Small Moons (Top Right)
    drawCircle(WIN_W - 100, WIN_H - 80, 25, 0.7f, 0.2f, 0.4f);
    drawCircle(WIN_W - 220, WIN_H - 60, 15, 0.6f, 0.15f, 0.35f);

    // 6. Jagged Mountains (Parallax Scrolling - FLICKER FIXED)
    // The fmod loop MUST be an exact multiple of the distance between mountains.
    float scrollSlow = fmodf(t * 15.0f, 300.0f);
    // Spacing is 300, modulo is 300
    float scrollFast = fmodf(t * 30.0f, 400.0f);
    // Spacing is 400, modulo is 400

    // Background Mountains (Darker, slower)
    // We start at i = -2 to make sure it spawns safely off-screen to the left
    for(int i = -2; i < 5; i++) {
        drawMountain(GX0 + i*300.0f - scrollSlow, 100.0f, 350.0f, 250.0f, 0.2f);
    }

    // Foreground Mountains (Brighter, faster)
    for(int i = -2; i < 4; i++) {
        drawMountain(GX0 + i*400.0f - scrollFast, 60.0f, 450.0f, 350.0f, -0.1f);
    }

    // 7. Wavy Sand Dunes (Fades into pitch black/purple at the bottom)
    drawDune(80.0f, 25.0f, 0.015f, 1.5f, t,
             0.55f, 0.05f, 0.35f,
             // Top Bright Purple
             0.07f, 0.03f, 0.15f);
             // Bottom Pitch Purple

    drawDune(45.0f, 15.0f, 0.02f, 2.5f, t,
             0.80f, 0.10f, 0.40f,
             // Top Bright Magenta/Pink
             0.05f, 0.02f, 0.10f);
             // Bottom Dark

    drawDune(15.0f, 10.0f, 0.012f, 3.5f, t,
             0.95f, 0.20f, 0.50f,
             // Top Glowing Pink
             0.02f, 0.01f, 0.05f);
             // Bottom Near Black

    glDisable(GL_BLEND);
}

void drawRockyMeteor(Meteor& m) {
    glPushMatrix();
    glTranslatef(m.x, m.y, 0);
    glRotatef(m.rot, 0, 0, 1);
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.22f, 0.2f, 0.18f);
    glVertex2f(0, 0);
    for (int i = 0; i <= 8; i++) {
        int idx = i % 8;
        float angle = i * (360.0f / 8.0f) * PI / 180.0f;
        float lightEffect = cosf(angle + m.rot * 0.0174f + 3.14f);
        float shade = 0.12f + (lightEffect > 0 ? 0.12f : 0.0f);
        glColor3f(shade, shade * 0.95f, shade * 0.9f);
        float r = m.size * m.offsets[idx];
        glVertex2f(cosf(angle) * r, sinf(angle) * r);
    }
    glEnd();
    glPopMatrix();
}

void drawMatteMercury(float radius) {
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= 360; i += 2) {
        float a = i * PI / 180.0f;
        float dot = cosf(a + PI);
        float light = 0.1f + 0.5f * powf((dot + 1.0f) / 2.0f, 1.5f);
        glColor3f(light * 0.35f, light * 0.33f, light * 0.3f);
        glVertex2f(radius * cosf(a), radius * sinf(a));
    }
    glEnd();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(1.3f);
    glBegin(GL_POINTS);
    for (auto& p : mercPoints) {
        glColor4f(0.0f, 0.0f, 0.0f, p.alpha);
        glVertex2f(p.x, p.y);
    }
    glEnd();
    glDisable(GL_BLEND);
}

void drawSolidCircleBG(float x, float y, float r, float rCol, float gCol, float bCol) {
    glColor3f(rCol, gCol, bCol);
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= 360; i += 10)
        glVertex2f(x + r * cosf(i*PI/180.f), y + r * sinf(i*PI/180.f));
    glEnd();
}

void drawSettingSunGlow() {
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glBegin(GL_QUADS);
    // Base glow
    glColor4f(0.4f, 0.15f, 0.05f, 0.25f);
    glVertex2f(-1.5f, 1.0f);
    glVertex2f(-1.5f, -1.0f);
    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    glVertex2f(0.5f, -1.0f);
    glVertex2f(0.5f, 1.0f);
    // Animated waves
    for(int i = 0; i < 5; i++) {
        float w = 0.8f + sinf(gameTime + i) * 0.1f;
        glColor4f(0.6f, 0.2f, 0.05f, 0.04f);
        glVertex2f(-1.5f, 1.0f);
        glVertex2f(-1.5f, -1.0f);
        glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
        glVertex2f(w, -1.0f);
        glVertex2f(w, 1.0f);
    }
    glEnd();

    // Sun Half-Circle
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 0.4f, 0.1f, 0.15f);
    glVertex2f(-1.6f, 0.0f);
    for(int i = -90; i <= 90; i += 10) {
        glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
        glVertex2f(-1.6f + 2.0f * cosf(i*PI/180.f), 2.0f * sinf(i*PI/180.f));
    }
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDisable(GL_BLEND);
}

void drawMercuryBackground() {
    float t = gameTime;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Sky & Stars
    glBegin(GL_QUADS);
    glColor3f(0.02f, 0.08f, 0.15f);
    glVertex2f(GX0, WIN_H);
    glVertex2f(WIN_W, WIN_H);
    glColor3f(0.10f, 0.25f, 0.35f);
    glVertex2f(WIN_W, 0);
    glVertex2f(GX0, 0);
    glEnd();

    glPointSize(1.5f); glBegin(GL_POINTS);
    for(int i = 0; i < 60; i++) {
        glColor4f(0.8f, 0.9f, 1.0f, 0.3f + 0.7f * sinf(t * 2.f + i));
        glVertex2f(GX0 + pseudoRandom(i)*(WIN_W-GX0), WIN_H*0.3f + pseudoRandom(i+100)*(WIN_H*0.7f));
    }
    glEnd();

    // Helpers
    auto glow = [](float cx, float cy, float r, float R, float G, float B, float A) {
        glColor4f(R,G,B,A);
        glBegin(GL_POLYGON);
        for(int i=0; i<=360; i+=15)
            glVertex2f(cx+cosf(i*PI/180.f)*r, cy+sinf(i*PI/180.f)*r); glEnd();
    };
    auto dune = [](float Y, float amp, float frq, float ph, float tR, float tG, float tB, float bR, float bG, float bB) {
        glBegin(GL_QUAD_STRIP);
        for(float x=GX0-20; x<=WIN_W+40; x+=20) {
                glColor3f(tR,tG,tB);
        glVertex2f(x, Y+sinf(x*frq+ph)*amp);
        glColor3f(bR,bG,bB); glVertex2f(x, 0); }
        glEnd();
    };

    // 2. Bright Star / Lens Flare
    float sX = GX0 + 220.f, sY = WIN_H - 120.f;
    glow(sX, sY, 60.f, 0.4f, 0.8f, 1.0f, 0.15f);
    glow(sX, sY, 20.f, 0.8f, 0.95f, 1.0f, 0.6f);
    glow(sX, sY, 5.f, 1.0f, 1.0f, 1.0f, 1.0f);

    glPushMatrix();
    glTranslatef(sX, sY, 0);
    glRotatef(-30, 0, 0, 1);
    glColor4f(0.6f, 0.9f, 1.0f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(-250,-1.5f);
    glVertex2f(250,-1.5f);
    glVertex2f(250,1.5f);
    glVertex2f(-250,1.5f);
    glEnd();
    glRotatef(90, 0, 0, 1);
    glColor4f(0.6f, 0.9f, 1.0f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(-120,-1.f);
    glVertex2f(120,-1.f);
    glVertex2f(120,1.f);
    glVertex2f(-120,1.f);
    glEnd();
    glPopMatrix();

    // 3. Giant Planet & Combined Rings
    float pX = GX0 + 320.f, pY = WIN_H/2 + 30.f, pR = 120.f;
    auto rings = [&](bool f) {
        glPushMatrix();
        glTranslatef(pX, pY, 0);
        glRotatef(-35, 0, 0, 1);
        int s = f?180:0, e = f?360:180;
        glLineWidth(2.f);
        glColor4f(f?0.6f:0.5f,
                  f?0.8f:0.7f,
                  f?1.0f:0.9f,
                  f?0.7f:0.4f);
        glBegin(GL_LINE_LOOP);
        for(int i=s; i<=e; i+=5)
            glVertex2f(cosf(i*PI/180.f)*240, sinf(i*PI/180.f)*40);
        glEnd();
        glBegin(GL_LINE_LOOP);
        for(int i=s; i<=e; i+=5)
            glVertex2f(cosf(i*PI/180.f)*260, sinf(i*PI/180.f)*45);
        glEnd();
        glColor4f(f?0.5f:0.6f, 0.8f, 1.0f, f?0.4f:0.25f);
        glBegin(GL_POLYGON);
        for(int i=s; i<=e; i+=10)
            glVertex2f(cosf(i*PI/180.f)*280, sinf(i*PI/180.f)*50);
        for(int i=e; i>=s; i-=10)
        glVertex2f(cosf(i*PI/180.f)*210, sinf(i*PI/180.f)*35);
        glEnd();
        glPopMatrix();
    };

    rings(false); // Back rings
    glow(pX, pY, pR, 0.05f, 0.15f, 0.25f, 1.0f); // Planet Base

    glColor4f(0.2f, 0.5f, 0.7f, 0.8f); // Planet Crescent
    glBegin(GL_POLYGON);
    for(int i=90; i<=270; i+=10)
        glVertex2f(pX + cosf(i*PI/180.f)*pR, pY + sinf(i*PI/180.f)*pR);
    for(int i=270; i>=90; i-=10)
    glVertex2f(pX+15 + cosf(i*PI/180.f)*pR*0.85f, pY-15 + sinf(i*PI/180.f)*pR*0.85f);
    glEnd();

    glow(pX-20, pY+140, 25.f, 0.1f, 0.3f, 0.5f, 1.0f); // Moon
    glColor4f(0.3f, 0.6f, 0.8f, 0.8f);
    glBegin(GL_POLYGON);
    for(int i=90; i<=270; i+=15)
        glVertex2f(pX-20 + cosf(i*PI/180.f)*25, pY+140 + sinf(i*PI/180.f)*25);
    for(int i=270; i>=90; i-=15)
    glVertex2f(pX-15 + cosf(i*PI/180.f)*20, pY+135 + sinf(i*PI/180.f)*20);
    glEnd();

    rings(true); // Front rings
    glLineWidth(1.f);

    // 4. Vertical Light Beams
    glColor4f(0.2f, 0.6f, 1.0f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(pX-50, WIN_H);
    glVertex2f(pX-25, WIN_H);
    glVertex2f(pX-25, 0);
    glVertex2f(pX-50, 0);
    glVertex2f(pX+30, WIN_H);
    glVertex2f(pX+45, WIN_H);

    glVertex2f(pX+45, 0);
    glVertex2f(pX+30, 0);
    glEnd();

    // 5. Icy Snow Dunes
    dune(120.f, 25.f, 0.01f, 1.f, 0.2f, 0.35f, 0.5f,  0.05f, 0.1f, 0.15f);
    dune(80.f,  15.f, 0.015f,3.f, 0.4f, 0.6f,  0.75f, 0.05f, 0.1f, 0.15f);
    dune(40.f,  10.f, 0.008f,0.f, 0.6f, 0.75f, 0.85f, 0.02f, 0.05f, 0.1f);

    glDisable(GL_BLEND);

    // 6. Gameplay Scenery (Meteors & Matte Planet)
    pushBG(-1.5f, 1.5f, -1.0f, 1.0f);
    for (auto& m : meteors) drawRockyMeteor(m);
    glPushMatrix(); glTranslatef(0.6f, -0.2f, 0.0f); drawMatteMercury(0.44f); glPopMatrix();
    popBG();
}

// Draw: Sun Background (Clean Starfield & Giant Sun)
void drawSunBackground() {
    // 1. Deep Black Space
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(GX0, 0);
        glVertex2f(WIN_W, 0);
        glVertex2f(WIN_W, WIN_H);
        glVertex2f(GX0, WIN_H);
    glEnd();

    // 2. Parallax Stars
    glPointSize(1.8f);
    glBegin(GL_POINTS);
    for (auto& s : pstars) {
        if (s.x >= GX0 && s.x <= WIN_W) {
            glColor3f(s.bright * 0.5f, s.bright * 0.6f, s.bright);
            glVertex2f(s.x, s.y);
        }
    }
    glEnd();

    // 3. Giant Sun Body
    float sx = WIN_W - 50, sy = WIN_H / 2, r = 250.0f;
    auto sunCircle = [&](float radius, float R, float G, float B) {
        glColor3f(R, G, B);
        glBegin(GL_TRIANGLE_FAN);
        for (int i = 0; i <= 360; i += 15)
            glVertex2f(sx + radius * cosf(i*PI/180.0f), sy + radius * sinf(i*PI/180.0f));
        glEnd();
    };
    sunCircle(r * 1.8f, 0.8f, 0.1f, 0.0f);
    sunCircle(r * 1.3f, 1.0f, 0.4f, 0.0f);
    sunCircle(r * 0.9f, 1.0f, 0.7f, 0.0f);
    sunCircle(r * 0.5f, 1.0f, 0.95f, 0.7f);

    // 4. Solar Flares
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 150; i++) {
        float a = (i * 15.0f + gameTime * 20.0f) * PI / 180.0f;
        float d = r * (0.6f + pseudoRandom(i) * 1.2f);
        (i % 2 == 0) ? glColor3f(1.0f, 0.5f, 0.0f) : glColor3f(1.0f, 0.8f, 0.0f);
        glVertex2f(sx + d * cosf(a), sy + d * sinf(a));
    }
    glEnd();
}

// Dispatcher — call this once per frame during PLAYING state
void drawLevelBackground() {
    switch (currentLevel) {
        case EARTH:    drawEarthBackground();    break;
        case VENUS:    drawVenusBackground();    break;
        case MERCURY:  drawMercuryBackground();  break;
        case SUN_BOSS: drawSunBackground();      break;
    }
}

// Text Helper
void drawText(float x, float y, const std::string& s,
              void* font = GLUT_BITMAP_HELVETICA_12) {
    glRasterPos2f(x, y);
    for (char c : s) glutBitmapCharacter(font, c);
}




// Draw: Mercury Player Ship (Neon Pink Plasma Interceptor)
void drawMercuryPlayer(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(1.5f, 1.5f, 1.0f);

    // 1. Plasma Engine Exhaust (Hot Pink & White)
    float pulse = 0.8f + 0.2f * sinf(gameTime * 35.0f);

    // Outer pink plasma
    glColor3f(1.0f, 0.2f * pulse, 0.8f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-22,  4);
        glVertex2f(-45 - 8*pulse,  0);
        glVertex2f(-22, -4);
    glEnd();

    // Inner white-hot core (with a slight pink tint)
    glColor3f(1.0f, 0.85f, 0.9f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-22,  2);
        glVertex2f(-32 - 4*pulse,  0);
        glVertex2f(-22, -2);
    glEnd();

    // 2. Main Body Engine Block (Dark Rose-Steel)
    glColor3f(0.25f, 0.20f, 0.22f);
    glBegin(GL_POLYGON);
        glVertex2f( 10,  4);
        glVertex2f(-15,  6);
        glVertex2f(-24,  4);
        glVertex2f(-24, -4);
        glVertex2f(-15, -6);
        glVertex2f( 10, -4);
    glEnd();

    // 3. Twin Railgun Prongs (Titanium White with a hint of rose)
    glColor3f(0.9f, 0.82f, 0.85f);
    // Top Prong
    glBegin(GL_POLYGON);
        glVertex2f( -5,  4);
        glVertex2f( 35,  3);
        glVertex2f( 32,  8);
        glVertex2f(  5,  9);
    glEnd();
    // Bottom Prong
    glBegin(GL_POLYGON);
        glVertex2f( -5, -4);
        glVertex2f( 35, -3);
        glVertex2f( 32, -8);
        glVertex2f(  5, -9);
    glEnd();

    // 4. Central Energy Core (Pulsing Neon Pink inside the prongs)
    float corePulse = 0.5f + 0.5f * sinf(gameTime * 20.0f);
    glColor3f(1.0f, 0.1f + 0.5f * corePulse, 0.6f);
    glBegin(GL_QUADS);
        glVertex2f( -5,  2);
        glVertex2f( 28,  1.5f);
        glVertex2f( 28, -1.5f);
        glVertex2f( -5, -2);
    glEnd();

    // Laser core highlight (Pure white line inside the pink)
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
        glVertex2f(-2, 0);
        glVertex2f(25, 0);
    glEnd();

    // 5. Heavy Swept-Back Wings (Dark Rose-Steel)
    glColor3f(0.35f, 0.28f, 0.30f);
    glBegin(GL_POLYGON); // Top wing
        glVertex2f( -5,  8);
        glVertex2f( -2, 22);
        glVertex2f(-12, 24);
        glVertex2f(-18,  6);
    glEnd();
    glBegin(GL_POLYGON); // Bottom wing
        glVertex2f( -5, -8);
        glVertex2f( -2,-22);
        glVertex2f(-12,-24);
        glVertex2f(-18, -6);
    glEnd();

    // 6. Glowing Edge Highlights on Wings (Neon Magenta/Pink)
    glColor3f(1.0f, 0.15f, 0.65f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP); // Top
        glVertex2f(-5, 8);
        glVertex2f(-2, 22);
        glVertex2f(-12, 24);
    glEnd();
    glBegin(GL_LINE_STRIP); // Bottom
        glVertex2f(-5, -8);
        glVertex2f(-2,-22);
        glVertex2f(-12,-24);
    glEnd();
    glLineWidth(1.0f);

    // 7. Cockpit (Dark Tinted Glass)
    glColor3f(0.08f, 0.05f, 0.06f);
    glBegin(GL_POLYGON);
        glVertex2f( 8,  0);
        glVertex2f( 2,  3);
        glVertex2f(-8,  3);
        glVertex2f(-8, -3);
        glVertex2f( 2, -3);
    glEnd();

    // Cockpit Glare (Pinkish reflection)
    glColor3f(0.6f, 0.3f, 0.45f);
    glBegin(GL_TRIANGLES);
        glVertex2f( 6,  0);
        glVertex2f( 0,  2);
        glVertex2f(-6,  2);
    glEnd();

    glPopMatrix();
}

// Draw: Haunted Solar Destroyer (Ethereal / Light Phantom Form)
void drawHauntedSolarDestroyer(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(1.7f, 1.7f, 1.0f); // Imposing size

    // 1. Ghostly Spectral Aura (Glowing pale white/cyan)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float auraPulse = 0.1f + 0.1f * sinf(gameTime * 20.0f); // Fast ethereal flicker
    glColor4f(0.8f, 0.95f, 1.0f, auraPulse); // Ghostly pale blue
    glBegin(GL_POLYGON);
    for (int i = 0; i < 20; i++) {
        float a = i * 18.0f * PI / 180.0f;
        float r = 40.0f + sinf(gameTime * 15.0f + i) * 3.0f; // Shifting phantom shape
        glVertex2f(cosf(a) * r, sinf(a) * r * 0.8f);
    }
    glEnd();
    glDisable(GL_BLEND);

    // 2. Core Engine (Blinding White/Cyan Plasma)
    float engineGlow = 0.6f + 0.4f * sinf(gameTime * 30.0f + PI/2);
    glColor3f(engineGlow * 0.7f, engineGlow * 0.9f, engineGlow * 1.0f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-20,  4);
        glVertex2f(-40 - 10*engineGlow,  0);
        glVertex2f(-20, -4);
    glEnd();

    // Pure white hot core
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_POLYGON);
        glVertex2f(-25,  3);
        glVertex2f(-35, 0);
        glVertex2f(-25, -3);
        glVertex2f(-28,  2);
        glVertex2f(-30, 0);
        glVertex2f(-28, -2);
    glEnd();

    // 3. Main Body (Pristine Spectral Platinum)
    glColor3f(0.85f, 0.90f, 0.95f); // Very light, reflective metal
    glBegin(GL_POLYGON);
        glVertex2f( 35,  0);
        glVertex2f( 10,  7);
        glVertex2f( -8, 10);
        glVertex2f(-20,  6);
        glVertex2f(-20, -6);
        glVertex2f( -8,-10);
        glVertex2f( 10, -7);
    glEnd();

    // 4. Jagged, Asymmetrical Wing Structures (Silver/Cyan armor)
    glColor3f(0.70f, 0.75f, 0.85f); // Lighter metallic silver
    glBegin(GL_POLYGON); // Top wing
        glVertex2f( -5,  8);
        glVertex2f(-18, 28);
        glVertex2f(  8, 25);
        glVertex2f(  0,  9);
    glEnd();
    glBegin(GL_POLYGON); // Bottom wing
        glVertex2f( -5, -8);
        glVertex2f(-18,-28);
        glVertex2f(  8,-25);
        glVertex2f(  0, -9);
    glEnd();

    // 5. Multiple Firing Cannons (Radiant Emitters)
    float cannonGlow = 0.6f + 0.4f * sinf(gameTime * 25.0f);

    // Top & Bottom front cannons (Piercing Cyan)
    glColor3f(cannonGlow * 0.3f, cannonGlow * 0.8f, cannonGlow * 1.0f);
    glBegin(GL_QUADS);
        glVertex2f(28,  4);
        glVertex2f(38,  4);
        glVertex2f(38,  2);
        glVertex2f(28,  2);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2f(28, -2);
        glVertex2f(38, -2);
        glVertex2f(38, -4);
        glVertex2f(28, -4);
    glEnd();

    // Side cannons integrated into wings (Spectral Violet)
    glColor3f(cannonGlow * 0.8f, cannonGlow * 0.4f, cannonGlow * 1.0f);
    glBegin(GL_QUADS);
        glVertex2f(10,  12);
        glVertex2f(20,  15);
        glVertex2f(18,  13);
        glVertex2f(8,  10);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2f(10, -12);
        glVertex2f(20, -15);
        glVertex2f(18, -13);
        glVertex2f(8, -10);
    glEnd();

    // 6. Corrupted Cockpit / "Eyes" (Eerie Golden Glow)
    float eyePulse = 0.7f + 0.3f * sinf(gameTime * 8.0f);
    glColor3f(eyePulse * 1.0f, eyePulse * 0.8f, eyePulse * 0.2f); // Piercing phantom gold
    glBegin(GL_TRIANGLES);
        glVertex2f( 25,  0);
        glVertex2f( 10,  4);
        glVertex2f( 10, -4);
    glEnd();
    glColor3f(1.0f, 1.0f, 0.8f);
    glBegin(GL_TRIANGLES); // Inner glow
        glVertex2f( 20,  0);
        glVertex2f( 12,  2);
        glVertex2f( 12, -2);
    glEnd();

    // 7. Skeletal protrusions / Hooks (Glowing light traces)
    glColor3f(0.5f, 0.8f, 1.0f); // Bright cyan accents instead of dark bone
    glLineWidth(2.0f);
    glBegin(GL_LINES);
        glVertex2f(25, 0);
        glVertex2f(30, 8);
        glVertex2f(30, 8);
        glVertex2f(35, 5);
        glVertex2f(25, 0);
        glVertex2f(30, -8);
        glVertex2f(30, -8);
        glVertex2f(35, -5);
    glEnd();
    glLineWidth(1.0f);

    glPopMatrix();
}


// Draw: Earth Enemy (Advanced Forward-Swept Interceptor Drone)
void drawEarthEnemy(float x, float y, float t) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    // Mirror facing left (towards the player)
    glScalef(-1.2f, 1.2f, 1.0f); // Scaled up slightly to be more menacing

    // 1. Engine Exhaust (Hostile Orange/Red pulse)
    float pulse = 0.8f + 0.2f * sinf(t * 25.0f);

    // Outer flame
    glColor3f(1.0f, 0.2f * pulse, 0.0f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-22,  3);
        glVertex2f(-38 - 6*pulse,  0);
        glVertex2f(-22, -3);
    glEnd();

    // Inner hot core
    glColor3f(1.0f, 0.8f, 0.2f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-22,  1.5f);
        glVertex2f(-30 - 3*pulse,  0);
        glVertex2f(-22, -1.5f);
    glEnd();

    // 2. Main Fuselage (Aggressive angular interceptor body - Gunmetal Grey)
    glColor3f(0.20f, 0.22f, 0.25f);
    glBegin(GL_POLYGON);
        glVertex2f( 28,  0);  // Sharp needle nose
        glVertex2f( 12,  5);
        glVertex2f(-12,  6);
        glVertex2f(-22,  3);
        glVertex2f(-22, -3);
        glVertex2f(-12, -6);
        glVertex2f( 12, -5);
    glEnd();

    // 3. Forward-Swept Wings (Crimson / Dark Red armor)
    glColor3f(0.45f, 0.08f, 0.12f);
    glBegin(GL_POLYGON); // Top wing
        glVertex2f( -2,  5);
        glVertex2f( 14, 24); // Swept forward tip
        glVertex2f(  4, 26);
        glVertex2f(-14,  6);
    glEnd();
    glBegin(GL_POLYGON); // Bottom wing
        glVertex2f( -2, -5);
        glVertex2f( 14,-24); // Swept forward tip
        glVertex2f(  4,-26);
        glVertex2f(-14, -6);
    glEnd();

    // 4. Canards (Small front stabilizing wings)
    glColor3f(0.3f, 0.3f, 0.35f);
    glBegin(GL_TRIANGLES);
        glVertex2f( 16,  3);
        glVertex2f(  8, 12);
        glVertex2f(  6,  4);
        glVertex2f( 16, -3);
        glVertex2f(  8,-12);
        glVertex2f(  6, -4);
    glEnd();

    // 5. Twin Tail Fins
    glColor3f(0.35f, 0.05f, 0.05f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-12,  5);
        glVertex2f(-20, 14);
        glVertex2f(-20,  4);
        glVertex2f(-12, -5);
        glVertex2f(-20,-14);
        glVertex2f(-20, -4);
    glEnd();

    // 6. Drone Sensor / Cockpit (Glowing hostile ruby red)
    glColor3f(0.9f, 0.0f, 0.0f);
    glBegin(GL_POLYGON);
        glVertex2f( 20,  0);
        glVertex2f( 10,  3);
        glVertex2f(  2,  3);
        glVertex2f(  2, -3);
        glVertex2f( 10, -3);
    glEnd();

    // Sensor glare (gives it a glowing glass look)
    glColor3f(1.0f, 0.6f, 0.6f);
    glBegin(GL_TRIANGLES);
        glVertex2f( 15,  0);
        glVertex2f(  8,  1.5f);
        glVertex2f(  8, -1.5f);
    glEnd();

    // 7. Wingtip Missile Pods
    glColor3f(0.1f, 0.1f, 0.12f);
    glBegin(GL_QUADS);
        // Top pod
        glVertex2f( 16, 23);
        glVertex2f(  2, 23);
        glVertex2f(  2, 26);
        glVertex2f( 16, 26);
        // Bottom pod
        glVertex2f( 16,-23);
        glVertex2f(  2,-23);
        glVertex2f(  2,-26);
        glVertex2f( 16,-26);
    glEnd();

    glPopMatrix();
}

// Draw: Venus UFO

void drawVenusUFO(float x, float y, float t) {
    glPushMatrix();
    float wobbleX = sinf(t * 2.3f) * 2.0f;
    float wobbleY = sinf(t * 2.7f) * 4.0f;
    glTranslatef(x + wobbleX, y + wobbleY + sinf(t * 3.0f)*5.0f, 0);
    glScalef(1.8f, 1.8f, 1.0f);

    // Main saucer body
    glColor3f(0.25f, 0.15f, 0.10f);
    glBegin(GL_POLYGON);
        glVertex2f( 30,  2);
        glVertex2f( 28,  6);
        glVertex2f( 22,  9);
        glVertex2f( 14, 12);
        glVertex2f(  4, 13);
        glVertex2f( -6, 12);
        glVertex2f(-16, 10);
        glVertex2f(-25,  6);
        glVertex2f(-31,  1);
        glVertex2f(-30, -3);
        glVertex2f(-26, -8);
        glVertex2f(-18,-11);
        glVertex2f( -8,-13);
        glVertex2f(  4,-13);
        glVertex2f( 14,-11);
        glVertex2f( 24, -7);
        glVertex2f( 29, -2);
    glEnd();

    // Armor spikes
    glColor3f(0.45f, 0.10f, 0.05f);
    glBegin(GL_TRIANGLES);
        glVertex2f( 10, 12);
        glVertex2f(  5, 20);
        glVertex2f(  0, 12);
        glVertex2f( -5, 12);
        glVertex2f(-12, 21);
        glVertex2f(-10, 11);
        glVertex2f(-18,  9);
        glVertex2f(-24, 16);
        glVertex2f(-20,  8);
        glVertex2f( 10,-12);
        glVertex2f(  4,-21);
        glVertex2f(  0,-12);
        glVertex2f( -8,-12);
        glVertex2f(-14,-22);
        glVertex2f(-10,-11);
        glVertex2f(-20, -8);
        glVertex2f(-26,-15);
        glVertex2f(-22, -7);
    glEnd();

    // Top dome
    glColor3f(0.30f, 0.20f, 0.15f);
    glBegin(GL_POLYGON);
        glVertex2f( -8, 13);
        glVertex2f( -3, 17);
        glVertex2f(  2, 19);
        glVertex2f(  8, 18);
        glVertex2f( 12, 15);
        glVertex2f( 14, 11);
        glVertex2f(  9, 12);
        glVertex2f(  2, 12);
        glVertex2f( -5, 12);
        glVertex2f(-10, 12);
    glEnd();

    // Evil eye
    glColor3f(1.0f, 0.2f, 0.1f);
    glBegin(GL_TRIANGLES);
        glVertex2f(  2, 15);
        glVertex2f( -4, 12);
        glVertex2f(  8, 12);
    glEnd();
    glColor3f(1.0f, 0.7f, 0.1f);
    glBegin(GL_TRIANGLES);
        glVertex2f(  2, 14);
        glVertex2f(  0, 12);
        glVertex2f(  4, 12);
    glEnd();

    // Cannon
    glColor3f(0.4f, 0.15f, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f( -3,-13);
        glVertex2f(  3,-13);
        glVertex2f(  2,-22);
        glVertex2f( -2,-22);
    glEnd();
    glColor3f(1.0f, 0.3f, 0.0f);
    glBegin(GL_TRIANGLES);
        glVertex2f( -2,-22);
        glVertex2f(  2,-22);
        glVertex2f(  0,-28);
    glEnd();

    // Tentacles
    glColor3f(0.5f, 0.1f, 0.1f);
    glBegin(GL_LINE_STRIP);
        glVertex2f(-10,-12);
        glVertex2f(-16,-18);
        glVertex2f(-14,-22);
        glVertex2f(-20,-26);
    glEnd();
    glBegin(GL_LINE_STRIP);
        glVertex2f( 12,-12);
        glVertex2f( 18,-19);
        glVertex2f( 15,-24);
        glVertex2f( 22,-28);
    glEnd();

    // Blinking war lights (MOVEMENT STOPPED)
    for (int i = 0; i < 12; i++) {
        // Removed "+ t * 80.0f" so they stay in place
        float angle = i * 30.0f;
        float rad = angle * PI / 180.0f;
        float rX = cosf(rad) * 28.0f, rY = sinf(rad) * 28.0f;

        // They will still blink between Red and Orange
        if (fmodf(t * 5.0f + i, 3.0f) < 1.5f)
            glColor3f(1.0f, 0.2f, 0.1f);
        else
            glColor3f(0.9f, 0.5f, 0.0f);

        glPointSize(6.0f);
        glBegin(GL_POINTS);
        glVertex2f(rX, rY);
        glEnd();
    }

    glPopMatrix();
}


void drawMercuryUFO(float x, float y, float t) {
    glPushMatrix();

    // No more bobbing motion. Just fixed translation and scaling.
    glTranslatef(x, y, 0);
    glScalef(-1.8f, 1.8f, 1.0f); // Mirror facing left and make it big

    // 1. Static Thorny Back Spikes (Replaces the writhing tentacles)
    glColor3f(0.2f, 0.05f, 0.1f);
    glBegin(GL_TRIANGLES);
        // Top spikes
        glVertex2f(-15,  5);
        glVertex2f(-35, 12);
        glVertex2f(-15,  1);
        glVertex2f(-18,  8);
        glVertex2f(-40,  4);
        glVertex2f(-20,  6);
        // Bottom spikes
        glVertex2f(-15, -5);
        glVertex2f(-35,-12);
        glVertex2f(-15, -1);
        glVertex2f(-18, -8);
        glVertex2f(-40, -4);
        glVertex2f(-20, -6);
    glEnd();

    // 2. Rigid, imposing bat-like demon wings (No flapping animation)
    glColor3f(0.3f, 0.08f, 0.12f);
    glBegin(GL_TRIANGLES);
        // Top wing
        glVertex2f(-5,  5);
        glVertex2f(-15, 26);
        glVertex2f( 8,  8);
        glVertex2f( 8,  8);
        glVertex2f(  2, 22);
        glVertex2f(-15, 26);
        // Bottom wing
        glVertex2f(-5, -5);
        glVertex2f(-15,-26);
        glVertex2f( 8, -8);
        glVertex2f( 8, -8);
        glVertex2f(  2,-22);
        glVertex2f(-15,-26);
    glEnd();

    // 3. Grotesque fleshy main body (Blood red/purple)
    glColor3f(0.45f, 0.05f, 0.05f);
    glBegin(GL_POLYGON);
        glVertex2f( 12,  0);
        glVertex2f(  8,  8);
        glVertex2f( -5, 10);
        glVertex2f(-15,  6);
        glVertex2f(-18,  0);
        glVertex2f(-15, -6);
        glVertex2f( -5,-10);
        glVertex2f(  8, -8);
    glEnd();

    // 4. Sickly green pustules / sores on the body
    glColor3f(0.3f, 0.9f, 0.1f);
    glPointSize(4.0f);
    glBegin(GL_POINTS);
        glVertex2f(-6,  5);
        glVertex2f(-10,-2);
        glVertex2f(-2, -6);
        glVertex2f(-8,  3);
    glEnd();

    // 5. Gaping Maw (Pitch black mouth interior)
    glColor3f(0.05f, 0.0f, 0.0f);
    glBegin(GL_POLYGON);
        glVertex2f( 12,  5);
        glVertex2f( 25,  4);
        glVertex2f( 20,  0);
        glVertex2f( 25, -4);
        glVertex2f( 12, -5);
    glEnd();

    // 6. Jagged dirty bone teeth
    glColor3f(0.85f, 0.85f, 0.7f);
    glBegin(GL_TRIANGLES);
        // Top row
        glVertex2f(12, 5);
        glVertex2f(15, 1);
        glVertex2f(18, 4);
        glVertex2f(18, 4);
        glVertex2f(21, 1);
        glVertex2f(24, 3);
        // Bottom row
        glVertex2f(12,-5);
        glVertex2f(15,-1);
        glVertex2f(18,-4);
        glVertex2f(18,-4);
        glVertex2f(21,-1);
        glVertex2f(24,-3);
    glEnd();

    // 7. Clustered spider-like glowing red eyes (Static, glaring)
    glColor3f(1.0f, 0.1f, 0.0f);
    glPointSize(5.0f); // Bigger, fixed piercing eyes
    glBegin(GL_POINTS);
        glVertex2f(10,  6);
        glVertex2f(14,  7);
        glVertex2f( 6,  3);
        glVertex2f( 6, -3);
        glVertex2f(10, -6);
        glVertex2f(13, -8);
    glEnd();

    // 8. Static acidic saliva drips hanging from the mouth
    glColor3f(0.6f, 1.0f, 0.2f);
    glBegin(GL_TRIANGLES);
        glVertex2f(18, -3);
        glVertex2f(20, -3);
        glVertex2f(19,  -9);
        glVertex2f(13, -4);
        glVertex2f(15, -4);
        glVertex2f(14, -13);
    glEnd();

    glPopMatrix();
}
// Draw: Sun Boss
void drawSunBoss(float x, float y, float t) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    // Pulsing aura
    float pulse = 0.8f + 0.2f * sinf(t * 2.0f);
    glColor3f(1.0f*pulse, 0.2f*pulse, 0.0f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 32; i++) {
        float a = i * 11.25f * PI / 180.0f;
        float r = 65.0f + sinf(t * 3.0f + i * 0.5f) * 8.0f;
        glVertex2f(cosf(a)*r, sinf(a)*r);
    }
    glEnd();

    // Molten core
    glColor3f(0.95f, 0.35f, 0.05f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 24; i++) {
        float a = i * 15.0f * PI / 180.0f;
        glVertex2f(cosf(a)*50, sinf(a)*50);
    }
    glEnd();

    // Inner lava
    glColor3f(1.0f, 0.7f, 0.1f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 20; i++) {
        float a = i * 18.0f * PI / 180.0f;
        float r = 30.0f + sinf(t * 4.0f + i) * 3.0f;
        glVertex2f(cosf(a)*r, sinf(a)*r);
    }
    glEnd();

    // Rotating fire spikes
    for (int i = 0; i < 8; i++) {
        float a = (i * 45.0f + t * 30.0f) * PI / 180.0f;
        glColor3f(1.0f, 0.4f, 0.0f);
        glBegin(GL_TRIANGLES);
            glVertex2f(cosf(a)*50, sinf(a)*50);
            glVertex2f(cosf(a+0.2f)*70, sinf(a+0.2f)*70);
            glVertex2f(cosf(a-0.2f)*70, sinf(a-0.2f)*70);
        glEnd();
        glColor3f(1.0f, 0.9f, 0.3f);
        glPointSize(5.0f);
        glBegin(GL_POINTS);
            glVertex2f(cosf(a)*72, sinf(a)*72);
        glEnd();
    }

    // Eyes
    float eyeGlow = 0.5f + 0.5f * sinf(t * 5.0f);
    glColor3f(eyeGlow, eyeGlow * 0.2f, 0.0f);
    glPointSize(8.0f);
    glBegin(GL_POINTS);
        glVertex2f(-15, 10);
        glVertex2f( 15, 10);
    glEnd();

    // Boss health bar
    glColor3f(0.08f, 0.08f, 0.08f);
    glBegin(GL_QUADS);
        glVertex2f(-50,-70);
        glVertex2f(50,-70);
        glVertex2f(50,-63);
        glVertex2f(-50,-63);
    glEnd();
    glColor3f(1.0f, 0.3f, 0.0f);
    float hw = 100.0f * (boss.health / 100.0f);
    glBegin(GL_QUADS);
        glVertex2f(-50,-70);
        glVertex2f(-50+hw,-70);
        glVertex2f(-50+hw,-63);
        glVertex2f(-50,-63);
    glEnd();

    glPopMatrix();
}

// Draw: Particles
void drawParticles() {
    for (auto& p : particles) {
        if (p.life <= 0) continue;
        float a = p.life / p.maxLife;
        glColor3f(p.r*a, p.g*a, p.b*a);
        glPointSize(p.size * a);
        glBegin(GL_POINTS);
            glVertex2f(p.x, p.y);
        glEnd();
    }
}

// Draw: UI Side Panel (Pixel-Art Edition)
void drawUI() {
    // 1. Panel Background (Deep Dark Retro Color)
    glColor3f(0.04f, 0.05f, 0.08f);
    glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(UI_W, 0);
        glVertex2f(UI_W, WIN_H);
        glVertex2f(0, WIN_H);
    glEnd();

    // 2. Pixel Border Line
    glColor3f(0.2f, 0.2f, 0.25f);
    glBegin(GL_LINES);
        glVertex2f(UI_W - 1, 0);
        glVertex2f(UI_W - 1, WIN_H);
    glEnd();

    auto divider = [](float y) {
        glColor3f(0.15f, 0.2f, 0.25f);
        glBegin(GL_LINES);
            glVertex2f(8, y);
            glVertex2f(UI_W - 8, y);
        glEnd();
    };

    // --- Helper: Pixel Art Segmented Bar ---
    // (Matches the segmented green/yellow bars from your reference image)
    auto drawSegmentedBar = [](float x, float y, float w, float h, float pct, float r, float g, float b) {
        // Outer dark casing
        glColor3f(0.1f, 0.1f, 0.15f);
        glBegin(GL_QUADS);
            glVertex2f(x-2, y-2);
            glVertex2f(x+w+2, y-2);
            glVertex2f(x+w+2, y+h+2);
            glVertex2f(x-2, y+h+2);
        glEnd();

        int segments = 8;
        float gap = 2.0f;
        float segW = (w - gap * (segments - 1)) / segments;
        int active = (int)(pct * segments + 0.5f);

        for (int i = 0; i < segments; i++) {
            float sx = x + i * (segW + gap);
            if (i < active) {
                // Colored segment
                glColor3f(r, g, b);
                glBegin(GL_QUADS);
                    glVertex2f(sx, y);
                    glVertex2f(sx+segW, y);
                    glVertex2f(sx+segW, y+h);
                    glVertex2f(sx, y+h);
                glEnd();
                // Pixel Highlight (Top Edge)
                glColor3f(std::min(1.0f, r+0.4f), std::min(1.0f, g+0.4f), std::min(1.0f, b+0.4f));
                glBegin(GL_QUADS);
                    glVertex2f(sx, y+h-3);
                    glVertex2f(sx+segW, y+h-3);
                    glVertex2f(sx+segW, y+h);
                    glVertex2f(sx, y+h);
                glEnd();
            } else {
                // Empty segment
                glColor3f(0.25f, 0.25f, 0.3f);
                glBegin(GL_QUADS);
                    glVertex2f(sx, y);
                    glVertex2f(sx+segW, y);
                    glVertex2f(sx+segW, y+h);
                    glVertex2f(sx, y+h);
                glEnd();
            }
        }
    };

    // --- Helper: Retro D-Pad Arrow ---
    auto drawPixelArrow = [](float cx, float cy, int dir, float r, float g, float b) {
        glPushMatrix();
        glTranslatef(cx, cy, 0);
        // 0=Up, 1=Right, 2=Down, 3=Left
        if(dir == 1) glRotatef(-90, 0, 0, 1);
        if(dir == 2) glRotatef(180, 0, 0, 1);
        if(dir == 3) glRotatef(90, 0, 0, 1);

        float ps = 1.5f; // Pixel Scale

        // Main Arrow Block Color
        glColor3f(r, g, b);
        glBegin(GL_QUADS);
            // Stem
            glVertex2f(-ps*2, -ps*3);
            glVertex2f(ps*2, -ps*3);
            glVertex2f(ps*2, ps*1);
            glVertex2f(-ps*2, ps*1);
            // Head Base
            glVertex2f(-ps*4, ps*1);
            glVertex2f(ps*4, ps*1);
            glVertex2f(ps*4, ps*3);
            glVertex2f(-ps*4, ps*3);
            // Tip
            glVertex2f(-ps*2, ps*3);
            glVertex2f(ps*2, ps*3);
            glVertex2f(ps*2, ps*5);
            glVertex2f(-ps*2, ps*5);
        glEnd();

        // White Pixel Highlight (top left)
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
            glVertex2f(-ps*2, ps*3);
            glVertex2f(0, ps*3);
            glVertex2f(0, ps*5);
            glVertex2f(-ps*2, ps*5);

            glVertex2f(-ps*4, ps*1);
            glVertex2f(-ps*2, ps*1);
            glVertex2f(-ps*2, ps*3);
            glVertex2f(-ps*4, ps*3);
        glEnd();

        glPopMatrix();
    };


    // -------------------------------------------------------------
    // TOP SECTION: Logic & Stats
    // -------------------------------------------------------------

    // Colony header
    glColor3f(0.30f, 0.68f, 1.0f);
    drawText(8, WIN_H-25, "SECTOR 7", GLUT_BITMAP_HELVETICA_12);
    glColor3f(0.20f, 0.50f, 0.80f);
    drawText(14, WIN_H-39, "COLONY", GLUT_BITMAP_HELVETICA_12);
    divider(WIN_H-48);

    // Level info
    float rLvl, gLvl, bLvl;
    getLevelColors(rLvl, gLvl, bLvl);
    glColor3f(rLvl, gLvl, bLvl);
    drawText(8, WIN_H-68, "WAVE INFO:", GLUT_BITMAP_HELVETICA_10);
    std::string lvlName = getLevelName();
    drawText(8, WIN_H-82, lvlName.substr(0, 12), GLUT_BITMAP_HELVETICA_10);

    if (currentLevel != SUN_BOSS) {
        std::ostringstream ss; ss << enemiesKilled << "/" << killsNeeded;
        glColor3f(1.0f, 0.90f, 0.25f);
        drawText(18, WIN_H-98, ss.str(), GLUT_BITMAP_HELVETICA_12);
    }
    divider(WIN_H-108);

    // Score
    glColor3f(0.92f, 0.72f, 0.10f);
    drawText(8, WIN_H-128, "SCRAP:", GLUT_BITMAP_HELVETICA_12);
    std::ostringstream ssScrap; ssScrap << score;
    glColor3f(1.0f, 0.90f, 0.25f);
    drawText(8, WIN_H-150, ssScrap.str(), GLUT_BITMAP_HELVETICA_18);
    divider(WIN_H-162);

    // Colony integrity (Yellow/Orange Bar)
    glColor3f(0.95f, 0.65f, 0.10f);
    drawText(8, WIN_H-182, "COLONY HP", GLUT_BITMAP_HELVETICA_10);
    drawSegmentedBar(10, WIN_H-202, UI_W-20, 12, colonyHP / 100.0f, 1.0f, 0.7f, 0.0f);

    // Pilot HP (Bright Green Bar)
    glColor3f(0.28f, 0.90f, 0.30f);
    drawText(8, WIN_H-232, "PILOT HP", GLUT_BITMAP_HELVETICA_10);
    drawSegmentedBar(10, WIN_H-252, UI_W-20, 12, pilotHP / 100.0f, 0.0f, 0.9f, 0.2f);
    divider(WIN_H-268);

    // Hostile count
    int activeEnemies = 0;
    for (auto& e : enemies) if (e.active) activeEnemies++;
    if (boss.active) activeEnemies++;

    glColor3f(0.85f, 0.38f, 0.10f);
    drawText(8, WIN_H-288, "HOSTILES", GLUT_BITMAP_HELVETICA_12);
    std::ostringstream ssHostiles; ssHostiles << activeEnemies;
    glColor3f(1.0f, 0.50f, 0.10f);
    drawText(35, WIN_H-310, ssHostiles.str(), GLUT_BITMAP_HELVETICA_18);


    // -------------------------------------------------------------
    // BOTTOM SECTION: Controls Placement (Retro D-Pad & Buttons)
    // -------------------------------------------------------------

    float dpadX = UI_W / 2.0f;
    float dpadY = 100.0f;
    float space = 16.0f;

    // Draw the colored D-pad Arrows just like the reference image
    drawPixelArrow(dpadX, dpadY + space, 0, 1.0f, 0.2f, 0.2f); // UP (Red)
    drawPixelArrow(dpadX + space, dpadY, 1, 0.1f, 0.5f, 1.0f); // RIGHT (Blue)
    drawPixelArrow(dpadX, dpadY - space, 2, 1.0f, 0.8f, 0.0f); // DOWN (Yellow)
    drawPixelArrow(dpadX - space, dpadY, 3, 0.1f, 0.9f, 0.2f); // LEFT (Green)

    glColor3f(0.5f, 0.6f, 0.7f);
    drawText(30, 60, "MOVE", GLUT_BITMAP_HELVETICA_10);

    // "A / B" Retro Fire Button Style
    // Square Button border
    glColor3f(0.1f, 0.1f, 0.15f);
    glBegin(GL_QUADS);
        glVertex2f(25, 25);
        glVertex2f(45, 25);
        glVertex2f(45, 45);
        glVertex2f(25, 45);
    glEnd();
    // Inner Orange Button Core
    glColor3f(1.0f, 0.4f, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(27, 27);
        glVertex2f(43, 27);
        glVertex2f(43, 43);
        glVertex2f(27, 43);
    glEnd();
    // Pixel Highlight
    glColor3f(1.0f, 0.7f, 0.4f);
    glBegin(GL_QUADS);
        glVertex2f(27, 39);
        glVertex2f(43, 39);
        glVertex2f(43, 43);
        glVertex2f(27, 43);
    glEnd();

    // Fire Label next to button
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(52, 32, "FIRE", GLUT_BITMAP_HELVETICA_12);

    // ESC to quit at very bottom
    glColor3f(0.4f, 0.45f, 0.5f);
    drawText(20, 10, "ESC : QUIT", GLUT_BITMAP_HELVETICA_10);
}
// Draw: HUD Overlay (warning flash)
void drawHUD() {
    if (colonyHP <= 30) {
        warnFlash += 0.16f;
        if (sinf(warnFlash) > 0) {
            glColor3f(0.85f, 0.0f, 0.0f);
            glLineWidth(3.5f);
            glBegin(GL_LINE_LOOP);
                glVertex2f(GX0+4,   4);
                glVertex2f(WIN_W-4, 4);
                glVertex2f(WIN_W-4, WIN_H-4);
                glVertex2f(GX0+4,   WIN_H-4);
            glEnd();
            glLineWidth(1.0f);
            glColor3f(1.0f, 0.15f, 0.15f);
            drawText(WIN_W/2-75, WIN_H-26, "!! COLONY CRITICAL !!", GLUT_BITMAP_HELVETICA_12);
        }
    }
}

// Init / Reset
void initGame() {
    srand((unsigned)time(nullptr));

    player = {160.f, WIN_H/2.f, 0,0, 42.f,22.f, true, 100, 0};
    boss.active = false;

    enemies.clear(); bullets.clear(); particles.clear();

    score = 0; colonyHP = 100; pilotHP = 100;
    enemiesKilled = 0;
    spawnTimer = 0; spawnInterval = 2.0f;
    gameTime = 0; bulletTimer = 0; droneFireT = 0; bossFireT = 0;
    levelTransitionTimer = 0; warnFlash = 0;

    currentLevel  = EARTH;
    killsNeeded   = 5;

    initScenery();
}

// Advance Level
void advanceLevel() {
    enemiesKilled = 0;
    enemies.clear();
    bullets.clear();

    if (currentLevel == EARTH) {
        currentLevel  = VENUS;
        killsNeeded   = 5;
        spawnInterval = 1.8f;
    } else if (currentLevel == VENUS) {
        currentLevel  = MERCURY;
        killsNeeded   = 6;
        spawnInterval = 1.5f;
    } else if (currentLevel == MERCURY) {
        currentLevel = SUN_BOSS;
        boss.active  = true;
        boss.type    = 4;
        boss.x       = WIN_W - 120;
        boss.y       = WIN_H / 2;
        boss.vx      = 0;
        boss.vy      = 1.2f;
        boss.w       = 100;
        boss.h       = 100;
        boss.health  = 40;
    }
}

// Spawn Enemy
void spawnEnemy() {

    Entity e;
    e.active = true;
    e.x = WIN_W + 20.0f;
    e.y = rnd(60, WIN_H-60); // Default spawn range

    if (currentLevel == EARTH) {
        // OVERRIDE: Keep Earth monsters in the upper half (sky)
        e.y      = rnd(WIN_H / 2.0f + 40.0f, WIN_H - 60.0f);

        e.type   = 1;
        e.vx     = -(2.0f + score / 900.0f);
        e.vy     = rnd(-0.5f, 0.5f);

        // BIGGER HITBOX to match the new size
        e.w      = 75.0f; // Increased from 40.0f
        e.h      = 60.0f; // Increased from 20.0f

        e.health = 2;
    }
     else if (currentLevel == VENUS) {
        e.type   = 2;
        e.vx     = -(2.2f + score / 800.0f);
        e.vy     = rnd(-0.6f, 0.6f);
        e.w      = 44.0f;
        e.h      = 28.0f;
        e.health = 2;
    } else if (currentLevel == MERCURY) {
        e.type   = 3;
        e.vx     = -(2.0f + score / 700.0f);
        e.vy     = rnd(-0.5f, 0.5f);
        e.w      = 50.0f;
        e.h      = 30.0f;
        e.health = 3;
    }
    enemies.push_back(e);
}

// Update (timer callback ~60 fps)
void update(int) {
    glutTimerFunc(16, update, 0);
    if (gameState != PLAYING && gameState != LEVEL_COMPLETE) return;

    const float dt = 0.016f;
    gameTime      += dt;
    bulletTimer   += dt;
    droneFireT    += dt;
    bossFireT     += dt;
    spawnTimer    += dt;

    // --- Scenery Update ---
    if (currentLevel == MERCURY) {
        for (auto& m : meteors) {
            m.x -= m.speed * dt;
            m.rot += m.rotSpeed;
            if (m.x < -1.8f) {
                m.x = 1.8f;
                m.y = rnd(-1.0f, 1.0f);
            }
        }
    } else if (currentLevel == SUN_BOSS) {
        for (auto& s : pstars) {
            s.x -= s.speed * 100.0f * dt;
            if (s.x < GX0) {
                s.x = WIN_W;
                s.y = rnd(0, WIN_H);
            }
        }
        for (auto& rock : spaceRocks) {
            rock.x -= (50.0f + rock.size * 0.5f) * dt;
            rock.rot += rock.rotSpd;
            if (rock.x + rock.size < GX0) {
                rock.x = WIN_W + rock.size;
                rock.y = rnd(0, WIN_H);
            }
        }
    }

    // Level transition delay
    if (gameState == LEVEL_COMPLETE) {
        levelTransitionTimer += dt;
        if (levelTransitionTimer > 3.0f) {
            advanceLevel();
            gameState = PLAYING;
            levelTransitionTimer = 0;
        }
        return;
    }

    // --- Player Movement ---
    float spd = 5.5f;
    if (keys['w'] || keys['W'] || skeys[GLUT_KEY_UP])    player.y += spd;
    if (keys['s'] || keys['S'] || skeys[GLUT_KEY_DOWN])  player.y -= spd;
    if (keys['a'] || keys['A'] || skeys[GLUT_KEY_LEFT])  player.x -= spd;
    if (keys['d'] || keys['D'] || skeys[GLUT_KEY_RIGHT]) player.x += spd;

    //player.x = std::max(GX0+24, std::min(player.x, GX1-24));
    //player.y = std::max(32.0f,  std::min(player.y, (float)WIN_H-32));
    float playerMinY = (currentLevel == EARTH) ? (WIN_H / 2.0f + 24.0f) : 32.0f;

    player.x = std::max(GX0+24, std::min(player.x, GX1-24));
    player.y = std::max(playerMinY, std::min(player.y, (float)WIN_H-32));
    // --- Player Fire ---
    if (keys[' '] && bulletTimer > 0.13f) {
        bulletTimer = 0;
        Bullet b;
        b.x = player.x+27; b.y = player.y;
        b.vx = 13.0f; b.vy = 0;
        b.active = true; b.isEnemy = false;
        b.r = 0.3f; b.g = 0.9f; b.b = 1.0f;
        bullets.push_back(b);
    }

    // --- Spawn enemies (not during boss fight) ---
    if (currentLevel != SUN_BOSS) {
        if (spawnTimer > spawnInterval) {
            spawnTimer = 0;
            spawnEnemy();
        }
    }

    // --- Boss Update ---
    if (boss.active) {
        boss.y += boss.vy;
        if (boss.y < 100 || boss.y > WIN_H-100) boss.vy = -boss.vy;

        if (bossFireT > 0.6f) {
            bossFireT = 0;
            for (int i = -1; i <= 1; i++) {
                float dx = player.x - boss.x;
                float dy = player.y - boss.y + i * 60;
                float len = sqrtf(dx*dx + dy*dy);
                if (len > 0) {
                    Bullet b;
                    b.x = boss.x - 30;
                    b.y = boss.y + i * 20;
                    b.vx = (dx/len) * 5.5f;
                    b.vy = (dy/len) * 5.5f;
                    b.active = true; b.isEnemy = true;
                    b.r = 1.0f; b.g = 0.35f; b.b = 0.0f;
                    bullets.push_back(b);
                }
            }
        }
    }

    // --- Enemy Update ---
    for (auto& e : enemies) {
        if (!e.active) continue;
        e.x += e.vx;
        e.y += e.vy;

        // Mercury UFOs fire plasma
        if (e.type == 3 && droneFireT > 1.2f) {
            droneFireT = 0;
            float dx = player.x - e.x, dy = player.y - e.y;
            float len = sqrtf(dx*dx + dy*dy);
            if (len > 0 && e.x > GX0) {
                Bullet b;
                b.x = e.x-12; b.y = e.y;
                b.vx = (dx/len)*4.5f; b.vy = (dy/len)*4.5f;
                b.active = true; b.isEnemy = true;
                b.r = 0.6f; b.g = 0.7f; b.b = 0.9f;
                bullets.push_back(b);
            }
        }

        // Earth jets fire missiles
        if (e.type == 1 && droneFireT > 1.5f) {
            droneFireT = 0;
            float dx = player.x - e.x, dy = player.y - e.y;
            float len = sqrtf(dx*dx + dy*dy);
            if (len > 0 && e.x > GX0) {
                Bullet b;
                b.x = e.x-15; b.y = e.y;
                b.vx = (dx/len)*5.0f; b.vy = (dy/len)*5.0f;
                b.active = true; b.isEnemy = true;
                b.r = 1.0f; b.g = 0.8f; b.b = 0.2f;
                bullets.push_back(b);
            }
        }

        // Set a minimum Y limit based on the enemy type.
        // Earth jets (1) stay in the upper half. UFOs (2, 3) can fly anywhere.
        float minY = (e.type == 1) ? (WIN_H / 2.0f + 35.0f) : 32.0f;

        if (e.y < minY || e.y > WIN_H - 32.0f) {
            e.vy = -e.vy;

            // Prevent the enemy from getting stuck outside boundaries
            if (e.y < minY) e.y = minY;
            if (e.y > WIN_H - 32.0f) e.y = WIN_H - 32.0f;
        }
        // Reached left wall → damages colony
        if (e.x < UI_W) {
            e.active = false;
            colonyHP -= 12;
            spawnParticles(UI_W, e.y, 24, 1.0f, 0.3f, 0.0f);
            if (colonyHP <= 0) { colonyHP = 0; gameState = GAME_OVER; }
        }
    }

    // --- Bullet Update + Collision ---
    for (auto& b : bullets) {
        if (!b.active) continue;
        b.x += b.vx; b.y += b.vy;

        if (b.x > WIN_W || b.x < GX0 || b.y < 0 || b.y > WIN_H) {
            b.active = false; continue;
        }

        if (b.isEnemy) {
            if (rectHit(b.x-5, b.y-5, 10, 10,
                        player.x-21, player.y-13, 42, 26)) {
                b.active = false;
                pilotHP -= 10;
                spawnParticles(player.x, player.y, 12, 0.2f, 0.7f, 1.0f);
                if (pilotHP <= 0) { pilotHP = 0; gameState = GAME_OVER; }
            }
        } else {
            // Hit boss
            if (boss.active && rectHit(b.x-5, b.y-5, 10, 10,
                                       boss.x-50, boss.y-50, 100, 100)) {
                b.active = false;
                boss.health--;
                spawnParticles(b.x, b.y, 15, 1.0f, 0.5f, 0.0f);
                if (boss.health <= 0) {
                    boss.active = false;
                    score += 1000;
                    spawnParticles(boss.x, boss.y, 80, 1.0f, 0.4f, 0.0f);
                    gameState = VICTORY;
                }
            }

            // Hit enemy
            for (auto& e : enemies) {
                if (!e.active) continue;
                if (rectHit(b.x-5, b.y-5, 10, 10,
                            e.x-e.w/2, e.y-e.h/2, e.w, e.h)) {
                    b.active = false;
                    e.health--;
                    spawnParticles(b.x, b.y, 10, 1.0f, 0.65f, 0.15f);
                    if (e.health <= 0) {
                        e.active = false;
                        int pts = (e.type == 3) ? 100 : (e.type == 2) ? 75 : 50;
                        score += pts;
                        enemiesKilled++;
                        spawnParticles(e.x, e.y, 28, 1.0f, 0.48f, 0.0f);

                        if (currentLevel != SUN_BOSS && enemiesKilled >= killsNeeded) {
                            gameState = LEVEL_COMPLETE;
                        }
                    }
                    break;
                }
            }
        }
    }

    // --- Ram collisions ---
    for (auto& e : enemies) {
        if (!e.active) continue;
        if (rectHit(player.x-21, player.y-13, 42, 26,
                    e.x-e.w/2,   e.y-e.h/2,   e.w, e.h)) {
            e.active = false;
            pilotHP -= 10;
            spawnParticles(e.x, e.y, 35, 1.0f, 0.45f, 0.0f);
            if (pilotHP <= 0) { pilotHP = 0; gameState = GAME_OVER; }
        }
    }
    if (boss.active && rectHit(player.x-21, player.y-13, 42, 26,
                               boss.x-50, boss.y-50, 100, 100)) {
        pilotHP -= 30;
        spawnParticles(player.x, player.y, 45, 1.0f, 0.3f, 0.0f);
        if (pilotHP <= 0) { pilotHP = 0; gameState = GAME_OVER; }
    }

    // --- Particle Decay ---
    for (auto& p : particles) {
        p.x += p.vx; p.y += p.vy;
        p.vx *= 0.94f; p.vy *= 0.94f;
        p.life -= dt;
    }

    // --- Cleanup ---
    enemies.erase(  std::remove_if(enemies.begin(),   enemies.end(),[](const Entity&   e){ return !e.active; }), enemies.end());
    bullets.erase(  std::remove_if(bullets.begin(),   bullets.end(),[](const Bullet&   b){ return !b.active; }), bullets.end());
    particles.erase(std::remove_if(particles.begin(), particles.end(),[](const Particle& p){ return p.life <= 0; }), particles.end());

    glutPostRedisplay();
}

// Draw: Vector Illustration Menu Background
void drawVectorMenuBackground() {
    auto drawPolyCircle = [](float cx, float cy, float r, float colR, float colG, float colB, float alpha = 1.0f) {
        glColor4f(colR, colG, colB, alpha);
        glBegin(GL_POLYGON);
        for(int i = 0; i < 360; i += 10) {
            float a = i * PI / 180.0f;
            glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
        }
        glEnd();
    };

    auto drawShadowCrescent = [](float cx, float cy, float r) {
        glColor4f(0.0f, 0.0f, 0.0f, 0.25f);
        glBegin(GL_POLYGON);
        for(int i = 90; i <= 270; i += 5) {
            float a = i * PI / 180.0f;
            glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
        }
        for(int i = 270; i >= 90; i -= 5) {
            float a = i * PI / 180.0f;
            glVertex2f(cx + 15.0f + cosf(a)*(r*0.85f), cy + 15.0f + sinf(a)*(r*0.85f));
        }
        glEnd();
    };

    auto drawSparkle = [](float x, float y, float size) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_POLYGON);
        glVertex2f(x, y + size);
        glVertex2f(x + size*0.15f, y + size*0.15f);
        glVertex2f(x + size, y);
        glVertex2f(x + size*0.15f, y - size*0.15f);
        glVertex2f(x, y - size);
        glVertex2f(x - size*0.15f, y - size*0.15f);
        glVertex2f(x - size, y);
        glVertex2f(x - size*0.15f, y + size*0.15f);
        glEnd();
    };

    auto drawComet = [](float x, float y, float angle, float length) {
        glPushMatrix();
        glTranslatef(x, y, 0);
        glRotatef(angle, 0, 0, 1);
        glBegin(GL_POLYGON);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glVertex2f(0, 4);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glVertex2f(10, 0);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glVertex2f(0, -4);
        glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
        glVertex2f(-length, 0);
        glEnd();
        glPopMatrix();
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Base Space (Deep Purple/Blue)
    glColor3f(0.17f, 0.15f, 0.40f);
    glBegin(GL_POLYGON);
    glVertex2f(0, 0); glVertex2f(WIN_W, 0);
    glVertex2f(WIN_W, WIN_H); glVertex2f(0, WIN_H);
    glEnd();

    // 2. Wavy Background Layers
    glColor3f(0.20f, 0.18f, 0.48f);
    glBegin(GL_POLYGON);
    glVertex2f(0, WIN_H);
    for(float x = 0; x <= WIN_W; x += 20)
        glVertex2f(x, 300 + sinf(x*0.003f)*150);
    glVertex2f(WIN_W, WIN_H);
    glEnd();

    glColor3f(0.24f, 0.22f, 0.53f);
    glBegin(GL_POLYGON);
    glVertex2f(WIN_W, 0);
    for(float x = WIN_W; x >= 0; x -= 20)
        glVertex2f(x, 150 + sinf(x*0.004f + 2.0f)*100);
    glVertex2f(0, 0);
    glEnd();

    // 3. Small Background Stars & Dots
    drawSparkle(120, 520, 20);
    drawSparkle(780, 380, 15);
    drawSparkle(820, 280, 25);
    drawSparkle(320, 200, 18);
    drawSparkle(520, 60, 12);
    drawSparkle(260, 240, 10);

    drawPolyCircle(230, 540, 3, 0.9f, 0.5f, 0.2f); // Orange dot
    drawPolyCircle(580, 560, 3, 0.9f, 0.5f, 0.2f);
    drawPolyCircle(740, 280, 3, 0.9f, 0.5f, 0.2f);
    drawPolyCircle(610, 80, 3, 0.2f, 0.7f, 0.9f);  // Cyan dot
    drawPolyCircle(370, 250, 3, 0.2f, 0.7f, 0.9f);

    drawComet(250, 300, -150, 80);
    drawComet(720, 520, -140, 120);
    drawComet(880, 40, -160, 60);

    // 4. Planet: Pink with Craters (Mid Left)
    drawPolyCircle(80, 350, 85, 0.1f, 0.1f, 0.3f, 0.4f); // Halo
    drawPolyCircle(80, 350, 75, 0.93f, 0.43f, 0.53f); // Base
    drawPolyCircle(50, 370, 15, 0.84f, 0.34f, 0.45f); // Craters
    drawPolyCircle(100, 320, 22, 0.84f, 0.34f, 0.45f);
    drawPolyCircle(120, 370, 10, 0.84f, 0.34f, 0.45f);
    drawPolyCircle(60, 310, 12, 0.84f, 0.34f, 0.45f);
    drawShadowCrescent(80, 350, 75);

    // 5. Planet: Small Blue Swirl (Top Center)
    drawPolyCircle(420, 500, 50, 0.1f, 0.1f, 0.3f, 0.4f);
    drawPolyCircle(420, 500, 42, 0.20f, 0.65f, 0.86f);
    // Swirl Stripes
    glColor3f(0.26f, 0.76f, 0.94f);
    glBegin(GL_POLYGON);
    for(float x = 385; x <= 455; x += 5)
        glVertex2f(x, 505 + sinf(x*0.1f)*5);
    for(float x = 455; x >= 385; x -= 5)
    glVertex2f(x, 495 + sinf(x*0.1f)*5);
    glEnd();
    glBegin(GL_POLYGON);
    for(float x = 390; x <= 445; x += 5)
    glVertex2f(x, 485 + sinf(x*0.1f)*5);
    for(float x = 445; x >= 390; x -= 5)
    glVertex2f(x, 475 + sinf(x*0.1f)*5);
    glEnd();
    drawShadowCrescent(420, 500, 42);

    // 6. Planet: Purple with Ring (Top Right)
    float prX = 880, prY = 480, prR = 70;
    drawPolyCircle(prX, prY, prR*1.3f, 0.1f, 0.1f, 0.3f, 0.4f);
    // Back Ring
    glPushMatrix();
    glTranslatef(prX, prY, 0);
    glRotatef(-20, 0, 0, 1);
    glColor3f(0.84f, 0.64f, 0.80f);
    glBegin(GL_POLYGON);
    for(int i = 0; i <= 180; i += 10)
        glVertex2f(cosf(i*PI/180.0f)*140,
                   sinf(i*PI/180.0f)*30);
    for(int i = 180; i >= 0; i -= 10)
    glVertex2f(cosf(i*PI/180.0f)*110,
               sinf(i*PI/180.0f)*20);
    glEnd();
    glPopMatrix();
    // Planet Base
    drawPolyCircle(prX, prY, prR, 0.57f, 0.20f, 0.50f);
    // Planet Stripes
    glColor3f(0.47f, 0.15f, 0.40f);
    glBegin(GL_POLYGON);
    for(float x=-55; x<=55; x+=5)
    glVertex2f(prX+x, prY+15);
    for(float x=55; x>=-55; x-=5)
    glVertex2f(prX+x, prY+5);
    glEnd();
    drawShadowCrescent(prX, prY, prR);
    // Front Ring
    glPushMatrix();
    glTranslatef(prX, prY, 0);
    glRotatef(-20, 0, 0, 1);
    glColor3f(0.90f, 0.70f, 0.85f);
    glBegin(GL_POLYGON);
    for(int i = 180; i <= 360; i += 10)
    glVertex2f(cosf(i*PI/180.0f)*140,
               sinf(i*PI/180.0f)*35);
    for(int i = 360; i >= 180; i -= 10)
        glVertex2f(cosf(i*PI/180.0f)*110,
                   sinf(i*PI/180.0f)*25);
    glEnd();
    glPopMatrix();

    // 7. Planet: Orange Striped (Bottom Right)
    drawPolyCircle(800, 150, 125, 0.1f, 0.1f, 0.3f, 0.4f);
    drawPolyCircle(800, 150, 115, 0.90f, 0.56f, 0.20f);

    glColor3f(0.94f, 0.66f, 0.25f);
    for (float y = 70; y <= 230; y += 30) {
        float dy1 = (y - 150), dy2 = (y + 12 - 150);
        if (dy1*dy1 > 13000 || dy2*dy2 > 13000) continue;
        float hw1 = sqrtf(13225 - dy1*dy1), hw2 = sqrtf(13225 - dy2*dy2);
        glBegin(GL_POLYGON);
        for(float x = 800 - hw2; x <= 800 + hw2; x += 10)
            glVertex2f(x, y + 12 + sinf(x*0.04f)*8);
        for(float x = 800 + hw1; x >= 800 - hw1; x -= 10)
        glVertex2f(x, y + sinf(x*0.04f)*8);
        glEnd();
    }

    drawShadowCrescent(800, 150, 115);

    // 8. Planet: Earth-like (Bottom Left)
    drawPolyCircle(120, 80, 160, 0.1f, 0.1f, 0.3f, 0.4f);
    drawPolyCircle(120, 80, 145, 0.43f, 0.78f, 0.82f); // Water

    glColor3f(0.12f, 0.70f, 0.47f); // Landmass 1
    glBegin(GL_POLYGON);
        glVertex2f(0, 180);
        glVertex2f(80, 190);
        glVertex2f(120, 130);
        glVertex2f(100, 70);
        glVertex2f(50, 50);
        glVertex2f(-10, 80);
    glEnd();

    glColor3f(0.12f, 0.70f, 0.47f); // Landmass 2
    glBegin(GL_POLYGON);
        glVertex2f(160, 40);
        glVertex2f(230, 50);
        glVertex2f(260, -20);
        glVertex2f(200, -50);
        glVertex2f(140, -10);
    glEnd();
    drawShadowCrescent(120, 80, 145);

    // 9. Rocket (Bottom Center)
    glPushMatrix();
    glTranslatef(440, 180, 0);
    glRotatef(45, 0, 0, 1);

    // Engine Flame
    glColor3f(0.93f, 0.58f, 0.15f); // Orange
    glBegin(GL_POLYGON);
    glVertex2f(-15, -35);
    glVertex2f(15, -35);
    glVertex2f(0, -80);
    glEnd();
    glColor3f(0.95f, 0.80f, 0.20f); // Yellow
    glBegin(GL_POLYGON);
    glVertex2f(-8, -35);
    glVertex2f(8, -35);
    glVertex2f(0, -60);
     glEnd();

    // Red Fins
    glColor3f(0.94f, 0.35f, 0.40f);
    glBegin(GL_POLYGON);
    glVertex2f(-20, -25);
    glVertex2f(-40, -45);
    glVertex2f(-30, 0);
    glVertex2f(-20, 10);
    glEnd(); // Left
    glBegin(GL_POLYGON);
    glVertex2f(20, -25);
    glVertex2f(40, -45);
    glVertex2f(30, 0);
    glVertex2f(20, 10);
    glEnd();   // Right

    // White Fuselage
    glColor3f(0.95f, 0.95f, 0.95f);
    glBegin(GL_POLYGON);
        glVertex2f(-20, -35);
        glVertex2f(20, -35);
        glVertex2f(25, -5);
        glVertex2f(20, 25);
        glVertex2f(0, 50);
        glVertex2f(-20, 25);
        glVertex2f(-25, -5);
    glEnd();

    // Red Nose
    glColor3f(0.94f, 0.35f, 0.40f);
    glBegin(GL_POLYGON);
    glVertex2f(-18, 28);
    glVertex2f(18, 28);
    glVertex2f(0, 50); glEnd();

    // Window
    drawPolyCircle(0, 0, 14, 0.94f, 0.35f, 0.40f); // Red border
    drawPolyCircle(0, 0, 9, 0.20f, 0.65f, 0.86f);  // Cyan inner

    glPopMatrix();
    glDisable(GL_BLEND);
}



// Display
void display() {
    glClearColor(0.0f, 0.02f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);


    // ===== MENU =====
   if (gameState == MENU) {

        // 1. Draw the vector illustration background
        drawVectorMenuBackground();

        // 2. HUD Panel Background (Sleeker, perfectly framed)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float boxTop = WIN_H/2 + 170.0f;
        float boxBot = WIN_H/2 - 20.0f;
        float boxL = WIN_W/2 - 280.0f;
        float boxR = WIN_W/2 + 280.0f;

        // Dark transparent fill
        glColor4f(0.04f, 0.05f, 0.15f, 0.75f);
        glBegin(GL_POLYGON);
            glVertex2f(boxL, boxTop);
            glVertex2f(boxR, boxTop);
            glVertex2f(boxR, boxBot);
            glVertex2f(boxL, boxBot);
        glEnd();

        // Subtle Cyan Border
        glColor4f(0.43f, 0.78f, 0.82f, 0.4f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(boxL, boxTop);
            glVertex2f(boxR, boxTop);
            glVertex2f(boxR, boxBot);
            glVertex2f(boxL, boxBot);
        glEnd();
        glLineWidth(1.0f);
        glDisable(GL_BLEND);

        // 3. Helper Lambda to Perfectly Center Text
        auto drawTextCentered = [](float y, const std::string& text, void* font) {
            int width = 0;
            for(char c : text) width += glutBitmapWidth(font, c);
            glRasterPos2f(WIN_W/2.0f - width/2.0f, y);
            for(char c : text) glutBitmapCharacter(font, c);
        };

        // 4. Draw Typography
        // Title
        glColor3f(1.0f, 1.0f, 1.0f);
        drawTextCentered(WIN_H/2 + 130, "SECTOR  7  :  PLANETARY  DEFENSE", GLUT_BITMAP_HELVETICA_18);

        // Subtitle
        glColor3f(0.7f, 0.8f, 0.9f);
        drawTextCentered(WIN_H/2 + 100, "Defend the L-5 Colony from alien UFO invasions!", GLUT_BITMAP_HELVETICA_12);
        glColor3f(0.43f, 0.78f, 0.82f);
        drawTextCentered(WIN_H/2 + 80, "Group 01 Computer Graphics Sec F", GLUT_BITMAP_HELVETICA_12);
        // Separator Line
        //glColor3f(0.43f, 0.78f, 0.82f);
        //glBegin(GL_LINES);
           // glVertex2f(WIN_W/2 - 120, WIN_H/2 + 85);
            //glVertex2f(WIN_W/2 + 120, WIN_H/2 + 85);
        //glEnd();

        // Levels List
        glColor3f(0.43f, 0.78f, 0.82f); // Earth Blue
        drawTextCentered(WIN_H/2 + 55, "LEVEL 1   -   EARTH SKIES", GLUT_BITMAP_HELVETICA_12);

        glColor3f(0.93f, 0.43f, 0.53f); // Pink Venus
        drawTextCentered(WIN_H/2 + 35, "LEVEL 2   -   VENUS ASSAULT", GLUT_BITMAP_HELVETICA_12);

        glColor3f(0.84f, 0.64f, 0.80f); // Purple Mercury
        drawTextCentered(WIN_H/2 + 15, "LEVEL 3   -   MERCURY STRIKE", GLUT_BITMAP_HELVETICA_12);

        glColor3f(0.94f, 0.66f, 0.25f); // Orange Sun Boss
        drawTextCentered(WIN_H/2 - 5,  "LEVEL 4   -   SUN TITAN BOSS", GLUT_BITMAP_HELVETICA_12);

        // Action Prompt (Positioned safely above the rocket's tip)
        glColor3f(1.0f, 0.90f, 0.20f);
        drawTextCentered(WIN_H/2 - 65, "P R E S S   E N T E R   T O   L A U N C H", GLUT_BITMAP_HELVETICA_18);

        // Controls Header (Positioned at the very bottom edge of the screen)
        glColor3f(0.6f, 0.65f, 0.75f);
        drawTextCentered(25, "WASD / ARROW KEYS : Move     |     SPACE : Fire     |     ESC : Quit", GLUT_BITMAP_HELVETICA_12);

        glutSwapBuffers(); return;
    }





    // ===== LEVEL COMPLETE =====
    if (gameState == LEVEL_COMPLETE) {
        glColor3f(0.15f, 0.75f, 0.25f);
        drawText(WIN_W/2-85, WIN_H/2+60, "LEVEL COMPLETE!",
                 GLUT_BITMAP_TIMES_ROMAN_24);

        std::string nextLevel;
        if      (currentLevel == EARTH)   nextLevel = "VENUS ASSAULT";
        else if (currentLevel == VENUS)   nextLevel = "MERCURY STRIKE";
        else if (currentLevel == MERCURY) nextLevel = "SUN TITAN BOSS";

        glColor3f(0.85f, 0.85f, 0.90f);
        drawText(WIN_W/2-80, WIN_H/2+20, "Preparing next wave...", GLUT_BITMAP_HELVETICA_12);
        glColor3f(1.0f, 0.75f, 0.25f);
        drawText(WIN_W/2-60, WIN_H/2-5, nextLevel, GLUT_BITMAP_HELVETICA_18);

        glutSwapBuffers(); return;
    }






    // ===== VICTORY =====
    if (gameState == VICTORY) {
        glColor3f(0.25f, 0.85f, 0.35f);
        drawText(WIN_W/2-110, WIN_H/2+70, "VICTORY!",
                 GLUT_BITMAP_TIMES_ROMAN_24);

        glColor3f(0.75f, 0.85f, 0.95f);
        drawText(WIN_W/2-145, WIN_H/2+30,
                 "You defeated the Sun Titan and saved Sector 7!",
                 GLUT_BITMAP_HELVETICA_12);

        std::ostringstream ss;
        ss << "FINAL SCRAP:  " << score;
        glColor3f(0.95f, 0.75f, 0.10f);
        drawText(WIN_W/2-95, WIN_H/2-10, ss.str(), GLUT_BITMAP_HELVETICA_18);

        glColor3f(0.45f, 0.85f, 0.45f);
        drawText(WIN_W/2-90, WIN_H/2-65,
                 "Press ENTER to play again", GLUT_BITMAP_HELVETICA_18);

        glutSwapBuffers(); return;
    }





    // ===== GAME OVER =====
    if (gameState == GAME_OVER) {
        glColor3f(0.75f, 0.08f, 0.08f);
        drawText(WIN_W/2-90, WIN_H/2+70, "COLONY LOST",
                 GLUT_BITMAP_TIMES_ROMAN_24);

        glColor3f(0.65f, 0.65f, 0.70f);
        drawText(WIN_W/2-125, WIN_H/2+30,
                 "The invasion overwhelmed Sector 7.", GLUT_BITMAP_HELVETICA_12);

        std::ostringstream ss;
        ss << "SCRAP COLLECTED:  " << score;
        glColor3f(0.95f, 0.75f, 0.10f);
        drawText(WIN_W/2-105, WIN_H/2-10, ss.str(), GLUT_BITMAP_HELVETICA_18);

        glColor3f(0.45f, 0.85f, 0.45f);
        drawText(WIN_W/2-90, WIN_H/2-65,
                 "Press ENTER to try again", GLUT_BITMAP_HELVETICA_18);

        glutSwapBuffers(); return;
    }




    // 1. Draw level-specific background FIRST (full canvas)
    drawLevelBackground();

    // 2. Game objects on top
    drawParticles();


    // 3. Bullets
    for (auto& b : bullets) {
        if (!b.active) continue;
        glColor3f(b.r*0.55f, b.g*0.55f, b.b*0.55f);
        glLineWidth(3.5f);
        glBegin(GL_LINES);
            glVertex2f(b.x, b.y);
            glVertex2f(b.x - b.vx*5, b.y - b.vy*5);
        glEnd();
        glLineWidth(1.0f);
        glColor3f(b.r, b.g, b.b);
        glPointSize(5.5f);
        glBegin(GL_POINTS); glVertex2f(b.x, b.y); glEnd();
    }


    // 4. Enemies
    for (auto& e : enemies) {
        if (!e.active) continue;


        // Draw the correct ship for this level
        if      (e.type == 1) drawEarthEnemy(e.x, e.y, gameTime);
        else if (e.type == 2) drawVenusUFO(e.x, e.y, gameTime);
        else if (e.type == 3) drawMercuryUFO(e.x, e.y, gameTime);


        // Health bar (multi-hit enemies)
        if (e.health > 1) {
            glColor3f(0.08f, 0.08f, 0.08f);
            glBegin(GL_QUADS);
                glVertex2f(e.x-20, e.y-25); glVertex2f(e.x+20, e.y-25);
                glVertex2f(e.x+20, e.y-20); glVertex2f(e.x-20, e.y-20);
            glEnd();
            float r, g, b;
            getLevelColors(r, g, b);
            glColor3f(r, g, b);
            int maxHp = (e.type == 3) ? 3 : 2;
            float hw = 40.0f * (e.health / (float)maxHp);
            glBegin(GL_QUADS);
                glVertex2f(e.x-20,    e.y-25);
                glVertex2f(e.x-20+hw, e.y-25);
                glVertex2f(e.x-20+hw, e.y-20);
                glVertex2f(e.x-20,    e.y-20);
            glEnd();
        }
    }

    // 5. Boss
    if (boss.active) drawSunBoss(boss.x, boss.y, gameTime);



    // 6. Player
    if (player.active) {
        if (currentLevel == EARTH) {
            // Earth: Old rugged prototype ship
            drawPlayer(player.x, player.y);
        } else if (currentLevel == VENUS) {
            // Venus: Upgraded sleek aerospace fighter
            drawModernPlayer(player.x, player.y);
        } else if (currentLevel == MERCURY) {
            // Mercury: Ultimate Solar Interceptor (heat-resistant, previous highest tier)
            drawMercuryPlayer(player.x, player.y);
        } else if (currentLevel == SUN_BOSS) {
            // Sun Boss: Haunted Solar Destroyer (final, most powerful and unsettling form)
            drawHauntedSolarDestroyer(player.x, player.y);
        }
    }

    // 7. UI panels (always on top)
    drawUI();
    drawHUD();

    glutSwapBuffers();
}

// Input
void keyDown(unsigned char k, int, int) {
    keys[k] = true;
    if (k == 13) {
        if (gameState == MENU || gameState == GAME_OVER || gameState == VICTORY) {
            initGame(); gameState = PLAYING;
        }
    }
    if (k == 27) exit(0);
}
void keyUp(unsigned char k, int, int)     { keys[k]  = false; }
void specialDown(int k, int, int)         { skeys[k] = true;  }
void specialUp(int k, int, int)           { skeys[k] = false; }

// Reshape
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("SECTOR 7 : PLANETARY DEFENSE");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    initGame();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);

    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}
