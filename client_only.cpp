#include <raylib.h>
#include <enet/enet.h>
#include <iostream>
#include <cstring>

struct NetworkGameState {
    float ballX, ballY;
    float ballSpeedX, ballSpeedY;
    float paddle1X;
    float paddle2X;
    int score;
    int life;
};

float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

int main() {
    InitWindow(800, 600, "客户端 - 高响应丝滑版");
    SetTargetFPS(60);

    enet_initialize();
    ENetHost* client = enet_host_create(NULL, 1, 2, 0, 0);

    ENetAddress addr;
    enet_address_set_host(&addr, "127.0.0.1");
    addr.port = 1234;
    ENetPeer* peer = enet_host_connect(client, &addr, 2, 0);

    NetworkGameState state = {0};
    NetworkGameState prevState = {0};
    bool connected = false;
    ENetEvent event;

    float paddle2X = 350.0f;
    const float speed = 12.0f; // 客户端板子速度也调快一点

    if (enet_host_service(client, &event, 2000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        connected = true;
        std::cout << "✅ 连接主机成功！" << std::endl;
    }

    while (!WindowShouldClose()) {
        if (enet_host_service(client, &event, 1) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                prevState = state;
                memcpy(&state, event.packet->data, sizeof(NetworkGameState));
                enet_packet_destroy(event.packet);
            }
        }

        if (IsKeyDown(KEY_LEFT))  paddle2X -= speed;
        if (IsKeyDown(KEY_RIGHT)) paddle2X += speed;

        if (paddle2X < 0.0f) paddle2X = 0.0f;
        if (paddle2X > 700.0f) paddle2X = 700.0f;

        // 每帧都发送位置，不做间隔
        ENetPacket* pkt = enet_packet_create(&paddle2X, sizeof(float), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(peer, 0, pkt);

        BeginDrawing();
        ClearBackground(BLACK);

        if (connected) {
            const float t = 0.2f;
            float bX = Lerp(prevState.ballX, state.ballX, t);
            float bY = Lerp(prevState.ballY, state.ballY, t);
            float p1X = Lerp(prevState.paddle1X, state.paddle1X, t);

            if (bX < 10 || bY < 10) {
                bX = state.ballX;
                bY = state.ballY;
            }

            DrawCircle(bX, bY, 8, WHITE);
            DrawRectangle(p1X, 550, 100, 20, BLUE);
            DrawRectangle(paddle2X, 50, 100, 20, GREEN);

            DrawText(TextFormat("Score: %d", state.score), 10, 10, 20, WHITE);
            DrawText(TextFormat("Life: %d", state.life), 700, 10, 20, WHITE);
        } else {
            DrawText("Connecting...", 280, 250, 30, RED);
        }

        EndDrawing();
    }

    enet_host_destroy(client);
    enet_deinitialize();
    CloseWindow();
    return 0;
}