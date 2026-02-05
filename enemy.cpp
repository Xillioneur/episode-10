#include "game.h"

bool CanSeePlayer(const Enemy& e) {
    Vector3 eye = Vector3Add(e.position, {0,2.4f,0});
    Vector3 target = Vector3Add(player.position, {0,1.6f,0});
    Vector3 dir = Vector3Subtract(target, eye);
    float dist = Vector3Length(dir);
    if (dist > 40.0f) return false;

    Vector3 forward = {sinf(e.rotation*DEG2RAD), 0, cosf(e.rotation*DEG2RAD)};
    float dot = Vector3DotProduct(Vector3Normalize(dir), forward);
    if (dot < cosf(65.0f * DEG2RAD)) return false;

    Ray ray{eye, Vector3Normalize(dir)};
    for (const auto& obs : obstacles) {
        BoundingBox box = {Vector3Subtract(obs, {5,7,5}), Vector3Add(obs, {5,7,5})};
        RayCollision col = GetRayCollisionBox(ray, box);
        if (col.hit && col.distance < dist - 0.8f) return false;
    }
    return true;
}

bool IsEnemyAttackSwingHittingPlayer(const Enemy& e) {
    Vector3 toPlayer = Vector3Subtract(player.position, e.position);
    toPlayer.y = 0.0f;
    float dist = Vector3Length(toPlayer);
    float maxDist = (e.type == BOSS && e.comboStep == 3) ? ATTACK_RANGE + 3.0f : ATTACK_RANGE + 1.2f;
    if (dist > maxDist) return false;

    Vector3 norm = Vector3Normalize(toPlayer);
    Vector3 eFacing = {sinf(e.rotation*DEG2RAD),0,cosf(e.rotation*DEG2RAD)};
    float minDot = (e.type == BOSS && e.comboStep == 3) ? 0.3f : 0.6f;
    if (Vector3DotProduct(eFacing, norm) > minDot) return true;
    return false;
}

void ApplyEnemyHitToPlayer(const Enemy& e) {
    Vector3 toPlayer = Vector3Subtract(player.position, e.position);
    toPlayer.y = 0.0f;
    Vector3 norm = Vector3Normalize(toPlayer);

    float dmgMult = (e.type == BOSS) ? ((e.comboStep == 5) ? 2.1f : ((e.comboStep == 3) ? 1.6f : 1.3f))
                                    : (e.isHeavyAttack ? 1.75f : 1.0f);
    float poiseMult = (e.type == BOSS) ? ((e.comboStep == 5) ? 2.2f : ((e.comboStep == 3) ? 1.8f : 1.4f))
                                      : (e.isHeavyAttack ? 1.85f : 1.0f);
    float knockMult = (e.type == BOSS) ? ((e.comboStep == 5) ? 1.8f : 1.3f)
                                      : (e.isHeavyAttack ? 1.5f : 1.0f);

    int damage = (int)(e.attackDamage * dmgMult);
    float poiseDmg = e.poiseDamage * poiseMult;

    player.health -= damage;
    player.hitInvuln = 0.5f;
    player.velocity = Vector3Add(player.velocity, Vector3Scale(norm, 12.0f * knockMult));

    SpawnDataParticles(player.position, (e.type == BOSS || e.isHeavyAttack) ? 24 : 16);

    float thisStop = (e.type == BOSS) ? ((e.comboStep == 5) ? 0.07f : 0.05f)
                                     : (e.isHeavyAttack ? 0.05f : 0.03f);
    float thisShake = (e.type == BOSS) ? 0.35f : (e.isHeavyAttack ? 0.28f : 0.20f);

    if (player.staggerTimer <= 0.0f) {
        player.poise -= poiseDmg;
        if (player.poise <= 0.0f) {
            player.poise = player.maxPoise;
            player.staggerTimer = 1.5f;
            player.velocity = Vector3Add(player.velocity, Vector3Scale(norm, 24.0f * knockMult));
            thisStop = 0.07f;
            thisShake = 0.42f;
        }
    }

    hitStopTimer = std::max(hitStopTimer, thisStop);
    player.shakeTimer = std::max(player.shakeTimer, thisShake);
}

bool CheckPlayerAttackHitEnemy(Enemy& e) {
    if (!e.alive || e.hitInvuln > 0) return false;

    Vector3 toEnemy = Vector3Subtract(e.position, player.position);
    toEnemy.y = 0;
    float dist = Vector3Length(toEnemy);
    if (dist > ATTACK_RANGE + 1.4f) return false;

    Vector3 normToEnemy = Vector3Normalize(toEnemy);
    Vector3 pFacing = {sinf(player.rotation*DEG2RAD), 0, cosf(player.rotation*DEG2RAD)};
    float dot = Vector3DotProduct(pFacing, normToEnemy);
    float minDot = (player.comboStep == 2) ? -0.45f : 0.35f;
    if (dot < minDot) return false;

    bool isHeavy = (player.currentAttack == HEAVY);
    int baseDamage = isHeavy ? 92 : 62;
    float basePoiseDmg = isHeavy ? 68.0f : 28.0f;

    if (isHeavy) {
        float prog = 1.0f - player.attackTimer / POWER_ATTACK_DURATION;
        float pp = prog * 3.0f;
        if (pp < 1.0f) baseDamage = 35;
        else if (pp < 2.0f) baseDamage = 42;
        else baseDamage = 55;
    }

    Vector3 eFacing = {sinf(e.rotation*DEG2RAD), 0, cosf(e.rotation*DEG2RAD)};
    float backstabDot = Vector3DotProduct(eFacing, normToEnemy);
    bool backstab = backstabDot < -0.75f;
    bool riposte = e.stunTimer > 0;

    float dmgMult = (backstab || riposte) ? 2.6f : 1.0f;
    float poiseMult = (backstab || riposte) ? 2.3f : 1.0f;
    if (e.stamina <= 0) poiseMult *= 1.7f;
    float knockMult = (backstab || riposte) ? 2.1f : 1.0f;

    bool blocked = e.isBlocking && !isHeavy;
    if (blocked) {
        dmgMult *= 0.4f;
        poiseMult *= 0.55f;
        e.isBlocking = false;
        SpawnHitSparks(e.position, 12);
    }

    int damage = (int)(baseDamage * dmgMult * player.weapon.damageMultiplier);
    float poiseDamage = basePoiseDmg * poiseMult * player.weapon.poiseDamageMultiplier;

    e.health -= damage;
    e.hitInvuln = 0.4f;
    e.velocity = Vector3Add(e.velocity, Vector3Scale(normToEnemy, 14.0f * knockMult));

    // Alert the enemy immediately
    e.state = CHASE;
    e.alertTimer = 15.0f;
    e.lastKnownPlayerPos = player.position;
    
    // Snap rotation to face player
    Vector3 faceDir = Vector3Subtract(player.position, e.position);
    if (Vector3Length(faceDir) > 0.1f) {
        e.rotation = atan2f(faceDir.x, faceDir.z) * RAD2DEG;
    }

    // Small flinch reaction for non-bosses
    if (e.type != BOSS && e.stunTimer <= 0.0f) {
        bool shouldFlinch = true;
        if (e.type == TANK && !isHeavy && !backstab && !riposte) shouldFlinch = false;
        
        if (shouldFlinch) {
            e.flinchTimer = isHeavy ? 0.35f : 0.22f;
            e.isAttacking = false; // Interrupt attack
        }
    }

    float thisStop = blocked ? 0.02f : (isHeavy ? 0.05f : 0.03f);
    if (backstab || riposte) thisStop = 0.06f;

    bool poiseBreak = false;
    if (e.stunTimer <= 0) {
        e.poise -= poiseDamage;
        if (e.poise <= 0) {
            e.poise = e.maxPoise;
            e.stunTimer = 2.4f;
            e.velocity = Vector3Add(e.velocity, Vector3Scale(normToEnemy, 26.0f));
            poiseBreak = true;
            thisStop = 0.07f;
        }
    }

    float thisShake = blocked ? 0.10f : (isHeavy ? 0.25f : 0.18f);
    if (backstab || riposte || poiseBreak) thisShake = 0.35f;

    hitStopTimer = std::max(hitStopTimer, thisStop);
    player.shakeTimer = std::max(player.shakeTimer, thisShake);

    if (backstab || riposte) {
        SpawnDataParticles(e.position, 24);
    } else {
        SpawnDataParticles(e.position, 12);
    }

    if (e.health <= 0) {
        e.alive = false;
        SpawnDataParticles(e.position, 30);
    }

    return true;
}

void UpdateEnemies(float dt) {
    for (auto& e : enemies) {
        if (!e.alive) continue;

        e.hitInvuln -= dt;
        e.stunTimer -= dt;
        e.flinchTimer -= dt;
        e.staminaRegenDelay -= dt;
        if (e.staminaRegenDelay <= 0) {
            e.stamina = std::min(e.stamina + 32.0f * dt, (float)MAX_STAMINA);
        }

        if (e.stunTimer > 0 || e.flinchTimer > 0) {
            e.velocity = Vector3Lerp(e.velocity, {0,0,0}, 10.0f * dt);
            continue;
        }

        bool seesPlayer = CanSeePlayer(e);
        Vector3 toPlayer = Vector3Subtract(player.position, e.position);
        toPlayer.y = 0.0f;
        float distToPlayer = Vector3Length(toPlayer);

        Vector3 moveDir{0,0,0};
        float moveSpeed = e.speed * (e.stamina <= 0.0f ? EXHAUSTED_MULTIPLIER : 1.0f);

        if (e.type == BOSS) {
            e.state = CHASE;
            e.alertTimer = 10.0f;
            if (distToPlayer > 0.5f) {
                e.rotation = atan2f(toPlayer.x, toPlayer.z) * RAD2DEG;
            }
            Vector3 forward = Vector3Normalize(toPlayer);
            Vector3 tangent = {forward.z, 0.0f, -forward.x};
            tangent = Vector3Scale(tangent, e.strafeSide * 0.3f);
            moveDir = Vector3Add(forward, tangent);
            moveDir = Vector3Normalize(moveDir);
            moveSpeed *= 1.1f;

            e.comboDelayTimer -= dt;
            Vector3 eFacing = {sinf(e.rotation*DEG2RAD), 0, cosf(e.rotation*DEG2RAD)};
            float dot = Vector3DotProduct(eFacing, Vector3Normalize(toPlayer));
            if (distToPlayer <= ATTACK_RANGE + 5.0f && dot > 0.5f && !e.isAttacking && e.comboDelayTimer <= 0.0f && e.stamina >= 30.0f) {
                e.comboStep = (e.comboStep % 5) + 1;
                if (e.comboStep == 1) e.comboDelayTimer = 2.2f;
                e.isAttacking = true;
                float dur = (e.comboStep == 3 || e.comboStep == 5) ? 0.85f : 0.55f;
                e.attackTimer = dur;
                e.stamina -= 30.0f;
                e.staminaRegenDelay = 1.2f;
            }
        } else {
            // Awareness
            if (seesPlayer) {
                e.lastKnownPlayerPos = player.position;
                e.alertTimer = 12.0f;
                e.state = CHASE;
            } else if (e.alertTimer > 0.0f) {
                e.alertTimer -= dt;
                if (Vector3Distance(e.position, e.lastKnownPlayerPos) < 8.0f) {
                    e.state = SEARCH;
                }
            } else {
                e.state = PATROL;
            }

            e.attackCooldown -= dt;

            bool inCombatRange = (e.state != PATROL) && distToPlayer < 45.0f;
            if (inCombatRange) {
                e.strafeTimer -= dt;
                if (e.strafeTimer <= 0.0f) {
                    e.strafeSide *= -1.0f;
                    e.strafeTimer = (float)GetRandomValue(30, 70) / 10.0f;
                }
            }

            // Patrol behavior
            if (e.state == PATROL) {
                e.patrolTimer -= dt;
                if (e.patrolTimer <= 0.0f || Vector3Distance(e.position, e.patrolTarget) < 6.0f) {
                    float ang = (float)GetRandomValue(0, 359) * DEG2RAD;
                    float r = (float)GetRandomValue(0, (int)e.patrolRadius);
                    e.patrolTarget = Vector3Add(e.homePosition, {cosf(ang)*r, 0.0f, sinf(ang)*r});
                    e.patrolTimer = (float)GetRandomValue(6, 14);
                }
                Vector3 toPatrol = Vector3Subtract(e.patrolTarget, e.position);
                toPatrol.y = 0.0f;
                if (Vector3Length(toPatrol) > 1.0f) {
                    moveDir = Vector3Normalize(toPatrol);
                    moveSpeed *= 0.55f;
                }
                e.rotation = atan2f(toPatrol.x, toPatrol.z) * RAD2DEG;
            } else {
                if (seesPlayer) {
                    e.rotation = atan2f(toPlayer.x, toPlayer.z) * RAD2DEG;
                }

                if (distToPlayer > 45.0f) {
                    moveDir = Vector3Normalize(toPlayer);
                } else {
                    Vector3 forward = Vector3Normalize(toPlayer);
                    Vector3 tangent = {forward.z, 0.0f, -forward.x};
                    tangent = Vector3Scale(tangent, e.strafeSide);
                    float forwardAmt = (distToPlayer > ATTACK_RANGE + 3.0f) ? 0.6f : 0.3f;
                    float strafeAmt = 0.7f;
                    if (e.type == TANK) {
                        forwardAmt = (distToPlayer > ATTACK_RANGE + 3.0f) ? 0.8f : 0.6f;
                        strafeAmt = 0.3f;
                    } else if (e.type == AGILE) {
                        forwardAmt = (distToPlayer > ATTACK_RANGE + 3.0f) ? 0.4f : 0.1f;
                        strafeAmt = 0.9f;
                        moveSpeed *= 1.15f;
                    }
                    moveDir = Vector3Add(Vector3Scale(forward, forwardAmt), Vector3Scale(tangent, strafeAmt));
                    if (Vector3Length(moveDir) > 0.01f) moveDir = Vector3Normalize(moveDir);
                    moveSpeed *= 0.85f;
                }
            }

            // Attack decision
            Vector3 eFacing = {sinf(e.rotation * DEG2RAD), 0.0f, cosf(e.rotation * DEG2RAD)};
            float dot = Vector3DotProduct(eFacing, Vector3Normalize(toPlayer));
            if (distToPlayer <= ATTACK_RANGE + 1.8f && dot > 0.55f && e.attackCooldown <= 0.0f &&
                e.stamina >= 26.0f && !e.isAttacking && !e.isDodging && !e.isBlocking && e.stunTimer <= 0.0f) {
                bool wantHeavy = (e.type == TANK && GetRandomValue(0, 100) < 40);
                bool canHeavy = (e.stamina >= 48.0f);
                e.isHeavyAttack = wantHeavy && canHeavy;
                float staminaCost = e.isHeavyAttack ? 48.0f : 26.0f;
                float durMult = e.isHeavyAttack ? 1.75f : 1.0f;
                e.attackTimer = e.attackDur * durMult;
                e.currentAttack = e.isHeavyAttack ? LIGHT_1 : static_cast<AttackType>(GetRandomValue(0, 2));
                e.isAttacking = true;
                e.stamina -= staminaCost;
                e.staminaRegenDelay = e.isHeavyAttack ? 1.4f : 0.8f;
                float baseCd = (e.type == AGILE) ? 0.9f : ((e.type == TANK) ? 2.5f : 1.6f);
                baseCd += e.isHeavyAttack ? 1.3f : 0.0f;
                e.attackCooldown = baseCd + (float)GetRandomValue(0, 15) / 10.0f;
            }
        }

        // Dodge decision
        if (player.isAttacking && distToPlayer < 9.0f && e.stamina >= 32.0f &&
            !e.isDodging && !e.isAttacking && !e.isBlocking &&
            GetRandomValue(0, 100) < (int)(e.dodgeChance * 100.0f)) {
            e.isDodging = true;
            e.dodgeTimer = ROLL_DURATION;
            Vector3 dodgeDir = Vector3Normalize(Vector3Subtract(e.position, player.position));
            if (e.type == AGILE && GetRandomValue(0, 100) < 60) {
                Vector3 side = {dodgeDir.z, 0.0f, -dodgeDir.x};
                side = Vector3Scale(side, GetRandomValue(0, 1) ? 1.0f : -1.0f);
                dodgeDir = Vector3Normalize(Vector3Add(dodgeDir, side));
            }
            e.dodgeDirection = dodgeDir;
            e.stamina -= 32.0f;
            e.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
        }

        // Tank block
        if (e.type == TANK && !e.isBlocking && !e.isAttacking && !e.isDodging &&
            player.isAttacking && distToPlayer < ATTACK_RANGE + 3.0f && e.stamina >= 22.0f &&
            GetRandomValue(0, 100) < 75) {
            e.isBlocking = true;
            e.blockTimer = 0.7f;
            e.stamina -= 22.0f;
            e.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
        }
        if (e.isBlocking) {
            e.blockTimer -= dt;
            if (e.blockTimer <= 0.0f) e.isBlocking = false;
        }

        // Unified velocity-based movement with strict collision
        float dodgeSpeed = 12.5f / ROLL_DURATION;
        Vector3 targetVelocity{0,0,0};

        if (e.isDodging) {
            targetVelocity = Vector3Scale(e.dodgeDirection, dodgeSpeed);
            e.dodgeTimer -= dt;
            if (e.dodgeTimer <= 0.0f) e.isDodging = false;
        } else if (e.isAttacking) {
            targetVelocity = {0,0,0};
        } else {
            targetVelocity = Vector3Scale(moveDir, moveSpeed);
        }

        if (e.isDodging) {
            e.velocity = targetVelocity;
        } else {
            e.velocity = Vector3Lerp(e.velocity, targetVelocity, 12.0f * dt);
        }

        Vector3 displacement = Vector3Scale(e.velocity, dt);
        Vector3 candidatePos = {e.position.x + displacement.x, e.position.y, e.position.z + displacement.z};

        float entityRadius = COLLISION_RADIUS_BASE * e.scale;

        bool blocked = false;
        for (const auto& obs : obstacles) {
            if (Vector3Distance({candidatePos.x, 0.0f, candidatePos.z}, obs) < entityRadius) {
                blocked = true;
                break;
            }
        }

        if (!blocked) {
            e.position.x = candidatePos.x;
            e.position.z = candidatePos.z;
        } else {
            e.velocity = Vector3Scale(e.velocity, 0.05f);
        }

        // Attack execution
        if (e.isAttacking) {
            float dur = (e.type == BOSS) ? ((e.comboStep == 3 || e.comboStep == 5) ? 0.85f : 0.55f)
                                        : e.attackDur * (e.isHeavyAttack ? 1.75f : 1.0f);
            float progress = 1.0f - (e.attackTimer / dur);

            // Boss combo animations
            if (e.type == BOSS) {
                switch(e.comboStep) {
                    case 1: e.swingYaw = Lerp(80.0f, -80.0f, progress);
                            e.swingPitch = Lerp(90.0f, -70.0f, progress); break;
                    case 2: e.swingYaw = Lerp(-120.0f, 120.0f, progress);
                            e.swingPitch = Lerp(40.0f, -40.0f, progress); break;
                    case 3: e.swingYaw = Lerp(-180.0f, 180.0f, progress);
                            e.swingPitch = Lerp(0.0f, 0.0f, progress); break;
                    case 4: e.swingYaw = Lerp(60.0f, -60.0f, progress);
                            e.swingPitch = Lerp(-100.0f, 100.0f, progress); break;
                    case 5: {
                        float pp = progress * 3.0f;
                        if (pp < 1.0f) {
                            e.swingYaw = Lerp(100.0f, -100.0f, pp);
                            e.swingPitch = Lerp(160.0f, -110.0f, pp);
                        } else if (pp < 2.0f) {
                            e.swingYaw = Lerp(-100.0f, 200.0f, pp - 1.0f);
                            e.swingPitch = -110.0f;
                        } else {
                            e.swingYaw = Lerp(200.0f, 0.0f, pp - 2.0f);
                            e.swingPitch = Lerp(-110.0f, 140.0f, pp - 2.0f);
                        }
                    } break;
                }
            } else {
                if (e.currentAttack == LIGHT_1) {
                    e.swingPitch = Lerp(110.0f, -95.0f, progress);
                    e.swingYaw = Lerp(80.0f, -80.0f, progress);
                } else if (e.currentAttack == LIGHT_2) {
                    e.swingPitch = Lerp(30.0f, -30.0f, progress);
                    e.swingYaw = Lerp(-170.0f, 170.0f, progress);
                } else {
                    e.swingPitch = Lerp(-90.0f, 125.0f, progress);
                    e.swingYaw = Lerp(-70.0f, 90.0f, progress);
                }
            }

            // Hit window
            float hitStart = (e.type == BOSS && (e.comboStep == 3 || e.comboStep == 5)) ? 0.25f : 0.20f;
            float hitEnd = (e.type == BOSS && e.comboStep == 3) ? 0.85f : 0.80f;
            if (progress > hitStart && progress < hitEnd) {
                if (IsEnemyAttackSwingHittingPlayer(e)) {
                    if (player.isParrying && player.parryTimer > 0.12f) {
                        player.riposteTimer = 1.8f;
                        e.stunTimer = 2.8f;
                        Vector3 knockDir = Vector3Normalize(Vector3Subtract(e.position, player.position));
                        e.velocity = Vector3Add(e.velocity, Vector3Scale(knockDir, 28.0f));
                        SpawnHitSparks(e.position, 24);
                        hitStopTimer = std::max(hitStopTimer, 0.06f);
                        player.shakeTimer = std::max(player.shakeTimer, 0.32f);
                    } else if (!player.isRolling && player.hitInvuln <= 0.0f) {
                        ApplyEnemyHitToPlayer(e);
                    }
                }
            }

            e.attackTimer -= dt;
            if (e.attackTimer <= 0.0f) {
                e.isAttacking = false;
                e.isHeavyAttack = false;
            }
        } else if (!e.isBlocking && e.stunTimer <= 0.0f) {
            e.swingPitch = Lerp(e.swingPitch, -30.0f, 14.0f * dt);
            e.swingYaw = Lerp(e.swingYaw, 30.0f, 14.0f * dt);
        }

        // Blade position
        float bladeLen = (e.type == BOSS) ? 9.5f : 5.8f;
        float er = e.rotation * DEG2RAD;
        Vector3 epivot = Vector3Add(e.position, Vector3RotateByAxisAngle({0.65f,1.65f,0.4f}, {0,1,0}, er));
        Vector3 ebaseLocal = {0,-0.7f,0.6f};
        Vector3 etipLocal = {0,-0.7f, bladeLen};
        Vector3 ebase = Vector3RotateByAxisAngle(ebaseLocal, {1,0,0}, e.swingPitch*DEG2RAD);
        ebase = Vector3RotateByAxisAngle(ebase, {0,1,0}, e.swingYaw*DEG2RAD);
        Vector3 etip = Vector3RotateByAxisAngle(etipLocal, {1,0,0}, e.swingPitch*DEG2RAD);
        etip = Vector3RotateByAxisAngle(etip, {0,1,0}, e.swingYaw*DEG2RAD);
        e.bladeStart = Vector3Add(epivot, ebase);
        e.bladeEnd = Vector3Add(epivot, etip);
    }
}

void DrawEnemy(const Enemy& e, int index) {
    rlPushMatrix();
    rlTranslatef(e.position.x, e.position.y, e.position.z);
    rlRotatef(e.rotation, 0,1,0);
    if (!e.alive) {
        rlRotatef(90, 1, 0, 0); // Banished
    }
    rlScalef(e.scale, e.scale, e.scale);

    Color infernalRed = {90, 30, 120, 255}; // Obsidian Purple (More realistic than pure red)
    Color infernalAsh = {25, 25, 30, 255};  // Shadow Gray
    Color moltenEmber = {255, 80, 30, 255}; // Complementary Ember
    Color body = e.bodyColor;
    
    if (!e.alive) body = {180, 200, 220, 255}; // Purified Silver-Blue
    else if (e.stunTimer > 0 || e.flinchTimer > 0) body = {255, 255, 255, 255};
    else if (e.isBlocking) body = {60, 70, 90, 255}; // Iron Guard
    else if (e.isDodging) body = moltenEmber;

    if (e.type == BOSS) {
        // High Demon / Arch-Fiend
        DrawCube({0, 1.2f, 0}, 2.4f, 3.8f, 1.8f, infernalAsh);
        DrawSphere({0, 3.8f, 0}, 0.9f, infernalRed);
        // Horns
        DrawCylinderEx({-0.5f, 4.2f, 0}, {-1.8f, 6.2f, 0.5f}, 0.3f, 0.05f, 8, {15, 15, 20, 255});
        DrawCylinderEx({ 0.5f, 4.2f, 0}, { 1.8f, 6.2f, 0.5f}, 0.3f, 0.05f, 8, {15, 15, 20, 255});
        // Spikes on back
        DrawCylinderEx({0, 1.8f, -0.8f}, {0, 4.5f, -1.5f}, 0.4f, 0.0f, 6, infernalAsh);
    } else {
        DrawCylinderEx({-0.4f, -0.9f, 0}, {-0.4f, 1.0f, 0}, 0.5f, 0.4f, 12, infernalAsh);
        DrawCylinderEx({ 0.4f, -0.9f, 0}, { 0.4f, 1.0f, 0}, 0.5f, 0.4f, 12, infernalAsh);
        DrawCube({0, 0.9f, 0}, 1.7f, 2.9f, 1.3f, body);
        DrawSphere({0, 2.4f, 0}, 0.62f, Fade(body, 0.8f));

        // Small Horns for Grunts/Agile
        if (e.alive) {
            DrawCylinderEx({-0.3f, 2.8f, 0}, {-0.6f, 3.6f, 0}, 0.15f, 0.0f, 8, {15, 15, 20, 255});
            DrawCylinderEx({ 0.3f, 2.8f, 0}, { 0.6f, 3.6f, 0}, 0.15f, 0.0f, 8, {15, 15, 20, 255});
        }

        if (e.type == TANK) {
            DrawCube({0, 2.7f, 0}, 1.5f, 1.8f, 1.5f, {50, 55, 70, 255}); // Slate armor
        }
    }

    // Weapon
    if (e.alive) {
        rlPushMatrix();
        rlTranslatef(0.65f, 1.65f, 0.4f);
        rlRotatef(e.swingYaw, 0, 1, 0);
        rlRotatef(e.swingPitch, 1, 0, 0);
        float bladeLen = (e.type == BOSS) ? 9.5f : 5.8f;
        DrawCylinderEx({0, -0.3f, 0}, {0, -1.0f, 0}, 0.18f, 0.18f, 12, {40, 35, 30, 255});
        // Fiery blade (Realistic gradient approximation)
        DrawCube({0, 0.0f, 2.9f}, 0.14f, 0.7f, bladeLen, {180, 60, 20, 255}); // Dark Ember
        DrawCube({0, 0.0f, 2.9f}, 0.08f, 0.4f, bladeLen + 0.2f, moltenEmber); // Bright core
        rlPopMatrix();
    }

    // Tank shield
    if (e.type == TANK && e.alive) {
        rlPushMatrix();
        rlTranslatef(-0.9f, 1.6f, 0.4f);
        rlRotatef(90, 0, 1, 0);
        float blockAngle = e.isBlocking ? 30.0f : -30.0f;
        rlRotatef(blockAngle, 1, 0, 0);
        float height = 3.8f;
        float width = 2.0f;
        float thick = 0.4f;
        DrawCube({0, 0, 0}, width, height, thick, Fade(body, 0.8f));
        DrawCube({0, 0, thick/2 + 0.08f}, width + 0.3f, height + 0.3f, 0.15f, DARKGRAY);
        DrawCylinder({0, 0, thick/2 + 0.1f}, 0.55f, 0.25f, 2, 20, GRAY);
        DrawCube({0, 0.9f, thick/2 + 0.15f}, 0.25f, 1.8f, 0.1f, GOLD);
        DrawCube({0, 0, thick/2 + 0.15f}, 1.4f, 0.25f, 0.1f, GOLD);
        rlPopMatrix();
    }

    // Lock-on indicator
    if (index == player.lockedTarget) {
        float pulse = 0.6f + 0.4f * sinf(GetTime() * 10.0f);
        Color lockCol = Fade(GOLD, pulse);
        DrawCircle3D({0, 1.5f, 0}, 3.5f, {1,0,0}, 90, lockCol);
        DrawCircle3D({0, 4.0f, 0}, 2.5f, {1,0,0}, 90, lockCol);
    }

    rlPopMatrix();
}