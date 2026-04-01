#pragma once

#include <vector>
#include <memory>
#include <string>
#include <optional>
#include <iostream>
#include <cstdint>
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

//folhas
struct Literal : RegexNode {
    char value;
    Literal(char c) : value(c) {}
};

struct CharClass : RegexNode {
    CharClassInfo clsinfo;
    CharClass(const CharClassInfo& ci) : clsinfo(ci) {}
};

//nos internos da arvore
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
void print_aux(std::unique_ptr<RegexNode> ast, int nivel) {
    print_aux(no->esq, nivel+1);
    for (int i = 0; i < nivel; i++) printf("\t");
    printf("%d\n", no->chave);
    print_aux(no->dir, nivel+1);
}
*/

inline std::vector<char> expandCharClass(const CharClassInfo& clss) {
    std::vector<char> ret;
    //problema adicionar mesmo char 2 vezes se regex for mal formado
    for (auto& range : clss.char_ranges) {
        for (char c = range.start; c <= range.end; c++) {
            ret.push_back(c);
        }
    }
    for (auto& c : clss.singles) {
        ret.push_back(c);
    }
    return ret;
}

inline void print_aux(std::ostream& out, int level) {
    for (int i = 0; i < level; ++i) {
        out << " ";
    }
}

//lida com escaped chars, util pra definir espaco branco
inline std::string charToPrintable(char c) {
    switch (c) {
        case '\n': return "\\n";
        case '\t': return "\\t";
        case '\r': return "\\r";
        case '\\': return "\\\\";
        default: return std::string(1, c);
    }
}

inline void print_CharClass(std::ostream& out, CharClassInfo clss) {
    out << "[";
    
    if(clss.negated) {
        out << "^";
    }

    for (const auto& r : clss.char_ranges) {
        out << charToPrintable(r.start) << "-" << charToPrintable(r.end);
    }

    for (char c : clss.singles) {
        out << charToPrintable(c);
    }

    out << "]";
}

inline void printAst(const RegexNode* node, std::ostream& out, int indent = 0) {
    if (!node) {
        print_aux(out, indent);
        out << "<null>\n";
        return;
    }

    if (auto lit = dynamic_cast<const Literal*>(node)) {
        print_aux(out, indent);
        out << "Literal(" << charToPrintable(lit->value) << ")\n";
        return;
    }

    if (auto cls = dynamic_cast<const CharClass*>(node)) {
        print_aux(out, indent);
        out << "CharClass(";
        print_CharClass(out, cls->clsinfo);
        out << ")\n";
        return;
    }

    if (auto cat = dynamic_cast<const Concat*>(node)) {
        print_aux(out, indent);
        out << "Concat(\n";
        printAst(cat->left.get(), out, indent + 2);
        printAst(cat->right.get(), out, indent + 2);
        print_aux(out, indent);
        out << ")\n";
        return;
    }

    if (auto uni = dynamic_cast<const Union*>(node)) {
        print_aux(out, indent);
        out << "Union(\n";
        printAst(uni->left.get(), out, indent + 2);
        printAst(uni->right.get(), out, indent + 2);
        print_aux(out, indent);
        out << ")\n";
        return;
    }

    if (auto star = dynamic_cast<const Star*>(node)) {
        print_aux(out, indent);
        out << "Star(\n";
        printAst(star->child.get(), out, indent + 2);
        print_aux(out, indent);
        out << ")\n";
        return;
    }

    if (auto plus = dynamic_cast<const Plus*>(node)) {
        print_aux(out, indent);
        out << "Plus(\n";
        printAst(plus->child.get(), out, indent + 2);
        print_aux(out, indent);
        out << ")\n";
        return;
    }

    throw std::runtime_error("Unkown regexnode??");
}

/*
chamamos pela ordem de precedencia
parse_regex()
    |
    v
parse_union()
     |
     v
parse_concat()
     |
     v
parse_repetition()
     |
     v
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
        throw std::runtime_error("Outro token era esperado!");
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
        //enquanto o proximo token puder começar uma nova expressão, concatenamos
        while (startsPrimary(peek())) {
            auto right = parse_repetition();
            left = std::make_unique<Concat>(std::move(left), std::move(right));
        }

        return left;
    }

    std::unique_ptr<RegexNode> parse_repetition() {
        auto node = parse_primary();

        while (true) {
            if (match(TokenType::STAR)) {
                node = std::make_unique<Star>(std::move(node));
            } else if (match(TokenType::PLUS)) {
                node = std::make_unique<Plus>(std::move(node));
            } else {
                break;
            }
        }

        return node;
    }

    std::unique_ptr<RegexNode> parse_primary() {
        if (match(TokenType::LITERAL)) {
            return std::make_unique<Literal>(*previous().literal);
        } else if (match(TokenType::CHAR_CLASS)) {
            return std::make_unique<CharClass>(*previous().charClass);
        }

        if (match(TokenType::LPAREN)) {//sub expresion! =D
            auto node = parse_union();
            expect(TokenType::RPAREN);
            return node;
        }

        throw std::runtime_error("Expressão Primaria era esperada!");
    }

    bool startsPrimary(const Token& t) {
        return t.type == TokenType::LITERAL 
        || t.type == TokenType::CHAR_CLASS || t.type == TokenType::LPAREN;
    }
};
