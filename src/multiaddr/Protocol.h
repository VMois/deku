#include <string>

class Protocol {
    public:
        int code, size;
        std::string name;
        Protocol(int ncode, int nsize, std::string nname): 
            code(ncode), size(nsize), name(nname) {};
};