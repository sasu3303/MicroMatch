#include "micromatch/engine.hpp"
#include <fstream>
#include <iostream>
int main(int argc, char** argv) {
    try {
        std::ifstream file;
        if (argc > 2) { std::cerr << "Usage: micromatch [commands.txt]\n"; return 2; }
        if (argc == 2) { file.open(argv[1]); if (!file) throw std::runtime_error("Cannot open command file"); }
        std::istream& input = argc == 2 ? file : std::cin;
        micromatch::Engine engine;
        for (std::string line; std::getline(input, line);) {
            if (line.empty() || line[0] == '#') continue;
            if (line == "QUIT") break;
            try { std::cout << micromatch::json(engine.submit(micromatch::parse(line)).get()) << '\n'; }
            catch (std::invalid_argument const& e) {
                micromatch::Response response; response.ok = false; response.code = e.what();
                std::cout << micromatch::json(response) << '\n';
            }
        }
    } catch (std::exception const& e) { std::cerr << "Fatal: " << e.what() << '\n'; return 1; }
}
