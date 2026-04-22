#include "breakoutgame.h"
#include <enet/enet.h>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>

// 碰撞检测实现
void BreakoutGame::HandleCollisions() {
    // 球-墙壁碰撞
    for (auto& ball : balls) {
        Vector2 p = ball->GetPosition();
        if (p.x < 8 || p.x > 792) ball->BounceX();
        if (p.y < 8) ball->BounceY();
    }

    // 球-两个挡板碰撞
    for (auto& ball : balls) {
        if (CheckCollisionCircleRec(ball->GetPosition(), ball->GetRadius(), paddle1->GetRect())) {
            ball->BounceY();
            float hitPos = (ball->GetPosition().x - paddle1->GetRect().x) / paddle1->GetRect().width;
            ball->SetSpeed({(hitPos - 0.5f) * 500.0f, ball->GetSpeed().y});
        }
        if (CheckCollisionCircleRec(ball->GetPosition(), ball->GetRadius(), paddle2->GetRect())) {
            ball->BounceY();
            float hitPos = (ball->GetPosition().x - paddle2->GetRect().x) / paddle2->GetRect().width;
            ball->SetSpeed({(hitPos - 0.5f) * 500.0f, ball->GetSpeed().y});
        }
    }

    // 球-砖块碰撞
    std::vector<bool> hit(bricks.size(), false);
    for (auto& ball : balls) {
        for (size_t i = 0; i < bricks.size(); i++) {
            Brick* brick = bricks[i];
            if (!brick->IsDestroyed() && !hit[i] &&
                CheckCollisionCircleRec(ball->GetPosition(), ball->GetRadius(), brick->GetRect())) {
                ball->BounceY();
                brick->Destroy();
                score += 10;
                hit[i] = true;

                // 粒子效果
                for (int j = 0; j < 10; j++) {
                    Vector2 vel = {(float)(rand() % 200 - 100), (float)(rand() % 200 - 100)};
                    particles.emplace_back(
                        Vector2{brick->GetRect().x + brick->GetRect().width/2, brick->GetRect().y + brick->GetRect().height/2},
                        vel, brick->GetColor(), 0.5f
                    );
                }

                // 生成道具
                if (rand() % 100 < 30) {
                    PowerUpType type = (PowerUpType)(rand() % 3);
                    powerUps.emplace_back(
                        brick->GetRect().x + brick->GetRect().width/2,
                        brick->GetRect().y + brick->GetRect().height/2,
                        type
                    );
                }
            }
        }
    }

    // 挡板-道具碰撞
    for (auto& pu : powerUps) {
        if (pu.CheckCollision(paddle1->GetRect())) {
            pu.Activate();
            switch (pu.GetType()) {
                case PowerUpType::PADDLE_EXTEND: paddle1->Extend(60.0f, 5.0f); break;
                case PowerUpType::MULTI_BALL: {
                    std::vector<Ball*> newBalls;
                    for (auto& ball : balls) newBalls.push_back(ball->Clone());
                    for (auto& ball : newBalls) balls.push_back(ball);
                    break;
                }
                case PowerUpType::SLOW_BALL:
                    for (auto& ball : balls) ball->Slow(0.6f, 5.0f);
                    break;
            }
        }
    }

    // 球出界处理
    for (auto it = balls.begin(); it != balls.end();) {
        if ((*it)->GetPosition().y > 600) {
            if (balls.size() == 1) {
                life--;
                Rectangle paddleRect = paddle1->GetRect();
                (*it)->SetPosition({paddleRect.x + paddleRect.width/2, paddleRect.y - 20});
                (*it)->SetSpeed({0, 0});
                isBallPaused = true;
                it++;
            } else {
                delete *it;
                it = balls.erase(it);
            }
        } else {
            it++;
        }
    }

    // 清理失效道具/粒子
    powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
        [](const PowerUp& pu) { return !pu.IsActive(); }), powerUps.end());
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.IsDead(); }), particles.end());

    // 更新游戏状态
    if (life <= 0) gameState = GameStateEnum::GAME_OVER;
    else if (std::all_of(bricks.begin(), bricks.end(), [](Brick* b){ return b->IsDestroyed(); })) {
        gameState = GameStateEnum::WIN;
    }
}

void BreakoutGame::SendGameState() {
    if (!clientPeer || balls.empty()) return;

    NetworkGameState state;
    state.ballX = balls[0]->GetPosition().x;
    state.ballY = balls[0]->GetPosition().y;
    state.ballSpeedX = balls[0]->GetSpeed().x;
    state.ballSpeedY = balls[0]->GetSpeed().y;
    state.paddle1X = paddle1->GetRect().x;
    state.paddle2X = paddle2->GetRect().x;
    state.score = score;
    state.life = life;

    // 确保发送结构体大小正确
    ENetPacket* packet = enet_packet_create(&state, sizeof(NetworkGameState), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(clientPeer, 0, packet);
}

// 保存上一次客户端的位置（用于插值）
float lastReceivedPaddle2X = 400.0f;  
// 当前真正显示的位置（平滑后）
float currentDisplayPaddle2X = 400.0f;

void BreakoutGame::ReceiveClientInput() {
    ENetEvent event;

    if (enet_host_service(enetHost, &event, 1) > 0)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
                clientPeer = event.peer;
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                if (event.packet->dataLength == sizeof(float))
                {
                    // 只更新【目标位置】，不直接跳过去
                    lastReceivedPaddle2X = *(float*)event.packet->data;
                }
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                clientPeer = nullptr;
                break;

            default: break;
        }
    }

    // ==============================
    // ✅ 插值：平滑移动到目标位置
    // ==============================
    const float smoothSpeed = 0.25f; // 数值越小越丝滑
    currentDisplayPaddle2X = currentDisplayPaddle2X + 
                             (lastReceivedPaddle2X - currentDisplayPaddle2X) * smoothSpeed;

    // 把平滑后的位置给板子
    paddle2->SetX(currentDisplayPaddle2X);
}

// 构造函数
BreakoutGame::BreakoutGame() 
    : score(0), life(5), gameState(GameStateEnum::START), isBallPaused(true), enetHost(nullptr), clientPeer(nullptr) {
    // 初始化窗口
    InitWindow(800, 600, "双人打砖块 ");
    SetTargetFPS(60);
    srand(time(NULL));

    // 初始化两个挡板
    paddle1 = std::make_unique<Paddle>(400, 550, 100, 20);
    paddle2 = std::make_unique<Paddle>(400, 50, 100, 20);

    // 初始化球
    balls.push_back(new Ball(400, 530, 0, 0, 8));

    // 初始化砖块
    const Color BRICK_COLORS[] = {RED, GREEN, BLUE, YELLOW, PURPLE};
    const int BRICK_COLOR_COUNT = sizeof(BRICK_COLORS)/sizeof(Color);
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 10; x++) {
            int colorIndex = (x + y) % BRICK_COLOR_COUNT;
            float brickX = 65 + x * 70;
            float brickY = 100 + y * 35;
            bricks.push_back(new Brick(brickX, brickY, 60, 25, BRICK_COLORS[colorIndex]));
        }
    }

    // 初始化ENet服务器（保留，不影响本地键盘控制）
    enet_initialize();
    ENetAddress address;
    enet_address_set_host(&address, "0.0.0.0");
    address.port = 1234;
    enetHost = enet_host_create(&address, 1, 2, 0, 0);

    if (!enetHost) {
        std::cerr << "❌ 主机网络初始化失败！" << std::endl;
        CloseWindow();
        exit(1);
    }
    std::cout << "🔌 主机已启动 等待客户端连接 :端口1234..." << std::endl;
}

// 析构函数
BreakoutGame::~BreakoutGame() {
    // 清理游戏资源
    for (auto& ball : balls) delete ball;
    for (auto& brick : bricks) delete brick;
    CloseWindow();

    // 清理网络资源
    if (clientPeer) enet_peer_disconnect(clientPeer, 0);
    if (enetHost) enet_host_destroy(enetHost);
    enet_deinitialize();
}

// 更新游戏逻辑
void BreakoutGame::UpdateGame() {
    // 起始界面
    if (gameState == GameStateEnum::START) {
        paddle1->Update();
        if (!balls.empty()) {
            Rectangle paddleRect = paddle1->GetRect();
            balls[0]->SetPosition({paddleRect.x + paddleRect.width/2, paddleRect.y - 20});
        }
        if (IsKeyPressed(KEY_SPACE)) {
            gameState = GameStateEnum::PLAYING;
            isBallPaused = true;
        }
        ReceiveClientInput();
        return;
    }

    // 结束界面
    if (gameState == GameStateEnum::GAME_OVER || gameState == GameStateEnum::WIN) {
        if (IsKeyPressed(KEY_R)) {
            score = 0;
            life = 5;
            gameState = GameStateEnum::START;
            isBallPaused = true;
        }
        ReceiveClientInput();
        return;
    }

    // 游戏运行中
    ReceiveClientInput();
    paddle1->Update();

    // 上方板键盘控制（灵敏 + 全屏移动）
    const float paddle2Speed = 18.0f;
    float x = paddle2->GetRect().x;

    if (IsKeyDown(KEY_LEFT))  x -= paddle2Speed;
    if (IsKeyDown(KEY_RIGHT)) x += paddle2Speed;

    // 边界限制：真正全屏可移动
    if (x < 0) x = 0;
    if (x + paddle2->GetRect().width > 800)
    x = 800 - paddle2->GetRect().width;

    paddle2->SetX(x);

    // 球暂停逻辑
    if (isBallPaused && !balls.empty()) {
        Rectangle paddleRect = paddle1->GetRect();
        balls[0]->SetPosition({paddleRect.x + paddleRect.width/2, paddleRect.y - 20});
        balls[0]->SetSpeed({0, 0});
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            balls[0]->SetSpeed({0, -250});
            isBallPaused = false;
        }
    } else {
        for (auto& ball : balls) ball->Update();
    }

    // 运行物理逻辑
    HandleCollisions();
    for (auto& pu : powerUps) pu.Update();
    for (auto& p : particles) p.Update();

    // 同步状态给客户端（可选）
    SendGameState();
}

// 绘制游戏
void BreakoutGame::DrawGame() {
    BeginDrawing();
    ClearBackground(BLACK);

    // 绘制游戏元素
    paddle1->Draw();
    paddle2->Draw();
    for (auto& ball : balls) ball->Draw();
    for (auto& brick : bricks) brick->Draw();
    for (auto& pu : powerUps) pu.Draw();
    for (auto& p : particles) p.Draw();

    // 绘制UI
    DrawText(TextFormat("Score: %d", score), 10, 10, 20, WHITE);
    DrawText(TextFormat("Life: %d", life), 700, 10, 20, WHITE);
    DrawText("main:control the upper paddle", 10, 570, 18, WHITE);

    // 状态提示
    if (gameState == GameStateEnum::START) {
        DrawText("Press Space to Start", 250, 300, 30, WHITE);
    } else if (gameState == GameStateEnum::GAME_OVER) {
        DrawText("Game Over! Press R to restart", 250, 300, 30, RED);
    } else if (gameState == GameStateEnum::WIN) {
        DrawText("Win! Press R to restart", 250, 300, 30, GREEN);
    }

    if (clientPeer) DrawText("client has connected", 600, 570, 20, GREEN);
     else DrawText("waiting for client...", 600, 570, 20, RED);

    EndDrawing();
}

// 主循环
void BreakoutGame::Run() {
    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }
}