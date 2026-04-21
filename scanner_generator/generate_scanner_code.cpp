#include "generate_scanner_code.hpp"
#include <algorithm>
#include <optional>
#include <fstream>
#include <filesystem>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace fs = std::filesystem;
using DFAState = std::set<State*>;

static std::string state_key(const DFAState& state) {
    std::vector<int> ids;
    ids.reserve(state.size());

    for (auto* st : state) {
        ids.push_back(st->id);
    }

    std::sort(ids.begin(), ids.end());

    std::ostringstream oss;
    for (size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) {
            oss << ",";
        }
        oss << ids[i];
    }
    return oss.str();
}

static std::string escape_cpp_string(const std::string& value) {
    std::ostringstream out;
    for (char c : value) {
        switch (c) {
            case '\\':
                out << "\\\\";
                break;
            case '"':
                out << "\\\"";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            default:
                out << c;
                break;
        }
    }
    return out.str();
}

// Utilitário: retorna lista ordenada dos estados
static std::vector<const DFAState*> ordered_states(const AFD& afd) {
    std::vector<const DFAState*> states;
    for (const auto& s : afd.states) states.push_back(&s);
    std::sort(states.begin(), states.end(), [](const DFAState* a, const DFAState* b) {
        return state_key(*a) < state_key(*b);
    });
    return states;
}

// Utilitário: retorna id do estado na lista ordenada
static std::map<std::string, int> state_id_map(const std::vector<const DFAState*>& states) {
    std::map<std::string, int> m;
    for (size_t i = 0; i < states.size(); ++i) {
        m[state_key(*states[i])] = static_cast<int>(i);
    }
    return m;
}

void generate_scanner_code(const AFD& afd, const std::string& outDir) {
    fs::create_directories(outDir);
    auto states = ordered_states(afd);
    auto stateId = state_id_map(states);

    const auto startIt = stateId.find(state_key(afd.start));
    if (startIt == stateId.end()) {
        throw std::runtime_error("Start state was not found in generated state map.");
    }
    const int startStateId = startIt->second;

    // Tabela de transição: [estado][byte] = prox_estado
    std::vector<std::vector<int>> trans_table(states.size(), std::vector<int>(256, -1));
    for (const auto& [line, target] : afd.transitions) {
        const int from = stateId.at(state_key(line.start));
        const int to = stateId.at(state_key(target));
        unsigned char c = static_cast<unsigned char>(line.input);
        trans_table[from][c] = to;
    }

    // Tabela de aceitação: [estado] = {token, prioridade} ou -1
    struct AcceptInfo { std::string token; int priority; };
    std::vector<std::optional<AcceptInfo>> accept_table(states.size());
    for (size_t i = 0; i < states.size(); ++i) {
        const auto& info = afd.info.at(*states[i]);
        if (info.is_final) {
            accept_table[i] = AcceptInfo{info.token, info.priority};
        }
    }

    // scanner_gerado.hpp
    std::ofstream hpp(outDir + "/scanner_gerado.hpp");
    if (!hpp.is_open()) {
        throw std::runtime_error("Could not open scanner_gerado.hpp for writing");
    }
    hpp << "#pragma once\n#include <string>\n#include <vector>\n#include <optional>\n";
    hpp << "struct TokenMatch { std::string token; int start; int end; int line; int column;};\n";
    hpp << "std::vector<TokenMatch> scan(const std::string& input);\n";
    hpp.close();

    // scanner_gerado.cpp
    std::ofstream cpp(outDir + "/scanner_gerado.cpp");
    if (!cpp.is_open()) {
        throw std::runtime_error("Could not open scanner_gerado.cpp for writing");
    }
    cpp << "#include \"scanner_gerado.hpp\"\n#include <cstddef>\n#include <limits>\n";
    cpp << "static const int START_STATE_ID = " << startStateId << ";\n";
    // Tabela de transição
    cpp << "static const int TRANS_TABLE[" << states.size() << "][256] = {\n";
    for (const auto& row : trans_table) {
        cpp << "  { ";
        for (int j = 0; j < 256; ++j) {
            cpp << row[j];
            if (j < 255) cpp << ", ";
        }
        cpp << " },\n";
    }
    cpp << "};\n";
    // Tabela de aceitação
    cpp << "static const int ACCEPT_TABLE[" << states.size() << "] = { ";
    for (size_t i = 0; i < accept_table.size(); ++i) {
        cpp << (accept_table[i] ? static_cast<int>(i) : -1);
        if (i + 1 < accept_table.size()) cpp << ", ";
    }
    cpp << " };\n";
    // Tabela de tokens
    cpp << "static const char* TOKEN_TABLE[" << states.size() << "] = { ";
    for (size_t i = 0; i < accept_table.size(); ++i) {
        if (accept_table[i]) cpp << '\"' << escape_cpp_string(accept_table[i]->token) << '\"';
        else cpp << "nullptr";
        if (i + 1 < accept_table.size()) cpp << ", ";
    }
    cpp << " };\n";
    // Tabela de prioridades
    cpp << "static const int PRIORITY_TABLE[" << states.size() << "] = { ";
    for (size_t i = 0; i < accept_table.size(); ++i) {
        if (accept_table[i]) cpp << accept_table[i]->priority;
        else cpp << -1;
        if (i + 1 < accept_table.size()) cpp << ", ";
    }
    cpp << " };\n";
    // Função scan
    cpp << R"(
std::vector<TokenMatch> scan(const std::string& input) {
    std::vector<TokenMatch> result;
    size_t pos = 0;

    int line = 1;
    int column = 1;

    while (pos < input.size()) {
        const int token_line = line;
        const int token_column = column;

        int state = START_STATE_ID;
        int last_accept = -1;
        size_t last_accept_pos = pos;
        int last_priority = std::numeric_limits<int>::max();

        for (size_t i = pos; i < input.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(input[i]);
            state = TRANS_TABLE[state][c];

            if (state == -1) break;

            if (ACCEPT_TABLE[state] != -1) {
                const size_t current_end = i + 1;
                if (last_accept == -1 ||
                    current_end > last_accept_pos ||
                    (current_end == last_accept_pos && PRIORITY_TABLE[state] < last_priority)) {
                    last_accept = state;
                    last_accept_pos = current_end;
                    last_priority = PRIORITY_TABLE[state];
                }
            }
        }

        if (last_accept != -1) {
            result.push_back(TokenMatch{
                TOKEN_TABLE[last_accept],
                static_cast<int>(pos),
                static_cast<int>(last_accept_pos),
                token_line,
                token_column
            });

            // advance line/column across the matched lexeme
            for (size_t i = pos; i < last_accept_pos; ++i) {
                if (input[i] == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
            }
            pos = last_accept_pos;
        } else {
            // unknown character: skip it, but still update line/column
            if (input[pos] == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            pos += 1;
        }
    }
    return result;
}
)";
    cpp.close();
}
