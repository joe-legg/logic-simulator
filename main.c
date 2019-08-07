#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tgraphics.h"

// Types

typedef struct Gate Gate;

typedef struct Connection {
    Gate *next_gate;
    int index;
} Connection;

typedef struct Gate {
    enum { NOT, AND, OR, XOR, CUSTOM } type;
    bool *inputs;
    Connection out;

    int x, y;
} Gate;

typedef struct Wire {
    int x0, y0;
    int x1, y1;
} Wire;

// Global variables

Gate **gate_list;
int gate_list_len;

Wire **wire_list;
int wire_list_len;

// Functions

Gate *new_gate(int num_of_inputs, int type)
{
    // Allocate memory
    Gate *gate = malloc(sizeof(Gate *));
    gate->inputs = malloc(num_of_inputs * sizeof(bool *));

    // Add gate to list
    gate_list = realloc(gate_list, ++gate_list_len * sizeof(Gate *));
    gate_list[gate_list_len - 1] = gate;

    gate->type = type; // Set type
    gate->out.index = -1;

    return gate;
}

Wire *new_wire(int x0, int y0, int x1, int y1)
{
    Wire *wire = malloc(sizeof(Wire *));
    wire->x0 = x0;
    wire->y0 = y0;
    wire->x1 = x1;
    wire->y1 = y1;
    wire_list = realloc(wire_list, ++wire_list_len * sizeof(Wire *));
    wire_list[wire_list_len - 1] = wire;
    return wire;
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

void update_circuit_from_top(Gate *top_level_gate)
{
    bool result = update_gate(top_level_gate); // Update the first gate
    // Stop if the current gate is the last
    if (top_level_gate->out.next_gate->out.index < 0)
        printf("Circuit updated. Result %d\n", result);
    else
        update_circuit_from_top(top_level_gate->out.next_gate);
}

void draw_wire(const Wire *wire)
{
    draw_line(wire->x0, wire->y0, wire->x1, wire->y1, '-', TB_RED, TB_DEFAULT);
}

char *get_gate_ascii(const Gate *gate)
{
    switch (gate->type) {
    case AND:
        return "-#######\n"
               " # AND #-\n"
               "-#######";
    case OR:
        return "-#######\n"
               " # OR  #-\n"
               "-#######";
    case NOT:
        return " #######\n"
               "-# NOT #*-\n"
               " #######";
    case XOR:
        return "-#######\n"
               " # XOR #-\n"
               "-#######";
    }   
}

void draw_circuit()
{
    for (int i = 0; i < gate_list_len; i++) {
        draw_text(get_gate_ascii(gate_list[i]),
                  gate_list[i]->x, gate_list[i]->y,
                  TB_GREEN, TB_DEFAULT);
    }
    for (int i = 0; i < wire_list_len; i++) {
        draw_wire(wire_list[i]);
    }
    tb_present();
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
    update_circuit_from_top(not2);

    // Set gate positions
    xor->x = 13;
    not->x = 28;

    // Create wires
    Wire *w1 = new_wire(10, 1, 12, 1);
    Wire *w2 = new_wire(12, 1, 12, 3);

    // Graphics
    if (tb_init()) { // Initialize termbox
        printf("Error initializing termbox.");
        return 1;
    }

    draw_circuit(not2);

    struct tb_event ev;
    tb_poll_event(&ev);

    tb_shutdown();
}
