#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ramhack.h"
extern unsigned int dasm_read_instruction(unsigned int address);
#define d6502_read_instruction(A) dasm_read_instruction(A)
#define d6502_read_imm(A) ramhack_read_x(A)
#define d6502_read_imm_16(A) (ramhack_read_x(A) | (ramhack_read_x(A+1)<<8))


#define MAKE_INT_8(A) ((char)(A))
#define OFFSET(BASE, OFF) (((BASE) + MAKE_INT_8(OFF)) & 0xffff)

typedef enum {ILL, IMP, ACC, IMM, ZPG, ZPX, ZPY, ABS, ABX, ABY, REL, IND, INX, INY, ZPI, IAX, ZRE, IZPG, IZPX, IABS, IABX, NUM_ADDR} addr_mode_t;


static int d6502i_instruction_size_tbl[NUM_ADDR] = { 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 2, 2, 2, 3, 3, 2, 2, 3, 3 };

static char* d6502i_instruction_tbl[256] =
{
/*        0      1      2      3      4      5      6      7      8      9      A      B      C      D      E      F */
/* 0 */ "BRK", "ORA", "$02", "$03", "$04", "ORA", "ASL", "$07", "PHP", "ORA", "ASL", "$0B", "$0C", "ORA", "ASL", "$0F",
/* 1 */ "BPL", "ORA", "$12", "$13", "$14", "ORA", "ASL", "$17", "CLC", "ORA", "$1A", "$1B", "$1C", "ORA", "ASL", "$1F",
/* 2 */ "JSR", "AND", "$22", "$23", "BIT", "AND", "ROL", "$27", "PLP", "AND", "ROL", "$2B", "BIT", "AND", "ROL", "$2F",
/* 3 */ "BMI", "AND", "$32", "$33", "$34", "AND", "ROL", "$37", "SEC", "AND", "$3A", "$3B", "$3C", "AND", "ROL", "$3F",
/* 4 */ "RTI", "EOR", "$42", "$43", "$44", "EOR", "LSR", "$47", "PHA", "EOR", "LSR", "$4B", "JMP", "EOR", "LSR", "$4F",
/* 5 */ "BVC", "EOR", "$52", "$53", "$54", "EOR", "LSR", "$57", "CLI", "EOR", "$5A", "$5B", "$5C", "EOR", "LSR", "$5F",
/* 6 */ "RTS", "ADC", "$62", "$63", "$64", "ADC", "ROR", "$67", "PLA", "ADC", "ROR", "$6B", "JMP", "ADC", "ROR", "$6F",
/* 7 */ "BVS", "ADC", "$72", "$73", "$74", "ADC", "ROR", "$77", "SEI", "ADC", "$7A", "$7B", "$7C", "ADC", "ROR", "$7F",
/* 8 */ "$80", "STA", "$82", "$83", "STY", "STA", "STX", "$87", "DEY", "$89", "TXA", "$8B", "STY", "STA", "STX", "$8F",
/* 9 */ "BCC", "STA", "$92", "$93", "STY", "STA", "STX", "$97", "TYA", "STA", "TXS", "$9B", "$9C", "STA", "$9E", "$9F",
/* A */ "LDY", "LDA", "LDX", "$A3", "LDY", "LDA", "LDX", "$A7", "TAY", "LDA", "TAX", "$AB", "LDY", "LDA", "LDX", "$AF",
/* B */ "BCS", "LDA", "$B2", "$B3", "LDY", "LDA", "LDX", "$B7", "CLV", "LDA", "TSX", "$BB", "LDY", "LDA", "LDX", "$BF",
/* C */ "CPY", "CMP", "$C2", "$C3", "CPY", "CMP", "DEC", "$C7", "INY", "CMP", "DEX", "$CB", "CPY", "CMP", "DEC", "$CF",
/* D */ "BNE", "CMP", "$D2", "$D3", "$D4", "CMP", "DEC", "$D7", "CLD", "CMP", "$DA", "$DB", "$DC", "CMP", "DEC", "$DF",
/* E */ "CPX", "SBC", "$E2", "$E3", "CPX", "SBC", "INC", "$E7", "INX", "SBC", "NOP", "$EB", "CPX", "SBC", "INC", "$EF",
/* F */ "BEQ", "SBC", "$F2", "$F3", "$F4", "SBC", "INC", "$F7", "SED", "SBC", "$FA", "$FB", "$FC", "SBC", "INC", "$FF"
};

static addr_mode_t d6502i_addressing_tbl[256] =
{
/*       0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f */
/* 0 */ IMP, INX, ILL, ILL, ILL, ZPG, ZPG, ILL, IMP, IMM, ACC, ILL, ILL, ABS, ABS, ILL,
/* 1 */ REL, INY, ILL, ILL, ILL, ZPX, ZPX, ILL, IMP, ABY, ILL, ILL, ILL, ABX, ABX, ILL,
/* 2 */ IMP, INX, ILL, ILL, ZPG, ZPG, ZPG, ILL, IMP, IMM, ACC, ILL, ABS, ABS, ABS, ILL,
/* 3 */ REL, INY, ILL, ILL, ILL, ZPX, ZPX, ILL, IMP, ABY, ILL, ILL, ILL, ABX, ABX, ILL,
/* 4 */ IMP, INX, ILL, ILL, ILL, ZPG, ZPG, ILL, IMP, IMM, ACC, ILL, ABS, ABS, ABS, ILL,
/* 5 */ REL, INY, ILL, ILL, ILL, ZPX, ZPX, ILL, IMP, ABY, ILL, ILL, ILL, ABX, ABX, ILL,
/* 6 */ IMP, INX, ILL, ILL, ILL, ZPG, ZPG, ILL, IMP, IMM, ACC, ILL, IND, ABS, ABS, ILL,
/* 7 */ REL, INY, ILL, ILL, ILL, ZPX, ZPX, ILL, IMP, ABY, ILL, ILL, ILL, ABX, ABX, ILL,
/* 8 */ ILL, INX, ILL, ILL, ZPG, ZPG, ZPG, ILL, IMP, ILL, IMP, ILL, ABS, ABS, ABS, ILL,
/* 9 */ REL, INY, ILL, ILL, ZPX, ZPX, ZPY, ILL, IMP, ABY, IMP, ILL, ILL, ABX, ILL, ILL,
/* a */ IMM, INX, IMM, ILL, ZPG, ZPG, ZPG, ILL, IMP, IMM, IMP, ILL, ABS, ABS, ABS, ILL,
/* b */ REL, INY, ILL, ILL, ZPX, ZPX, ZPY, ILL, IMP, ABY, IMP, ILL, ABX, ABX, ABY, ILL,
/* c */ IMM, INX, ILL, ILL, ZPG, ZPG, ZPG, ILL, IMP, IMM, IMP, ILL, ABS, ABS, ABS, ILL,
/* d */ REL, INY, ILL, ILL, ILL, ZPX, ZPX, ILL, IMP, ABY, ILL, ILL, ILL, ABX, ABX, ILL,
/* e */ IMM, INX, ILL, ILL, ZPG, ZPG, ZPG, ILL, IMP, IMM, IMP, ILL, ABS, ABS, ABS, ILL,
/* f */ REL, INY, ILL, ILL, ILL, ZPX, ZPX, ILL, IMP, ABY, ILL, ILL, ILL, ABX, ABX, ILL
};


int d6502_disassemble(char* buff, unsigned int pc)
{
   unsigned int instruction = d6502_read_instruction(pc) & 0xff;
   pc++;

   strcpy(buff, d6502i_instruction_tbl[instruction]);
   buff += 3;

   switch(d6502i_addressing_tbl[instruction])
   {
      case IMM: sprintf(buff, " #$%02X",    d6502_read_imm(pc));             break;
      case ZPG: sprintf(buff, " $%02X",     d6502_read_imm(pc));             break;
      case ABS: sprintf(buff, " $%04X",     d6502_read_imm_16(pc));          break;
      case IND: sprintf(buff, " ($%04X)",   d6502_read_imm_16(pc));          break;
      case ABX: sprintf(buff, " $%04X,X",   d6502_read_imm_16(pc));          break;
      case ABY: sprintf(buff, " $%04X,Y",   d6502_read_imm_16(pc));          break;
      case ZPX: sprintf(buff, " $%02X,X",   d6502_read_imm(pc));             break;
      case ZPY: sprintf(buff, " $%02X,Y",   d6502_read_imm(pc));             break;
      case INX: sprintf(buff, " ($%02X,X)", d6502_read_imm(pc));             break;
      case INY: sprintf(buff, " ($%02X),Y", d6502_read_imm(pc));             break;
      case REL: sprintf(buff, " %04X",    OFFSET(pc+1, d6502_read_imm(pc))); break;
      case ACC: sprintf(buff, " A");                                         break;
   }
   return d6502i_instruction_size_tbl[d6502i_addressing_tbl[instruction]];
}

int d6502_bytecode(char* buff, unsigned int pc)
{
   unsigned int instruction = d6502_read_instruction(pc) & 0xff;
   pc++;

   sprintf(buff, "%02X", instruction);
   buff += 2;

   switch(d6502i_addressing_tbl[instruction])
   {
      case IMM: case ZPG: case ZPX: case ZPY: case INX: case INY: case REL:
         sprintf(buff, " %02X", d6502_read_imm(pc));
         break;
      case ABS: case IND: case ABX: case ABY:
         sprintf(buff, " %02X%02X", d6502_read_imm(pc), d6502_read_imm(pc+1));
         break;
   }
   return d6502i_instruction_size_tbl[d6502i_addressing_tbl[instruction]];
}









static char* d65c02i_instruction_tbl[256] =
{
/*        0      1      2      3      4      5      6      7      8      9      A      B      C      D      E      F */
/* 0 */ "BRK", "ORA", "$02", "$03", "TSB", "ORA", "ASL", "$07", "PHP", "ORA", "ASL", "$0B", "TSB", "ORA", "ASL", "BBR0",
/* 1 */ "BPL", "ORA", "ORA", "$13", "TRB", "ORA", "ASL", "$17", "CLC", "ORA", "INA", "$1B", "TRB", "ORA", "ASL", "BBR1",
/* 2 */ "JSR", "AND", "$22", "$23", "BIT", "AND", "ROL", "$27", "PLP", "AND", "ROL", "$2B", "BIT", "AND", "ROL", "BBR2",
/* 3 */ "BMI", "AND", "AND", "$33", "BIT", "AND", "ROL", "$37", "SEC", "AND", "DEA", "$3B", "BIT", "AND", "ROL", "BBR3",
/* 4 */ "RTI", "EOR", "$42", "$43", "$44", "EOR", "LSR", "$47", "PHA", "EOR", "LSR", "$4B", "JMP", "EOR", "LSR", "BBR4",
/* 5 */ "BVC", "EOR", "EOR", "$53", "$54", "EOR", "LSR", "$57", "CLI", "EOR", "PHY", "$5B", "$5C", "EOR", "LSR", "BBR5",
/* 6 */ "RTS", "ADC", "$62", "$63", "STZ", "ADC", "ROR", "$67", "PLA", "ADC", "ROR", "$6B", "JMP", "ADC", "ROR", "BBR6",
/* 7 */ "BVS", "ADC", "ADC", "$73", "STZ", "ADC", "ROR", "$77", "SEI", "ADC", "PLY", "$7B", "JMP", "ADC", "ROR", "BBR7",
/* 8 */ "BRA", "STA", "$82", "$83", "STY", "STA", "STX", "$87", "DEY", "BIT", "TXA", "$8B", "STY", "STA", "STX", "BBS0",
/* 9 */ "BCC", "STA", "STA", "$93", "STY", "STA", "STX", "$97", "TYA", "STA", "TXS", "$9B", "STZ", "STA", "STZ", "BBS1",
/* A */ "LDY", "LDA", "LDX", "$A3", "LDY", "LDA", "LDX", "$a7", "TAY", "LDA", "TAX", "$AB", "LDY", "LDA", "LDX", "BBS2",
/* B */ "BCS", "LDA", "LDA", "$B3", "LDY", "LDA", "LDX", "$b7", "CLV", "LDA", "TSX", "$BB", "LDY", "LDA", "LDX", "BBS3",
/* C */ "CPY", "CMP", "$C2", "$C3", "CPY", "CMP", "DEC", "$c7", "INY", "CMP", "DEX", "$CB", "CPY", "CMP", "DEC", "BBS4",
/* D */ "BNE", "CMP", "CMP", "$D3", "$D4", "CMP", "DEC", "$d7", "CLD", "CMP", "PHX", "$DB", "$DC", "CMP", "DEC", "BBS5",
/* E */ "CPX", "SBC", "$E2", "$E3", "CPX", "SBC", "INC", "$e7", "INX", "SBC", "NOP", "$EB", "CPX", "SBC", "INC", "BBS6",
/* F */ "BEQ", "SBC", "SBC", "$F3", "$F4", "SBC", "INC", "$f7", "SED", "SBC", "PLX", "$FB", "$FC", "SBC", "INC", "BBS7"
};

static addr_mode_t d65c02i_addressing_tbl[256] =
{
/*        0      1      2      3      4      5      6      7      8      9      a      b      c      d      e      f */
/* 0 */  IMP,   INX,   ILL,   ILL,   ZPG,   ZPG,   ZPG,   ILL,   IMP,   IMM,   ACC,   ILL,   ABS,   ABS,   ABS,   ZRE,  
/* 1 */  REL,   INY,   ZPI,   ILL,   ZPG,   ZPX,   ZPX,   ILL,   IMP,   ABY,   IMP,   ILL,   ABS,   ABX,   ABX,   ZRE,  
/* 2 */  IMP,   INX,   ILL,   ILL,   ZPG,   ZPG,   ZPG,   ILL,   IMP,   IMM,   ACC,   ILL,   ABS,   ABS,   ABS,   ZRE,  
/* 3 */  REL,   INY,   ZPI,   ILL,   ZPX,   ZPX,   ZPX,   ILL,   IMP,   ABY,   IMP,   ILL,   ABX,   ABX,   ABX,   ZRE,  
/* 4 */  IMP,   INX,   ILL,   ILL,   ILL,   ZPG,   ZPG,   ILL,   IMP,   IMM,   ACC,   ILL,   ABS,   ABS,   ABS,   ZRE,  
/* 5 */  REL,   INY,   ZPI,   ILL,   ILL,   ZPX,   ZPX,   ILL,   IMP,   ABY,   IMP,   ILL,   ILL,   ABX,   ABX,   ZRE,  
/* 6 */  IMP,   INX,   ILL,   ILL,   ZPG,   ZPG,   ZPG,   ILL,   IMP,   IMM,   ACC,   ILL,   IND,   ABS,   ABS,   ZRE,  
/* 7 */  REL,   INY,   ZPI,   ILL,   ZPX,   ZPX,   ZPX,   ILL,   IMP,   ABY,   IMP,   ILL,   IAX,   ABX,   ABX,   ZRE,  
/* 8 */  REL,   INX,   ILL,   ILL,   ZPG,   ZPG,   ZPG,   ILL,   IMP,   IMM,   IMP,   ILL,   ABS,   ABS,   ABS,   ZRE,  
/* 9 */  REL,   INY,   ZPI,   ILL,   ZPX,   ZPX,   ZPY,   ILL,   IMP,   ABY,   IMP,   ILL,   ABS,   ABX,   ABX,   ZRE,  
/* a */  IMM,   INX,   IMM,   ILL,   ZPG,   ZPG,   ZPG,   ILL,   IMP,   IMM,   IMP,   ILL,   ABS,   ABS,   ABS,   ZRE,  
/* b */  REL,   INY,   ZPI,   ILL,   ZPX,   ZPX,   ZPY,   ILL,   IMP,   ABY,   IMP,   ILL,   ABX,   ABX,   ABY,   ZRE,  
/* c */  IMM,   INX,   ILL,   ILL,   ZPG,   ZPG,   ZPG,   ILL,   IMP,   IMM,   IMP,   ILL,   ABS,   ABS,   ABS,   ZRE,  
/* d */  REL,   INY,   ZPI,   ILL,   ILL,   ZPX,   ZPX,   ILL,   IMP,   ABY,   IMP,   ILL,   ILL,   ABX,   ABX,   ZRE,  
/* e */  IMM,   INX,   ILL,   ILL,   ZPG,   ZPG,   ZPG,   ILL,   IMP,   IMM,   IMP,   ILL,   ABS,   ABS,   ABS,   ZRE,  
/* f */  REL,   INY,   ZPI,   ILL,   ILL,   ZPX,   ZPX,   ILL,   IMP,   ABY,   IMP,   ILL,   ILL,   ABX,   ABX,   ZRE
};


int d65c02_disassemble(char* buff, unsigned int pc)
{
   unsigned int instruction = d6502_read_instruction(pc) & 0xff;
   pc++;

   strcpy(buff, d65c02i_instruction_tbl[instruction]);
   buff += strlen(buff);

   switch(d65c02i_addressing_tbl[instruction])
   {
      case IMM: sprintf(buff, " #$%02X",    d6502_read_imm(pc));             break;
      case ZPG: sprintf(buff, " $%02X",     d6502_read_imm(pc));             break;
      case ABS: sprintf(buff, " $%04X",     d6502_read_imm_16(pc));          break;
      case IND: sprintf(buff, " ($%04X)",   d6502_read_imm_16(pc));          break;
      case ABX: sprintf(buff, " $%04X,X",   d6502_read_imm_16(pc));          break;
      case ABY: sprintf(buff, " $%04X,Y",   d6502_read_imm_16(pc));          break;
      case ZPX: sprintf(buff, " $%02X,X",   d6502_read_imm(pc));             break;
      case ZPY: sprintf(buff, " $%02X,Y",   d6502_read_imm(pc));             break;
      case INX: sprintf(buff, " ($%02X,X)", d6502_read_imm(pc));             break;
      case INY: sprintf(buff, " ($%02X),Y", d6502_read_imm(pc));             break;
      case REL: sprintf(buff, " %04X",    OFFSET(pc+1, d6502_read_imm(pc))); break;
      case ACC: sprintf(buff, " A");                                         break;
      case ZPI: sprintf(buff, " ($%02X)",   d6502_read_imm(pc));             break;
      case IAX: sprintf(buff, " ($%04X,X)", d6502_read_imm_16(pc));          break;
      case ZRE: sprintf(buff, " $%02X, %04X", d6502_read_imm(pc), OFFSET(pc+2, d6502_read_imm(pc+1))); break;
   }
   return d6502i_instruction_size_tbl[d6502i_addressing_tbl[instruction]];
}

int d65c02_bytecode(char* buff, unsigned int pc)
{
   unsigned int instruction = d6502_read_instruction(pc) & 0xff;
   pc++;

   sprintf(buff, "%02X", instruction);
   buff += 2;

   switch(d65c02i_addressing_tbl[instruction])
   {
      case IMM: case ZPG: case ZPX: case ZPY: case INX: case INY: case REL: case ZPI:
         sprintf(buff, " %02X", d6502_read_imm(pc));
         break;
      case ABS: case IND: case ABX: case ABY: case IAX:
         sprintf(buff, " %02X%02X", d6502_read_imm(pc), d6502_read_imm(pc+1));
         break;
      case ZRE:
         sprintf(buff, " %02X %02X", d6502_read_imm(pc), d6502_read_imm(pc+1));
         break;
   }
   return d6502i_instruction_size_tbl[d6502i_addressing_tbl[instruction]];
}



static char* d6510i_instruction_tbl[256] =
{
/*        0      1      2      3      4      5      6      7      8      9      A      B      C      D      E      F */
/* 0 */ "BRK", "ORA", "$02", "$03", "$04", "ORA", "ASL", "$07", "PHP", "ORA", "ASL", "$0B", "$0C", "ORA", "ASL", "$0F",
/* 1 */ "BPL", "ORA", "$12", "$13", "$14", "ORA", "ASL", "$17", "CLC", "ORA", "$1A", "$1B", "$1C", "ORA", "ASL", "$1F",
/* 2 */ "JSR", "AND", "$22", "$23", "BIT", "AND", "ROL", "$27", "PLP", "AND", "ROL", "$2B", "BIT", "AND", "ROL", "$2F",
/* 3 */ "BMI", "AND", "$32", "$33", "$34", "AND", "ROL", "$37", "SEC", "AND", "$3A", "$3B", "$3C", "AND", "ROL", "$3F",
/* 4 */ "RTI", "EOR", "$42", "$43", "$44", "EOR", "LSR", "$47", "PHA", "EOR", "LSR", "$4B", "JMP", "EOR", "LSR", "$4F",
/* 5 */ "BVC", "EOR", "$52", "$53", "$54", "EOR", "LSR", "$57", "CLI", "EOR", "$5A", "$5B", "$5C", "EOR", "LSR", "$5F",
/* 6 */ "RTS", "ADC", "$62", "$63", "$64", "ADC", "ROR", "$67", "PLA", "ADC", "ROR", "$6B", "JMP", "ADC", "ROR", "$6F",
/* 7 */ "BVS", "ADC", "$72", "$73", "$74", "ADC", "ROR", "$77", "SEI", "ADC", "$7A", "$7B", "$7C", "ADC", "ROR", "$7F",
/* 8 */ "$80", "STA", "$82", "$83", "STY", "STA", "STX", "$87", "DEY", "$89", "TXA", "$8B", "STY", "STA", "STX", "$8F",
/* 9 */ "BCC", "STA", "$92", "$93", "STY", "STA", "STX", "$97", "TYA", "STA", "TXS", "$9B", "$9C", "STA", "$9E", "$9F",
/* A */ "LDY", "LDA", "LDX", "$A3", "LDY", "LDA", "LDX", "$A7", "TAY", "LDA", "TAX", "$AB", "LDY", "LDA", "LDX", "$AF",
/* B */ "BCS", "LDA", "$B2", "$B3", "LDY", "LDA", "LDX", "$B7", "CLV", "LDA", "TSX", "$BB", "LDY", "LDA", "LDX", "$BF",
/* C */ "CPY", "CMP", "$C2", "$C3", "CPY", "CMP", "DEC", "$C7", "INY", "CMP", "DEX", "$CB", "CPY", "CMP", "DEC", "$CF",
/* D */ "BNE", "CMP", "$D2", "$D3", "$D4", "CMP", "DEC", "$D7", "CLD", "CMP", "$DA", "$DB", "$DC", "CMP", "DEC", "$DF",
/* E */ "CPX", "SBC", "$E2", "$E3", "CPX", "SBC", "INC", "$E7", "INX", "SBC", "NOP", "$EB", "CPX", "SBC", "INC", "$EF",
/* F */ "BEQ", "SBC", "$F2", "$F3", "$F4", "SBC", "INC", "$F7", "SED", "SBC", "$FA", "$FB", "$FC", "SBC", "INC", "$FF"
};

static addr_mode_t d6510i_addressing_tbl[256] =
{
/*       0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f */
/* 0 */ IMP, INX, ILL, ILL, ILL, ZPG, ZPG, ILL, IMP, IMM, ACC, ILL, ILL, ABS, ABS, ILL,
/* 1 */ REL, INY, ILL, ILL, ILL, ZPX, ZPX, ILL, IMP, ABY, ILL, ILL, ILL, ABX, ABX, ILL,
/* 2 */ IMP, INX, ILL, ILL, ZPG, ZPG, ZPG, ILL, IMP, IMM, ACC, ILL, ABS, ABS, ABS, ILL,
/* 3 */ REL, INY, ILL, ILL, ILL, ZPX, ZPX, ILL, IMP, ABY, ILL, ILL, ILL, ABX, ABX, ILL,
/* 4 */ IMP, INX, ILL, ILL, ILL, ZPG, ZPG, ILL, IMP, IMM, ACC, ILL, ABS, ABS, ABS, ILL,
/* 5 */ REL, INY, ILL, ILL, ILL, ZPX, ZPX, ILL, IMP, ABY, ILL, ILL, ILL, ABX, ABX, ILL,
/* 6 */ IMP, INX, ILL, ILL, ILL, ZPG, ZPG, ILL, IMP, IMM, ACC, ILL, IND, ABS, ABS, ILL,
/* 7 */ REL, INY, ILL, ILL, ILL, ZPX, ZPX, ILL, IMP, ABY, ILL, ILL, ILL, ABX, ABX, ILL,
/* 8 */ ILL, INX, ILL, ILL, ZPG, ZPG, ZPG, ILL, IMP, ILL, IMP, ILL, ABS, ABS, ABS, ILL,
/* 9 */ REL, INY, ILL, ILL, ZPX, ZPX, ZPY, ILL, IMP, ABY, IMP, ILL, ILL, ABX, ILL, ILL,
/* a */ IMM, INX, IMM, ILL, ZPG, ZPG, ZPG, ILL, IMP, IMM, IMP, ILL, ABS, ABS, ABS, ILL,
/* b */ REL, INY, ILL, ILL, ZPX, ZPX, ZPY, ILL, IMP, ABY, IMP, ILL, ABX, ABX, ABY, ILL,
/* c */ IMM, INX, ILL, ILL, ZPG, ZPG, ZPG, ILL, IMP, IMM, IMP, ILL, ABS, ABS, ABS, ILL,
/* d */ REL, INY, ILL, ILL, ILL, ZPX, ZPX, ILL, IMP, ABY, ILL, ILL, ILL, ABX, ABX, ILL,
/* e */ IMM, INX, ILL, ILL, ZPG, ZPG, ZPG, ILL, IMP, IMM, IMP, ILL, ABS, ABS, ABS, ILL,
/* f */ REL, INY, ILL, ILL, ILL, ZPX, ZPX, ILL, IMP, ABY, ILL, ILL, ILL, ABX, ABX, ILL
};


int d6510_disassemble(char* buff, unsigned int pc)
{
   unsigned int instruction = d6502_read_instruction(pc) & 0xff;
   pc++;

   strcpy(buff, d6510i_instruction_tbl[instruction]);
   buff += 3;

   switch(d6510i_addressing_tbl[instruction])
   {
      case IMM: sprintf(buff, " #$%02X",    d6502_read_imm(pc));             break;
      case ZPG: sprintf(buff, " $%02X",     d6502_read_imm(pc));             break;
      case ABS: sprintf(buff, " $%04X",     d6502_read_imm_16(pc));          break;
      case IND: sprintf(buff, " ($%04X)",   d6502_read_imm_16(pc));          break;
      case ABX: sprintf(buff, " $%04X,X",   d6502_read_imm_16(pc));          break;
      case ABY: sprintf(buff, " $%04X,Y",   d6502_read_imm_16(pc));          break;
      case ZPX: sprintf(buff, " $%02X,X",   d6502_read_imm(pc));             break;
      case ZPY: sprintf(buff, " $%02X,Y",   d6502_read_imm(pc));             break;
      case INX: sprintf(buff, " ($%02X,X)", d6502_read_imm(pc));             break;
      case INY: sprintf(buff, " ($%02X),Y", d6502_read_imm(pc));             break;
      case REL: sprintf(buff, " %04X",    OFFSET(pc+1, d6502_read_imm(pc))); break;
      case ACC: sprintf(buff, " A");                                         break;
   }
   return d6502i_instruction_size_tbl[d6510i_addressing_tbl[instruction]];
}

int d6510_bytecode(char* buff, unsigned int pc)
{
   unsigned int instruction = d6502_read_instruction(pc) & 0xff;
   pc++;

   sprintf(buff, "%02X", instruction);
   buff += 2;

   switch(d6510i_addressing_tbl[instruction])
   {
      case IMM: case ZPG: case ZPX: case ZPY: case INX: case INY: case REL:
         sprintf(buff, " %02X", d6502_read_imm(pc));
         break;
      case ABS: case IND: case ABX: case ABY:
         sprintf(buff, " %02X%02X", d6502_read_imm(pc), d6502_read_imm(pc+1));
         break;
   }
   return d6502i_instruction_size_tbl[d6510i_addressing_tbl[instruction]];
}
