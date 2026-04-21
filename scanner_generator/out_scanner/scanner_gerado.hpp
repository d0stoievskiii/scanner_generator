#pragma once
#include <string>
#include <vector>
#include <optional>
struct TokenMatch { std::string token; int start; int end; int line; int column;};
std::vector<TokenMatch> scan(const std::string& input);
