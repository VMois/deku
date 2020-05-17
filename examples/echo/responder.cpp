#include "src/Responder.h"

int main() {
    Responder r = Responder();

    r.on("echo", [] (const std::stringstream& input, std::stringstream& output) {
        zclock_sleep(100);
        output.write(input.str().data(), input.str().size());
    });

    r.on("exception", [] (const std::stringstream& input, std::stringstream& output) {
        zclock_sleep(100);
        throw std::logic_error("something wrong in your code. Run fool...");
    });

    r.start();
    return 0;
}