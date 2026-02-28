#include "game.h"
#include "raylib.h"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Divine Sentinel – The Celestial Nexus");

    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    HideCursor();
    DisableCursor();

    InitAudioDevice();
    InitGame();
    ResetLevel();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Cursor management
        if (gameState == PLAYING) {
            if (!IsCursorHidden()) {
                DisableCursor();
                HideCursor();
            }
        } else {
            if (IsCursorHidden()) {
                EnableCursor();
                ShowCursor();
            }
        }

        // 1. INPUT & GLOBAL TOGGLES
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (gameState == PLAYING) gameState = PAUSED;
            else if (gameState == INVENTORY) gameState = PLAYING;
            else if (gameState == SANCTUARY) gameState = PLAYING;
            else if (gameState == PAUSED) gameState = PLAYING;
            else if (gameState == VICTORY) break; // Quit from Victory
        }

        if (IsKeyPressed(KEY_TAB)) {
            if (gameState == PLAYING) gameState = INVENTORY;
            else if (gameState == INVENTORY) gameState = PLAYING;
        }

        // 2. STATE LOGIC
        switch (gameState) {
            case TITLE_SCREEN:
                if (IsKeyPressed(KEY_ENTER)) gameState = INSTRUCTIONS;
                break;
            case INSTRUCTIONS:
                if (IsKeyPressed(KEY_ENTER)) gameState = PLAYING;
                break;
            case PLAYING:
                UpdateGame(dt);
                // Level transition
                if ((currentLevel == 1 || currentLevel == 2) && exitActive && Vector3Distance(player.position, exitPosition) < 9.0f) {
                    currentLevel++;
                    ResetLevel();
                }
                // Death trigger
                if (player.health <= 0 && !player.isDead) {
                    player.isDead = true;
                    player.deathTimer = 2.5f;
                    player.deathFallAngle = 90.0f;
                }
                if (player.isDead && player.deathTimer <= 0) {
                    gameState = DEAD;
                    currentDeathMessage = deathMessages[GetRandomValue(0, (int)deathMessages.size()-1)].c_str();
                }
                break;
            case INVENTORY:
                UpdateGame(dt); // Keeps camera alive
                break;
            case SANCTUARY:
                UpdateGame(dt); // Keeps light shafts moving
                if (IsKeyPressed(KEY_F)) gameState = PLAYING;
                
                // Upgrade Logic
                if (IsKeyPressed(KEY_ONE) && player.mercyRelics >= 3) {
                    player.mercyRelics -= 3;
                    player.mercyLevel++;
                    Notification n{"PATH OF MERCY ATTUNED", player.position, 2.0f, GOLD};
                    notifications.push_back(n);
                }
                if (IsKeyPressed(KEY_TWO) && player.disciplineRelics >= 3) {
                    player.disciplineRelics -= 3;
                    player.disciplineLevel++;
                    Notification n{"PATH OF DISCIPLINE ATTUNED", player.position, 2.0f, SKYBLUE};
                    notifications.push_back(n);
                }
                if (IsKeyPressed(KEY_THREE) && player.fortitudeRelics >= 3) {
                    player.fortitudeRelics -= 3;
                    player.fortitudeLevel++;
                    Notification n{"PATH OF FORTITUDE ATTUNED", player.position, 2.0f, WHITE};
                    notifications.push_back(n);
                }
                break;
            case PAUSED:
                break;
            case DEAD:
                UpdatePlayer(dt);
                if (IsKeyPressed(KEY_R)) ResetLevel();
                break;
            case VICTORY:
                if (IsKeyPressed(KEY_R)) {
                    currentLevel = 1;
                    player.mercyRelics = 0;
                    player.disciplineRelics = 0;
                    player.fortitudeRelics = 0;
                    ResetLevel();
                }
                break;
        }

        // 3. RENDERING
        BeginDrawing();
        ClearBackground(BLACK);

        if (gameState != TITLE_SCREEN && gameState != INSTRUCTIONS) {
            BeginTextureMode(target);
            ClearBackground({12, 12, 22, 255});
            BeginMode3D(camera);
            Draw3DScene();
            EndMode3D();
            EndTextureMode();

            if (bloomShader.id > 0) BeginShaderMode(bloomShader);
            DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, (float)-target.texture.height}, {0, 0}, WHITE);
            if (bloomShader.id > 0) EndShaderMode();

            DrawHUD();
            
            if (gameState == INVENTORY) DrawInventory();
            if (gameState == SANCTUARY) DrawSanctuary();
            if (gameState == PAUSED) {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.65f));
                DrawText("PAUSED", SCREEN_WIDTH/2 - MeasureText("PAUSED", 80)/2, SCREEN_HEIGHT/2 - 60, 80, GOLD);
                DrawText("ESC to Resume", SCREEN_WIDTH/2 - MeasureText("ESC to Resume", 40)/2, SCREEN_HEIGHT/2 + 40, 40, LIGHTGRAY);
                
                Rectangle quitBtn = { SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 120, 300, 60 };
                bool hover = CheckCollisionPointRec(GetMousePosition(), quitBtn);
                DrawRectangleRec(quitBtn, hover ? RED : MAROON);
                DrawRectangleLinesEx(quitBtn, 4, GOLD);
                DrawText("Quit Game", quitBtn.x + (quitBtn.width - MeasureText("Quit Game", 30)) / 2, quitBtn.y + 15, 30, WHITE);
                if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) break;
            }
        } else {
            ClearBackground({12, 12, 22, 255});
        }

        if (gameState == TITLE_SCREEN) DrawTitleScreen();
        else if (gameState == INSTRUCTIONS) DrawInstructionsScreen();
        if (gameState == DEAD) DrawDeathScreen();
        if (gameState == VICTORY) DrawVictoryScreen();

        EndDrawing();
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
