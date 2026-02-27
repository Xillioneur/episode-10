#include "game.h"

void Draw3DScene() {
    DrawPlane({0,-1.0f,0}, {600,600}, {15, 10, 25, 255}); // Dark Void

    // Celestial Nebula (Deep Purple atmosphere)
    DrawSphere({player.position.x, 0, player.position.z}, 180.0f, {35, 15, 65, 40});

    for (const auto& obs : obstacles) {
        rlPushMatrix();
        rlTranslatef(obs.pos.x, obs.pos.y, obs.pos.z);
        rlRotatef(obs.rotation, 0, 1, 0);

        switch (obs.type) {
            case OBS_SHARD: {
                // Jagged Monolith
                float h = obs.height;
                DrawCylinderEx({0, 0, 0}, {0, h, 0}, obs.radius, obs.radius * 0.6f, 6, {45, 50, 65, 255});
                DrawCylinderEx({0, h, 0}, {0, h + 2.0f, 0}, obs.radius * 0.6f, 0.0f, 6, {60, 65, 85, 255});
            } break;

            case OBS_ARCH: {
                // Sacred Archway
                float h = obs.height;
                float w = obs.radius * 2.0f;
                // Pillars
                DrawCube({-w/2, h/2, 0}, 2.0f, h, 2.0f, {240, 240, 245, 255});
                DrawCube({ w/2, h/2, 0}, 2.0f, h, 2.0f, {240, 240, 245, 255});
                // Arch Top
                DrawCube({0, h, 0}, w + 4.0f, 2.5f, 3.0f, {245, 245, 250, 255});
                DrawSphere({0, h + 2.5f, 0}, 2.0f, GOLD);
            } break;

            case OBS_TREE: {
                // Tree of Light
                float h = obs.height;
                DrawCylinderEx({0, 0, 0}, {0, h, 0}, obs.radius, obs.radius * 0.4f, 8, {255, 255, 255, 255});
                // Canopy (Particles simulated with spheres)
                float pulse = 0.8f + 0.2f * sinf(GetTime() * 2.0f);
                DrawSphere({0, h, 0}, obs.radius * 4.0f * pulse, Fade(GOLD, 0.15f));
                DrawSphere({0, h + 2.0f, 0}, obs.radius * 2.5f, Fade(WHITE, 0.1f));
            } break;

            case OBS_STATUE: {
                float h = obs.height;
                DrawCylinderEx({0, 0, 0}, {0, h, 0}, obs.radius, obs.radius * 0.8f, 12, {240, 240, 245, 255});
                DrawSphere({0, h + 1.5f, 0}, obs.radius * 1.2f, GOLD);
                DrawCircle3D({0, h + 4.5f, 0}, obs.radius * 1.5f, {1,0,0}, 90, Fade(GOLD, 0.4f));
            } break;

            case OBS_ALTAR: {
                DrawCube({0, 3.0f, 0}, 8.0f, 6.0f, 5.0f, {240, 240, 245, 255});
                DrawCube({0, 6.2f, 0}, 9.0f, 0.5f, 6.0f, {255, 255, 255, 255});
            } break;

            case OBS_DEBRIS: {
                float pulse = sinf(GetTime() * 0.5f + obs.pos.x) * 2.0f;
                rlTranslatef(0, pulse, 0);
                DrawCube({0,0,0}, 4.0f, 4.0f, 4.0f, {65, 70, 90, 255});
                DrawCube({0,0,0}, 4.2f, 4.2f, 4.2f, Fade(GOLD, 0.1f));
            } break;
        }
        rlPopMatrix();
    }

    // Exit portal for Levels 1 and 2
    if (currentLevel == 1 || currentLevel == 2) {
        Color exitCol = exitActive ? GOLD : DARKGRAY;
        DrawCube(Vector3Add(exitPosition, {0,6.0f,0}), 10.0f, 12.0f, 4.0f, Fade(exitCol, 0.6f));
        DrawSphere(Vector3Add(exitPosition, {0,10.0f,0}), 4.0f, exitCol);
    }

    DrawPlayer();
    for (size_t i = 0; i < enemies.size(); i++) {
        DrawEnemy(enemies[i], (int)i);
    }

    // Relic Orbs
    for (const auto& orb : relicOrbs) {
        if (!orb.active) continue;
        float pulse = 0.6f + 0.4f * sinf(GetTime() * 6.0f);
        Color c = (orb.type == RELIC_MERCY) ? GOLD : (orb.type == RELIC_DISCIPLINE ? SKYBLUE : WHITE);
        DrawSphere(orb.pos, 0.8f * pulse, Fade(c, 0.8f));
        DrawSphere(orb.pos, 1.1f, Fade(c, 0.2f));
    }

    for (const auto& p : particles) {
        DrawSphere(p.position, p.size, p.color);
    }

    // Judgment Notifications
    for (const auto& n : notifications) {
        float alpha = n.timer / 2.5f;
        Vector3 labelPos = Vector3Add(n.pos, {0, (1.0f - alpha) * 4.0f, 0});
        // DrawBillboard(camera, {0}, labelPos, 2.0f, Fade(n.color, alpha)); // Removed buggy line
        
        Vector2 screenPos = GetWorldToScreen(labelPos, camera);
        if (screenPos.x > 0) {
            DrawText(n.text.c_str(), screenPos.x - MeasureText(n.text.c_str(), 24)/2, screenPos.y, 24, Fade(n.color, alpha));
        }
    }

    // Weapon trail (Grace Ribbon)
    for (size_t i = 1; i < weaponTrail.size(); i++) {
        float alpha = 1.0f - (weaponTrail[i].time / 0.5f);
        if (alpha <= 0) continue;
        Color c = player.powerReady ? Fade(WHITE, alpha) : Fade(player.weapon.bladeColor, alpha * 0.6f);
        
        // Draw multiple lines to create a ribbon/sheet effect
        for (float h = -0.4f; h <= 0.4f; h += 0.1f) {
            Vector3 offset = {0, h, 0};
            DrawLine3D(Vector3Add(weaponTrail[i-1].pos, offset), Vector3Add(weaponTrail[i].pos, offset), c);
        }
        // Glow core
        DrawLine3D(weaponTrail[i-1].pos, weaponTrail[i].pos, Fade(WHITE, alpha * 0.4f));
    }
}

void DrawHUD() {
    // Resolve (Spiritual Purity)
    float hpRatio = (float)player.health / player.maxHealth;
    Color hpColor = (hpRatio > 0.5f) ? SKYBLUE : (hpRatio > 0.25f) ? YELLOW : ORANGE;
    DrawRectangle(40, 40, 480, 44, Fade(BLACK, 0.7f));
    DrawRectangle(44, 44, 472 * hpRatio, 36, hpColor);
    DrawText("SPIRITUAL RESOLVE", 50, 48, 24, WHITE);

    // Spirit (Divine Grace)
    float stamRatio = player.stamina / MAX_STAMINA;
    DrawRectangle(40, 94, 480, 44, Fade(BLACK, 0.7f));
    DrawRectangle(44, 98, 472 * stamRatio, 36, GOLD);
    DrawText("DIVINE SPIRIT", 50, 102, 24, WHITE);

    // Clarity (Faith Strength)
    float poiseRatio = player.poise / player.maxPoise;
    DrawRectangle(40, 148, 480, 28, Fade(BLACK, 0.7f));
    DrawRectangle(44, 152, 472 * poiseRatio, 20, WHITE);
    DrawText("INNER CLARITY", 50, 152, 18, BLACK);

    // Holy Essence
    DrawText(TextFormat("Holy Essence: %d", player.flasks), 40, 190, 30, SKYBLUE);

    // Inventory Prompt
    if (gameState == PLAYING) {
        DrawText("TAB - Sacred Inventory", SCREEN_WIDTH - 320, 30, 24, LIGHTGRAY);
        for (const auto& orb : relicOrbs) {
            if (orb.active && Vector3Distance(player.position, orb.pos) < 6.0f) {
                DrawText("Walk over Relic to Collect", SCREEN_WIDTH/2 - 160, SCREEN_HEIGHT/2 + 100, 24, WHITE);
                break;
            }
        }
    }

    // Heavy charge
    if (player.isCharging || player.powerReady) {
        float charge = player.chargeTimer / POWER_ATTACK_CHARGE;
        DrawRectangle(40, SCREEN_HEIGHT - 120, 480, 40, Fade(BLACK, 0.7f));
        DrawRectangle(44, SCREEN_HEIGHT - 116, 472 * std::min(charge, 1.0f), 32, player.powerReady ? WHITE : GOLD);
        DrawText(player.powerReady ? "SERMON OF LIGHT READY" : "GATHERING LIGHT", 50, SCREEN_HEIGHT - 110, 24, player.powerReady ? WHITE : BLACK);
    }

    // Feedback text
    if (player.riposteTimer > 0.0f) DrawText("DIVINE EMBRACE!", SCREEN_WIDTH/2 - 220, SCREEN_HEIGHT/2 - 120, 64, GOLD);
    if (player.perfectRollTimer > 0.0f) DrawText("MERCIFUL STEP!", SCREEN_WIDTH/2 - 240, SCREEN_HEIGHT/2 - 80, 64, SKYBLUE);

    // Boss health bar
    if (enemies.size() > 0 && currentLevel == 2) {
        Enemy& boss = enemies[0];
        if (boss.type == BOSS && boss.alive) {
            float bossRatio = (float)boss.health / boss.maxHealth;
            int barWidth = 1000;
            int barHeight = 12;
            int barX = SCREEN_WIDTH/2 - barWidth/2;
            int barY = SCREEN_HEIGHT - 100;
            
            DrawRectangle(barX - 4, barY - 4, barWidth + 8, barHeight + 8, Fade(BLACK, 0.8f));
            DrawRectangle(barX, barY, barWidth * bossRatio, barHeight, boss.isPhase2 ? ORANGE : GOLD);
            
            DrawText("THE CORRUPTED ARBITER", SCREEN_WIDTH/2 - MeasureText("THE CORRUPTED ARBITER", 30)/2, barY - 40, 30, GOLD);
            if (boss.isPhase2) {
                DrawText("PHASE II: BLAZING RESTORATION", SCREEN_WIDTH/2 - MeasureText("PHASE II: BLAZING RESTORATION", 20)/2, barY + 20, 20, WHITE);
            }
        }
    }
}

void DrawInventory() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
    
    int uiX = SCREEN_WIDTH/2 - 400;
    int uiY = 120;
    
    // Parchment Background
    DrawRectangle(uiX - 20, uiY - 20, 840, 500, {245, 235, 215, 255}); // Beige Parchment
    DrawRectangleLinesEx({(float)uiX - 20, (float)uiY - 20, 840, 500}, 6, GOLD); // Gold Border
    DrawRectangleLinesEx({(float)uiX - 14, (float)uiY - 14, 828, 488}, 2, {160, 140, 100, 255}); // Inner detailed border
    
    Color inkCol = {60, 50, 40, 255}; // Dark Brown Ink
    
    DrawText("THE BOOK OF LIFE", SCREEN_WIDTH/2 - MeasureText("THE BOOK OF LIFE", 50)/2, uiY + 10, 50, inkCol);
    DrawText("Virtues Restored to the Celestial Nexus", SCREEN_WIDTH/2 - MeasureText("Virtues Restored to the Celestial Nexus", 24)/2, uiY + 70, 24, {100, 90, 80, 255});
    
    int itemY = uiY + 150;
    // Mercy
    DrawCircle(uiX + 60, itemY + 25, 28, GOLD);
    DrawCircleLines(uiX + 60, itemY + 25, 28, inkCol);
    DrawText("Relics of Mercy", uiX + 110, itemY, 32, inkCol);
    DrawText(TextFormat("Count: %d", player.mercyRelics), uiX + 110, itemY + 35, 20, DARKGRAY);
    DrawText("Effect: Increases the potency of Holy Essence.", uiX + 380, itemY + 15, 20, {80, 70, 60, 255});
    itemY += 100;

    // Discipline
    DrawCircle(uiX + 60, itemY + 25, 28, SKYBLUE);
    DrawCircleLines(uiX + 60, itemY + 25, 28, inkCol);
    DrawText("Relics of Discipline", uiX + 110, itemY, 32, inkCol);
    DrawText(TextFormat("Count: %d", player.disciplineRelics), uiX + 110, itemY + 35, 20, DARKGRAY);
    DrawText("Effect: Increases the Clarity impact of Blessings.", uiX + 380, itemY + 15, 20, {80, 70, 60, 255});
    itemY += 100;

    // Fortitude
    DrawCircle(uiX + 60, itemY + 25, 28, WHITE);
    DrawCircleLines(uiX + 60, itemY + 25, 28, inkCol);
    DrawText("Relics of Fortitude", uiX + 110, itemY, 32, inkCol);
    DrawText(TextFormat("Count: %d", player.fortitudeRelics), uiX + 110, itemY + 35, 20, DARKGRAY);
    DrawText("Effect: Strengthens your Spiritual Resolve.", uiX + 380, itemY + 15, 20, {80, 70, 60, 255});
    
    DrawText("Press TAB to Close the Book", SCREEN_WIDTH/2 - MeasureText("Press TAB to Close the Book", 24)/2, uiY + 450, 24, inkCol);
}

void DrawTitleScreen() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, Fade(BLACK, 0.8f));

    DrawText("DIVINE SENTINEL", SCREEN_WIDTH/2 - MeasureText("DIVINE SENTINEL", 100)/2,
             SCREEN_HEIGHT/2 - 180, 100, WHITE);
    DrawText("The Celestial Nexus", SCREEN_WIDTH/2 - MeasureText("The Celestial Nexus", 50)/2,
             SCREEN_HEIGHT/2 - 80, 50, GOLD);

    DrawText("A Journey of Restoration and Grace", 
             SCREEN_WIDTH/2 - MeasureText("A Journey of Restoration and Grace", 40)/2,
             SCREEN_HEIGHT/2 + 20, 40, SKYBLUE);

    DrawText("Press ENTER to Begin the Rite", SCREEN_WIDTH/2 - MeasureText("Press ENTER to Begin the Rite", 50)/2,
             SCREEN_HEIGHT - 140, 50, WHITE);
}

void DrawInstructionsScreen() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, Fade(BLACK, 0.85f));

    DrawText("PATH OF GRACE", SCREEN_WIDTH/2 - MeasureText("PATH OF GRACE", 80)/2,
             60, 80, GOLD);

    int y = 160;
    const int lineHeight = 40;
    const int storyFont = 36;
    const int listFont = 32;
    const Color textCol = LIGHTGRAY;

    DrawText("You are the Emissary of Grace, sent to bring peace", 
             SCREEN_WIDTH/2 - MeasureText("You are the Emissary of Grace, sent to bring peace", storyFont)/2,
             y, storyFont, textCol); y += lineHeight;
    DrawText("to the lost spirits of the Celestial Nexus.", 
             SCREEN_WIDTH/2 - MeasureText("to the lost spirits of the Celestial Nexus.", storyFont)/2,
             y, storyFont, textCol); y += lineHeight + 10;

    DrawText("Soothe all agitated spirits to open the Golden Gate.", 
             SCREEN_WIDTH/2 - MeasureText("Soothe all agitated spirits to open the Golden Gate.", storyFont)/2,
             y, storyFont, textCol); y += lineHeight;

    DrawText("Benevolence", SCREEN_WIDTH/2 - MeasureText("Benevolence", 50)/2, y, 50, GOLD); y += 60;

    int leftX = 260;
    DrawText("WASD          - Movement", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Mouse         - Divine Sight", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Left Click    - Extend Grace", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Hold LClick   - Sermon of Light", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Shift (tap)   - Merciful Step", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("E             - Holy Essence", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Left Ctrl     - Sacred Redirection", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Middle Click  - Seek Clarity", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Mouse flick   - Switch Focus", leftX, y, listFont, textCol); y += lineHeight + 30;

    DrawText("Divine Wisdom", SCREEN_WIDTH/2 - MeasureText("Divine Wisdom", 50)/2, y, 50, SKYBLUE); y += 60;

    DrawText("- Step through confusion at the right moment for Grace", leftX, y, listFont, LIME); y += lineHeight;
    DrawText("- Sacred Redirection allows for a Divine Embrace", leftX, y, listFont, LIME); y += lineHeight;
    DrawText("- Manage Divine Spirit to maintain your composure", leftX, y, listFont, LIME); y += lineHeight;
    DrawText("- Restoring spirits from behind brings swifter peace", leftX, y, listFont, LIME); y += lineHeight + 50;

    DrawText("Go in Peace.", SCREEN_WIDTH/2 - MeasureText("Go in Peace.", 60)/2, y, 60, WHITE); y += 100;

    DrawText("Press ENTER to Begin the Rite", SCREEN_WIDTH/2 - MeasureText("Press ENTER to Begin the Rite", 50)/2,
             SCREEN_HEIGHT - 100, 50, WHITE);
}

void DrawDeathScreen() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, Fade(BLACK, 0.9f));
    DrawText("RETURNING TO LIGHT", SCREEN_WIDTH/2 - MeasureText("RETURNING TO LIGHT", 120)/2,
             SCREEN_HEIGHT/2 - 140, 120, GOLD);
    DrawText(currentDeathMessage, SCREEN_WIDTH/2 - MeasureText(currentDeathMessage, 60)/2,
             SCREEN_HEIGHT/2 + 20, 60, SKYBLUE);
    DrawText("Press R to Rejuvenate Your Spirit", SCREEN_WIDTH/2 - MeasureText("Press R to Rejuvenate Your Spirit", 50)/2,
             SCREEN_HEIGHT/2 + 140, 50, WHITE);
}

void DrawVictoryScreen() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, Fade(BLACK, 0.8f));
    if (currentLevel == 3) {
        DrawText("ALL SOULS ASCENDED!", SCREEN_WIDTH/2 - MeasureText("ALL SOULS ASCENDED!", 80)/2,
                 SCREEN_HEIGHT/2 - 140, 80, WHITE);
        DrawText("THE NEXUS IS RESTORED", SCREEN_WIDTH/2 - MeasureText("THE NEXUS IS RESTORED", 60)/2,
                 SCREEN_HEIGHT/2 - 20, 60, GOLD);
        DrawText("Shadows Banished – Eternal Grace Abides", SCREEN_WIDTH/2 - MeasureText("Shadows Banished – Eternal Grace Abides", 50)/2,
                 SCREEN_HEIGHT/2 + 80, 50, WHITE);
    } else {
        DrawText("AREA PURIFIED", SCREEN_WIDTH/2 - MeasureText("AREA PURIFIED", 80)/2,
                 SCREEN_HEIGHT/2 - 100, 80, SKYBLUE);
        DrawText("Ascending to the Inner Sanctum...", SCREEN_WIDTH/2 - MeasureText("Ascending to the Inner Sanctum...", 50)/2,
                 SCREEN_HEIGHT/2 + 20, 50, GOLD);
    }
    DrawText("ESC to Power Down", SCREEN_WIDTH/2 - MeasureText("ESC to Power Down", 50)/2,
             SCREEN_HEIGHT/2 + 180, 50, WHITE);
}
