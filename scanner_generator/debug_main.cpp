#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "scanner_manager.hpp"
#include "state_minimizator.hpp"

using DFAState = std::set<State*>;

static std::string printableChar(char c) {
    const unsigned char uc = static_cast<unsigned char>(c);

    switch (c) {
        case '\n':
            return "\\\\n";
        case '\t':
            return "\\\\t";
        case '\r':
            return "\\\\r";
        case '\0':
            return "\\\\0";
        case '\\':
            return "\\\\\\\\";
        default:
            break;
    }

    if (std::isprint(uc)) {
        return std::string(1, c);
    }

    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(uc);
    return out.str();
}

static std::vector<int> stateIds(const DFAState& s) {
    std::vector<int> ids;
    ids.reserve(s.size());

    for (State* st : s) {
        ids.push_back(st->id);
    }

    std::sort(ids.begin(), ids.end());
    return ids;
}

static std::string stateSetToString(const DFAState& s) {
    auto ids = stateIds(s);
    std::ostringstream out;
    out << "{";

    for (size_t i = 0; i < ids.size(); ++i) {
        if (i) {
            out << ",";
        }
        out << ids[i];
    }

    out << "}";
    return out.str();
}

static std::unordered_map<std::string, int> buildDFAIndex(const AFD& dfa) {
    std::vector<const DFAState*> ordered;
    ordered.reserve(dfa.states.size());

    for (const auto& st : dfa.states) {
        ordered.push_back(&st);
    }

    std::sort(ordered.begin(), ordered.end(), [](const DFAState* a, const DFAState* b) {
        return stateIds(*a) < stateIds(*b);
    });

    std::unordered_map<std::string, int> index;
    for (size_t i = 0; i < ordered.size(); ++i) {
        index[stateSetToString(*ordered[i])] = static_cast<int>(i);
    }

    return index;
}

static void printDFA(const AFD& dfa, const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";

    auto index = buildDFAIndex(dfa);

    std::cout << "Start: D" << index[stateSetToString(dfa.start)] << " "
              << stateSetToString(dfa.start) << "\n";

    for (const auto& s : dfa.states) {
        const std::string key = stateSetToString(s);
        const int id = index[key];
        const auto& info = dfa.info.at(s);

        std::cout << "State D" << id << " " << key;
        if (info.is_final) {
            std::cout << " [final token=" << info.token
                      << " priority=" << info.priority << "]";
        }
        std::cout << "\n";
    }

    std::cout << "\nTransitions:\n";
    for (const auto& [line, target] : dfa.transitions) {
        const int fromId = index[stateSetToString(line.start)];
        const int toId = index[stateSetToString(target)];

        std::cout << "  D" << fromId << " --" << printableChar(line.input)
                  << "--> D" << toId << "\n";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <regex_spec_file>\n";
        return 1;
    }

    try {
        Manager manager;

        auto specs = manager.readSpecs(argv[1]);
        auto parsed = manager.parseRegex(specs);
        auto nfas = manager.buildNFAs(parsed);

        std::cout << "=== Individual NFAs ===\n";
        for (size_t i = 0; i < nfas.size(); ++i) {
            std::cout << "\n--- NFA #" << i << " token=" << specs[i].first << " ---\n";
            printAutomato(nfas[i]);
        }

        Automato global = manager.buildGlobalNFA(nfas);
        std::cout << "\n=== Global NFA ===\n";
        printAutomato(global);

        AFD dfa = subset_construction(global);
        printDFA(dfa, "DFA (subset construction)");

        AFD minimized = minimizeDFA(dfa);
        printDFA(minimized, "Minimized DFA");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}