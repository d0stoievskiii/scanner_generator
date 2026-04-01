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

struct TableLine {
    std::set<State*> start;
    char input;

    bool operator<(const TableLine& other) const {
    if (start != other.start)
        return start < other.start;
    return input < other.input;
    }

};

struct DFAStateInfo {
    bool is_final = false;
    std::string token = "";
    int priority = -1;
};

struct AFD {
    std::set<State*> start;
    std::set<std::set<State*>> states;
    std::map<std::set<State*>, DFAStateInfo> info;
    std::map<TableLine, std::set<State*>> transitions;
};

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

inline std::set<State*> move(const std::set<State*>& T, char a) {

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

// retorna o token baseado na prioridade
inline DFAStateInfo getBestToken(const std::set<State*>& states) {
    DFAStateInfo result;
    State* best = nullptr;

    for (State* s : states) {
        if (s->is_final) {
            if (!best || s->priority < best->priority) {
                best = s;
            }
        }
    }

    if (best) {
        result.is_final = true;
        result.token = best->token;
        result.priority = best->priority;
    }

    return result;
}

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
inline AFD subset_construction(Automato& NFA) {

      
    std::set<char> alphabet;
    /*
    for (char a = 0; a < 256; a++) {
        alphabet.insert(a);
    }
    */
    

    for (int a = 0; a < 256; a++) { //assuma alfabeto é a tabela ASCII
        alphabet.insert(static_cast<char>(a));
    }


    
    std::set<std::set<State*>> Dstates;
    std::queue<std::set<State*>> unmarked;
    std::map<TableLine, std::set<State*>> Dtran;

    AFD dfa;
    
    auto startset = eClosure(NFA.start);

    dfa.start = startset;
    dfa.states.insert(startset);
    dfa.info[startset] = getBestToken(startset);

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

                dfa.info[U] = getBestToken(U);
            }
            Dtran[{T, a}]= U;     
        }
    }

    dfa.states = Dstates;
    dfa.transitions = Dtran;
    dfa.start = startset;

    return dfa;
}
