#pragma once

#include <map>
#include <set>
#include <queue>
#include <unordered_set>
#include <iostream>
#include "regex_engine.hpp"

struct State {
    int id;

    std::set<State*> epsilon_transitions;
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

//(left.start)-char->((left.accept)-epsilon->(right.start))-char->(right.accept)
inline Automato buildConcat(Automato left, Automato right) {
    left.accept->epsilon_transitions.insert(right.start);

    return Automato{left.start, right.accept};
}

/*
Suppose r = sit. Then N(r), the NFA for r(...) Here, i and j are new states, the start and accepting states of N(r) ,
respectively. There are E-transitions from i to the start states of N (s)
and N ( t) , and each of their accepting states have E-transitions to the
accepting state j. //pg. 160
*/
/*   --epsilon-->(left.start)--->(left.accept) \
    /                                           \
(start)                                           (accept)
    \                                            /
     --epsilon-->(right.start)--->(right.accept)/
*/
inline Automato buildUnion(Automato left, Automato right) {
    auto start = StateFactory::newState();
    auto accept = StateFactory::newState();

    start->epsilon_transitions.insert(left.start);
    start->epsilon_transitions.insert(right.start);

    left.accept->epsilon_transitions.insert(accept);
    right.accept->epsilon_transitions.insert(accept);

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

    start->epsilon_transitions.insert(s.start);
    start->epsilon_transitions.insert(accept);
    s.accept->epsilon_transitions.insert(s.start);
    s.accept->epsilon_transitions.insert(accept);

    return Automato{start, accept};

}

inline Automato buildNFA(const RegexNode* node) {
    if (!node) {
        throw std::runtime_error("regex nula!");
    }
    // if Literal -> return literal fragment
    if (auto lit = dynamic_cast<const Literal*>(node)) {
        return buildLiteral(lit);
    }
    // if CharClass -> return class fragment
    if (auto cls = dynamic_cast<const CharClass*>(node)) {
        return buildCharClass(cls);
    }
    // if Concat -> connect left + right
    if (auto cat = dynamic_cast<const Concat*>(node)) {
        Automato left = buildNFA(cat->left.get());
        Automato right = buildNFA(cat->right.get());
        return buildConcat(left, right);
    }
    // if Union -> build branching
    if (auto uni = dynamic_cast<const Union*>(node)) {
        Automato left = buildNFA(uni->left.get());
        Automato right = buildNFA(uni->right.get());
        return buildUnion(left, right);
    }
    // if Star -> build loop
    if (auto star = dynamic_cast<const Star*>(node)) {
        Automato child = buildNFA(star->child.get());
        return buildStar(child);
    }
    // if Plus -> build loop (no skip)
    if (auto plus = dynamic_cast<const Plus*>(node)) {
        Automato child = buildNFA(plus->child.get());

        Automato star = buildStar(child);
        return buildConcat(child, star);
    }

    throw std::runtime_error("Regex invalida!");
}


inline std::vector<State*> collectStates(State* start) {
    std::vector<State*> result;
    std::set<State*> visited;
    std::queue<State*> q;

    q.push(start);
    visited.insert(start);

    while (!q.empty()) {
        State* s = q.front();
        q.pop();

        result.push_back(s);

        //chars
        for (auto& [ch, targets] : s->transitions) {
            for (State* t : targets) {
                if (!visited.count(t)) {
                    visited.insert(t);
                    q.push(t);
                }
            }
        }

        //empties
        for (State* t : s->epsilon_transitions) {
            if (!visited.count(t)) {
                visited.insert(t);
                q.push(t);
            }
        }
    }

    return result;
}

inline void printState(State* s, State* accept) {
    std::cout << "State " << s->id;

    if (s == accept) {
        std::cout << " (accept)";
    }

    std::cout << ":\n";

    //char transitions
    for (const auto& [ch, targets] : s->transitions) {
        for (State* t : targets) {
            std::cout << "  --" << ch << "--> " << t->id << "\n";
        }
    }

    //epsilon transitions
    for (State* t : s->epsilon_transitions) {
        std::cout << "  --ε--> " << t->id << "\n";
    }
}

inline void printAutomato(const Automato& a) {
    auto states = collectStates(a.start);

    std::cout << "Start state: " << a.start->id << "\n";
    std::cout << "Accept state: " << a.accept->id << "\n\n";

    for (State* s : states) {
        printState(s, a.accept);
        std::cout << "\n";
    }
}
