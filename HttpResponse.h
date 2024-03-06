//
// Created by hrkkk on 2024/2/3.
//

#ifndef WEBSERVER_HTTPRESPONSE_H
#define WEBSERVER_HTTPRESPONSE_H

#include <vector>
#include <string>
#include <unordered_map>

#define OK 200
#define CREATED 201
#define NO_CONTENT 204
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define FORBIDDEN 403
#define INTERNAL_SERVER_ERROR 500
#define OPTIONS 1000

#define WEB_ROOT "/tmp/WebServer/resource"

#define CRLF "\r\n"
#define HTTP_VERSION "HTTP/1.0"

class HttpResponse {
public:
    HttpResponse(): m_statusCode(OK), m_blankLine(CRLF), m_fd(-1), m_size(0) { }
    // HTTP请求内容
    std::string m_statusLine;                       // 响应行
    std::vector<std::string> m_responseHeader;       // 响应头
    std::string m_blankLine;                        // 空行
    std::string m_responseBody;                      // 响应正文

    std::unordered_map<std::string, std::string> m_headerMap;
    int m_statusCode;
    int m_fd;
    int m_size;
    std::string m_suffix;
};


#endif //WEBSERVER_HTTPRESPONSE_H
