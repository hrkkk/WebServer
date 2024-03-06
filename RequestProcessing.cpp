//
// Created by hrkkk on 2024/2/3.
//

#include "RequestProcessing.h"
#include "Utils.h"
#include "json.hpp"

#include <fstream>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <regex>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <algorithm>
#include <queue>

using json = nlohmann::json;

RequestProcessing::RequestProcessing(int sock) : m_socket(sock), m_stop(false) {}

bool RequestProcessing::processRequest()
{
    recvHttpRequest();
    handleHttpRequest();
    buildHttpResponse();
    return sendHttpResponse();
}

#define BUFFER_SIZE 256
void RequestProcessing::recvHttpRequest() {
    LOG(INFO, "Start processing");
    // 接收一整个HTTP报文
    std::string httpMsg;
    char buffer[BUFFER_SIZE];

    while (true) {
        // 从socket缓冲区中接收256个字节到用户缓冲区
        ssize_t recvSize = recv(m_socket, buffer, BUFFER_SIZE, 0);

        if (recvSize > 0) {
            std::string receivedData(buffer, recvSize);
            httpMsg += receivedData;

            size_t pos = httpMsg.find("Content-Length:");
            int contentLength = 0;
            if (pos != std::string::npos) {
                contentLength  = stoi(httpMsg.substr(pos + 15));
            }

            // 如果没有Content-Length，代表没有正文，则读取到\r\n\r\n就结束
            if (contentLength == 0) {
                size_t pos = httpMsg.find("\r\n\r\n");
                if (pos != std::string::npos) {
                    break;
                }
            } else {    // 如果有Content-Length，代表有正文，则读取到指定字节数的正文后就结束
                size_t pos = httpMsg.find("\r\n\r\n");
                if (pos != std::string::npos) {
                    std::string bodyContent = httpMsg.substr(pos + 4);
                    if (bodyContent.size() == contentLength) {
                        break;
                    }
                }
            }

            /* TODO ------------- */
            // 数据接收部分存在BUG，如果客户端迟迟不发送\r\n\r\n，或发送了\r\n\r\n却没有接收到正文，此处将无法退出循环
            // 应当设置一个定时器，如果超过定时上限还未收到特定的数据，则认为请求非法，退出循环，断开当前连接
        } else if (recvSize == 0) {
            LOG(WARNING, "Connection closed by client");
            return;
        } else {
            LOG(ERROR, "Receive error");
            return;
        }
    }

    // 从报文中解析出请求行
    std::istringstream requestStream(httpMsg, std::ios::binary);
    requestStream >> m_httpRequest.m_method >> m_httpRequest.m_uri >> m_httpRequest.m_version;

    // 解析头部信息
    std::string headerLine;
    while (std::getline(requestStream, headerLine)) {
        size_t pos = headerLine.find('\r');
        if (pos != std::string::npos) {
            headerLine = headerLine.substr(0, pos);
        }
        pos = headerLine.find(':');
        if (pos != std::string::npos) {
            std::string key = headerLine.substr(0, pos);
            std::string value = headerLine.substr(pos + 1);
            m_httpRequest.m_requestHeader.insert({key, value});
        }
    }

    // 解析正文
    std::getline(requestStream, m_httpRequest.m_requestBody);
    size_t pos = requestStream.str().find("\r\n\r\n");
    if (pos != std::string::npos) {
        m_httpRequest.m_requestBody = requestStream.str().substr(pos + 4);
    }
}

void RequestProcessing::handleHttpRequest() {
    int& code = m_httpResponse.m_statusCode;

    // 目前仅支持GET请求和POST请求
    if (m_httpRequest.m_method == "OPTIONS") {
        code = OPTIONS;
        return;
    }

    if (m_httpRequest.m_method != "GET" && m_httpRequest.m_method != "POST") {
        LOG(WARNING, "method is not GET or POST");
        code = BAD_REQUEST;
        return;
    }

    if (m_httpRequest.m_method == "GET") {
        size_t pos = m_httpRequest.m_uri.find('?');
        if (pos != std::string::npos) {     // uri携带参数
            // 切割uri，得到请求路径和参数

        } else {     // uri不携带参数
            if (m_httpRequest.m_uri == "/") {
                m_httpRequest.m_path = "/index.html";
            } else {
                m_httpRequest.m_path = m_httpRequest.m_uri;
            }
            m_httpRequest.m_cgi = false;
        }
    } else if (m_httpRequest.m_method == "POST") {
        m_httpRequest.m_path = m_httpRequest.m_uri;
        m_httpRequest.m_cgi = true;
    }

    auto hasKey = [&](const std::string& keyName) -> std::string {
        auto iter = m_httpRequest.m_requestHeader.find(keyName);
        if (iter != m_httpRequest.m_requestHeader.end()) {
            return StringUtils::trim((*iter).second);
        }
        return "";
    };

    // 进行CGI或非CGI处理
    // CGI为true的情况：
    // 1. GET方法的URI带参数
    // 2. 使用POST方法
    // 3. 请求的资源是可执行程序
    if (m_httpRequest.m_cgi) {
        if (m_httpRequest.m_path == "/upload") {
            static int fileLength = 0;
            static std::string title;
            static std::string author;
            static std::string date;
            static std::string type;
            static std::map<int, std::string*> chunks;     // map默认按照key的大小升序排序
            //　处理第一包数据
            if (hasKey("Content-Type") == "application/json") {
                // 解析JSON，记录信息
                if (!m_httpRequest.m_requestBody.empty()) {
                    auto jsonObj = json::parse(m_httpRequest.m_requestBody);
                    fileLength = jsonObj["TotalSize"];
                    title = jsonObj["Title"];
                    author = jsonObj["Author"];
                    date = jsonObj["Date"];
                    type = jsonObj["Type"];
                    chunks.clear();
                    code = NO_CONTENT;
                    LOG(INFO, "首包接收完成");
                }
                return;
            }

            // 接收正文
            std::string range = hasKey("Chunk");
            if (range != "") {
                int index = stoi(range);
                auto chunk  = new std::string(m_httpRequest.m_requestBody);
                chunks.insert({ index, chunk });
                std::cout << "Chunk index: " << index << std::endl;
                std::cout << "Chunk num: " << chunks.size() << std::endl;
                code = NO_CONTENT;
                LOG(INFO, std::string("第" + range + "包数据块接收完成"));
                return;
            }

            // 接收完成请求
            if (m_httpRequest.m_requestBody == "Upload Finished") {
                // 组合数据包
                auto totalFile = new std::string;
                std::cout << "Chunk num: " << chunks.size() << std::endl;
                while (!chunks.empty()) {
                    auto iter = chunks.begin();
                    totalFile->append(*(*iter).second);
                    delete (*iter).second;
                    chunks.erase(iter);
                }

                // 判断合并后的数据长度是否等于上传的文件长度
                std::cout << "TotalFile: " << totalFile->size() << std::endl;
                std::cout << "FileLength: " << fileLength << std::endl;
                if (totalFile->size() != fileLength) {
                    delete totalFile;
                    code = FORBIDDEN;
                    return;
                }

                // 将临时缓冲区中的内容全部写入文件
                std::string filePath = std::string(WEB_ROOT) + "/" + title;
                std::ofstream file(filePath);

                if (file.is_open()) {
                    file << *totalFile;
                    file.close();
                    code = CREATED;
                    LOG(INFO, "Upload success");
                }
                delete totalFile;
                return;
            }

            code = FORBIDDEN;
        }
    } else {    // 简单的文件请求，返回静态资源
        // 为请求资源拼接web根目录
        m_httpRequest.m_path = WEB_ROOT + m_httpRequest.m_path;
        // 获取文件后缀并转小写
        size_t lastDotPos = m_httpRequest.m_path.rfind('.');
        if (lastDotPos != std::string::npos) {
            std::string suffix = m_httpRequest.m_path.substr(lastDotPos + 1);
            std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
            m_httpResponse.m_suffix = suffix;
        }

        LOG(INFO, "Require file: " + m_httpRequest.m_path);

        // 获取请求资源文件的属性信息
        struct stat fileStat;
        if (stat(m_httpRequest.m_path.c_str(), &fileStat) == 0) {     // 属性信息获取成功，说明资源存在
            m_httpResponse.m_size = fileStat.st_size;
            LOG(INFO, "Resource is existed");
        } else {    // 属性信息获取失败，认为资源不存在
            LOG(WARNING, "Resource not found");
            code = NOT_FOUND;
            return;
        }

        code = acquireRequestFile();
    }
}

void RequestProcessing::buildHttpResponse() {
    int& code = m_httpResponse.m_statusCode;

    // 构建响应报头
    switch (code) {
        case OK:
            if (!m_httpResponse.m_suffix.empty()) {
                m_httpResponse.m_headerMap.insert({"Content-Type", Utils::extensionToMime(m_httpResponse.m_suffix)});
            }
            break;
        case OPTIONS:
            code = OK;
            m_httpResponse.m_headerMap.insert({"Access-Control-Allow-Origin", "*"});
            m_httpResponse.m_headerMap.insert({"Access-Control-Allow-Method", "GET, POST, OPTIONS"});
            m_httpResponse.m_headerMap.insert({"Access-Control-Allow-Headers", "Content-Type"});
            break;
        default:
            break;
    }

    // 构建状态行
    std::string& statusLine = m_httpResponse.m_statusLine;
    statusLine += HTTP_VERSION;
    statusLine += " ";
    statusLine += std::to_string(code);
    statusLine += " ";
    statusLine += Utils::codeToDesc(code);
    statusLine += CRLF;

    for (auto& iter : m_httpResponse.m_headerMap) {
        m_httpResponse.m_responseHeader.push_back(iter.first + ": " + iter.second + CRLF);
    }
}

bool RequestProcessing::sendHttpResponse() {
    // 发送状态行
    if (send(m_socket, m_httpResponse.m_statusLine.c_str(), m_httpResponse.m_statusLine.size(), 0) <= 0) {
        m_stop = true;
    }

    // 发送响应报头
    if (!m_stop) {
        for (auto& iter : m_httpResponse.m_responseHeader) {
            if (send(m_socket, iter.c_str(), iter.size(), 0) <= 0) {
                m_stop = true;
                break;
            }
        }
    }

    // 发送空行
    if (!m_stop) {
        if (send(m_socket, m_httpResponse.m_blankLine.c_str(), m_httpResponse.m_blankLine.size(), 0) <= 0) {
            m_stop = true;
        }
    }

    // 发送响应正文
    if (!m_stop) {
        if (m_httpRequest.m_cgi) {

        } else {
            if (m_httpResponse.m_statusCode == OK) {
                // sendfile 函数用于直接在两个文件描述符之间传递数据，避免了数据在磁盘、CPU、Socket缓冲区之间的多次复制。
                // out_fd 必须是一个 socket，in_fd 必须是一个支持类似 mmap 函数的文件描述，不能是 socket，可见 sendfile 函数是专门为在网络上传输文件而设计的
                if (sendfile(m_socket, m_httpResponse.m_fd, nullptr, m_httpResponse.m_size) <= 0) {
                    m_stop = true;
                    LOG(WARNING, "Send file failed");
                }
                close(m_httpResponse.m_fd);
            }
        }
    }

    return m_stop;
}

int RequestProcessing::acquireRequestFile()
{
    // 打开客户端请求的资源文件，以供后续发送
    m_httpResponse.m_fd = open(m_httpRequest.m_path.c_str(), O_RDONLY);
    if (m_httpResponse.m_fd >= 0) {
        LOG(INFO, "Open resource file success");
        return OK;
    }
    LOG(ERROR, "Open resource file failed");
    return INTERNAL_SERVER_ERROR;
}