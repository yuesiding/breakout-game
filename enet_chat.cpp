#include <enet/enet.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// 服务器端逻辑：监听连接 + 接收消息
void run_server() {
    ENetAddress address;
    enet_address_set_host(&address, "0.0.0.0");
    address.port = 1234;
    ENetHost* server = enet_host_create(&address, 1, 2, 0, 0);

    if (!server) {
        std::cerr << "❌ 服务器创建失败！" << std::endl;
        return;
    }
    std::cout << "✅ 服务器已启动，等待客户端连接（端口1234）..." << std::endl;

    while (true) {
        ENetEvent event;
        while (enet_host_service(server, &event, 1000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "🎉 客户端已连接！" << std::endl;
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    std::cout << "📥 收到客户端消息：" << (char*)event.packet->data << std::endl;
                    // 删掉了原来的回复代码，就不会有变量'reply'的警告了
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "👋 客户端已断开连接" << std::endl;
                    break;
                default:
                    break;
            }
        }
    }

    enet_host_destroy(server);
}

// 客户端逻辑：连接服务器 + 发送消息
void run_client() {
    // 1. 创建ENet客户端主机
    ENetHost* client = enet_host_create(
        NULL,       // 无监听地址（客户端不需要）
        1,          // 最大连接数
        2,          // 通道数
        0,          // 上行带宽
        0           // 下行带宽
    );

    if (!client) {
        std::cerr << "❌ 客户端创建失败！" << std::endl;
        return;
    }

    // 2. 连接服务器（本地测试用127.0.0.1，局域网填服务器IP）
    ENetAddress server_addr;
    enet_address_set_host(&server_addr, "127.0.0.1"); // 服务器IP
    server_addr.port = 1234;                          // 和服务器端口一致
    ENetPeer* peer = enet_host_connect(client, &server_addr, 2, 0);

    if (!peer) {
        std::cerr << "❌ 连接服务器失败！" << std::endl;
        enet_host_destroy(client);
        return;
    }

    // 3. 等待连接结果（5000ms超时）
    ENetEvent event;
    if (enet_host_service(client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        std::cout << "✅ 连接服务器成功！" << std::endl;
    } else {
        std::cerr << "❌ 连接服务器超时！" << std::endl;
        enet_peer_reset(peer);
        enet_host_destroy(client);
        return;
    }

    // 4. 循环发送消息给服务器
    while (true) {
        std::string msg;
        std::cout << "请输入要发送的消息（输入quit退出）：";
        std::cin >> msg;

        if (msg == "quit") {
            enet_peer_disconnect(peer, 0); // 主动断开连接
            break;
        }

        // 创建可靠数据包（确保消息必达）
        ENetPacket* packet = enet_packet_create(
            msg.c_str(), msg.size()+1, ENET_PACKET_FLAG_RELIABLE
        );
        enet_peer_send(peer, 0, packet); // 发送给服务器

        // 检测服务器回复
        while (enet_host_service(client, &event, 100) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                std::cout << "📥 服务器回复：" << (char*)event.packet->data << std::endl;
                enet_packet_destroy(event.packet);
            }
        }
    }

    // 5. 清理资源
    enet_host_destroy(client);
}

int main(int argc, char* argv[]) {
    // 初始化ENet库（必须！）
    if (enet_initialize() != 0) {
        std::cerr << "❌ ENet库初始化失败！" << std::endl;
        return 1;
    }

    // 传参数区分服务器/客户端：./enet_chat server 是服务器，否则是客户端
    if (argc == 2 && std::string(argv[1]) == "server") {
        run_server();
    } else {
        run_client();
    }

    // 反初始化ENet库
    enet_deinitialize();
    return 0;
}