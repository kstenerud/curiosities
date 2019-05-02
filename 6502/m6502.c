#define ENTRIES 32

/* ======================================================================== */
/* =============================== COPYRIGHT ============================== */
/* ======================================================================== */
/*

M6502, M65C02, M6510, N2A03 emulator
All rights reserved.

This work is free for non-commercial use.
You must report any significant changes to the author.


*/
/* ======================================================================== */
/* ================================= NOTES ================================ */
/* ======================================================================== */
/*

Regarding the macros:
	All of the memory access macros are designed to be atomic, and so will have
	no side-effects if the external memory access functions are implemented
	by the host as macros.

	As a result of this, macro calls such as EA_xxx(), OPER_8_xxx(), and PULL()
	will modify the calling parameter (you must call them with an lvalue).

	SET_REG_P() should only be called with a single-term parameter, since it
	will read that parameter multiple times.


Regarding the flag variables:

	In order to speed up emulation, the flags are stored in separate variables,
	and are used in the operations to store values during operation execution
	(this saves on extra copies to the flags).  As a result, only certain bits
	of the flag variables are useful:

	Variable  Most common use                                 Flag set when
	--------  ---------------------------------------------   -------------
	 FLAG_N   assigned the result of the operation            Bit 7 is set
	 FLAG_V   set using VFLAG_ADD() or VFLAG_SUB()            Bit 7 is set
	 FLAG_R   RESERVED                                        RESERVED
	 FLAG_B   not used as a temp variable                     FLAG_B = 1
	 FLAG_D   not used as a temp variable                     FLAG_D = 1
	 FLAG_I   not used as a temp variable                     FLAG_I = 1
	 FLAG_Z   assigned the result of the operation            FLAG_Z = 0
	 FLAG_C   assigned the result of the operation            Bit 8 is set

	(see GET_REG_P(), SET_REG_P() and the condition code macros).


*/
/* ======================================================================== */
/* ================================ INCLUDES ============================== */
/* ======================================================================== */

#include <limits.h>
#include "driver.h"
#include "state.h"
#include "mamedbg.h"
#include "m6502.h"
#include <stdio.h>


extern unsigned Dasm6502(char *buffer, unsigned pc);

/* ======================================================================== */
/* ==================== ARCHITECTURE-DEPENDANT DEFINES ==================== */
/* ======================================================================== */

/* Fallback on static if we don't have inline */
#ifndef INLINE
#define INLINE static
#endif

/* This should be set to the default size of your processor (min 16 bit) */
#undef uint
#define uint unsigned int

#undef uint8
#define uint8 unsigned char

#undef int8

/* Allow for architectures that don't have 8-bit sizes */
#if UCHAR_MAX == 0xff
#define int8 char
#define MAKE_INT_8(A) (int8)((A)&0xff)
#else
#define int8   int
INLINE int MAKE_INT_8(int A) {return (A & 0x80) ? A | ~0xff : A & 0xff;}
#endif /* UCHAR_MAX == 0xff */

#define MAKE_UINT_8(A) ((A)&0xff)
#define MAKE_UINT_16(A) ((A)&0xffff)



/* ======================================================================== */
/* =============================== PROTOTYPES ============================= */
/* ======================================================================== */

/* CPU Structure */
typedef struct
{
	uint a;			/* Accumulator */
	uint x;			/* Index Register X */
	uint y;			/* Index Register Y */
	uint s;			/* Stack Pointer */
	uint pc;		/* Program Counter */
	uint ppc;		/* Previous Program Counter */
	uint flag_n;	/* Negative Flag */
	uint flag_v;	/* Overflow Flag */
	uint flag_b;	/* BRK Instruction Flag */
	uint flag_d;	/* Decimal Mode Flag */
	uint flag_i;	/* Interrupt Mask Flag */
	uint flag_z;	/* Zero Flag (inverted) */
	uint flag_c;	/* Carry Flag */
	uint line_irq;	/* Status of the IRQ line */
	uint line_nmi;	/* Status of the NMI line */
	uint line_rst;	/* Status of the RESET line */
	uint ir;		/* Instruction Register */
	int  irq_delay;	/* delay 1 instruction before checking irq */
	int (*int_ack)(int); /* Interrupt Acknowledge */
} m6502i_cpu_struct;



/* ======================================================================== */
/* ================================= DATA ================================= */
/* ======================================================================== */

/* Our CPU structure */
static m6502i_cpu_struct m6502i_cpu = {0};

int m65xx_ICount = 0;

/* Temporary Variables */
static uint m6502i_source;
static uint m6502i_destination;
static uint m6502i_old_c_flag;

#ifdef USING_MAME

/* Layout of the registers in the MAME debugger */
static unsigned char m65xx_register_layout[] =
{
	M6502_PC, M6502_S, M6502_P, M6502_A, M6502_X, M6502_Y, -1,
	M6502_NMI_STATE, M6502_IRQ_STATE, 0
};

/* Layout of the MAME debugger windows x,y,w,h */
static unsigned char m65xx_window_layout[] = {
	25, 0,55, 2, /* register window (top, right rows) */
	 0, 0,24,22, /* disassembler window (left colums) */
	25, 3,55, 9, /* memory #1 window (right, upper middle) */
	25,13,55, 9, /* memory #2 window (right, lower middle) */
	 0,23,80, 1, /* command line window (bottom rows) */
};

#endif






/* ======================================================================== */
/* ============================ GENERAL DEFINES =========================== */
/* ======================================================================== */

/* Bits */
#define BIT_0		0x01
#define BIT_1		0x02
#define BIT_2		0x04
#define BIT_3		0x08
#define BIT_4		0x10
#define BIT_5		0x20
#define BIT_6		0x40
#define BIT_7		0x80
#define BIT_8		0x100

/* Flag positions in Processor Status Register */
#define FLAGPOS_N	BIT_7				/* Negative         */
#define FLAGPOS_V	BIT_6				/* Overflow         */
#define FLAGPOS_R	BIT_5				/* Reserved         */
#define FLAGPOS_B	BIT_4				/* BRK Instruction  */
#define FLAGPOS_D	BIT_3				/* Decimal Mode     */
#define FLAGPOS_I	BIT_2				/* Interrupt Mask   */
#define FLAGPOS_Z	BIT_1				/* Zero             */
#define FLAGPOS_C	BIT_0				/* Carry            */

#define NFLAG_SET	FLAGPOS_N
#define VFLAG_SET	BIT_7
#define RFLAG_SET	FLAGPOS_R
#define BFLAG_SET	FLAGPOS_B
#define DFLAG_SET	FLAGPOS_D
#define IFLAG_SET	FLAGPOS_I
#define ZFLAG_SET	0
#define CFLAG_SET	BIT_8
#define NFLAG_CLEAR	0
#define VFLAG_CLEAR	0
#define RFLAG_CLEAR	0
#define BFLAG_CLEAR	0
#define DFLAG_CLEAR	0
#define IFLAG_CLEAR	0
#define ZFLAG_CLEAR	1
#define CFLAG_CLEAR 0

#define NMI_SET		1
#define NMI_CLEAR	0
#define IRQ_SET		IFLAG_CLEAR
#define IRQ_CLEAR	IFLAG_SET
#define IRQ_DELAY_SET	IFLAG_CLEAR
#define IRQ_DELAY_CLEAR	IFLAG_SET

#define STACK_PAGE	0x100				/* Stack Page Offset */

#define VECTOR_IRQ	0xfffe				/* Interrupt Request */
#define VECTOR_BRK	0xfffe				/* Break Instruction */
#define VECTOR_RST	0xfffc				/* Reset */
#define VECTOR_NMI	0xfffa				/* Non-Maskable Interrupt */

#define REG_A		m6502i_cpu.a		/* Accumulator */
#define REG_X		m6502i_cpu.x		/* Index X Register */
#define REG_Y		m6502i_cpu.y		/* Index Y Register */
#define REG_S		m6502i_cpu.s		/* Stack Pointer */
#define REG_PC		m6502i_cpu.pc		/* Program Counter */
#define REG_PPC		m6502i_cpu.ppc		/* Previous Program Counter */
#define REG_P		m6502i_cpu.p		/* Processor Status Register */
#define FLAG_N		m6502i_cpu.flag_n	/* Negative Flag */
#define FLAG_V		m6502i_cpu.flag_v	/* Overflow Flag */
#define FLAG_B		m6502i_cpu.flag_b	/* BRK Instruction Flag */
#define FLAG_D		m6502i_cpu.flag_d	/* Decimal Mode Flag */
#define FLAG_I		m6502i_cpu.flag_i	/* Interrupt Mask Flag */
#define FLAG_Z		m6502i_cpu.flag_z	/* Zero Flag (inverted) */
#define FLAG_C		m6502i_cpu.flag_c	/* Carry Flag */
#define LINE_IRQ	m6502i_cpu.line_irq	/* Status of the IRQ line */
#define LINE_NMI	m6502i_cpu.line_nmi	/* Status of the NMI line */
#define LINE_RST	m6502i_cpu.line_rst	/* Status of the RESET line */
#define REG_IR		m6502i_cpu.ir		/* Instruction Register */
#define INT_ACK		m6502i_cpu.int_ack	/* Interrupt Acknowledge function pointer */
#define CLOCKS		m65xx_ICount		/* Clock cycles remaining */
#define IRQ_DELAY	m6502i_cpu.irq_delay /* Delay 1 instruction before checking IRQ */

#define SRC			m6502i_source		/* Source Operand */
#define DST			m6502i_destination	/* Destination Operand */
#define OLD_C		m6502i_old_c_flag	/* Old C Flag value (used by SBC) */



/* ======================================================================== */
/* ============================ GENERAL MACROS ============================ */
/* ======================================================================== */

/* Codition code tests */
#define COND_CC()	(!(FLAG_C&0x100))	/* Carry Clear */
#define COND_CS()	(FLAG_C&0x100)		/* Carry Set */
#define COND_EQ()	(!FLAG_Z)			/* Equal */
#define COND_NE()	FLAG_Z				/* Not Equal */
#define COND_MI()	(FLAG_N&0x80)		/* Minus */
#define COND_PL()	(!(FLAG_N&0x80))	/* Plus */
#define COND_VC()	(!(FLAG_V&0x80))	/* Overflow Clear */
#define COND_VS()	(FLAG_V&0x80)		/* Overflow Set */

/* Set Overflow flag in math operations */
#define VFLAG_ADD(S, D, R) ((S^R) & (D^R))
#define VFLAG_SUB(S, D, R) ((S^D) & (R^D))

#define CFLAG_AS_1() ((FLAG_C>>8)&1)


/* ======================================================================== */
/* ============================ UTILITY MACROS ============================ */
/* ======================================================================== */

/* Use up clock cycles */
#define CLK(A) CLOCKS -= (A)
#define USE_ALL_CLKS() CLOCKS = 0


INLINE uint read_8_normal(uint address)
{
	address = MAKE_UINT_16(address);
	return g65816_read_8(address);
}

INLINE uint read_8_immediate(uint address)
{
	address = MAKE_UINT_16(address);
	return g65816_read_8_immediate(address);
}

INLINE uint read_8_zeropage(uint address)
{
	address = MAKE_UINT_8(address);
	return g65816_read_8_zeropage(address);
}

INLINE void write_8_normal(uint address, uint value)
{
	address = MAKE_UINT_16(address);
	value = MAKE_UINT_8(value);
	g65816_write_8(address, value);
}

INLINE void write_8_direct(uint address, uint value)
{
	address = MAKE_UINT_8(address);
	value = MAKE_UINT_8(value);
	g65816_write_8_zeropage(address, value);
}


INLINE uint read_16_normal(uint address)
{
	return read_8_normal(address) | (read_8_normal(address+1)<<8);
}

INLINE uint read_16_immediate(uint address)
{
	return read_8_immediate(address) | (read_8_immediate(address+1)<<8);
}

INLINE uint read_16_zeropage(uint address)
{
	return read_8_zeropage(address) | (read_8_zeropage(address+1)<<8);
}

INLINE uint read_16_absolute(uint address)
{
	return read_8_normal(address) | (read_8_normal(((address+1) & 0xff) | (address & 0xff00))<<8);
}


/* Low level memory access macros */
#define read_8_NORM(A)		read_8_normal(A)
#define read_8_IMM(A)		read_8_immediate(A)
#define read_8_ZPG(A)		read_8_zeropage(A)
#define read_8_ABS(A)		read_8_normal(A)
#define read_8_ZPX(A)		read_8_zeropage(A)
#define read_8_ZPY(A)		read_8_zeropage(A)
#define read_8_ABX(A)		read_8_normal(A)
#define read_8_ABX_P(A)		read_8_normal(A)
#define read_8_ABY(A)		read_8_normal(A)
#define read_8_ABY_P(A)		read_8_normal(A)
#define read_8_IND(A)		read_8_normal(A)
#define read_8_IDX(A)		read_8_normal(A)
#define read_8_IDY(A)		read_8_normal(A)
#define read_8_IDY_P(A)		read_8_normal(A)
#define read_8_ZPI(A)		read_8_normal(A)
#define read_8_STK(A)		read_8_normal(A)

#define read_16_NORM(A)		read_16_normal(A)
#define read_16_IMM(A)		read_16_immediate(A)
#define read_16_ABS(A)		read_16_absolute(A)
#define read_16_ZPG(A)		read_16_zeropage(A)
#define read_16_ABX(A)		read_16_normal(A)
#define read_16_VEC(A)		read_16_normal(A)

#define write_8_NORM(A, V)	write_8_normal(A, V)
#define write_8_IMM(A, V)	write_8_normal(A, V)
#define write_8_ZPG(A, V)	write_8_zeropage(A, V)
#define write_8_ABS(A, V)	write_8_normal(A, V)
#define write_8_ZPX(A, V)	write_8_zeropage(A, V)
#define write_8_ZPY(A, V)	write_8_zeropage(A, V)
#define write_8_ABX(A, V)	write_8_normal(A, V)
#define write_8_ABY(A, V)	write_8_normal(A, V)
#define write_8_IND(A, V)	write_8_normal(A, V)
#define write_8_IDX(A, V)	write_8_normal(A, V)
#define write_8_IDY(A, V)	write_8_normal(A, V)
#define write_8_ZPI(A, V)	write_8_normal(A, V)
#define write_8_STK(A, V)	write_8_normal(A, V)


#define OPER_8_IMM()	read_8_IMM(EA_IMM())
#define OPER_16_IMM()	read_16_IMM(EA_IMM16())
#define OPER_8_ZPG()	read_8_ZPG(EA_ZPG())
#define OPER_8_ABS()	read_8_ABS(EA_ABS())
#define OPER_8_ZPX()	read_8_ZPX(EA_ZPX())
#define OPER_8_ZPY()	read_8_ZPY(EA_ZPY())
#define OPER_8_ABX()	read_8_ABX(EA_ABX())
#define OPER_8_ABY()	read_8_ABY(EA_ABY())
#define OPER_8_IND()	read_8_IND(EA_IND())
#define OPER_8_IDX()	read_8_IDX(EA_IDX())
#define OPER_8_IDY()	read_8_IDY(EA_IDY())
#define OPER_8_ZPI()		read_8_ZPI(EA_ZPI())
#define OPER_8_TFL()		read_8_TFL(EA_TFL())

/* Effective Address Caluclations */
INLINE uint EA_IMM(void)   {return REG_PC++;}
INLINE uint EA_IMM16(void) {REG_PC += 2; return REG_PC-2;}
INLINE uint EA_ZPG(void)   {return OPER_8_IMM();}
INLINE uint EA_ABS(void)   {return OPER_16_IMM();}
INLINE uint EA_ABX(void)   {return EA_ABS() + REG_X;}
INLINE uint EA_ABY(void)   {return EA_ABS() + REG_Y;}
INLINE uint EA_ZPX(void)   {return (EA_ZPG() + REG_X)&0xff;}
INLINE uint EA_ZPY(void)   {return (EA_ZPG() + REG_Y)&0xff;}
INLINE uint EA_IND(void)   {return OPER_16_ABS();}
INLINE uint EA_IDX(void)   {return OPER_16_ZPX();}
INLINE uint EA_IDY(void)   {uint addr = OPER_16_ZPG(); if(addr&0xff00 != (addr+REG_Y)&0xff00) CLK(1); return addr + REG_Y;}
INLINE uint EA_ZPI(void)   {return OPER_16_ZPG();}
INLINE uint EA_IAX(void)   {return OPER_16_ABX();}

/* Change the Program Counter */
INLINE void JUMP(uint address)
{
	REG_PC = address;
	m65xx_jumping(REG_PC);
}

INLINE void BRANCH(uint offset)
{
	uint old_pc = REG_PC;
	REG_PC = MAKE_UINT_16(REG_PC + MAKE_INT_8(offset));
	if((REG_PC^old_pc)&0xff00)
		CLK(1);
	m65xx_branching(REG_PC);
}

/* Get the Processor Status Register */
#define GET_REG_P()				\
	((FLAG_N & 0x80)		|	\
	((FLAG_V & 0x80) >> 1)	|	\
	FLAGPOS_R				|	\
	FLAG_B					|	\
	FLAG_D					|	\
	FLAG_I					|	\
	((!FLAG_Z) << 1)		|	\
	CFLAG_AS_1()

/* Get Processor Status Register with B flag set (when executing BRK instruction) */
#define GET_REG_P_BRK()			\
	((FLAG_N & 0x80)		|	\
	((FLAG_V & 0x80) >> 1)	|	\
	FLAGPOS_R				|	\
	FLAGPOS_B				|	\
	FLAG_D					|	\
	FLAG_I					|	\
	((!FLAG_Z) << 1)		|	\
	CFLAG_AS_1()

/* Get Processor Status Register with B flag cleared (when servicing an interrupt) */
#define GET_REG_P_INT()			\
	((FLAG_N & 0x80)		|	\
	((FLAG_V & 0x80) >> 1)	|	\
	FLAGPOS_R				|	\
	FLAG_D					|	\
	FLAG_I					|	\
	((!FLAG_Z) << 1)		|	\
	CFLAG_AS_1()

/* Set the Process Status Register */
INLINE void SET_REG_P(uint value)
{
	FLAG_N = value;
	FLAG_V = (value<<1);
	FLAG_B = value & FLAGPOS_B;
	FLAG_D = value & FLAGPOS_D;
	FLAG_I = value & FLAGPOS_I;
	FLAG_Z = !(value & 2);
	FLAG_C = value << 8;
}

/* Push/Pull data to/from the stack */
INLINE void PUSH_8(uint value)
{
	write_8_STK(REG_S+STACK_PAGE, value);
	REG_S = MAKE_UINT_8(REG_S - 1);
}

INLINE uint PULL_8()
{
	REG_S = MAKE_UINT_8(REG_S + 1);
	return read_8_STK(REG_S+STACK_PAGE);
}

INLINE void PUSH_16(uint value)
{
	PUSH_8(value>>8);
	PUSH_8(value);
}

INLINE uint PULL_16()
{
	uint value = PULL_8();
	return value | (PULL_8()<<8);
}


INLINE void SERVICE_IRQ()
{
	CLK(7);
	PUSH_16(REG_PC);
	PUSH_8(GET_REG_P_INT());
	FLAG_I = IFLAG_SET;
	FLAG_D = DFLAG_CLEAR;
	if(INT_ACK)
		INT_ACK(0);
	JUMP(read_16_VEC(VECTOR_IRQ));
}

/* compare I flag and inverted IRQ line and inverted IRQ delay status */
#define INT_PENDING_ICHANGED() (FLAG_I & LINE_IRQ & IRQ_DELAY)
#define INT_PENDING() (FLAG_I & LINE_IRQ)

INLINE void CHECK_IRQ()
{
	if(INT_PENDING_ICHANGED())
		SERVICE_IRQ();
}

/* ======================================================================== */
/* =========================== OPERATION MACROS =========================== */
/* ======================================================================== */

/* M6502   Add With Carry */
#if M6502_USE_NEW_ADC_SBC
#define OP_ADC(MODE)		SRC = OPER_8_##MODE();											\
							if(FLAG_D)														\
							{																\
								FLAG_C = CFLAG_AS_1();										\
								FLAG_N = REG_A + SRC + FLAG_C;								\
								FLAG_Z = MAKE_UINT_8(FLAG_N);								\
								if(((REG_A&0x0f) + (SRC&0x0f) + FLAG_C) > 9) FLAG_N += 6;	\
								FLAG_V = VFLAG_ADD(SRC, REG_A, FLAG_N);						\
								REG_A = FLAG_N;												\
								FLAG_C = (REG_A > 0x99)<<8;									\
								if(FLAG_C) REG_A -= 0xa0;									\
								break;														\
							}																\
							FLAG_C = REG_A + SRC + CFLAG_AS_1();							\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);							\
							FLAG_V = VFLAG_ADD(SRC, REG_A, FLAG_Z);							\
							REG_A = FLAG_Z
#else
#define OP_ADC(MODE)		SRC = OPER_8_##MODE();											\
							if(FLAG_D)														\
							{																\
								FLAG_Z = (SRC&0x0f) + (REG_A&0x0f) + CFLAG_AS_1());			\
								FLAG_N = (SRC&0xf0) + (REG_A&0xf0);							\
								if(FLAG_Z > 0x09)											\
								{															\
									FLAG_Z += 0x06;											\
									FLAG_N += 0x10;											\
								}															\
								FLAG_V = VFLAG_ADD(SRC, REG_A, FLAG_N);						\
								if((FLAG_C = (FLAG_N > 0x90)<<8))							\
									FLAG_N += 0x60;											\
								REG_A = FLAG_N = FLAG_Z = (FLAG_N&0xf0) | (FLAG_Z&0x0f);	\
								break;														\
							}																\
							FLAG_C = REG_A + SRC + CFLAG_AS_1();							\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);							\
							FLAG_V = VFLAG_ADD(SRC, REG_A, FLAG_Z);							\
							REG_A = FLAG_Z
#endif

/* N2a03   Add With Carry, no Decimal mode */
#define OP_ADC_ND(MODE)		SRC = OPER_8_##MODE();							\
							FLAG_C = REG_A + SRC + CFLAG_AS_1();			\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);			\
							FLAG_V = VFLAG_ADD(SRC, REG_A, FLAG_Z);			\
							REG_A = FLAG_Z

/* M6510   Logical AND to accumulator setting Carry from result */
#define OP_ANC(MODE)		REG_A &= OPER_8_##MODE();							\
							FLAG_N = FLAG_Z = REG_A;						\
							FLAG_C = REG_A << 1

/* M6502   Logical AND with accumulator */
#define OP_AND(MODE)		REG_A &= OPER_8_##MODE();							\
							FLAG_N = FLAG_Z = REG_A

/* M6502   Arithmetic Shift Left accumulator */
#define OP_ASL_A()			FLAG_C = REG_A << 1;							\
							FLAG_N = FLAG_Z = REG_A = MAKE_UINT_8(FLAG_C)

/* M6502   Arithmetic Shift Left operand */
#define OP_ASL_M(MODE)		DST = EA_##MODE();								\
							FLAG_C = read_8_##MODE(DST) << 1;				\
							FLAG_N = FLAG_Z = MAKE_UINT_8(FLAG_C);			\
							write_8_##MODE(DST, FLAG_Z)

/* M6510   Logical AND, then Rotate Right */
#define OP_ARR(MODE)		DST = EA_##MODE();											\
							FLAG_Z = (read_8_##MODE(DST) & REG_A) | (FLAG_C & 0x100);	\
							FLAG_C = FLAG_Z << 8;										\
							FLAG_Z >>= 1;												\
							FLAG_N = FLAG_Z;											\
							write_8_##MODE(DST, FLAG_Z)

/* M6510   Logical AND, then Logical Shift Left */
#define OP_ASR(MODE)		DST = EA_##MODE();								\
							FLAG_N = 0;										\
							FLAG_C = FLAG_Z = read_8_##MODE(DST) & REG_A;	\
							FLAG_C <<= 8;									\
							FLAG_Z >>= 1;									\
							write_8_##MODE(DST, FLAG_Z)

/* M6510   Logical AND to stack and transfer result to accumulator and X */
#define OP_AST(MODE)		FLAG_N = FLAG_Z = REG_A = REG_X = REG_S &= OPER_8_##MODE()

/* M6510   Logical AND accumulator to X, then subtract operand from X */
#define OP_ASX(MODE)		REG_X &= REG_A;									\
							FLAG_N = FLAG_Z = FLAG_C = REG_X - OPER_8_##MODE()

/* M6510   ??? Incorrect */
#define OP_AXA(MODE)		DST = EA_##MODE();								\
							FLAG_N = FLAG_Z = read_8_##MODE(DST) & REG_X;	\
							write_8_##MODE(DST, FLAG_Z)

/* M65C02  Branch if Bit Reset */
#define OP_BBR(BIT)			SRC = OPER_8_ZPG();								\
							DST = OPER_8_IMM();								\
							if(!(SRC & BIT))								\
							{												\
								CLK(1);										\
								BRANCH(DST);								\
							}

/* M65C02  Branch if Bit Set */
#define OP_BBS(BIT)			SRC = OPER_8_ZPG();								\
							DST = OPER_8_IMM();								\
							if(SRC & BIT)									\
							{												\
								CLK(1);										\
								BRANCH(DST);								\
							}

/* M6502   Branch on Condition Code */
#define OP_BCC(COND)		DST = EA_IMM();									\
							if(COND)										\
							{												\
								CLK(1);										\
								BRANCH(read_8_IMM(DST));					\
							}

/* M6502   Set flags according to bits */
#define OP_BIT(MODE)		FLAG_N = OPER_8_##MODE();						\
							FLAG_Z = FLAG_N & REG_A;						\
							FLAG_V = FLAG_N >> 1

/* M65C02  Branch Unconditional */
/* If we're in a busy loop and there's no interrupt, eat all clock cycles */
#define OP_BRA()			BRANCH(OPER_8_IMM());							\
							if(REG_PC == REG_PPC && !INT_PENDING())			\
								USE_ALL_CLKS()

/* M6502   Cause a Break interrupt */
/* Unusual behavior: Pushes opcode+2, not opcode+1   */
#define OP_BRK()			PUSH_16(REG_PC+1);								\
							PUSH_8(GET_REG_P_BRK());						\
							FLAG_I = IFLAG_SET;								\
							FLAG_D = DFLAG_CLEAR;							\
							JUMP(read_16_VEC(VECTOR_BRK))

/* M6502   Clear Carry flag */
#define OP_CLC()			FLAG_C = CFLAG_CLEAR

/* M6502   Clear Decimal flag */
#define OP_CLD()			FLAG_D = DFLAG_CLEAR

/* M6502   Clear Interrupt Mask flag */
#define OP_CLI()			if(FLAG_I == IFLAG_SET)							\
								IRQ_DELAY = IRQ_DELAY_SET;					\
							FLAG_I = IFLAG_CLEAR

/* M6502   Clear oVerflow flag */
#define OP_CLV()			FLAG_V = VFLAG_CLEAR

/* M6502   Compare operand to register */
/* Unusual behavior: C flag is inverted */
#define OP_CMP(REG, MODE)	SRC = OPER_8_##MODE();							\
							FLAG_C = REG - SRC;								\
							FLAG_N = FLAG_Z = MAKE_UINT_8(FLAG_C);			\
							FLAG_C ^= 0x100

/* M6510   Decrement and Compare */
/* Unusual behavior: C flag is inverted */
#define OP_DCP(MODE)		DST = EA_##MODE();								\
							SRC = MAKE_UINT_8(read_8_##MODE(DST) - 1);		\
							FLAG_C = REG_A - SRC;							\
							FLAG_N = FLAG_Z = MAKE_UINT_8(FLAG_C);			\
							FLAG_C ^= 0x100;								\
							write_8_##MODE(DST, SRC)

/* M6502   Decrement operand */
#define OP_DEC_M(MODE)		DST = EA_##MODE();										\
							FLAG_N = FLAG_Z = MAKE_UINT_8(read_8_##MODE(DST) - 1);	\
							write_8_##MODE(DST, FLAG_Z)

/* M6502   Decrement register */
#define OP_DEC_R(REG)		FLAG_N = FLAG_Z = REG = MAKE_UINT_8(REG - 1)

/* M6510   Double No Operation */
#define OP_DOP()			REG_PC++

/* M6502   Exclusive Or operand to accumulator */
#define OP_EOR(MODE)		FLAG_N = FLAG_Z = REG_A ^= OPER_8_##MODE()

/* M6502   Increment operand */
#define OP_INC_M(MODE)		DST = EA_##MODE();								\
							FLAG_N = FLAG_Z = MAKE_UINT_8(read_8_##MODE(DST) + 1);	\
							write_8_##MODE(DST, FLAG_Z)

/* M6502   Increment register */
#define OP_INC_R(REG)		FLAG_N = FLAG_Z = REG = MAKE_UINT_8(REG + 1)

/* M6510   Increment and Subtract with Carry */
/* Unusual behavior: C flag is inverted */
#define OP_ISB(MODE)		DST = EA_##MODE();									\
							SRC = MAKE_UINT_8(read_8_##MODE(DST) + 1);			\
							write_8_##MODE(DST, SRC);							\
							OLD_C = CFLAG_AS_1()^1;								\
							FLAG_C = REG_A - SRC - OLD_C;						\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);				\
							FLAG_C ^= 0x100;									\
							FLAG_V = VFLAG_SUB(SRC, REG_A, FLAG_Z);				\
							if(!FLAG_D)											\
							{													\
								REG_A = FLAG_Z;									\
								break;											\
							}													\
							if((DST = (REG_A&0x0f) - (SRC&0x0f) - OLD_C) > 9)	\
								DST -= 6;										\
							if((DST += (REG_A&0xf0) - (SRC&0xf0)) > 0x99)		\
								DST += 0xa0;									\
							REG_A = DST

/* M6502   Jump */
/* If we're in a busy loop and there's no interrupt, eat all clock cycles */
#define OP_JMP(MODE)		JUMP(EA_##MODE());								\
							if(REG_PC == REG_PPC && !INT_PENDING())			\
								USE_ALL_CLKS()

/* M6502   Jump to Subroutine */
/* Unusual behavior: pushes opcode+2, not opcode+3 */
#define OP_JSR(MODE)		PUSH_16(REG_PC+1);								\
							JUMP(EA_##MODE())

/* M6510   Load Accumulator and X */
#define OP_LAX(MODE)		FLAG_N = FLAG_Z = REG_A = REG_X = OPER_8_##MODE()

/* M6502   Load register with operand */
#define OP_LD(REG, MODE)	FLAG_Z = FLAG_N = REG = OPER_8_##MODE()

/* M6502   Logical Shift Right accumulator */
#define OP_LSR_A()			FLAG_N = 0;										\
							FLAG_C = REG_A << 8;							\
							FLAG_Z = REG_A >>= 1

/* M6502   Logical Shift Right operand */
#define OP_LSR_M(MODE)		DST = EA_##MODE();								\
							FLAG_N = 0;										\
							FLAG_Z = read_8_##MODE(DST);					\
							FLAG_C = FLAG_Z << 8;							\
							FLAG_Z >>= 1;									\
							write_8_##MODE(DST, FLAG_Z)

/* M6502   No Operation */
#define OP_NOP()

/* M6502   Logical OR operand to accumulator */
#define OP_ORA(MODE)		FLAG_N = FLAG_Z = REG_A |= OPER_8_##MODE()

/* M6502   Push a register to the stack */
#define OP_PUSH(REG)		PUSH_8(REG)

/* M6502   Push the Processor Status Register to the stack */
#define OP_PHP()			PUSH_8(GET_REG_P())

/* M6502   Pull a register from the stack */
#define OP_PULL(REG)		REG = PULL_8()

/* M6502   Pull the accumulator from the stack */
#define OP_PLA()			FLAG_Z = FLAG_N = REG_A = PULL_8()

/* M6502   Pull the Processor Status Register from the stack */
#define OP_PLP()			SRC = FLAG_I;									\
							SET_REG_P(PULL_8());							\
							if(SRC == IFLAG_SET && FLAG_I == IFLAG_CLEAR)	\
								IRQ_DELAY = IRQ_DELAY_SET

/* M6510   Rotate Left, then logical AND with accumulator */
#define OP_RLA(MODE)		DST = EA_##MODE();									\
							FLAG_C = (read_8_##MODE(DST)<<1) | CFLAG_AS_1();	\
							REG_A &= MAKE_UINT_8(FLAG_C);						\
							FLAG_N = FLAG_Z = REG_A; 							\
							write_8_##MODE(DST, FLAG_Z)

/* M65C02  Reset Memory Bit */
#define OP_RMB(MODE, BIT)	DST = EA_##MODE();								\
							SRC = read_8_##MODE(DST) & ~BIT;				\
							write_8_##MODE(DST, SRC)

/* M6502   Rotate Left the accumulator */
#define OP_ROL_A()			FLAG_C = (REG_A<<1) | CFLAG_AS_1();				\
							FLAG_N = FLAG_Z = REG_A = MAKE_UINT_8(FLAG_C)

/* M6502   Rotate Left an operand */
#define OP_ROL_M(MODE)		DST = EA_##MODE();									\
							FLAG_C = (read_8_##MODE(DST)<<1) | CFLAG_AS_1();	\
							FLAG_N = FLAG_Z = MAKE_UINT_8(FLAG_C);				\
							write_8_##MODE(DST, FLAG_Z)

/* M6502   Rotate Right the accumulator */
#define OP_ROR_A()			REG_A |= FLAG_C & 0x100;						\
							FLAG_C = REG_A << 8;							\
							FLAG_N = FLAG_Z = REG_A >>= 1

/* M6502   Rotate Right an operand */
#define OP_ROR_M(MODE)		DST = EA_##MODE();								\
							FLAG_Z = read_8_##MODE(DST) | (FLAG_C & 0x100);	\
							FLAG_C = FLAG_Z << 8;							\
							FLAG_N = FLAG_Z >>= 1;							\
							write_8_##MODE(DST, FLAG_Z)

/* M6510   Rotate Right and Add with Carry */
#define OP_RRA(MODE)		DST = EA_##MODE();												\
							FLAG_Z = read_8_##MODE(DST) | (FLAG_C & 0x100);					\
							FLAG_C = FLAG_Z << 8;											\
							SRC = FLAG_N = FLAG_Z >>= 1;									\
							write_8_##MODE(DST, SRC);										\
							if(FLAG_D)														\
							{																\
								FLAG_C = CFLAG_AS_1();										\
								FLAG_N = REG_A + SRC + FLAG_C;								\
								FLAG_Z = MAKE_UINT_8(FLAG_N);								\
								if(((REG_A&0x0f) + (SRC&0x0f) + FLAG_C) > 9) FLAG_N += 6;	\
								FLAG_V = VFLAG_ADD(SRC, REG_A, FLAG_N);						\
								REG_A = FLAG_N;												\
								FLAG_C = (REG_A > 0x99) << 8;								\
								if(FLAG_C) REG_A -= 0xa0;									\
								break;														\
							}																\
							FLAG_C = REG_A + SRC + CFLAG_AS_1();							\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);							\
							FLAG_V = VFLAG_ADD(SRC, REG_A, FLAG_Z);							\
							REG_A = FLAG_Z


/* M6502   Return from Interrupt */
#define OP_RTI()			SRC = FLAG_I;									\
							SET_REG_P(PULL_8());							\
							JUMP(PULL_16());								\
							if(SRC == IFLAG_SET && FLAG_I == IFLAG_CLEAR)	\
								IRQ_DELAY = IRQ_DELAY_SET

/* M6502   Return from Subroutine */
/* Unusual behavior: Pulls PC and adds 1 */
#define OP_RTS()			JUMP(PULL_16()+1)

/* M6510   Store Accumulator AND Index High + 1 */
#define OP_SAH(MODE)		DST = EA_##MODE();								\
							SRC = REG_PC + 1;								\
							SRC = REG_A & REG_X & (read_8_IMM(SRC)+1);		\
							write_8_##MODE(DST, SRC)

/* M6510   Store Accumulator AND X */
#define OP_SAX(MODE)		DST = EA_##MODE();								\
							FLAG_N = FLAG_Z = REG_A & REG_X;				\
							write_8_##MODE(DST, FLAG_Z)

/* M6502   Subtract with Carry */
/* Unusual behavior: C flag is inverted */
#if M6502_USE_NEW_ADC_SBC
#define OP_SBC(MODE)		SRC = OPER_8_##MODE();								\
							OLD_C = CFLAG_AS_1()^1;								\
							FLAG_C = REG_A - SRC - OLD_C;						\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);				\
							FLAG_C ^= 0x100;									\
							FLAG_V = VFLAG_SUB(SRC, REG_A, FLAG_Z);				\
							if(!FLAG_D)											\
							{													\
								REG_A = FLAG_Z;									\
								break;											\
							}													\
							if((DST = (REG_A&0x0f) - (SRC&0x0f) - OLD_C) > 9)	\
								DST -= 6;										\
							if((DST += (REG_A&0xf0) - (SRC&0xf0)) > 0x99)		\
								DST += 0xa0;									\
							REG_A = DST
#else
#define OP_SBC(MODE)		SRC = OPER_8_##MODE();											\
							FLAG_C = CFLAG_AS_1()^1;										\
							if(FLAG_D)														\
							{																\
								FLAG_Z = (REG_A&0x0f) - (SRC&0x0f) - FLAG_C;				\
								FLAG_N = (REG_A&0xf0) - (SRC&0xf0);							\
								FLAG_C = REG_A - SRC - FLAG_C;								\
								FLAG_V = VFLAG_SUB(SRC, REG_A, FLAG_C);						\
								FLAG_C = ((FLAG_C&0xff00) == 0)<<8;							\
								if(FLAG_Z&0xf0)												\
									FLAG_Z -= 6;											\
								if(FLAG_Z&0x80)												\
									FLAG_N -= 0x10;											\
								if(FLAG_N&0x0f00)											\
									FLAG_N -= 0x60;											\
								FLAG_N = FLAG_Z = REG_A = (FLAG_Z&0x0f) | (FLAG_N&0xf0);	\
								break;														\
							}																\
							FLAG_C = REG_A - SRC - FLAG_C;									\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);							\
							FLAG_C ^= 0x100;												\
							FLAG_V = VFLAG_SUB(SRC, REG_A, FLAG_Z);							\
							REG_A = FLAG_Z
#endif

/* N2a03   Subtract with Carry, no Decimal mode */
/* Unusual behavior: C flag is inverted */
#define OP_SBC_ND(MODE)		SRC = OPER_8_##MODE();							\
							OLD_C = CFLAG_AS_1()^1;							\
							FLAG_C = REG_A - SRC - OLD_C;					\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);			\
							FLAG_C ^= 0x100;								\
							FLAG_V = VFLAG_SUB(SRC, REG_A, FLAG_Z);			\
							REG_A = FLAG_Z

/* M6502   Set Carry flag */
#define OP_SEC()			FLAG_C = CFLAG_SET

/* M6502   Set Decimal flag */
#define OP_SED()			FLAG_D = DFLAG_SET

/* M6502   Set Interrupt Mask flag */
#define OP_SEI()			FLAG_I = IFLAG_SET

/* M6510   Shift Left and logical OR */
#define OP_SLO(MODE)		DST = EA_##MODE();								\
							FLAG_C = read_8_##MODE(DST) << 1;				\
							FLAG_N = FLAG_Z = REG_A |= MAKE_UINT_8(FLAG_C);	\
							write_8_##MODE(DST, FLAG_Z)

/* M65C02  Set Memory Bit */
#define OP_SMB(MODE, BIT)	DST = EA_##MODE();								\
							SRC = read_8_##MODE(DST) | BIT;					\
							write_8_##MODE(DST, SRC)

/* M6510   Shift Right and Exclusive OR */
#define OP_SRE(MODE)		DST = EA_##MODE();								\
							FLAG_Z = read_8_##MODE(DST);					\
							FLAG_C = FLAG_Z << 8;							\
							FLAG_N = FLAG_Z = REG_A ^= FLAG_Z >> 1;			\
							write_8_##MODE(DST, FLAG_Z)

/* M6510   Store Stack High */
#define OP_SSH(MODE)		DST = EA_##MODE();								\
							REG_S = REG_A & REG_X;							\
							SRC = REG_PC + 1;								\
							SRC = REG_S & (read_8_IMM(SRC)+1);				\
							write_8_##MODE(DST, SRC)

/* M6502   Store register to memory */
#define OP_ST(REG, MODE)	write_8_##MODE(EA_##MODE(), REG)

/* M6510   Store X High */
#define OP_SXH(MODE)		DST = EA_##MODE();								\
							SRC = REG_PC + 1;								\
							SRC = REG_X & (read_8_IMM(SRC)+1);				\
							write_8_##MODE(DST, SRC)

/* M6510   Store Y High */
#define OP_SYH(MODE)		DST = EA_##MODE();								\
							SRC = REG_PC + 1;								\
							SRC = REG_Y & (read_8_IMM(SRC)+1);				\
							write_8_##MODE(DST, SRC)

/* M6510   Triple No Operation */
#define OP_TOP()			REG_PC += 2

/* M6502   Transfer source register to destination register */
#define OP_TRANS(RS, RD)	FLAG_N = FLAG_Z = RD = RS

/* M6502   Transfer X to the stack pointer */
#define OP_TXS()			REG_S = REG_X

/* M65C02  Test and Reset Bits */
#define OP_TRB(MODE)		DST = EA_##MODE();								\
							FLAG_Z = read_8_##MODE(DST);					\
							SRC = FLAG_Z & ~REG_A;							\
							write_8_##MODE(DST, SRC);						\
							FLAG_Z &= REG_A

/* M65C02  Test and Set Bits */
#define OP_TSB(MODE)		DST = EA_##MODE();								\
							FLAG_Z = read_8_##MODE(DST);					\
							SRC = FLAG_Z | REG_A;							\
							write_8_##MODE(DST, SRC);						\
							FLAG_Z &= REG_A

#define OP_ILLEGAL()



/* ======================================================================== */
/* ================================= API ================================== */
/* ======================================================================== */



void m65xx_reset(void* param)
{
	REG_S = 0;
	FLAG_Z = ZFLAG_CLEAR;
	FLAG_I = IFLAG_SET;
	LINE_IRQ = IRQ_CLEAR;
	LINE_NMI = NMI_CLEAR;
	IRQ_DELAY = IRQ_DELAY_CLEAR;
	JUMP(read_16_VEC(VECTOR_RST));
}

/* Exit and clean up */
void m65xx_exit(void)
{
	/* nothing to do yet */
}


/* Get the current CPU context */
unsigned m65xx_get_context(void *dst_context)
{
	if(dst_context)
		*(m6502i_cpu_struct*)dst_context = m6502i_cpu;
	return sizeof(m6502i_cpu);
}

/* Set the current CPU context */
void m65xx_set_context(void *src_context)
{
	if(src_context)
	{
		m6502i_cpu = *(m6502i_cpu_struct*)src_context;
		JUMP(REG_PC);
	}
}

/* Get the current Program Counter */
unsigned m65xx_get_pc(void)
{
	return REG_PC;
}

/* Set the Program Counter */
void m65xx_set_pc(unsigned val)
{
	JUMP(val);
}

/* Get the current Stack Pointer */
unsigned m65xx_get_sp(void)
{
	return REG_S + STACK_PAGE;
}

/* Set the Stack Pointer */
void m65xx_set_sp(unsigned val)
{
	REG_S = MAKE_UINT_8(val);
}

#ifdef USING_MAME

/* Get a register from the CPU core */
unsigned m65xx_get_reg(int regnum)
{
	switch(regnum)
	{
		case M6502_PC: return REG_PC;
		case M6502_S: return REG_S + STACK_PAGE;
		case M6502_P: return GET_REG_P();
		case M6502_A: return REG_A;
		case M6502_X: return REG_X;
		case M6502_Y: return REG_Y;
		case M6502_NMI_STATE: return LINE_NMI == NMI_SET;
		case M6502_IRQ_STATE: return LINE_IRQ == IRQ_SET;
		case REG_PREVIOUSPC: return REG_PPC;
		default:
			if(regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = REG_S + STACK_PAGE + 2 * (REG_SP_CONTENTS - regnum);
				if(offset < 0x1ff)
					return read_8_STK(offset) | (read_8_STK(offset + 1) << 8);
			}
	}
	return 0;
}

/* Set a register in the CPU core */
void m65xx_set_reg(int regnum, unsigned val)
{
	switch(regnum)
	{
		case M6502_PC: REG_PC = MAKE_UINT_16(val); break;
		case M6502_S: REG_S = MAKE_UINT_8(val); break;
		case M6502_P: SET_REG_P(val); break;
		case M6502_A: REG_A = MAKE_UINT_8(val); break;
		case M6502_X: REG_X = MAKE_UINT_8(val); break;
		case M6502_Y: REG_Y = MAKE_UINT_8(val); break;
		case M6502_NMI_STATE: m65xx_set_nmi_line(val); break;
		case M6502_IRQ_STATE: m65xx_set_irq_line(0, val); break;
		default:
			if(regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = REG_S + STACK_PAGE + 2 * (REG_SP_CONTENTS - regnum);
				if(offset < 0x1ff)
				{
					write_8_STK(offset, MAKE_UINT_8(val));
					write_8_STK(offset + 1, MAKE_UINT_8(val >> 8));
				}
			}
	 }
}

#endif

/* Assert or clear the NMI line of the CPU */
void m65xx_set_nmi_line(int state)
{
	if(state == CLEAR_LINE)
		LINE_NMI = 0;
	else if(!LINE_NMI)
	{
		LINE_NMI = 1;
		CLK(7);
		PUSH_16(REG_PC);
		PUSH_8(GET_REG_P_INT());
//		FLAG_I = IFLAG_SET; this shouldn't be set
		FLAG_D = DFLAG_CLEAR;
		JUMP(read_16_VEC(VECTOR_NMI));
	}
}

/* Assert or clear the IRQ line of the CPU */
void m65xx_set_irq_line(int line, int state)
{
	LINE_IRQ = (state != CLEAR_LINE) ? IRQ_SET : IRQ_CLEAR;
//	CHECK_IRQ();
}

/* Set the callback that is called when servicing an interrupt */
void m65xx_set_irq_callback(int (*callback)(int))
{
	INT_ACK = callback;
}

/* Save the current CPU state to disk */
void m65xx_state_save(void *file)
{
#if 0
	int cpu = cpu_getactivecpu();
	uint p = GET_REG_P();
	state_save_UINT16(file,"m6502",cpu,"PC",&REG_PC,2);
	state_save_UINT16(file,"m6502",cpu,"PPC",&REG_PPC,2);
	state_save_UINT8(file,"m6502",cpu,"SP",&REG_SP,1);
	state_save_UINT8(file,"m6502",cpu,"P",&p,1);
	state_save_UINT8(file,"m6502",cpu,"A",&REG_A,1);
	state_save_UINT8(file,"m6502",cpu,"X",&REG_X,1);
	state_save_UINT8(file,"m6502",cpu,"Y",&REG_Y,1);
	state_save_UINT8(file,"m6502",cpu,"RESET",&LINE_RST,1);
	state_save_UINT8(file,"m6502",cpu,"IRQ",&LINE_IRQ,1);
	state_save_UINT8(file,"m6502",cpu,"NMI",&LINE_NMI,1);
	state_save_UINT8(file,"m6502",cpu,"IRQ_DELAY",&IRQ_DELAY,1);
#endif
}

/* Load a CPU state from disk */
void m65xx_state_load(void *file)
{
#if 0
	int cpu = cpu_getactivecpu();
	uint p;
	state_load_UINT16(file,"m6502",cpu,"PC",&REG_PC,2);
	state_load_UINT16(file,"m6502",cpu,"PPC",&REG_PPC,2);
	state_load_UINT8(file,"m6502",cpu,"SP",&REG_SP,1);
	state_load_UINT8(file,"m6502",cpu,"P",&p,1);
	state_load_UINT8(file,"m6502",cpu,"A",&REG_A,1);
	state_load_UINT8(file,"m6502",cpu,"X",&REG_X,1);
	state_load_UINT8(file,"m6502",cpu,"Y",&REG_Y,1);
	state_load_UINT8(file,"m6502",cpu,"RESET",&LINE_RST,1);
	state_load_UINT8(file,"m6502",cpu,"IRQ",&LINE_IRQ,1);
	state_load_UINT8(file,"m6502",cpu,"NMI",&LINE_NMI,1);
	state_load_UINT8(file,"m6502",cpu,"IRQ_DELAY",&IRQ_DELAY,1);
	SET_REG_P(p);
#endif
}

#ifdef USING_MAME

/* Get a formatted string representing a register and its contents */
const char *m65xx_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	m6502i_cpu_struct* r = context;
	uint p;

	which = ++which % 16;
	buffer[which][0] = '\0';
	if(!context)
		r = &m6502i_cpu;

	p =  ((r->flag_n & 0x80)			|
			((r->flag_v & 0x80) >> 1)	|
			FLAGPOS_R					|
			(r->flag_b << 4)			|
			(r->flag_d << 3)			|
			(r->flag_i << 2)			|
			((!r->flag_z) << 1)			|
			((r->flag_c >> 8)&1));

	 switch(regnum)
	{
		case CPU_INFO_REG+M6502_PC:			sprintf(buffer[which], "PC:%04X", r->pc); break;
		case CPU_INFO_REG+M6502_S:			sprintf(buffer[which], "S:%02X", r->s); break;
		case CPU_INFO_REG+M6502_P:			sprintf(buffer[which], "P:%02X", p); break;
		case CPU_INFO_REG+M6502_A:			sprintf(buffer[which], "A:%02X", r->a); break;
		case CPU_INFO_REG+M6502_X:			sprintf(buffer[which], "X:%02X", r->x); break;
		case CPU_INFO_REG+M6502_Y:			sprintf(buffer[which], "Y:%02X", r->y); break;
		case CPU_INFO_REG+M6502_NMI_STATE:	sprintf(buffer[which], "NMI:%X", r->line_nmi == NMI_SET); break;
		case CPU_INFO_REG+M6502_IRQ_STATE:	sprintf(buffer[which], "IRQ:%X", r->line_irq == IRQ_SET); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				p & 0x80 ? 'N':'.',
				p & 0x40 ? 'V':'.',
				p & 0x20 ? 'R':'.',
				p & 0x10 ? 'B':'.',
				p & 0x08 ? 'D':'.',
				p & 0x04 ? 'I':'.',
				p & 0x02 ? 'Z':'.',
				p & 0x01 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "M6502";
		case CPU_INFO_FAMILY: return "Motorola 6502";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (c) 1999 Karl Stenerud, all rights reserved.";
		case CPU_INFO_REG_LAYOUT: return (const char*)m65xx_register_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)m65xx_window_layout;
	}
	return buffer[which];
}

#endif

/* Disassemble an instruction */
unsigned m65xx_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm6502(buffer, pc);
#else
	sprintf(buffer, "$%02X", read_8_instruction(pc));
	return 1;
#endif
}


/* Pulse the SO pin (Set Overflow) */
void m65xx_pulse_so(void)
{
	FLAG_V = 0x80;
}

/* Old get context function */
void m65xx_get_context_old(m6502_cpu_struct* cpu)
{
	cpu->a   = REG_A;
	cpu->x   = REG_X;
	cpu->y   = REG_Y;
	cpu->s   = REG_S;
	cpu->pc  = REG_PC & 0xffff;
	cpu->p   = GET_REG_P();
	cpu->irq = LINE_IRQ == IRQ_SET;
	cpu->nmi = LINE_NMI == IRQ_SET;
	cpu->ir  = REG_IR;
}

/* Old set context function */
void m65xx_set_context_old(m6502_cpu_struct* cpu)
{
	REG_A  = cpu->a & 0xff;
	REG_X  = cpu->x & 0xff;
	REG_Y  = cpu->y & 0xff;
	REG_S  = cpu->s & 0xff;
	REG_PC = cpu->pc & 0xffff;
	SET_REG_P(cpu->p);
	LINE_IRQ = cpu->irq ? IRQ_SET : IRQ_CLEAR;
	LINE_NMI = cpu->nmi ? IRQ_SET : IRQ_CLEAR;
	REG_IR = cpu->ir & 0xff;
}


#if HAS_M6502

extern int d6502_disassemble(char* buff, unsigned int pc);

/* Execute instructions for <clocks> cycles */
int m6502_execute(int clocks)
{
//printf("execute %d\n", clocks);
	CLOCKS = clocks;
	while(CLOCKS > 0)
	{
		IRQ_DELAY = IRQ_DELAY_CLEAR;
		REG_PPC = REG_PC;
		CALL_MAME_DEBUG;
		REG_PC++;

		switch(REG_IR = read_8_instruction(REG_PPC))
		{
			case 0x00: /* BRK     */ CLK(7); OP_BRK   (              ); break;
			case 0x01: /* ORA idx */ CLK(6); OP_ORA   ( IDX          ); break;
			case 0x05: /* ORA zp  */ CLK(3); OP_ORA   ( ZPG          ); break;
			case 0x06: /* ASL zp  */ CLK(5); OP_ASL_M ( ZPG          ); break;
			case 0x08: /* PHP     */ CLK(2); OP_PHP   (              ); break;
			case 0x09: /* ORA imm */ CLK(2); OP_ORA   ( IMM          ); break;
			case 0x0a: /* ASL acc */ CLK(2); OP_ASL_A (              ); break;
			case 0x0d: /* ORA abs */ CLK(4); OP_ORA   ( ABS          ); break;
			case 0x0e: /* ASL abs */ CLK(6); OP_ASL_M ( ABS          ); break;
			case 0x10: /* BPL     */ CLK(2); OP_BCC   ( COND_PL()    ); break;
			case 0x11: /* ORA idy */ CLK(5); OP_ORA   ( IDY          ); break;
			case 0x15: /* ORA zpx */ CLK(4); OP_ORA   ( ZPX          ); break;
			case 0x16: /* ASL zpx */ CLK(6); OP_ASL_M ( ZPX          ); break;
			case 0x18: /* CLC     */ CLK(2); OP_CLC   (              ); break;
			case 0x19: /* ORA aby */ CLK(4); OP_ORA   ( ABY_P        ); break;
			case 0x1d: /* ORA abx */ CLK(4); OP_ORA   ( ABX_P        ); break;
			case 0x1e: /* ASL abx */ CLK(7); OP_ASL_M ( ABX          ); break;
			case 0x20: /* JSR     */ CLK(6); OP_JSR   ( ABS          ); break;
			case 0x21: /* AND idx */ CLK(6); OP_AND   ( IDX          ); break;
			case 0x24: /* BIT zp  */ CLK(3); OP_BIT   ( ZPG          ); break;
			case 0x25: /* AND zp  */ CLK(3); OP_AND   ( ZPG          ); break;
			case 0x26: /* ROL zp  */ CLK(5); OP_ROL_M ( ZPG          ); break;
			case 0x28: /* PLP     */ CLK(2); OP_PLP   (              ); break;
			case 0x29: /* AND imm */ CLK(2); OP_AND   ( IMM          ); break;
			case 0x2a: /* ROL acc */ CLK(2); OP_ROL_A (              ); break;
			case 0x2c: /* BIT abs */ CLK(4); OP_BIT   ( ABS          ); break;
			case 0x2d: /* AND abs */ CLK(4); OP_AND   ( ABS          ); break;
			case 0x2e: /* ROL abs */ CLK(6); OP_ROL_M ( ABS          ); break;
			case 0x30: /* BMI     */ CLK(2); OP_BCC   ( COND_MI()    ); break;
			case 0x31: /* AND idy */ CLK(5); OP_AND   ( IDY          ); break;
			case 0x35: /* AND zpx */ CLK(4); OP_AND   ( ZPX          ); break;
			case 0x36: /* ROL zpx */ CLK(6); OP_ROL_M ( ZPX          ); break;
			case 0x38: /* SEC     */ CLK(2); OP_SEC   (              ); break;
			case 0x39: /* AND aby */ CLK(4); OP_AND   ( ABY_P        ); break;
			case 0x3d: /* AND abx */ CLK(4); OP_AND   ( ABX_P        ); break;
			case 0x3e: /* ROL abx */ CLK(7); OP_ROL_M ( ABX          ); break;
			case 0x40: /* RTI     */ CLK(6); OP_RTI   (              ); break;
			case 0x41: /* EOR idx */ CLK(6); OP_EOR   ( IDX          ); break;
			case 0x45: /* EOR zp  */ CLK(3); OP_EOR   ( ZPG          ); break;
			case 0x46: /* LSR zp  */ CLK(5); OP_LSR_M ( ZPG          ); break;
			case 0x48: /* PHA     */ CLK(2); OP_PUSH  ( REG_A        ); break;
			case 0x49: /* EOR imm */ CLK(2); OP_EOR   ( IMM          ); break;
			case 0x4a: /* LSR acc */ CLK(2); OP_LSR_A (              ); break;
			case 0x4c: /* JMP abs */ CLK(3); OP_JMP   ( ABS          ); break;
			case 0x4d: /* EOR abs */ CLK(4); OP_EOR   ( ABS          ); break;
			case 0x4e: /* LSR abs */ CLK(6); OP_LSR_M ( ABS          ); break;
			case 0x50: /* BVC     */ CLK(2); OP_BCC   ( COND_VC()    ); break;
			case 0x51: /* EOR idy */ CLK(5); OP_EOR   ( IDY_P        ); break;
			case 0x55: /* EOR zpx */ CLK(4); OP_EOR   ( ZPX          ); break;
			case 0x56: /* LSR zpx */ CLK(6); OP_LSR_M ( ZPX          ); break;
			case 0x58: /* CLI     */ CLK(2); OP_CLI   (              ); break;
			case 0x59: /* EOR aby */ CLK(4); OP_EOR   ( ABY_P        ); break;
			case 0x5d: /* EOR abx */ CLK(4); OP_EOR   ( ABX_P        ); break;
			case 0x5e: /* LSR abx */ CLK(7); OP_LSR_M ( ABX          ); break;
			case 0x60: /* RTS     */ CLK(6); OP_RTS   (              ); break;
			case 0x61: /* ADC idx */ CLK(6); OP_ADC   ( IDX          ); break;
			case 0x65: /* ADC zp  */ CLK(3); OP_ADC   ( ZPG          ); break;
			case 0x66: /* ROR zp  */ CLK(5); OP_ROR_M ( ZPG          ); break;
			case 0x68: /* PLA     */ CLK(2); OP_PLA   (              ); break;
			case 0x69: /* ADC imm */ CLK(2); OP_ADC   ( IMM          ); break;
			case 0x6a: /* ROR acc */ CLK(2); OP_ROR_A (              ); break;
			case 0x6c: /* JMP ind */ CLK(5); OP_JMP   ( IND          ); break;
			case 0x6d: /* ADC abs */ CLK(4); OP_ADC   ( ABS          ); break;
			case 0x6e: /* ROR abs */ CLK(6); OP_ROR_M ( ABS          ); break;
			case 0x70: /* BVS     */ CLK(2); OP_BCC   ( COND_VS()    ); break;
			case 0x71: /* ADC idy */ CLK(5); OP_ADC   ( IDY_P        ); break;
			case 0x75: /* ADC zpx */ CLK(4); OP_ADC   ( ZPX          ); break;
			case 0x76: /* ROR zpx */ CLK(6); OP_ROR_M ( ZPX          ); break;
			case 0x78: /* SEI     */ CLK(2); OP_SEI   (              ); break;
			case 0x79: /* ADC aby */ CLK(4); OP_ADC   ( ABY_P        ); break;
			case 0x7d: /* ADC abx */ CLK(4); OP_ADC   ( ABX_P        ); break;
			case 0x7e: /* ROR abx */ CLK(7); OP_ROR_M ( ABX          ); break;
			case 0x81: /* STA idx */ CLK(6); OP_ST    ( REG_A, IDX   ); break;
			case 0x84: /* STY zp  */ CLK(3); OP_ST    ( REG_Y, ZPG   ); break;
			case 0x85: /* STA zp  */ CLK(3); OP_ST    ( REG_A, ZPG   ); break;
			case 0x86: /* STX zp  */ CLK(3); OP_ST    ( REG_X, ZPG   ); break;
			case 0x88: /* DEY     */ CLK(2); OP_DEC_R ( REG_Y        ); break;
			case 0x8a: /* TXA     */ CLK(2); OP_TRANS ( REG_X, REG_A ); break;
			case 0x8c: /* STY abs */ CLK(4); OP_ST    ( REG_Y, ABS   ); break;
			case 0x8d: /* STA abs */ CLK(4); OP_ST    ( REG_A, ABS   ); break;
			case 0x8e: /* STX abs */ CLK(5); OP_ST    ( REG_X, ABS   ); break;
			case 0x90: /* BCC     */ CLK(2); OP_BCC   ( COND_CC()    ); break;
			case 0x91: /* STA idy */ CLK(6); OP_ST    ( REG_A, IDY   ); break;
			case 0x94: /* STY zpx */ CLK(4); OP_ST    ( REG_Y, ZPX   ); break;
			case 0x95: /* STA zpx */ CLK(4); OP_ST    ( REG_A, ZPX   ); break;
			case 0x96: /* STX zpy */ CLK(4); OP_ST    ( REG_X, ZPY   ); break;
			case 0x98: /* TYA     */ CLK(2); OP_TRANS ( REG_Y, REG_A ); break;
			case 0x99: /* STA aby */ CLK(5); OP_ST    ( REG_A, ABY   ); break;
			case 0x9a: /* TXS     */ CLK(2); OP_TXS   (              ); break;
			case 0x9d: /* STA abx */ CLK(5); OP_ST    ( REG_A, ABX   ); break;
			case 0xa0: /* LDY imm */ CLK(2); OP_LD    ( REG_Y, IMM   ); break;
			case 0xa1: /* LDA idx */ CLK(6); OP_LD    ( REG_A, IDX   ); break;
			case 0xa2: /* LDX imm */ CLK(2); OP_LD    ( REG_X, IMM   ); break;
			case 0xa4: /* LDY zp  */ CLK(3); OP_LD    ( REG_Y, ZPG   ); break;
			case 0xa5: /* LDA zp  */ CLK(3); OP_LD    ( REG_A, ZPG   ); break;
			case 0xa6: /* LDX zp  */ CLK(3); OP_LD    ( REG_X, ZPG   ); break;
			case 0xa8: /* TAY     */ CLK(2); OP_TRANS ( REG_A, REG_Y ); break;
			case 0xa9: /* LDA imm */ CLK(2); OP_LD    ( REG_A, IMM   ); break;
			case 0xaa: /* TAX     */ CLK(2); OP_TRANS ( REG_A, REG_X ); break;
			case 0xac: /* LDY abs */ CLK(4); OP_LD    ( REG_Y, ABS   ); break;
			case 0xad: /* LDA abs */ CLK(4); OP_LD    ( REG_A, ABS   ); break;
			case 0xae: /* LDX abs */ CLK(4); OP_LD    ( REG_X, ABS   ); break;
			case 0xb0: /* BCS     */ CLK(2); OP_BCC   ( COND_CS()    ); break;
			case 0xb1: /* LDA idy */ CLK(5); OP_LD    ( REG_A, IDY_P ); break;
			case 0xb4: /* LDY zpx */ CLK(4); OP_LD    ( REG_Y, ZPX   ); break;
			case 0xb5: /* LDA zpx */ CLK(4); OP_LD    ( REG_A, ZPX   ); break;
			case 0xb6: /* LDX zpy */ CLK(4); OP_LD    ( REG_X, ZPY   ); break;
			case 0xb8: /* CLV     */ CLK(2); OP_CLV   (              ); break;
			case 0xb9: /* LDA aby */ CLK(4); OP_LD    ( REG_A, ABY_P ); break;
			case 0xba: /* TSX     */ CLK(2); OP_TRANS ( REG_S, REG_X ); break;
			case 0xbc: /* LDY abx */ CLK(4); OP_LD    ( REG_Y, ABX_P ); break;
			case 0xbd: /* LDA abx */ CLK(4); OP_LD    ( REG_A, ABX_P ); break;
			case 0xbe: /* LDX aby */ CLK(4); OP_LD    ( REG_X, ABY_P ); break;
			case 0xc0: /* CPY imm */ CLK(2); OP_CMP   ( REG_Y, IMM   ); break;
			case 0xc1: /* CMP idx */ CLK(6); OP_CMP   ( REG_A, IDX   ); break;
			case 0xc4: /* CPY zp  */ CLK(3); OP_CMP   ( REG_Y, ZPG   ); break;
			case 0xc5: /* CMP zp  */ CLK(3); OP_CMP   ( REG_A, ZPG   ); break;
			case 0xc6: /* DEC zp  */ CLK(5); OP_DEC_M ( ZPG          ); break;
			case 0xc8: /* INY     */ CLK(2); OP_INC_R ( REG_Y        ); break;
			case 0xc9: /* CMP imm */ CLK(2); OP_CMP   ( REG_A, IMM   ); break;
			case 0xca: /* DEX     */ CLK(2); OP_DEC_R ( REG_X        ); break;
			case 0xcc: /* CPY abs */ CLK(4); OP_CMP   ( REG_Y, ABS   ); break;
			case 0xcd: /* CMP abs */ CLK(4); OP_CMP   ( REG_A, ABS   ); break;
			case 0xce: /* DEC abs */ CLK(6); OP_DEC_M ( ABS          ); break;
			case 0xd0: /* BNE     */ CLK(2); OP_BCC   ( COND_NE()    ); break;
			case 0xd1: /* CMP idy */ CLK(5); OP_CMP   ( REG_A, IDY_P ); break;
			case 0xd5: /* CMP zpx */ CLK(4); OP_CMP   ( REG_A, ZPX   ); break;
			case 0xd6: /* DEC zpx */ CLK(6); OP_DEC_M ( ZPX          ); break;
			case 0xd8: /* CLD     */ CLK(2); OP_CLD   (              ); break;
			case 0xd9: /* CMP aby */ CLK(4); OP_CMP   ( REG_A, ABY_P ); break;
			case 0xdd: /* CMP abx */ CLK(4); OP_CMP   ( REG_A, ABX_P ); break;
			case 0xde: /* DEC abx */ CLK(7); OP_DEC_M ( ABX          ); break;
			case 0xe0: /* CPX imm */ CLK(2); OP_CMP   ( REG_X, IMM   ); break;
			case 0xe1: /* SBC idx */ CLK(6); OP_SBC   ( IDX          ); break;
			case 0xe4: /* CPX zp  */ CLK(3); OP_CMP   ( REG_X, ZPG   ); break;
			case 0xe5: /* SBC zp  */ CLK(3); OP_SBC   ( ZPG          ); break;
			case 0xe6: /* INC zp  */ CLK(5); OP_INC_M ( ZPG          ); break;
			case 0xe8: /* INX     */ CLK(2); OP_INC_R ( REG_X        ); break;
			case 0xe9: /* SBC imm */ CLK(2); OP_SBC   ( IMM          ); break;
			case 0xea: /* NOP     */ CLK(2); OP_NOP   (              ); break;
			case 0xec: /* CPX abs */ CLK(4); OP_CMP   ( REG_X, ABS   ); break;
			case 0xed: /* SBC abs */ CLK(4); OP_SBC   ( ABS          ); break;
			case 0xee: /* INC abs */ CLK(6); OP_INC_M ( ABS          ); break;
			case 0xf0: /* BEQ     */ CLK(2); OP_BCC   ( COND_EQ()    ); break;
			case 0xf1: /* SBC idy */ CLK(5); OP_SBC   ( IDY          ); break;
			case 0xf5: /* SBC zpx */ CLK(4); OP_SBC   ( ZPX          ); break;
			case 0xf6: /* INC zpx */ CLK(6); OP_INC_M ( ZPX          ); break;
			case 0xf8: /* SED     */ CLK(2); OP_SED   (              ); break;
			case 0xf9: /* SBC aby */ CLK(4); OP_SBC   ( ABY_P        ); break;
			case 0xfd: /* SBC abx */ CLK(4); OP_SBC   ( ABX_P        ); break;
			case 0xfe: /* INC abx */ CLK(7); OP_INC_M ( ABX          ); break;
			default:   /* illegal */ CLK(2); OP_ILLEGAL();           break;
		}
		CHECK_IRQ();
	}
	return clocks - CLOCKS;
}

#endif /* HAS_M6502 */


#if HAS_M65C02

/* Execute instructions for <clocks> cycles */
int m65c02_execute(int clocks)
{
	CLOCKS = LINE_RST ? 0 : clocks;
	while(CLOCKS > 0)
	{
#if M6502_STRICT_IRQ
		IRQ_DELAY = 0;
#endif /* M6502_STRICT_IRQ */

		REG_PPC = REG_PC;

		CALL_MAME_DEBUG;

		REG_PC++;

		switch(REG_IR = read_8_instruction(REG_PPC))
		{
			case 0x00: /* BRK     */ CLK(7); OP_BRK   (              ); break;
			case 0x01: /* ORA idx */ CLK(6); OP_ORA   ( IDX          ); break;
			case 0x04: /* TSB zp  */ CLK(3); OP_TSB   ( ZPG          ); break; /* C */
			case 0x05: /* ORA zp  */ CLK(3); OP_ORA   ( ZPG          ); break;
			case 0x06: /* ASL zp  */ CLK(5); OP_ASL_M ( ZPG          ); break;
			case 0x07: /* RMB0    */ CLK(5); OP_RMB   ( ZPG, BIT_0   ); break; /* C */
			case 0x08: /* PHP     */ CLK(2); OP_PHP   (              ); break;
			case 0x09: /* ORA imm */ CLK(2); OP_ORA   ( IMM          ); break;
			case 0x0a: /* ASL acc */ CLK(2); OP_ASL_A (              ); break;
			case 0x0c: /* TSB abs */ CLK(2); OP_TSB   ( ABS          ); break; /* C */
			case 0x0d: /* ORA abs */ CLK(4); OP_ORA   ( ABS          ); break;
			case 0x0e: /* ASL abs */ CLK(6); OP_ASL_M ( ABS          ); break;
			case 0x0f: /* BBR0    */ CLK(5); OP_BBR   ( BIT_0        ); break; /* C */
			case 0x10: /* BPL     */ CLK(2); OP_BCC   ( COND_PL()    ); break;
			case 0x11: /* ORA idy */ CLK(5); OP_ORA   ( IDY          ); break;
			case 0x12: /* ORA zpi */ CLK(3); OP_ORA   ( ZPI          ); break; /* C */
			case 0x14: /* TRB zp  */ CLK(3); OP_TRB   ( ZPG          ); break; /* C */
			case 0x15: /* ORA zpx */ CLK(4); OP_ORA   ( ZPX          ); break;
			case 0x16: /* ASL zpx */ CLK(6); OP_ASL_M ( ZPX          ); break;
			case 0x17: /* RMB1    */ CLK(5); OP_RMB   ( ZPG, BIT_1   ); break; /* C */
			case 0x18: /* CLC     */ CLK(2); OP_CLC   (              ); break;
			case 0x19: /* ORA aby */ CLK(4); OP_ORA   ( ABY_P        ); break;
			case 0x1a: /* INA     */ CLK(2); OP_INC_R ( REG_A        ); break; /* C */
			case 0x1c: /* TRB abs */ CLK(4); OP_TRB   ( ABS          ); break; /* C */
			case 0x1d: /* ORA abx */ CLK(4); OP_ORA   ( ABX_P        ); break;
			case 0x1e: /* ASL abx */ CLK(7); OP_ASL_M ( ABX          ); break;
			case 0x1f: /* BBR1    */ CLK(5); OP_BBR   ( BIT_1        ); break; /* C */
			case 0x20: /* JSR     */ CLK(6); OP_JSR   ( ABS          ); break;
			case 0x21: /* AND idx */ CLK(6); OP_AND   ( IDX          ); break;
			case 0x24: /* BIT zp  */ CLK(3); OP_BIT   ( ZPG          ); break;
			case 0x25: /* AND zp  */ CLK(3); OP_AND   ( ZPG          ); break;
			case 0x26: /* ROL zp  */ CLK(5); OP_ROL_M ( ZPG          ); break;
			case 0x27: /* RMB2    */ CLK(5); OP_RMB   ( ZPG, BIT_2   ); break; /* C */
			case 0x28: /* PLP     */ CLK(2); OP_PLP   (              ); break;
			case 0x29: /* AND imm */ CLK(2); OP_AND   ( IMM          ); break;
			case 0x2a: /* ROL acc */ CLK(2); OP_ROL_A (              ); break;
			case 0x2c: /* BIT abs */ CLK(4); OP_BIT   ( ABS          ); break;
			case 0x2d: /* AND abs */ CLK(4); OP_AND   ( ABS          ); break;
			case 0x2e: /* ROL abs */ CLK(6); OP_ROL_M ( ABS          ); break;
			case 0x2f: /* BBR2    */ CLK(5); OP_BBR   ( BIT_2        ); break; /* C */
			case 0x30: /* BMI     */ CLK(2); OP_BCC   ( COND_MI()    ); break;
			case 0x31: /* AND idy */ CLK(5); OP_AND   ( IDY          ); break;
			case 0x32: /* AND zpi */ CLK(3); OP_AND   ( ZPI          ); break; /* C */
			case 0x34: /* BIT zpx */ CLK(4); OP_BIT   ( ZPX          ); break; /* C */
			case 0x35: /* AND zpx */ CLK(4); OP_AND   ( ZPX          ); break;
			case 0x36: /* ROL zpx */ CLK(6); OP_ROL_M ( ZPX          ); break;
			case 0x37: /* RMB3    */ CLK(5); OP_RMB   ( ZPG, BIT_3   ); break; /* C */
			case 0x38: /* SEC     */ CLK(2); OP_SEC   (              ); break;
			case 0x39: /* AND aby */ CLK(4); OP_AND   ( ABY_P        ); break;
			case 0x3a: /* DEA     */ CLK(2); OP_DEC_R ( REG_A        ); break; /* C */
			case 0x3c: /* BIT abx */ CLK(4); OP_BIT   ( ABX          ); break; /* C */
			case 0x3d: /* AND abx */ CLK(4); OP_AND   ( ABX_P        ); break;
			case 0x3e: /* ROL abx */ CLK(7); OP_ROL_M ( ABX          ); break;
			case 0x3f: /* BBR3    */ CLK(5); OP_BBR   ( BIT_3        ); break; /* C */
			case 0x40: /* RTI     */ CLK(6); OP_RTI   (              ); break;
			case 0x41: /* EOR idx */ CLK(6); OP_EOR   ( IDX          ); break;
			case 0x45: /* EOR zp  */ CLK(3); OP_EOR   ( ZPG          ); break;
			case 0x46: /* LSR zp  */ CLK(5); OP_LSR_M ( ZPG          ); break;
			case 0x47: /* RMB4    */ CLK(5); OP_RMB   ( ZPG, BIT_4   ); break; /* C */
			case 0x48: /* PHA     */ CLK(2); OP_PUSH  ( REG_A        ); break;
			case 0x49: /* EOR imm */ CLK(2); OP_EOR   ( IMM          ); break;
			case 0x4a: /* LSR acc */ CLK(2); OP_LSR_A (              ); break;
			case 0x4c: /* JMP abs */ CLK(3); OP_JMP   ( ABS          ); break;
			case 0x4d: /* EOR abs */ CLK(4); OP_EOR   ( ABS          ); break;
			case 0x4e: /* LSR abs */ CLK(6); OP_LSR_M ( ABS          ); break;
			case 0x4f: /* BBR4    */ CLK(5); OP_BBR   ( BIT_4        ); break; /* C */
			case 0x50: /* BVC     */ CLK(2); OP_BCC   ( COND_VC()    ); break;
			case 0x51: /* EOR idy */ CLK(5); OP_EOR   ( IDY_P        ); break;
			case 0x52: /* EOR zpi */ CLK(3); OP_EOR   ( ZPI          ); break; /* C */
			case 0x55: /* EOR zpx */ CLK(4); OP_EOR   ( ZPX          ); break;
			case 0x56: /* LSR zpx */ CLK(6); OP_LSR_M ( ZPX          ); break;
			case 0x57: /* RMB5    */ CLK(5); OP_RMB   ( ZPG, BIT_5   ); break; /* C */
			case 0x58: /* CLI     */ CLK(2); OP_CLI   (              ); break;
			case 0x59: /* EOR aby */ CLK(4); OP_EOR   ( ABY_P        ); break;
			case 0x5a: /* PHY     */ CLK(3); OP_PUSH  ( REG_Y        ); break; /* C */
			case 0x5d: /* EOR abx */ CLK(4); OP_EOR   ( ABX_P        ); break;
			case 0x5e: /* LSR abx */ CLK(7); OP_LSR_M ( ABX          ); break;
			case 0x5f: /* BBR5    */ CLK(5); OP_BBR   ( BIT_5        ); break; /* C */
			case 0x60: /* RTS     */ CLK(6); OP_RTS   (              ); break;
			case 0x61: /* ADC idx */ CLK(6); OP_ADC   ( IDX          ); break;
			case 0x64: /* STZ zp  */ CLK(2); OP_ST    ( 0, ZPG       ); break; /* C */
			case 0x65: /* ADC zp  */ CLK(3); OP_ADC   ( ZPG          ); break;
			case 0x66: /* ROR zp  */ CLK(5); OP_ROR_M ( ZPG          ); break;
			case 0x67: /* RMB6    */ CLK(5); OP_RMB   ( ZPG, BIT_6   ); break; /* C */
			case 0x68: /* PLA     */ CLK(2); OP_PLA   (              ); break;
			case 0x69: /* ADC imm */ CLK(2); OP_ADC   ( IMM          ); break;
			case 0x6a: /* ROR acc */ CLK(2); OP_ROR_A (              ); break;
			case 0x6c: /* JMP ind */ CLK(5); OP_JMP   ( IND          ); break;
			case 0x6d: /* ADC abs */ CLK(4); OP_ADC   ( ABS          ); break;
			case 0x6e: /* ROR abs */ CLK(6); OP_ROR_M ( ABS          ); break;
			case 0x6f: /* BBR6    */ CLK(5); OP_BBR   ( BIT_6        ); break; /* C */
			case 0x70: /* BVS     */ CLK(2); OP_BCC   ( COND_VS()    ); break;
			case 0x71: /* ADC idy */ CLK(5); OP_ADC   ( IDY_P        ); break;
			case 0x72: /* ADC zpi */ CLK(3); OP_ADC   ( ZPI          ); break; /* C */
			case 0x74: /* STZ zpx */ CLK(4); OP_ST    ( 0, ZPX       ); break; /* C */
			case 0x75: /* ADC zpx */ CLK(4); OP_ADC   ( ZPX          ); break;
			case 0x76: /* ROR zpx */ CLK(6); OP_ROR_M ( ZPX          ); break;
			case 0x77: /* RMB7    */ CLK(5); OP_RMB   ( ZPG, BIT_7   ); break; /* C */
			case 0x78: /* SEI     */ CLK(2); OP_SEI   (              ); break;
			case 0x79: /* ADC aby */ CLK(4); OP_ADC   ( ABY_P        ); break;
			case 0x7a: /* PLY     */ CLK(4); OP_PULL  ( REG_Y        ); break; /* C */
			case 0x7c: /* JMP iax */ CLK(2); OP_JMP   ( IAX          ); break; /* C */
			case 0x7d: /* ADC abx */ CLK(4); OP_ADC   ( ABX_P        ); break;
			case 0x7e: /* ROR abx */ CLK(7); OP_ROR_M ( ABX          ); break;
			case 0x7f: /* BBR7    */ CLK(5); OP_BBR   ( BIT_7        ); break; /* C */
			case 0x80: /* BRA     */ CLK(3); OP_BRA   (              ); break; /* C */
			case 0x81: /* STA idx */ CLK(6); OP_ST    ( REG_A, IDX   ); break;
			case 0x84: /* STY zp  */ CLK(3); OP_ST    ( REG_Y, ZPG   ); break;
			case 0x85: /* STA zp  */ CLK(3); OP_ST    ( REG_A, ZPG   ); break;
			case 0x86: /* STX zp  */ CLK(3); OP_ST    ( REG_X, ZPG   ); break;
			case 0x87: /* SMB0    */ CLK(5); OP_SMB   ( ZPG, BIT_0   ); break; /* C */
			case 0x88: /* DEY     */ CLK(2); OP_DEC_R ( REG_Y        ); break;
			case 0x89: /* BIT imm */ CLK(2); OP_BIT   ( IMM          ); break; /* C */
			case 0x8a: /* TXA     */ CLK(2); OP_TRANS ( REG_X, REG_A ); break;
			case 0x8c: /* STY abs */ CLK(4); OP_ST    ( REG_Y, ABS   ); break;
			case 0x8d: /* STA abs */ CLK(4); OP_ST    ( REG_A, ABS   ); break;
			case 0x8e: /* STX abs */ CLK(5); OP_ST    ( REG_X, ABS   ); break;
			case 0x8f: /* BBS0    */ CLK(5); OP_BBS   ( BIT_0        ); break; /* C */
			case 0x90: /* BCC     */ CLK(2); OP_BCC   ( COND_CC()    ); break;
			case 0x91: /* STA idy */ CLK(5); OP_ST    ( REG_A, IDY   ); break;
			case 0x92: /* STA zpi */ CLK(4); OP_ST    ( REG_A, ZPI   ); break; /* C */
			case 0x94: /* STY zpx */ CLK(4); OP_ST    ( REG_Y, ZPX   ); break;
			case 0x95: /* STA zpx */ CLK(4); OP_ST    ( REG_A, ZPX   ); break;
			case 0x96: /* STX zpy */ CLK(4); OP_ST    ( REG_X, ZPY   ); break;
			case 0x97: /* SMB1    */ CLK(5); OP_SMB   ( ZPG, BIT_1   ); break; /* C */
			case 0x98: /* TYA     */ CLK(2); OP_TRANS ( REG_Y, REG_A ); break;
			case 0x99: /* STA aby */ CLK(5); OP_ST    ( REG_A, ABY   ); break;
			case 0x9a: /* TXS     */ CLK(2); OP_TXS   (              ); break;
			case 0x9c: /* STZ abs */ CLK(4); OP_ST    ( 0, ABS       ); break; /* C */
			case 0x9d: /* STA abx */ CLK(5); OP_ST    ( REG_A, ABX   ); break;
			case 0x9e: /* STZ abx */ CLK(5); OP_ST    ( 0, ABX       ); break; /* C */
			case 0x9f: /* BBS1    */ CLK(5); OP_BBS   ( BIT_1        ); break; /* C */
			case 0xa0: /* LDY imm */ CLK(2); OP_LD    ( REG_Y, IMM   ); break;
			case 0xa1: /* LDA idx */ CLK(6); OP_LD    ( REG_A, IDX   ); break;
			case 0xa2: /* LDX imm */ CLK(2); OP_LD    ( REG_X, IMM   ); break;
			case 0xa4: /* LDY zp  */ CLK(3); OP_LD    ( REG_Y, ZPG   ); break;
			case 0xa5: /* LDA zp  */ CLK(3); OP_LD    ( REG_A, ZPG   ); break;
			case 0xa6: /* LDX zp  */ CLK(3); OP_LD    ( REG_X, ZPG   ); break;
			case 0xa7: /* SMB2    */ CLK(5); OP_SMB   ( ZPG, BIT_2   ); break; /* C */
			case 0xa8: /* TAY     */ CLK(2); OP_TRANS ( REG_A, REG_Y ); break;
			case 0xa9: /* LDA imm */ CLK(2); OP_LD    ( REG_A, IMM   ); break;
			case 0xaa: /* TAX     */ CLK(2); OP_TRANS ( REG_A, REG_X ); break;
			case 0xac: /* LDY abs */ CLK(4); OP_LD    ( REG_Y, ABS   ); break;
			case 0xad: /* LDA abs */ CLK(4); OP_LD    ( REG_A, ABS   ); break;
			case 0xae: /* LDX abs */ CLK(4); OP_LD    ( REG_X, ABS   ); break;
			case 0xaf: /* BBS2    */ CLK(5); OP_BBS   ( BIT_2        ); break; /* C */
			case 0xb0: /* BCS     */ CLK(2); OP_BCC   ( COND_CS()    ); break;
			case 0xb1: /* LDA idy */ CLK(5); OP_LD    ( REG_A, IDY_P ); break;
			case 0xb2: /* LDA zpi */ CLK(3); OP_LD    ( REG_A, ZPI   ); break; /* C */
			case 0xb4: /* LDY zpx */ CLK(4); OP_LD    ( REG_Y, ZPX   ); break;
			case 0xb5: /* LDA zpx */ CLK(4); OP_LD    ( REG_A, ZPX   ); break;
			case 0xb6: /* LDX zpy */ CLK(4); OP_LD    ( REG_X, ZPY   ); break;
			case 0xb7: /* SMB3    */ CLK(5); OP_SMB   ( ZPG, BIT_3   ); break; /* C */
			case 0xb8: /* CLV     */ CLK(2); OP_CLV   (              ); break;
			case 0xb9: /* LDA aby */ CLK(4); OP_LD    ( REG_A, ABY_P ); break;
			case 0xba: /* TSX     */ CLK(2); OP_TRANS ( REG_S, REG_X ); break;
			case 0xbc: /* LDY abx */ CLK(4); OP_LD    ( REG_Y, ABX_P ); break;
			case 0xbd: /* LDA abx */ CLK(4); OP_LD    ( REG_A, ABX_P ); break;
			case 0xbe: /* LDX aby */ CLK(4); OP_LD    ( REG_X, ABY_P ); break;
			case 0xbf: /* BBS3    */ CLK(5); OP_BBS   ( BIT_3        ); break; /* C */
			case 0xc0: /* CPY imm */ CLK(2); OP_CMP   ( REG_Y, IMM   ); break;
			case 0xc1: /* CMP idx */ CLK(6); OP_CMP   ( REG_A, IDX   ); break;
			case 0xc4: /* CPY zp  */ CLK(3); OP_CMP   ( REG_Y, ZPG   ); break;
			case 0xc5: /* CMP zp  */ CLK(3); OP_CMP   ( REG_A, ZPG   ); break;
			case 0xc6: /* DEC zp  */ CLK(5); OP_DEC_M ( ZPG          ); break;
			case 0xc7: /* SMB4    */ CLK(5); OP_SMB   ( ZPG, BIT_4   ); break; /* C */
			case 0xc8: /* INY     */ CLK(2); OP_INC_R ( REG_Y        ); break;
			case 0xc9: /* CMP imm */ CLK(2); OP_CMP   ( REG_A, IMM   ); break;
			case 0xca: /* DEX     */ CLK(2); OP_DEC_R ( REG_X        ); break;
			case 0xcc: /* CPY abs */ CLK(4); OP_CMP   ( REG_Y, ABS   ); break;
			case 0xcd: /* CMP abs */ CLK(4); OP_CMP   ( REG_A, ABS   ); break;
			case 0xce: /* DEC abs */ CLK(6); OP_DEC_M ( ABS          ); break;
			case 0xcf: /* BBS4    */ CLK(5); OP_BBS   ( BIT_4        ); break; /* C */
			case 0xd0: /* BNE     */ CLK(2); OP_BCC   ( COND_NE()    ); break;
			case 0xd1: /* CMP idy */ CLK(5); OP_CMP   ( REG_A, IDY_P ); break;
			case 0xd2: /* CMP zpi */ CLK(3); OP_CMP   ( REG_A, ZPI   ); break; /* C */
			case 0xd5: /* CMP zpx */ CLK(4); OP_CMP   ( REG_A, ZPX   ); break;
			case 0xd6: /* DEC zpx */ CLK(6); OP_DEC_M ( ZPX          ); break;
			case 0xd7: /* SMB5    */ CLK(5); OP_SMB   ( ZPG, BIT_5   ); break; /* C */
			case 0xd8: /* CLD     */ CLK(2); OP_CLD   (              ); break;
			case 0xd9: /* CMP aby */ CLK(4); OP_CMP   ( REG_A, ABY_P ); break;
			case 0xda: /* PHX     */ CLK(3); OP_PUSH  ( REG_X        ); break; /* C */
			case 0xdd: /* CMP abx */ CLK(4); OP_CMP   ( REG_A, ABX_P ); break;
			case 0xde: /* DEC abx */ CLK(7); OP_DEC_M ( ABX          ); break;
			case 0xdf: /* BBS5    */ CLK(5); OP_BBS   ( BIT_5        ); break; /* C */
			case 0xe0: /* CPX imm */ CLK(2); OP_CMP   ( REG_X, IMM   ); break;
			case 0xe1: /* SBC idx */ CLK(6); OP_SBC   ( IDX          ); break;
			case 0xe4: /* CPX zp  */ CLK(3); OP_CMP   ( REG_X, ZPG   ); break;
			case 0xe5: /* SBC zp  */ CLK(3); OP_SBC   ( ZPG          ); break;
			case 0xe6: /* INC zp  */ CLK(5); OP_INC_M ( ZPG          ); break;
			case 0xe7: /* SMB6    */ CLK(5); OP_SMB   ( ZPG, BIT_6   ); break; /* C */
			case 0xe8: /* INX     */ CLK(2); OP_INC_R ( REG_X        ); break;
			case 0xe9: /* SBC imm */ CLK(2); OP_SBC   ( IMM          ); break;
			case 0xea: /* NOP     */ CLK(2); OP_NOP   (              ); break;
			case 0xec: /* CPX abs */ CLK(4); OP_CMP   ( REG_X, ABS   ); break;
			case 0xed: /* SBC abs */ CLK(4); OP_SBC   ( ABS          ); break;
			case 0xee: /* INC abs */ CLK(6); OP_INC_M ( ABS          ); break;
			case 0xef: /* BBS6    */ CLK(5); OP_BBS   ( BIT_6        ); break; /* C */
			case 0xf0: /* BEQ     */ CLK(2); OP_BCC   ( COND_EQ()    ); break;
			case 0xf1: /* SBC idy */ CLK(5); OP_SBC   ( IDY          ); break;
			case 0xf2: /* SBC zpi */ CLK(3); OP_SBC   ( ZPI          ); break; /* C */
			case 0xf5: /* SBC zpx */ CLK(4); OP_SBC   ( ZPX          ); break;
			case 0xf6: /* INC zpx */ CLK(6); OP_INC_M ( ZPX          ); break;
			case 0xf7: /* SMB7    */ CLK(5); OP_SMB   ( ZPG, BIT_7   ); break; /* C */
			case 0xf8: /* SED     */ CLK(2); OP_SED   (              ); break;
			case 0xf9: /* SBC aby */ CLK(4); OP_SBC   ( ABY_P        ); break;
			case 0xfa: /* PLX     */ CLK(4); OP_PULL  ( REG_X        ); break; /* C */
			case 0xfd: /* SBC abx */ CLK(4); OP_SBC   ( ABX_P        ); break;
			case 0xfe: /* INC abx */ CLK(7); OP_INC_M ( ABX          ); break;
			case 0xff: /* BBS7    */ CLK(5); OP_BBS   ( BIT_7        ); break; /* C */
			default:   /* illegal */ CLK(2); OP_ILLEGAL();           break;
		}
#if M6502_STRICT_IRQ
		if(!IRQ_DELAY && LINE_IRQ && !FLAG_I)
			SERVICE_IRQ();
#endif /* M6502_STRICT_IRQ */
	}
	return clocks - CLOCKS;
}

#endif /* HAS_M65C02 */


#if HAS_M6510

/* Execute instructions for <clocks> cycles */
int m6510_execute(int clocks)
{
	CLOCKS = LINE_RST ? 0 : clocks;
	while(CLOCKS > 0)
	{
#if M6502_STRICT_IRQ
		IRQ_DELAY = 0;
#endif /* M6502_STRICT_IRQ */

		REG_PPC = REG_PC;

		CALL_MAME_DEBUG;

		REG_PC++;

		switch(REG_IR = read_8_instruction(REG_PPC))
		{
			case 0x00: /* BRK     */ CLK(7); OP_BRK   (              ); break;
			case 0x01: /* ORA idx */ CLK(6); OP_ORA   ( IDX          ); break;
			case 0x03: /* SLO idx */ CLK(7); OP_SLO   ( IDX          ); break; /* 10 */
			case 0x04: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0x05: /* ORA zp  */ CLK(3); OP_ORA   ( ZPG          ); break;
			case 0x06: /* ASL zp  */ CLK(5); OP_ASL_M ( ZPG          ); break;
			case 0x07: /* SLO zpg */ CLK(5); OP_SLO   ( ZPG          ); break; /* 10 */
			case 0x08: /* PHP     */ CLK(2); OP_PHP   (              ); break;
			case 0x09: /* ORA imm */ CLK(2); OP_ORA   ( IMM          ); break;
			case 0x0a: /* ASL acc */ CLK(2); OP_ASL_A (              ); break;
			case 0x0b: /* ANC imm */ CLK(2); OP_ANC   ( IMM          ); break; /* 10 */
			case 0x0c: /* TOP     */ CLK(2); OP_TOP   (              ); break; /* 10 */
			case 0x0d: /* ORA abs */ CLK(4); OP_ORA   ( ABS          ); break;
			case 0x0e: /* ASL abs */ CLK(6); OP_ASL_M ( ABS          ); break;
			case 0x0f: /* SLO abs */ CLK(6); OP_SLO   ( ABS          ); break; /* 10 */
			case 0x10: /* BPL     */ CLK(2); OP_BCC   ( COND_PL()    ); break;
			case 0x11: /* ORA idy */ CLK(5); OP_ORA   ( IDY          ); break;
			case 0x13: /* SLO idy */ CLK(6); OP_SLO   ( IDY          ); break; /* 10 */
			case 0x14: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0x15: /* ORA zpx */ CLK(4); OP_ORA   ( ZPX          ); break;
			case 0x16: /* ASL zpx */ CLK(6); OP_ASL_M ( ZPX          ); break;
			case 0x17: /* SLO zpx */ CLK(6); OP_SLO   ( ZPX          ); break; /* 10 */
			case 0x18: /* CLC     */ CLK(2); OP_CLC   (              ); break;
			case 0x19: /* ORA aby */ CLK(4); OP_ORA   ( ABY_P        ); break;
			case 0x1a: /* NOP     */ CLK(2); OP_NOP   (              ); break; /* 10 */
			case 0x1b: /* SLO aby */ CLK(4); OP_SLO   ( ABY          ); break; /* 10 */
			case 0x1c: /* TOP     */ CLK(2); OP_TOP   (              ); break; /* 10 */
			case 0x1d: /* ORA abx */ CLK(4); OP_ORA   ( ABX_P        ); break;
			case 0x1e: /* ASL abx */ CLK(7); OP_ASL_M ( ABX          ); break;
			case 0x1f: /* SLO abx */ CLK(4); OP_SLO   ( ABX          ); break; /* 10 */
			case 0x20: /* JSR     */ CLK(6); OP_JSR   ( ABS          ); break;
			case 0x21: /* AND idx */ CLK(6); OP_AND   ( IDX          ); break;
			case 0x23: /* RLA idx */ CLK(7); OP_RLA   ( IDX          ); break; /* 10 */
			case 0x24: /* BIT zp  */ CLK(3); OP_BIT   ( ZPG          ); break;
			case 0x25: /* AND zp  */ CLK(3); OP_AND   ( ZPG          ); break;
			case 0x26: /* ROL zp  */ CLK(5); OP_ROL_M ( ZPG          ); break;
			case 0x27: /* RLA zpg */ CLK(5); OP_RLA   ( ZPG          ); break; /* 10 */
			case 0x28: /* PLP     */ CLK(2); OP_PLP   (              ); break;
			case 0x29: /* AND imm */ CLK(2); OP_AND   ( IMM          ); break;
			case 0x2a: /* ROL acc */ CLK(2); OP_ROL_A (              ); break;
			case 0x2b: /* ANC imm */ CLK(2); OP_ANC   ( IMM          ); break; /* 10 */
			case 0x2c: /* BIT abs */ CLK(4); OP_BIT   ( ABS          ); break;
			case 0x2d: /* AND abs */ CLK(4); OP_AND   ( ABS          ); break;
			case 0x2e: /* ROL abs */ CLK(6); OP_ROL_M ( ABS          ); break;
			case 0x2f: /* RLA abs */ CLK(6); OP_RLA   ( ABS          ); break; /* 10 */
			case 0x30: /* BMI     */ CLK(2); OP_BCC   ( COND_MI()    ); break;
			case 0x31: /* AND idy */ CLK(5); OP_AND   ( IDY          ); break;
			case 0x33: /* RLA idy */ CLK(6); OP_RLA   ( IDY          ); break; /* 10 */
			case 0x34: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0x35: /* AND zpx */ CLK(4); OP_AND   ( ZPX          ); break;
			case 0x36: /* ROL zpx */ CLK(6); OP_ROL_M ( ZPX          ); break;
			case 0x37: /* RLA zpx */ CLK(6); OP_RLA   ( ZPX          ); break; /* 10 */
			case 0x38: /* SEC     */ CLK(2); OP_SEC   (              ); break;
			case 0x39: /* AND aby */ CLK(4); OP_AND   ( ABY_P        ); break;
			case 0x3a: /* NOP     */ CLK(2); OP_NOP   (              ); break; /* 10 */
			case 0x3b: /* RLA aby */ CLK(4); OP_RLA   ( ABY          ); break; /* 10 */
			case 0x3c: /* TOP     */ CLK(2); OP_TOP   (              ); break; /* 10 */
			case 0x3d: /* AND abx */ CLK(4); OP_AND   ( ABX_P        ); break;
			case 0x3e: /* ROL abx */ CLK(7); OP_ROL_M ( ABX          ); break;
			case 0x3f: /* RLA abx */ CLK(4); OP_RLA   ( ABX          ); break; /* 10 */
			case 0x40: /* RTI     */ CLK(6); OP_RTI   (              ); break;
			case 0x41: /* EOR idx */ CLK(6); OP_EOR   ( IDX          ); break;
			case 0x43: /* SRE idx */ CLK(7); OP_SRE   ( IDX          ); break; /* 10 */
			case 0x44: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0x45: /* EOR zp  */ CLK(3); OP_EOR   ( ZPG          ); break;
			case 0x46: /* LSR zp  */ CLK(5); OP_LSR_M ( ZPG          ); break;
			case 0x47: /* SRE zpg */ CLK(5); OP_SRE   ( ZPG          ); break; /* 10 */
			case 0x48: /* PHA     */ CLK(2); OP_PUSH  ( REG_A        ); break;
			case 0x49: /* EOR imm */ CLK(2); OP_EOR   ( IMM          ); break;
			case 0x4a: /* LSR acc */ CLK(2); OP_LSR_A (              ); break;
			case 0x4b: /* ASR imm */ CLK(2); OP_ASR   ( IMM          ); break; /* 10 */
			case 0x4c: /* JMP abs */ CLK(3); OP_JMP   ( ABS          ); break;
			case 0x4d: /* EOR abs */ CLK(4); OP_EOR   ( ABS          ); break;
			case 0x4e: /* LSR abs */ CLK(6); OP_LSR_M ( ABS          ); break;
			case 0x4f: /* SRE abs */ CLK(6); OP_SRE   ( ABS          ); break; /* 10 */
			case 0x50: /* BVC     */ CLK(2); OP_BCC   ( COND_VC()    ); break;
			case 0x51: /* EOR idy */ CLK(5); OP_EOR   ( IDY_P        ); break;
			case 0x53: /* SRE idy */ CLK(6); OP_SRE   ( IDY          ); break; /* 10 */
			case 0x54: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0x55: /* EOR zpx */ CLK(4); OP_EOR   ( ZPX          ); break;
			case 0x56: /* LSR zpx */ CLK(6); OP_LSR_M ( ZPX          ); break;
			case 0x57: /* SRE zpx */ CLK(6); OP_SRE   ( ZPX          ); break; /* 10 */
			case 0x58: /* CLI     */ CLK(2); OP_CLI   (              ); break;
			case 0x59: /* EOR aby */ CLK(4); OP_EOR   ( ABY_P        ); break;
			case 0x5a: /* NOP     */ CLK(2); OP_NOP   (              ); break; /* 10 */
			case 0x5b: /* SRE aby */ CLK(4); OP_SRE   ( ABY          ); break; /* 10 */
			case 0x5c: /* TOP     */ CLK(2); OP_TOP   (              ); break; /* 10 */
			case 0x5d: /* EOR abx */ CLK(4); OP_EOR   ( ABX_P        ); break;
			case 0x5e: /* LSR abx */ CLK(7); OP_LSR_M ( ABX          ); break;
			case 0x5f: /* SRE abx */ CLK(4); OP_SRE   ( ABX          ); break; /* 10 */
			case 0x60: /* RTS     */ CLK(6); OP_RTS   (              ); break;
			case 0x61: /* ADC idx */ CLK(6); OP_ADC   ( IDX          ); break;
			case 0x63: /* RRA idx */ CLK(7); OP_RRA   ( IDX          ); break; /* 10 */
			case 0x64: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0x65: /* ADC zp  */ CLK(3); OP_ADC   ( ZPG          ); break;
			case 0x66: /* ROR zp  */ CLK(5); OP_ROR_M ( ZPG          ); break;
			case 0x67: /* RRA zpg */ CLK(5); OP_RRA   ( ZPG          ); break; /* 10 */
			case 0x68: /* PLA     */ CLK(2); OP_PLA   (              ); break;
			case 0x69: /* ADC imm */ CLK(2); OP_ADC   ( IMM          ); break;
			case 0x6a: /* ROR acc */ CLK(2); OP_ROR_A (              ); break;
			case 0x6b: /* ARR imm */ CLK(2); OP_ARR   ( IMM          ); break; /* 10 */
			case 0x6c: /* JMP ind */ CLK(5); OP_JMP   ( IND          ); break;
			case 0x6d: /* ADC abs */ CLK(4); OP_ADC   ( ABS          ); break;
			case 0x6e: /* ROR abs */ CLK(6); OP_ROR_M ( ABS          ); break;
			case 0x6f: /* RRA abs */ CLK(6); OP_RRA   ( ABS          ); break; /* 10 */
			case 0x70: /* BVS     */ CLK(2); OP_BCC   ( COND_VS()    ); break;
			case 0x71: /* ADC idy */ CLK(5); OP_ADC   ( IDY_P        ); break;
			case 0x73: /* RRA idy */ CLK(6); OP_RRA   ( IDY          ); break; /* 10 */
			case 0x74: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0x75: /* ADC zpx */ CLK(4); OP_ADC   ( ZPX          ); break;
			case 0x76: /* ROR zpx */ CLK(6); OP_ROR_M ( ZPX          ); break;
			case 0x77: /* RRA zpx */ CLK(6); OP_RRA   ( ZPX          ); break; /* 10 */
			case 0x78: /* SEI     */ CLK(2); OP_SEI   (              ); break;
			case 0x79: /* ADC aby */ CLK(4); OP_ADC   ( ABY_P        ); break;
			case 0x7a: /* NOP     */ CLK(2); OP_NOP   (              ); break; /* 10 */
			case 0x7b: /* RRA aby */ CLK(4); OP_RRA   ( ABY          ); break; /* 10 */
			case 0x7c: /* TOP     */ CLK(2); OP_TOP   (              ); break; /* 10 */
			case 0x7d: /* ADC abx */ CLK(4); OP_ADC   ( ABX_P        ); break;
			case 0x7e: /* ROR abx */ CLK(7); OP_ROR_M ( ABX          ); break;
			case 0x7f: /* RRA abx */ CLK(4); OP_RRA   ( ABX          ); break; /* 10 */
			case 0x80: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0x81: /* STA idx */ CLK(6); OP_ST    ( REG_A, IDX   ); break;
			case 0x82: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0x83: /* SAX idx */ CLK(6); OP_SAX   ( IDX          ); break; /* 10 */
			case 0x84: /* STY zp  */ CLK(3); OP_ST    ( REG_Y, ZPG   ); break;
			case 0x85: /* STA zp  */ CLK(3); OP_ST    ( REG_A, ZPG   ); break;
			case 0x86: /* STX zp  */ CLK(3); OP_ST    ( REG_X, ZPG   ); break;
			case 0x87: /* SAX zpg */ CLK(3); OP_SAX   ( ZPG          ); break; /* 10 */
			case 0x88: /* DEY     */ CLK(2); OP_DEC_R ( REG_Y        ); break;
			case 0x89: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0x8a: /* TXA     */ CLK(2); OP_TRANS ( REG_X, REG_A ); break;
			case 0x8b: /* AXA imm */ CLK(2); OP_AXA   ( IMM          ); break; /* 10 */
			case 0x8c: /* STY abs */ CLK(4); OP_ST    ( REG_Y, ABS   ); break;
			case 0x8d: /* STA abs */ CLK(4); OP_ST    ( REG_A, ABS   ); break;
			case 0x8e: /* STX abs */ CLK(5); OP_ST    ( REG_X, ABS   ); break;
			case 0x8f: /* SAX abs */ CLK(4); OP_SAX   ( ABS          ); break; /* 10 */
			case 0x90: /* BCC     */ CLK(2); OP_BCC   ( COND_CC()    ); break;
			case 0x91: /* STA idy */ CLK(5); OP_ST    ( REG_A, IDY   ); break;
			case 0x93: /* SAX idy */ CLK(5); OP_SAX   ( IDY          ); break; /* 10 */
			case 0x94: /* STY zpx */ CLK(4); OP_ST    ( REG_Y, ZPX   ); break;
			case 0x95: /* STA zpx */ CLK(4); OP_ST    ( REG_A, ZPX   ); break;
			case 0x96: /* STX zpy */ CLK(4); OP_ST    ( REG_X, ZPY   ); break;
			case 0x97: /* SAX zpx */ CLK(4); OP_SAX   ( ZPX          ); break; /* 10 */
			case 0x98: /* TYA     */ CLK(2); OP_TRANS ( REG_Y, REG_A ); break;
			case 0x99: /* STA aby */ CLK(5); OP_ST    ( REG_A, ABY   ); break;
			case 0x9a: /* TXS     */ CLK(2); OP_TXS   (              ); break;
			case 0x9b: /* SSH aby */ CLK(5); OP_SSH   ( ABY          ); break; /* 10 */
			case 0x9c: /* SYH abx */ CLK(5); OP_SYH   ( ABX          ); break; /* 10 */
			case 0x9d: /* STA abx */ CLK(5); OP_ST    ( REG_A, ABX   ); break;
			case 0x9f: /* SAX aby */ CLK(6); OP_SAX   ( ABY          ); break; /* 10 */
			case 0xa0: /* LDY imm */ CLK(2); OP_LD    ( REG_Y, IMM   ); break;
			case 0xa1: /* LDA idx */ CLK(6); OP_LD    ( REG_A, IDX   ); break;
			case 0xa2: /* LDX imm */ CLK(2); OP_LD    ( REG_X, IMM   ); break;
			case 0xa3: /* LAX idx */ CLK(6); OP_LAX   ( IDX          ); break; /* 10 */
			case 0xa4: /* LDY zp  */ CLK(3); OP_LD    ( REG_Y, ZPG   ); break;
			case 0xa5: /* LDA zp  */ CLK(3); OP_LD    ( REG_A, ZPG   ); break;
			case 0xa6: /* LDX zp  */ CLK(3); OP_LD    ( REG_X, ZPG   ); break;
			case 0xa7: /* LAX zpg */ CLK(3); OP_LAX   ( ZPG          ); break; /* 10 */
			case 0xa8: /* TAY     */ CLK(2); OP_TRANS ( REG_A, REG_Y ); break;
			case 0xa9: /* LDA imm */ CLK(2); OP_LD    ( REG_A, IMM   ); break;
			case 0xaa: /* TAX     */ CLK(2); OP_TRANS ( REG_A, REG_X ); break;
			case 0xab: /* LAX imm */ CLK(2); OP_LAX   ( IMM          ); break; /* 10 */
			case 0xac: /* LDY abs */ CLK(4); OP_LD    ( REG_Y, ABS   ); break;
			case 0xad: /* LDA abs */ CLK(4); OP_LD    ( REG_A, ABS   ); break;
			case 0xae: /* LDX abs */ CLK(4); OP_LD    ( REG_X, ABS   ); break;
			case 0xaf: /* LAX abs */ CLK(5); OP_LAX   ( ABS          ); break; /* 10 */
			case 0xb0: /* BCS     */ CLK(2); OP_BCC   ( COND_CS()    ); break;
			case 0xb1: /* LDA idy */ CLK(5); OP_LD    ( REG_A, IDY_P ); break;
			case 0xb3: /* LAX idy */ CLK(5); OP_LAX   ( IDY          ); break; /* 10 */
			case 0xb4: /* LDY zpx */ CLK(4); OP_LD    ( REG_Y, ZPX   ); break;
			case 0xb5: /* LDA zpx */ CLK(4); OP_LD    ( REG_A, ZPX   ); break;
			case 0xb6: /* LDX zpy */ CLK(4); OP_LD    ( REG_X, ZPY   ); break;
			case 0xb7: /* LAX zpx */ CLK(4); OP_LAX   ( ZPX          ); break; /* 10 */
			case 0xb8: /* CLV     */ CLK(2); OP_CLV   (              ); break;
			case 0xb9: /* LDA aby */ CLK(4); OP_LD    ( REG_A, ABY_P ); break;
			case 0xba: /* TSX     */ CLK(2); OP_TRANS ( REG_S, REG_X ); break;
			case 0xbb: /* AST aby */ CLK(4); OP_AST   ( ABY          ); break; /* 10 */
			case 0xbc: /* LDY abx */ CLK(4); OP_LD    ( REG_Y, ABX_P ); break;
			case 0xbd: /* LDA abx */ CLK(4); OP_LD    ( REG_A, ABX_P ); break;
			case 0xbe: /* LDX aby */ CLK(4); OP_LD    ( REG_X, ABY_P ); break;
			case 0xbf: /* LAX aby */ CLK(6); OP_LAX   ( ABY          ); break; /* 10 */
			case 0xc0: /* CPY imm */ CLK(2); OP_CMP   ( REG_Y, IMM   ); break;
			case 0xc1: /* CMP idx */ CLK(6); OP_CMP   ( REG_A, IDX   ); break;
			case 0xc2: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0xc3: /* DCP idx */ CLK(7); OP_DCP   ( IDX          ); break; /* 10 */
			case 0xc4: /* CPY zp  */ CLK(3); OP_CMP   ( REG_Y, ZPG   ); break;
			case 0xc5: /* CMP zp  */ CLK(3); OP_CMP   ( REG_A, ZPG   ); break;
			case 0xc6: /* DEC zp  */ CLK(5); OP_DEC_M ( ZPG          ); break;
			case 0xc7: /* DCP zpg */ CLK(5); OP_DCP   ( ZPG          ); break; /* 10 */
			case 0xc8: /* INY     */ CLK(2); OP_INC_R ( REG_Y        ); break;
			case 0xc9: /* CMP imm */ CLK(2); OP_CMP   ( REG_A, IMM   ); break;
			case 0xca: /* DEX     */ CLK(2); OP_DEC_R ( REG_X        ); break;
			case 0xcb: /* ASX imm */ CLK(2); OP_ASX   ( IMM          ); break; /* 10 */
			case 0xcc: /* CPY abs */ CLK(4); OP_CMP   ( REG_Y, ABS   ); break;
			case 0xcd: /* CMP abs */ CLK(4); OP_CMP   ( REG_A, ABS   ); break;
			case 0xce: /* DEC abs */ CLK(6); OP_DEC_M ( ABS          ); break;
			case 0xcf: /* DCP abs */ CLK(6); OP_DCP   ( ABS          ); break; /* 10 */
			case 0xd0: /* BNE     */ CLK(2); OP_BCC   ( COND_NE()    ); break;
			case 0xd1: /* CMP idy */ CLK(5); OP_CMP   ( REG_A, IDY_P ); break;
			case 0xd3: /* DCP idy */ CLK(6); OP_DCP   ( IDY          ); break; /* 10 */
			case 0xd4: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0xd5: /* CMP zpx */ CLK(4); OP_CMP   ( REG_A, ZPX   ); break;
			case 0xd6: /* DEC zpx */ CLK(6); OP_DEC_M ( ZPX          ); break;
			case 0xd7: /* DCP zpx */ CLK(6); OP_DCP   ( ZPX          ); break; /* 10 */
			case 0xd8: /* CLD     */ CLK(2); OP_CLD   (              ); break;
			case 0xd9: /* CMP aby */ CLK(4); OP_CMP   ( REG_A, ABY_P ); break;
			case 0xda: /* NOP     */ CLK(2); OP_NOP   (              ); break; /* 10 */
			case 0xdb: /* DCP aby */ CLK(6); OP_DCP   ( ABY          ); break; /* 10 */
			case 0xdc: /* TOP     */ CLK(2); OP_TOP   (              ); break; /* 10 */
			case 0xdd: /* CMP abx */ CLK(4); OP_CMP   ( REG_A, ABX_P ); break;
			case 0xde: /* DEC abx */ CLK(7); OP_DEC_M ( ABX          ); break;
			case 0xdf: /* DCP abx */ CLK(7); OP_DCP   ( ABX          ); break; /* 10 */
			case 0xe0: /* CPX imm */ CLK(2); OP_CMP   ( REG_X, IMM   ); break;
			case 0xe1: /* SBC idx */ CLK(6); OP_SBC   ( IDX          ); break;
			case 0xe2: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0xe3: /* ISB idx */ CLK(7); OP_ISB   ( IDX          ); break; /* 10 */
			case 0xe4: /* CPX zp  */ CLK(3); OP_CMP   ( REG_X, ZPG   ); break;
			case 0xe5: /* SBC zp  */ CLK(3); OP_SBC   ( ZPG          ); break;
			case 0xe6: /* INC zp  */ CLK(5); OP_INC_M ( ZPG          ); break;
			case 0xe7: /* ISB zpg */ CLK(5); OP_ISB   ( ZPG          ); break; /* 10 */
			case 0xe8: /* INX     */ CLK(2); OP_INC_R ( REG_X        ); break;
			case 0xe9: /* SBC imm */ CLK(2); OP_SBC   ( IMM          ); break;
			case 0xea: /* NOP     */ CLK(2); OP_NOP   (              ); break;
			case 0xeb: /* SBC imm */ CLK(2); OP_SBC   ( IMM          ); break; /* 10 */
			case 0xec: /* CPX abs */ CLK(4); OP_CMP   ( REG_X, ABS   ); break;
			case 0xed: /* SBC abs */ CLK(4); OP_SBC   ( ABS          ); break;
			case 0xee: /* INC abs */ CLK(6); OP_INC_M ( ABS          ); break;
			case 0xef: /* ISB abs */ CLK(6); OP_ISB   ( ABS          ); break; /* 10 */
			case 0xf0: /* BEQ     */ CLK(2); OP_BCC   ( COND_EQ()    ); break;
			case 0xf1: /* SBC idy */ CLK(5); OP_SBC   ( IDY          ); break;
			case 0xf3: /* ISB idy */ CLK(6); OP_ISB   ( IDY          ); break; /* 10 */
			case 0xf4: /* DOP     */ CLK(2); OP_DOP   (              ); break; /* 10 */
			case 0xf5: /* SBC zpx */ CLK(4); OP_SBC   ( ZPX          ); break;
			case 0xf6: /* INC zpx */ CLK(6); OP_INC_M ( ZPX          ); break;
			case 0xf7: /* ISB zpx */ CLK(6); OP_ISB   ( ZPX          ); break; /* 10 */
			case 0xf8: /* SED     */ CLK(2); OP_SED   (              ); break;
			case 0xf9: /* SBC aby */ CLK(4); OP_SBC   ( ABY_P        ); break;
			case 0xfa: /* NOP     */ CLK(2); OP_NOP   (              ); break; /* 10 */
			case 0xfb: /* ISB aby */ CLK(6); OP_ISB   ( ABY          ); break; /* 10 */
			case 0xfc: /* TOP     */ CLK(2); OP_TOP   (              ); break; /* 10 */
			case 0xfd: /* SBC abx */ CLK(4); OP_SBC   ( ABX_P        ); break;
			case 0xfe: /* INC abx */ CLK(7); OP_INC_M ( ABX          ); break;
			case 0xff: /* ISB abx */ CLK(7); OP_ISB   ( ABX          ); break; /* 10 */
			default:   /* illegal */ CLK(2); OP_ILLEGAL();           break;
		}
#if M6502_STRICT_IRQ
		if(!IRQ_DELAY && LINE_IRQ && !FLAG_I)
			SERVICE_IRQ();
#endif /* M6502_STRICT_IRQ */
	}
	return clocks - CLOCKS;
}

#endif /* HAS_M6510 */


#if HAS_N2A03

/* Execute instructions for <clocks> cycles */
int n2a03_execute(int clocks)
{
	CLOCKS = LINE_RST ? 0 : clocks;
	while(CLOCKS > 0)
	{
#if M6502_STRICT_IRQ
		IRQ_DELAY = 0;
#endif /* M6502_STRICT_IRQ */

		REG_PPC = REG_PC;

		CALL_MAME_DEBUG;

		REG_PC++;

		switch(REG_IR = read_8_instruction(REG_PPC))
		{
			case 0x00: /* BRK     */ CLK(7); OP_BRK   (              ); break;
			case 0x01: /* ORA idx */ CLK(6); OP_ORA   ( IDX          ); break;
			case 0x05: /* ORA zp  */ CLK(3); OP_ORA   ( ZPG          ); break;
			case 0x06: /* ASL zp  */ CLK(5); OP_ASL_M ( ZPG          ); break;
			case 0x08: /* PHP     */ CLK(2); OP_PHP   (              ); break;
			case 0x09: /* ORA imm */ CLK(2); OP_ORA   ( IMM          ); break;
			case 0x0a: /* ASL acc */ CLK(2); OP_ASL_A (              ); break;
			case 0x0d: /* ORA abs */ CLK(4); OP_ORA   ( ABS          ); break;
			case 0x0e: /* ASL abs */ CLK(6); OP_ASL_M ( ABS          ); break;
			case 0x10: /* BPL     */ CLK(2); OP_BCC   ( COND_PL()    ); break;
			case 0x11: /* ORA idy */ CLK(5); OP_ORA   ( IDY          ); break;
			case 0x15: /* ORA zpx */ CLK(4); OP_ORA   ( ZPX          ); break;
			case 0x16: /* ASL zpx */ CLK(6); OP_ASL_M ( ZPX          ); break;
			case 0x18: /* CLC     */ CLK(2); OP_CLC   (              ); break;
			case 0x19: /* ORA aby */ CLK(4); OP_ORA   ( ABY_P        ); break;
			case 0x1d: /* ORA abx */ CLK(4); OP_ORA   ( ABX_P        ); break;
			case 0x1e: /* ASL abx */ CLK(7); OP_ASL_M ( ABX          ); break;
			case 0x20: /* JSR     */ CLK(6); OP_JSR   ( ABS          ); break;
			case 0x21: /* AND idx */ CLK(6); OP_AND   ( IDX          ); break;
			case 0x24: /* BIT zp  */ CLK(3); OP_BIT   ( ZPG          ); break;
			case 0x25: /* AND zp  */ CLK(3); OP_AND   ( ZPG          ); break;
			case 0x26: /* ROL zp  */ CLK(5); OP_ROL_M ( ZPG          ); break;
			case 0x28: /* PLP     */ CLK(2); OP_PLP   (              ); break;
			case 0x29: /* AND imm */ CLK(2); OP_AND   ( IMM          ); break;
			case 0x2a: /* ROL acc */ CLK(2); OP_ROL_A (              ); break;
			case 0x2c: /* BIT abs */ CLK(4); OP_BIT   ( ABS          ); break;
			case 0x2d: /* AND abs */ CLK(4); OP_AND   ( ABS          ); break;
			case 0x2e: /* ROL abs */ CLK(6); OP_ROL_M ( ABS          ); break;
			case 0x30: /* BMI     */ CLK(2); OP_BCC   ( COND_MI()    ); break;
			case 0x31: /* AND idy */ CLK(5); OP_AND   ( IDY          ); break;
			case 0x35: /* AND zpx */ CLK(4); OP_AND   ( ZPX          ); break;
			case 0x36: /* ROL zpx */ CLK(6); OP_ROL_M ( ZPX          ); break;
			case 0x38: /* SEC     */ CLK(2); OP_SEC   (              ); break;
			case 0x39: /* AND aby */ CLK(4); OP_AND   ( ABY_P        ); break;
			case 0x3d: /* AND abx */ CLK(4); OP_AND   ( ABX_P        ); break;
			case 0x3e: /* ROL abx */ CLK(7); OP_ROL_M ( ABX          ); break;
			case 0x40: /* RTI     */ CLK(6); OP_RTI   (              ); break;
			case 0x41: /* EOR idx */ CLK(6); OP_EOR   ( IDX          ); break;
			case 0x45: /* EOR zp  */ CLK(3); OP_EOR   ( ZPG          ); break;
			case 0x46: /* LSR zp  */ CLK(5); OP_LSR_M ( ZPG          ); break;
			case 0x48: /* PHA     */ CLK(2); OP_PUSH  ( REG_A        ); break;
			case 0x49: /* EOR imm */ CLK(2); OP_EOR   ( IMM          ); break;
			case 0x4a: /* LSR acc */ CLK(2); OP_LSR_A (              ); break;
			case 0x4c: /* JMP abs */ CLK(3); OP_JMP   ( ABS          ); break;
			case 0x4d: /* EOR abs */ CLK(4); OP_EOR   ( ABS          ); break;
			case 0x4e: /* LSR abs */ CLK(6); OP_LSR_M ( ABS          ); break;
			case 0x50: /* BVC     */ CLK(2); OP_BCC   ( COND_VC()    ); break;
			case 0x51: /* EOR idy */ CLK(5); OP_EOR   ( IDY_P        ); break;
			case 0x55: /* EOR zpx */ CLK(4); OP_EOR   ( ZPX          ); break;
			case 0x56: /* LSR zpx */ CLK(6); OP_LSR_M ( ZPX          ); break;
			case 0x58: /* CLI     */ CLK(2); OP_CLI   (              ); break;
			case 0x59: /* EOR aby */ CLK(4); OP_EOR   ( ABY_P        ); break;
			case 0x5d: /* EOR abx */ CLK(4); OP_EOR   ( ABX_P        ); break;
			case 0x5e: /* LSR abx */ CLK(7); OP_LSR_M ( ABX          ); break;
			case 0x60: /* RTS     */ CLK(6); OP_RTS   (              ); break;
			case 0x61: /* ADC idx */ CLK(6); OP_ADC_ND( IDX          ); break; /* N */
			case 0x65: /* ADC zp  */ CLK(3); OP_ADC_ND( ZPG          ); break; /* N */
			case 0x66: /* ROR zp  */ CLK(5); OP_ROR_M ( ZPG          ); break;
			case 0x68: /* PLA     */ CLK(2); OP_PLA   (              ); break;
			case 0x69: /* ADC imm */ CLK(2); OP_ADC_ND( IMM          ); break; /* N */
			case 0x6a: /* ROR acc */ CLK(2); OP_ROR_A (              ); break;
			case 0x6c: /* JMP ind */ CLK(5); OP_JMP   ( IND          ); break;
			case 0x6d: /* ADC abs */ CLK(4); OP_ADC_ND( ABS          ); break; /* N */
			case 0x6e: /* ROR abs */ CLK(6); OP_ROR_M ( ABS          ); break;
			case 0x70: /* BVS     */ CLK(2); OP_BCC   ( COND_VS()    ); break;
			case 0x71: /* ADC idy */ CLK(5); OP_ADC_ND( IDY_P        ); break; /* N */
			case 0x75: /* ADC zpx */ CLK(4); OP_ADC_ND( ZPX          ); break; /* N */
			case 0x76: /* ROR zpx */ CLK(6); OP_ROR_M ( ZPX          ); break;
			case 0x78: /* SEI     */ CLK(2); OP_SEI   (              ); break;
			case 0x79: /* ADC aby */ CLK(4); OP_ADC_ND( ABY_P        ); break; /* N */
			case 0x7d: /* ADC abx */ CLK(4); OP_ADC_ND( ABX_P        ); break; /* N */
			case 0x7e: /* ROR abx */ CLK(7); OP_ROR_M ( ABX          ); break;
			case 0x81: /* STA idx */ CLK(6); OP_ST    ( REG_A, IDX   ); break;
			case 0x84: /* STY zp  */ CLK(3); OP_ST    ( REG_Y, ZPG   ); break;
			case 0x85: /* STA zp  */ CLK(3); OP_ST    ( REG_A, ZPG   ); break;
			case 0x86: /* STX zp  */ CLK(3); OP_ST    ( REG_X, ZPG   ); break;
			case 0x88: /* DEY     */ CLK(2); OP_DEC_R ( REG_Y        ); break;
			case 0x8a: /* TXA     */ CLK(2); OP_TRANS ( REG_X, REG_A ); break;
			case 0x8c: /* STY abs */ CLK(4); OP_ST    ( REG_Y, ABS   ); break;
			case 0x8d: /* STA abs */ CLK(4); OP_ST    ( REG_A, ABS   ); break;
			case 0x8e: /* STX abs */ CLK(5); OP_ST    ( REG_X, ABS   ); break;
			case 0x90: /* BCC     */ CLK(2); OP_BCC   ( COND_CC()    ); break;
			case 0x91: /* STA idy */ CLK(5); OP_ST    ( REG_A, IDY   ); break;
			case 0x94: /* STY zpx */ CLK(4); OP_ST    ( REG_Y, ZPX   ); break;
			case 0x95: /* STA zpx */ CLK(4); OP_ST    ( REG_A, ZPX   ); break;
			case 0x96: /* STX zpy */ CLK(4); OP_ST    ( REG_X, ZPY   ); break;
			case 0x98: /* TYA     */ CLK(2); OP_TRANS ( REG_Y, REG_A ); break;
			case 0x99: /* STA aby */ CLK(5); OP_ST    ( REG_A, ABY   ); break;
			case 0x9a: /* TXS     */ CLK(2); OP_TXS   (              ); break;
			case 0x9d: /* STA abx */ CLK(5); OP_ST    ( REG_A, ABX   ); break;
			case 0xa0: /* LDY imm */ CLK(2); OP_LD    ( REG_Y, IMM   ); break;
			case 0xa1: /* LDA idx */ CLK(6); OP_LD    ( REG_A, IDX   ); break;
			case 0xa2: /* LDX imm */ CLK(2); OP_LD    ( REG_X, IMM   ); break;
			case 0xa4: /* LDY zp  */ CLK(3); OP_LD    ( REG_Y, ZPG   ); break;
			case 0xa5: /* LDA zp  */ CLK(3); OP_LD    ( REG_A, ZPG   ); break;
			case 0xa6: /* LDX zp  */ CLK(3); OP_LD    ( REG_X, ZPG   ); break;
			case 0xa8: /* TAY     */ CLK(2); OP_TRANS ( REG_A, REG_Y ); break;
			case 0xa9: /* LDA imm */ CLK(2); OP_LD    ( REG_A, IMM   ); break;
			case 0xaa: /* TAX     */ CLK(2); OP_TRANS ( REG_A, REG_X ); break;
			case 0xac: /* LDY abs */ CLK(4); OP_LD    ( REG_Y, ABS   ); break;
			case 0xad: /* LDA abs */ CLK(4); OP_LD    ( REG_A, ABS   ); break;
			case 0xae: /* LDX abs */ CLK(4); OP_LD    ( REG_X, ABS   ); break;
			case 0xb0: /* BCS     */ CLK(2); OP_BCC   ( COND_CS()    ); break;
			case 0xb1: /* LDA idy */ CLK(5); OP_LD    ( REG_A, IDY_P ); break;
			case 0xb4: /* LDY zpx */ CLK(4); OP_LD    ( REG_Y, ZPX   ); break;
			case 0xb5: /* LDA zpx */ CLK(4); OP_LD    ( REG_A, ZPX   ); break;
			case 0xb6: /* LDX zpy */ CLK(4); OP_LD    ( REG_X, ZPY   ); break;
			case 0xb8: /* CLV     */ CLK(2); OP_CLV   (              ); break;
			case 0xb9: /* LDA aby */ CLK(4); OP_LD    ( REG_A, ABY_P ); break;
			case 0xba: /* TSX     */ CLK(2); OP_TRANS ( REG_S, REG_X ); break;
			case 0xbc: /* LDY abx */ CLK(4); OP_LD    ( REG_Y, ABX_P ); break;
			case 0xbd: /* LDA abx */ CLK(4); OP_LD    ( REG_A, ABX_P ); break;
			case 0xbe: /* LDX aby */ CLK(4); OP_LD    ( REG_X, ABY_P ); break;
			case 0xc0: /* CPY imm */ CLK(2); OP_CMP   ( REG_Y, IMM   ); break;
			case 0xc1: /* CMP idx */ CLK(6); OP_CMP   ( REG_A, IDX   ); break;
			case 0xc4: /* CPY zp  */ CLK(3); OP_CMP   ( REG_Y, ZPG   ); break;
			case 0xc5: /* CMP zp  */ CLK(3); OP_CMP   ( REG_A, ZPG   ); break;
			case 0xc6: /* DEC zp  */ CLK(5); OP_DEC_M ( ZPG          ); break;
			case 0xc8: /* INY     */ CLK(2); OP_INC_R ( REG_Y        ); break;
			case 0xc9: /* CMP imm */ CLK(2); OP_CMP   ( REG_A, IMM   ); break;
			case 0xca: /* DEX     */ CLK(2); OP_DEC_R ( REG_X        ); break;
			case 0xcc: /* CPY abs */ CLK(4); OP_CMP   ( REG_Y, ABS   ); break;
			case 0xcd: /* CMP abs */ CLK(4); OP_CMP   ( REG_A, ABS   ); break;
			case 0xce: /* DEC abs */ CLK(6); OP_DEC_M ( ABS          ); break;
			case 0xd0: /* BNE     */ CLK(2); OP_BCC   ( COND_NE()    ); break;
			case 0xd1: /* CMP idy */ CLK(5); OP_CMP   ( REG_A, IDY_P ); break;
			case 0xd5: /* CMP zpx */ CLK(4); OP_CMP   ( REG_A, ZPX   ); break;
			case 0xd6: /* DEC zpx */ CLK(6); OP_DEC_M ( ZPX          ); break;
			case 0xd8: /* CLD     */ CLK(2); OP_CLD   (              ); break;
			case 0xd9: /* CMP aby */ CLK(4); OP_CMP   ( REG_A, ABY_P ); break;
			case 0xdd: /* CMP abx */ CLK(4); OP_CMP   ( REG_A, ABX_P ); break;
			case 0xde: /* DEC abx */ CLK(7); OP_DEC_M ( ABX          ); break;
			case 0xe0: /* CPX imm */ CLK(2); OP_CMP   ( REG_X, IMM   ); break;
			case 0xe1: /* SBC idx */ CLK(6); OP_SBC_ND( IDX          ); break; /* N */
			case 0xe4: /* CPX zp  */ CLK(3); OP_CMP   ( REG_X, ZPG   ); break;
			case 0xe5: /* SBC zp  */ CLK(3); OP_SBC_ND( ZPG          ); break; /* N */
			case 0xe6: /* INC zp  */ CLK(5); OP_INC_M ( ZPG          ); break;
			case 0xe8: /* INX     */ CLK(2); OP_INC_R ( REG_X        ); break;
			case 0xe9: /* SBC imm */ CLK(2); OP_SBC_ND( IMM          ); break; /* N */
			case 0xea: /* NOP     */ CLK(2); OP_NOP   (              ); break;
			case 0xec: /* CPX abs */ CLK(4); OP_CMP   ( REG_X, ABS   ); break;
			case 0xed: /* SBC abs */ CLK(4); OP_SBC_ND( ABS          ); break; /* N */
			case 0xee: /* INC abs */ CLK(6); OP_INC_M ( ABS          ); break;
			case 0xf0: /* BEQ     */ CLK(2); OP_BCC   ( COND_EQ()    ); break;
			case 0xf1: /* SBC idy */ CLK(5); OP_SBC_ND( IDY          ); break; /* N */
			case 0xf5: /* SBC zpx */ CLK(4); OP_SBC_ND( ZPX          ); break; /* N */
			case 0xf6: /* INC zpx */ CLK(6); OP_INC_M ( ZPX          ); break;
			case 0xf8: /* SED     */ CLK(2); OP_SED   (              ); break;
			case 0xf9: /* SBC aby */ CLK(4); OP_SBC_ND( ABY_P        ); break; /* N */
			case 0xfd: /* SBC abx */ CLK(4); OP_SBC_ND( ABX_P        ); break; /* N */
			case 0xfe: /* INC abx */ CLK(7); OP_INC_M ( ABX          ); break;
			default:   /* illegal */ OP_ILLEGAL();           break;
		}
#if M6502_STRICT_IRQ
		if(!IRQ_DELAY && LINE_IRQ && !FLAG_I)
			SERVICE_IRQ();
#endif /* M6502_STRICT_IRQ */
	}
	return clocks - CLOCKS;
}

#endif /* HAS_N2A03 */


/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */
