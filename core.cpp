#include "game.h"

// ======================================================================
// Global Definitions
// ======================================================================
GameState gameState = TITLE_SCREEN;
int currentLevel = 1;
Player player;
std::vector<Enemy> enemies;
std::vector<Obstacle> obstacles;
Vector3 exitPosition;
bool exitActive = false;
std::vector<Particle> particles;
std::vector<RelicOrb> relicOrbs;
std::vector<TrailPoint> weaponTrail;
std::vector<Notification> notifications;
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
    // Persist inventory
    int mercy = player.mercyRelics;
    int discipline = player.disciplineRelics;
    int fortitude = player.fortitudeRelics;

    player = {};
    player.mercyRelics = mercy;
    player.disciplineRelics = discipline;
    player.fortitudeRelics = fortitude;

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

    // Border (Ancient Shard wall)
    int border = 85;
    for (int i = 0; i < 40; i++) {
        float angle = (float)i / 40.0f * 2.0f * PI;
        obstacles.push_back({OBS_SHARD, {cosf(angle)*border, 0, sinf(angle)*border}, (float)GetRandomValue(0, 360), 4.5f, (float)GetRandomValue(35, 65)});
    }

    if (currentLevel == 1) {
        // Level 1: Garden of Serenity (Trees and Shards)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-border+25, border-25);

        // Trees of Light
        for (int i = 0; i < 12; i++) {
            float tx = dis(gen);
            float tz = dis(gen);
            if (Vector3Distance({tx, 0, tz}, {0, 0, 0}) < 20.0f) continue;
            obstacles.push_back({OBS_TREE, {tx, 0, tz}, (float)GetRandomValue(0, 360), 2.2f, (float)GetRandomValue(18, 32)});
        }

        // Ancient Shards
        for (int i = 0; i < 15; i++) {
            float sx = dis(gen);
            float sz = dis(gen);
            if (Vector3Distance({sx, 0, sz}, {0, 0, 0}) < 20.0f) continue;
            obstacles.push_back({OBS_SHARD, {sx, 0, sz}, (float)GetRandomValue(0, 360), 3.5f, (float)GetRandomValue(8, 22)});
        }

        // Floating Debris
        for (int i = 0; i < 40; i++) {
            float dx = dis(gen);
            float dz = dis(gen);
            float dy = (float)GetRandomValue(18, 55);
            obstacles.push_back({OBS_DEBRIS, {dx, dy, dz}, 0, 0, 0}); 
        }

        // Enemies
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
                    if (obs.type != OBS_DEBRIS && Vector3Distance(pos, {obs.pos.x, 0, obs.pos.z}) < (obs.radius + 6.0f)) {
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
            e.trait = static_cast<SpiritTrait>(GetRandomValue(0, 4));

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
        // Level 2: Astral Courtyard (Archways and Monoliths)
        player.position = {0, 0, -75.0f};
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-border+30, border-30);

        // Sacred Archways (In a circle)
        for (int i = 0; i < 8; i++) {
            float ang = (float)i * 45.0f * DEG2RAD;
            float r = 45.0f;
            obstacles.push_back({OBS_ARCH, {cosf(ang)*r, 0, sinf(ang)*r}, (float)i * 45.0f, 6.0f, 22.0f});
        }

        // Marble Monoliths (Shards used as pillars)
        for (int i = 0; i < 12; i++) {
            float ang = (float)GetRandomValue(0, 359) * DEG2RAD;
            float r = (float)GetRandomValue(20, 75);
            obstacles.push_back({OBS_SHARD, {cosf(ang)*r, 0, sinf(ang)*r}, (float)GetRandomValue(0, 360), 4.5f, (float)GetRandomValue(25, 55)});
        }

        // Floating Sanctuary Platforms
        for (int i = 0; i < 30; i++) {
            float dx = dis(gen);
            float dz = dis(gen);
            float dy = (float)GetRandomValue(25, 60);
            obstacles.push_back({OBS_DEBRIS, {dx, dy, dz}, 0, 0, 0}); 
        }

        // Sacred Statues
        for (int i = 0; i < 4; i++) {
            float ang = (float)i * 90.0f * DEG2RAD + 22.5f*DEG2RAD;
            float r = 60.0f;
            obstacles.push_back({OBS_STATUE, {cosf(ang)*r, 0, sinf(ang)*r}, (float)i*90.0f, 3.5f, 22.0f});
        }

        // Tough Guard Souls
        for (int i = 0; i < 3; i++) {
            float ang = (float)i / 3.0f * 2.0f * PI + 45.0f*DEG2RAD;
            float r = 35.0f;
            Enemy e{};
            e.type = TANK;
            e.position = {cosf(ang)*r, 0, sinf(ang)*r};
            e.homePosition = e.position;
            e.patrolTarget = e.position;
            e.patrolRadius = 15.0f; 
            e.alive = true;
            e.scale = 1.35f;
            e.health = 420; e.maxHealth = 420;
            e.poise = 180; e.maxPoise = 180;
            e.speed = ENEMY_BASE_SPEED * 0.85f;
            e.bodyColor = {35, 35, 40, 255};
            e.attackDamage = 52.0f; e.poiseDamage = 65.0f; e.attackDur = 0.62f; e.dodgeChance = 0.2f;
            e.trait = static_cast<SpiritTrait>(GetRandomValue(0, 4));
            enemies.push_back(e);
        }

        exitPosition = {0, 0, 85.0f};
    }
    else if (currentLevel == 3) {
        // Level 3: Inner Sanctum (Celestial Cathedral)
        player.position = {0, 0, -65.0f};

        // Cathedral Nave (Side Monoliths)
        for (int z = -80; z <= 80; z += 20) {
            float h = (float)GetRandomValue(45, 65);
            obstacles.push_back({OBS_SHARD, {-35.0f, 0, (float)z}, 0, 6.0f, h});
            obstacles.push_back({OBS_SHARD, { 35.0f, 0, (float)z}, 0, 6.0f, h});
        }

        // Circular Apse (Archways at the back)
        int numArches = 6;
        float radius = 45.0f;
        for (int i = 0; i < numArches; i++) {
            float ang = (float)i / (numArches-1) * PI; // Semi-circle
            obstacles.push_back({OBS_ARCH, {cosf(ang) * radius, 0, 40.0f + sinf(ang) * 20.0f}, (float)i*30.0f, 5.0f, 35.0f});
        }

        // Floating "Halo" Rubble
        for (int i = 0; i < 25; i++) {
            float ang = (float)GetRandomValue(0, 359) * DEG2RAD;
            float r = (float)GetRandomValue(50, 90);
            float h = (float)GetRandomValue(30, 70);
            obstacles.push_back({OBS_DEBRIS, {cosf(ang)*r, 100.0f + h, sinf(ang)*r}, 0, 0, 0});
        }

        // The Corrupted Arbiter (Final Form)
        Enemy boss{};
        
        // Sacred Altars (Detailed geometry)
        Vector3 altarPositions[] = { {-15, 0, 20}, {15, 0, 20}, {-15, 0, 50}, {15, 0, 50} };
        for(auto& ap : altarPositions) {
            obstacles.push_back({OBS_ALTAR, ap, 0, 4.0f, 6.0f}); 
            // Heart of Faith (Floating above)
            obstacles.push_back({OBS_DEBRIS, {ap.x, 10.0f, ap.z}, 0, 0, 0}); 
        }
        boss.type = BOSS;
        boss.position = {0, 0, 35.0f};
        boss.homePosition = boss.position;
        boss.scale = 2.65f;
        boss.health = 2200;
        boss.maxHealth = 2200;
        boss.poise = 420.0f;
        boss.maxPoise = 420.0f;
        boss.speed = ENEMY_BASE_SPEED * 0.95f;
        boss.bodyColor = GOLD;
        boss.attackDamage = 58.0f;
        boss.poiseDamage = 82.0f;
        boss.attackDur = 0.50f;
        boss.dodgeChance = 0.48f;
        enemies.push_back(boss);
    }

    gameState = PLAYING;
}

// ======================================================================
// Core Update & Camera
// ======================================================================
void UpdateGame(float dt) {
    // Notifications should always update to prevent them getting stuck
    for (auto it = notifications.begin(); it != notifications.end(); ) {
        it->timer -= GetFrameTime();
        if (it->timer <= 0) it = notifications.erase(it);
        else ++it;
    }

    UpdateCamera(dt);

    if (gameState != PLAYING) return;

    float effectiveDt = dt;
    if (hitStopTimer > 0.0f) {
        hitStopTimer -= dt;
        if (hitStopTimer <= 0.0f) hitStopTimer = 0.0f;
        effectiveDt = 0.0f;
    }

    UpdatePlayer(effectiveDt);
    UpdateEnemies(effectiveDt);
    UpdateParticles(effectiveDt);

    // Apply Passive Buffs from Relics
    player.maxHealth = MAX_PLAYER_HEALTH + (player.fortitudeRelics * 20);
    player.weapon.poiseDamageMultiplier = 1.0f + (player.disciplineRelics * 0.08f);

    // Update Relic Orbs (Collection)
    for (auto& orb : relicOrbs) {
        if (!orb.active) continue;
        if (Vector3Distance(player.position, orb.pos) < 4.0f) {
            orb.active = false;
            
            Notification n{};
            n.pos = orb.pos;
            n.timer = 2.0f;
            
            if (orb.type == RELIC_MERCY) {
                player.mercyRelics++;
                n.text = "COLLECTED: RELIC OF MERCY";
                n.color = GOLD;
            } else if (orb.type == RELIC_DISCIPLINE) {
                player.disciplineRelics++;
                n.text = "COLLECTED: RELIC OF DISCIPLINE";
                n.color = SKYBLUE;
            } else {
                player.fortitudeRelics++;
                n.text = "COLLECTED: RELIC OF FORTITUDE";
                n.color = WHITE;
            }
            
            // Collection Particles
            for(int i=0; i<12; i++) {
                Particle p{};
                p.position = player.position;
                p.velocity = { (float)GetRandomValue(-20,20)/10.0f, (float)GetRandomValue(10,30)/10.0f, (float)GetRandomValue(-20,20)/10.0f };
                p.lifetime = p.maxLife = 0.6f;
                p.color = n.color;
                p.size = 0.4f;
                particles.push_back(p);
            }
            
            notifications.push_back(n);
        }
    }

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