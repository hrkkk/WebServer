#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "TcpServer.h"
#include "sys/epoll.h"

constexpr int MAX_EVENT_NUMBER = 10000;     // 最大事件数

class HttpServer {
public:
    explicit HttpServer(int port);
    ~HttpServer();

    void loop();

private:
    int m_port;
    int m_epollFd;
    epoll_event m_events[MAX_EVENT_NUMBER];
};


#endif