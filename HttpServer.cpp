#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "HttpServer.h"
#include "Utils.h"
#include "RequestProcessing.h"

HttpServer::HttpServer(int port) : m_port(port) {}

HttpServer::~HttpServer() {}

void HttpServer::loop()
{
    TcpServer* tcpServer = TcpServer::getInstance(m_port);
    int listenSocket = tcpServer->getSocket();

    LOG(INFO, "Loop begin\r\n");

    // 创建epoll实例
    m_epollFd = epoll_create(256);
    if (m_epollFd == -1) {
        close(listenSocket);
        return;
    }

    epoll_event ev;
    ev.data.fd = listenSocket;
    ev.events = EPOLLIN | EPOLLET;

    epoll_ctl(m_epollFd, EPOLL_CTL_ADD, listenSocket, &ev);

    while (true) {
        // 等待epoll事件发生
        int number = epoll_wait(m_epollFd, m_events, MAX_EVENT_NUMBER, -1);

        // 处理所发生的所有事件
        for (int i = 0; i < number; ++i) {
            if (m_events[i].data.fd == listenSocket) {   // 如果监听到了一个新的socket连接请求
                sockaddr_in peer;
                memset(&peer, 0, sizeof(peer));
                socklen_t len = sizeof(peer);
                int peerSocket = accept(listenSocket, (sockaddr*)&peer, &len);
                if (peerSocket < 0) {
                    continue;
                }

                std::string clientIP = inet_ntoa(peer.sin_addr);
                int clientPort = ntohs(peer.sin_port);
                LOG(INFO, "Get a new link:[" + clientIP + ":" + std::to_string(clientPort) + "]");

                // 将该socket添加到epoll中
                ev.data.fd = peerSocket;
                ev.events = EPOLLIN | EPOLLET;
                // 注册event
                epoll_ctl(m_epollFd, EPOLL_CTL_ADD, peerSocket, &ev);
            } else if (m_events[i].events & EPOLLIN) {   // 如果是已连接的用户，并且收到数据，则进行读取
                int socket = m_events[i].data.fd;
                // 读取数据

                // 读完数据
                ev.data.fd = socket;
                ev.events = EPOLLOUT | EPOLLET;

                epoll_ctl(m_epollFd, EPOLL_CTL_MOD, socket, &ev);
            } else if (m_events[i].events & EPOLLOUT) {      // 如果有数据要发送
                int socket = m_events[i].data.fd;
                // 写入数据

                // 写完数据
                ev.data.fd = socket;
                ev.events = EPOLLIN | EPOLLET;

                epoll_ctl(m_epollFd, EPOLL_CTL_MOD, socket, &ev);
            }
        }
//        // 开启子线程，处理任务
//        RequestProcessing requestProcessing(sock);
//        if (requestProcessing.processRequest()) {
//            LOG(WARNING, "Request process failed");
//        }
//
//        close(sock);
//        LOG(INFO, "Link closed:[" + clientIP + ":" + std::to_string(clientPort) + "]\n");
    }

    close(listenSocket);
    close(m_epollFd);
}
