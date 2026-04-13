// #include <cassert>
// #include <raylib.h>

// int main() {
//     // 测试1：球和挡板正常碰撞（修正坐标）
//     {
//         Vector2 ballPos = {400, 542};  // 球心在挡板上方，刚好碰到
//         Rectangle paddle = {350, 550, 100, 15};
//         float radius = 8.0f;
//         bool result = CheckCollisionCircleRec(ballPos, radius, paddle);
//         assert(result == true && "球和挡板应该碰撞");
//     }

//     // 测试2：球和挡板不碰撞（保持不变）
//     {
//         Vector2 ballPos = {100, 100};
//         Rectangle paddle = {350, 550, 100, 15};
//         float radius = 8.0f;
//         bool result = CheckCollisionCircleRec(ballPos, radius, paddle);
//         assert(result == false && "球和挡板不应该碰撞");
//     }

//     // 测试3：球和砖块正常碰撞
//     {
//         Vector2 ballPos = {100, 118};
//         Rectangle brick = {80, 120, 40, 20};
//         float radius = 8.0f;
//         bool result = CheckCollisionCircleRec(ballPos, radius, brick);
//         assert(result == true && "球和砖块应该碰撞");
//     }

//     // 测试4：球和砖块边缘碰撞
//     {
//         Vector2 ballPos = {78, 118};
//         Rectangle brick = {80, 120, 40, 20};
//         float radius = 8.0f;
//         bool result = CheckCollisionCircleRec(ballPos, radius, brick);
//         assert(result == true && "球和砖块边缘应该碰撞");
//     }

//     return 0;
// }

#include <cassert>
#include <raylib.h>
#include <iostream>

int test_paddle_collision() {
    // 正常碰撞
    {
        Vector2 ballPos = {400, 542};
        Rectangle paddle = {350, 550, 100, 15};
        float radius = 8.0f;
        bool result = CheckCollisionCircleRec(ballPos, radius, paddle);
        if (!result) {
            std::cerr << "paddle collision failed\n";
            return 1;
        }
    }
    // 不碰撞
    {
        Vector2 ballPos = {100, 100};
        Rectangle paddle = {350, 550, 100, 15};
        float radius = 8.0f;
        bool result = CheckCollisionCircleRec(ballPos, radius, paddle);
        if (result) {
            std::cerr << "paddle no-hit failed\n";
            return 1;
        }
    }
    return 0;
}

int test_brick_collision() {
    // 正常碰撞
    {
        Vector2 ballPos = {100, 118};
        Rectangle brick = {80, 120, 40, 20};
        float radius = 8.0f;
        bool result = CheckCollisionCircleRec(ballPos, radius, brick);
        if (!result) {
            std::cerr << "brick normal collision failed\n";
            return 1;
        }
    }
    // 边缘碰撞
    {
        Vector2 ballPos = {78, 118};
        Rectangle brick = {80, 120, 40, 20};
        float radius = 8.0f;
        bool result = CheckCollisionCircleRec(ballPos, radius, brick);
        if (!result) {
            std::cerr << "brick edge collision failed\n";
            return 1;
        }
    }
    return 0;
}

int main() {
    int ret = 0;
    ret |= test_paddle_collision();
    ret |= test_brick_collision();
    
    if (ret == 0) {
        std::cout << "collision_test passed: normal/edge/no-hit cases\n";
    } else {
        std::cout << "collision_test failed\n";
    }
    return ret;
}