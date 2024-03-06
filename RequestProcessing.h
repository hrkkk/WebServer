//
// Created by hrkkk on 2024/2/3.
//

#ifndef WEBSERVER_REQUESTPROCESSING_H
#define WEBSERVER_REQUESTPROCESSING_H

#include "HttpRequest.h"
#include "HttpResponse.h"

class RequestProcessing {
public:
    RequestProcessing(int sock);

    bool processRequest();

private:
    // 读取请求
    void recvHttpRequest();
    // 处理请求
    void handleHttpRequest();
    // 构建响应
    void buildHttpResponse();
    // 发送响应
    bool sendHttpResponse();

    int acquireRequestFile();

private:
    HttpResponse m_httpResponse;
    HttpRequest m_httpRequest;
    int m_socket;
    bool m_stop;
};


#endif //WEBSERVER_REQUESTPROCESSING_H
