#include <vector>
#include <memory>
#include <string>
#include <optional>
#include <stdexcept>
#include <sstream>


//tipos de token da expressao
enum class TokenType {
    LITERAL,
    CHAR_CLASS,
    PIPE,
    STAR,
    PLUS,
    LPAREN,
    RPAREN,
    END_OF_INPUT
};

struct CharRange {
    char start;
    char end;
};

struct CharClassInfo {
    bool negated = false;
    std::vector<CharRange> char_ranges;
    std::vector<char> singles;
};

struct Token {
    TokenType type;
    std::optional<char> literal;
    std::optional<CharClassInfo> charClass;
};

class RegexTokenizer {
private:
    std::string input;
    size_t pos = 0;

public:
    RegexTokenizer(const std::string& s) : input(s), pos(0) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;

        while (!isAtEnd()) {
            char c = peek();

            switch (c) {
                case '|':
                    advance();
                    tokens.push_back(Token{TokenType::PIPE, std::nullopt, std::nullopt});
                    break;

                case '*':
                    advance();
                    tokens.push_back(Token{TokenType::STAR, std::nullopt, std::nullopt});
                    break;

                case '+':
                    advance();
                    tokens.push_back(Token{TokenType::PLUS, std::nullopt, std::nullopt});
                    break;

                case '(':
                    advance();
                    tokens.push_back(Token{TokenType::LPAREN, std::nullopt, std::nullopt});
                    break;

                case ')':
                    advance();
                    tokens.push_back(Token{TokenType::RPAREN, std::nullopt, std::nullopt});
                    break;

                case '\\':
                    tokens.push_back(readEscapedLiteral());
                    break;

                case '[':
                    tokens.push_back(readCharClass());
                    break;

                default:
                    advance();
                    tokens.push_back(Token{TokenType::LITERAL, c, std::nullopt});
                    break;
            }
        }

        tokens.push_back(Token{TokenType::END_OF_INPUT, std::nullopt, std::nullopt});
        return tokens;
    }

private:
    bool isAtEnd() const {
        return pos >= input.size();
    }

    char peek() const {
        if (isAtEnd()) {
            throw std::runtime_error("Unexpected end of regex");
        }
        return input[pos];
    }

    char peekNext() const {
        if (pos + 1 >= input.size()) {
            return '\0';
        }
        return input[pos + 1];
    }

    char advance() {
        if (isAtEnd()) {
            throw std::runtime_error("Unexpected end of regex");
        }
        return input[pos++];
    }

    Token readEscapedLiteral() {
        advance(); // consume '\'

        if (isAtEnd()) {
            throw std::runtime_error("Dangling escape at end of regex");
        }

        char escaped = advance();

        switch (escaped) {
            case 'n':  return Token{TokenType::LITERAL, '\n', std::nullopt};
            case 't':  return Token{TokenType::LITERAL, '\t', std::nullopt};
            case 'r':  return Token{TokenType::LITERAL, '\r', std::nullopt};
            case '\\': return Token{TokenType::LITERAL, '\\', std::nullopt};
            case '(':  return Token{TokenType::LITERAL, '(', std::nullopt};
            case ')':  return Token{TokenType::LITERAL, ')', std::nullopt};
            case '[':  return Token{TokenType::LITERAL, '[', std::nullopt};
            case ']':  return Token{TokenType::LITERAL, ']', std::nullopt};
            case '|':  return Token{TokenType::LITERAL, '|', std::nullopt};
            case '*':  return Token{TokenType::LITERAL, '*', std::nullopt};
            case '+':  return Token{TokenType::LITERAL, '+', std::nullopt};
            default:
                return Token{TokenType::LITERAL, escaped, std::nullopt};
        }
    }

    Token readCharClass() {
        advance(); // consume '['

        CharClassInfo data;

        if (!isAtEnd() && peek() == '^') {
            data.negated = true;
            advance();
        }

        bool closed = false;

        while (!isAtEnd()) {
            if (peek() == ']') {
                advance();
                closed = true;
                break;
            }

            char first = readCharClassChar();

            if (!isAtEnd() && peek() == '-' && peekNext() != ']' && peekNext() != '\0') {
                advance(); // consume '-'
                char last = readCharClassChar();

                if (first > last) {
                    throw std::runtime_error("Invalid range in char class");
                }

                data.char_ranges.push_back(CharRange{first, last});
            } else {
                data.singles.push_back(first);
            }
        }

        if (!closed) {
            throw std::runtime_error("Unterminated character class");
        }

        return Token{TokenType::CHAR_CLASS, std::nullopt, data};
    }

    char readCharClassChar() {
        if (isAtEnd()) {
            throw std::runtime_error("Unexpected end inside character class");
        }

        if (peek() == '\\') {
            advance(); // consume '\'

            if (isAtEnd()) {
                throw std::runtime_error("Dangling escape in character class");
            }

            char escaped = advance();

            switch (escaped) {
                case 'n': return '\n';
                case 't': return '\t';
                case 'r': return '\r';
                case '\\': return '\\';
                case ']': return ']';
                case '[': return '[';
                case '-': return '-';
                default: return escaped;
            }
        }

        return advance();
    }
};

inline std::string tokenToString(const Token& tok) {
    std::ostringstream out;

    switch (tok.type) {
        case TokenType::LITERAL:
            out << "LITERAL(";
            if (*tok.literal == '\n') out << "\\n";
            else if (*tok.literal == '\t') out << "\\t";
            else if (*tok.literal == '\r') out << "\\r";
            else out << *tok.literal;
            out << ")";
            break;

        case TokenType::CHAR_CLASS:
            out << "CHAR_CLASS";
            break;

        case TokenType::PIPE:
            out << "PIPE";
            break;

        case TokenType::STAR:
            out << "STAR";
            break;

        case TokenType::PLUS:
            out << "PLUS";
            break;

        case TokenType::LPAREN:
            out << "LPAREN";
            break;

        case TokenType::RPAREN:
            out << "RPAREN";
            break;

        case TokenType::END_OF_INPUT:
            out << "END_OF_INPUT";
            break;
    }

    return out.str();
}


