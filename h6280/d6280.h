#ifndef D6280__HEADER
#define D6280__HEADER

//#include "common.h"
//int d6280_read_instruction(int address);

int d6280_read(int address);

int d6280_disassemble(char* buff, int pc);
int d6280_bytecode(char* buff, int pc);

#endif /* D6280__HEADER */
