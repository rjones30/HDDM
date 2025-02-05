#include <httpstream.hpp>

int main() {
    httpIstream httpStream("http://nod25.phys.uconn.edu:2880/Gluex/simulation"
                           "/simsamples/particle_gun-v5.2.0/particle_gun001_001.hddm");
    std::string line;
    while (std::getline(httpStream, line)) {
        std::cout << line << std::endl;
    }
    return 0;
}

#include <httpstream.cpp>
