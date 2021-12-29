#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "common.h"
#include <vector>
#include <string>

int disassemble(u8 *data);

int disassemble(u8 *data, std::vector<std::string>& opcodes);

#endif
