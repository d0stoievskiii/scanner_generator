#pragma once

#include <algorithm>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "state_minimizator.hpp"

namespace afd_serializer_detail {

inline std::string jsonEscape(const std::string& s) {
    std::ostringstream out;

    for (char c : s) {
        switch (c) {
            case '"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            case '\b':
                out << "\\b";
                break;
            case '\f':
                out << "\\f";
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
                if (static_cast<unsigned char>(c) < 0x20) {
                    out << "\\u" << std::hex << std::uppercase << std::setw(4)
                        << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c))
                        << std::dec << std::nouppercase;
                } else {
                    out << c;
                }
                break;
        }
    }

    return out.str();
}

inline std::string nowIso8601Utc() {
    std::time_t now = std::time(nullptr);
    std::tm tm_utc{};
#if defined(_WIN32)
    gmtime_s(&tm_utc, &now);
#else
    gmtime_r(&now, &tm_utc);
#endif

    std::ostringstream out;
    out << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

inline std::vector<int> stateIds(const DFAState& s) {
    std::vector<int> ids;
    ids.reserve(s.size());

    for (State* st : s) {
        ids.push_back(st->id);
    }

    std::sort(ids.begin(), ids.end());
    return ids;
}

inline std::string stateKey(const DFAState& s) {
    const auto ids = stateIds(s);
    std::ostringstream out;

    for (size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << ids[i];
    }

    return out.str();
}

inline std::string printableChar(char c) {
    const unsigned char uc = static_cast<unsigned char>(c);

    switch (c) {
        case '\n':
            return "\\n";
        case '\t':
            return "\\t";
        case '\r':
            return "\\r";
        case '\0':
            return "\\0";
        case '\\':
            return "\\\\";
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

struct SerializedTransition {
    int from = -1;
    int to = -1;
    int inputCode = 0;
    std::string inputSymbol;
};

}  // namespace afd_serializer_detail

inline void writeSerializedAfd(const std::string& outputPath,
                               const std::string& specPath,
                               const AFD& dfa,
                               const AFD& minimized) {
    std::ofstream out(outputPath);
    if (!out.is_open()) {
        throw std::runtime_error("Could not open output file: " + outputPath);
    }

    std::vector<const DFAState*> orderedStates;
    orderedStates.reserve(minimized.states.size());
    for (const auto& st : minimized.states) {
        orderedStates.push_back(&st);
    }

    std::sort(orderedStates.begin(), orderedStates.end(), [](const DFAState* a, const DFAState* b) {
        return afd_serializer_detail::stateIds(*a) < afd_serializer_detail::stateIds(*b);
    });

    std::map<std::string, int> dfaId;
    for (size_t i = 0; i < orderedStates.size(); ++i) {
        dfaId[afd_serializer_detail::stateKey(*orderedStates[i])] = static_cast<int>(i);
    }

    std::vector<afd_serializer_detail::SerializedTransition> transitions;
    transitions.reserve(minimized.transitions.size());

    for (const auto& [line, target] : minimized.transitions) {
        afd_serializer_detail::SerializedTransition tr;

        const auto fromIt = dfaId.find(afd_serializer_detail::stateKey(line.start));
        const auto toIt = dfaId.find(afd_serializer_detail::stateKey(target));
        if (fromIt == dfaId.end() || toIt == dfaId.end()) {
            throw std::runtime_error("Internal error while serializing DFA states.");
        }

        tr.from = fromIt->second;
        tr.to = toIt->second;
        tr.inputCode = static_cast<int>(static_cast<unsigned char>(line.input));
        tr.inputSymbol = afd_serializer_detail::printableChar(line.input);
        transitions.push_back(tr);
    }

    std::sort(transitions.begin(), transitions.end(), [](const afd_serializer_detail::SerializedTransition& a,
                                                         const afd_serializer_detail::SerializedTransition& b) {
        if (a.from != b.from) return a.from < b.from;
        if (a.inputCode != b.inputCode) return a.inputCode < b.inputCode;
        return a.to < b.to;
    });

    size_t finalCount = 0;
    for (const auto* st : orderedStates) {
        if (minimized.info.at(*st).is_final) {
            finalCount++;
        }
    }

    const auto startIt = dfaId.find(afd_serializer_detail::stateKey(minimized.start));
    if (startIt == dfaId.end()) {
        throw std::runtime_error("Internal error: start state not found in serialized DFA.");
    }

    out << "{\n";
    out << "  \"schema\": \"scanner-generator-afd/v1\",\n";
    out << "  \"generated_at_utc\": \"" << afd_serializer_detail::nowIso8601Utc() << "\",\n";
    out << "  \"source\": {\n";
    out << "    \"spec_file\": \"" << afd_serializer_detail::jsonEscape(specPath) << "\"\n";
    out << "  },\n";
    out << "  \"alphabet\": {\n";
    out << "    \"kind\": \"byte\",\n";
    out << "    \"size\": 256\n";
    out << "  },\n";
    out << "  \"construction\": {\n";
    out << "    \"subset_state_count\": " << dfa.states.size() << ",\n";
    out << "    \"minimized_state_count\": " << minimized.states.size() << "\n";
    out << "  },\n";
    out << "  \"summary\": {\n";
    out << "    \"state_count\": " << minimized.states.size() << ",\n";
    out << "    \"transition_count\": " << transitions.size() << ",\n";
    out << "    \"final_state_count\": " << finalCount << "\n";
    out << "  },\n";
    out << "  \"afd\": {\n";
    out << "    \"start_state_id\": " << startIt->second << ",\n";
    out << "    \"states\": [\n";

    for (size_t i = 0; i < orderedStates.size(); ++i) {
        const auto& s = *orderedStates[i];
        const auto& info = minimized.info.at(s);
        const auto ids = afd_serializer_detail::stateIds(s);

        out << "      {\n";
        out << "        \"id\": " << i << ",\n";
        out << "        \"is_final\": " << (info.is_final ? "true" : "false") << ",\n";
        out << "        \"token\": ";
        if (info.is_final) {
            out << "\"" << afd_serializer_detail::jsonEscape(info.token) << "\"";
        } else {
            out << "null";
        }
        out << ",\n";
        out << "        \"priority\": ";
        if (info.is_final) {
            out << info.priority;
        } else {
            out << "null";
        }
        out << ",\n";
        out << "        \"nfa_state_ids\": [";

        for (size_t j = 0; j < ids.size(); ++j) {
            if (j > 0) {
                out << ", ";
            }
            out << ids[j];
        }

        out << "]\n";
        out << "      }";
        if (i + 1 < orderedStates.size()) {
            out << ",";
        }
        out << "\n";
    }

    out << "    ],\n";
    out << "    \"transitions\": [\n";

    for (size_t i = 0; i < transitions.size(); ++i) {
        const auto& t = transitions[i];

        out << "      { \"from\": " << t.from
            << ", \"input_code\": " << t.inputCode
            << ", \"input_symbol\": \"" << afd_serializer_detail::jsonEscape(t.inputSymbol)
            << "\", \"to\": " << t.to << " }";

        if (i + 1 < transitions.size()) {
            out << ",";
        }
        out << "\n";
    }

    out << "    ]\n";
    out << "  }\n";
    out << "}\n";
}