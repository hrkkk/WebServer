//
// Created by hrkkk on 2024/2/3.
//

#ifndef WEBSERVER_HTTPREQUEST_H
#define WEBSERVER_HTTPREQUEST_H

#include <string>
#include <vector>
#include <unordered_map>

class HttpRequest {
public:
    HttpRequest(): m_contentLength(0), m_cgi(false) {}

    // HTTP请求内容
    std::string m_requestLine;                      // 请求行
    std::unordered_map<std::string, std::string> m_requestHeader;       // 请求头
    std::string m_blankLine;                        // 空行
    std::string m_requestBody;                      // 请求正文

    // 存放解析结果
    std::string m_method;
    std::string m_uri;
    std::string m_version;
    int m_contentLength;
    std::string m_path;
    std::string m_queryParam;
    bool m_cgi;
};


#endif //WEBSERVER_HTTPREQUEST_H
