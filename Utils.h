//
// Created by hrkkk on 2024/2/3.
//

#ifndef WEBSERVER_UTILS_H
#define WEBSERVER_UTILS_H

#include <iostream>
#include <map>

#define INFO 1
#define WARNING 2
#define ERROR 3
#define FATAL 4
#define LOG(level, message) Log(#level, message)

void Log(const char* level, std::string message);

const std::map<std::string, std::string> mimeMapping = {
        {"css", "text/css"},
        {"csv", "text/csv"},
        {"txt", "text/plain"},
        {"vtt", "text/vtt"},
        {"html", "text/html"},
        {"htm", "text/html"},
        {"apng", "image/apng"},
        {"avif", "image/avif"},
        {"bmp", "image/bmp"},
        {"gif", "image/gif"},
        {"png", "image/png"},
        {"svg", "image/svg+xml"},
        {"webp", "image/webp"},
        {"ico", "image/x-icon"},
        {"tif", "image/tiff"},
        {"tiff", "image/tiff"},
        {"jpeg", "image/jpeg"},
        {"jpg", "image/jpg"},
        {"mp4", "video/mp4"},
        {"mpeg", "video/mpeg"},
        {"webm", "video/webm"},
        {"mp3", "audio/mp3"},
        {"mpga", "audio/mpeg"},
        {"weba", "audio/webm"},
        {"wav", "audio/wave"},
        {"otf", "font/otf"},
        {"ttf", "font/ttf"},
        {"woff", "font/woff"},
        {"woff2", "font/woff2"},
        {"7z", "application/x-7z-compressed"},
        {"atom", "application/atom+xml"},
        {"pdf", "application/pdf"},
        {"mjs", "application/javascript"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"rss", "application/rss+xml"},
        {"tar", "application/x-tar"},
        {"xhtml", "application/xhtml+xml"},
        {"xht", "application/xhtml+xml"},
        {"xslt", "application/xslt+xml"},
        {"xml", "application/xml"},
        {"gz", "application/gzip"},
        {"zip", "application/zip"},
        {"wasm", "application/wasm"}
};

const std::map<int, std::string> codeMapping = {
        {200, "OK"},
        {201, "CREATED"},
        {204, "NO_CONTENT"},
        {400, "BAD_REQUEST"},
        {403, "FORBIDDEN"},
        {404, "NOT_FOUND"},
        {500, "INTERNAL_SERVER_ERROR"},
};

class Utils {
public:
    static std::string codeToDesc(int code) {
        auto iter = codeMapping.find(code);
        if (iter != codeMapping.end()) {
            return (*iter).second;
        }
        return "";
    }

    static std::string extensionToMime(const std::string& extension) {
        auto iter = mimeMapping.find(extension);
        if (iter != mimeMapping.end()) {
            return (*iter).second;
        }
        return "";
    }
};

class StringUtils {
public:
    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");     // 查找第一个非空格字符的位置
        size_t end = str.find_last_not_of(" \t\r\n");        // 查找最后一个非空格字符的位置

        // 如果字符串全为空格，则返回空字符串
        if (start == std::string::npos || end == std::string::npos) {
            return "";
        }

        // 返回去除两端空格后的子字符串
        return str.substr(start, end - start + 1);
    }
};


#endif //WEBSERVER_UTILS_H
