#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "out_scanner/scanner_gerado.hpp"

static std::string jsonEscape(const std::string& input) {
    std::ostringstream out;

    for (unsigned char c : input) {
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
                if (c < 0x20) {
                    static const char* hex = "0123456789ABCDEF";
                    out << "\\u00" << hex[(c >> 4) & 0x0F] << hex[c & 0x0F];
                } else {
                    out << static_cast<char>(c);
                }
                break;
        }
    }

    return out.str();
}

int main() {
    const std::string inputPath = "racket_input.rkt";
    const std::string outputPath = "token_list.json";

    std::ifstream in(inputPath, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Erro: nao foi possivel abrir o arquivo de entrada: " << inputPath << "\n";
        return 1;
    }

    const std::string source((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const auto tokens = scan(source);

    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Erro: nao foi possivel criar o arquivo de saida: " << outputPath << "\n";
        return 1;
    }

    out << "{\n";
    out << "  \"source_file\": \"" << jsonEscape(inputPath) << "\",\n";
    out << "  \"token_count\": " << tokens.size() << ",\n";
    out << "  \"tokens\": [\n";

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& t = tokens[i];
        const std::string lexeme = source.substr(static_cast<size_t>(t.start),
                                                 static_cast<size_t>(t.end - t.start));

        out << "    { \"token\": \"" << jsonEscape(t.token)
            << "\", \"lexeme\": \"" << jsonEscape(lexeme)
            << "\", \"line\": " << t.line
            << ", \"column\": " << t.column << " }";

        if (i + 1 < tokens.size()) {
            out << ",";
        }
        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";

    std::cout << "Arquivo token_list.json gerado com " << tokens.size() << " tokens.\n";
    return 0;
}