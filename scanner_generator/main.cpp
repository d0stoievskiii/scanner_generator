#include <iostream>
#include <stdexcept>

#include "scanner_manager.hpp"
#include "state_minimizator.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <regex_spec_file>\n";
        return 1;
    }

    try {
        Manager manager;

        AFD dfa = manager.buildDFA(argv[1]);
        AFD minimized = minimizeDFA(dfa);

        std::cout << "DFA states: " << dfa.states.size() << "\n";
        std::cout << "Minimized DFA states: " << minimized.states.size() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}