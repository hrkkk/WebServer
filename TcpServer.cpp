//
// Created by hrkkk on 2024/2/3.
//

#include "TcpServer.h"
#include "Utils.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

TcpServer* TcpServer::m_instance = nullptr;

TcpServer::TcpServer(int port) : m_listenSocket(-1), m_port(port), m_backlog(5) {}

TcpServer::~TcpServer()
{
    if (m_listenSocket >= 0) {
        close(m_listenSocket);
    }
}

TcpServer* TcpServer::getInstance(int port)
{
    if (m_instance == nullptr) {
        m_instance = new TcpServer(port);
        m_instance->initServer();
    }
    return m_instance;
}

void TcpServer::initServer()
{
    createSocket();
    bindSocket();
    listenSocket();
    LOG(INFO, "Init Tcp Server Success!");
}

void TcpServer::createSocket()
{
    // 创建监听套接字
    // AF_INET: 使用IPv4地址族
    // SOCK_STREAM: 使用TCP传输协议
    m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenSocket < 0) {
        LOG(FATAL, "Create socket error!");
        exit(1);
    }

    // 设置允许在同一端口上快速重用socket
    int opt = 1;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    LOG(INFO, "Create socket success!");
}

void TcpServer::bindSocket()
{
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(m_port);
    local.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_listenSocket, (struct sockaddr*)&local, sizeof(local))) {
        LOG(FATAL, "Bind error");
        close(m_listenSocket);
        exit(2);
    }
    LOG(INFO, "Port bind socket success");
}

void TcpServer::listenSocket()
{
    if (listen(m_listenSocket, m_backlog) < 0) {
        LOG(FATAL, "Listen error");
        close(m_listenSocket);
        exit(3);
    }
    LOG(INFO, "Listen socket success");
}

int TcpServer::getSocket()
{
    return m_listenSocket;
}