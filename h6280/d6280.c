#include "d6280.h"
#include <stdio.h>
#include <string.h>


#define MAKE_INT_8(A) ((char)(A))
#define OFFSET(BASE, OFF) (((BASE) + MAKE_INT_8(OFF)) & 0xffff)

#define d6280_read_16(A) (d6280_read(A) | (d6280_read(A)<<8))

typedef enum {ILL, IMP, ACC, IMM, ZPG, ZPX, ZPY, ABS, ABX, ABY, REL, IND, INX, INY, ZPI, IAX, ZRE, IZPG, IZPX, IABS, IABX, NUM_ADDR} addr_mode_t;


static int d6280i_instruction_size_tbl[NUM_ADDR] = { 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 2, 2, 2, 3, 3, 2, 2, 3, 3 };

static char* d6280i_instruction_tbl[256] =
{
/*        0      1      2      3      4      5      6      7       8      9      A      B      C      D      E      F */
/* 0 */ "BRK", "ORA", "SXY", "ST0", "TSB", "ORA", "ASL", "RMB0", "PHP", "ORA", "ASL", "$0B", "TSB", "ORA", "ASL", "BBR0",
/* 1 */ "BPL", "ORA", "ORA", "ST1", "TRB", "ORA", "ASL", "RMB1", "CLC", "ORA", "INA", "$1B", "TRB", "ORA", "ASL", "BBR1",
/* 2 */ "JSR", "AND", "SAX", "ST2", "BIT", "AND", "ROL", "RMB2", "PLP", "AND", "ROL", "$2B", "BIT", "AND", "ROL", "BBR2",
/* 3 */ "BMI", "AND", "AND", "$33", "BIT", "AND", "ROL", "RMB3", "SEC", "AND", "DEA", "$3B", "BIT", "AND", "ROL", "BBR3",
/* 4 */ "RTI", "EOR", "SAY", "TMA", "BSR", "EOR", "LSR", "RMB4", "PHA", "EOR", "LSR", "$4B", "JMP", "EOR", "LSR", "BBR4",
/* 5 */ "BVC", "EOR", "EOR", "TAM", "CSL", "EOR", "LSR", "RMB5", "CLI", "EOR", "PHY", "$5B", "$5C", "EOR", "LSR", "BBR5",
/* 6 */ "RTS", "ADC", "CLA", "$63", "STZ", "ADC", "ROR", "RMB6", "PLA", "ADC", "ROR", "$6B", "JMP", "ADC", "ROR", "BBR6",
/* 7 */ "BVS", "ADC", "ADC", "TII", "STZ", "ADC", "ROR", "RMB7", "SEI", "ADC", "PLY", "$7B", "JMP", "ADC", "ROR", "BBR7",
/* 8 */ "BRA", "STA", "CLX", "TST", "STY", "STA", "STX", "SMB0", "DEY", "BIT", "TXA", "$8B", "STY", "STA", "STX", "BBS0",
/* 9 */ "BCC", "STA", "STA", "TST", "STY", "STA", "STX", "SMB1", "TYA", "STA", "TXS", "$9B", "STZ", "STA", "STZ", "BBS1",
/* A */ "LDY", "LDA", "LDX", "TST", "LDY", "LDA", "LDX", "SMB2", "TAY", "LDA", "TAX", "$AB", "LDY", "LDA", "LDX", "BBS2",
/* B */ "BCS", "LDA", "LDA", "TST", "LDY", "LDA", "LDX", "SMB3", "CLV", "LDA", "TSX", "$BB", "LDY", "LDA", "LDX", "BBS3",
/* C */ "CPY", "CMP", "CLY", "TDD", "CPY", "CMP", "DEC", "SMB4", "INY", "CMP", "DEX", "$CB", "CPY", "CMP", "DEC", "BBS4",
/* D */ "BNE", "CMP", "CMP", "TIN", "CSH", "CMP", "DEC", "SMB5", "CLD", "CMP", "PHX", "$DB", "$DC", "CMP", "DEC", "BBS5",
/* E */ "CPX", "SBC", "$E2", "TIA", "CPX", "SBC", "INC", "SMB6", "INX", "SBC", "NOP", "$EB", "CPX", "SBC", "INC", "BBS6",
/* F */ "BEQ", "SBC", "SBC", "TAI", "SET", "SBC", "INC", "SMB7", "SED", "SBC", "PLX", "$FB", "$FC", "SBC", "INC", "BBS7"
};

static addr_mode_t d6280i_addressing_tbl[256] =
{
/*        0      1      2      3      4      5      6      7       8      9      a      b      c      d      e      f */
/* 0 */  IMP,   INX,   IMP,   IMM,   ZPG,   ZPG,   ZPG,   ZPG,    IMP,   IMM,   ACC,   ILL,   ABS,   ABS,   ABS,   ZRE,  
/* 1 */  REL,   INY,   ZPI,   IMM,   ZPG,   ZPX,   ZPX,   ZPG,    IMP,   ABY,   IMP,   ILL,   ABS,   ABX,   ABX,   ZRE,  
/* 2 */  IMP,   INX,   IMP,   IMM,   ZPG,   ZPG,   ZPG,   ZPG,    IMP,   IMM,   ACC,   ILL,   ABS,   ABS,   ABS,   ZRE,  
/* 3 */  REL,   INY,   ZPI,   ILL,   ZPX,   ZPX,   ZPX,   ZPG,    IMP,   ABY,   IMP,   ILL,   ABX,   ABX,   ABX,   ZRE,  
/* 4 */  IMP,   INX,   IMP,   IMM,   REL,   ZPG,   ZPG,   ZPG,    IMP,   IMM,   ACC,   ILL,   ABS,   ABS,   ABS,   ZRE,  
/* 5 */  REL,   INY,   ZPI,   IMM,   IMP,   ZPX,   ZPX,   ZPG,    IMP,   ABY,   IMP,   ILL,   ILL,   ABX,   ABX,   ZRE,
/* 6 */  IMP,   INX,   IMP,   ILL,   ZPG,   ZPG,   ZPG,   ZPG,    IMP,   IMM,   ACC,   ILL,   IND,   ABS,   ABS,   ZRE,
/* 7 */  REL,   INY,   ZPI,   IMM,   ZPX,   ZPX,   ZPX,   ZPG,    IMP,   ABY,   IMP,   ILL,   IAX,   ABX,   ABX,   ZRE,
/* 8 */  REL,   INX,   IMP,   IZPG,  ZPG,   ZPG,   ZPG,   ZPG,    IMP,   IMM,   IMP,   ILL,   ABS,   ABS,   ABS,   ZRE,
/* 9 */  REL,   INY,   ZPI,   IABS,  ZPX,   ZPX,   ZPY,   ZPG,    IMP,   ABY,   IMP,   ILL,   ABS,   ABX,   ABX,   ZRE,
/* a */  IMM,   INX,   IMM,   IZPX,  ZPG,   ZPG,   ZPG,   ZPG,    IMP,   IMM,   IMP,   ILL,   ABS,   ABS,   ABS,   ZRE,
/* b */  REL,   INY,   ZPI,   IABX,  ZPX,   ZPX,   ZPY,   ZPG,    IMP,   ABY,   IMP,   ILL,   ABX,   ABX,   ABY,   ZRE,
/* c */  IMM,   INX,   IMP,   IMM,   ZPG,   ZPG,   ZPG,   ZPG,    IMP,   IMM,   IMP,   ILL,   ABS,   ABS,   ABS,   ZRE,
/* d */  REL,   INY,   ZPI,   IMM,   IMP,   ZPX,   ZPX,   ZPG,    IMP,   ABY,   IMP,   ILL,   ILL,   ABX,   ABX,   ZRE,
/* e */  IMM,   INX,   ILL,   IMM,   ZPG,   ZPG,   ZPG,   ZPG,    IMP,   IMM,   IMP,   ILL,   ABS,   ABS,   ABS,   ZRE,
/* f */  REL,   INY,   ZPI,   IMM,   IMP,   ZPX,   ZPX,   ILL,    IMP,   ABY,   IMP,   ILL,   ILL,   ABX,   ABX,   ZRE
};


int d6280_disassemble(char* buff, int pc)
{
   unsigned int instruction = d6280_read(pc);
   pc++;

   strcpy(buff, d6280i_instruction_tbl[instruction]);
   buff += strlen(buff);

   switch(d6280i_addressing_tbl[instruction])
   {
      case IMM: sprintf(buff, " #$%02X",          d6280_read(pc));                      break;
      case ZPG: sprintf(buff, " $%02X",           d6280_read(pc));                      break;
      case ABS: sprintf(buff, " $%04X",           d6280_read_16(pc));                   break;
      case IND: sprintf(buff, " ($%04X)",         d6280_read_16(pc));                   break;
      case ABX: sprintf(buff, " $%04X,X",         d6280_read_16(pc));                   break;
      case ABY: sprintf(buff, " $%04X,Y",         d6280_read_16(pc));                   break;
      case ZPX: sprintf(buff, " $%02X,X",         d6280_read(pc));                      break;
      case ZPY: sprintf(buff, " $%02X,Y",         d6280_read(pc));                      break;
      case INX: sprintf(buff, " ($%02X,X)",       d6280_read(pc));                      break;
      case INY: sprintf(buff, " ($%02X),Y",       d6280_read(pc));                      break;
      case REL: sprintf(buff, " %04X",            OFFSET(pc+1, d6280_read(pc)));        break;
      case ACC: sprintf(buff, " A");                                                    break;
      case ZPI: sprintf(buff, " ($%02X)",         d6280_read(pc));                      break;
      case IAX: sprintf(buff, " ($%04X,X)",       d6280_read_16(pc));                   break;
      case ZRE: sprintf(buff, " $%02X, $%04X",    d6280_read(pc), OFFSET(pc+2, d6280_read(pc+1))); break;
      case IZPG: sprintf(buff, " $%02X, $%02X",   d6280_read(pc), d6280_read(pc+1));    break;
      case IZPX: sprintf(buff, " $%02X, $%02X,X", d6280_read(pc), d6280_read(pc+1));    break;
      case IABS: sprintf(buff, " $%02X, $%04X",   d6280_read(pc), d6280_read_16(pc+1)); break;
      case IABX: sprintf(buff, " $%02X, $%04X,X", d6280_read(pc), d6280_read_16(pc+1)); break;
   }
   return d6280i_instruction_size_tbl[d6280i_addressing_tbl[instruction]];
}

int d6280_bytecode(char* buff, int pc)
{
   unsigned int instruction = d6280_read(pc);
   pc++;

   sprintf(buff, "%02X", instruction);
   buff += 2;

   switch(d6280i_addressing_tbl[instruction])
   {
      case IMM: case ZPG: case ZPX: case ZPY: case INX: case INY: case REL: case ZPI:
         sprintf(buff, " %02X", d6280_read(pc));
         break;
      case ABS: case IND: case ABX: case ABY: case IAX: case ZRE: case IZPG: case IZPX:
         sprintf(buff, " %02X %02X", d6280_read(pc), d6280_read(pc+1));
         break;
      case IABS: case IABX:
         sprintf(buff, " %02X %02X %02X", d6280_read(pc), d6280_read(pc+1), d6280_read(pc+2));
         break;
   }
   return d6280i_instruction_size_tbl[d6280i_addressing_tbl[instruction]];
}
