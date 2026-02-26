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
    "Returning to Light", "Spirit Recalibrating", "Divine Connection Renewing", "Faith Tested",
    "Resting in Grace", "Ascension Delayed", "Trial of Patience", "Purification Ongoing",
    "Seeking Guidance", "Soul Rejuvenating"
};
const char* currentDeathMessage = "Returning to Light";

// ======================================================================
// Initialization & Level Generation
// ======================================================================
void InitGame() {
    camera.fovy = 62.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.up = {0,1,0};

    target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Simple bloom shader (extract bright parts and blur)
    // Updated for cross-platform compatibility
    const char* bloomFrag = 
#if defined(PLATFORM_WEB) || defined(GRAPHICS_API_OPENGL_ES3)
        "#version 300 es\n"
        "precision highp float;\n"
#else
        "#version 330\n"
#endif
        "in vec2 fragTexCoord;\n"
        "in vec4 fragColor;\n"
        "uniform sampler2D texture0;\n"
        "uniform vec4 colDiffuse;\n"
        "uniform float chromaticAmount;\n"
        "out vec4 finalColor;\n"
        "void main() {\n"
        "    vec2 uv = fragTexCoord;\n"
        "    float amount = chromaticAmount;\n"
        "    \n"
        "    // Chromatic Aberration\n"
        "    vec3 col;\n"
        "    col.r = texture(texture0, vec2(uv.x + amount, uv.y)).r;\n"
        "    col.g = texture(texture0, uv).g;\n"
        "    col.b = texture(texture0, vec2(uv.x - amount, uv.y)).b;\n"
        "    \n"
        "    vec3 bloom = vec3(0.0);\n"
        "    float threshold = 0.82;\n"
        "    \n"
        "    vec2 size = vec2(1.0/1440.0, 1.0/810.0);\n"
        "    for (int x = -1; x <= 1; x++) {\n"
        "        for (int y = -1; y <= 1; y++) {\n"
        "            vec3 sampleColor = texture(texture0, uv + vec2(float(x), float(y)) * size * 2.0).rgb;\n"
        "            float brightness = max(sampleColor.r, max(sampleColor.g, sampleColor.b));\n"
        "            if (brightness > threshold) bloom += sampleColor * 0.12;\n"
        "        }\n"
        "    }\n"
        "    finalColor = vec4(col + bloom * 0.55, 1.0);\n"
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
    player.weapon = {"Scepter of Dawn", 1.0f, 1.0f, 6.8f, {255, 240, 150, 255}, true};
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
        obstacles.push_back({(float)i, 22.0f, (float)-border});
        obstacles.push_back({(float)i, 22.0f, (float)border});
        obstacles.push_back({(float)-border, 22.0f, (float)i});
        obstacles.push_back({(float)border, 22.0f, (float)i});
    }

    if (currentLevel == 1) {
        // Level 1: Garden of Serenity (Organic Clusters)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-border+25, border-25);

        // Circular clusters to create "Islands"
        for (int c = 0; c < 18; c++) {
            float cx = dis(gen);
            float cz = dis(gen);
            if (Vector3Distance({cx, 0, cz}, {0, 0, 0}) < 22.0f) continue;

            int clusterSize = GetRandomValue(4, 9);
            float clusterRadius = (float)GetRandomValue(6, 14);
            for (int i = 0; i < clusterSize; i++) {
                float ang = (float)GetRandomValue(0, 359) * DEG2RAD;
                float dist = (float)GetRandomValue(0, (int)clusterRadius);
                float h = (float)GetRandomValue(10, 32);
                obstacles.push_back({cx + cosf(ang)*dist, h, cz + sinf(ang)*dist});
            }
        }

        // Floating Debris (Atmosphere)
        for (int i = 0; i < 50; i++) {
            float dx = dis(gen);
            float dz = dis(gen);
            float dy = (float)GetRandomValue(18, 55);
            obstacles.push_back({dx, 100.0f + dy, dz}); 
        }

        // Enemies (Reduced count for higher quality encounters)
        const int MAX_ENEMIES = 6;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Vector3 pos;
            bool valid = false;
            int attempts = 0;
            while (!valid && attempts < 80) {
                attempts++;
                float angle = GetRandomValue(0, 359) * DEG2RAD;
                float dist = GetRandomValue(20, 75);
                pos = { cosf(angle)*dist, 0, sinf(angle)*dist };
                valid = Vector3Distance(pos, {0,0,0}) > 18.0f;
                for (const auto& obs : obstacles) {
                    if (obs.y < 100.0f && Vector3Distance(pos, {obs.x, 0, obs.z}) < 10.0f) {
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
            e.patrolRadius = GetRandomValue(18, 35);
            e.alive = true;
            e.swingYaw = 30.0f;
            e.swingPitch = -30.0f;
            e.attackCooldown = (float)GetRandomValue(0, 100) / 100.0f;
            e.strafeTimer = (float)GetRandomValue(30, 80) / 10.0f;
            e.strafeSide = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;

            int typeRoll = GetRandomValue(0, 100);
            if (typeRoll < 60) { e.type = GRUNT; e.scale = 0.95f; e.health = 180; e.maxHealth = 180; e.poise = 65; e.maxPoise = 65; e.speed = ENEMY_BASE_SPEED * 1.05f; e.bodyColor = {110, 45, 130, 255}; e.attackDamage = 31.0f; e.poiseDamage = 36.0f; e.attackDur = 0.43f; e.dodgeChance = 0.52f; }
            else { e.type = AGILE; e.scale = 1.05f; e.health = 160; e.maxHealth = 160; e.poise = 55; e.maxPoise = 55; e.speed = ENEMY_BASE_SPEED * 1.25f; e.bodyColor = {45, 40, 80, 255}; e.attackDamage = 27.0f; e.poiseDamage = 32.0f; e.attackDur = 0.36f; e.dodgeChance = 0.82f; }
            enemies.push_back(e);
        }

        // Exit portal
        do {
            exitPosition.x = GetRandomValue(-border+30, border-30);
            exitPosition.z = GetRandomValue(-border+30, border-30);
        } while (Vector3Distance(exitPosition, {0,0,0}) < 60.0f);
        exitPosition.y = 0;
    }
    else if (currentLevel == 2) {
        // Level 2: Astral Courtyard (Spacious & Organic)
        player.position = {0, 0, -75.0f};
        
        // Circular Layout with inner hubs
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-border+30, border-30);

        // Large Sacred Monoliths
        for (int i = 0; i < 14; i++) {
            float ang = (float)GetRandomValue(0, 359) * DEG2RAD;
            float dist = (float)GetRandomValue(25, 70);
            float h = (float)GetRandomValue(20, 45);
            obstacles.push_back({cosf(ang)*dist, h, sinf(ang)*dist});
        }

        // Small Prayer Altars (lower cover)
        for (int i = 0; i < 25; i++) {
            float ang = (float)GetRandomValue(0, 359) * DEG2RAD;
            float dist = (float)GetRandomValue(15, 65);
            obstacles.push_back({cosf(ang)*dist, 10.0f, sinf(ang)*dist});
        }

        // Floating Sanctuary Platforms
        for (int i = 0; i < 30; i++) {
            float dx = dis(gen);
            float dz = dis(gen);
            float dy = (float)GetRandomValue(25, 60);
            obstacles.push_back({dx, 100.0f + dy, dz}); 
        }

        // Tough Guard Souls (Reduced count, increased intelligence)
        for (int i = 0; i < 3; i++) {
            float ang = (float)i / 7.0f * 2.0f * PI;
            float r = 40.0f;
            Enemy e{};
            e.type = TANK;
            e.position = {cosf(ang)*r, 0, sinf(ang)*r};
            e.homePosition = e.position;
            e.patrolTarget = e.position;
            e.patrolRadius = 15.0f; // Much larger patrol area
            e.alive = true;
            e.scale = 1.35f;
            e.health = 420; e.maxHealth = 420;
            e.poise = 180; e.maxPoise = 180;
            e.speed = ENEMY_BASE_SPEED * 0.85f;
            e.bodyColor = {35, 35, 40, 255};
            e.attackDamage = 52.0f; e.poiseDamage = 65.0f; e.attackDur = 0.62f; e.dodgeChance = 0.2f;
            enemies.push_back(e);
        }

        exitPosition = {0, 0, 85.0f};
    }
    else if (currentLevel == 3) {
        // Level 3: Inner Sanctum (Final Boss)
        player.position = {0, 0, -45.0f};

        // Arena circle
        int numPillars = 20;
        float radius = 55.0f;
        for (int i = 0; i < numPillars; i++) {
            float ang = (float)i / numPillars * 2 * PI;
            obstacles.push_back({cosf(ang) * radius, 32.0f, sinf(ang) * radius});
        }

        // Boss
        Enemy boss{};
        boss.type = BOSS;
        boss.position = {0, 0, 45.0f};
        boss.homePosition = boss.position;
        boss.scale = 2.45f;
        boss.health = 1850;
        boss.maxHealth = 1850;
        boss.poise = 380.0f;
        boss.maxPoise = 380.0f;
        boss.speed = ENEMY_BASE_SPEED * 0.92f;
        boss.bodyColor = GOLD;
        boss.attackDamage = 54.0f;
        boss.poiseDamage = 78.0f;
        boss.attackDur = 0.52f;
        boss.dodgeChance = 0.45f;
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

    // Update chromatic aberration intensity
    if (bloomShader.id > 0) {
        int chromLoc = GetShaderLocation(bloomShader, "chromaticAmount");
        float chromVal = 0.0012f + player.shakeTimer * 0.045f;
        SetShaderValue(bloomShader, chromLoc, &chromVal, SHADER_UNIFORM_FLOAT);
    }

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

    if (currentLevel == 1 || currentLevel == 2) {
        exitActive = (aliveCount == 0);
    } else if (currentLevel == 3 && aliveCount == 0 && gameState == PLAYING) {
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