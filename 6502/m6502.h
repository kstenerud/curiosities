#ifndef M6502__HEADER
#define M6502__HEADER

#define PIGGYBACK
#define USING_MAME

/* ======================================================================== */
/* ============================= Configuration ============================ */
/* ======================================================================== */

#ifndef USING_MAME
#define HAS_M6502			1 /* Set to 1 if you will use m6502 */
#define HAS_M65C02			1 /* Set to 1 if you will use m65c02 */
#define HAS_M6510			1 /* Set to 1 if you will use m6510 */
#define HAS_N2A03			1 /* Set to 1 if you will use n2a03 */
#endif

#define M6502_STRICT_TIMING		1 /* Slower but accurate cycle count */
#define M6502_STRICT_IRQ		1 /* Slower but more accurate irq handling */

#define M6502_USE_NEW_ADC_SBC	0

#include "cpuintrf.h"
#include "osd_cpu.h"
#include "common.h"

#define M6502_INT_NONE			0
#define M6502_INT_IRQ			1
#define M6502_INT_NMI			2

#define M65C02_INT_NONE			M6502_INT_NONE
#define M65C02_INT_IRQ			M6502_INT_IRQ
#define M65C02_INT_NMI			M6502_INT_NMI

#define M6510_INT_NONE			M6502_INT_NONE
#define M6510_INT_IRQ			M6502_INT_IRQ
#define M6510_INT_NMI			M6502_INT_NMI

#define N2A03_INT_NONE			M6502_INT_NONE
#define N2A03_INT_IRQ			M6502_INT_IRQ
#define N2A03_INT_NMI			M6502_INT_NMI

#ifndef INLINE
#define INLINE static
#endif /* INLINE */
#ifndef CLEAR_LINE
#define CLEAR_LINE				0 /* clear (a fired, held or pulsed) line */
#endif /* CLEAR_LINE */
#ifndef ASSERT_LINE
#define ASSERT_LINE				1 /* assert an interrupt immediately */
#endif /* ASSERT_LINE */


/* CPU Structure */
typedef struct
{
	unsigned int a;		/* Accumulator */
	unsigned int x;		/* Index Register X */
	unsigned int y;		/* Index Register Y */
	unsigned int s;		/* Stack Pointer */
	unsigned int pc;	/* Program Counter */
	unsigned int p;		/* Processor Status Register */
	unsigned int irq;	/* Set if IRQ is asserted */
	unsigned int nmi;	/* Set if NMI is asserted */
	unsigned int ir;	/* Instruction Register */
} m6502_cpu_struct;

/* ======================================================================== */
/* ============================== PROTOTYPES ============================== */
/* ======================================================================== */

#ifdef USING_MAME
enum
{
	M6502_PC=1, M6502_S, M6502_P, M6502_A, M6502_X, M6502_Y,
	M6502_NMI_STATE, M6502_IRQ_STATE
};
#endif


/* ======================================================================== */
/* ============================= INTERFACE API ============================ */
/* ======================================================================== */

/* This is the interface, not the implementation.  Please call the  */
/* implementation APIs below.                                       */


/* Pulse the RESET pin on the CPU */
void m65xx_reset(void* param);

/* Set the RESET line on the CPU */
void m65xx_set_reset_line(int state, void* param);

/* Clean up after the emulation core - Not used in this core - */
void m65xx_exit(void);

/* Get the current CPU context */
unsigned m65xx_get_context(void *dst);

/* Set the current CPU context */
void m65xx_set_context(void *src);

/* Get the current Program Counter */
unsigned m65xx_get_pc(void);

/* Set the current Program Counter */
void m65xx_set_pc(unsigned val);

/* Get the current Stack Pointer */
unsigned m65xx_get_sp(void);

/* Set the current Stack Pointer */
void m65xx_set_sp(unsigned val);

/* Get a register from the core */
unsigned m65xx_get_reg(int regnum);

/* Set a register in the core */
void m65xx_set_reg(int regnum, unsigned val);

/* Note about NMI:
 *   NMI is a one-shot trigger.  In order to trigger NMI again, you must
 *   clear NMI and then assert it again.
 */
void m65xx_set_nmi_line(int state);

/* Assert or clear the IRQ pin */
void m65xx_set_irq_line(int line, int state);

/* Set the callback that will be called when an interrupt is serviced */
void m65xx_set_irq_callback(int (*callback)(int));

/* Save the current CPU state to disk */
void m65xx_state_save(void *file);

/* Load a CPU state from disk */
void m65xx_state_load(void *file);

/* Get a formatted string representing a register and its contents */
const char *m65xx_info(void *context, int regnum);

/* Disassemble an instruction */
unsigned m65xx_dasm(char *buffer, unsigned pc);


/* Pulse the SO (Set Overflow) pin on the CPU */
void m65xx_pulse_so(void);

/* Get the current CPU context - temporary - */
void m65xx_get_context_old(m6502_cpu_struct* cpu);

/* Set the current CPU context - temporary - */
void m65xx_set_context_old(m6502_cpu_struct* cpu);

extern int m65xx_ICount;				/* cycle count */


/* ======================================================================== */
/* ========================== IMPLEMENTATION API ========================== */
/* ======================================================================== */

/* Call these functions to use the m6502 */
#if HAS_M6502

#define m6502_ICount			m65xx_ICount
#define m6502_reset				m65xx_reset
#define m6502_exit				m65xx_exit
#define m6502_get_context		m65xx_get_context
#define m6502_set_context		m65xx_set_context
#define m6502_get_pc			m65xx_get_pc
#define m6502_set_pc			m65xx_set_pc
#define m6502_get_sp			m65xx_get_sp
#define m6502_set_sp			m65xx_set_sp
#define m6502_get_reg			m65xx_get_reg
#define m6502_set_reg			m65xx_set_reg
#define m6502_set_nmi_line		m65xx_set_nmi_line
#define m6502_set_irq_line		m65xx_set_irq_line
#define m6502_set_irq_callback	m65xx_set_irq_callback
#define m6502_state_save		m65xx_state_save
#define m6502_state_load		m65xx_state_load
#define m6502_info				m65xx_info
#define m6502_dasm				m65xx_dasm
#define m6502_pulse_so			m65xx_pulse_so

int m6502_execute(int clocks);

#endif /* HAS_M6502 */


/* Call these functions to use the m65c02 */
#if HAS_M65C02

#define m65c02_ICount			m65xx_ICount
#define m65c02_reset			m65xx_reset
#define m65c02_exit				m65xx_exit
#define m65c02_get_context		m65xx_get_context
#define m65c02_set_context		m65xx_set_context
#define m65c02_get_pc			m65xx_get_pc
#define m65c02_set_pc			m65xx_set_pc
#define m65c02_get_sp			m65xx_get_sp
#define m65c02_set_sp			m65xx_set_sp
#define m65c02_get_reg			m65xx_get_reg
#define m65c02_set_reg			m65xx_set_reg
#define m65c02_set_nmi_line		m65xx_set_nmi_line
#define m65c02_set_irq_line		m65xx_set_irq_line
#define m65c02_set_irq_callback	m65xx_set_irq_callback
#define m65c02_state_save		m65xx_state_save
#define m65c02_state_load		m65xx_state_load
#define m65c02_info				m65xx_info
#define m65c02_dasm				m65xx_dasm
#define m65c02_pulse_so			m65xx_pulse_so

int m65c02_execute(int clocks);

#endif /* HAS_M65C02 */


/* Call these functions to use the m6510 */
#if HAS_M6510

#define m6510_ICount			m65xx_ICount
#define m6510_reset				m65xx_reset
#define m6510_exit				m65xx_exit
#define m6510_get_context		m65xx_get_context
#define m6510_set_context		m65xx_set_context
#define m6510_get_pc			m65xx_get_pc
#define m6510_set_pc			m65xx_set_pc
#define m6510_get_sp			m65xx_get_sp
#define m6510_set_sp			m65xx_set_sp
#define m6510_get_reg			m65xx_get_reg
#define m6510_set_reg			m65xx_set_reg
#define m6510_set_nmi_line		m65xx_set_nmi_line
#define m6510_set_irq_line		m65xx_set_irq_line
#define m6510_set_irq_callback	m65xx_set_irq_callback
#define m6510_state_save		m65xx_state_save
#define m6510_state_load		m65xx_state_load
#define m6510_info				m65xx_info
#define m6510_dasm				m65xx_dasm
#define m6510_pulse_so			m65xx_pulse_so

int m6510_execute(int clocks);

#endif /* HAS_M6510 */


/* Call these functions to use the n2a03 */
#if HAS_N2A03

#define n2a03_ICount			m65xx_ICount
#define n2a03_reset				m65xx_reset
#define n2a03_exit				m65xx_exit
#define n2a03_get_context		m65xx_get_context
#define n2a03_set_context		m65xx_set_context
#define n2a03_get_pc			m65xx_get_pc
#define n2a03_set_pc			m65xx_set_pc
#define n2a03_get_sp			m65xx_get_sp
#define n2a03_set_sp			m65xx_set_sp
#define n2a03_get_reg			m65xx_get_reg
#define n2a03_set_reg			m65xx_set_reg
#define n2a03_set_nmi_line		m65xx_set_nmi_line
#define n2a03_set_irq_line		m65xx_set_irq_line
#define n2a03_set_irq_callback	m65xx_set_irq_callback
#define n2a03_state_save		m65xx_state_save
#define n2a03_state_load		m65xx_state_load
#define n2a03_info				m65xx_info
#define n2a03_dasm				m65xx_dasm
#define n2a03_pulse_so			m65xx_pulse_so

int n2a03_execute(int clocks);

#endif /* HAS_N2A03 */



/* ======================================================================== */
/* =================== Functions Implemented by the Host ================== */
/* ======================================================================== */

/* Read data from RAM */
unsigned int m65xx_read_8(unsigned int address);

/* Read data from the zero page */
unsigned int m65xx_read_8_zeropage(unsigned int address);

/* Read data from ROM */
unsigned int m65xx_read_8_immediate(unsigned int address);
unsigned int m65xx_read_instruction(unsigned int address);

/* Write data to RAM */
void m65xx_write_8(unsigned int address, unsigned int value);
void m65xx_write_8_zeropage(unsigned int address, unsigned int value);

void m65xx_jumping(unsigned int new_pc);
void m65xx_branching(unsigned int new_pc);



/* ======================================================================== */
/* ================================= MAME ================================= */
/* ======================================================================== */


#include "cpuintrf.h"
#include "memory.h"
#include "mamedbg.h"


#ifdef USING_MAME

#undef M6502_STRICT_TIMING
#undef M6502_STRICT_IRQ

#define M6502_STRICT_TIMING 1
#define M6502_STRICT_IRQ 1
#ifndef FAST_MEMORY
#define FAST_MEMORY 0
#endif

#if FAST_MEMORY

//extern  MHELE   *cur_mwhard;
//extern	MHELE	*cur_mrhard;
//extern	UINT8	*RAM;

#define m65xx_read_8(addr)                                 \
	((cur_mrhard[(addr) >> (ABITS2_16 + ABITS_MIN_16)]) ?  \
		cpu_readmem16(addr) : RAM[addr])

#define m65xx_write_8(addr,data)                           \
	if (cur_mwhard[(addr) >> (ABITS2_16 + ABITS_MIN_16)])  \
		cpu_writemem16(addr,data);                         \
	else                                                   \
		RAM[addr] = data

#else

#define m65xx_read_8(addr) cpu_readmem16(addr)
#ifndef PIGGYBACK
#define m65xx_write_8(addr,data) cpu_writemem16(addr,data)
#endif

#endif /* FAST_MEMORY */

#define m65xx_read_8_zeropage(A)     m65xx_read_8(A)
#define m65xx_write_8_zeropage(A, V) m65xx_write_8(A, V)
#define m65xx_read_instruction(A)    cpu_readop(A)
#define m65xx_read_8_immediate(A)    cpu_readop_arg(A)
#define m65xx_jumping(A)             change_pc16(A)
#define m65xx_branching(A)



#else


#include "ramhack.h"

extern int g_bcd_bad_value;

#define m65xx_read_8            ramhack_read_b
#define m65xx_read_8_zeropage   ramhack_read_b
#define m65xx_write_8           ramhack_write_b
#define m65xx_write_8_zeropage  ramhack_write_b
//#define m65xx_read_instruction  ramhack_read_b
#define m65xx_read_8_immediate  ramhack_read_b
#define m65xx_jumping(A)
#define m65xx_branching(A)


#endif

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* M6502__HEADER */

