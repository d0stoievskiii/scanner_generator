#include <vector>
#include <memory>
#include <string>
#include <optional>
#include "regex_tokenizer.hpp"

/*
definimos as structs para montar a arvore sintatica apartir do regex

RegexNode
├── Literal(char) -> 'a', '1'
├── CharClass(set/ranges of chars) -> "[A-Z]", "[0-9]"
├── Concat(left, right) -> "ab" = Concat(Literal(a), Literal(b))
├── Union(left, right) -> "a|b" = Union(Literal(a), Literal(b))
├── Star(child) -> a* = Star(Literal(a))
├── Plus(child) -> a+ = Plus(Literal(a)) -> indica pelo menos uma ocorrencia
*/

//tipos de node da arvore sintatica
enum class RegexNodeType {
    Literal,
    CharClass,
    Concat,
    Union,
    Star,
    Plus
};

//AST TREE NODES
struct RegexNode {
    virtual ~RegexNode() = default;
};

struct Literal : RegexNode {
    char value;
    Literal(char c) : value(c) {}
};

struct CharClass : RegexNode {
    CharClassInfo clsinfo;
    CharClass(const CharClassInfo& ci) : clsinfo(ci) {}
};

struct Concat : RegexNode {
    std::unique_ptr<RegexNode> left;
    std::unique_ptr<RegexNode> right;
    Concat(std::unique_ptr<RegexNode> l, std::unique_ptr<RegexNode> r)
     : left(std::move(l)), right(std::move(r)) {}
};

struct Union : RegexNode {
    std::unique_ptr<RegexNode> left;
    std::unique_ptr<RegexNode> right;
    Union(std::unique_ptr<RegexNode> l, std::unique_ptr<RegexNode> r)
     : left(std::move(l)), right(std::move(r)) {}
};

struct Star : RegexNode {
    std::unique_ptr<RegexNode> child;

    Star(std::unique_ptr<RegexNode> c) : child(std::move(c)) {}
};

struct Plus : RegexNode {
    std::unique_ptr<RegexNode> child;

    Plus(std::unique_ptr<RegexNode> c) : child(std::move(c)) {}
};

/*
parse_regex()        -> parse_union()
parse_union()
parse_concat()
parse_repetition()
parse_primary()
*/
//top down regex parser
class RegexEngine
{
    std::vector<Token> tokens;
    uint32_t pos;

    bool match(TokenType type)
    {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    const Token& expect(TokenType type) {
        if (check(type)) {
            return advance();
        }
        throw std::runtime_error("Esperava outro token");
    }

    bool check(TokenType type) const
    {
        if (isAtEnd()) {
            return type == TokenType::END_OF_INPUT;
        }
        return tokens[pos].type == type;
    }

    const Token& advance()
    {
        if (!isAtEnd()) {
            pos++;
        }
        return previous();
    }

    const Token& peek() const
    {
        return tokens[pos];
    }

    const Token& previous() const
    {
        return tokens[pos - 1];
    }

    bool isAtEnd() const
    {
        return tokens[pos].type == TokenType::END_OF_INPUT;
    }

public:
    RegexEngine(std::string regex) {
        RegexTokenizer tokenizer = RegexTokenizer(regex);
        tokens = tokenizer.tokenize();
        pos = 0;
    }

    std::unique_ptr<RegexNode> parse() {
        auto node =  parse_union();
        expect(TokenType::END_OF_INPUT);
        return node;
    }

    std::unique_ptr<RegexNode> parse_union() {
        auto left = parse_concat();

        while(match(TokenType::PIPE)) {//char |
            auto right = parse_concat();
            left = std::make_unique<Union>(std::move(left), std::move(right));
        }
        return left;
    }

    std::unique_ptr<RegexNode> parse_concat() {
        auto left = parse_repetition();

        while (startsPrimary(peek())) {
            auto right = parse_repetition();
            left = std::make_unique<Concat>(std::move(left), std::move(right));
        }

        return left;
    }

    std::unique_ptr<RegexNode> parse_repetition();

    std::unique_ptr<RegexNode> parse_primary();

    bool startsPrimary(const Token& t);




};
