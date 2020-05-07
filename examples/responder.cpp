#include "src/Responder.h"

int main() {
    Responder r = Responder();

    r.on("echo", [] (const std::stringstream& input, std::stringstream& output) {
        std::cout << "Echo: " << input.str() << std::endl;
        output.write(input.str().data(), input.str().size());
    });

    r.start();
    return 0;
}