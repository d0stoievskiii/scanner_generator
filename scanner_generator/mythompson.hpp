
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

    for (char c : expandCharClass(node->clsinfo)) {
        start->transitions[c].insert(accept);
    }

    return Automato{start, accept};
}

/*
Suppose r = st. Then construct N(r) as in Fig. 3.41. The start state of
N ( s) becomes t
he start state of N (r ) , and the accepting state of N (t) is
the only accepting state of N (r ) . The accepting state of N ( s) and the
start state of N(t) are merged into a single state, with all the transitions
in or out of either state. //livro pg.160
*/

//(left.start)-char->((left.accept)-epilson->(right.start))-char->(right.accept)
inline Automato buildConcat(Automato left, Automato right) {
    left.accept->epilson_transitions.insert(right.start);

    return Automato{left.start, right.accept};
}

/*
Suppose r = sit. Then N(r), the NFA for r(...) Here, i and j are new states, the start and accepting states of N(r) ,
respectively. There are E-transitions from i to the start states of N (s)
and N ( t) , and each of their accepting states have E-transitions to the
accepting state j. //pg. 160
*/
/*   --epilson-->(left.start)--->(left.accept) \
    /                                           \
(start)                                           (accept)
    \                                            /
     --epilson-->(right.start)--->(right.accept)/
*/
inline Automato buildUnion(Automato left, Automato right) {
    auto start = StateFactory::newState();
    auto accept = StateFactory::newState();

    start->epilson_transitions.insert(left.start);
    start->epilson_transitions.insert(right.start);

    left.accept->epilson_transitions.insert(accept);
    right.accept->epilson_transitions.insert(accept);

    return Automato{start, accept};
}

/*
Suppose r = s* . Then for r we construct the NFA N(r) shown in Fig. 3.42.
Here, i and f are new states, the start state and lone accepting state of
N (r) . To get from i to f, we can either follow the introduced path labeled
E, which takes care of the one string in L(s)°, or we can go to the start
state of N(8) , through that NFA, then from its accepting state back to
its start state zero or more times. These options allow N(r) to accept all
the strings in L(s)1, L(s)2, and so on, so the entire set of strings accepted
by N(r) is L(s*)
*/

//ver figura na pg 161
inline Automato buildStar(Automato s) {
    auto start = StateFactory::newState();
    auto accept = StateFactory::newState();

    start->epilson_transitions.insert(s.start);
    start->epilson_transitions.insert(accept);
    s.accept->epilson_transitions.insert(s.start);
    s.accept->epilson_transitions.insert(accept);

    return Automato{start, accept};

}

inline Automato buildNFA(const RegexNode* node) {
    /*
    if Literal -> return literal fragment
    if CharClass -> return class fragment
    if Concat -> connect left + right
    if Union -> build branching
    if Star -> build loop
    if Plus -> build loop (no skip)
    */
}
