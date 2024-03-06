//
// Created by hrkkk on 2024/2/3.
//

#ifndef WEBSERVER_TCPSERVER_H
#define WEBSERVER_TCPSERVER_H


class TcpServer {
public:
    explicit TcpServer(int port);
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    ~TcpServer();

    static TcpServer* getInstance(int port);
    void initServer();
    void createSocket();
    void bindSocket();
    void listenSocket();
    int getSocket();

private:
    static TcpServer* m_instance;
    int m_port;
    int m_listenSocket;
    int m_backlog;
};


#endif //WEBSERVER_TCPSERVER_H
