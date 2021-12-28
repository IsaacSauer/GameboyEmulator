#ifndef TESTER_H
#define TESTER_H

#include <functional>

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
struct tester_flags {
    bool keep_going_on_mismatch;
    bool enable_cb_instruction_testing;
    bool print_tested_instruction;
    bool print_verbose_inputs;
};

struct tester_operations {
    std::function<void(size_t instruction_mem_size, uint8_t* instruction_mem)> init;
    std::function<void(state* state)> set_state;
    std::function<void(state* state)> get_state;
    std::function<int(uint8_t opCode)> step;

    //void (*init)(size_t instruction_mem_size, uint8_t *instruction_mem);
    //void (*set_state)(struct state *state);
    //void (*get_state)(struct state *state);
    //int (*step)(void);
};

/*
 * Runs the tester, which will run all enabled instruction from instructions.h
 * and compare the output to a reference CPU. Returns a non-zero value if one or
 * more instructions did not test successfully, and zero on success.
 */
int tester_run(struct tester_flags *flags, struct tester_operations *ops);

#ifdef __cplusplus
}
#endif

#endif
