#pragma once

#include <algorithm>
#include <stack>
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

        for (const auto e : eClosure(std::set<State*>{s})) {
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
