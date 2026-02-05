#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <random>

// ======================================================================
// Constants & Configuration
// ======================================================================
const int SCREEN_WIDTH = 1440;
const int SCREEN_HEIGHT = 810;
const float BASE_PLAYER_SPEED = 7.4f;
const float SPRINT_MULTIPLIER = 1.85f;
const float EXHAUSTED_MULTIPLIER = 0.45f;
const float ATTACK_RANGE = 6.2f;
const float ROLL_DURATION = 0.22f;
const float ROLL_DISTANCE = 13.0f;
const float ROLL_COST = 18.0f;
const float PERFECT_ROLL_WINDOW = 0.10f;
const int MAX_PLAYER_HEALTH = 420;
const int MAX_STAMINA = 145;
const float STAMINA_REGEN_RATE = 38.0f;
const float STAMINA_SPRINT_COST = 14.0f;
const float STAMINA_ATTACK_COST = 22.0f;
const float STAMINA_POWER_COST = 52.0f;
const float STAMINA_PARRY_COST = 28.0f;
const float NORMAL_ATTACK_DURATION = 0.42f;
const float POWER_ATTACK_CHARGE = 0.85f;
const float POWER_ATTACK_DURATION = 1.05f;
const float COMBO_RESET_TIME = 1.1f;
const float REGEN_DELAY_AFTER_ACTION = 0.8f;
const float CAMERA_DISTANCE = 19.0f;
const float CAMERA_HEIGHT = 15.2f;
const float CAMERA_SMOOTH = 13.5f;
const float MOUSE_SENSITIVITY = 0.28f;
const int MAX_FLASKS = 4;
const float FLASK_HEAL_AMOUNT = 135.0f;
const float FLASK_USE_TIME = 1.35f;
const float ENEMY_BASE_SPEED = 7.9f;
const float ENEMY_ATTACK_DURATION = 0.45f;
const float GRAVITY = -32.0f;
const float JUMP_VELOCITY = 14.0f;
const float COLLISION_RADIUS_BASE = 6.8f;

// ======================================================================
// Enums
// ======================================================================
enum GameState { TITLE_SCREEN, INSTRUCTIONS, PLAYING, PAUSED, DEAD, VICTORY };enum EnemyType { GRUNT, TANK, AGILE, BOSS };
enum EnemyState { PATROL, ALERT, CHASE, SEARCH, STAGGERED };
enum AttackType { LIGHT_1, LIGHT_2, LIGHT_3, HEAVY, DASH_ATTACK };

// ======================================================================
// Structs
// ======================================================================
struct Particle {
    Vector3 position;
    Vector3 velocity;
    float lifetime;
    float maxLife;
    Color color;
    float size;
};

struct TrailPoint {
    Vector3 pos;
    float time;
};

struct Weapon {
    std::string name;
    float damageMultiplier;
    float poiseDamageMultiplier;
    float length;
    Color bladeColor;
    bool hasGlow;
};

struct Player {
    Vector3 position {0,0,0};
    Vector3 velocity {0,0,0};
    float yVelocity = 0.0f;
    float rotation = 0.0f;
    Weapon weapon {"Ashen Greatblade", 1.0f, 1.0f, 6.8f, {180,200,255,255}, true};
    int comboStep = 0;
    float comboTimer = 0.0f;
    bool isAttacking = false;
    float attackTimer = 0.0f;
    AttackType currentAttack = LIGHT_1;
    bool isCharging = false;
    float chargeTimer = 0.0f;
    bool powerReady = false;
    bool isRolling = false;
    float rollTimer = 0.0f;
    Vector3 rollDirection {0,0,0};
    bool isParrying = false;
    float parryTimer = 0.0f;
    int health = MAX_PLAYER_HEALTH;
    int maxHealth = MAX_PLAYER_HEALTH;
    float stamina = MAX_STAMINA;
    float staminaRegenDelay = 0.0f;
    int flasks = MAX_FLASKS;
    float poise = 120.0f;
    float maxPoise = 120.0f;
    float staggerTimer = 0.0f;
    float hitInvuln = 0.0f;
    int lockedTarget = -1;
    float targetSwitchCooldown = 0.0f;
    Vector3 bladeStart, bladeEnd;
    float swingYaw = 30.0f;
    float swingPitch = -30.0f;
    float shakeTimer = 0.0f;
    bool isDead = false;
    float deathTimer = 0.0f;
    float deathFallAngle = 0.0f;
    bool isHealing = false;
    float healTimer = 0.0f;
    float perfectRollTimer = 0.0f;
    float riposteTimer = 0.0f;
};

struct Enemy {
    EnemyType type;
    Vector3 position {0,0,0};
    Vector3 velocity {0,0,0};
    float rotation = 0.0f;
    int health = 220;
    int maxHealth = 220;
    float stamina = MAX_STAMINA;
    float staminaRegenDelay = 0.0f;
    float poise = 80.0f;
    float maxPoise = 80.0f;
    bool alive = true;
    bool isAttacking = false;
    float attackTimer = 0.0f;
    AttackType currentAttack = LIGHT_1;
    bool isHeavyAttack = false;
    bool isDodging = false;
    float dodgeTimer = 0.0f;
    Vector3 dodgeDirection {0,0,0};
    bool isBlocking = false;
    float blockTimer = 0.0f;
    float hitInvuln = 0.0f;
    float stunTimer = 0.0f;
    float flinchTimer = 0.0f;
    EnemyState state = PATROL;
    Vector3 homePosition;
    float patrolRadius = 22.0f;
    Vector3 patrolTarget;
    float patrolTimer = 0.0f;
    Vector3 lastKnownPlayerPos;
    float alertTimer = 0.0f;
    Color bodyColor;
    float scale = 1.0f;
    float speed = ENEMY_BASE_SPEED;
    float strafeSide = 1.0f;
    float strafeTimer = 4.0f;
    float attackCooldown = 0.0f;
    Vector3 bladeStart, bladeEnd;
    float swingYaw = 30.0f;
    float swingPitch = -30.0f;
    float attackDamage = 32.0f;
    float poiseDamage = 38.0f;
    float attackDur = ENEMY_ATTACK_DURATION;
    float dodgeChance = 0.55f;
    int comboStep = 0;
    float comboDelayTimer = 0.0f;
};

// ======================================================================
// Globals (declared extern)
// ======================================================================
extern GameState gameState;
extern int currentLevel;
extern Player player;
extern std::vector<Enemy> enemies;
extern std::vector<Vector3> obstacles;
extern Vector3 exitPosition;
extern bool exitActive;
extern std::vector<Particle> particles;
extern std::vector<TrailPoint> weaponTrail;
extern Camera3D camera;
extern Shader bloomShader;
extern RenderTexture2D target;
extern Vector3 camPos;
extern float hitStopTimer;
extern std::vector<std::string> deathMessages;
extern const char* currentDeathMessage;

// ======================================================================
// Function Prototypes
// ======================================================================
void InitGame();
void ResetLevel();
void UpdateGame(float dt);
void UpdatePlayer(float dt);
void UpdateEnemies(float dt);
void UpdateParticles(float dt);
void UpdateCamera(float dt);
void Draw3DScene();
void DrawPlayer();
void DrawEnemy(const Enemy& e, int index);
void DrawHUD();
void DrawTitleScreen();
void DrawInstructionsScreen();
void DrawDeathScreen();
void DrawVictoryScreen();
void SpawnDataParticles(Vector3 pos, int count = 12);
void SpawnHitSparks(Vector3 pos, int count = 8);
void AddWeaponTrailPoint();
bool CanSeePlayer(const Enemy& e);
bool IsEnemyAttackSwingHittingPlayer(const Enemy& e);
void ApplyEnemyHitToPlayer(const Enemy& e);
bool CheckPlayerAttackHitEnemy(Enemy& e);

#endif
