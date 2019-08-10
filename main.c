#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "tgraphics.h"

// Types

typedef struct Wire {
    int id;
    bool state;

    int x0, y0;
    int x1, y1;
} Wire;

typedef struct Gate {
    enum { NOT, AND, OR, XOR, INPUT, CUSTOM } type;
    Wire **inputs;
    int num_of_inputs;
    Wire *output;

    bool value; // Used for inputs

    int x, y;
    int width, height;
} Gate;

// Global variables and constants

const char help[] = "\033[1;96mHelp\033[39;49m\n\n"
                    "Key                 Description\n"
                    "------------------------------------------------\n"
                    "arrow keys, hjkl    Move the cursor.\n"
                    "ctrl + q            Quit the simulator.\n"
                    "a                   Place a new gate/IO.\n"
                    "w                   Place a new wire.\n"
                    "d                   Delete component.\n"
                    "m                   Move component under cursor.\n"
                    "i                   Toggle an input's value.\n"
                    "v                   Output verilog to stdout.\n\n";

bool running = true;
bool simulate_circuit = false;

int cursor_y, cursor_x;

Gate **gate_list;
int gate_list_len;

Wire **wire_list;
int wire_list_len;

int last_wire_id;

// Functions

Gate *new_gate(int num_of_inputs, int type, int x, int y)
{
    // Allocate memory
    Gate *gate = malloc(sizeof(Gate));
    gate->inputs = malloc(num_of_inputs * sizeof(Wire *));

    // Add gate to list
    gate_list = realloc(gate_list, ++gate_list_len * sizeof(Gate *));
    gate_list[gate_list_len - 1] = gate;

    gate->type = type; // Set type

    gate->x = x;
    gate->y = y;
    if (type != CUSTOM) {
        gate->width = 8;
        gate->height = 3;
    }

    gate->value = 0;

    return gate;
}

Wire *new_wire(int x0, int y0, int x1, int y1)
{
    Wire *wire = malloc(sizeof(Wire));
    wire->x0 = x0;
    wire->y0 = y0;
    wire->x1 = x1;
    wire->y1 = y1;
    wire->state = 0;
    wire_list = realloc(wire_list, ++wire_list_len * sizeof(Wire *));
    wire_list[wire_list_len - 1] = wire;
    wire->id = ++last_wire_id;
    return wire;
}

// Simulation

void sim_and(const Gate *and)
{
    if (and->num_of_inputs > 0 && and->output != NULL) {
        and->output->state = and->inputs[0]->state;
        for (int i = 1; i < and->num_of_inputs; i++)
            and->output->state = and->output->state & and->inputs[i]->state;
    }
}

void sim_or(const Gate *or)
{
    if (or->num_of_inputs > 0 && or->output != NULL) {
        or->output->state = or->inputs[0]->state;
        for (int i = 1; i < or->num_of_inputs; i++)
            or->output->state = or->output->state | or->inputs[i]->state;
    }
}

void sim_not(const Gate *not)
{
    if (not->num_of_inputs > 0 && not->output != NULL) {
        not->output->state = !not->inputs[0]->state;
    }
}

void sim_xor(const Gate *xor)
{
    if (xor->num_of_inputs > 0 && xor->output != NULL) {
        xor->output->state = xor->inputs[0]->state;
        for (int i = 1; i < xor->num_of_inputs; i++)
            xor->output->state = xor->output->state ^ xor->inputs[i]->state;
    }
}

void sim_input(const Gate *input)
{
    if (input->output != NULL) {
        input->output->state = input->value;
    }
}

void sim_gate(const Gate *gate)
{
    switch (gate->type) {
    case AND:
        sim_and(gate);
        break;
    case OR:
        sim_or(gate);
        break;
    case NOT:
        sim_not(gate);
        break;
    case XOR:
        sim_xor(gate);
        break;
    case INPUT:
        sim_input(gate);
        break;
    case CUSTOM:
        break;
    }
}

void gate_to_verilog(char *input_str, const Gate *gate, int output,
                     int left_inp_id, int right_inp_id)
{

    switch (gate->type) {
    case AND:
        sprintf(input_str, "assign w%d = w%d & w%d;\n", output, left_inp_id,
                right_inp_id);
        break;
    case OR:
        sprintf(input_str, "assign w%d = w%d | w%d;\n", output, left_inp_id,
                right_inp_id);
        break;
    case NOT:
        sprintf(input_str, "assign w%d = ~w%d;\n", output, left_inp_id);
        break;
    case XOR:
        sprintf(input_str, "assign w%d = w%d ^ w%d;\n", output, left_inp_id,
                right_inp_id);
        break;
    case INPUT:
        // TODO
        break;
    case CUSTOM:
        break;
    }
}

void create_verilog()
{
    char *verilog = malloc(25 * sizeof(char)); // Temp string for output

    tb_shutdown();
    printf("module main;\n");
    for (int i = 0; i < wire_list_len; i++) { // Declare wires
        sprintf(verilog, "wire w%d;\n", wire_list[i]->id);
        printf("%s", verilog);
    }

    for (int i = 0; i < gate_list_len; i++) { // Ouput gates
        if (gate_list[i] != NOT) {
            gate_to_verilog(verilog, gate_list[i], gate_list[i]->output->id,
                            gate_list[i]->inputs[0]->id,
                            gate_list[i]->inputs[1]->id);
        } else {
            gate_to_verilog(verilog, gate_list[i], gate_list[i]->output->id,
                            gate_list[i]->inputs[0]->id, 0);
        }
        printf(verilog);
    }
    printf("endmodule\n");
    tb_init();
    free(verilog);
}

void update_circuit()
{
    for (int j = 0; j < gate_list_len; j++)
        for (int i = 0; i < gate_list_len; i++)
            sim_gate(gate_list[i]);
}

void build_representation_from_graphics()
{
    // Scan for connections between gates
    for (int i = 0; i < gate_list_len; i++) {
        Gate *cur_gate = gate_list[i];
        cur_gate->output = NULL;
        cur_gate->num_of_inputs = 0; // Reset gate inputs

        for (int j = 0; j < wire_list_len; j++) {
            Wire *cur_wire = wire_list[j];
            // Connection to input
            if (cur_wire->x1 == cur_gate->x && cur_wire->y1 >= cur_gate->y &&
                cur_wire->y1 != cur_gate->y + 1 &&
                cur_wire->y1 <= cur_gate->y + cur_gate->height) {

                cur_gate->inputs = realloc(cur_gate->inputs,
                        ++cur_gate->num_of_inputs * sizeof(Wire *));
                cur_gate->inputs[cur_gate->num_of_inputs - 1] = cur_wire;
            // Not gate input
            } else if (cur_wire->x1 == cur_gate->x &&
                       cur_wire->y1 == cur_gate->y + 1 &&
                       cur_gate->type == NOT) {

                cur_gate->inputs = realloc(cur_gate->inputs,
                        ++cur_gate->num_of_inputs * sizeof(Wire *));
                cur_gate->inputs[cur_gate->num_of_inputs - 1] = cur_wire;
            // Outputs
            } else if (cur_wire->x0 == cur_gate->x + cur_gate->width &&
                       cur_wire->y0 == cur_gate->y + 1) {

                cur_gate->output = cur_wire;
            // Swap wires around
            } else if ((cur_wire->x0 == cur_gate->x && 
                       cur_wire->y0 >= cur_gate->y &&
                       cur_wire->y0 != cur_gate->y + 1 &&
                       cur_wire->y0 <= cur_gate->y + cur_gate->height) ||

                       (cur_wire->x0 == cur_gate->x &&
                       cur_wire->y0 == cur_gate->y + 1 &&
                       cur_gate->type == NOT) ||

                       (cur_wire->x0 == cur_gate->x + cur_gate->width &&
                       cur_wire->y0 == cur_gate->y + 1)) {

                int tmp_x = cur_wire->x0;
                int tmp_y = cur_wire->y0;
                cur_wire->y0 = cur_wire->y1;
                cur_wire->x0 = cur_wire->x1;
                cur_wire->x1 = tmp_x;
                cur_wire->y1 = tmp_y;
            }
        }
    }

    /*
    // Scan for connections between wires
    for (int i = 0; i < wire_list_len; i++) {
        for (int j = 0; j < wire_list_len; j++) {
            if (j == i) continue;
            Wire *wire_a = wire_list[i];
            Wire *wire_b = wire_list[j];

            // Check for wire connection
            if (((wire_b->x1 == wire_a->x0 && wire_b->y1 < wire_a->y1) ||
                (wire_b->x1 < wire_a->x1 && wire_b->y1 == wire_a->y1)  ||
                (wire_b->x1 == wire_a->x1 || wire_b->y1 == wire_a->y1))
                ||
                ((wire_b->x0 == wire_a->x0 && wire_b->y0 < wire_a->y1) ||
                (wire_b->x0 < wire_a->x1 && wire_b->y0 == wire_a->y1)  ||
                (wire_b->x0 == wire_a->x1 || wire_b->y0 == wire_a->y1))) {
                
                if (wire_b->state || wire_a->state) {
                    wire_b->state = wire_b->state | wire_a->state;
                }
            }
        }
    }
    */
}

// Graphics

void draw_wire(const Wire *wire)
{
    if (wire->state) {
        tb_change_cell(wire->x0, wire->y0, '-', TB_BLUE|TB_BOLD, TB_DEFAULT);
        tb_change_cell(wire->x1, wire->y1, '-', TB_BLUE|TB_BOLD, TB_DEFAULT);
        draw_line(wire->x0, wire->y0 + 1, wire->x0, wire->y1, '-',
                  TB_BLUE|TB_BOLD, TB_DEFAULT);
        draw_line(wire->x0, wire->y1, wire->x1, wire->y1, '-', TB_BLUE|TB_BOLD,
                  TB_DEFAULT);
    } else {
        tb_change_cell(wire->x0, wire->y0, '-', TB_RED|TB_BOLD, TB_DEFAULT);
        tb_change_cell(wire->x1, wire->y1, '-', TB_RED|TB_BOLD, TB_DEFAULT);
        draw_line(wire->x0, wire->y0 + 1, wire->x0, wire->y1, '-',
                  TB_RED|TB_BOLD, TB_DEFAULT);
        draw_line(wire->x0, wire->y1, wire->x1, wire->y1, '-', TB_RED|TB_BOLD,
                  TB_DEFAULT);
    }
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
               "-# NOT #-\n"
               " #######";
    case XOR:
        return "-#######\n"
               " # XOR #-\n"
               "-#######";
    case INPUT:
        return "########\n"
               "# Inp  #-\n"
               "########";
    case CUSTOM:
        break;
    }
    return "";
}

void draw_circuit()
{
    for (int i = 0; i < gate_list_len; i++) {
        if (gate_list[i]->type == INPUT) {
            if (gate_list[i]->value == 1)
                draw_text(get_gate_ascii(gate_list[i]),
                          gate_list[i]->x, gate_list[i]->y,
                          TB_BLUE|TB_BOLD, TB_DEFAULT);
            else
                draw_text(get_gate_ascii(gate_list[i]),
                          gate_list[i]->x, gate_list[i]->y,
                          TB_RED|TB_BOLD, TB_DEFAULT);
        } else {
            draw_text(get_gate_ascii(gate_list[i]),
                      gate_list[i]->x, gate_list[i]->y,
                      TB_GREEN, TB_DEFAULT);
        }
    }
    for (int i = 0; i < wire_list_len; i++)
        draw_wire(wire_list[i]);
}

void draw()
{
    tb_clear();
    draw_circuit();
    tb_change_cell(cursor_x, cursor_y, '+', TB_WHITE, TB_DEFAULT);
    if (simulate_circuit) draw_text("Simulation running.", 0, tb_height() - 1,
                                    TB_WHITE, TB_DEFAULT);
    tb_present();
}

// User Input

int get_gate_under_cursor()
{
    for (int i = 0; i < gate_list_len; i++)
        if (cursor_x >= gate_list[i]->x &&
            cursor_y >= gate_list[i]->y &&
            cursor_x < gate_list[i]->width + gate_list[i]->x &&
            cursor_y < gate_list[i]->height + gate_list[i]->y)
            return i;
    return -1;
}

int get_wire_under_cursor()
{
    for (int i = 0; i < wire_list_len; i++)
        if ((cursor_x == wire_list[i]->x0 && cursor_y < wire_list[i]->y1) ||
            (cursor_x < wire_list[i]->x1 && cursor_y == wire_list[i]->y1) ||
            (cursor_x == wire_list[i]->x1 || cursor_y == wire_list[i]->y1))
            return i;
    return -1;
}

void handle_cursor_input(const struct tb_event *event)
{
    if (event->key == TB_KEY_ARROW_DOWN || event->ch == 'j' ||
        event->ch == 'J') {
        if (!(cursor_y + 2 > tb_height())) cursor_y++;
    } else if (event->key == TB_KEY_ARROW_UP || event->ch == 'k' ||
               event->ch == 'K') {
        if (!(cursor_y - 1 < 0)) cursor_y--;
    } else if (event->key == TB_KEY_ARROW_LEFT || event->ch == 'h' ||
               event->ch == 'H') {
        if (!(cursor_x - 1 < 0)) cursor_x--;
    } else if (event->key == TB_KEY_ARROW_RIGHT || event->ch == 'l' ||
               event->ch == 'L') {
        if (!(cursor_x + 2 > tb_width())) cursor_x++;
    }
}

void move_component_at_cursor()
{
    struct tb_event event;
    int gate_index = get_gate_under_cursor();

    if (gate_index >= 0) { // If there is a gate under the cursor
        Gate *gate_to_move = gate_list[gate_index];
        int gate_start_x = gate_to_move->x;
        int gate_start_y = gate_to_move->y;

        tb_peek_event(&event, 1);
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
        int wire_index = get_wire_under_cursor();

        if (wire_index >= 0) { // If there is a gate under the cursor
            Wire *wire_to_move = wire_list[wire_index];
            int wire_start_x0 = wire_to_move->x0;
            int wire_start_y0 = wire_to_move->y0;
            int wire_start_x1 = wire_to_move->x1;
            int wire_start_y1 = wire_to_move->y1;

            int dx = abs(wire_start_x0 - wire_start_x1);
            int dy = abs(wire_start_y0 - wire_start_y1);

            tb_peek_event(&event, 1);
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

void place_wire()
{
    struct tb_event event;
    Wire *wire = new_wire(cursor_x, cursor_y, 0, 0);

    tb_peek_event(&event, 1);
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

void place_gate_at_cursor()
{
    struct tb_event event;

    // Display the options
    draw_text("Select the gate to place.\n0. None\n1. AND\n2. OR\n3. XOR\n"
              "4. NOT\n5. Input",
              0, tb_height() - 7, TB_WHITE, TB_DEFAULT);
    tb_present();
    tb_poll_event(&event);

    if (event.ch < '0' || event.ch > '5') {
        draw_text("Invalid selection!", 0, tb_height() - 1,
                  TB_RED|TB_BOLD, TB_DEFAULT);
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
    case '5':
        new_gate(0, INPUT, cursor_x, cursor_y);
        break;
    }
}

void handle_input()
{
    struct tb_event event;
    tb_peek_event(&event, 1);

    handle_cursor_input(&event);

    if (event.key == TB_KEY_CTRL_Q) { // Quit
        draw_text("Are you sure you want to quit? [y/n]", 0, tb_height() - 1,
                  TB_WHITE, TB_DEFAULT);
        tb_present();
        tb_poll_event(&event);
        if (event.ch == 'y' || event.ch == 'Y')
            running = false;
    } else if (event.ch == '?') { // Help
        draw_line(0, tb_height() - 14, tb_width(), tb_height() - 14, '_',
                  TB_WHITE, TB_DEFAULT);
        draw_text(help, 0, tb_height() - 13, TB_WHITE, TB_DEFAULT);
        tb_present();
        tb_poll_event(&event);
    } else if (event.ch == 'd' || event.ch == 'D') { // Delete gate
        int gate_to_delete = get_gate_under_cursor();

        if (gate_to_delete >= 0) {
            free(gate_list[gate_to_delete]);
            for(int i = gate_to_delete; i < gate_list_len; i++)
                gate_list[i] = gate_list[i + 1];
            gate_list_len--;
        } else {
            int wire_to_delete = get_wire_under_cursor();

            if (wire_to_delete >= 0) {
                free(wire_list[wire_to_delete]);
                for(int i = wire_to_delete; i < wire_list_len; i++)
                    wire_list[i] = wire_list[i + 1];
                wire_list_len--;
            }
        }
    } else if (event.ch == 'm' || event.ch == 'M') { // Move component
        move_component_at_cursor();
    } else if (event.ch == 'a' || event.ch == 'A') { // Place gate
        place_gate_at_cursor();
    } else if (event.ch == 'w' || event.ch == 'W') { // Place wire
        place_wire();
    } else if (event.key == TB_KEY_SPACE) { // Toggle simulation
        simulate_circuit = !simulate_circuit;
    } else if (event.ch == 'i' || event.ch == 'I') { // Toggle input's value
        Gate *gate_under_cursor = gate_list[get_gate_under_cursor()];
        if (gate_under_cursor->type == INPUT)
            gate_under_cursor->value = !gate_under_cursor->value;
    } else if (event.ch == 'v' || event.ch == 'V') {
        create_verilog();
    }
}

int main()
{
    // Graphics
    if (tb_init()) { // Initialize termbox
        printf("Error initializing termbox.");
        return 1;
    }
    
    // Center cursor
    cursor_x = tb_width() / 2;
    cursor_y = tb_height() / 2;

    // Main loop
    while (running) {
        handle_input();
        draw();
        build_representation_from_graphics();
        if (simulate_circuit) update_circuit();
    }


    tb_shutdown();
}
