#include <iostream>
#include <stdexcept>
#include <string>

#include "afd_serializer.hpp"
#include "scanner_manager.hpp"
#include "state_minimizator.hpp"
#include "generate_scanner_code.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <regex_spec_file> [output_afd_json]" << std::endl;
        std::cerr << "       or: " << argv[0] << " generate-scanner <regex_spec_file> <output_code_dir>" << std::endl;
        return 1;
    }

    try {
        if (std::string(argv[1]) == "generate-scanner") {
            if (argc < 4) {
                std::cerr << "Usage: " << argv[0] << " generate-scanner <regex_spec_file> <output_code_dir>" << std::endl;
                return 1;
            }
            const std::string specPath = argv[2];
            const std::string outDir = argv[3];

            Manager manager;
            AFD dfa = manager.buildDFA(specPath);
            AFD minimized = minimizeDFA(dfa);


            generate_scanner_code(minimized, outDir);

            std::cout << "Scanner code generated in: " << outDir << std::endl;
            return 0;
        }

        Manager manager;
        AFD dfa = manager.buildDFA(argv[1]);
        AFD minimized = minimizeDFA(dfa);

        const std::string outputPath = (argc >= 3) ? argv[2] : "afd.json";

        writeSerializedAfd(outputPath, argv[1], dfa, minimized);

        std::cout << "DFA states: " << dfa.states.size() << "\n";
        std::cout << "Minimized DFA states: " << minimized.states.size() << "\n";
        std::cout << "Serialized DFA JSON: " << outputPath << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}