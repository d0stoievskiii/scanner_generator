#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cctype>
#include <ctime>
#include <cstdlib>
#include <cerrno>

#include "scanner_manager.hpp"
#include "state_minimizator.hpp"

struct Position {
    size_t index = 0;
    size_t line = 1;
    size_t column = 1;
};

struct TokenData {
    size_t id = 0;
    std::string type;
    std::string symbol;
    int priority = -1;
    std::string lexeme;
    size_t length = 0;
    Position start;
    Position end;
    std::string channel = "default";
    bool ignored = false;

    std::string valueKind = "symbol";
    std::string valueString;
    bool hasInt = false;
    long long intValue = 0;
    bool hasFloat = false;
    double floatValue = 0.0;
    bool hasBool = false;
    bool boolValue = false;
};

static std::string readTextFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("Could not open input file: " + path);
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

static std::string jsonEscape(const std::string& s) {
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

static std::string nowIso8601Utc() {
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

static std::string toLowerCopy(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

static bool parseInteger(const std::string& s, long long& value) {
    if (s.empty()) return false;

    char* end = nullptr;
    errno = 0;
    long long v = std::strtoll(s.c_str(), &end, 10);

    if (errno != 0 || end == s.c_str() || *end != '\0') {
        return false;
    }

    value = v;
    return true;
}

static bool parseFloat(const std::string& s, double& value) {
    if (s.empty()) return false;

    char* end = nullptr;
    errno = 0;
    double v = std::strtod(s.c_str(), &end);

    if (errno != 0 || end == s.c_str() || *end != '\0') {
        return false;
    }

    // evita tratar inteiro puro como float
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos && s.find('E') == std::string::npos) {
        return false;
    }

    value = v;
    return true;
}

static bool parseBoolean(const std::string& s, bool& value) {
    std::string l = toLowerCopy(s);
    if (l == "#t" || l == "true") {
        value = true;
        return true;
    }
    if (l == "#f" || l == "false") {
        value = false;
        return true;
    }
    return false;
}

static bool parseQuotedString(const std::string& s, std::string& value) {
    if (s.size() < 2 || s.front() != '"' || s.back() != '"') {
        return false;
    }

    std::string out;
    out.reserve(s.size() - 2);

    for (size_t i = 1; i + 1 < s.size(); i++) {
        char c = s[i];
        if (c == '\\' && i + 2 < s.size()) {
            char n = s[i + 1];
            switch (n) {
                case 'n': out.push_back('\n'); break;
                case 't': out.push_back('\t'); break;
                case 'r': out.push_back('\r'); break;
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                default: out.push_back(n); break;
            }
            i++;
        } else {
            out.push_back(c);
        }
    }

    value = out;
    return true;
}

static void inferTypedValue(TokenData& token) {
    token.valueString = token.lexeme;

    std::string quoted;
    if (parseQuotedString(token.lexeme, quoted)) {
        token.valueKind = "string";
        token.valueString = quoted;
        return;
    }

    bool b = false;
    if (parseBoolean(token.lexeme, b)) {
        token.valueKind = "boolean";
        token.hasBool = true;
        token.boolValue = b;
        token.valueString = b ? "true" : "false";
        return;
    }

    long long i = 0;
    if (parseInteger(token.lexeme, i)) {
        token.valueKind = "integer";
        token.hasInt = true;
        token.intValue = i;
        return;
    }

    double f = 0.0;
    if (parseFloat(token.lexeme, f)) {
        token.valueKind = "float";
        token.hasFloat = true;
        token.floatValue = f;
        return;
    }

    token.valueKind = "symbol";
}

static void advancePosition(Position& p, char c) {
    p.index++;
    if (c == '\n') {
        p.line++;
        p.column = 1;
    } else {
        p.column++;
    }
}

static bool nextDfaState(const AFD& dfa, const DFAState& current, char input, DFAState& out) {
    auto it = dfa.transitions.find({current, input});
    if (it == dfa.transitions.end()) {
        return false;
    }

    out = it->second;
    return true;
}

static std::vector<TokenData> scanTokens(const AFD& dfa, const std::string& input) {
    std::vector<TokenData> tokens;
    size_t i = 0;
    Position cursor;
    size_t nextId = 0;

    while (i < input.size()) {
        DFAState state = dfa.start;
        size_t j = i;
        bool hasFinal = false;
        size_t bestEnd = i;
        DFAState bestState;

        while (j < input.size()) {
            DFAState next;
            if (!nextDfaState(dfa, state, input[j], next)) {
                break;
            }

            state = next;
            j++;

            auto infoIt = dfa.info.find(state);
            if (infoIt != dfa.info.end() && infoIt->second.is_final) {
                hasFinal = true;
                bestEnd = j;
                bestState = state;
            }
        }

        if (!hasFinal) {
            if (std::isspace(static_cast<unsigned char>(input[i]))) {
                advancePosition(cursor, input[i]);
                i++;
                continue;
            }

            std::ostringstream msg;
            msg << "Lexical error at line " << cursor.line
                << ", column " << cursor.column
                << ": unexpected character '" << input[i] << "'";
            throw std::runtime_error(msg.str());
        }

        const auto& info = dfa.info.at(bestState);
        std::string lexeme = input.substr(i, bestEnd - i);

        Position startPos = cursor;
        for (size_t k = i; k < bestEnd; k++) {
            advancePosition(cursor, input[k]);
        }
        Position endPos = cursor;

        TokenData token;
        token.id = nextId++;
        token.type = info.token;
        token.symbol = info.token;
        token.priority = info.priority;
        token.lexeme = lexeme;
        token.length = lexeme.size();
        token.start = startPos;
        token.end = endPos;
        inferTypedValue(token);

        tokens.push_back(token);
        i = bestEnd;
    }

    TokenData eof;
    eof.id = nextId++;
    eof.type = "EOF";
    eof.symbol = "EOF";
    eof.priority = -1;
    eof.lexeme = "";
    eof.length = 0;
    eof.start = cursor;
    eof.end = cursor;
    eof.valueKind = "eof";
    eof.valueString = "";
    tokens.push_back(eof);

    return tokens;
}

static void writeJsonTokens(const std::string& outputPath,
                            const std::string& specPath,
                            const std::string& inputPath,
                            const std::vector<TokenData>& tokens,
                            size_t originalLength) {
    std::ofstream out(outputPath);
    if (!out.is_open()) {
        throw std::runtime_error("Could not open output file: " + outputPath);
    }

    out << "{\n";
    out << "  \"schema\": \"scanner-generator-token-stream/v2\",\n";
    out << "  \"language\": \"racket\",\n";
    out << "  \"generated_at_utc\": \"" << nowIso8601Utc() << "\",\n";
    out << "  \"source\": {\n";
    out << "    \"spec_file\": \"" << jsonEscape(specPath) << "\",\n";
    out << "    \"input_file\": \"" << jsonEscape(inputPath) << "\",\n";
    out << "    \"input_length\": " << originalLength << "\n";
    out << "  },\n";
    out << "  \"summary\": {\n";
    out << "    \"token_count\": " << tokens.size() << ",\n";
    out << "    \"includes_eof\": true\n";
    out << "  },\n";
    out << "  \"tokens\": [\n";

    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& t = tokens[i];
        const bool isEOF = (t.type == "EOF");

        out << "    {\n";
        out << "      \"id\": " << t.id << ",\n";
        out << "      \"type\": \"" << jsonEscape(t.type) << "\",\n";
        out << "      \"symbol\": \"" << jsonEscape(t.symbol) << "\",\n";
        out << "      \"priority\": " << t.priority << ",\n";
        out << "      \"lexeme\": \"" << jsonEscape(t.lexeme) << "\",\n";
        out << "      \"length\": " << t.length << ",\n";
        out << "      \"channel\": \"" << jsonEscape(t.channel) << "\",\n";
        out << "      \"ignored\": " << (t.ignored ? "true" : "false") << ",\n";
        out << "      \"value_kind\": \"" << jsonEscape(t.valueKind) << "\",\n";
        out << "      \"value_string\": \"" << jsonEscape(t.valueString) << "\",\n";

        out << "      \"value_int\": ";
        if (t.hasInt) out << t.intValue;
        else out << "null";
        out << ",\n";

        out << "      \"value_float\": ";
        if (t.hasFloat) out << std::setprecision(17) << t.floatValue;
        else out << "null";
        out << ",\n";

        out << "      \"value_bool\": ";
        if (t.hasBool) out << (t.boolValue ? "true" : "false");
        else out << "null";
        out << ",\n";

        out << "      \"start\": { \"index\": " << t.start.index
            << ", \"line\": " << t.start.line
            << ", \"column\": " << t.start.column << " },\n";
        out << "      \"end\": { \"index\": " << t.end.index
            << ", \"line\": " << t.end.line
            << ", \"column\": " << t.end.column << " },\n";
        out << "      \"span\": { \"start\": " << t.start.index
            << ", \"end\": " << t.end.index << " },\n";
        out << "      \"parser\": {\n";
        out << "        \"racket\": {\n";
        out << "          \"token_type\": \"" << jsonEscape(t.type) << "\",\n";
        out << "          \"datum_kind\": \"" << jsonEscape(t.valueKind) << "\",\n";
        out << "          \"is_eof\": " << (isEOF ? "true" : "false") << "\n";
        out << "        }\n";
        out << "      }\n";
        out << "    }";
        if (i + 1 < tokens.size()) {
            out << ",";
        }
        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <regex_spec_file> [input_file] [output_json]\n";
        return 1;
    }

    try {
        Manager manager;

        AFD dfa = manager.buildDFA(argv[1]);
        AFD minimized = minimizeDFA(dfa);

        if (argc == 2) {
            std::cout << "DFA states: " << dfa.states.size() << "\n";
            std::cout << "Minimized DFA states: " << minimized.states.size() << "\n";
            return 0;
        }

        const std::string inputPath = argv[2];
        const std::string outputPath = (argc >= 4) ? argv[3] : "tokens.json";

        std::string inputText = readTextFile(inputPath);
        auto tokens = scanTokens(minimized, inputText);
        writeJsonTokens(outputPath, argv[1], inputPath, tokens, inputText.size());

        std::cout << "DFA states: " << dfa.states.size() << "\n";
        std::cout << "Minimized DFA states: " << minimized.states.size() << "\n";
        std::cout << "Tokens generated: " << tokens.size() << "\n";
        std::cout << "JSON written to: " << outputPath << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}