#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct Gate Gate;

typedef struct Output {
    Gate *next_gate;
    int index;
} Output;

typedef struct Gate {
    enum GateType { NOT, AND, OR, XOR, CUSTOM } type;
    bool *inputs;
    Output out;
} Gate;

Gate *new_tri_gate()
{
    Gate *gate = malloc(sizeof(Gate *));
    gate->inputs = malloc(2 * sizeof(bool *));
    return gate;
}

Gate *new_not_gate()
{
    Gate *not= malloc(sizeof(Gate *));
    not->inputs = malloc(sizeof(bool *));
    not->type = NOT;
    return not;
}

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

void update_gate(Gate *gate)
{
    (gate->out.next_gate)->inputs[gate->out.index] = sim_gate(gate);
}

int main()
{
    Gate *xor = new_tri_gate();
    xor->type = XOR;
    Gate *not = new_not_gate();
    Gate *not2 = new_not_gate();

    // Connect outputs
    not2->out.next_gate = xor;
    not2->out.index = 0;
    xor->out.next_gate = not;
    xor->out.index = 0;
    
    // Set inputs
    not2->inputs[0] = 1;
    xor->inputs[0] = 1;

    // Evaluate circuit
    update_gate(not2);
    update_gate(xor);
    printf("output %d\n", sim_gate(not));
}
