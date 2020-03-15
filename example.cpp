#include "spdlog/spdlog.h"
#include "src/Responder.h"

int main() {
    spdlog::set_level(spdlog::level::debug);
    Responder r = Responder();

    r.on("echo", [] (std::stringstream a) { 
        return a; 
    });
    
    r.start();
    return 0;
}