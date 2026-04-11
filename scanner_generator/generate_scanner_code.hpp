#pragma once
#include <string>
#include "subset_construction.hpp"

// Gera scanner_gerado.cpp e scanner_gerado.hpp a partir do AFD minimizado
void generate_scanner_code(const AFD& afd, const std::string& outDir);
