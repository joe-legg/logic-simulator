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
    int width, height;
} Gate;

typedef struct Wire {
    int x0, y0;
    int x1, y1;
} Wire;

// Global variables and constants

const char help[] = "\033[1;96mHelp\033[39;49m\n\n"
                    "Key                 Description\n"
                    "--------------------------------------\n"
                    "arrow keys, hjkl    Move the cursor.\n"
                    "q                   Quit the simulator.\n"
                    "a                   Place a new gate.\n"
                    "w                   Place a new wire.\n"
                    "d                   Delete component.\n"
                    "m                   Move component under cursor.\n\n";

bool running = true;

int cursor_y, cursor_x;

Gate **gate_list;
int gate_list_len;

Wire **wire_list;
int wire_list_len;

// Functions

Gate *new_gate(int num_of_inputs, int type, int x, int y)
{
    // Allocate memory
    Gate *gate = malloc(sizeof(Gate *));
    gate->inputs = malloc(num_of_inputs * sizeof(bool *));

    // Add gate to list
    gate_list = realloc(gate_list, ++gate_list_len * sizeof(Gate *));
    gate_list[gate_list_len - 1] = gate;

    gate->type = type; // Set type
    gate->out.index = -1;

    gate->x = x;
    gate->y = y;
    if (type != CUSTOM) {
        gate->width = 9;
        gate->height = 3;
    }

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
    case CUSTOM:
        break;
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
    tb_change_cell(wire->x0, wire->y0, '-', TB_RED|TB_BOLD, TB_DEFAULT);
    draw_line(wire->x0, wire->y0 + 1, wire->x0, wire->y1, '-',
              TB_RED|TB_BOLD, TB_DEFAULT);
    draw_line(wire->x0, wire->y1, wire->x1, wire->y1, '-', TB_RED|TB_BOLD,
              TB_DEFAULT);
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
    case CUSTOM:
        break;
    }
    return "";
}

void draw_circuit()
{
    for (int i = 0; i < gate_list_len; i++) {
        draw_text(get_gate_ascii(gate_list[i]),
                  gate_list[i]->x, gate_list[i]->y,
                  TB_GREEN, TB_DEFAULT);
    }
    for (int i = 0; i < wire_list_len; i++)
        draw_wire(wire_list[i]);
}

void draw()
{
    tb_clear();
    draw_circuit();
    tb_change_cell(cursor_x, cursor_y, '+', TB_WHITE, TB_DEFAULT);
    tb_present();
}

Gate *get_gate_under_cursor()
{
    for (int i = 0; i < gate_list_len; i++)
        if (cursor_x >= gate_list[i]->x &&
            cursor_y >= gate_list[i]->y &&
            cursor_x < gate_list[i]->width + gate_list[i]->x &&
            cursor_y < gate_list[i]->height + gate_list[i]->y)
            return gate_list[i];
    return NULL;
}

Wire *get_wire_under_cursor()
{
    for (int i = 0; i < wire_list_len; i++)
        if ((cursor_x == wire_list[i]->x0 && cursor_y < wire_list[i]->y1) ||
            (cursor_x < wire_list[i]->x1 && cursor_y == wire_list[i]->y1))
            return wire_list[i];
    return NULL;
}

void handle_cursor_input(const struct tb_event *event)
{
    if (event->key == TB_KEY_ARROW_DOWN || event->ch == 'j') {
        if (!(cursor_y + 2 > tb_height())) cursor_y++;
    } else if (event->key == TB_KEY_ARROW_UP || event->ch == 'k') {
        if (!(cursor_y - 1 < 0)) cursor_y--;
    } else if (event->key == TB_KEY_ARROW_LEFT || event->ch == 'h') {
        if (!(cursor_x - 1 < 0)) cursor_x--;
    } else if (event->key == TB_KEY_ARROW_RIGHT || event->ch == 'l') {
        if (!(cursor_x + 2 > tb_width())) cursor_x++;
    }
}

void move_component_at_cursor()
{
    struct tb_event event;
    Gate *gate_to_move = get_gate_under_cursor();

    if (gate_to_move != NULL) { // If there is a gate under the cursor
        int gate_start_x = gate_to_move->x;
        int gate_start_y = gate_to_move->y;

        while (event.key != TB_KEY_ENTER) {
            // Handle input
            tb_poll_event(&event);
            if (event.key == TB_KEY_ESC) {
                gate_to_move->x = gate_start_x;
                gate_to_move->y = gate_start_y;
                break;
            }
            handle_cursor_input(&event);

            // Move gate to cursor
            gate_to_move->x = cursor_x;
            gate_to_move->y = cursor_y;

            // Draw to screen
            draw();
            draw_text("Press enter to place gate.", 0, tb_height() - 1,
                      TB_WHITE, TB_DEFAULT);
            tb_present();
        }
    } else {
        Wire *wire_to_move = get_wire_under_cursor();

        if (wire_to_move != NULL) {
            int wire_start_x0 = wire_to_move->x0;
            int wire_start_y0 = wire_to_move->y0;
            int wire_start_x1 = wire_to_move->x1;
            int wire_start_y1 = wire_to_move->y1;

            int dx = abs(wire_start_x0 - wire_start_x1);
            int dy = abs(wire_start_y0 - wire_start_y1);

            while (event.key != TB_KEY_ENTER) {
                // Handle input
                tb_poll_event(&event);
                if (event.key == TB_KEY_ESC) { // Reset wire
                    wire_to_move->x0 = wire_start_x0;
                    wire_to_move->y0 = wire_start_y0;
                    wire_to_move->x1 = wire_start_x1;
                    wire_to_move->y1 = wire_start_y1;
                    break;
                }
                handle_cursor_input(&event);

                // Move wire to cursor
                wire_to_move->x0 = cursor_x;
                wire_to_move->y0 = cursor_y;
                wire_to_move->x1 = cursor_x + dx;
                wire_to_move->y1 = cursor_y + dy;

                // Draw to screen
                draw();
                draw_text("Press enter to place wire.", 0, tb_height() - 1,
                          TB_WHITE, TB_DEFAULT);
                tb_present();
            }
        }
    }
}

void place_gate_at_cursor()
{
    struct tb_event event;

    // Display the options
    draw_text("Select the gate to place.\n0. None\n1. AND\n2. OR\n3. XOR\n4. NOT",
              0, tb_height() - 6, TB_WHITE, TB_DEFAULT);
    tb_present();
    tb_poll_event(&event);

    if (event.ch < '0' || event.ch > '4') {
        draw_text("Invalid selection!", 0, tb_height() - 1,
                  TB_RED, TB_DEFAULT);
        tb_present();
        tb_poll_event(&event);
    }

    switch (event.ch) {
    case '1':
        new_gate(2, AND, cursor_x, cursor_y);
        break;
    case '2':
        new_gate(2, OR, cursor_x, cursor_y);
        break;
    case '3':
        new_gate(2, XOR, cursor_x, cursor_y);
        break;
    case '4':
        new_gate(1, NOT, cursor_x, cursor_y);
        break;
    }
}

void handle_input()
{
    struct tb_event event;
    tb_peek_event(&event, 1);

    handle_cursor_input(&event);

    if (event.ch == 'q') { // Quit
        draw_text("Are you sure you want to quit? [y/n]", 0, tb_height() - 1,
                  TB_WHITE, TB_DEFAULT);
        tb_present();
        tb_poll_event(&event);
        if (event.ch == 'y' || event.ch == 'Y')
            running = false;
    } else if (event.ch == '?') { // Help
        draw_line(0, tb_height() - 12, tb_width(), tb_height() - 12, '_',
                  TB_WHITE, TB_DEFAULT);
        draw_text(help, 0, tb_height() - 11, TB_WHITE, TB_DEFAULT);
        tb_present();
        tb_poll_event(&event);
    } else if (event.ch == 'd' || event.ch == 'D') { // Delete gate
        Gate *gate_to_delete = get_gate_under_cursor();

        if (gate_to_delete != NULL) {
            gate_list_len--;
            free(gate_to_delete);
        } else {
            Wire *wire_to_delete = get_wire_under_cursor();

            if (wire_to_delete != NULL) {
                wire_list_len--;
                free(wire_to_delete);
            }
        }
    } else if (event.ch == 'm' || event.ch == 'M') { // Move component
        move_component_at_cursor();
    } else if (event.ch == 'a' || event.ch == 'A') { // Place gate
        place_gate_at_cursor();
    } else if (event.ch == 'w' || event.ch == 'W') { // Place wire
        Wire *wire = new_wire(cursor_x, cursor_y, 0, 0);

        while (event.key != TB_KEY_ENTER) {
            // Handle input
            tb_poll_event(&event);
            if (event.key == TB_KEY_ESC) {
                wire_list_len--;
                free(wire);
                break;
            }
            handle_cursor_input(&event);
            wire->x1 = cursor_x;
            wire->y1 = cursor_y;

            // Draw to screen
            draw();
            draw_text("Press enter to place wire.", 0, tb_height() - 1,
                      TB_WHITE, TB_DEFAULT);
            tb_present();
        }
    }
}

int main()
{
    // Graphics
    if (tb_init()) { // Initialize termbox
        printf("Error initializing termbox.");
        return 1;
    }

    // Main loop
    while (running) {
        handle_input();
        draw();
    }

    tb_shutdown();
}
