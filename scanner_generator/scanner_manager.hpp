#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>

class Manager {
public:
    using Spec = std::pair<std::string, std::string>;

    // função de leitura de arquivo
    std::vector<Spec> readSpecs(const std::string& path) {
        std::ifstream file(path);

        if (!file.is_open()) {
            throw std::runtime_error("Erro ao abrir arquivo: " + path);
        }

        std::vector<Spec> specs;
        std::string line;

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            std::istringstream iss(line);
            std::string token, regex;

            if (!(iss >> token >> regex)) {
                throw std::runtime_error("Linha inválida: " + line);
            }

            specs.emplace_back(token, regex);
        }

        return specs;
    }
};