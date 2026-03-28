#pragma once

#include <algorithm>
#include <stack>
#include <queue>
#include "mythompson.hpp"

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

}

struct TableLine {
    std::set<State*> start;
    char input;
};


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
