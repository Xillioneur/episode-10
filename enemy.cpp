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
        if (obs.type == OBS_DEBRIS) continue;
        
        float h = obs.height;
        float r = obs.radius;
        BoundingBox box = {Vector3Subtract(obs.pos, {r, 0, r}), Vector3Add(obs.pos, {r, h, r})};
        
        RayCollision col = GetRayCollisionBox(ray, box);
        if (col.hit && col.distance < dist - 0.8f) return false;
    }
    return true;
}

bool IsEnemyAttackSwingHittingPlayer(const Enemy& e) {
    Vector3 pCenter = Vector3Add(player.position, {0, 1.5f, 0});
    Vector3 closest = GetClosestPointOnSegment(pCenter, e.bladeStart, e.bladeEnd);
    float dist = Vector3Distance(pCenter, closest);
    return (dist < 2.8f); // Player hit radius - INCREASED
}

void ApplyEnemyHitToPlayer(const Enemy& e) {
    Vector3 toPlayer = Vector3Subtract(player.position, e.position);
    toPlayer.y = 0.0f;
    Vector3 norm = Vector3Normalize(toPlayer);

    float dmgMult = (e.type == BOSS) ? ((e.comboStep == 5) ? 2.1f : ((e.comboStep == 3) ? 1.6f : 1.3f))
                                    : (e.isHeavyAttack ? 1.75f : (1.0f + e.comboStep * 0.15f));
    if (e.type == BOSS && e.isPhase2) dmgMult *= 1.25f;

    float poiseMult = (e.type == BOSS) ? ((e.comboStep == 5) ? 2.2f : ((e.comboStep == 3) ? 1.8f : 1.4f))
                                      : (e.isHeavyAttack ? 1.85f : (1.0f + e.comboStep * 0.2f));
    if (e.type == BOSS && e.isPhase2) poiseMult *= 1.3f;
    float knockMult = (e.type == BOSS) ? ((e.comboStep == 5) ? 1.8f : 1.3f)
                                      : (e.isHeavyAttack ? 1.5f : (1.0f + e.comboStep * 0.1f));

    float damage = e.attackDamage * dmgMult;
    float poiseDmg = e.poiseDamage * poiseMult;

    player.health -= damage;
    TriggerSFX(2); // Player hit sound
    const_cast<Enemy&>(e).karma = std::max(0.0f, e.karma - 15.0f);
    player.hitInvuln = 0.5f;
    player.velocity = Vector3Add(player.velocity, Vector3Scale(norm, 55.0f * knockMult)); // HYPER-IMPACT KNOCKBACK

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

    Vector3 enemyCenter = Vector3Add(e.position, {0, 1.5f * e.scale, 0});
    Vector3 closest = GetClosestPointOnSegment(enemyCenter, player.bladeStart, player.bladeEnd);
    float dist = Vector3Distance(enemyCenter, closest);
    
    // Increased detection radius for larger enemies/bosses
    float hitRadius = (e.type == BOSS) ? 4.5f : 2.2f * e.scale;
    if (dist > hitRadius) return false;

    Vector3 toEnemy = Vector3Subtract(e.position, player.position);
    toEnemy.y = 0;
    Vector3 normToEnemy = Vector3Normalize(toEnemy);
    
    bool isHeavy = (player.currentAttack == HEAVY);
    int baseDamage = isHeavy ? 92 : 62;
    float basePoiseDmg = isHeavy ? 68.0f : 28.0f;

    // Fatigue penalty
    if (player.stamina < 0.0f) {
        baseDamage = (int)(baseDamage * 0.5f);
        basePoiseDmg *= 0.6f;
    }

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
    bool riposte = (e.stunTimer > 0 && dist < 10.0f);

    if (riposte) {
        baseDamage = 1000; 
        basePoiseDmg = 500.0f;
        // Divine Embrace visual burst
        for(int i=0; i<40; i++) {
            Particle p{};
            p.position = Vector3Add(e.position, {0, 1.5f, 0});
            p.velocity = { (float)GetRandomValue(-100,100)/10.0f, (float)GetRandomValue(-100,100)/10.0f, (float)GetRandomValue(-100,100)/10.0f };
            p.lifetime = p.maxLife = 0.8f;
            p.color = GOLD;
            p.size = 0.6f;
            particles.push_back(p);
        }
    }

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
    TriggerSFX(2); // Spirit hit sound
    e.karma = std::min(100.0f, e.karma + 10.0f); // Blessing increases Karma
    e.hitInvuln = 0.4f;
    float finalKnock = riposte ? 85.0f : (65.0f * knockMult);
    e.velocity = Vector3Add(e.velocity, Vector3Scale(normToEnemy, finalKnock)); // HYPER-IMPACT KNOCKBACK

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
        TriggerSFX(3); // Ascension sound
        SpawnAscensionParticles(e.position);
        
        // Drop Relic
        RelicOrb orb{};
        orb.pos = Vector3Add(e.position, {0, 1.5f, 0});
        orb.active = true;
        
        if (e.trait == TRAIT_COMPASSIONATE) orb.type = RELIC_MERCY;
        else if (e.trait == TRAIT_CRUEL || e.trait == TRAIT_WRATHFUL) orb.type = RELIC_DISCIPLINE;
        else orb.type = RELIC_FORTITUDE;
        
        relicOrbs.push_back(orb);

        if (player.lockedTarget != -1 && &enemies[player.lockedTarget] == &e) {
            player.lockedTarget = -1;
        }
        hitStopTimer = 0.15f; // Moment of Peace
    }

    return true;
}

void UpdateEnemies(float dt) {
    // AI Coordination Pass
    int activeAttackers = 0;
    for (const auto& e : enemies) {
        if (e.alive && (e.isAttacking || e.isWindingUp)) activeAttackers++;
    }

    for (size_t i = 0; i < enemies.size(); i++) {
        Enemy& e = enemies[i];
        if (!e.alive) continue;

        e.hitInvuln -= dt;
        e.stunTimer -= dt;
        e.flinchTimer -= dt;
        e.staminaRegenDelay -= dt;

        Vector3 toPlayer = Vector3Subtract(player.position, e.position);
        float distToPlayer = Vector3Length(toPlayer);
        bool seesPlayer = (distToPlayer < 65.0f && CanSeePlayer(e));

        // Tactical distance goals - Lowered for aggression
        float idealDist = (e.type == TANK) ? 6.5f : (e.type == AGILE ? 9.5f : 8.0f);
        if (activeAttackers >= 3 && !e.isAttacking && !e.isWindingUp) {
            idealDist += 6.0f; // Less of a penalty
        }
        // Removed Fear logic

        if (e.isWindingUp) {
            e.windupTimer -= dt;
            e.velocity = Vector3Lerp(e.velocity, {0,0,0}, 12.0f * dt);
            
            // Physical windup pose (Targeted based on attack type) - ABSOLUTE VELOCITY
            float targetPitch = 0.0f;
            float targetYaw = 0.0f;

            if (e.type == BOSS) {
                targetPitch = -110.0f; 
                targetYaw = 120.0f;
            } else {
                if (e.currentAttack == LIGHT_1) { targetPitch = -85.0f; targetYaw = 0.0f; } 
                else if (e.currentAttack == LIGHT_2) { targetPitch = 0.0f; targetYaw = 110.0f; } 
                else { targetPitch = -45.0f; targetYaw = -90.0f; } 
            }

            e.swingPitch = Lerp(e.swingPitch, targetPitch, 45.0f * dt);
            e.swingYaw = Lerp(e.swingYaw, targetYaw, 45.0f * dt);

            // Visual feedback during windup (face player)
            Vector3 toP = Vector3Subtract(player.position, e.position);
            if (Vector3Length(toP) > 0.1f) {
                e.rotation = atan2f(toP.x, toP.z) * RAD2DEG;
            }

            if (e.windupTimer <= 0.0f) {
                e.isWindingUp = false;
                e.isAttacking = true;
                TriggerSFX(5); // Spirit swing sound
                
                float dur = (e.type == BOSS) ? ((e.comboStep == 3 || e.comboStep == 5) ? 0.45f : 0.28f)
                                            : e.attackDur * (e.isHeavyAttack ? 1.3f : 1.0f);
                if (e.type == BOSS && e.isPhase2) dur *= 0.6f;
                e.attackTimer = dur;

                // Forward Lunge Impulse - EXPLOSIVE MOMENTUM
                Vector3 lungeDir = {sinf(e.rotation*DEG2RAD), 0, cosf(e.rotation*DEG2RAD)};
                float lungePower = (e.type == BOSS) ? 62.0f : (e.type == AGILE ? 48.0f : 42.0f);
                if (e.isHeavyAttack) lungePower *= 1.6f;
                e.velocity = Vector3Add(e.velocity, Vector3Scale(lungeDir, lungePower));
            }
            continue; // Skip movement while winding up
        }
        if (e.staminaRegenDelay <= 0) {
            e.stamina = std::min(e.stamina + 32.0f * dt, (float)MAX_STAMINA);
        }

        if (e.stunTimer > 0 || e.flinchTimer > 0) {
            e.velocity = Vector3Lerp(e.velocity, {0,0,0}, 10.0f * dt);
            continue;
        }

        Vector3 moveDir{0,0,0};
        float moveSpeed = e.speed * (e.stamina <= 0.0f ? EXHAUSTED_MULTIPLIER : 1.0f);

        if (e.type == BOSS) {
            e.state = CHASE;
            e.alertTimer = 10.0f;

            // Phase 2 transition logic
            if (!e.hasTriggeredPhase2 && e.health < e.maxHealth / 2) {
                e.hasTriggeredPhase2 = true;
                e.isPhase2 = true;
                e.phaseTransitionTimer = 3.5f;
                e.stunTimer = 3.5f; // Pause boss during roar
                e.isAttacking = false;
                e.bodyColor = WHITE; // Blazing Light
                SpawnAscensionParticles(e.position);
                SpawnAscensionParticles(Vector3Add(e.position, {0, 4.0f, 0}));
                hitStopTimer = 0.5f;
                player.shakeTimer = 0.8f;
            }

            if (e.phaseTransitionTimer > 0) {
                e.phaseTransitionTimer -= dt;
                // Resplendent burst
                if (GetRandomValue(0, 3) == 0) {
                    SpawnAscensionParticles(Vector3Add(e.position, {0, (float)GetRandomValue(0, 8), 0}));
                }
                e.velocity = {0,0,0};
                continue; 
            }

            // Sermon of Light (Projectile release)
            static float sermonTimer = 0.0f;
            if (e.isPhase2) {
                sermonTimer += dt;
                if (sermonTimer > 5.5f) {
                    sermonTimer = 0.0f;
                    // Release 8 spheres of grace
                    for (int i = 0; i < 8; i++) {
                        float angle = (float)i / 8.0f * 2.0f * PI;
                        Particle p{};
                        p.position = Vector3Add(e.position, {0, 3.5f, 0});
                        p.velocity = {cosf(angle) * 12.0f, 0, sinf(angle) * 12.0f};
                        p.lifetime = p.maxLife = 2.5f;
                        p.color = WHITE;
                        p.size = 1.4f;
                        particles.push_back(p);
                    }
                }

                // Divine Radiance Aura (Proximity sap)
                static float radiancePulseTimer = 0.0f;
                radiancePulseTimer += dt;
                if (radiancePulseTimer > 0.25f) {
                    radiancePulseTimer = 0.0f;
                    if (distToPlayer < 18.0f && !player.isDead) {
                        player.health -= 3;
                        // Visual cue for aura
                        for(int i=0; i<3; i++) {
                            Particle p{};
                            p.position = Vector3Add(e.position, {(float)GetRandomValue(-10,10), (float)GetRandomValue(0,8), (float)GetRandomValue(-10,10)});
                            p.velocity = {0, 2.0f, 0};
                            p.lifetime = p.maxLife = 0.8f;
                            p.color = Fade(WHITE, 0.4f);
                            p.size = 0.6f;
                            particles.push_back(p);
                        }
                    }
                }
            }

            if (e.isPhase2) moveSpeed *= 1.35f;

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
            float range = (e.isPhase2) ? ATTACK_RANGE + 7.5f : ATTACK_RANGE + 5.0f;
            
            if (distToPlayer <= range && dot > 0.5f && !e.isAttacking && !e.isWindingUp && e.comboDelayTimer <= 0.0f && e.stamina >= 25.0f) {
                e.comboStep = (e.comboStep % 5) + 1;
                
                // Long recovery only after the full combo
                if (e.comboStep == 5) {
                    float cd = (e.isPhase2) ? 1.2f : 2.8f;
                    e.comboDelayTimer = cd;
                }
                
                e.isWindingUp = true;
                e.windupTimer = (e.isPhase2) ? 0.04f : 0.08f;
                
                e.stamina -= 25.0f;
                e.staminaRegenDelay = 1.0f;
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
                // Hyper-Intelligence: Relentless Predictive Interception
                Vector3 predictedPlayerPos = Vector3Add(player.position, Vector3Scale(player.velocity, 0.42f));
                Vector3 toTarget = Vector3Subtract(predictedPlayerPos, e.position);
                
                bool canAffordAttack = (activeAttackers < 3); // Relentless: 3 spirits attack at once
                float targetDist = canAffordAttack ? 4.0f : idealDist; // Charge in if token available

                if (seesPlayer) {
                    e.rotation = atan2f(toPlayer.x, toPlayer.z) * RAD2DEG;
                }

                if (distToPlayer > targetDist + 1.5f) {
                    moveDir = Vector3Normalize(toTarget);
                } else if (distToPlayer < targetDist - 1.5f) {
                    moveDir = Vector3Scale(Vector3Normalize(toPlayer), -1.0f);
                } else {
                    // Circle and flank aggressively
                    Vector3 forward = Vector3Normalize(toPlayer);
                    Vector3 tangent = {forward.z, 0.0f, -forward.x};
                    float circleDir = (i % 2 == 0) ? 1.0f : -1.0f;
                    moveDir = Vector3Scale(tangent, circleDir);
                    moveSpeed *= 1.45f; // Fast flanking
                }
                moveSpeed *= 1.25f; // High chase aggression
            }

            // Coordinated Attack decision
            Vector3 eFacing = {sinf(e.rotation * DEG2RAD), 0.0f, cosf(e.rotation * DEG2RAD)};
            float dot = Vector3DotProduct(eFacing, Vector3Normalize(toPlayer));
            
            bool canAffordAttack = (activeAttackers < 3); 
            
            if (canAffordAttack && distToPlayer <= ATTACK_RANGE + 4.5f && dot > 0.55f && e.attackCooldown <= 0.0f &&
                e.stamina >= 20.0f && !e.isAttacking && !e.isWindingUp && !e.isDodging && !e.isBlocking && e.stunTimer <= 0.0f) {
                
                // Determine combo length based on type
                int comboTarget = (e.type == AGILE) ? 3 : 2;
                e.comboStep = (e.comboStep % comboTarget) + 1;
                
                bool wantHeavy = (e.type == TANK && GetRandomValue(0, 100) < 45);
                e.isHeavyAttack = wantHeavy && (e.stamina >= 40.0f);
                
                e.isWindingUp = true;
                float baseWindup = (e.type == AGILE) ? 0.03f : 0.06f; // Instant windups
                e.windupTimer = baseWindup * (e.isHeavyAttack ? 1.3f : 1.0f);
                
                e.currentAttack = static_cast<AttackType>(e.comboStep - 1);
                
                float staminaCost = e.isHeavyAttack ? 40.0f : 22.0f;
                e.stamina -= staminaCost;
                e.staminaRegenDelay = e.isHeavyAttack ? 1.2f : 0.6f;
                
                // Attack cooldown is shorter for combos
                float baseCd = (e.comboStep < comboTarget) ? 0.15f : ((e.type == AGILE) ? 0.8f : 1.8f);
                e.attackCooldown = baseCd + (float)GetRandomValue(0, 5) / 10.0f;
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
        } else if (e.isAttacking || e.stunTimer > 0 || e.flinchTimer > 0) {
            // Momentum Preservation: Natural decay for lunge and knockback
            float friction = (e.stunTimer > 0 || e.flinchTimer > 0) ? 3.2f : 1.4f;
            e.velocity = Vector3Scale(e.velocity, 1.0f - friction * dt);
        } else {
            e.velocity = Vector3Lerp(e.velocity, targetVelocity, 12.0f * dt);
        }

        Vector3 displacement = Vector3Scale(e.velocity, dt);
        Vector3 candidatePos = {e.position.x + displacement.x, e.position.y, e.position.z + displacement.z};

        float entityRadius = COLLISION_RADIUS_BASE * e.scale;

        bool blocked = false;
        for (const auto& obs : obstacles) {
            if (obs.type == OBS_DEBRIS) continue;
            float d = Vector2Distance({candidatePos.x, candidatePos.z}, {obs.pos.x, obs.pos.z});
            if (d < (obs.radius + entityRadius * 0.45f)) { // Adjusted for spirit core size
                blocked = true;
                break;
            }
        }

        if (!blocked) {
            e.position.x = candidatePos.x;
            e.position.z = candidatePos.z;

            // Spirit Footstep Trigger
            if (Vector3Length(e.velocity) > 2.0f && !e.isDodging && !e.isAttacking && !e.isWindingUp) {
                e.stepTimer -= dt;
                if (e.stepTimer <= 0.0f) {
                    TriggerSFX(6);
                    // Variety based on enemy type
                    if (e.type == TANK) {
                        synth.frequency = 650.0f; // Heavier
                        synth.amplitude = 0.65f;
                    } else if (e.type == AGILE) {
                        synth.frequency = 1250.0f; // Sharper
                        synth.amplitude = 0.35f;
                    }
                    e.stepTimer = 0.42f / (Vector3Length(e.velocity) / 10.0f);
                    e.stepTimer = Clamp(e.stepTimer, 0.2f, 0.5f);
                }
            } else {
                e.stepTimer = 0.0f;
            }
        } else {
            e.velocity = Vector3Scale(e.velocity, 0.05f);
        }

        // Update Blade BEFORE collision checks - FIXED ROTATION MATH
        float bladeLen = (e.type == BOSS) ? 12.5f : 8.2f;
        float er = e.rotation * DEG2RAD;
        Vector3 epivot = Vector3Add(e.position, Vector3RotateByAxisAngle({0.65f,1.65f,0.4f}, {0,1,0}, er));
        Vector3 ebaseLocal = {0,-0.7f,0.6f};
        Vector3 etipLocal = {0,-0.7f, bladeLen};
        
        float totalYaw = (e.rotation + e.swingYaw) * DEG2RAD;
        float pitchRad = e.swingPitch * DEG2RAD;

        Vector3 ebase = Vector3RotateByAxisAngle(ebaseLocal, {1,0,0}, pitchRad);
        ebase = Vector3RotateByAxisAngle(ebase, {0,1,0}, totalYaw);
        Vector3 etip = Vector3RotateByAxisAngle(etipLocal, {1,0,0}, pitchRad);
        etip = Vector3RotateByAxisAngle(etip, {0,1,0}, totalYaw);
        
        e.bladeStart = Vector3Add(epivot, ebase);
        e.bladeEnd = Vector3Add(epivot, etip);

        // Attack execution
        if (e.isAttacking) {
            float dur = (e.type == BOSS) ? ((e.comboStep == 3 || e.comboStep == 5) ? 0.85f : 0.55f)
                                        : e.attackDur * (e.isHeavyAttack ? 1.75f : 1.0f);
            if (e.type == BOSS && e.isPhase2) dur *= 0.75f;
            
            float progress = 1.0f - (e.attackTimer / dur);

            // Hyperrealistic Strike Acceleration (Ease-In) - ABSOLUTE VELOCITY
            float snapProg = powf(progress, 7.0f); 

            // Boss combo animations - CLEAN ARCS
            if (e.type == BOSS) {
                switch(e.comboStep) {
                    case 1: e.swingYaw = 0.0f; 
                            e.swingPitch = Lerp(-110.0f, 90.0f, snapProg); break; // Overhead
                    case 2: e.swingYaw = Lerp(120.0f, -120.0f, snapProg); 
                            e.swingPitch = 0.0f; break; // Wide Side
                    case 3: e.swingYaw = Lerp(-120.0f, 120.0f, snapProg); 
                            e.swingPitch = 30.0f; break; // Low Side
                    case 4: e.swingYaw = 0.0f; 
                            e.swingPitch = Lerp(-110.0f, 90.0f, snapProg); break; // Overhead
                    case 5: {
                        float pp = progress * 3.0f;
                        if (pp < 1.0f) {
                            float spp = powf(pp, 3.0f);
                            e.swingYaw = Lerp(120.0f, 0.0f, spp);
                            e.swingPitch = Lerp(-110.0f, 140.0f, spp);
                        } else if (pp < 2.0f) {
                            e.swingYaw = 0.0f;
                            e.swingPitch = 140.0f;
                        } else {
                            e.swingYaw = 0.0f;
                            e.swingPitch = Lerp(140.0f, 30.0f, pp - 2.0f);
                        }
                    } break;
                }
            } else {
                if (e.currentAttack == LIGHT_1) {
                    e.swingPitch = Lerp(-85.0f, 90.0f, snapProg);
                    e.swingYaw = 0.0f;
                } else if (e.currentAttack == LIGHT_2) {
                    e.swingPitch = 0.0f;
                    e.swingYaw = Lerp(110.0f, -110.0f, snapProg);
                } else {
                    e.swingPitch = Lerp(-45.0f, 45.0f, snapProg);
                    e.swingYaw = Lerp(-90.0f, 90.0f, snapProg);
                }
            }

            // Hit window (Adjusted for Snap Acceleration)
            float hitStart = (e.type == BOSS && (e.comboStep == 3 || e.comboStep == 5)) ? 0.40f : 0.45f;
            float hitEnd = 0.95f;

            // VOID SLAM Logic (Phase 2, Step 5)
            if (e.type == BOSS && e.isPhase2 && e.comboStep == 5) {
                static bool slamTriggered = false;
                if (progress < 0.1f) slamTriggered = false;
                
                if (progress > 0.75f && !slamTriggered) {
                    slamTriggered = true;
                    SpawnVoidShockwave(e.position);
                    player.shakeTimer = 0.6f;
                    
                    float dist = Vector3Distance(player.position, e.position);
                    if (dist < 18.0f && !player.isRolling && player.hitInvuln <= 0.0f) {
                        ApplyEnemyHitToPlayer(e);
                        player.velocity = Vector3Add(player.velocity, {0, 15.0f, 0}); // Knock up
                    }
                }
            }

            if (progress > hitStart && progress < hitEnd) {
                if (IsEnemyAttackSwingHittingPlayer(e)) {
                    if (player.isParrying && player.parryTimer > 0.12f) {
                        player.riposteTimer = 1.8f;
                        e.karma = std::min(100.0f, e.karma + 20.0f);
                        e.stunTimer = 2.8f;
                        
                        // Burst of Clarity Visual
                        for(int i=0; i<20; i++) {
                            float ang = (float)i/20.0f * 2.0f * PI;
                            Particle p{};
                            p.position = Vector3Add(e.position, {0, 1.5f, 0});
                            p.velocity = { cosf(ang)*15.0f, 0, sinf(ang)*15.0f };
                            p.lifetime = p.maxLife = 0.4f;
                            p.color = WHITE;
                            p.size = 0.8f;
                            particles.push_back(p);
                        }
                        hitStopTimer = 0.12f;
                        player.shakeTimer = 0.25f;
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
    Color divineAsh = {30, 35, 50, 255};    // Deep Void
    Color celestialSky = {100, 180, 240, 255}; // Celestial Blue
    Color body = e.bodyColor;
    
    if (!e.alive) body = {180, 200, 220, 255}; // Purified Silver-Blue
    else if (e.health <= 0) {
        float pulse = 0.6f + 0.4f * sinf(GetTime() * 45.0f); // Intense pulse
        body = ColorAlphaBlend(e.bodyColor, GOLD, Fade(WHITE, pulse));
    }
    else if (e.isWindingUp) {
        float pulse = 0.35f + 0.35f * sinf(GetTime() * 18.0f); // Slower, more subtle pulse
        body = ColorAlphaBlend(e.bodyColor, WHITE, Fade(WHITE, pulse));
    }
    else if (e.stunTimer > 0 || e.flinchTimer > 0) body = {255, 255, 255, 255};
    else if (e.isBlocking) body = {60, 70, 90, 255}; // Iron Guard
    else if (e.isDodging) body = celestialSky;

    if (e.type == BOSS) {
        // Celestial Arbiter Form
        DrawCube({0, 1.2f, 0}, 2.4f, 3.8f, 1.8f, divineAsh);
        DrawSphere({0, 3.8f, 0}, 0.9f, GOLD); // Golden Head

        // Halo Crown
        float haloPulse = 0.6f + 0.4f * sinf(GetTime() * 3.0f);
        DrawCircle3D({0, 4.8f, 0}, 1.2f, {1,0,0}, 90, Fade(GOLD, haloPulse));
        DrawCircle3D({0, 4.8f, 0}, 1.3f, {1,0,0}, 90, Fade(WHITE, haloPulse * 0.5f));

        // Sacred Wings (Phase 2)
        if (e.isPhase2 && e.alive) {
            float wingPulse = 0.7f + 0.3f * sinf(GetTime() * 4.0f);
            Color wingCol = Fade(WHITE, wingPulse * 0.6f);

            // Left Wing
            rlPushMatrix();
            rlRotatef(25 + sinf(GetTime()*2)*10, 0, 0, 1);
            DrawTriangle3D({-0.5f, 2.5f, -0.2f}, {-6.0f, 5.0f, -1.5f}, {-1.5f, 0.5f, -0.5f}, wingCol);
            rlPopMatrix();

            // Right Wing
            rlPushMatrix();
            rlRotatef(-25 - sinf(GetTime()*2)*10, 0, 0, 1);
            DrawTriangle3D({0.5f, 2.5f, -0.2f}, {1.5f, 0.5f, -0.5f}, {6.0f, 5.0f, -1.5f}, wingCol);
            rlPopMatrix();
        }
    } else {
        Color legCol = {30, 35, 50, 255}; // Deep Void
        DrawCylinderEx({-0.4f, -0.9f, 0}, {-0.4f, 1.0f, 0}, 0.5f, 0.4f, 12, legCol);
        DrawCylinderEx({ 0.4f, -0.9f, 0}, { 0.4f, 1.0f, 0}, 0.5f, 0.4f, 12, legCol);

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
        // Radiance Scepter
        Color scepterCol = (e.type == BOSS) ? GOLD : (Color){180, 60, 20, 255};
        Color coreCol = (e.type == BOSS) ? (Color)WHITE : celestialSky;
        if (e.type == BOSS) {
            DrawCube({0, 0.0f, 2.9f}, 0.22f, 0.8f, bladeLen, scepterCol); 
            DrawCube({0, 0.0f, 2.9f}, 0.12f, 0.5f, bladeLen + 0.4f, coreCol); 
        } else {
            DrawCube({0, 0.0f, 2.9f}, 0.14f, 0.7f, bladeLen, scepterCol);
            DrawCube({0, 0.0f, 2.9f}, 0.08f, 0.4f, bladeLen + 0.2f, coreCol);
        }
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