#include <iostream>
#include "subset_construction.hpp" // seu arquivo

int main() {
    
    // Criando estados
    State* q0 = new State{0};
    State* q1 = new State{1};
    State* q2 = new State{2};

    // Transições
    q0->transitions['a'].insert(q1);
    q1->transitions['b'].insert(q2);

    // AFN
    Automato nfa;
    nfa.start = q0;
    nfa.accept = q2;

    // Converter para DFA
    
    AFD dfa = subset_construction(nfa);
    
    // 🔍 Debug: imprimir transições
    std::cout << "DFA transitions:\n";

    for (const auto& [key, value] : dfa.transitions) {
        std::cout << "{ ";
        for (auto s : key.start) {
            std::cout << "q" << s->id << " ";
        }
        std::cout << "} --" << key.input << "--> { ";

        for (auto s : value) {
            std::cout << "q" << s->id << " ";
        }
        std::cout << "}\n";
    }

    // 🔍 Estados finais
    std::cout << "\nFinal states:\n";
    for (const auto& st : dfa.final_states) {
        std::cout << "{ ";
        for (auto s : st) {
            std::cout << "q" << s->id << " ";
        }
        std::cout << "}\n";
    }

    return 0;
}