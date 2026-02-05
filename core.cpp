#include "game.h"

// ======================================================================
// Global Definitions
// ======================================================================
GameState gameState = TITLE_SCREEN;
int currentLevel = 1;
Player player;
std::vector<Enemy> enemies;
std::vector<Vector3> obstacles;
Vector3 exitPosition;
bool exitActive = false;
std::vector<Particle> particles;
std::vector<TrailPoint> weaponTrail;
Camera3D camera = { 0 };
Shader bloomShader = { 0 };
RenderTexture2D target = { 0 };
Vector3 camPos = {0, CAMERA_HEIGHT, CAMERA_DISTANCE};
float hitStopTimer = 0.0f;
std::vector<std::string> deathMessages = {
    "Spirit Banished", "Vessel Shattered", "Divine Connection Lost", "Faith Tested",
    "Fallen from Grace", "Returning to Light", "Trial Incomplete", "Purification Failed",
    "Ascension Delayed", "Seek Forgiveness", "Soul Recalibrating"
};
const char* currentDeathMessage = "Spirit Banished";

// ======================================================================
// Initialization & Level Generation
// ======================================================================
void InitGame() {
    camera.fovy = 62.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.up = {0,1,0};

    target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Simple bloom shader (extract bright parts and blur)
    const char* bloomFrag = 
        "#version 330\n"
        "in vec2 fragTexCoord;\n"
        "in vec4 fragColor;\n"
        "uniform sampler2D texture0;\n"
        "uniform vec4 colDiffuse;\n"
        "out vec4 finalColor;\n"
        "void main() {\n"
        "    vec4 texel = texture(texture0, fragTexCoord);\n"
        "    vec3 bloom = vec3(0.0);\n"
        "    float threshold = 0.85;\n"
        "    if (length(texel.rgb) > threshold) bloom = texel.rgb;\n"
        "    // Simple 3x3 blur approximation\n"
        "    vec2 size = vec2(1.0/1280.0, 1.0/720.0);\n"
        "    for (int x = -1; x <= 1; x++) {\n"
        "        for (int y = -1; y <= 1; y++) {\n"
        "            vec3 sample = texture(texture0, fragTexCoord + vec2(x, y) * size * 2.0).rgb;\n"
        "            if (length(sample) > threshold) bloom += sample * 0.1;\n"
        "        }\n"
        "    }\n"
        "    finalColor = vec4(texel.rgb + bloom * 0.3, texel.a);\n"
        "}\n";

    bloomShader = LoadShaderFromMemory(NULL, bloomFrag);
}

void ResetLevel() {
    player = {};
    player.position = {0,0,0};
    player.health = MAX_PLAYER_HEALTH;
    player.maxHealth = MAX_PLAYER_HEALTH;
    player.stamina = MAX_STAMINA;
    player.flasks = MAX_FLASKS;
    player.poise = 120.0f;
    player.maxPoise = 120.0f;
    player.weapon = {"Divine Scepter", 1.0f, 1.0f, 6.8f, {255, 215, 0, 255}, true};
    player.swingYaw = 30.0f;
    player.swingPitch = -30.0f;

    enemies.clear();
    obstacles.clear();
    particles.clear();
    weaponTrail.clear();
    hitStopTimer = 0.0f;
    exitActive = false;

    // Border walls
    int border = 80;
    int step = 12;
    for (int i = -border; i <= border; i += step) {
        obstacles.push_back({(float)i, 0, (float)-border});
        obstacles.push_back({(float)i, 0, (float)border});
        obstacles.push_back({(float)-border, 0, (float)i});
        obstacles.push_back({(float)border, 0, (float)i});
    }

    if (currentLevel == 1) {
        // Random pillars in open field
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-border+15, border-15);
        for (int i = 0; i < 90; i++) {
            float x = dis(gen);
            float z = dis(gen);
            if (Vector3Distance({x,0,z}, {0,0,0}) > 18.0f) {
                obstacles.push_back({x, 0, z});
            }
        }

        // Enemies
        const int MAX_ENEMIES = 14;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Vector3 pos;
            bool valid = false;
            int attempts = 0;
            while (!valid && attempts < 60) {
                attempts++;
                float angle = GetRandomValue(0, 359) * DEG2RAD;
                float dist = GetRandomValue(18, 75);
                pos = { cosf(angle)*dist, 0, sinf(angle)*dist };
                valid = Vector3Distance(pos, {0,0,0}) > 16.0f;
                for (const auto& obs : obstacles) {
                    if (Vector3Distance(pos, obs) < 9.0f) {
                        valid = false;
                        break;
                    }
                }
            }
            if (!valid) continue;

            Enemy e{};
            e.position = pos;
            e.homePosition = pos;
            e.patrolTarget = pos;
            e.patrolRadius = GetRandomValue(16, 32);
            e.alive = true;
            e.swingYaw = 30.0f;
            e.swingPitch = -30.0f;
            e.attackCooldown = (float)GetRandomValue(0, 100) / 100.0f;
            e.strafeTimer = (float)GetRandomValue(30, 80) / 10.0f;
            e.strafeSide = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;

            int typeRoll = GetRandomValue(0, 100);
            if (typeRoll < 45) {
                e.type = GRUNT; // Lesser Imp
                e.scale = 0.95f;
                e.health = 180; e.maxHealth = 180;
                e.poise = 65; e.maxPoise = 65;
                e.speed = ENEMY_BASE_SPEED * 1.05f;
                e.bodyColor = {110, 45, 130, 255}; // Desaturated Void Purple
                e.attackDamage = 31.0f;
                e.poiseDamage = 36.0f;
                e.attackDur = 0.43f;
                e.dodgeChance = 0.52f;
            }
            else if (typeRoll < 80) {
                e.type = TANK; // Hell-Guard
                e.scale = 1.28f;
                e.health = 340; e.maxHealth = 340;
                e.poise = 160; e.maxPoise = 160;
                e.speed = ENEMY_BASE_SPEED * 0.82f;
                e.bodyColor = {35, 35, 40, 255}; // Cold Charcoal
                e.patrolRadius *= 0.7f;
                e.attackDamage = 46.0f;
                e.poiseDamage = 60.0f;
                e.attackDur = 0.60f;
                e.dodgeChance = 0.25f;
            }
            else {
                e.type = AGILE; // Shadow Stalker
                e.scale = 1.05f;
                e.health = 160; e.maxHealth = 160;
                e.poise = 55; e.maxPoise = 55;
                e.speed = ENEMY_BASE_SPEED * 1.25f;
                e.bodyColor = {45, 40, 80, 255}; // Shadow Indigo
                e.attackDamage = 27.0f;
                e.poiseDamage = 32.0f;
                e.attackDur = 0.36f;
                e.dodgeChance = 0.82f;
            }
            enemies.push_back(e);
        }

        // Exit portal position
        do {
            exitPosition.x = GetRandomValue(-border+25, border-25);
            exitPosition.z = GetRandomValue(-border+25, border-25);
        } while (Vector3Distance(exitPosition, {0,0,0}) < 55.0f);
        exitPosition.y = 0;
    }
    else if (currentLevel == 2) {
        player.position = {0, 0, -35.0f};

        // Boss arena pillars
        int numPillars = 16;
        float radius = 45.0f;
        for (int i = 0; i < numPillars; i++) {
            float ang = (float)i / numPillars * 2 * PI;
            Vector3 pos = {cosf(ang) * radius, 0, sinf(ang) * radius};
            obstacles.push_back(pos);
        }
        numPillars = 8;
        radius = 20.0f;
        for (int i = 0; i < numPillars; i++) {
            float ang = (float)i / numPillars * 2 * PI + PI / 16.0f;
            Vector3 pos = {cosf(ang) * radius, 0, sinf(ang) * radius};
            obstacles.push_back(pos);
        }

        // Boss
        Enemy boss{};
        boss.type = BOSS;
        boss.position = {0, 0, 40.0f};
        boss.homePosition = boss.position;
        boss.scale = 2.3f;
        boss.health = 1600;
        boss.maxHealth = 1600;
        boss.poise = 320.0f;
        boss.maxPoise = 320.0f;
        boss.speed = ENEMY_BASE_SPEED * 0.88f;
        boss.bodyColor = {251, 188, 5, 255}; // Google Yellow
        boss.attackDamage = 48.0f;
        boss.poiseDamage = 72.0f;
        boss.attackDur = 0.55f;
        boss.dodgeChance = 0.35f;
        enemies.push_back(boss);
    }

    gameState = PLAYING;
}

// ======================================================================
// Core Update & Camera
// ======================================================================
void UpdateGame(float dt) {
    UpdateCamera(dt);

    float effectiveDt = dt;
    if (hitStopTimer > 0.0f) {
        hitStopTimer -= dt;
        if (hitStopTimer <= 0.0f) hitStopTimer = 0.0f;
        effectiveDt = 0.0f;
    }

    UpdatePlayer(effectiveDt);
    UpdateEnemies(effectiveDt);
    UpdateParticles(effectiveDt);

    // Floating ash particles
    if (GetRandomValue(0, 30) == 0) {
        float x = player.position.x + GetRandomValue(-80, 80);
        float z = player.position.z + GetRandomValue(-80, 80);
        Vector3 pos = {x, 35.0f + GetRandomValue(0, 20), z};
        Particle p{};
        p.position = pos;
        p.velocity = {GetRandomValue(-8, 8)/10.0f, -2.2f, GetRandomValue(-8, 8)/10.0f};
        p.lifetime = p.maxLife = 20.0f;
        p.color = Fade(GRAY, 0.35f);
        p.size = GetRandomValue(3, 8)/10.0f;
        particles.push_back(p);
    }

    // Victory conditions
    int aliveCount = 0;
    for (const auto& e : enemies) if (e.alive) aliveCount++;

    if (currentLevel == 1) {
        exitActive = (aliveCount == 0);
    } else if (currentLevel == 2 && aliveCount == 0 && gameState == PLAYING) {
        gameState = VICTORY;
    }
}

void UpdateCamera(float dt) {
    Vector3 desiredTarget = Vector3Add(player.position, {0, 2.0f, 0});

    if (player.lockedTarget != -1 && enemies[player.lockedTarget].alive) {
        Enemy& tgt = enemies[player.lockedTarget];
        desiredTarget = Vector3Lerp(desiredTarget, Vector3Add(tgt.position, {0, 2.8f, 0}), 0.55f);
        Vector3 to = Vector3Subtract(tgt.position, player.position);
        to.y = 0;
        float len = Vector3Length(to);
        if (len > 0.6f) {
            player.rotation = atan2f(to.x, to.z) * RAD2DEG;
        }
    }

    float rad = player.rotation * DEG2RAD;
    Vector3 behind = Vector3Scale({sinf(rad), 0, cosf(rad)}, -CAMERA_DISTANCE);
    Vector3 desiredPos = Vector3Add(player.position, Vector3Add(behind, {0, CAMERA_HEIGHT, 0}));

    camPos = Vector3Lerp(camPos, desiredPos, CAMERA_SMOOTH * dt);
    Vector3 camTarget = Vector3Lerp(camera.target, desiredTarget, CAMERA_SMOOTH * dt);

    Vector3 shake{0};
    if (player.shakeTimer > 0) {
        player.shakeTimer -= dt;
        float str = player.shakeTimer * 60.0f;
        shake.x = GetRandomValue(-100,100)/1000.0f * str;
        shake.y = GetRandomValue(-100,100)/1000.0f * str;
        shake.z = GetRandomValue(-100,100)/1000.0f * str;
    }

    camera.position = Vector3Add(camPos, shake);
    camera.target = camTarget;
}