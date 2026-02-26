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

void SpawnVoidShockwave(Vector3 pos) {
    int count = 60;
    for (int i = 0; i < count; i++) {
        float angle = (float)i / count * 2.0f * PI;
        Particle p{};
        p.position = pos;
        float speed = 25.0f;
        p.velocity = { cosf(angle) * speed, 0.5f, sinf(angle) * speed };
        p.lifetime = p.maxLife = 0.65f;
        p.color = { 160, 220, 255, 255 }; // Cloud Blue
        p.size = 1.2f;
        particles.push_back(p);
    }
}

void SpawnAscensionParticles(Vector3 pos) {
    int count = 40;
    for (int i = 0; i < count; i++) {
        Particle p{};
        p.position = pos;
        p.velocity = {GetRandomValue(-40,40)/20.0f,
                      GetRandomValue(60,180)/20.0f,
                      GetRandomValue(-40,40)/20.0f};
        p.lifetime = p.maxLife = GetRandomValue(120,250)/100.0f;
        p.color = Fade(GOLD, 0.8f);
        p.size = GetRandomValue(5,15)/10.0f;
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
        // Ascension particles defy gravity
        if (it->color.r == 255 && it->color.g == 203) { // Close enough to GOLD
             it->velocity.y += 0.8f * dt; 
             it->velocity.x *= 0.98f;
             it->velocity.z *= 0.98f;
        } else if (it->color.r == 40 && it->color.b == 60) {
            // Sinister Descent (Drag down)
            it->velocity.y -= 12.0f * dt;
            it->size *= 0.98f;
        } else {
            it->velocity.y -= 3.5f * dt;
        }
        ++it;
    }
}
