cmake_minimum_required(VERSION 3.22)
project(WebServer)

set(CMAKE_CXX_STANDARD 20)

add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

add_executable(WebServer main.cpp TcpServer.cpp TcpServer.h HttpServer.cpp HttpServer.h Utils.cpp Utils.h HttpRequest.h HttpResponse.h RequestProcessing.cpp RequestProcessing.h ThreadPool.cpp ThreadPool.h)
