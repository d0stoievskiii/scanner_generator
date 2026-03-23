
#include <map>
#include <set>
#include "regex_engine.hpp"

struct State {
    int id;

    std::set<State*> epilson_transitions;
    std::map<char, std::set<State*>> transitions;
};

struct Automato {
    State* start;
    State* accept;
};

namespace StateFactory {
    inline int next_state = 0;

    inline State* newState() {
        return new State{next_state++};
    }
}


//(start) --a--> (accept)
inline Automato buildLiteral(const Literal* node) {
    auto start = StateFactory::newState();
    auto accept = StateFactory::newState();

    start->transitions[node->value].insert(accept);

    return Automato{start, accept};
}


//(start) --a,b,c,...--> (accept)
inline Automato buildCharClass(const CharClass* node) {
    auto start = StateFactory::newState();
    auto accept = StateFactory::newState();

    //funcao pra expandir a range de chars, for loop e adiciona cada um ao mapa


    return Automato{start, accept};
}


inline Automato buildNFA(const RegexNode* node) {
    /*
    if Literal → return literal fragment
    if CharClass → return class fragment
    if Concat → connect left + right
    if Union → build branching
    if Star → build loop
    if Plus → build loop (no skip)
    */
}
