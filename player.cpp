#include "game.h"

void AddWeaponTrailPoint() {
    static float trailTimer = 0;
    trailTimer += GetFrameTime();
    if (trailTimer < 0.016f) return;
    trailTimer = 0;

    if (player.isAttacking || player.isCharging) {
        weaponTrail.push_back({player.bladeEnd, 0.0f});
    }

    for (auto it = weaponTrail.begin(); it != weaponTrail.end(); ) {
        it->time += GetFrameTime();
        if (it->time > 0.5f) {
            it = weaponTrail.erase(it);
        } else {
            ++it;
        }
    }
}

void UpdatePlayer(float dt) {
    bool attackInput = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool attackRelease = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    player.perfectRollTimer = std::max(0.0f, player.perfectRollTimer - dt);
    player.riposteTimer = std::max(0.0f, player.riposteTimer - dt);

    if (player.isDead) {
        player.deathTimer -= dt;
        player.deathFallAngle = Lerp(player.deathFallAngle, 90.0f, 5.0f * dt);
        return;
    }

    player.hitInvuln -= dt;
    player.staggerTimer -= dt;
    player.targetSwitchCooldown -= dt;
    player.comboTimer -= dt;

    // Mouse look
    Vector2 mouseDelta = GetMouseDelta();
    float sens = MOUSE_SENSITIVITY;
    if (player.isAttacking || player.isParrying || player.staggerTimer > 0) sens *= 0.4f;
    player.rotation -= mouseDelta.x * sens;

    // Target lock
    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
        if (player.lockedTarget != -1) {
            player.lockedTarget = -1;
        } else {
            float bestDist = 1e9f;
            int best = -1;
            for (size_t i = 0; i < enemies.size(); i++) {
                if (!enemies[i].alive) continue;
                float dist = Vector3Distance(player.position, enemies[i].position);
                if (dist > 45.0f) continue;
                Vector3 to = Vector3Subtract(enemies[i].position, player.position);
                to.y = 0;
                float angDiff = fabsf(atan2f(to.x, to.z) - player.rotation*DEG2RAD);
                if (angDiff > PI) angDiff = 2*PI - angDiff;
                float score = dist * 0.6f + angDiff * 30.0f;
                if (score < bestDist) {
                    bestDist = score;
                    best = (int)i;
                }
            }
            player.lockedTarget = best;
        }
    }

    // Target switch with mouse flick
    if (player.lockedTarget != -1 && player.targetSwitchCooldown <= 0.0f) {
        float flick = 110.0f;
        int dir = 0;
        if (mouseDelta.x > flick) dir = 1;
        else if (mouseDelta.x < -flick) dir = -1;
        if (dir != 0) {
            struct Candidate { float angle; int idx; };
            std::vector<Candidate> cands;
            for (size_t i = 0; i < enemies.size(); i++) {
                if (!enemies[i].alive) continue;
                Vector3 to = Vector3Subtract(enemies[i].position, player.position);
                float d = Vector3Length(to);
                if (d > 55.0f) continue;
                float ang = atan2f(to.x, to.z) - player.rotation*DEG2RAD;
                ang = fmodf(ang + 5*PI, 2*PI) - PI;
                cands.push_back({ang, (int)i});
            }
            if (cands.size() > 1) {
                std::sort(cands.begin(), cands.end(), [](const Candidate& a, const Candidate& b){
                    return a.angle < b.angle;
                });
                int curr = -1;
                for (size_t j = 0; j < cands.size(); j++) {
                    if (cands[j].idx == player.lockedTarget) { curr = (int)j; break; }
                }
                if (curr != -1) {
                    int next = (curr + dir + (int)cands.size()) % (int)cands.size();
                    player.lockedTarget = cands[next].idx;
                    player.targetSwitchCooldown = 0.35f;
                }
            }
        }
    }

    if (player.lockedTarget != -1) {
        if (!enemies[player.lockedTarget].alive ||
            Vector3Distance(player.position, enemies[player.lockedTarget].position) > 60.0f) {
            player.lockedTarget = -1;
        }
    }

    // Movement input
    Vector3 moveInput{0,0,0};
    if (IsKeyDown(KEY_W)) moveInput.z += 1;
    if (IsKeyDown(KEY_S)) moveInput.z -= 1;
    if (IsKeyDown(KEY_D)) moveInput.x -= 1;
    if (IsKeyDown(KEY_A)) moveInput.x += 1;
    bool hasMoveInput = Vector3Length(moveInput) > 0.01f;
    if (hasMoveInput) moveInput = Vector3Normalize(moveInput);

    Vector3 moveDir{0,0,0};
    float rad = player.rotation * DEG2RAD;
    Vector3 camForward{ sinf(rad), 0, cosf(rad) };
    Vector3 camRight { cosf(rad), 0,-sinf(rad) };

    if (player.lockedTarget == -1) {
        moveDir = Vector3Add(Vector3Scale(camForward, moveInput.z),
                             Vector3Scale(camRight, moveInput.x));
    } else {
        Vector3 toTarget = Vector3Subtract(enemies[player.lockedTarget].position, player.position);
        toTarget.y = 0;
        float len = Vector3Length(toTarget);
        Vector3 targetForward = (len > 0.4f) ? Vector3Normalize(toTarget) : camForward;
        Vector3 targetRight { targetForward.z, 0, -targetForward.x };
        moveDir = Vector3Add(Vector3Scale(targetForward, moveInput.z),
                             Vector3Scale(targetRight, moveInput.x));
    }

    // Movement logic
    float speed = BASE_PLAYER_SPEED;
    static bool sprintLock = false;
    if (player.stamina <= 0.0f) sprintLock = true;
    if (sprintLock && player.stamina >= MAX_STAMINA * 0.25f) sprintLock = false;

    bool sprinting = IsKeyDown(KEY_LEFT_SHIFT) && hasMoveInput && !sprintLock && !player.isRolling;
    if (sprinting) {
        speed *= SPRINT_MULTIPLIER;
        player.stamina -= STAMINA_SPRINT_COST * dt;
        player.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;

        // Angelic Flight particles
        if (GetRandomValue(0, 4) == 0) {
            Particle p{};
            p.position = Vector3Add(player.position, {0, 1.5f, 0});
            p.velocity = {(float)GetRandomValue(-20, 20)/40.0f, 1.5f, (float)GetRandomValue(-20, 20)/40.0f};
            p.lifetime = p.maxLife = 0.5f;
            p.color = Fade(WHITE, 0.6f);
            p.size = 0.35f;
            particles.push_back(p);
        }
    }
    if (player.stamina <= 0.0f) speed *= EXHAUSTED_MULTIPLIER;

    if (player.isAttacking || player.isRolling || player.isParrying || player.isHealing || player.staggerTimer > 0) {
        speed *= 0.38f;
    }

    // Roll
    if (IsKeyPressed(KEY_LEFT_SHIFT) && hasMoveInput && player.stamina > 0.0f &&
        !player.isAttacking && !player.isRolling && !player.isParrying && !player.isHealing && player.staggerTimer <= 0) {
        player.isRolling = true;
        player.rollTimer = ROLL_DURATION;
        player.rollDirection = moveDir;
        player.stamina -= ROLL_COST;
        player.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
        player.hitInvuln = ROLL_DURATION + 0.15f;
    }

    float rollSpeed =  ROLL_DISTANCE / ROLL_DURATION;
    Vector3 targetVelocity = Vector3Scale(moveDir, speed);

    if (player.isRolling) {
        targetVelocity = Vector3Scale(player.rollDirection, rollSpeed);
        player.rollTimer -= dt;

        // Divine Lunge follow-up
        if (attackInput && player.rollTimer > 0.05f && player.stamina > 0.0f) {
            player.isRolling = false;
            player.isAttacking = true;
            player.attackTimer = NORMAL_ATTACK_DURATION * 0.95f;
            player.currentAttack = LIGHT_1;
            player.comboStep = 1;
            player.stamina -= STAMINA_ATTACK_COST;
            player.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
            player.velocity = Vector3Add(player.velocity, Vector3Scale(player.rollDirection, 22.0f));

            // Burst of Light
            for (int i = 0; i < 15; i++) {
                Particle p{};
                p.position = Vector3Add(player.position, {0, 1.2f, 0});
                p.velocity = Vector3Scale({(float)GetRandomValue(-100, 100)/10.0f, 0.5f, (float)GetRandomValue(-100, 100)/10.0f}, 1.2f);
                p.lifetime = p.maxLife = 0.45f;
                p.color = WHITE;
                p.size = 0.4f;
                particles.push_back(p);
            }
        }

        bool perfectWindowActive = (player.rollTimer < PERFECT_ROLL_WINDOW);
        if (perfectWindowActive && player.perfectRollTimer <= 0.0f) {
            for (auto& e : enemies) {
                if (!e.alive || !e.isAttacking) continue;
                float dur = e.attackDur * (e.isHeavyAttack ? 1.75f : 1.0f);
                float prog = 1.0f - (e.attackTimer / dur);
                float hitStart = e.isHeavyAttack ? 0.22f : 0.20f;
                float hitEnd = e.isHeavyAttack ? 0.85f : 0.80f;
                if (prog > hitStart && prog < hitEnd && IsEnemyAttackSwingHittingPlayer(e)) {
                    player.perfectRollTimer = 1.5f;
                    player.stamina = std::min(player.stamina + 24.0f, (float)MAX_STAMINA);
                    SpawnHitSparks(player.position, 18);
                    hitStopTimer = std::max(hitStopTimer, 0.04f);
                    break;
                }
            }
        }

        if (player.rollTimer <= 0.0f) {
            player.isRolling = false;
        }
    }

    // Footstep Audio
    static float stepTimer = 0.0f;
    if (hasMoveInput && !player.isRolling && player.position.y <= 0.5f) {
        float stepInterval = sprinting ? 0.25f : 0.38f;
        stepTimer -= dt;
        if (stepTimer <= 0.0f) {
            stepTimer = stepInterval;
            TriggerSFX(4);
        }
    } else {
        stepTimer = 0.0f; // Reset for next step
    }

    // Set velocity (snap to target during roll, allow momentum during attack/stagger)
    if (player.isRolling) {
        player.velocity = targetVelocity;
    } else if (player.isAttacking || player.staggerTimer > 0) {
        // Momentum Preservation Model: Natural decay rather than snapping to zero
        float friction = (player.staggerTimer > 0) ? 2.8f : 1.2f; 
        player.velocity = Vector3Scale(player.velocity, 1.0f - friction * dt);
    } else {
        player.velocity = Vector3Lerp(player.velocity, targetVelocity, 22.0f * dt);
    }

    // Apply movement with obstacle collision
    Vector3 displacement = Vector3Scale(player.velocity, dt);
    Vector3 candidate = {player.position.x + displacement.x, player.position.y, player.position.z + displacement.z};

    bool collision = false;
    for (const auto& obs : obstacles) {
        if (obs.type == OBS_DEBRIS) continue;
        float d = Vector2Distance({candidate.x, candidate.z}, {obs.pos.x, obs.pos.z});
        if (d < (obs.radius + 1.2f)) {
            collision = true;
            break;
        }
    }

    if (!collision) {
        player.position.x = candidate.x;
        player.position.z = candidate.z;
    } else {
        player.velocity = Vector3Scale(player.velocity, 0.15f);
    }

    // Normal movement
    if (!player.isRolling) {
        Vector3 desiredVel = Vector3Scale(moveDir, speed);
        player.velocity = Vector3Lerp(player.velocity, desiredVel, 22.0f * dt);

        Vector3 newPos = Vector3Add(player.position, Vector3Scale(player.velocity, dt));
        bool collision = false;
        for (const auto& obs : obstacles) {
            if (obs.type == OBS_DEBRIS) continue; // Floating debris is not solid
            
            float dist = Vector2Distance({newPos.x, newPos.z}, {obs.pos.x, obs.pos.z});
            float combinedRadius = obs.radius + 1.2f; // Player radius approx 1.2f
            if (dist < combinedRadius) {
                collision = true;
                break;
            }
        }
        if (!collision) {
            player.position.x = newPos.x;
            player.position.z = newPos.z;
        } else {
            player.velocity = Vector3Scale(player.velocity, 0.15f);
        }
    }

    // Gravity & jump
    player.yVelocity += GRAVITY * dt;
    player.position.y += player.yVelocity * dt;
    if (player.position.y < 0.0f) {
        player.position.y = 0.0f;
        player.yVelocity = 0.0f;
    }

    bool grounded = (player.position.y <= 0.05f);
    if (IsKeyPressed(KEY_SPACE) && grounded && player.stamina >= 5.0f &&
        !player.isAttacking && !player.isRolling && !player.isParrying && !player.isHealing &&
        player.staggerTimer <= 0) {
        player.yVelocity = JUMP_VELOCITY;
        player.stamina -= 5.0f;
    }

    // Flask
    if (IsKeyPressed(KEY_E) && player.flasks > 0 && !player.isHealing &&
        !player.isAttacking && !player.isRolling && !player.isParrying && player.staggerTimer <= 0) {
        player.isHealing = true;
        player.healTimer = FLASK_USE_TIME;
        player.flasks--;
    }
    if (player.isHealing) {
        player.healTimer -= dt;
        if (player.healTimer <= 0.0f) {
            float bonus = player.mercyRelics * 10.0f;
            player.health = std::min((float)player.maxHealth, player.health + FLASK_HEAL_AMOUNT + bonus);
            player.isHealing = false;
        }
    }

    // Parry
    if (IsKeyPressed(KEY_LEFT_CONTROL) && player.stamina > 0.0f &&
        !player.isAttacking && !player.isRolling && !player.isHealing && player.staggerTimer <= 0) {
        player.isParrying = true;
        player.parryTimer = 0.38f;
        player.stamina = std::max(0.0f, player.stamina - STAMINA_PARRY_COST);
        player.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
    }
    if (player.isParrying) {
        player.parryTimer -= dt;
        if (player.parryTimer <= 0.0f) player.isParrying = false;
    }

    // Attack logic
    if (attackInput && !player.isCharging && !player.isAttacking && !player.isRolling &&
        !player.isParrying && !player.isHealing && player.stamina > 0.0f &&
        player.staggerTimer <= 0) {
        player.isCharging = true;
        player.chargeTimer = 0.0f;
        player.powerReady = false;
    }

    if (player.isCharging) {
        player.chargeTimer += dt;
        if (player.chargeTimer >= POWER_ATTACK_CHARGE) player.powerReady = true;
    }

    if (attackRelease && player.isCharging) {
        player.isCharging = false;
        bool fatigued = (player.stamina <= 0.0f); // Check if we started attack while exhausted

        if (player.powerReady) {
            player.isAttacking = true;
            player.attackTimer = POWER_ATTACK_DURATION * (fatigued ? 1.4f : 1.0f);
            player.currentAttack = HEAVY;
            player.stamina -= STAMINA_ATTACK_COST * 2.0f; // More consistent cost
            player.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION * 2.0f;
            player.comboStep = 0;
            player.comboTimer = COMBO_RESET_TIME;
        }
        else if (player.stamina > 0.0f) {
            player.isAttacking = true;
            player.stamina -= STAMINA_ATTACK_COST;
            player.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
            if (player.comboTimer <= 0 || player.comboStep == 0) {
                player.comboStep = 1;
            } else {
                player.comboStep = (player.comboStep % 3) + 1;
            }
            player.comboTimer = COMBO_RESET_TIME;
            player.attackTimer = NORMAL_ATTACK_DURATION * (fatigued ? 1.3f : 1.0f);
            player.currentAttack = static_cast<AttackType>(player.comboStep - 1);
        }
        
        if (player.isAttacking) {
            TriggerSFX(1);
            // Forward Lunge Impulse - SIGNIFICANTLY INCREASED
            Vector3 lungeDir = {sinf(player.rotation*DEG2RAD), 0, cosf(player.rotation*DEG2RAD)};
            float power = (player.currentAttack == HEAVY) ? 72.0f : 55.0f;
            player.velocity = Vector3Add(player.velocity, Vector3Scale(lungeDir, power));
        }
        player.powerReady = false;
    }

    // Attack animation & hit detection
    if (player.isAttacking) {
        player.comboTimer = COMBO_RESET_TIME;
        bool fatigued = (player.stamina < 0.0f); // Current state during animation
        float durationBase = (player.currentAttack == HEAVY) ? POWER_ATTACK_DURATION : NORMAL_ATTACK_DURATION;
        float duration = durationBase * (fatigued ? 1.35f : 1.0f);
        float progress = 1.0f - (player.attackTimer / duration);
        
        // Hyperrealistic Strike Acceleration (Ease-In) - ELITE PLAYER SNAP
        float snapProg = powf(progress, 7.0f);

        // Swing animation - CLEAN ARCS IN FRONT
        if (player.currentAttack == HEAVY) {
            float pp = progress * 3.0f;
            if (pp < 1.0f) {
                float spp = powf(pp, 7.0f);
                player.swingYaw = 0.0f;
                player.swingPitch = Lerp(-110.0f, 140.0f, spp);
            } else if (pp < 2.0f) {
                player.swingYaw = 0.0f;
                player.swingPitch = 140.0f;
            } else {
                player.swingYaw = 0.0f;
                player.swingPitch = Lerp(140.0f, 30.0f, pp - 2.0f);
            }
        } else {
            if (player.comboStep == 1) {
                player.swingPitch = Lerp(-85.0f, 90.0f, snapProg);
                player.swingYaw = 0.0f;
            } else if (player.comboStep == 2) {
                player.swingPitch = 0.0f;
                player.swingYaw = Lerp(110.0f, -110.0f, snapProg);
            } else {
                player.swingPitch = Lerp(-45.0f, 45.0f, snapProg);
                player.swingYaw = Lerp(-90.0f, 90.0f, snapProg);
            }
        }

        // Blade position (Update BEFORE collision checks) - FIXED ROTATION MATH
        float totalYaw = (player.rotation + player.swingYaw) * DEG2RAD;
        float pitchRad = player.swingPitch * DEG2RAD;
        Vector3 pivotLocal = {0.65f, 1.65f, 0.4f};
        Vector3 pivotWorld = Vector3Add(player.position,
                                        Vector3RotateByAxisAngle(pivotLocal, {0,1,0}, player.rotation * DEG2RAD));
        Vector3 baseLocal = {0, -0.7f, 0.6f};
        Vector3 tipLocal = {0, -0.7f, player.weapon.length};
        Vector3 base = Vector3RotateByAxisAngle(baseLocal, {1,0,0}, pitchRad);
        base = Vector3RotateByAxisAngle(base, {0,1,0}, totalYaw);
        Vector3 tip = Vector3RotateByAxisAngle(tipLocal, {1,0,0}, pitchRad);
        tip = Vector3RotateByAxisAngle(tip, {0,1,0}, totalYaw);
        player.bladeStart = Vector3Add(pivotWorld, base);
        player.bladeEnd = Vector3Add(pivotWorld, tip);

        // Hit window (Adjusted for Snap Acceleration)
        if (progress > 0.45f && progress < 0.92f) {
            for (auto& e : enemies) {
                CheckPlayerAttackHitEnemy(e);
            }
        }

        player.attackTimer -= dt;
        if (player.attackTimer <= 0.0f) {
            player.isAttacking = false;
        }
    }
    else if (player.staggerTimer <= 0.0f && !player.isRolling && !player.isParrying) {
        player.swingPitch = Lerp(player.swingPitch, -30.0f, 14.0f * dt);
        player.swingYaw = Lerp(player.swingYaw, 30.0f, 14.0f * dt);
        if (player.comboTimer <= 0.0f) player.comboStep = 0;
    }

    AddWeaponTrailPoint();

    // Stamina regen
    player.staminaRegenDelay -= dt;
    bool actionPause = player.isAttacking || player.isRolling || player.isParrying || player.isCharging;
    if (player.staminaRegenDelay <= 0.0f && !actionPause) {
        float regenMult = 1.0f;
        // Detect if in light (Radiant Empowerment)
        for (int i = 0; i < 6; i++) {
            float ang = (float)i * 60.0f * DEG2RAD + GetTime() * 0.03f;
            float dist = 70.0f + 10.0f * sinf(GetTime() * 0.15f + i);
            Vector3 lightPos = {cosf(ang) * dist, 0, sinf(ang) * dist};
            if (Vector2Distance({player.position.x, player.position.z}, {lightPos.x, lightPos.z}) < 15.0f) {
                regenMult = 1.5f;
                break;
            }
        }
        player.stamina = std::min(player.stamina + STAMINA_REGEN_RATE * regenMult * dt, (float)MAX_STAMINA);
    }
}

void DrawPlayer() {
    rlPushMatrix();
    rlTranslatef(player.position.x, player.position.y, player.position.z);
    rlRotatef(player.rotation, 0,1,0);
    if (player.isDead) {
        rlRotatef(player.deathFallAngle, 1,0,0);
    }

    Color divineWhite = {220, 235, 255, 255}; // Moonlight Silver
    Color divineGold = {212, 175, 55, 255};  // Champagne Gold
    Color divineSky = {100, 180, 240, 255};  // Celestial Blue

    Color bodyColor = divineWhite;
    if (player.isHealing) bodyColor = divineSky;
    if (player.staggerTimer > 0) bodyColor = {180, 60, 60, 255}; // Desaturated Red Stagger
    if (player.stamina < 0.0f) bodyColor = {130, 130, 150, 255}; // Exhausted Gray

    // Legs
    DrawCylinderEx({-0.4f, -0.9f, 0}, {-0.4f, 1.0f, 0}, 0.5f, 0.4f, 12, {30, 35, 50, 255}); // Dark Void
    DrawCylinderEx({ 0.4f, -0.9f, 0}, { 0.4f, 1.0f, 0}, 0.5f, 0.4f, 12, {30, 35, 50, 255});
    DrawSphere({-0.4f, -0.9f, 0}, 0.52f, divineWhite);
    DrawSphere({ 0.4f, -0.9f, 0}, 0.52f, divineWhite);

    // Torso (Angelic Armor)
    DrawCube({0, 0.9f, 0}, 1.7f, 2.9f, 1.3f, bodyColor);
    DrawCube({0, 1.1f, 0.45f}, 1.9f, 2.2f, 0.5f, divineGold);

    // Wings
    rlPushMatrix();
    rlTranslatef(0, 1.8f, -0.6f);
    float wingSwing = sinf(GetTime() * 2.0f) * 10.0f;
    // Left Wing
    rlPushMatrix();
    rlRotatef(20 + wingSwing, 0, 1, 0);
    DrawCylinderEx({0, 0, 0}, {-3.5f, 2.5f, -0.5f}, 0.2f, 0.05f, 8, divineWhite);
    DrawCube({-1.75f, 1.25f, -0.25f}, 3.0f, 1.5f, 0.1f, Fade(divineSky, 0.3f));
    rlPopMatrix();
    // Right Wing
    rlPushMatrix();
    rlRotatef(-20 - wingSwing, 0, 1, 0);
    DrawCylinderEx({0, 0, 0}, {3.5f, 2.5f, -0.5f}, 0.2f, 0.05f, 8, divineWhite);
    DrawCube({1.75f, 1.25f, -0.25f}, 3.0f, 1.5f, 0.1f, Fade(divineSky, 0.3f));
    rlPopMatrix();
    rlPopMatrix();

    // Arms & Head
    DrawSphere({-1.0f, 1.9f, 0}, 0.55f, divineWhite);
    DrawSphere({ 1.0f, 1.9f, 0}, 0.55f, divineWhite);
    DrawSphere({0, 2.4f, 0}, 0.65f, divineGold); 
    DrawCircle3D({0, 3.2f, 0}, 0.8f, {1, 0, 0}, 90, divineGold); 

    // Left arm (parry pose)
    rlPushMatrix();
    rlTranslatef(-0.9f, 1.4f, 0);
    float leftAngle = player.isParrying ? 80.0f : -25.0f;
    rlRotatef(leftAngle, 1, 0, 0);
    DrawCylinderEx({0, 0, 0}, {0, -1.4f, 0}, 0.35f, 0.3f, 12, divineWhite);
    DrawSphere({0, -1.4f, 0}, 0.38f, divineGold);
    rlPopMatrix();

    // Neural Link (Divine Thread)
    rlPushMatrix();
    rlTranslatef(0.65f, 1.65f, 0.4f);
    rlRotatef(player.swingYaw, 0, 1, 0);
    rlRotatef(player.swingPitch, 1, 0, 0);

    // Divine Scepter
    DrawCylinderEx({0, -0.4f, 0}, {0, -1.2f, 0}, 0.2f, 0.2f, 16, {80, 70, 60, 255}); // Aged Bronze
    DrawSphere({0, 0, 0}, 0.45f, divineGold);

    // Data Stream (Divine Ray)
    float length = player.weapon.length;
    float thickness = 0.08f;
    
    if (player.isAttacking) {
        float time = (float)GetTime();
        
        // Central Ray
        DrawCylinderEx({0, 0, 0}, {0, 0, length}, thickness, thickness * 0.2f, 12, divineWhite);
        DrawSphere({0, 0, length}, thickness * 4.0f, divineGold);

        // Spiraling Light Strands
        int strands = 2;
        for (int i = 0; i < strands; i++) {
            float angleOffset = (float)i * PI;
            float spiralRadius = 0.4f * sinf(time * 8.0f + i);
            
            Vector3 lastPos = {0, 0, 0};
            int segments = 16;
            for (int s = 1; s <= segments; s++) {
                float t = (float)s / segments;
                float spiralAngle = t * 8.0f + time * 15.0f + angleOffset;
                Vector3 currentPos = {
                    cosf(spiralAngle) * spiralRadius,
                    sinf(spiralAngle) * spiralRadius,
                    t * length
                };
                
                DrawLine3D(lastPos, currentPos, (s % 2 == 0) ? divineGold : divineSky);
                if (s % 4 == 0) DrawSphere(currentPos, 0.08f, WHITE);
                lastPos = currentPos;
            }
        }
    }

    if (player.powerReady) {
        float pulse = 0.4f + 0.4f * sinf(GetTime() * 15.0f);
        DrawSphere({0, 0, 0}, 1.2f, Fade(divineGold, pulse));
    }

    // Avatar of Virtue (Aura based on dominant path)
    Color auraCol = WHITE;
    if (player.mercyLevel >= player.disciplineLevel && player.mercyLevel >= player.fortitudeLevel && player.mercyLevel > 0) auraCol = GOLD;
    else if (player.disciplineLevel > player.mercyLevel && player.disciplineLevel >= player.fortitudeLevel) auraCol = SKYBLUE;

    if (player.mercyLevel > 0 || player.disciplineLevel > 0 || player.fortitudeLevel > 0) {
        float auraTime = GetTime();
        rlPushMatrix();
        rlRotatef(auraTime * 30.0f, 0, 1, 0);
        DrawCircle3D({0, 0.5f, 0}, 1.8f, {1,0,0}, 90.0f, Fade(auraCol, 0.15f));
        DrawCircle3D({0, 0.5f, 0}, 1.8f, {1,0,0}, 90.0f, Fade(auraCol, 0.15f)); 
        rlRotatef(90.0f, 1, 0, 0); // Cross ring
        DrawCircle3D({0, 0, 0}, 1.6f, {0,1,0}, 90.0f, Fade(auraCol, 0.1f));
        rlPopMatrix();
    }

    rlPopMatrix();
    rlPopMatrix();
    }