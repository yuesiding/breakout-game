#include <raylib.h>
#include <enet/enet.h>
#include <iostream>

struct NetworkGameState
{
    float ballX, ballY;
    float ballSpeedX, ballSpeedY;
    float paddle1X;
    float paddle2X;
    int score;
    int life;
};

int main()
{
    InitWindow(800, 600, "Client");
    SetTargetFPS(60);

    enet_initialize();
    ENetHost* client = enet_host_create(NULL, 1, 2, 0, 0);

    ENetAddress addr;
    enet_address_set_host(&addr, "127.0.0.1");
    addr.port = 1234;
    ENetPeer* peer = enet_host_connect(client, &addr, 2, 0);

    NetworkGameState state = {};
    bool connected = false;
    ENetEvent event;

    float paddleX = 350;
    const float speed = 8;

    if (enet_host_service(client, &event, 2000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        connected = true;
        std::cout << "✅ Connected\n";
    }

    while (!WindowShouldClose())
    {
        // 接收主机状态
        if (enet_host_service(client, &event, 0) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
            {
                state = *(NetworkGameState*)event.packet->data;
                enet_packet_destroy(event.packet);
            }
        }

        // 键盘控制
        if (IsKeyDown(KEY_LEFT))  paddleX -= speed;
        if (IsKeyDown(KEY_RIGHT)) paddleX += speed;

        if (paddleX < 0) paddleX = 0;
        if (paddleX + 100 > 800) paddleX = 700;

        // ✅ 【稳定发送位置给主机】每帧都发，但用非可靠包，不卡
        ENetPacket* packet = enet_packet_create(&paddleX, sizeof(float), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(peer, 0, packet);

        // 绘制
        BeginDrawing();
        ClearBackground(BLACK);

        if (connected)
        {
            DrawCircle(state.ballX, state.ballY, 8, WHITE);
            DrawRectangle(state.paddle1X, 550, 100, 20, BLUE);
            DrawRectangle(paddleX, 50, 100, 20, GREEN);

            DrawText(TextFormat("Score: %d", state.score), 10, 10, 20, WHITE);
            DrawText(TextFormat("Life: %d", state.life), 700, 10, 20, WHITE);
        }
        else
        {
            DrawText("Connecting...", 280, 250, 30, RED);
        }

        EndDrawing();
    }

    enet_host_destroy(client);
    enet_deinitialize();
    CloseWindow();
    return 0;
}