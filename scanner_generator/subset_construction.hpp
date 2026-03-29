#pragma once

#include <algorithm>
#include <stack>
#include <queue>
#include "mythompson.hpp"


/*
push all states of T onto stack;
initialize t-closure(T) to T;
while ( stack is not empty ) 
    pop t, the top element, off stack;
    for ( each state u with an edge from t to u labeled t )
        if ( u is not in t-closure(T) )
            add u to t-closure(T);
            push u onto stack;
-->livro pg.154
*/
inline std::set<State*> eClosure(const std::set<State*>& T) {
    std::set<State*> ret = T;//todo estado é alcançavel por si mesmo numa transição vazia
    std::stack<State*> stack;
    for (const auto s : T) {
        stack.push(s);
    }

    while (!stack.empty()) {
        auto s = stack.top();
        stack.pop();

        for (const auto e : s->epsilon_transitions) {
            if (!ret.count(e)) {
                ret.insert(e);
                stack.push(e);
            }
        }
    }
    
    return ret;
}

inline std::set<State*> eClosure(State* s) {
    return eClosure(std::set<State*>{s});
}

inline std::set<State*> move(std::set<State*> T, char a) {

    std::set<State*> ret ; 

    for (const auto s : T) {
        if (s->transitions.count(a)) {
            for (const auto t : s->transitions[a]) {
                ret.insert(t);
            }
        }
    }

    return ret; 
}

struct TableLine {
    std::set<State*> start;
    char input;
};

/*
while ( there is an unmarked state T in Dstates )
    mark T;
    for ( each input symbol a )

        U = t-closure( move(T, a));
        if ( U is not in Dstates )
            add U as an unmarked state to Dstates;
        Dtran[T, a] = U; 

-->livro pg.154
*/
inline std::map<TableLine, std::set<State*>> subset_construction(Automato& NFA) {

    std::set<char> alphabet;
    for (char a = 0; a < 256; a++) {//assuma alfabeto é a tabela ASCII
        alphabet.insert(a);
    }


    std::set<std::set<State*>> Dstates;
    std::queue<std::set<State*>> unmarked;
    std::map<TableLine, std::set<State*>> Dtran;
    
    auto startset = eClosure(NFA.start);
    Dstates.insert(startset);
    unmarked.emplace(startset);

    while (!unmarked.empty()) {
        auto T = unmarked.front();
        unmarked.pop();

        for (const auto& a : alphabet) {
            
            auto U = eClosure(move(T, a));
            if (U.empty())
                continue;
            if (!Dstates.count(U)) {
                Dstates.insert(U);
                unmarked.emplace(U);
            }
            Dtran[{T, a}]= U;     
        }
    }
    return Dtran;
}
