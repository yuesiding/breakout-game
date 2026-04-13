#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <raylib.h>

struct Particle {
    Vector2 pos;
    Vector2 speed;
    Color color;
    float life;

    // 更新粒子
    void Update() {
        pos.x += speed.x * GetFrameTime();
        pos.y += speed.y * GetFrameTime();
        life -= GetFrameTime();
    }

    // 判断是否死亡
    bool IsDead() const { return life <= 0; }

    // 绘制粒子
    void Draw() const {
        DrawCircleV(pos, 3, Fade(color, life / 0.5f));
    }
};

#endif