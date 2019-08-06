#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct Gate {
    bool inp_a;
    bool inp_b;
    bool out;
} Gate;

void sim_and(Gate *and) { and->out = and->inp_a & and->inp_b; }
void sim_or(Gate *or)   { or->out = or->inp_a | or->inp_b; }
void sim_not(Gate *not) { not->out = !not->inp_a; }
void sim_xor(Gate *xor) { xor->out = xor->inp_a ^ xor->inp_b; }

int main()
{
    Gate *xor = malloc(sizeof(Gate *));

    xor->inp_a = 0;
    xor->inp_b = 0;
    sim_xor(xor);

    printf("output %d\n", xor->out);
    free(xor);
}
