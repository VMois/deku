#include <fstream>
#include "src/Responder.h"

int main() {
    Responder r = Responder();
    std::string filename = "test.txt";

    // we can access the same file bacause Responder is processing a single job at the time
    // if we would increase the number of workers, 
    // we could have a conflict while accessing the same file
    r.on("save", [&] (const std::stringstream& input, std::stringstream& output) {
        std::ofstream file;
        file.open(filename, std::ios::app);
        if (file.is_open()) {
            file << input.str();
            file.close();
            output.write("OK", 2);
        } else {
            output.write("FAIL", 4);
        }
    });

    r.on("load", [&] (const std::stringstream& input, std::stringstream& output) {
        std::string line;
        std::ifstream file (filename);
        if (file.is_open()) {
            while (getline(file,line) ) {
                output << line;
            }
            file.close();
        }
    });

    r.start();
    return 0;
}