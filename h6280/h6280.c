/*
TRB and TSB and TST  set V and N from src, not src anded with dst
Note IO_BASE
problem with push/pull 16 and getting 0x10000

SUB
supbtime
robocop
hippodrm
slyspy
midres
darkseal
cninja
edrandy
tumblepop
vaportra
twocrude
funkyjet
madmotor

MAIN
actfancr
triothep

*/

/* ======================================================================== */
/* ================================= NOTES ================================ */
/* ======================================================================== */
/*

Regarding the flag variables:

	In order to speed up emulation, the flags are stored in separate variables,
	and are used in the operations to store values during operation execution
	(this saves on extra copies to the flags).  As a result, only certain bits
	of the flag variables are useful:

	Variable  Most common use                             Flag set when
	--------  -----------------------------------------   ------------------
	 FLAG_N   assigned the result of the operation        Bit 7 is set
	 FLAG_V   set using VFLAG_ADD() or VFLAG_SUB()        Bit 7 is set
	 FLAG_T   not used as a temp variable                 FLAG_T = FLAGPOS_T
	 FLAG_B   not used as a temp variable                 FLAG_B = FLAGPOS_B
	 FLAG_D   not used as a temp variable                 FLAG_D = FLAGPOS_D
	 FLAG_I   not used as a temp variable                 FLAG_I = FLAGPOS_I
	 FLAG_Z   assigned the result of the operation        FLAG_Z = 0
	 FLAG_C   assigned the result of the operation        Bit 8 is set

	(see GET_REG_P(), SET_REG_P() and the condition code macros).


Regarding the MPR registers:

	In order to speed up memory accesses, the MPR values are pre-shifted
	left by 13 bits.


Regarding the Big Switch:
	Since I'm using lots and lots of macros as well as making almost every
	function inline, the Big Switch version of this core creates a
	VERY BIG SWITCH.
	I've been able to get the big switch version of this core to compile only
	if no optimizations are used.  If the MAME optimizations are used, gcc
	fails with "out of virtual memory".
	This is unfortunate, since now I can't measure the speed differences
	between the big switch and the jump table.



CPU INFORMATION:


Registers:

All registers are 8-bit except for the PC, which is 16-bit.

A - Accumulator.  Holds the result of most operations.
X - X Index Register. Used mainly for indexed addressing.
Y - Y Index Register.  Used similarly to X.
S - Stack Pointer. All stack operations access 0x100 + S.
    Stack accesses are done through the zero page (MPR[1]).
PC - Program Counter.
MPR - 8 memory page registers.  A 21-bit physical address is calculated as follows:
      real_address = (MPR[addr>>13] << 13) | (addr&0x1fff)
P - Processor status word.  Contains 8 flags:
	N (bit 7): Set if the operation result is negative.
	V (bit 6): Set if the operation caused an overflow.
	T (bit 5): Causes ADC, AND, EOR, and ORA to read from and store the result
	           to the zero page at index from register X.
	B (bit 4): Set if the BRK instruction is executed.
	D (bit 3): Causes ADC and SBC to operate in decimal mode.
	I (bit 2): Interrupt mask flag.
	Z (bit 1): Set if the operation result is zero.
	C (bit 0): Set if the operation caused a carry.


Paging:

There are two paging schemes at work in this CPU: m6502 paging and huc6280
paging.

The m6502 paging splits the "ram" that it sees into byte-sized chunks (00-ff).
Some operations will take more time if they cross a page boundary, and others
will yield strange results because they don't increase the page as they cross
a boundary.
All zero page operations are done through the "zero page", where the high
order byte of the address is 0.

The huc6280 paging scheme splits the external ram into 8K chunks.
It combines the lower 13 bits of the address with one of the 8 MPRs indexed by
the high 3 bits of the address to create a 21 bit address (giving a 2
megabyte address space).
In C, the calculation looks like this:
	real_address = (MPR[addr>>13] << 13) | (addr&0x1fff)
All zero page and stack operations are done through MPR[1] (not MPR[0]!).
Note that the highest page (ff) is reserved for port I/O.


Interrupts:

There are two maskable interrupt lines (IRQ nad IRQ2), one non-maskable
interrupt line (NMI), and a maskable virtual timer interrupt (TIQ) line.

NMI is edge triggered, so you must set the line low before setting it high
again will cause an interrupt.

The maskable interrupts can be all masked by the I flag, as well as
individually masked by the interrupt mask register.

TIQ is a virtual interrupt line which is set by the timer in the CPU.
If you write to the interrupt status register, it will clear the TIQ "line".
TIQ is edge triggered, like NMI, and so it will not keep interrupting if the
TIQ "line" is held high.

The timer is a free-running counter that is decremented every CPU cycle when
it is enabled.  When decrementing the timer generates a carry, it reloads with
the timer period register and sets the TIQ "line" high.
If the timer is disabled and then later enabled, the timer value is set to the
timer period value.


Port I/O:

Port accesses are done through h6280 page ff (corresponding to physical address
range 1fe000 - 1fffff).
The address is then further subdivided into device and port.  A10-A12 maps
to the device (0-7), and A0-A7 maps to the port number (0-255).
Many ports will only read a certain number of lines from the data bus.
Any data on the other data lines is ignored.


Port Registers:

Devices 3 ($0c00) and 5 ($1400) are used internally by the CPU.
The following port registers are available:
	Name				Value	Port	Access
	----------------	-----	----	------
	timer value			00-7f	0c00 	(read)
	timer period		00-7f	0c00 	(write)
	timer enable		00-01	0c01 	(read/write)
	interrupt mask		00-07	1402 	(read/write)
	interrupt status	00-07	1403 	(read, special case write)

Timer value is the current value in the timer.

Timer period is the value that the timer gets reset to when decrementing it
generates a carry.

Timer enable turns the timer on or off.

Interrupt mask is a 3-bit register which masks the corresponding maskable
interrupts:
	bit 0: IRQ2
	bit 1: IRQ
	bit 3: TIQ

The interrupt status reflects the current state of the interrupt lines and the
virtual TIQ line.
If an interrupt line is set or cleared, the interrupt status register will
immediately reflect it.
Reading the interrupt status register won't clear it since the register just
reflects the current interrupt line states.


Peculiarities:

- There are 2 kinds of pages: m6502 pages and h6280 pages.
  m6502 pages are 256 bytes each and run from xx00 to xxFF.
  h6280 pages refer to the paging scheme using MPR to access a 21-bit address space.
- Zero page is accessed from m6502 page 0 (0000-00ff), through MPR[1].
- Indirect addressing modes used by the JMP instruction stay in the same m6502 page.
- All stack operations are done through MPR[1] with an address of 0x100 + S.
- Subtract operations (CMP and SBC) first invert the C flag, do the operation, set
  the C flag, and invert it again.
- ADC and SBC take an extra cycle to complete if the D flag is set.
- BRK pushes opcode+2, not opcode+1 (i.e. the byte after BRK is lost).
- JSR pushes opcode+2, not opcode+3.
- RTS pulls PC and adds 1.
- When the I flag is cleared, the CPU must wait 1 cycle before checking for IRQ.
- When calculating IDY, add 1 extra cycle if adding Y to the zero page address
  crosses an m6502 page boundary.
- Add 1 extra cycle when branching takes you to a new m6502 page.
- The T flag is cleared after every instruction except for SET.
- The T flag only affects ADC, AND, EOR, and ORA.  For these instructions,
  an operand from zero page indexed by the Y register is used instead
  of the accumulator.


*/
/* ======================================================================== */
/* ================================ INCLUDES ============================== */
/* ======================================================================== */

#include <limits.h>
#include <stdlib.h>
#include "h6280.h"



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
//#define MAKE_INT_8(A) (int8)((A)&0xff)
#define MAKE_INT_8(A) ((int8)(A))
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
	uint a;				/* Accumulator */
	uint x;				/* Index Register X */
	uint y;				/* Index Register Y */
	uint s;				/* Stack Pointer */
	uint pc;			/* Program Counter */
	uint ppc;			/* Previous Program Counter */
	uint flag_n;		/* Negative Flag */
	uint flag_v;		/* Overflow Flag */
	uint flag_t;		/* Next ADC, AND, EOR, ORA in special mode */
	uint flag_b;		/* BRK Instruction Flag */
	uint flag_d;		/* Decimal Mode Flag */
	uint flag_i;		/* Interrupt Mask Flag */
	uint flag_z;		/* Zero Flag (inverted) */
	uint flag_c;		/* Carry Flag */
	uint mpr[8];		/* 8 Memory Page Registers */
	uint timer_period;	/* Timer period */
	uint timer_value;	/* Current value of the timer */
	uint timer_enable;	/* Timer is enabled if this is 1 */
	uint int_mask;		/* Masks out interrupts */
	uint int_states;	/* State of the maskable interrupt lines */
	uint pending_ints;	/* Current interrupts pending */
	uint line_nmi;		/* Status of the NMI line */
	uint line_rst;		/* Status of the RESET line */
	uint ir;			/* Instruction Register */
	int (*int_ack)(int); /* Interrupt Acknowledge */
} h6280i_cpu_struct;



/* ======================================================================== */
/* ================================= DATA ================================= */
/* ======================================================================== */

/* Our CPU structure */
static h6280i_cpu_struct h6280i_cpu = {0};

/* Cycle and timer counters */
int h6280_ICount = 0;
static int h6280i_tiq_point = 0;

/* Temporary Variables */
static uint h6280i_source;
static uint h6280i_destination;
static uint h6280i_length;
static uint h6280i_data;

/* MPR selector for TMA instruction */
static int8 h6280i_mpr_num[256] =
{/*		0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0 */ 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
/* 1 */ 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
/* 2 */ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
/* 3 */ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
/* 4 */ 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
/* 5 */ 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
/* 6 */ 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
/* 7 */ 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
/* 8 */ 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
/* 9 */ 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
/* A */ 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
/* B */ 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
/* C */ 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
/* D */ 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
/* E */ 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
/* F */ 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};



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

/* Flag positions in Processor Status Register */
#define FLAGPOS_N	BIT_7				/* Negative         */
#define FLAGPOS_V	BIT_6				/* Overflow         */
#define FLAGPOS_T	BIT_5				/* T flag           */
#define FLAGPOS_B	BIT_4				/* BRK Instruction  */
#define FLAGPOS_D	BIT_3				/* Decimal Mode     */
#define FLAGPOS_I	BIT_2				/* Interrupt Mask   */
#define FLAGPOS_Z	BIT_1				/* Zero             */
#define FLAGPOS_C	BIT_0				/* Carry            */

#define STACK_PAGE	0x100				/* Stack Page Offset */

#define	VECTOR_RST	0xfffe				/* Reset                   */
#define	VECTOR_NMI	0xfffc				/* Non-Maskable Interrupt  */
#define	VECTOR_TIQ	0xfffa				/* Timer                   */
#define	VECTOR_IRQ	0xfff8				/* Interrupt Request       */
#define	VECTOR_IRQ2	0xfff6				/* Interrupt 2 Request     */
#define	VECTOR_BRK	0xfff6				/* BRK Instruction         */

//#define IO_BASE     0x1fe000			/* Base address of memory-mapped I/O ports */
#define IO_BASE 0xffffffff

#define INT_IRQ2 1						/* IRQ2 interrupt line */
#define INT_IRQ  2						/* IRQ interrupt line */
#define INT_TIQ  4						/* Timer interrupt virtual line */

#define REG_A		h6280i_cpu.a		/* Accumulator */
#define REG_X		h6280i_cpu.x		/* Index X Register */
#define REG_Y		h6280i_cpu.y		/* Index Y Register */
#define REG_S		h6280i_cpu.s		/* Stack Pointer */
#define REG_PC		h6280i_cpu.pc		/* Program Counter */
#define REG_PPC		h6280i_cpu.ppc		/* Previous Program Counter */
#define REG_P		h6280i_cpu.p		/* Processor Status Register */
#define FLAG_N		h6280i_cpu.flag_n	/* Negative Flag */
#define FLAG_V		h6280i_cpu.flag_v	/* Overflow Flag */
#define FLAG_T		h6280i_cpu.flag_t	/* T Flag  */
#define FLAG_B		h6280i_cpu.flag_b	/* BRK Instruction Flag */
#define FLAG_D		h6280i_cpu.flag_d	/* Decimal Mode Flag */
#define FLAG_I		h6280i_cpu.flag_i	/* Interrupt Mask Flag */
#define FLAG_Z		h6280i_cpu.flag_z	/* Zero Flag (inverted) */
#define FLAG_C		h6280i_cpu.flag_c	/* Carry Flag */
#define REG_MPR		h6280i_cpu.mpr		/* Memory Page Registers  */
#define REG_TIMER_PERIOD	h6280i_cpu.timer_period
#define REG_TIMER_VALUE		h6280i_cpu.timer_value
#define REG_TIMER_ENABLE	h6280i_cpu.timer_enable
#define REG_INT_MASK		h6280i_cpu.int_mask
#define REG_INT_STATES		h6280i_cpu.int_states
#define PENDING_INTS		h6280i_cpu.pending_ints
#define LINE_NMI	h6280i_cpu.line_nmi	/* Status of the NMI line */
#define LINE_RST	h6280i_cpu.line_rst	/* Status of the RESET line */
#define REG_IR		h6280i_cpu.ir		/* Instruction Register */
#define INT_ACK		h6280i_cpu.int_ack	/* Interrupt Acknowledge function pointer */

#define CLOCKS		h6280_ICount		/* Clock cycles remaining */
#define TIQ_POINT	h6280i_tiq_point	/* Point where the timer will cause a TIQ */

/* Temporary values used in opcode handlers */
#define SRC			h6280i_source		/* Source Variable */
#define DST			h6280i_destination	/* Destination Variable */
#define LEN			h6280i_length		/* Length Variable */
#define DAT			h6280i_data			/* Temporary Data Variable */



/* ======================================================================== */
/* ============================ GENERAL MACROS ============================ */
/* ======================================================================== */

/* Use breaks (big switch) or returns (func tbl) to break out of a function */
#undef BREAKOUT
#if H6280_BIG_SWITCH
#define BREAKOUT break
#else
#define BREAKOUT return
#endif

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



/* ======================================================================== */
/* ====================== UTILITY MACROS & FUNCTIONS ====================== */
/* ======================================================================== */

/* Use up clock cycles */
#define CLK(A) CLOCKS -= (A)
#define USE_ALL_CLKS() CLOCKS = 0

/* Calculate the physical address from a logical one */
#define ADDRESS_NORMAL(A)		( REG_MPR[((A)>>13)&7] | ((A)&0x1fff) )
#define ADDRESS_ZPG(A)			( REG_MPR[1] | ((A)&0xff) )
#define ADDRESS_STK(A)			( REG_MPR[1] | ((A)&0xff) | STACK_PAGE )


/* Top level memory access functions.  These functions check for internally
 * mapped I/O ports, and will forward all other accesses in the I/O range
 * to the host's port I/O routines if needed.
 * Addresses >= 0x1fe000 are port accesses.
 * The device is encoded from A10-A12 (8 devices)
 * The register is encoded from A0-A7.
 * The data is transmitted through the data bus.
 * Device addresses 3 and 5 are used internally by the timer and interrupt controller.
 */
INLINE uint read_8_normal(uint address)
{
	address = ADDRESS_NORMAL(address);		/* Decode physical address */

	if(address < IO_BASE)					/* Memory */
		return h6280_read_8(address);

	switch(address & 0x1c00)				/* Internal device */
	{
		case 0x0c00:						/* Timer - only checks A0 */
			if(address & 1)					/* 1: Timer Enabled (00-01) */
				return REG_TIMER_ENABLE;
			return CLOCKS - TIQ_POINT;		/* 0: Timer Value (00-7f) */

		case 0x1400:						/* Interrupt controller */
			switch(address & 15)			/* Only checks A0-A3 */
			{
				case 2:
					return REG_INT_MASK;	/* 2: IRQ Mask (00-07) */
				case 3:
					return REG_INT_STATES;	/* 3: IRQ Status (00-07) */
			}
			return 0;						/* All others ignored */
	}
	return h6280_read_8_io(address&0x1cff);	/* External device */
}

INLINE void write_8_normal(uint address, uint value)
{
	address = ADDRESS_NORMAL(address);			/* Decode physical address */

	if(address < IO_BASE)						/* Memory */
	{
		h6280_write_8(address, value);
		return;
	}

	switch(address & 0x1c00)					/* Internal device */
	{
		case 0x0c00:							/* Timer - only checks A0 */
			if(address & 1)						/* 1: Timer Enabled (00-01) */
			{
//				uint old_value = REG_TIMER_ENABLE;
//				REG_TIMER_ENABLE = value & 1;
//				if(REG_TIMER_ENABLE && !old_value)
//					TIQ_POINT = CLOCKS - REG_TIMER_PERIOD;
				return;
			}
			REG_TIMER_PERIOD = value & 0x7f;	/* 0: Timer Value (00-7f) */
			return;

		case 0x1400:							/* Interrupt Controller */
			switch(address & 15)				/* Only checks A0-A3 */
			{
				case 2:							/* 2: IRQ Mask (00-07) */
					REG_INT_MASK = value & 7;
					return;
				case 3:							/* 3: Clear TIQ */
					REG_INT_STATES &= ~INT_TIQ;
					return;
			}
			return;								/* All others ignored */
	}
	h6280_write_8_io(address&0x1cff, value);	/* External device */
}

INLINE uint read_8_zeropage(uint address)
{
	address = ADDRESS_ZPG(address);		/* Decode physical address */

	if(address < IO_BASE)					/* Memory */
		return h6280_read_8(address);
	return h6280_read_8_io(address&0x1cff);	/* External device */
}

INLINE void write_8_zeropage(uint address, uint value)
{
	address = ADDRESS_ZPG(address);			/* Decode physical address */

	if(address < IO_BASE)						/* Memory */
	{
		h6280_write_8(address, value);
		return;
	}
	h6280_write_8_io(address&0x1cff, value);	/* External device */
}

INLINE uint read_8_stack(uint address)
{
	address = ADDRESS_STK(address);				/* Decode physical address */
	return h6280_read_8(address);
}

INLINE void write_8_stack(uint address, uint value)
{
	address = ADDRESS_STK(address);				/* Decode physical address */
	h6280_write_8(address, value);
}

INLINE uint read_8_immediate(uint address)
{
	address = ADDRESS_NORMAL(address);
	return h6280_read_8_immediate(address);
}

INLINE uint read_8_instruction(uint address)
{
	address = ADDRESS_NORMAL(address);
	return h6280_read_instruction(address);
}

#define read_8_io(A)			h6280_read_8_io(A)
#define write_8_io(A, D)		h6280_write_8_io(A, D)

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

INLINE uint read_16_nopage(uint address)
{
	return read_8_normal(address) | (read_8_normal(((address&0xff00)|((address+1)&0x00ff)))<<8);
}


/* Low level memory access macros */
#define read_8_NORM(A)		read_8_normal(A)
#define read_8_IMM(A)		read_8_immediate(A)
#define read_8_ZPG(A)		read_8_zeropage(A)
#define read_8_ABS(A)		read_8_normal(A)
#define read_8_ZPX(A)		read_8_zeropage(A)
#define read_8_ZPY(A)		read_8_zeropage(A)
#define read_8_ABX(A)		read_8_normal(A)
#define read_8_ABY(A)		read_8_normal(A)
#define read_8_IND(A)		read_8_normal(A)
#define read_8_IDX(A)		read_8_normal(A)
#define read_8_IDY(A)		read_8_normal(A)
#define read_8_ZPI(A)		read_8_normal(A)
#define read_8_STK(A)		read_8_stack(A)
#define read_8_TFL(A)		read_8_zeropage(A)
#define read_8_IO(A)		read_8_io(A)

#define read_16_NORM(A)		read_16_normal(A)
#define read_16_IMM(A)		read_16_immediate(A)
#define read_16_IDX(A)		read_16_zeropage(A)
#define read_16_IDY(A)		read_16_zeropage(A)
#define read_16_ZPI(A)		read_16_zeropage(A)
#define read_16_IND(A)		read_16_nopage(A)
#define read_16_IAX(A)		read_16_nopage(A)
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
#define write_8_STK(A, V)	write_8_stack(A, V)
#define write_8_TFL(A, V)	write_8_zeropage(A, V)
#define write_8_IO(A, V)	write_8_io(A, V)


/* Effective Address Caluclations */
INLINE uint EA_IMM(void)   {return REG_PC++;}
INLINE uint EA_IMM16(void) {REG_PC += 2; return REG_PC-2;}
INLINE uint EA_ZPG(void)   {return read_8_IMM(EA_IMM());}
INLINE uint EA_ABS(void)   {return read_16_IMM(EA_IMM16());}
INLINE uint EA_ABX(void)   {return EA_ABS() + REG_X;}
INLINE uint EA_ABY(void)   {return EA_ABS() + REG_Y;}
INLINE uint EA_ZPX(void)   {return EA_ZPG() + REG_X;}
INLINE uint EA_ZPY(void)   {return EA_ZPG() + REG_Y;}
INLINE uint EA_IND(void)   {return read_16_IND(EA_ABS());}
INLINE uint EA_IDX(void)   {return read_16_IDX((EA_ZPG() + REG_X)&0xff);}
INLINE uint EA_IDY(void)   {return read_16_IDY(EA_ZPG()) + REG_Y;}
INLINE uint EA_ZPI(void)   {return read_16_ZPI(EA_ZPG());}
INLINE uint EA_IAX(void)   {return read_16_IAX(EA_ABX());}
INLINE uint EA_TFL(void)   {return REG_X; /* Special case when T flag is set */}


/* Operand Retrieval */
#define OPER_IMM()		read_8_IMM(EA_IMM())
#define OPER_IMM16()	read_16_IMM(EA_IMM16()) /* HuC6280 */
#define OPER_ZPG()		read_8_ZPG(EA_ZPG())
#define OPER_ABS()		read_8_ABS(EA_ABS())
#define OPER_ZPX()		read_8_ZPX(EA_ZPX())
#define OPER_ZPY()		read_8_ZPY(EA_ZPY())
#define OPER_ABX()		read_8_ABX(EA_ABX())
#define OPER_ABY()		read_8_ABY(EA_ABY())
#define OPER_IND()		read_8_IND(EA_IND())
#define OPER_IDX()		read_8_IDX(EA_IDX())
#define OPER_IDY()		read_8_IDY(EA_IDY())
#define OPER_ZPI()		read_8_ZPI(EA_ZPI())
#define OPER_TFL()		read_8_TFL(EA_TFL())


/* Change the Program Counter */
INLINE void jump(uint address)
{
	REG_PC = address;
	h6280_jumping(REG_PC);
}

/* Branch - add an extra cycle if we cross a page boundary */
INLINE void branch(uint address)
{
	uint old_pc = REG_PC;
	REG_PC += MAKE_INT_8(address);
	if((old_pc ^ REG_PC)&0xff00)
		CLK(1);
	h6280_branching(REG_PC);
}

/* Push/Pull data to/from the stack */
INLINE void push_8(uint value)
{
	write_8_STK(REG_S, value);
	REG_S = MAKE_UINT_8(REG_S - 1);
}

INLINE uint pull_8(void)
{
	REG_S = MAKE_UINT_8(REG_S + 1);
	return read_8_STK(REG_S);
}

INLINE void push_16(uint value)
{
	push_8(MAKE_UINT_8(value>>8));
	push_8(MAKE_UINT_8(value));
}

INLINE uint pull_16(void)
{
	uint value = pull_8();
	return value | (pull_8() << 8);
}



/* Get the Processor Status Register (Note that we ignore T) */
#define GET_REG_P()				\
	((FLAG_N & 0x80)		|	\
	((FLAG_V & 0x80) >> 1)	|	\
	FLAG_B					|	\
	FLAG_D					|	\
	FLAG_I					|	\
	((!FLAG_Z) << 1)		|	\
	((FLAG_C >> 8)&1))

/* Get Processor Status Register with B flag set (when executing BRK instruction) */
#define GET_REG_P_BRK()			\
	((FLAG_N & 0x80)		|	\
	((FLAG_V & 0x80) >> 1)	|	\
	FLAGPOS_B				|	\
	FLAG_D					|	\
	FLAG_I					|	\
	((!FLAG_Z) << 1)		|	\
	((FLAG_C >> 8)&1))

/* Get Processor Status Register with B flag cleared (when servicing an interrupt) */
#define GET_REG_P_INT()			\
	((FLAG_N & 0x80)		|	\
	((FLAG_V & 0x80) >> 1)	|	\
	FLAG_D					|	\
	FLAG_I					|	\
	((!FLAG_Z) << 1)		|	\
	((FLAG_C >> 8)&1))

/* Set the Process Status Register */
INLINE void SET_REG_P(uint value)
{
	FLAG_N = value;
	FLAG_V = (value<<1);
	FLAG_B = value&FLAGPOS_B;
	FLAG_D = value&FLAGPOS_D;
	FLAG_I = value&FLAGPOS_I;
	FLAG_Z = !(value & 2);
	FLAG_C = value << 8;
}

/* Access the pre-shifted MPR values */
#define SET_REG_MPR(I,V) (REG_MPR[I] = (V)<<13)
#define GET_REG_MPR(I)   (REG_MPR[I]>>13)



/* Check and service an interrupt of type IRQ, IRQ2, or TIQ */
INLINE void check_interrupts(void)
{
	if(!FLAG_I && (PENDING_INTS & ~REG_INT_MASK))
	{
		CLK(7);
		push_16(REG_PC);
		SRC = GET_REG_P_INT();
		push_8(SRC);
		FLAG_I = FLAGPOS_I;
		FLAG_B = 0;
		FLAG_D = 0;
		if(PENDING_INTS & ~REG_INT_MASK & INT_IRQ)
		{
			jump(read_16_VEC(VECTOR_IRQ));
			if(INT_ACK)
				INT_ACK(0);
		}
		else if(PENDING_INTS & ~REG_INT_MASK & INT_IRQ2)
		{
			jump(read_16_VEC(VECTOR_IRQ2));
			if(INT_ACK)
				INT_ACK(1);
		}
		else if(PENDING_INTS & ~REG_INT_MASK & INT_TIQ)
		{
			PENDING_INTS &= ~INT_TIQ;
			jump(read_16_VEC(VECTOR_TIQ));
		}
	}
}



/* ======================================================================== */
/* =========================== OPERATION MACROS =========================== */
/* ======================================================================== */

/* M6502   Add With Carry */
#if H6280_USE_NEW_ADC_SBC
#define OP_ADC(MODE)		SRC = OPER_##MODE();											\
							if(FLAG_D)														\
							{																\
								CLK(1);														\
								FLAG_C = (FLAG_C>>8)&1;										\
								FLAG_N = REG_A + SRC + FLAG_C;								\
								FLAG_Z = MAKE_UINT_8(FLAG_N);								\
								if(((REG_A&0x0f) + (SRC&0x0f) + FLAG_C) > 9) FLAG_N += 6;	\
								FLAG_V = VFLAG_ADD(SRC, REG_A, FLAG_N);						\
								REG_A = FLAG_N;												\
								FLAG_C = (REG_A > 0x99)<<8;									\
								if(FLAG_C) REG_A -= 0xa0;									\
								BREAKOUT;													\
							}																\
							FLAG_C = REG_A + SRC + ((FLAG_C>>8)&1);							\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);							\
							FLAG_V = VFLAG_ADD(SRC, REG_A, FLAG_Z);							\
							REG_A = FLAG_Z
#else
#define OP_ADC(MODE)		SRC = OPER_##MODE();											\
							if(FLAG_D)														\
							{																\
								CLK(1);														\
								FLAG_Z = (SRC&0x0f) + (REG_A&0x0f) + ((FLAG_C>>8)&1);		\
								FLAG_N = (SRC&0xf0) + (REG_A&0xf0);							\
								if(FLAG_Z > 0x09)											\
								{															\
									FLAG_Z += 0x06;											\
									FLAG_N += 0x10;											\
								}															\
								FLAG_V = VFLAG_ADD(SRC, REG_A, FLAG_N);						\
								FLAG_C = (FLAG_N > 0x90)<<8;								\
								if(FLAG_C)													\
									FLAG_N += 0x60;											\
								REG_A = FLAG_N = FLAG_Z = (FLAG_N&0xf0) | (FLAG_Z&0x0f);	\
								BREAKOUT;													\
							}																\
							FLAG_C = REG_A + SRC + ((FLAG_C>>8)&1);							\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);							\
							FLAG_V = VFLAG_ADD(SRC, REG_A, FLAG_Z);							\
							REG_A = FLAG_Z
#endif

/* H6280   Add With Carry (When T Flag is set) */
#if H6280_USE_NEW_ADC_SBC
#define OP_ADC_T(MODE)		DST = EA_TFL();													\
							DAT = read_8_TFL(DST);											\
							SRC = OPER_##MODE();											\
							if(FLAG_D)														\
							{																\
								CLK(1);														\
								FLAG_C = (FLAG_C>>8)&1;										\
								FLAG_N = DAT + SRC + FLAG_C;								\
								FLAG_Z = MAKE_UINT_8(FLAG_N);								\
								if(((DAT&0x0f) + (SRC&0x0f) + FLAG_C) > 9) FLAG_N += 6;		\
								FLAG_V = VFLAG_ADD(SRC, DAT, FLAG_N);						\
								write_8_ZPG(DST, FLAG_N);									\
								FLAG_C = (DAT > 0x99)<<8;									\
								if(FLAG_C) DAT -= 0xa0;										\
								BREAKOUT;													\
							}																\
							FLAG_C = DAT + SRC + ((FLAG_C>>8)&1);							\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);							\
							FLAG_V = VFLAG_ADD(SRC, DAT, FLAG_Z);							\
							write_8_TFL(DST, FLAG_Z)
#else
#define OP_ADC_T(MODE)		DST = EA_TFL();													\
							DAT = read_8_TFL(DST);											\
							SRC = OPER_##MODE();											\
							if(FLAG_D)														\
							{																\
								CLK(1);														\
								FLAG_Z = (SRC&0x0f) + (DAT&0x0f) + ((FLAG_C>>8)&1);			\
								FLAG_N = (SRC&0xf0) + (DAT&0xf0);							\
								if(FLAG_Z > 0x09)											\
								{															\
									FLAG_Z += 0x06;											\
									FLAG_N += 0x10;											\
								}															\
								FLAG_V = VFLAG_ADD(SRC, DAT, FLAG_N);						\
								FLAG_C = (FLAG_N > 0x90) << 8;								\
								if(FLAG_C)													\
									FLAG_N += 0x60;											\
								DAT = FLAG_N = FLAG_Z = (FLAG_N&0xf0) | (FLAG_Z&0x0f);		\
								write_8_TFL(DST, DAT);										\
								BREAKOUT;													\
							}																\
							FLAG_C = DAT + SRC + ((FLAG_C>>8)&1);							\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);							\
							FLAG_V = VFLAG_ADD(SRC, DAT, FLAG_Z);							\
							write_8_TFL(DST, FLAG_Z)
#endif

/* M6502   Logical AND with accumulator */
#define OP_AND(MODE)		FLAG_N = FLAG_Z = REG_A &= OPER_##MODE()

/* H6280   Logical AND (When T Flag is set) */
#define OP_AND_T(MODE)		DST = EA_TFL();									\
							FLAG_Z = read_8_TFL(DST);						\
							FLAG_N = FLAG_Z &= OPER_##MODE();				\
							write_8_TFL(DST, FLAG_Z)

/* M6502   Arithmetic Shift Left accumulator */
#define OP_ASL_A()			FLAG_C = REG_A << 1;							\
							FLAG_N = FLAG_Z = REG_A = MAKE_UINT_8(FLAG_C)

/* M6502   Arithmetic Shift Left operand */
#define OP_ASL_M(MODE)		DST = EA_##MODE();								\
							FLAG_C = read_8_##MODE(DST) << 1;				\
							FLAG_N = FLAG_Z = MAKE_UINT_8(FLAG_C);			\
							write_8_##MODE(DST, FLAG_Z)

/* M65C02  Branch if Bit Reset */
#define OP_BBR(BIT)			if(!(OPER_ZPG() & BIT))							\
							{												\
								CLK(8);										\
								branch(OPER_IMM());							\
								BREAKOUT;									\
							}												\
							CLK(6);											\
							REG_PC++

/* M65C02  Branch if Bit Set */
#define OP_BBS(BIT)			if(OPER_ZPG() & BIT)							\
							{												\
								CLK(8);										\
								branch(OPER_IMM());							\
								BREAKOUT;									\
							}												\
							CLK(6);											\
							REG_PC++

/* M6502   Branch on Condition Code */
#define OP_BCC(COND)		if(COND)										\
							{												\
								CLK(3);										\
								branch(OPER_IMM());							\
								BREAKOUT;									\
							}												\
							CLK(2);											\
							REG_PC++

/* M6502   Set flags according to bits */
#define OP_BIT(MODE)		FLAG_N = OPER_##MODE();							\
							FLAG_Z = FLAG_N & REG_A;						\
							FLAG_V = FLAG_N << 1

/* M65C02  Branch Unconditional */
/* If we're in a busy loop and there's no interrupt, eat all clock cycles */
#define OP_BRA()			branch(OPER_IMM());															\
							if(REG_PC == REG_PPC && !(!FLAG_I && (PENDING_INTS & ~REG_INT_MASK)))	\
								USE_ALL_CLKS()

/* M6502   Cause a Break interrupt */
/* Unusual behavior: Pushes opcode+2, not opcode+1   */
#define OP_BRK()			push_16(REG_PC+1);								\
							push_8(GET_REG_P_BRK());						\
							FLAG_I = FLAGPOS_I;								\
							FLAG_D = 0;										\
							jump(read_16_VEC(VECTOR_BRK))

/* H6280   Branch to Subroutine */
#define OP_BSR()			push_16(REG_PC);								\
							branch(OPER_IMM())

/* H6280   Clear the Accumulator */
#define OP_CLA()			REG_A = 0

/* M6502   Clear Carry flag */
#define OP_CLC()			FLAG_C = 0

/* M6502   Clear Decimal flag */
#define OP_CLD()			FLAG_D = 0

/* M6502   Clear Interrupt Mask flag */
#define OP_CLI()			if(FLAG_I)	\
							{	\
							FLAG_I = 0;										\
							check_interrupts();	\
							}

/* M6502   Clear oVerflow flag */
#define OP_CLV()			FLAG_V = 0

/* H6280   Clear the X register */
#define OP_CLX()			REG_X = 0

/* H6280   Clear the Y register */
#define OP_CLY()			REG_Y = 0

/* M6502   Compare operand to register */
/* Unusual behavior: C flag is inverted */
#define OP_CMP(REG, MODE)	FLAG_C = REG - OPER_##MODE();					\
							FLAG_N = FLAG_Z = MAKE_UINT_8(FLAG_C);			\
							FLAG_C = ~FLAG_C

/* H6280   Change Speed High - used for protection check in US games */
#define OP_CSH()			/* Do nothing */

/* H6280   Change Speed Low - used for protection check in US games */
#define OP_CSL()			/* Do nothing */

/* M6502   Decrement operand */
#define OP_DEC_M(MODE)		DST = EA_##MODE();								\
							FLAG_N = read_8_##MODE(DST) - 1;				\
							FLAG_Z = MAKE_UINT_8(FLAG_N);					\
							write_8_##MODE(DST, FLAG_Z)

/* M6502   Decrement register */
#define OP_DEC_R(REG)		FLAG_N = FLAG_Z = REG = MAKE_UINT_8(REG - 1)

/* M6502   Exclusive Or operand to accumulator */
#define OP_EOR(MODE)		FLAG_N = FLAG_Z = REG_A ^= OPER_##MODE()

/* H6280   Exclusive Or (When T Flag is set) */
#define OP_EOR_T(MODE)		DST = EA_TFL();									\
							FLAG_Z = read_8_TFL(DST);						\
							FLAG_N = FLAG_Z ^= OPER_##MODE();				\
							write_8_TFL(DST, FLAG_Z)

/* M6502   Increment operand */
#define OP_INC_M(MODE)		DST = EA_##MODE();								\
							FLAG_N = read_8_##MODE(DST) + 1;				\
							FLAG_Z = MAKE_UINT_8(FLAG_N);					\
							write_8_##MODE(DST, FLAG_Z)

/* M6502   Increment register */
#define OP_INC_R(REG)		FLAG_N = FLAG_Z = REG = MAKE_UINT_8(REG + 1)

/* M6502   Jump */
/* If we're in a busy loop and there's no interrupt, eat all clock cycles */
#define OP_JMP(MODE)		jump(EA_##MODE());								\
							if(REG_PC == REG_PPC && !(!FLAG_I && (PENDING_INTS & ~REG_INT_MASK)))	\
								USE_ALL_CLKS()

/* M6502   Jump to Subroutine */
/* Unusual behavior: pushes opcode+2, not opcode+3 */
#define OP_JSR(MODE)		push_16(REG_PC+1);								\
							jump(EA_##MODE())

/* M6502   Load register with operand */
#define OP_LD(REG, MODE)	FLAG_Z = FLAG_N = REG = OPER_##MODE()

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
#define OP_ORA(MODE)		FLAG_N = FLAG_Z = REG_A |= OPER_##MODE()

/* H6280   Logical OR (When T Flag is set) */
#define OP_ORA_T(MODE)		DST = EA_TFL();									\
							FLAG_Z = read_8_TFL(DST);						\
							FLAG_N = FLAG_Z |= OPER_##MODE();				\
							write_8_TFL(DST, FLAG_Z)

/* M6502   Push a register to the stack */
#define OP_PUSH(REG)		push_8(REG)

/* M6502   Push the Processor Status Register to the stack */
#define OP_PHP()			push_8(GET_REG_P())

/* M6502   Pull a register from the stack */
#define OP_PULL(REG)		REG = pull_8()

/* M6502   Pull the accumulator from the stack */
#define OP_PLA()			FLAG_Z = FLAG_N = REG_A = pull_8()

/* M6502   Pull the Processor Status Register from the stack */
#define OP_PLP()			SET_REG_P(pull_8());							\
							check_interrupts()

/* M65C02  Reset Memory Bit */
#define OP_RMB(MODE, BIT)	DST = EA_##MODE();								\
							SRC = read_8_##MODE(DST) & ~BIT;				\
							write_8_##MODE(DST, SRC)

/* M6502   Rotate Left the accumulator */
#define OP_ROL_A()			FLAG_C = (REG_A<<1) | ((FLAG_C>>8)&1);			\
							FLAG_N = FLAG_Z = REG_A = MAKE_UINT_8(FLAG_C)

/* M6502   Rotate Left an operand */
#define OP_ROL_M(MODE)		DST = EA_##MODE();									\
							FLAG_C = (read_8_##MODE(DST)<<1) | ((FLAG_C>>8)&1);	\
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

/* M6502   Return from Interrupt */
#define OP_RTI()			SET_REG_P(pull_8());							\
							jump(pull_16());								\
							check_interrupts()

/* M6502   Return from Subroutine */
/* Unusual behavior: Pulls PC and adds 1 */
#define OP_RTS()			jump(pull_16() + 1)

/* M6502   Subtract with Carry */
/* Unusual behavior: C flag is inverted */
#if H6280_USE_NEW_ADC_SBC
#define OP_SBC(MODE)		SRC = OPER_##MODE();							\
							DAT = ((FLAG_C>>8)&1)^1;						\
							FLAG_C = REG_A - SRC - DAT;						\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);			\
							FLAG_C = ~FLAG_C;								\
							FLAG_V = VFLAG_SUB(SRC, REG_A, FLAG_Z);			\
							if(!FLAG_D)										\
							{												\
								REG_A = FLAG_Z;								\
								BREAKOUT;									\
							}												\
							CLK(1);											\
							if((DST = (REG_A&0x0f) - (SRC&0x0f) - DAT) > 9)	\
								DST -= 6;									\
							if((DST += (REG_A&0xf0) - (SRC&0xf0)) > 0x99)	\
								DST += 0xa0;								\
							REG_A = DST
#else
#define OP_SBC(MODE)		SRC = OPER_##MODE();											\
							FLAG_C = ((FLAG_C>>8)&1)^1;										\
							if(FLAG_D)														\
							{																\
								CLK(1);														\
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
								BREAKOUT;													\
							}																\
							FLAG_C = REG_A - SRC - FLAG_C;									\
							FLAG_Z = FLAG_N = MAKE_UINT_8(FLAG_C);							\
							FLAG_C = ~FLAG_C;												\
							FLAG_V = VFLAG_SUB(SRC, REG_A, FLAG_Z);							\
							REG_A = FLAG_Z
#endif

/* M6502   Set Carry flag */
#define OP_SEC()			FLAG_C = 0x100

/* M6502   Set Decimal flag */
#define OP_SED()			FLAG_D = FLAGPOS_D

/* M6502   Set Interrupt Mask flag */
#define OP_SEI()			FLAG_I = FLAGPOS_I

/* H6280   Set the T flag */
/* Pretend T is set and execute next instruction */
#define OP_SET()															\
		REG_PPC = REG_PC;													\
		H6280_CALL_DEBUGGER;												\
		REG_PC++;															\
		switch(REG_IR = read_8_instruction(REG_PPC))						\
		{																	\
			case 0x6d: /* ADC abs */ CLK(5+3); OP_ADC_T ( ABS ); BREAKOUT;	\
			case 0x7d: /* ADC abx */ CLK(5+3); OP_ADC_T ( ABX ); BREAKOUT;	\
			case 0x79: /* ADC aby */ CLK(5+3); OP_ADC_T ( ABY ); BREAKOUT;	\
			case 0x61: /* ADC idx */ CLK(7+3); OP_ADC_T ( IDX ); BREAKOUT;	\
			case 0x71: /* ADC idy */ CLK(7+3); OP_ADC_T ( IDY ); BREAKOUT;	\
			case 0x69: /* ADC imm */ CLK(2+3); OP_ADC_T ( IMM ); BREAKOUT;	\
			case 0x65: /* ADC zp  */ CLK(4+3); OP_ADC_T ( ZPG ); BREAKOUT;	\
			case 0x72: /* ADC zpi */ CLK(7+3); OP_ADC_T ( ZPI ); BREAKOUT;	\
			case 0x75: /* ADC zpx */ CLK(4+3); OP_ADC_T ( ZPX ); BREAKOUT;	\
			case 0x2d: /* AND abs */ CLK(5+3); OP_AND_T ( ABS ); BREAKOUT;	\
			case 0x3d: /* AND abx */ CLK(5+3); OP_AND_T ( ABX ); BREAKOUT;	\
			case 0x39: /* AND aby */ CLK(5+3); OP_AND_T ( ABY ); BREAKOUT;	\
			case 0x21: /* AND idx */ CLK(7+3); OP_AND_T ( IDX ); BREAKOUT;	\
			case 0x31: /* AND idy */ CLK(7+3); OP_AND_T ( IDY ); BREAKOUT;	\
			case 0x29: /* AND imm */ CLK(2+3); OP_AND_T ( IMM ); BREAKOUT;	\
			case 0x25: /* AND zp  */ CLK(4+3); OP_AND_T ( ZPG ); BREAKOUT;	\
			case 0x32: /* AND zpi */ CLK(7+3); OP_AND_T ( ZPI ); BREAKOUT;	\
			case 0x35: /* AND zpx */ CLK(4+3); OP_AND_T ( ZPX ); BREAKOUT;	\
			case 0x4d: /* EOR abs */ CLK(5+3); OP_EOR_T ( ABS ); BREAKOUT;	\
			case 0x5d: /* EOR abx */ CLK(5+3); OP_EOR_T ( ABX ); BREAKOUT;	\
			case 0x59: /* EOR aby */ CLK(5+3); OP_EOR_T ( ABY ); BREAKOUT;	\
			case 0x41: /* EOR idx */ CLK(7+3); OP_EOR_T ( IDX ); BREAKOUT;	\
			case 0x51: /* EOR idy */ CLK(7+3); OP_EOR_T ( IDY ); BREAKOUT;	\
			case 0x49: /* EOR imm */ CLK(2+3); OP_EOR_T ( IMM ); BREAKOUT;	\
			case 0x45: /* EOR zp  */ CLK(4+3); OP_EOR_T ( ZPG ); BREAKOUT;	\
			case 0x52: /* EOR zpi */ CLK(7+3); OP_EOR_T ( ZPI ); BREAKOUT;	\
			case 0x55: /* EOR zpx */ CLK(4+3); OP_EOR_T ( ZPX ); BREAKOUT;	\
			case 0x0d: /* ORA abs */ CLK(5+3); OP_ORA_T ( ABS ); BREAKOUT;	\
			case 0x1d: /* ORA abx */ CLK(5+3); OP_ORA_T ( ABX ); BREAKOUT;	\
			case 0x19: /* ORA aby */ CLK(5+3); OP_ORA_T ( ABY ); BREAKOUT;	\
			case 0x01: /* ORA idx */ CLK(7+3); OP_ORA_T ( IDX ); BREAKOUT;	\
			case 0x11: /* ORA idy */ CLK(7+3); OP_ORA_T ( IDY ); BREAKOUT;	\
			case 0x09: /* ORA imm */ CLK(2+3); OP_ORA_T ( IMM ); BREAKOUT;	\
			case 0x05: /* ORA zp  */ CLK(4+3); OP_ORA_T ( ZPG ); BREAKOUT;	\
			case 0x12: /* ORA zpi */ CLK(7+3); OP_ORA_T ( ZPI ); BREAKOUT;	\
			case 0x15: /* ORA zpx */ CLK(4+3); OP_ORA_T ( ZPX ); BREAKOUT;	\
			default: REG_PC--; /* "Put" the instruction back */				\
		}

/* M65C02  Set Memory Bit */
#define OP_SMB(MODE, BIT)	DST = EA_##MODE();								\
							SRC = read_8_##MODE(DST) | BIT;					\
							write_8_##MODE(DST, SRC)

/* M6502   Store register to memory */
#define OP_ST(REG, MODE)	write_8_##MODE(EA_##MODE(), REG)

/* H6280   Store the immediate operand to port 0 */
#define OP_ST0()			write_8_IO(0, OPER_IMM());

/* H6280   Store the immediate operand to port 1 */
#define OP_ST1()			write_8_IO(2, OPER_IMM());

/* H6280   Store the immediate operand to port 2 */
#define OP_ST2()			write_8_IO(3, OPER_IMM());

/* H6280   Swap two registers */
#define OP_SWAP(RA, RB)		RA ^= RB;										\
							RB ^= RA;										\
							RA ^= RB

/* H6280   Transfer Alternate Increment */
#define OP_TAI()			SRC = OPER_IMM16();								\
							DST = OPER_IMM16();								\
							LEN = OPER_IMM16();								\
							DAT = 0;										\
							CLK(17 + LEN*6);								\
							while(LEN--)									\
							{												\
								write_8_NORM(DST++, read_8_NORM(SRC+DAT));	\
								DAT ^= 1;									\
							}

/* H6280   Transfer Accumulator to MPR# */
#define OP_TAM()			DST = OPER_IMM();								\
							if(DST & 0x01) SET_REG_MPR(0, REG_A);			\
							if(DST & 0x02) SET_REG_MPR(1, REG_A);			\
							if(DST & 0x04) SET_REG_MPR(2, REG_A);			\
							if(DST & 0x08) SET_REG_MPR(3, REG_A);			\
							if(DST & 0x10) SET_REG_MPR(4, REG_A);			\
							if(DST & 0x20) SET_REG_MPR(5, REG_A);			\
							if(DST & 0x40) SET_REG_MPR(6, REG_A);			\
							if(DST & 0x80) SET_REG_MPR(7, REG_A);			\

/* H6280   Transfer Decrement Decrement */
#define OP_TDD()			SRC = OPER_IMM16();								\
							DST = OPER_IMM16();								\
							LEN = OPER_IMM16();								\
							CLK(17 + LEN*6);								\
							while(LEN--)									\
								write_8_NORM(DST--, read_8_NORM(SRC--))

/* H6280   Transfer Increment Alternate */
#define OP_TIA()			SRC = OPER_IMM16();								\
							DST = OPER_IMM16();								\
							LEN = OPER_IMM16();								\
							DAT = 0;										\
							CLK(17 + LEN*6);								\
							while(LEN--)									\
							{												\
								write_8_NORM(DST+DAT, read_8_NORM(SRC++));	\
								DAT ^= 1;									\
							}

/* H6280   Transfer Increment Increment */
#define OP_TII()			SRC = OPER_IMM16();								\
							DST = OPER_IMM16();								\
							LEN = OPER_IMM16();								\
							CLK(17 + LEN*6);								\
							while(LEN--)									\
								write_8_NORM(DST++, read_8_NORM(SRC++))

/* H6280   Transfer Increment None */
#define OP_TIN()			SRC = OPER_IMM16();								\
							DST = OPER_IMM16();								\
							LEN = OPER_IMM16();								\
							CLK(17 + LEN*6);								\
							while(LEN--)									\
								write_8_NORM(DST, read_8_NORM(SRC++))

/* H6280   Transfer MPR# to accumulator */
#define OP_TMA()			SRC = OPER_IMM();								\
							if(SRC)											\
								REG_A = GET_REG_MPR((int)h6280i_mpr_num[SRC])

/* M6502   Transfer source register to destination register */
#define OP_TRANS(RS, RD)	FLAG_N = FLAG_Z = RD = RS

/* M65C02  Test and Reset Bits */
#define OP_TRB(MODE)		DST = EA_##MODE();								\
							FLAG_N = read_8_##MODE(DST);					\
							SRC = FLAG_N & ~REG_A;							\
							write_8_##MODE(DST, SRC);						\
							FLAG_Z = FLAG_N & REG_A;						\
							FLAG_V = FLAG_N << 1

/* M65C02  Test and Set Bits */
#define OP_TSB(MODE)		DST = EA_##MODE();								\
							FLAG_N = read_8_##MODE(DST);					\
							SRC = FLAG_N | REG_A;							\
							write_8_##MODE(DST, SRC);						\
							FLAG_Z = FLAG_N & REG_A;						\
							FLAG_V = FLAG_N << 1

/* H6280   Test and Mask Bits */
#define OP_TST(MODE)		FLAG_N = OPER_IMM();							\
							FLAG_Z = OPER_##MODE() & FLAG_N;				\
							FLAG_V = FLAG_N << 1

/* M6502   Transfer X to the stack pointer */
#define OP_TXS()			REG_S = REG_X

/* M6502   Illegal instruction */
#define OP_ILL()



/* ======================================================================== */
/* ========================== MAME-SPECIFIC API =========================== */
/* ======================================================================== */

#if H6280_USING_MAME

static UINT8 h6280_register_layout[] = {
	H6280_PC, H6280_S, H6280_P, H6280_A, H6280_X, H6280_Y, -1,
	H6280_IRQ_MASK, H6280_TIMER_STATE, H6280_NMI_STATE, H6280_IRQ1_STATE, H6280_IRQ2_STATE, H6280_IRQT_STATE,
	-1,
	H6280_M1, H6280_M2, H6280_M3, H6280_M4, -1,
	H6280_M5, H6280_M6, H6280_M7, H6280_M8,
	0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 h6280_window_layout[] = {
	25, 0,55, 4,	/* register window (top rows) */
	 0, 0,24,22,	/* disassembler window (left colums) */
	25, 5,55, 8,	/* memory #1 window (right, upper middle) */
	25,14,55, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/* Get a formatted string representing a register and its contents */
const char *h6280_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	h6280i_cpu_struct* r = context;
	uint p;

	which = ++which % 16;
	buffer[which][0] = '\0';
	if(!context)
		r = &h6280i_cpu;

	p =  ((r->flag_n & 0x80)			|
			((r->flag_v & 0x80) >> 1)	|
			(r->flag_b << 4)			|
			(r->flag_d << 3)			|
			(r->flag_i << 2)			|
			((!r->flag_z) << 1)			|
			((r->flag_c >> 8)&1));

	 switch(regnum)
	{
		case CPU_INFO_REG+H6280_PC:			sprintf(buffer[which], "PC:%04X", r->pc); break;
		case CPU_INFO_REG+H6280_S:			sprintf(buffer[which], "S:%02X", r->s); break;
		case CPU_INFO_REG+H6280_P:			sprintf(buffer[which], "P:%02X", p); break;
		case CPU_INFO_REG+H6280_A:			sprintf(buffer[which], "A:%02X", r->a); break;
		case CPU_INFO_REG+H6280_X:			sprintf(buffer[which], "X:%02X", r->x); break;
		case CPU_INFO_REG+H6280_Y:			sprintf(buffer[which], "Y:%02X", r->y); break;
		case CPU_INFO_REG+H6280_NMI_STATE:	sprintf(buffer[which], "NMI:%X", r->line_nmi); break;
		case CPU_INFO_REG+H6280_IRQ1_STATE: sprintf(buffer[which], "IRQ1:%X", (r->int_states & INT_IRQ)); break;
		case CPU_INFO_REG+H6280_IRQ2_STATE: sprintf(buffer[which], "IRQ2:%X", (r->int_states & INT_IRQ2)); break;
		case CPU_INFO_REG+H6280_IRQT_STATE: sprintf(buffer[which], "IRQT:%X", (r->int_states & INT_TIQ)); break;
		case CPU_INFO_REG+H6280_M1: sprintf(buffer[which], "M1:%02X", r->mpr[0]); break;
		case CPU_INFO_REG+H6280_M2: sprintf(buffer[which], "M2:%02X", r->mpr[1]); break;
		case CPU_INFO_REG+H6280_M3: sprintf(buffer[which], "M3:%02X", r->mpr[2]); break;
		case CPU_INFO_REG+H6280_M4: sprintf(buffer[which], "M4:%02X", r->mpr[3]); break;
		case CPU_INFO_REG+H6280_M5: sprintf(buffer[which], "M5:%02X", r->mpr[4]); break;
		case CPU_INFO_REG+H6280_M6: sprintf(buffer[which], "M6:%02X", r->mpr[5]); break;
		case CPU_INFO_REG+H6280_M7: sprintf(buffer[which], "M7:%02X", r->mpr[6]); break;
		case CPU_INFO_REG+H6280_M8: sprintf(buffer[which], "M8:%02X", r->mpr[7]); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				p & 0x80 ? 'N':'.',
				p & 0x40 ? 'V':'.',
				p & 0x20 ? 'T':'.',
				p & 0x10 ? 'B':'.',
				p & 0x08 ? 'D':'.',
				p & 0x04 ? 'I':'.',
				p & 0x02 ? 'Z':'.',
				p & 0x01 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "H6280";
		case CPU_INFO_FAMILY: return "Hudson 6280";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return ", all rights reserved.";
		case CPU_INFO_REG_LAYOUT: return (const char*)h6280_register_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)h6280_window_layout;
	}
	return buffer[which];
}

/* Save the current CPU state to disk */
void h6280_state_save(void *file)
{
	int cpu = cpu_getactivecpu();
	UINT16 v16;
	UINT8 v8;

	v8 = (UINT8)REG_A;
	state_save_UINT8(file,"h6280",cpu,"A",&v8,1);
	v8 = (UINT8)REG_X;
	state_save_UINT8(file,"h6280",cpu,"X",&v8,1);
	v8 = (UINT8)REG_Y;
	state_save_UINT8(file,"h6280",cpu,"Y",&v8,1);
	v8 = (UINT8)REG_S;
	state_save_UINT8(file,"h6280",cpu,"S",&v8,1);
	v16 = (UINT16)REG_PC;
	state_save_UINT16(file,"h6280",cpu,"PC",&v16,2);
	v16 = (UINT16)REG_PPC;
	state_save_UINT16(file,"h6280",cpu,"PPC",&v16,2);
	v8 = (UINT8) GET_REG_P();
	state_save_UINT8(file,"h6280",cpu,"P",&v8,1);
	v8 = (UINT8)GET_REG_MPR(0);
	state_save_UINT8(file,"h6280",cpu,"MPR0",&v8,1);
	v8 = (UINT8)GET_REG_MPR(1);
	state_save_UINT8(file,"h6280",cpu,"MPR1",&v8,1);
	v8 = (UINT8)GET_REG_MPR(2);
	state_save_UINT8(file,"h6280",cpu,"MPR2",&v8,1);
	v8 = (UINT8)GET_REG_MPR(3);
	state_save_UINT8(file,"h6280",cpu,"MPR3",&v8,1);
	v8 = (UINT8)GET_REG_MPR(4);
	state_save_UINT8(file,"h6280",cpu,"MPR4",&v8,1);
	v8 = (UINT8)GET_REG_MPR(5);
	state_save_UINT8(file,"h6280",cpu,"MPR5",&v8,1);
	v8 = (UINT8)GET_REG_MPR(6);
	state_save_UINT8(file,"h6280",cpu,"MPR6",&v8,1);
	v8 = (UINT8)GET_REG_MPR(7);
	state_save_UINT8(file,"h6280",cpu,"MPR7",&v8,1);
	v8 = (UINT8)REG_TIMER_PERIOD;
	state_save_UINT8(file,"h6280",cpu,"TIMER_PERIOD",&v8,1);
	v8 = (UINT8)REG_TIMER_VALUE;
	state_save_UINT8(file,"h6280",cpu,"TIMER_VALUE",&v8,1);
	v8 = (UINT8)REG_TIMER_ENABLE;
	state_save_UINT8(file,"h6280",cpu,"TIMER_ENABLE",&v8,1);
	v8 = (UINT8)REG_INT_MASK;
	state_save_UINT8(file,"h6280",cpu,"INT_MASK",&v8,1);
	v8 = (UINT8)REG_INT_STATES;
	state_save_UINT8(file,"h6280",cpu,"INT_STATES",&v8,1);
	v8 = (UINT8)PENDING_INTS;
	state_save_UINT8(file,"h6280",cpu,"PENDING_INTS",&v8,1);
	v8 = (UINT8)LINE_NMI;
	state_save_UINT8(file,"h6280",cpu,"LINE_NMI",&v8,1);
	v8 = (UINT8)LINE_RST;
	state_save_UINT8(file,"h6280",cpu,"LINE_RST",&v8,1);
}

/* Load a CPU state from disk */
void h6280_state_load(void *file)
{
	int cpu = cpu_getactivecpu();
	UINT16 v16;
	UINT8 v8;

	state_load_UINT8(file,"h6280",cpu,"A",&v8,1);
	REG_A = v8;
	state_load_UINT8(file,"h6280",cpu,"X",&v8,1);
	REG_X = v8;
	state_load_UINT8(file,"h6280",cpu,"Y",&v8,1);
	REG_Y = v8;
	state_load_UINT8(file,"h6280",cpu,"S",&v8,1);
	REG_S = v8;
	state_load_UINT16(file,"h6280",cpu,"PC",&v16,2);
	REG_PC = v16;
	state_load_UINT16(file,"h6280",cpu,"PPC",&v16,2);
	REG_PPC = v16;
	state_load_UINT8(file,"h6280",cpu,"P",&v8,1);
	SET_REG_P(v8);
	state_load_UINT8(file,"h6280",cpu,"MPR0",&v8,1);
	SET_REG_MPR(0, v8);
	state_load_UINT8(file,"h6280",cpu,"MPR1",&v8,1);
	SET_REG_MPR(1, v8);
	state_load_UINT8(file,"h6280",cpu,"MPR2",&v8,1);
	SET_REG_MPR(2, v8);
	state_load_UINT8(file,"h6280",cpu,"MPR3",&v8,1);
	SET_REG_MPR(3, v8);
	state_load_UINT8(file,"h6280",cpu,"MPR4",&v8,1);
	SET_REG_MPR(4, v8);
	state_load_UINT8(file,"h6280",cpu,"MPR5",&v8,1);
	SET_REG_MPR(5, v8);
	state_load_UINT8(file,"h6280",cpu,"MPR6",&v8,1);
	SET_REG_MPR(6, v8);
	state_load_UINT8(file,"h6280",cpu,"MPR7",&v8,1);
	SET_REG_MPR(7, v8);
	state_load_UINT8(file,"h6280",cpu,"TIMER_PERIOD",&v8,1);
	REG_TIMER_PERIOD = v8;
	state_load_UINT8(file,"h6280",cpu,"TIMER_VALUE",&v8,1);
	REG_TIMER_VALUE = v8;
	state_load_UINT8(file,"h6280",cpu,"TIMER_ENABLE",&v8,1);
	REG_TIMER_ENABLE = v8;
	state_load_UINT8(file,"h6280",cpu,"INT_MASK",&v8,1);
	REG_INT_MASK = v8;
	state_load_UINT8(file,"h6280",cpu,"INT_STATES",&v8,1);
	REG_INT_STATES = v8;
	state_load_UINT8(file,"h6280",cpu,"PENDING_INTS",&v8,1);
	PENDING_INTS = v8;
	state_load_UINT8(file,"h6280",cpu,"LINE_NMI",&v8,1);
	LINE_NMI = v8;
	state_load_UINT8(file,"h6280",cpu,"LINE_RST",&v8,1);
	LINE_RST = v8;
}

#include "mamedbg.h"
#ifdef MAME_DEBUG

extern unsigned Dasm6280(char *buffer, unsigned pc);
UINT8 h6280_debug_mmr[8];

#undef H6280_CALL_DEBUGGER
#define H6280_CALL_DEBUGGER							\
	for(int didx=7;didx>=0;didx--)					\
		h6280_debug_mmr[didx] = GET_REG_MPR(didx);	\
	CALL_MAME_DEBUG

#endif /* MAME_DEBUG */

/* Disassemble an instruction */
unsigned h6280_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm6280(buffer, pc);
#else
	sprintf(buffer, "$%02X", read_8_instruction(pc));
	return 1;
#endif
}

/* Exit and clean up */
void h6280_exit(void)
{
	/* nothing to do */
}

#endif /* H6280_USING_MAME */



/* ======================================================================== */
/* ================================= API ================================== */
/* ======================================================================== */

/* --------------------- */
/* CPU Peek and Poke API */
/* --------------------- */

/* Get the current CPU context */
unsigned h6280_get_context(void *dst_context)
{
	if(dst_context)
		*(h6280i_cpu_struct*)dst_context = h6280i_cpu;
	return sizeof(h6280i_cpu_struct);
}

/* Set the current CPU context */
void h6280_set_context(void *src_context)
{
	if(src_context)
	{
		h6280i_cpu = *(h6280i_cpu_struct*)src_context;
		h6280_jumping(REG_PC);
	}
}

/* Get the current Program Counter */
unsigned h6280_get_pc(void)
{
	return MAKE_UINT_16(REG_PC);
}

/* Set the Program Counter */
void h6280_set_pc(unsigned val)
{
	jump(val);
}

/* Get the current Stack Pointer */
unsigned h6280_get_sp(void)
{
	return REG_S + STACK_PAGE;
}

/* Set the Stack Pointer */
void h6280_set_sp(unsigned val)
{
	REG_S = MAKE_UINT_8(val);
}

/* Get a register from the CPU core */
unsigned h6280_get_reg(int regnum)
{
	switch(regnum)
	{
		case H6280_PC: return MAKE_UINT_16(REG_PC);
		case H6280_S: return REG_S + STACK_PAGE;
		case H6280_P: return GET_REG_P();
		case H6280_A: return REG_A;
		case H6280_X: return REG_X;
		case H6280_Y: return REG_Y;
		case H6280_NMI_STATE: return LINE_NMI;
		case H6280_IRQ_MASK: return REG_INT_MASK;
		case H6280_TIMER_STATE: return REG_TIMER_ENABLE;
		case H6280_IRQ1_STATE: return (REG_INT_STATES & INT_IRQ) != 0;
		case H6280_IRQ2_STATE: return (REG_INT_STATES & INT_IRQ2) != 0;
		case H6280_IRQT_STATE: return (REG_INT_STATES & INT_TIQ) != 0;
		case H6280_M1: return GET_REG_MPR(0);
		case H6280_M2: return GET_REG_MPR(1);
		case H6280_M3: return GET_REG_MPR(2);
		case H6280_M4: return GET_REG_MPR(3);
		case H6280_M5: return GET_REG_MPR(4);
		case H6280_M6: return GET_REG_MPR(5);
		case H6280_M7: return GET_REG_MPR(6);
		case H6280_M8: return GET_REG_MPR(7);
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
void h6280_set_reg(int regnum, unsigned val)
{
	switch(regnum)
	{
		case H6280_PC: jump(val); break;
		case H6280_S: REG_S = MAKE_UINT_8(val); break;
		case H6280_P: SET_REG_P(val); break;
		case H6280_A: REG_A = MAKE_UINT_8(val); break;
		case H6280_X: REG_X = MAKE_UINT_8(val); break;
		case H6280_Y: REG_Y = MAKE_UINT_8(val); break;
		case H6280_NMI_STATE: h6280_set_nmi_line(val); break;
		case H6280_TIMER_STATE: REG_TIMER_ENABLE = val & 1; break;
		case H6280_IRQ1_STATE: h6280_set_irq_line( INT_IRQ, val ); break;
		case H6280_IRQ2_STATE: h6280_set_irq_line( INT_IRQ2, val ); break;
		case H6280_IRQT_STATE: h6280_set_irq_line( INT_TIQ, val ); break;
		case H6280_M1: SET_REG_MPR(0, val); break;
		case H6280_M2: SET_REG_MPR(1, val); break;
		case H6280_M3: SET_REG_MPR(2, val); break;
		case H6280_M4: SET_REG_MPR(3, val); break;
		case H6280_M5: SET_REG_MPR(4, val); break;
		case H6280_M6: SET_REG_MPR(5, val); break;
		case H6280_M7: SET_REG_MPR(6, val); break;
		case H6280_M8: SET_REG_MPR(7, val); break;
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

/* Set the callback that is called when servicing an interrupt */
void h6280_set_irq_callback(int (*callback)(int))
{
	INT_ACK = callback;
}


/* -------------------------- */
/* CPU Hardware Interface API */
/* -------------------------- */

/* Assert or clear the RESET line of the CPU */
#if 0
static void* h6280i_set_reset_line_ptr;
void h6280_set_reset_line(int state, void* param)
{
	h6280i_set_reset_line_ptr = param;
	switch(state)
	{
		case CLEAR_LINE:
		case PULSE_LINE:
			memset(&h6280i_cpu, 0, sizeof(h6280i_cpu));
			LINE_RST = 0;
			SET_REG_MPR(7, 0x00);
			SET_REG_MPR(6, 0x05);
			SET_REG_MPR(5, 0x04);
			SET_REG_MPR(4, 0x03);
			SET_REG_MPR(3, 0x02);
			SET_REG_MPR(2, 0x01);
			SET_REG_MPR(1, 0xf8);
			SET_REG_MPR(0, 0xff);
			REG_S = 0xff;
			FLAG_Z = 0; /* zero flag is "high" when it is zero */
			FLAG_I = FLAGPOS_I;
			INT_ACK = NULL;
			REG_TIMER_VALUE = 0;
			REG_TIMER_PERIOD = 0x7f;
			REG_TIMER_ENABLE = 0;
			REG_INT_MASK = 0;
			REG_INT_STATES = 0;
			PENDING_INTS = 0;
			LINE_NMI = 0;
			jump(read_16_VEC(VECTOR_RST));
			break;
		default:
			LINE_RST = 1;
			USE_ALL_CLKS();
	}
}
#endif

/* Assert or clear the NMI line of the CPU */
void h6280_set_nmi_line(int state)
{
	if(state == CLEAR_LINE)
	{
		LINE_NMI = 0;
		return;
	}
	if(!LINE_NMI)
	{
		LINE_NMI = 1;
		CLK(7);
		push_16(REG_PC);
		push_8(GET_REG_P_INT());
		FLAG_I = FLAGPOS_I;
		FLAG_D = 0;
		FLAG_B = 0;
		jump(read_16_VEC(VECTOR_NMI));
	}
}

/* Assert or clear the IRQ line of the CPU */
void h6280_set_irq_line(int line, int state)
{
	switch(line)
	{
		case 0: /* IRQ */
			if(state == CLEAR_LINE)
			{
				REG_INT_STATES &= ~INT_IRQ;		/* clear line */
				PENDING_INTS &= ~INT_IRQ;		/* remove from pending interrupt list */
				return;
			}
			REG_INT_STATES |= INT_IRQ;			/* set line */
			PENDING_INTS |= INT_IRQ;			/* add to pending interrupt list */
check_interrupts();																	\
			return;
		case 1: /* IRQ2 */
			if(state == CLEAR_LINE)
			{
				REG_INT_STATES &= ~INT_IRQ2;	/* clear line */
				PENDING_INTS &= ~INT_IRQ2;		/* remove from pending interrupt list */
				return;
			}
			REG_INT_STATES |= INT_IRQ2;			/* set line */
			PENDING_INTS |= INT_IRQ2;			/* add to pending interrupt list */
check_interrupts();																	\
			return;
	}
}


/* Pulse the SO pin (Set Overflow) */
void h6280_pulse_so(void)
{
	FLAG_V = 0x80;
}


/* Include the execute function */
#include "h6280exe.c"



/* ------------------------------------- */
/* Older stuff that will soon be removed */
/* ------------------------------------- */

/* Pulse the RESET line of the CPU */
void h6280_reset(void* param)
{
//	h6280_set_reset_line(PULSE_LINE, param);
	memset(&h6280i_cpu, 0, sizeof(h6280i_cpu));
	FLAG_I = FLAGPOS_I;
	REG_S = 0xff;
	jump(read_16_VEC(VECTOR_RST));
}

#include "comcpu.h"

/* Old get context function */
void h6280_common_get_cpu(common_cpu* cpu)
{
	cpu->a   = REG_A;
	cpu->x   = REG_X;
	cpu->y   = REG_Y;
	cpu->s   = REG_S;
	cpu->pc  = REG_PC & 0xffff;
	cpu->p   = GET_REG_P();
	cpu->nmi = LINE_NMI;
	cpu->irq_mask = REG_INT_MASK;
	cpu->irq_status = REG_INT_STATES | PENDING_INTS;
	cpu->mpr[0] = GET_REG_MPR(0);
	cpu->mpr[1] = GET_REG_MPR(1);
	cpu->mpr[2] = GET_REG_MPR(2);
	cpu->mpr[3] = GET_REG_MPR(3);
	cpu->mpr[4] = GET_REG_MPR(4);
	cpu->mpr[5] = GET_REG_MPR(5);
	cpu->mpr[6] = GET_REG_MPR(6);
	cpu->mpr[7] = GET_REG_MPR(7);
	cpu->ir  = REG_IR;
}

/* Old set context function */
void h6280_common_set_cpu(common_cpu* cpu)
{
	REG_A  = cpu->a;
	REG_X  = cpu->x;
	REG_Y  = cpu->y;
	REG_S  = cpu->s;
	REG_PC = cpu->pc;
	SET_REG_P(cpu->p);
	PENDING_INTS = REG_INT_STATES = cpu->irq_status;
	REG_INT_MASK = cpu->irq_mask;
	LINE_NMI = cpu->nmi;
	REG_IR = cpu->ir;
	SET_REG_MPR(0, cpu->mpr[0]);
	SET_REG_MPR(1, cpu->mpr[1]);
	SET_REG_MPR(2, cpu->mpr[2]);
	SET_REG_MPR(3, cpu->mpr[3]);
	SET_REG_MPR(4, cpu->mpr[4]);
	SET_REG_MPR(5, cpu->mpr[5]);
	SET_REG_MPR(6, cpu->mpr[6]);
	SET_REG_MPR(7, cpu->mpr[7]);
}

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */



int H6280_irq_status_r(int offset)
{
return 0;
}

void H6280_irq_status_w(int offset, int data)
{
}

int H6280_timer_r(int offset)
{
return 0;
}

void H6280_timer_w(int offset, int data)
{
}
