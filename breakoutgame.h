// #pragma once
// #define SERVER_MODE 1
// #include <raylib.h>
// #include <vector>
// #include <memory>
// #include <enet/enet.h> // 新增ENet头文件
// #include <iostream>
// // 道具类型枚举
// enum class PowerUpType {
//     PADDLE_EXTEND,
//     MULTI_BALL,
//     SLOW_BALL
// };

// // 游戏状态枚举
// enum class GameStateEnum {
//     START,
//     PLAYING,
//     GAME_OVER,
//     WIN
// };

// // 网络同步的游戏状态结构体（核心！）
// struct NetworkGameState {
//     float ballX, ballY;       // 球的位置
//     float ballSpeedX, ballSpeedY; // 球的速度
//     float paddle1X, paddle2X; // 两个挡板的X位置（Y固定）
//     int score;                // 分数
//     int life;                 // 生命值
// };

// // 挡板类（支持两个挡板）
// class Paddle {
// private:
//     Rectangle rect;
//     float originalWidth;
//     float extendTimer;
// public:
//     Paddle(float x, float y, float width, float height) 
//         : rect({x, y, width, height}), originalWidth(width), extendTimer(0.0f) {}
    
//     void Update() {
//         // 主机：Paddle1（下方）由鼠标控制；客户端：Paddle2（上方）由鼠标控制
//         float mouseX = GetMouseX();
//         rect.x = mouseX - rect.width / 2.0f;
//         // 边界限制
//         if (rect.x < 0) rect.x = 0;
//         if (rect.x + rect.width > GetScreenWidth()) rect.x = GetScreenWidth() - rect.width;
//         // 加长效果计时
//         if (extendTimer > 0) extendTimer -= GetFrameTime();
//         else rect.width = originalWidth;
//     }

//     void Draw() const {
//         DrawRectangleRec(rect, BLUE);
//     }

//     void Extend(float extra, float time) {
//         rect.width = originalWidth + extra;
//         extendTimer = time;
//     }

//     Rectangle GetRect() const { return rect; }
//     void SetX(float x) { rect.x = x; } // 新增：设置挡板X位置（用于同步）
// };

// // 球类
// class Ball {
// private:
//     Vector2 position;
//     Vector2 speed;
//     float radius;
//     float slowTimer;
//     float slowFactor;
// public:
//     Ball(float x, float y, float speedX, float speedY, float radius)
//         : position({x, y}), speed({speedX, speedY}), radius(radius), slowTimer(0.0f), slowFactor(1.0f) {}

//     void Update() {
//         if (slowTimer > 0) {
//             slowTimer -= GetFrameTime();
//             if (slowTimer <= 0) slowFactor = 1.0f;
//         }
//         position.x += speed.x * slowFactor * GetFrameTime();
//         position.y += speed.y * slowFactor * GetFrameTime();
//     }

//     void Draw() const {
//         DrawCircleV(position, radius, WHITE);
//     }

//     void BounceX() { speed.x *= -1; }
//     void BounceY() { speed.y *= -1; }
//     void Slow(float factor, float time) {
//         slowFactor = factor;
//         slowTimer = time;
//     }

//     Vector2 GetPosition() const { return position; }
//     float GetRadius() const { return radius; }
//     Vector2 GetSpeed() const { return speed; }
//     void SetPosition(Vector2 pos) { position = pos; }
//     void SetSpeed(Vector2 spd) { speed = spd; }
//     Ball* Clone() const { return new Ball(position.x, position.y, speed.x, speed.y, radius); }
// };

// // 砖块类
// class Brick {
// private:
//     Rectangle rect;
//     Color color;
//     bool destroyed;
// public:
//     Brick(float x, float y, float width, float height, Color color)
//         : rect({x, y, width, height}), color(color), destroyed(false) {}

//     void Draw() const {
//         if (!destroyed) DrawRectangleRec(rect, color);
//     }

//     void Destroy() { destroyed = true; }
//     bool IsDestroyed() const { return destroyed; }
//     Rectangle GetRect() const { return rect; }
//     Color GetColor() const { return color; }
// };

// // 道具类
// class PowerUp {
// private:
//     Vector2 position;
//     PowerUpType type;
//     bool active;
//     float speed;
// public:
//     PowerUp(float x, float y, PowerUpType type)
//         : position({x, y}), type(type), active(true), speed(150.0f) {}

//     void Update() {
//         if (active) position.y += speed * GetFrameTime();
//         if (position.y > GetScreenHeight()) active = false;
//     }

//     void Draw() const {
//         if (active) {
//             switch (type) {
//                 case PowerUpType::PADDLE_EXTEND: DrawCircleV(position, 10, GREEN); break;
//                 case PowerUpType::MULTI_BALL: DrawCircleV(position, 10, YELLOW); break;
//                 case PowerUpType::SLOW_BALL: DrawCircleV(position, 10, RED); break;
//             }
//         }
//     }

//     bool CheckCollision(Rectangle paddleRect) {
//         return active && CheckCollisionCircleRec(position, 10, paddleRect);
//     }

//     void Activate() { active = false; }
//     bool IsActive() const { return active; }
//     PowerUpType GetType() const { return type; }
//     Vector2 GetPosition() const { return position; }
// };

// // 粒子类
// class Particle {
// private:
//     Vector2 position;
//     Vector2 velocity;
//     Color color;
//     float lifetime;
//     float maxLifetime;
// public:
//     Particle(Vector2 pos, Vector2 vel, Color col, float life)
//         : position(pos), velocity(vel), color(col), lifetime(life), maxLifetime(life) {}

//     void Update() {
//         lifetime -= GetFrameTime();
//         position.x += velocity.x * GetFrameTime();
//         position.y += velocity.y * GetFrameTime();
//         color.a = (unsigned char)(255 * (lifetime / maxLifetime));
//     }

//     void Draw() const {
//         if (lifetime > 0) DrawPixelV(position, color);
//     }

//     bool IsDead() const { return lifetime <= 0; }
// };

// // 核心游戏类（新增网络成员）
// class BreakoutGame {
// private:
//     // 原有游戏成员
//     std::unique_ptr<Paddle> paddle1; // 主机控制的挡板（下方）
//     std::unique_ptr<Paddle> paddle2; // 客户端控制的挡板（上方）
//     std::vector<Ball*> balls;
//     std::vector<Brick*> bricks;
//     std::vector<PowerUp> powerUps;
//     std::vector<Particle> particles;
//     int score;
//     int life;
//     GameStateEnum gameState;
//     bool isBallPaused;

//     // 新增网络成员
//     ENetHost* enetHost;
//     ENetPeer* peer;
//     bool isServer; // true=主机，false=客户端
//     NetworkGameState currentNetState; // 本地游戏状态（用于同步）

//     // 碰撞检测（原有逻辑）
//     void HandleCollisions() {
//         // 球-墙壁碰撞
//         for (auto& ball : balls) {
//             Vector2 p = ball->GetPosition();
//             if (p.x < 8 || p.x > 792) ball->BounceX();
//             if (p.y < 8) ball->BounceY();
//         }

//         // 球-两个挡板碰撞
//         for (auto& ball : balls) {
//             if (CheckCollisionCircleRec(ball->GetPosition(), ball->GetRadius(), paddle1->GetRect())) {
//                 ball->BounceY();
//                 float hitPos = (ball->GetPosition().x - paddle1->GetRect().x) / paddle1->GetRect().width;
//                 Vector2 currentSpeed = ball->GetSpeed();
//                 ball->SetSpeed({(hitPos - 0.5f) * 500.0f, currentSpeed.y});
//             }
//             if (CheckCollisionCircleRec(ball->GetPosition(), ball->GetRadius(), paddle2->GetRect())) {
//                 ball->BounceY();
//                 float hitPos = (ball->GetPosition().x - paddle2->GetRect().x) / paddle2->GetRect().width;
//                 Vector2 currentSpeed = ball->GetSpeed();
//                 ball->SetSpeed({(hitPos - 0.5f) * 500.0f, currentSpeed.y});
//             }
//         }

//         // 球-砖块碰撞
//         std::vector<bool> hit(bricks.size(), false);
//         for (auto& ball : balls) {
//             for (size_t i = 0; i < bricks.size(); i++) {
//                 Brick* brick = bricks[i];
//                 if (!brick->IsDestroyed() && !hit[i] &&
//                     CheckCollisionCircleRec(ball->GetPosition(), ball->GetRadius(), brick->GetRect())) {
//                     ball->BounceY();
//                     brick->Destroy();
//                     score += 10;
//                     hit[i] = true;
//                     // 粒子效果
//                     for (int j = 0; j < 10; j++) {
//                         Vector2 vel = {(float)(rand() % 200 - 100), (float)(rand() % 200 - 100)};
//                         particles.emplace_back(brick->GetRect().x + brick->GetRect().width/2, 
//                                                brick->GetRect().y + brick->GetRect().height/2, 
//                                                brick->GetColor(), vel, 0.5f);
//                     }
//                     // 生成道具
//                     if (rand() % 100 < 30) {
//                         PowerUpType type = (PowerUpType)(rand() % 3);
//                         powerUps.emplace_back(brick->GetRect().x + brick->GetRect().width/2,
//                                               brick->GetRect().y + brick->GetRect().height/2, type);
//                     }
//                 }
//             }
//         }

//         // 挡板-道具碰撞
//         for (auto& pu : powerUps) {
//             if (pu.CheckCollision(paddle1->GetRect())) {
//                 pu.Activate();
//                 switch (pu.GetType()) {
//                     case PowerUpType::PADDLE_EXTEND: paddle1->Extend(60.0f, 5.0f); break;
//                     case PowerUpType::MULTI_BALL: {
//                         std::vector<Ball*> newBalls;
//                         for (auto& ball : balls) newBalls.push_back(ball->Clone());
//                         for (auto& ball : newBalls) balls.push_back(ball);
//                         break;
//                     }
//                     case PowerUpType::SLOW_BALL:
//                         for (auto& ball : balls) ball->Slow(0.6f, 5.0f);
//                         break;
//                 }
//             }
//         }

//         // 球出界处理
//         for (auto it = balls.begin(); it != balls.end();) {
//             if ((*it)->GetPosition().y > 600) {
//                 if (balls.size() == 1) {
//                     life--;
//                     Rectangle paddleRect = paddle1->GetRect();
//                     (*it)->SetPosition({paddleRect.x + paddleRect.width/2, paddleRect.y - 20});
//                     (*it)->SetSpeed({0, 0});
//                     isBallPaused = true;
//                     it++;
//                 } else {
//                     delete *it;
//                     it = balls.erase(it);
//                 }
//             } else {
//                 it++;
//             }
//         }

//         // 清理失效道具/粒子
//         powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
//             [](const PowerUp& pu) { return !pu.IsActive(); }), powerUps.end());
//         particles.erase(std::remove_if(particles.begin(), particles.end(),
//             [](const Particle& p) { return p.IsDead(); }), particles.end());

//         // 更新游戏状态
//         if (life <= 0) gameState = GameStateEnum::GAME_OVER;
//         else if (std::all_of(bricks.begin(), bricks.end(), [](Brick* b){ return b->IsDestroyed(); })) {
//             gameState = GameStateEnum::WIN;
//         }
//     }

//     // 新增：发送游戏状态到网络
//     void SendGameState() {
//         if (!isServer || !peer) return;
//         // 填充网络状态
//         currentNetState.ballX = balls[0]->GetPosition().x;
//         currentNetState.ballY = balls[0]->GetPosition().y;
//         currentNetState.ballSpeedX = balls[0]->GetSpeed().x;
//         currentNetState.ballSpeedY = balls[0]->GetSpeed().y;
//         currentNetState.paddle1X = paddle1->GetRect().x;
//         currentNetState.paddle2X = paddle2->GetRect().x;
//         currentNetState.score = score;
//         currentNetState.life = life;
//         // 序列化并发送
//         ENetPacket* packet = enet_packet_create(&currentNetState, sizeof(NetworkGameState), ENET_PACKET_FLAG_RELIABLE);
//         enet_peer_send(peer, 0, packet);
//     }

//     // 新增：接收网络游戏状态
//     void ReceiveGameState() {
//         if (isServer || !peer) return;
//         ENetEvent event;
//         while (enet_host_service(enetHost, &event, 0) > 0) {
//             if (event.type == ENET_EVENT_TYPE_RECEIVE) {
//                 // 反序列化状态
//                 NetworkGameState* recvState = (NetworkGameState*)event.packet->data;
//                 // 更新本地显示（不运行物理逻辑）
//                 balls[0]->SetPosition({recvState->ballX, recvState->ballY});
//                 balls[0]->SetSpeed({recvState->ballSpeedX, recvState->ballSpeedY});
//                 paddle1->SetX(recvState->paddle1X); // 同步主机挡板
//                 score = recvState->score;
//                 life = recvState->life;
//                 enet_packet_destroy(event.packet);
//             }
//         }
//     }

//     // 新增：发送客户端挡板位置给主机
//     void SendClientPaddlePos() {
//         if (isServer || !peer) return;
//         // 只发送客户端挡板的X位置
//         float paddle2X = paddle2->GetRect().x;
//         ENetPacket* packet = enet_packet_create(&paddle2X, sizeof(float), ENET_PACKET_FLAG_RELIABLE);
//         enet_peer_send(peer, 0, packet);
//     }

//     // 新增：接收客户端挡板位置（主机用）
//     void ReceiveClientPaddlePos() {
//         if (!isServer || !peer) return;
//         ENetEvent event;
//         while (enet_host_service(enetHost, &event, 0) > 0) {
//             if (event.type == ENET_EVENT_TYPE_RECEIVE) {
//                 // 反序列化客户端挡板位置
//                 float* paddle2X = (float*)event.packet->data;
//                 paddle2->SetX(*paddle2X); // 更新客户端挡板位置
//                 enet_packet_destroy(event.packet);
//             }
//         }
//     }

// public:
//     // 构造函数：新增isServer参数（true=主机，false=客户端）
//     BreakoutGame(bool isServer) 
//         : score(0), life(5), gameState(GameStateEnum::START), isBallPaused(true),
//           isServer(isServer), enetHost(nullptr), peer(nullptr) {
//         InitWindow(800, 600, "双人打砖块");
//         SetTargetFPS(60);
//         // 初始化两个挡板（paddle1：下方550，paddle2：上方50）
//         paddle1 = std::make_unique<Paddle>(400, 550, 100, 20);
//         paddle2 = std::make_unique<Paddle>(400, 50, 100, 20);
//         // 初始化球
//         balls.push_back(new Ball(400, 530, 0, 0, 8));
//         // 初始化砖块
//         const Color BRICK_COLORS[] = {RED, GREEN, BLUE, YELLOW, PURPLE};
//         const int BRICK_COLOR_COUNT = sizeof(BRICK_COLORS)/sizeof(Color);
//         for (int y = 0; y < 4; y++) {
//             for (int x = 0; x < 10; x++) {
//                 int colorIndex = (x + y) % BRICK_COLOR_COUNT;
//                 float brickX = 65 + x * 70;
//                 float brickY = 100 + y * 35; // 上移，给paddle2留空间
//                 bricks.push_back(new Brick(brickX, brickY, 60, 25, BRICK_COLORS[colorIndex]));
//             }
//         }

//         // 初始化ENet网络
//         enet_initialize();
//         if (isServer) {
//             // 主机：创建服务器
//             ENetAddress address;
//             enet_address_set_host(&address, "0.0.0.0");
//             address.port = 1234;
//             enetHost = enet_host_create(&address, 1, 2, 0, 0);
//             if (!enetHost) std::cerr << "主机网络初始化失败！" << std::endl;
//         } else {
//             // 客户端：连接主机
//             enetHost = enet_host_create(NULL, 1, 2, 0, 0);
//             if (!enetHost) {
//                 std::cerr << "客户端网络初始化失败！" << std::endl;
//                 return;
//             }
//             ENetAddress serverAddr;
//             enet_address_set_host(&serverAddr, "127.0.0.1");
//             serverAddr.port = 1234;
//             peer = enet_host_connect(enetHost, &serverAddr, 2, 0);
//             // 等待连接
//             ENetEvent event;
//             if (enet_host_service(enetHost, &event, 5000) <= 0 || event.type != ENET_EVENT_TYPE_CONNECT) {
//                 std::cerr << "连接主机失败！" << std::endl;
//                 enet_peer_reset(peer);
//                 peer = nullptr;
//             } else {
//                 std::cout << "连接主机成功！" << std::endl;
//             }
//         }
//     }

//     ~BreakoutGame() {
//         // 清理游戏资源
//         for (auto& ball : balls) delete ball;
//         for (auto& brick : bricks) delete brick;
//         CloseWindow();
//         // 清理网络资源
//         if (peer) enet_peer_disconnect(peer, 0);
//         if (enetHost) enet_host_destroy(enetHost);
//         enet_deinitialize();
//     }

//     // 核心更新函数（区分主机/客户端）
//     void UpdateGame() {
//         // 起始/结束界面逻辑
//         if (gameState == GameStateEnum::START) {
//             paddle1->Update();
//             if (!balls.empty()) {
//                 Rectangle paddleRect = paddle1->GetRect();
//                 balls[0]->SetPosition({paddleRect.x + paddleRect.width/2, paddleRect.y - 20});
//             }
//             if (IsKeyPressed(KEY_SPACE)) {
//                 gameState = GameStateEnum::PLAYING;
//                 isBallPaused = true;
//             }
//             return;
//         }
//         if (gameState == GameStateEnum::GAME_OVER || gameState == GameStateEnum::WIN) {
//             if (IsKeyPressed(KEY_R)) {
//                 // 重置游戏（简化版）
//                 score = 0;
//                 life = 5;
//                 gameState = GameStateEnum::START;
//                 isBallPaused = true;
//             }
//             return;
//         }

//         // 网络逻辑：区分主机/客户端
//         if (isServer) {
//             // 主机：运行完整游戏逻辑
//             paddle1->Update(); // 主机控制paddle1
//             ReceiveClientPaddlePos(); // 接收客户端paddle2位置
            
//             // 球暂停逻辑
//             if (isBallPaused && !balls.empty()) {
//                 Rectangle paddleRect = paddle1->GetRect();
//                 balls[0]->SetPosition({paddleRect.x + paddleRect.width/2, paddleRect.y - 20});
//                 balls[0]->SetSpeed({0, 0});
//                 if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
//                     balls[0]->SetSpeed({0, -250});
//                     isBallPaused = false;
//                 }
//             } else {
//                 for (auto& ball : balls) ball->Update();
//             }

//             HandleCollisions(); // 运行物理碰撞
//             SendGameState(); // 同步状态给客户端
//         } else {
//             // 客户端：只控制paddle2 + 接收状态
//             paddle2->Update(); // 客户端控制paddle2
//             ReceiveGameState(); // 接收主机的游戏状态
//             SendClientPaddlePos(); // 发送paddle2位置给主机
//         }

//         // 通用更新：道具、粒子
//         for (auto& pu : powerUps) pu.Update();
//         for (auto& p : particles) p.Update();
//     }

//     // 绘制函数（双人挡板）
//     void DrawGame() {
//         BeginDrawing();
//         ClearBackground(BLACK);

//         // 绘制游戏元素
//         paddle1->Draw();
//         paddle2->Draw(); // 绘制第二个挡板
//         for (auto& ball : balls) ball->Draw();
//         for (auto& brick : bricks) brick->Draw();
//         for (auto& pu : powerUps) pu.Draw();
//         for (auto& p : particles) p.Draw();

//         // 绘制UI
//         DrawText(TextFormat("Score: %d", score), 10, 10, 20, WHITE);
//         DrawText(TextFormat("Life: %d", life), 700, 10, 20, WHITE);

//         // 起始/结束界面
//         if (gameState == GameStateEnum::START) {
//             DrawText("按Space开始游戏", 250, 300, 30, WHITE);
//         } else if (gameState == GameStateEnum::GAME_OVER) {
//             DrawText("游戏结束！按R重启", 250, 300, 30, RED);
//         } else if (gameState == GameStateEnum::WIN) {
//             DrawText("恭喜获胜！按R重启", 250, 300, 30, GREEN);
//         }

//         EndDrawing();
//     }

//     // 游戏主循环
//     void Run() {
//         while (!WindowShouldClose()) {
//             UpdateGame();
//             DrawGame();
//         }
//     }
// };

#pragma once
#include <iostream>
#include <raylib.h>
#include <vector>
#include <memory>
#include <enet/enet.h>

// 道具类型枚举
enum class PowerUpType {
    PADDLE_EXTEND,
    MULTI_BALL,
    SLOW_BALL
};

// 游戏状态枚举
enum class GameStateEnum {
    START,
    PLAYING,
    GAME_OVER,
    WIN
};

// 网络同步的游戏状态结构体（必须和客户端一致）
struct NetworkGameState {
    float ballX, ballY;
    float ballSpeedX, ballSpeedY;
    float paddle1X, paddle2X;
    int score;
    int life;
};

// 挡板类
class Paddle {
private:
    Rectangle rect;
    float originalWidth;
    float extendTimer;
public:
    Paddle(float x, float y, float width, float height) 
        : rect({x, y, width, height}), originalWidth(width), extendTimer(0.0f) {}
    
    void Update() {
        float mouseX = GetMouseX();
        rect.x = mouseX - rect.width / 2.0f;
        if (rect.x < 0) rect.x = 0;
        if (rect.x + rect.width > GetScreenWidth()) rect.x = GetScreenWidth() - rect.width;
        
        if (extendTimer > 0) extendTimer -= GetFrameTime();
        else rect.width = originalWidth;
    }

    void Draw() const { DrawRectangleRec(rect, BLUE); }
    void Extend(float extra, float time) { rect.width = originalWidth + extra; extendTimer = time; }
    Rectangle GetRect() const { return rect; }
    void SetX(float x) { rect.x = x; }
};

// 球类
class Ball {
private:
    Vector2 position;
    Vector2 speed;
    float radius;
    float slowTimer;
    float slowFactor;
public:
    Ball(float x, float y, float speedX, float speedY, float radius)
        : position({x, y}), speed({speedX, speedY}), radius(radius), slowTimer(0.0f), slowFactor(1.0f) {}

    void Update() {
        if (slowTimer > 0) {
            slowTimer -= GetFrameTime();
            if (slowTimer <= 0) slowFactor = 1.0f;
        }
        position.x += speed.x * slowFactor * GetFrameTime();
        position.y += speed.y * slowFactor * GetFrameTime();
    }

    void Draw() const { DrawCircleV(position, radius, WHITE); }
    void BounceX() { speed.x *= -1; }
    void BounceY() { speed.y *= -1; }
    void Slow(float factor, float time) { slowFactor = factor; slowTimer = time; }

    Vector2 GetPosition() const { return position; }
    float GetRadius() const { return radius; }
    Vector2 GetSpeed() const { return speed; }
    void SetPosition(Vector2 pos) { position = pos; }
    void SetSpeed(Vector2 spd) { speed = spd; }
    Ball* Clone() const { return new Ball(position.x, position.y, speed.x, speed.y, radius); }
};

// 砖块类
class Brick {
private:
    Rectangle rect;
    Color color;
    bool destroyed;
public:
    Brick(float x, float y, float width, float height, Color color)
        : rect({x, y, width, height}), color(color), destroyed(false) {}

    void Draw() const { if (!destroyed) DrawRectangleRec(rect, color); }
    void Destroy() { destroyed = true; }
    bool IsDestroyed() const { return destroyed; }
    Rectangle GetRect() const { return rect; }
    Color GetColor() const { return color; }
};

// 道具类
class PowerUp {
private:
    Vector2 position;
    PowerUpType type;
    bool active;
    float speed;
public:
    PowerUp(float x, float y, PowerUpType type)
        : position({x, y}), type(type), active(true), speed(150.0f) {}

    void Update() {
        if (active) position.y += speed * GetFrameTime();
        if (position.y > GetScreenHeight()) active = false;
    }

    void Draw() const {
        if (active) {
            switch (type) {
                case PowerUpType::PADDLE_EXTEND: DrawCircleV(position, 10, GREEN); break;
                case PowerUpType::MULTI_BALL: DrawCircleV(position, 10, YELLOW); break;
                case PowerUpType::SLOW_BALL: DrawCircleV(position, 10, RED); break;
            }
        }
    }

    bool CheckCollision(Rectangle paddleRect) {
        return active && CheckCollisionCircleRec(position, 10, paddleRect);
    }

    void Activate() { active = false; }
    bool IsActive() const { return active; }
    PowerUpType GetType() const { return type; }
    Vector2 GetPosition() const { return position; }
};

// 粒子类
class Particle {
private:
    Vector2 position;
    Vector2 velocity;
    Color color;
    float lifetime;
    float maxLifetime;
public:
    Particle(Vector2 pos, Vector2 vel, Color col, float life)
        : position(pos), velocity(vel), color(col), lifetime(life), maxLifetime(life) {}

    void Update() {
        lifetime -= GetFrameTime();
        position.x += velocity.x * GetFrameTime();
        position.y += velocity.y * GetFrameTime();
        color.a = (unsigned char)(255 * (lifetime / maxLifetime));
    }

    void Draw() const { if (lifetime > 0) DrawPixelV(position, color); }
    bool IsDead() const { return lifetime <= 0; }
};

// 核心游戏类（主机专用，和你的实现完全匹配）
class BreakoutGame {
private:
    // 游戏核心成员（和你原来的变量名一致）
    std::unique_ptr<Paddle> paddle1; // 主机控制的下方挡板
    std::unique_ptr<Paddle> paddle2; // 客户端控制的上方挡板
    std::vector<Ball*> balls;
    std::vector<Brick*> bricks;
    std::vector<PowerUp> powerUps;
    std::vector<Particle> particles;
    int score;
    int life;
    GameStateEnum gameState;
    bool isBallPaused;

    // 网络成员（主机用）
    ENetHost* enetHost;
    ENetPeer* clientPeer;

    // 碰撞检测（原有逻辑）
    void HandleCollisions();

    // 网络同步函数
    void SendGameState();
    void ReceiveClientInput();

public:
    // 构造/析构
    BreakoutGame();
    ~BreakoutGame();

    // 主循环
    void UpdateGame();
    void DrawGame();
    void Run();
};