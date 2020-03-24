#include "spdlog/spdlog.h"
#include "src/Responder.h"

int main() {
    spdlog::set_level(spdlog::level::debug);
    Responder r = Responder();

    r.on("echo", [] (const std::stringstream& input, std::stringstream& output) {
        output.write(input.str().data(), input.str().size());
    });

    r.start();
    return 0;
}