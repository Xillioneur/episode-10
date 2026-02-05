#include "game.h"

void Draw3DScene() {
    DrawPlane({0,-1.0f,0}, {600,600}, {20, 25, 35, 255}); // Midnight Steel

    for (const auto& obs : obstacles) {
        DrawCube(obs, 8.0f, 16.0f, 8.0f, {45, 50, 65, 255}); // Slate Blue Stone
        DrawCube(Vector3Add(obs, {0,9.0f,0}), 6.0f, 2.0f, 6.0f, {60, 65, 85, 255});
    }

    // Exit portal only in level 1
    if (currentLevel == 1) {
        Color exitCol = exitActive ? GOLD : DARKGRAY;
        DrawCube(Vector3Add(exitPosition, {0,6.0f,0}), 10.0f, 12.0f, 4.0f, Fade(exitCol, 0.6f));
        DrawSphere(Vector3Add(exitPosition, {0,10.0f,0}), 4.0f, exitCol);
    }

    DrawPlayer();
    for (size_t i = 0; i < enemies.size(); i++) {
        DrawEnemy(enemies[i], (int)i);
    }

    for (const auto& p : particles) {
        DrawSphere(p.position, p.size, p.color);
    }

    // Weapon trail
    for (size_t i = 1; i < weaponTrail.size(); i++) {
        float alpha = 1.0f - (weaponTrail[i].time / 0.5f);
        if (alpha <= 0) continue;
        Color c = player.powerReady ? Fade(ORANGE, alpha) : Fade(player.weapon.bladeColor, alpha*0.8f);
        DrawLine3D(weaponTrail[i-1].pos, weaponTrail[i].pos, c);
        DrawLine3D(Vector3Add(weaponTrail[i-1].pos, {0,0.2f,0}), Vector3Add(weaponTrail[i].pos, {0,0.2f,0}), c);
    }
}

void DrawHUD() {
    // Health (Spiritual Purity)
    float hpRatio = (float)player.health / player.maxHealth;
    Color hpColor = (hpRatio > 0.5f) ? SKYBLUE : (hpRatio > 0.25f) ? YELLOW : RED;
    DrawRectangle(40, 40, 480, 44, Fade(BLACK, 0.7f));
    DrawRectangle(44, 44, 472 * hpRatio, 36, hpColor);
    DrawText("SPIRITUAL PURITY", 50, 48, 24, WHITE);

    // Stamina (Divine Grace)
    float stamRatio = player.stamina / MAX_STAMINA;
    DrawRectangle(40, 94, 480, 44, Fade(BLACK, 0.7f));
    DrawRectangle(44, 98, 472 * stamRatio, 36, GOLD);
    DrawText("DIVINE GRACE", 50, 102, 24, WHITE);

    // Poise (Faith Buffer)
    float poiseRatio = player.poise / player.maxPoise;
    DrawRectangle(40, 148, 480, 28, Fade(BLACK, 0.7f));
    DrawRectangle(44, 152, 472 * poiseRatio, 20, WHITE);
    DrawText("FAITH STRENGTH", 50, 152, 18, BLACK);

    // Holy Essence
    DrawText(TextFormat("Holy Essence: %d", player.flasks), 40, 190, 30, SKYBLUE);

    // Lock indicator
    if (player.lockedTarget != -1) {
        DrawText("JUDGMENT CAST", SCREEN_WIDTH - 320, 30, 36, GOLD);
    }

    // Heavy charge
    if (player.isCharging || player.powerReady) {
        float charge = player.chargeTimer / POWER_ATTACK_CHARGE;
        DrawRectangle(40, SCREEN_HEIGHT - 120, 480, 40, Fade(BLACK, 0.7f));
        DrawRectangle(44, SCREEN_HEIGHT - 116, 472 * charge, 32, player.powerReady ? WHITE : GOLD);
        DrawText("DIVINE WRATH READY", 540, SCREEN_HEIGHT - 110, 36, player.powerReady ? WHITE : GOLD);
    }

    // Feedback text
    if (player.riposteTimer > 0.0f) DrawText("HOLY SMITE!", SCREEN_WIDTH/2 - 220, SCREEN_HEIGHT/2 - 120, 64, GOLD);
    if (player.perfectRollTimer > 0.0f) DrawText("CELESTIAL STEP!", SCREEN_WIDTH/2 - 240, SCREEN_HEIGHT/2 - 80, 64, SKYBLUE);

    // Boss health bar
    if (player.lockedTarget != -1 && enemies[player.lockedTarget].type == BOSS && enemies[player.lockedTarget].alive) {
        Enemy& boss = enemies[player.lockedTarget];
        float bossRatio = (float)boss.health / boss.maxHealth;
        DrawRectangle(SCREEN_WIDTH/2 - 310, 50, 620, 40, Fade(BLACK, 0.8f));
        DrawRectangle(SCREEN_WIDTH/2 - 300, 60, 600 * bossRatio, 20, MAROON);
        DrawText("THE ARCH-FIEND", SCREEN_WIDTH/2 - MeasureText("THE ARCH-FIEND", 50)/2, 20, 50, RED);
    }
}

void DrawTitleScreen() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, Fade(BLACK, 0.8f));

    DrawText("DIVINE SENTINEL", SCREEN_WIDTH/2 - MeasureText("DIVINE SENTINEL", 100)/2,
             SCREEN_HEIGHT/2 - 180, 100, WHITE);
    DrawText("The Celestial Nexus", SCREEN_WIDTH/2 - MeasureText("The Celestial Nexus", 50)/2,
             SCREEN_HEIGHT/2 - 80, 50, GOLD);

    DrawText("A Heroic Defense of the Eternal Light", 
             SCREEN_WIDTH/2 - MeasureText("A Heroic Defense of the Eternal Light", 40)/2,
             SCREEN_HEIGHT/2 + 20, 40, LIGHTGRAY);

    DrawText("Press ENTER to Transcend", SCREEN_WIDTH/2 - MeasureText("Press ENTER to Transcend", 50)/2,
             SCREEN_HEIGHT - 140, 50, WHITE);
}
// render.cpp (replace the entire DrawInstructionsScreen function with this updated version)
void DrawInstructionsScreen() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, Fade(BLACK, 0.85f));

    DrawText("DIVINE AWAKENING", SCREEN_WIDTH/2 - MeasureText("DIVINE AWAKENING", 80)/2,
             60, 80, GOLD);

    int y = 160;
    const int lineHeight = 40;
    const int storyFont = 36;
    const int listFont = 32;
    const Color textCol = LIGHTGRAY;

    DrawText("You are the Divine Sentinel, a celestial hero tasked with", 
             SCREEN_WIDTH/2 - MeasureText("You are the Divine Sentinel, a celestial hero tasked with", storyFont)/2,
             y, storyFont, textCol); y += lineHeight;
    DrawText("purifying the Celestial Nexus from infernal corruption.", 
             SCREEN_WIDTH/2 - MeasureText("purifying the Celestial Nexus from infernal corruption.", storyFont)/2,
             y, storyFont, textCol); y += lineHeight + 10;

    DrawText("Purify all corrupted spirits to open the Golden Gate.", 
             SCREEN_WIDTH/2 - MeasureText("Purify all corrupted spirits to open the Golden Gate.", storyFont)/2,
             y, storyFont, textCol); y += lineHeight;

    DrawText("Transcendence", SCREEN_WIDTH/2 - MeasureText("Transcendence", 50)/2, y, 50, GOLD); y += 60;

    int leftX = 260;
    DrawText("WASD          - Movement", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Mouse         - Divine Sight", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Left Click    - Holy Strike", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Hold LClick   - Divine Wrath", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Shift (tap)   - Celestial Step", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Shift (hold)  - Angelic Flight", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Space         - Ascension", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("E             - Holy Essence", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Left Ctrl     - Sacred Parry", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("F             - Cast Judgment", leftX, y, listFont, textCol); y += lineHeight;
    DrawText("Mouse flick   - Switch Judgment", leftX, y, listFont, textCol); y += lineHeight + 30;

    DrawText("Wisdom", SCREEN_WIDTH/2 - MeasureText("Wisdom", 50)/2, y, 50, SKYBLUE); y += 60;

    DrawText("- Step through darkness at the right moment for Grace", leftX, y, listFont, LIME); y += lineHeight;
    DrawText("- Successful parry allows for a Direct Soul Purification", leftX, y, listFont, LIME); y += lineHeight;
    DrawText("- Manage Divine Grace to prevent spiritual exhaustion", leftX, y, listFont, LIME); y += lineHeight;
    DrawText("- Backstabs & ripostes deal massive holy damage", leftX, y, listFont, LIME); y += lineHeight + 50;

    DrawText("Go in Peace.", SCREEN_WIDTH/2 - MeasureText("Go in Peace.", 60)/2, y, 60, WHITE); y += 100;

    DrawText("Press ENTER to Begin Trial", SCREEN_WIDTH/2 - MeasureText("Press ENTER to Begin Trial", 50)/2,
             SCREEN_HEIGHT - 100, 50, WHITE);
}
void DrawDeathScreen() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, Fade(BLACK, 0.9f));
    DrawText("CONNECTION SEVERED", SCREEN_WIDTH/2 - MeasureText("CONNECTION SEVERED", 140)/2,
             SCREEN_HEIGHT/2 - 140, 140, MAROON);
    DrawText(currentDeathMessage, SCREEN_WIDTH/2 - MeasureText(currentDeathMessage, 60)/2,
             SCREEN_HEIGHT/2 + 20, 60, GOLD);
    DrawText("Press R to Reclaim Your Spirit", SCREEN_WIDTH/2 - MeasureText("Press R to Reclaim Your Spirit", 50)/2,
             SCREEN_HEIGHT/2 + 140, 50, WHITE);
}

void DrawVictoryScreen() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, Fade(BLACK, 0.8f));
    if (currentLevel == 2) {
        DrawText("SPIRITUAL ASCENSION!", SCREEN_WIDTH/2 - MeasureText("SPIRITUAL ASCENSION!", 80)/2,
                 SCREEN_HEIGHT/2 - 140, 80, WHITE);
        DrawText("THE HEAVENS ARE SECURED", SCREEN_WIDTH/2 - MeasureText("THE HEAVENS ARE SECURED", 60)/2,
                 SCREEN_HEIGHT/2 - 20, 60, GOLD);
        DrawText("Darkness Banished – Eternal Light Restored", SCREEN_WIDTH/2 - MeasureText("Darkness Banished – Eternal Light Restored", 50)/2,
                 SCREEN_HEIGHT/2 + 80, 50, WHITE);
    } else {
        DrawText("TRIAL 1 COMPLETE", SCREEN_WIDTH/2 - MeasureText("TRIAL 1 COMPLETE", 80)/2,
                 SCREEN_HEIGHT/2 - 100, 80, SKYBLUE);
        DrawText("Ascending to the Inner Sanctum...", SCREEN_WIDTH/2 - MeasureText("Ascending to the Inner Sanctum...", 50)/2,
                 SCREEN_HEIGHT/2 + 20, 50, GOLD);
    }
    DrawText("ESC to Power Down", SCREEN_WIDTH/2 - MeasureText("ESC to Power Down", 50)/2,
             SCREEN_HEIGHT/2 + 180, 50, WHITE);
}