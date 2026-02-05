#include "game.h"

void SpawnDataParticles(Vector3 pos, int count) {
    Color googleColors[] = {
        {66, 133, 244, 255},  // Blue
        {234, 67, 53, 255},   // Red
        {251, 188, 5, 255},   // Yellow
        {52, 168, 83, 255}    // Green
    };

    for (int i = 0; i < count; i++) {
        Particle p{};
        p.position = pos;
        p.velocity = {GetRandomValue(-100,100)/20.0f,
                      GetRandomValue(40,140)/20.0f,
                      GetRandomValue(-100,100)/20.0f};
        p.lifetime = p.maxLife = GetRandomValue(40,90)/100.0f;
        p.color = Fade(googleColors[GetRandomValue(0, 3)], 0.9f);
        p.size = GetRandomValue(4,12)/10.0f;
        particles.push_back(p);
    }
}

void SpawnHitSparks(Vector3 pos, int count) {
    for (int i = 0; i < count; i++) {
        Particle p{};
        p.position = pos;
        p.velocity = {GetRandomValue(-120,120)/15.0f,
                      GetRandomValue(60,180)/15.0f,
                      GetRandomValue(-120,120)/15.0f};
        p.lifetime = p.maxLife = GetRandomValue(30,70)/100.0f;
        p.color = Fade(YELLOW, 0.95f);
        p.size = GetRandomValue(3,9)/10.0f;
        particles.push_back(p);
    }
}

void UpdateParticles(float dt) {
    for (auto it = particles.begin(); it != particles.end(); ) {
        it->lifetime -= dt;
        if (it->lifetime <= 0) {
            it = particles.erase(it);
            continue;
        }
        it->position = Vector3Add(it->position, Vector3Scale(it->velocity, dt));
        it->velocity.y -= 3.5f * dt;
        ++it;
    }
}