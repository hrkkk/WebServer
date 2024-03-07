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
#include "ThreadPool.h"

HttpServer::HttpServer(int port) : m_port(port) {}

HttpServer::~HttpServer() {}

void HttpServer::loop()
{
    TcpServer* tcpServer = TcpServer::getInstance(m_port);
    int listenSocket = tcpServer->getSocket();

    // 创建epoll实例
    m_epollFd = epoll_create(256);
    if (m_epollFd == -1) {
        close(listenSocket);
        return;
    }

    // 将服务器socket添加到epoll监听中
    epoll_event ev;
    ev.data.fd = listenSocket;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(m_epollFd, EPOLL_CTL_ADD, listenSocket, &ev);

    // 创建线程池
    ThreadPool threadPool(4);

    LOG(INFO, "Loop begin\r\n");

    // epoll事件循环
    while (true) {
        // 等待epoll事件发生
        int number = epoll_wait(m_epollFd, m_events, MAX_EVENT_NUMBER, -1);

        // 处理所发生的所有事件
        for (int i = 0; i < number; ++i) {
            if (m_events[i].data.fd == listenSocket) {   // 如果监听到了一个新的socket连接请求
                sockaddr_in peer;
                memset(&peer, 0, sizeof(peer));
                socklen_t len = sizeof(peer);
                int clientSocket = accept(listenSocket, (sockaddr*)&peer, &len);
                if (clientSocket < 0) {
                    continue;
                }

                std::string clientIP = inet_ntoa(peer.sin_addr);
                int clientPort = ntohs(peer.sin_port);
                LOG(INFO, "Get a new link:[" + clientIP + ":" + std::to_string(clientPort) + "]");

                // 将该客户端socket添加到epoll监听中
                ev.data.fd = clientSocket;
                ev.events = EPOLLIN | EPOLLET;
                // 注册event
                epoll_ctl(m_epollFd, EPOLL_CTL_ADD, clientSocket, &ev);
            } else if (m_events[i].events & EPOLLIN) {   // 如果是已连接的用户，并且收到数据，则进行读取
                int clientSocket = m_events[i].data.fd;
                // 读取数据
                // 将读写任务添加到任务队列中
                threadPool.addTask([clientSocket]() {
                    RequestProcessing requestProcessing(clientSocket);
                    requestProcessing.processRequest();
                    close(clientSocket);
                    LOG(INFO, "Link has disconnected\r\n");
                });
            } else if (m_events[i].events & EPOLLOUT) {      // 如果有数据要发送
                // 数据发送操作封装在任务中，不使用EPOLLOUT事件来触发
            }
        }
    }

    close(listenSocket);
    close(m_epollFd);
}
