#include "HttpServer.h"

int main() {
    // new-branch test
    HttpServer httpServer(3000);
    httpServer.loop();

    return 0;
}
