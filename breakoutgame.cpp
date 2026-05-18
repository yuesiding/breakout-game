#include "breakoutgame.h"
#include <enet/enet.h>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include "json.hpp"
using json = nlohmann::json;
// 关卡配置结构体
struct LevelCfg
{
    int level_id;
    int rows;
    int cols;
    float ball_speed;
    float paddle_speed;
    std::vector<std::vector<int>> layout;
};
// 存储所有关卡配置
std::vector<LevelCfg> allLevels;
bool hasSaveFile = false;

SaveData gameSave;

#include <thread>     // 多线程
#include <mutex>      // 线程安全（锁）
#include <future>     // 异步加载
#include <chrono>     // 时间

// 保存提示全局变量（放cpp最顶部include下面）
bool showSaveTip = false;
float saveTipTime = 0.0f;

void BreakoutGame::LoadLevelData(int lid)
{
    if(lid < 1 || lid > 3) return;
    nowLevel = lid;

    LevelCfg targetCfg{};
    bool findOk = false;
    for(auto &c : allLevels)
    {
        if(c.level_id == lid)
        {
            targetCfg = c;
            findOk = true;
            break;
        }
    }
    if(!findOk) return;

    // 赋值速度倍率
    curBallSpeedRate = targetCfg.ball_speed;
    curPaddleSpeedRate = targetCfg.paddle_speed;

    // 清空旧砖块
    for(auto &b : bricks) delete b;
    bricks.clear();

    const Color BRICK_COLORS[] = {RED, GREEN, BLUE, YELLOW, PURPLE};
    int r = targetCfg.rows;
    int c = targetCfg.cols;

    for(int y = 0; y < r; y++)
    {
        for(int x = 0; x < c; x++)
        {
            if(targetCfg.layout[y][x] == 1)
            {
                int idx = (x + y) % 5;
                float bx = 65 + x * 70;
                float by = 100 + y * 35;
                bricks.push_back(new Brick(bx, by, 60, 25, BRICK_COLORS[idx]));
            }
        }
    }

    // 重置球位置
    if(!balls.empty())
    {
        Rectangle rect = paddle1->GetRect();
        balls[0]->SetPosition({rect.x + rect.width/2, rect.y - 20});
        balls[0]->SetSpeed({0,0});
    }
    isBallPaused = true;
}

void saveLevel(const std::string& filename, const LevelData& level) {
    json j;
    j["rows"] = level.rows;
    j["cols"] = level.cols;
    j["layout"] = level.brickLayout;

    std::ofstream file(filename);
    if (file.is_open()) {
        file << j.dump(4); // 4个空格缩进，方便阅读
        file.close();
    }
}

LevelData loadLevel(const std::string& filename) {
    LevelData level;
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        return level;
    }

    json j;
    file >> j;
    file.close();

    level.rows = j["rows"];
    level.cols = j["cols"];
    level.brickLayout = j["layout"].get<std::vector<std::vector<int>>>();

    return level;
}

// ====================== 第10课 性能优化：粒子对象池 ======================
const int MAX_PARTICLES = 500;  // 池大小

struct PooledParticle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    bool inUse;
};

PooledParticle particlePool[MAX_PARTICLES];

// 从对象池获取一个空闲粒子（不new、不delete）
void SpawnParticleFromPool(Vector2 pos, Color color) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particlePool[i].inUse) {
            particlePool[i].position = pos;
            particlePool[i].velocity = {
                (float)(rand() % 200 - 100),
                (float)(rand() % 200 - 100)
            };
            particlePool[i].color = color;
            particlePool[i].life = 0.5f;
            particlePool[i].inUse = true;
            break;
        }
    }
}

// 更新对象池里所有粒子
void UpdateParticlePool() {
    float dt = GetFrameTime();
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particlePool[i].inUse) {
            particlePool[i].position.x += particlePool[i].velocity.x * dt;
            particlePool[i].position.y += particlePool[i].velocity.y * dt;
            particlePool[i].life -= dt;

            if (particlePool[i].life <= 0) {
                particlePool[i].inUse = false;
            }
        }
    }
}

// 绘制对象池里所有粒子
void DrawParticlePool() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particlePool[i].inUse) {
            DrawCircleV(particlePool[i].position, 2, particlePool[i].color);
        }
    }
}

// ===================== 多线程 第二步 =====================
// 加载状态（对应PPT要求）
enum class LoadState
{
    IDLE,     // 空闲：没在加载
    LOADING,  // 正在异步加载资源
    DONE      // 加载完成
};

// 全局变量：记录当前加载状态
LoadState currentLoadState = LoadState::IDLE;
// 新增：保存异步任务的future
std::future<void> loadFuture;
bool showLoadComplete = false;
float completeTimer = 0.0f;

// 线程安全锁（防止多线程冲突，PPT 必考点）
std::mutex loadMutex;

// 后台加载函数（在另一个线程运行，不卡游戏）
void LoadResourcesAsync()
{
    // 模拟加载资源：等待2秒（比如加载图片、声音、地图）
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 线程安全地修改状态
    std::lock_guard<std::mutex> lock(loadMutex);
    currentLoadState = LoadState::DONE;
}


// 碰撞检测实现
void BreakoutGame::HandleCollisions() {
    // ====================== 【优化课：开始计时】 ======================
    //double startTime = GetTime();

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

                // 【对象池优化】不再new粒子，从池里取
for (int j = 0; j < 10; j++) {
    SpawnParticleFromPool(
        { brick->GetRect().x + brick->GetRect().width/2, brick->GetRect().y + brick->GetRect().height/2 },
        brick->GetColor()
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
    // particles.erase(std::remove_if(particles.begin(), particles.end(),
    //     [](const Particle& p) { return p.IsDead(); }), particles.end());

    // 更新游戏状态
    if (life <= 0) gameState = GameStateEnum::GAME_OVER;
    else if (std::all_of(bricks.begin(), bricks.end(), [](Brick* b){ return b->IsDestroyed(); })) {
        gameState = GameStateEnum::WIN;
    }

    // ====================== 【优化课：结束计时 + 输出】 ======================
    //double elapsed = GetTime() - startTime;
    //TraceLog(LOG_INFO, "【碰撞检测耗时】：%.3f ms", elapsed * 1000);
}

// 发送游戏状态给客户端
void BreakoutGame::SendGameState() {
    if (!clientPeer) return;

    NetworkGameState state;
    state.ballX = balls[0]->GetPosition().x;
    state.ballY = balls[0]->GetPosition().y;
    state.ballSpeedX = balls[0]->GetSpeed().x;
    state.ballSpeedY = balls[0]->GetSpeed().y;
    state.paddle1X = paddle1->GetRect().x;
    state.paddle2X = paddle2->GetRect().x;
    state.score = score;
    state.life = life;

    ENetPacket* packet = enet_packet_create(&state, sizeof(NetworkGameState), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(clientPeer, 0, packet);
}

void BreakoutGame::ReceiveClientInput() {
    ENetEvent event;
    if (enet_host_service(enetHost, &event, 1) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                std::cout << "✅ 客户端已连接！" << std::endl;
                clientPeer = event.peer;
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE: {
                float paddle2X = *(float*)event.packet->data;
                paddle2->SetX(paddle2X);
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT: {
                std::cout << "❌ 客户端已断开连接！" << std::endl;
                clientPeer = nullptr;
                break;
            }
            default:
                break;
        }
    }
}

// 构造函数
BreakoutGame::BreakoutGame() 
    : score(0), life(5), gameState(GameStateEnum::START), isBallPaused(true), enetHost(nullptr), clientPeer(nullptr),nowLevel(1), curBallSpeedRate(1.0f), curPaddleSpeedRate(1.0f)
     {
    // 启动时自动读档
SetTraceLogLevel(LOG_NONE);
// 读取外部关卡配置文件 level.json
std::ifstream levelFile("level.json");
if(levelFile.is_open())
{
    json levJson;
    levelFile >> levJson;
    levelFile.close();
    allLevels.clear();
    for(auto &item : levJson["levels"])
    {
        LevelCfg cfg;
        cfg.level_id = item["level_id"];
        cfg.rows = item["rows"];
        cfg.cols = item["cols"];
        cfg.ball_speed = item["ball_speed"];
        cfg.paddle_speed = item["paddle_speed"];
        cfg.layout = item["layout"].get<std::vector<std::vector<int>>>();
        allLevels.push_back(cfg);
    }
}
    // 开机检测是否有存档文件
// 开机检测是否有存档文件
std::ifstream checkFile("save.json");
if(checkFile.is_open())
{
    hasSaveFile = true;
    json j;
    checkFile >> j;
    checkFile.close();
    
    gameSave.score = j["score"];
    gameSave.life = j["life"];
    gameSave.level.rows = j["level"]["rows"];
    gameSave.level.cols = j["level"]["cols"];
    gameSave.level.brickLayout = j["level"]["layout"].get<std::vector<std::vector<int>>>();
    gameSave.ballPos = {j["ballPos"][0], j["ballPos"][1]};
    gameSave.ballSpeed = {j["ballSpeed"][0], j["ballSpeed"][1]};
    
    // 进入读档菜单
    gameState = GameStateEnum::LOAD_MENU;
}
else
{
    hasSaveFile = false;
    gameState = GameStateEnum::START;
}
    
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
    // ==============================================
// 第8课 多线程：按 L 键启动异步加载
// ==============================================
if (IsKeyPressed(KEY_L))
{
    // 只有空闲状态才能开始加载
    std::lock_guard<std::mutex> lock(loadMutex);
    if (currentLoadState == LoadState::IDLE)
    {
        // 切换为加载中状态
        currentLoadState = LoadState::LOADING;

        // 启动异步线程（后台加载，游戏不卡顿）
        loadFuture = std::async(std::launch::async, LoadResourcesAsync);
    }
}

// ==============================================
// 第8课：检查异步加载是否完成
// ==============================================
{
    std::lock_guard<std::mutex> lock(loadMutex);

    // 如果正在加载，并且任务已经完成
    if (currentLoadState == LoadState::LOADING)
    {
        if (loadFuture.valid() &&
            loadFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            // 切换成完成状态
            currentLoadState = LoadState::DONE;
        }
    }

    // ==============================================
    // 加载完成后：做一个效果（例如砖块变红）
    // ==============================================
    if (currentLoadState == LoadState::DONE)
{
    showLoadComplete = true;
    completeTimer = 2.0f; // 显示2秒
    currentLoadState = LoadState::IDLE;
}

// 计时器倒计时
if (showLoadComplete)
{
    completeTimer -= GetFrameTime();
    if (completeTimer <= 0)
    {
        showLoadComplete = false; // 时间到，消失
    }
}
}

// ========== 必须放这里：存档选择界面 ==========
    if (gameState == GameStateEnum::LOAD_MENU)
    {
        // 按1继续上次游戏
       // 按1继续上次游戏
if(IsKeyPressed(KEY_ONE))
{
    // 1. 恢复分数和生命
    score = gameSave.score;
    life = gameSave.life;

    // 2. 加载砖块布局
    for (auto& b : bricks) delete b;
    bricks.clear();
    const Color BRICK_COLORS[] = {RED, GREEN, BLUE, YELLOW, PURPLE};
    const int BRICK_COLOR_COUNT = sizeof(BRICK_COLORS)/sizeof(Color);
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 10; x++) {
            int colorIndex = (x + y) % BRICK_COLOR_COUNT;
            float brickX = 65 + x * 70;
            float brickY = 100 + y * 35;
            Brick* brick = new Brick(brickX, brickY, 60, 25, BRICK_COLORS[colorIndex]);
            if (!gameSave.level.brickLayout[y][x]) {
                brick->Destroy(); // 恢复砖块是否被打碎的状态
            }
            bricks.push_back(brick);
        }
    }

    // 3. 恢复球的位置和速度
    if (!balls.empty()) {
        balls[0]->SetPosition(gameSave.ballPos);
        balls[0]->SetSpeed(gameSave.ballSpeed);
    }

    // 4. 进入游戏状态
    gameState = GameStateEnum::PLAYING;
    isBallPaused = true;
}
        // 按2全新开局
        if(IsKeyPressed(KEY_TWO))
        {
            score = 0;
            life = 5;
            gameState = GameStateEnum::START;
            isBallPaused = true;
        }
        ReceiveClientInput();
        return;
    }



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

    // S保存 K加载
// 【S键】保存完整游戏状态
if (IsKeyPressed(KEY_S) && gameState == GameStateEnum::PLAYING)
{
    // 1. 保存关卡布局
    LevelData data;
    data.rows = 4;
    data.cols = 10;
    data.brickLayout.resize(4, std::vector<int>(10, 1));
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 10; x++) {
            data.brickLayout[y][x] = !bricks[y*10 + x]->IsDestroyed();
        }
    }

    // 2. 保存完整游戏数据
    gameSave.score = score;
    gameSave.life = life;
    gameSave.level = data;
    gameSave.ballPos = balls[0]->GetPosition();
    gameSave.ballSpeed = balls[0]->GetSpeed();

    // 3. 写入文件
    json j;
    j["score"] = gameSave.score;
    j["life"] = gameSave.life;
    j["level"]["rows"] = gameSave.level.rows;
    j["level"]["cols"] = gameSave.level.cols;
    j["level"]["layout"] = gameSave.level.brickLayout;
    j["ballPos"] = {gameSave.ballPos.x, gameSave.ballPos.y};
    j["ballSpeed"] = {gameSave.ballSpeed.x, gameSave.ballSpeed.y};

    std::ofstream saveFile("save.json");
    if (saveFile.is_open()) {
        saveFile << j.dump(4);
        saveFile.close();
    }

    // 4. 暂停游戏并显示提示
    isBallPaused = true;
    showSaveTip = true;
    saveTipTime = 1.5f;
}

if (showSaveTip)
{
    saveTipTime -= GetFrameTime();
    if (saveTipTime <= 0)
        showSaveTip = false;
}

if (IsKeyPressed(KEY_K)) 
{
    LevelData loaded = loadLevel("level.json");
    for (auto& b : bricks) delete b;
    bricks.clear();
    
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
}
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
        // 5. 发球逻辑绑定球速倍率（按关卡变快）
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
    {
        float baseSpeed = 250.0f;
        // 球速 = 基础速度 × 当前关卡倍率
        balls[0]->SetSpeed({0, -baseSpeed * curBallSpeedRate});
        isBallPaused = false;
    }
    } else {
        for (auto& ball : balls) ball->Update();
    }

        HandleCollisions();
    for (auto& pu : powerUps) pu.Update();
    UpdateParticlePool();
    if (std::all_of(bricks.begin(), bricks.end(), [](Brick* b) {
        return b->IsDestroyed();
    })) {
        if (nowLevel < 3) {
            LoadLevelData(nowLevel + 1);  // 自动进下一关
        } else {
            gameState = GameStateEnum::WIN; // 3关通关
        }
    }

    SendGameState();
}
// 绘制游戏
void BreakoutGame::DrawGame() {
    BeginDrawing();
    ClearBackground(BLACK);

    // 在 DrawGame() 里加入这段代码，用来显示加载动画
{
    std::lock_guard<std::mutex> lock(loadMutex);
    if (currentLoadState == LoadState::LOADING)
    {
        DrawText("LOADING...", 320, 270, 40, YELLOW);
    }
}
DrawText(TextFormat("LEVEL : %d", nowLevel), 20, 260, 26, SKYBLUE);
// 加载完成提示
if (showLoadComplete)
{
    DrawText("LOAD COMPLETE!", 240, 270, 40, GREEN);
}

    // 绘制游戏元素
    paddle1->Draw();
    paddle2->Draw();
    for (auto& ball : balls) ball->Draw();
    for (auto& brick : bricks) brick->Draw();
    for (auto& pu : powerUps) pu.Draw();
    // 【对象池】绘制粒子
    DrawParticlePool();

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
     // 绘制保存成功文字
if(showSaveTip)
{
    DrawText("SAVE SUCCESS !", 260, 260, 35, GREEN);
}

// 存档选择菜单（英文）
if(gameState == GameStateEnum::LOAD_MENU)
{
    DrawText("SAVE FILE DETECTED", 220, 200, 40, YELLOW);
    DrawText("PRESS 1 -> CONTINUE GAME", 230, 280, 30, WHITE);
    DrawText("PRESS 2 -> NEW GAME", 270, 330, 30, WHITE);
}

// // 保存成功提示（英文）
// if(showSaveTip)
// {
//     DrawText("GAME SAVED!", 280, 260, 35, GREEN);
// }

     // 性能数据（屏幕正上方居中）
DrawText(TextFormat("FPS: %d | Frame time: %.2f ms", GetFPS(), GetFrameTime() * 1000), 280, 10, 20, YELLOW);

    EndDrawing();
}

// 主循环
void BreakoutGame::Run() {
    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }
}