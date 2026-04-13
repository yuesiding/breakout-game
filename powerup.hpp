#ifndef POWERUP_HPP
#define POWERUP_HPP

#include <raylib.h>

// 道具类型枚举
enum class PowerUpType {
    PADDLE_EXTEND,  // 加长挡板
    MULTI_BALL,     // 多球
    SLOW_BALL       // 减速球
};

// 道具类
class PowerUp {
private:
    Vector2 pos;
    Vector2 speed;
    float radius;
    PowerUpType type;
    bool active;

public:
    // 构造函数
    PowerUp(float x, float y, PowerUpType t)
        : pos{x, y}, speed{0, 180}, radius{14}, type{t}, active(true) {}

    // 更新道具
    void Update() {
        if (!active) return;
        pos.y += speed.y * GetFrameTime();
        // 超出屏幕则失效
        if (pos.y > 600) active = false;
    }

    // 绘制道具
    void Draw() const {
        if (!active) return;
        // 不同道具不同颜色
        Color c = WHITE;
        switch (type) {
            case PowerUpType::PADDLE_EXTEND: c = BLUE; break;
            case PowerUpType::MULTI_BALL: c = GREEN; break;
            case PowerUpType::SLOW_BALL: c = YELLOW; break;
        }
        DrawCircleV(pos, radius, c);
        DrawCircleLines(pos.x, pos.y, radius, BLACK);
    }

    // 碰撞检测
    bool CheckCollision(Rectangle rect) const {
        return active && CheckCollisionCircleRec(pos, radius, rect);
    }

    // 激活道具（失效）
    void Activate() { active = false; }

    // 获取道具类型
    PowerUpType GetType() const { return type; }

    // 判断是否激活
    bool IsActive() const { return active; }
};

#endif