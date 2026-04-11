#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include "subset_construction.hpp"

// Lê arquivo de regexes com ids ordenados por prioridade
// cria uma lista ordenada das regexes
// cria um afn para cada uma das regexes
// une os afns com transições eps
// transforma em afd
// minimiza (A FAZER)

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

            // ignora linhas que só possuem espaços em branco
            bool only_spaces = true;
            for (char c : line) {
                if (!std::isspace(static_cast<unsigned char>(c))) {
                    only_spaces = false;
                    break;
                }
            }
            if (only_spaces) continue;

            std::istringstream iss(line);
            std::string token, regex;

            if (!(iss >> token)) {
                throw std::runtime_error("Linha inválida: " + line);
            }

            std::getline(iss, regex);

            // remove espaço inicial da regex restante na linha
            size_t begin = 0;
            while (begin < regex.size() && std::isspace(static_cast<unsigned char>(regex[begin]))) {
                begin++;
            }
            regex = regex.substr(begin);

            if (regex.empty()) {
                throw std::runtime_error("Regex ausente para token: " + token);
            }

            specs.emplace_back(token, regex);
        }

        return specs;
    }

    std::vector<std::pair<std::string, std::unique_ptr<RegexNode>>> parseRegex(const std::vector<Spec>& specs)
    {
        std::vector<std::pair<std::string, std::unique_ptr<RegexNode>>> result;

        for (const auto& [token, regex] : specs) {
            RegexEngine parser(regex);
            auto ast = parser.parse();

            result.emplace_back(token, std::move(ast));
        }

        return result;
    }

    std::vector<Automato> buildNFAs(const std::vector<std::pair<std::string, std::unique_ptr<RegexNode>>>& parsed)
    {
        std::vector<Automato> nfas;

        int priority = 0;

        for (const auto& [token, ast] : parsed) {
            Automato nfa = buildNFA(ast.get());

            markFinal(nfa.accept, token, priority++);

            nfas.push_back(nfa);
        }

        return nfas;
    }

    Automato buildGlobalNFA(const std::vector<Automato>& automatos) {
        auto global_start = StateFactory::newState();

        for (const auto& a : automatos) {
            global_start->epsilon_transitions.insert(a.start);
        }

        return Automato{global_start, nullptr};
    }

    AFD buildDFA(const std::string& path) {
        auto specs = readSpecs(path);
        auto parsed = parseRegex(specs);
        auto nfas = buildNFAs(parsed);
        auto global = buildGlobalNFA(nfas);

        return subset_construction(global);
    }
};