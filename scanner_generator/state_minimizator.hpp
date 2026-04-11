#pragma once
#include <set>
#include <map>
#include <vector>
#include <utility>
#include "subset_construction.hpp"

using DFAState = std::set<State*>;
using Group = std::set<DFAState>;
using Partition = std::vector<Group>;

inline Partition initialPartition(const AFD& dfa) {
    std::map<std::pair<std::string,int>, Group> finals;
    Group nonFinals;

    for (const auto& s : dfa.states) {
        const auto& info = dfa.info.at(s);

        if (!info.is_final) {
            nonFinals.insert(s);
        } else {
            finals[{info.token, info.priority}].insert(s);
        }
    }

    Partition P;

    if (!nonFinals.empty())
        P.push_back(nonFinals);

    for (auto& [_, group] : finals)
        P.push_back(group);

    return P;
}

inline int findGroup(const Partition& P, const DFAState& s) {
    for (int i = 0; i < (int)P.size(); i++) {
        if (P[i].count(s))
            return i;
    }
    return -1;
}

inline DFAState getTransition(const AFD& dfa, const DFAState& s, char a) {
    auto it = dfa.transitions.find({s, a});
    if (it != dfa.transitions.end())
        return it->second;

    return {}; // estado morto
}

// assinatura de estado
inline std::vector<int> buildSignature(
    const AFD& dfa,
    const Partition& P,
    const DFAState& s,
    const std::set<char>& alphabet)
{
    std::vector<int> sig;

    for (char a : alphabet) {
        auto target = getTransition(dfa, s, a);
        sig.push_back(findGroup(P, target));
    }

    return sig;
}

inline std::vector<Group> splitGroup(
    const AFD& dfa,
    const Partition& P,
    const Group& G,
    const std::set<char>& alphabet)
{
    std::map<std::vector<int>, Group> buckets;

    for (const auto& s : G) {
        auto sig = buildSignature(dfa, P, s, alphabet);
        buckets[sig].insert(s);
    }

    std::vector<Group> result;

    for (auto& [_, group] : buckets) {
        result.push_back(group);
    }

    return result;
}

// loop de partição
inline Partition minimizePartition(const AFD& dfa) {

    std::set<char> alphabet;
    for (int a = 0; a < 256; a++) {
        alphabet.insert(static_cast<char>(a));
    }

    Partition P = initialPartition(dfa);

    bool changed = true;

    while (changed) {
        changed = false;
        Partition newP;

        for (const auto& G : P) {
            auto splits = splitGroup(dfa, P, G, alphabet);

            if (splits.size() > 1)
                changed = true;

            for (auto& g : splits)
                newP.push_back(g);
        }

        P = newP;
    }

    return P;
}

// construir dfa minimizado
inline AFD buildMinimizedDFA(const AFD& dfa, const Partition& P) {
    AFD result;

    std::map<DFAState, DFAState> repr;

    for (const auto& group : P) {
        const DFAState& representative = *group.begin();

        for (const auto& s : group) {
            repr[s] = representative;
        }

        result.states.insert(representative);
        result.info[representative] = dfa.info.at(representative);
    }

    // estado inicial
    result.start = repr[dfa.start];

    // transições
    for (const auto& [key, target] : dfa.transitions) {
        DFAState from = repr[key.start];
        DFAState to   = repr[target];

        result.transitions[{from, key.input}] = to;
    }

    return result;
}

// minimizador principal
inline AFD minimizeDFA(const AFD& dfa) {
    auto P = minimizePartition(dfa);
    return buildMinimizedDFA(dfa, P);
}