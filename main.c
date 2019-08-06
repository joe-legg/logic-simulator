#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct Gate Gate;

typedef struct Output {
    Gate *next_gate;
    int index;
} Output;

typedef struct Gate {
    enum { NOT, AND, OR, XOR, CUSTOM } type;
    bool *inputs;
    Output out;
} Gate;

// Global variables

Gate **gate_list;
int gate_list_len;

// Functions

Gate *new_gate(int num_of_inputs, int type)
{
    // Allocate memory
    Gate *gate = malloc(sizeof(Gate *));
    gate->inputs = malloc(num_of_inputs * sizeof(bool *));

    // Add gate to list
    gate_list = realloc(gate_list, ++gate_list_len * sizeof(Gate *));
    gate_list[gate_list_len] = gate;
    
    gate->type = type; // Set type
    gate->out.index = -1;

    return gate;
}

// Simulation

bool sim_and(const Gate *and) { return and->inputs[0] & and->inputs[1]; }
bool sim_or(const Gate *or)   { return or->inputs[0] | or->inputs[1];   }
bool sim_not(const Gate *not) { return !not->inputs[0];                 }
bool sim_xor(const Gate *xor) { return xor->inputs[0] ^ xor->inputs[1]; }

bool sim_gate(const Gate *gate)
{
    switch (gate->type) {
    case AND:
        return sim_and(gate);
    case OR:
        return sim_or(gate);
    case NOT:
        return sim_not(gate);
    case XOR:
        return sim_xor(gate);
    }
}

bool update_gate(Gate *gate)
{
    return (gate->out.next_gate)->inputs[gate->out.index] = sim_gate(gate);
}

void update_circuit(Gate *top_level_gate)
{
    bool result = update_gate(top_level_gate); // Update the first gate
    if (top_level_gate->out.next_gate->out.index < 0)
        printf("Circuit updated. Result %d\n", result);
    else
        update_circuit(top_level_gate->out.next_gate);
}

int main()
{
    // Create gates
    Gate *xor = new_gate(2, XOR);
    Gate *not = new_gate(1, NOT);
    Gate *not2 = new_gate(1, NOT);

    // Connect outputs
    not2->out.next_gate = xor;
    not2->out.index = 0;
    xor->out.next_gate = not;
    xor->out.index = 0;
    
    // Set inputs
    not2->inputs[0] = 0;
    xor->inputs[0] = 1;

    // Evaluate circuit
    update_circuit(not2);

    //for (int i = 0; i < gate_list_len; i++) TODO
    //    free(gate_list[i]);
}
