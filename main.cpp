#include "HttpServer.h"

int main() {
    HttpServer httpServer(3000);
    httpServer.loop();

    return 0;
}
