#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <filesystem>

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

int main(int argc, char* argv[]) {
    namespace fs = std::filesystem;

    if (argc < 2) {
        std::cerr << "Uso: scanner <arquivo.rkt>\n";
        return 1;
    }

    const fs::path inputPath = argv[1];

    if (!fs::exists(inputPath)) {
        std::cerr << "Erro: arquivo nao encontrado: " << inputPath.string() << "\n";
        return 1;
    }

    std::ifstream in(inputPath, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Erro: nao foi possivel abrir o arquivo de entrada: "
                  << inputPath.string() << "\n";
        return 1;
    }

    const std::string source(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
    );

    const auto tokens = scan(source);

    // output folder: ../token_lists/
    fs::path outputDir = "../token_lists";
    fs::create_directories(outputDir);

    // use input filename without extension
    fs::path outputPath = outputDir / (inputPath.stem().string() + ".json");

    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Erro: nao foi possivel criar o arquivo de saida: "
                  << outputPath.string() << "\n";
        return 1;
    }

    out << "{\n";
    out << "  \"source_file\": \"" << jsonEscape(inputPath.string()) << "\",\n";
    out << "  \"token_count\": " << tokens.size() << ",\n";
    out << "  \"tokens\": [\n";

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& t = tokens[i];

        const std::string lexeme =
            source.substr(
                static_cast<size_t>(t.start),
                static_cast<size_t>(t.end - t.start)
            );

        out << "    { \"token\": \"" << jsonEscape(t.token)
            << "\", \"lexeme\": \"" << jsonEscape(lexeme)
            << "\", \"line\": " << t.line
            << ", \"column\": " << t.column
            << " }";

        if (i + 1 < tokens.size()) {
            out << ",";
        }

        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";

    std::cout << "Arquivo gerado: "
              << outputPath.string()
              << " (" << tokens.size() << " tokens)\n";

    return 0;
}
