#ifndef H6280__HEADER
#define H6280__HEADER

/* ======================================================================== */
/* ============================= Configuration ============================ */
/* ======================================================================== */

/* Set this if your compiler can handle big switches.
 * Note: the big switch is too big for gcc 2.9.1 with optimizations on
 */
#define H6280_BIG_SWITCH		0

/* This uses a supposedly more correct ADC and SBC algorithm */
#define H6280_USE_NEW_ADC_SBC	0

/* Set this if you are using mame */
#define H6280_USING_MAME		0


/* Point this to your debugger code.  Leave this blank for MAME */
#define H6280_CALL_DEBUGGER



/* ======================================================================== */
/* =============================== DEFINES ================================ */
/* ======================================================================== */

#if !H6280_USING_MAME
#ifndef INLINE
#define INLINE static __inline__
#endif /* INLINE */
/* These are used with h6280_set_irq_line() */
#define CLEAR_LINE				0 /* clear (a fired, held or pulsed) line */
#define ASSERT_LINE				1 /* assert an interrupt immediately */
#define PULSE_LINE				3 /* pulse line for one instruction */

/* These are used with h6280_get_reg() and h6280_set_reg() */
#define REG_PREVIOUSPC	-1
#define REG_SP_CONTENTS -2

#endif /* H6280_USING_MAME */


/* Interrupt lines - used with h6280_set_irq_line() */
#define H6280_INT_NONE	0
#define H6280_INT_NMI	1
#define H6280_INT_TIMER	2
#define H6280_INT_IRQ1	3
#define H6280_INT_IRQ2	4


/* Registers - used by h6280_set_reg() and h6280_get_reg() */
enum
{
	H6280_PC=1, H6280_S, H6280_P, H6280_A, H6280_X, H6280_Y,
	H6280_IRQ_MASK, H6280_TIMER_STATE,
	H6280_NMI_STATE, H6280_IRQ1_STATE, H6280_IRQ2_STATE, H6280_IRQT_STATE,
	H6280_M1, H6280_M2, H6280_M3, H6280_M4,
	H6280_M5, H6280_M6, H6280_M7, H6280_M8
};



/* ======================================================================== */
/* ============================== PROTOTYPES ============================== */
/* ======================================================================== */

extern int h6280_ICount;				/* cycle count */



/* ======================================================================== */
/* ================================== API ================================= */
/* ======================================================================== */

/* --------------------- */
/* CPU Peek and Poke API */
/* --------------------- */

/* Get the current CPU context */
unsigned h6280_get_context(void *dst);

/* Set the current CPU context */
void h6280_set_context(void *src);

/* Get the current Program Counter */
unsigned h6280_get_pc(void);

/* Set the current Program Counter */
void h6280_set_pc(unsigned val);

/* Get the current Stack Pointer */
unsigned h6280_get_sp(void);

/* Set the current Stack Pointer */
void h6280_set_sp(unsigned val);

/* Get a register from the core */
unsigned h6280_get_reg(int regnum);

/* Set a register in the core */
void h6280_set_reg(int regnum, unsigned val);

/* Set the callback that will be called when an interrupt is serviced */
void h6280_set_irq_callback(int (*callback)(int));


/* -------------------------- */
/* CPU Hardware Interface API */
/* -------------------------- */

/* Set the RESET line on the CPU */
void h6280_set_reset_line(int state, void* param);

/* Note about NMI:
 *   NMI is a one-shot trigger.  In order to trigger NMI again, you must
 *   clear NMI and then assert it again.
 */
void h6280_set_nmi_line(int state);

/* Assert or clear the IRQ  or IRQ2 pin */
void h6280_set_irq_line(int line, int state);

/* Pulse the SO (Set Overflow) pin on the CPU */
void h6280_pulse_so(void);

/* Execute instructions for <clocks> CPU cycles */
int h6280_execute(int clocks);


/* ------------------------------------- */
/* Older stuff that will soon be removed */
/* ------------------------------------- */

/* Pulse the reset line */
void h6280_reset(void* param);



/* ======================================================================== */
/* =================== Functions Implemented by the Host ================== */
/* ======================================================================== */

/* Read data from RAM */
unsigned int h6280_read_8(unsigned int address);

/* Read data from the zero page */
unsigned int h6280_read_8_zeropage(unsigned int address);

/* Read data from ROM */
unsigned int h6280_read_8_immediate(unsigned int address);
unsigned int h6280_read_instruction(unsigned int address);

/* Write data to RAM */
void h6280_write_8(unsigned int address, unsigned int value);
void h6280_write_8_zeropage(unsigned int address, unsigned int value);

/* Port I/O */
unsigned int h6280_read_8_io(unsigned int port);
void h6280_write_8_io(unsigned int port, unsigned int value);

/* Notification of PC changes */
void h6280_jumping(unsigned int new_pc);
void h6280_branching(unsigned int new_pc);



/* ======================================================================== */
/* ================================= MAME ================================= */
/* ======================================================================== */

#if H6280_USING_MAME

/* Clean up after the emulation core - Not used in this core - */
void h6280_exit(void);

/* Save the current CPU state to disk */
void h6280_state_save(void *file);

/* Load a CPU state from disk */
void h6280_state_load(void *file);

/* Get a formatted string representing a register and its contents */
const char *h6280_info(void *context, int regnum);

/* Disassemble an instruction */
unsigned h6280_dasm(char *buffer, unsigned pc);


#include "cpuintrf.h"
#include "memory.h"
#include "driver.h"
#include "state.h"
#include "mamedbg.h"



#define h6280_read_8(addr)				cpu_readmem21(addr)
#define h6280_write_8(addr,data)		cpu_writemem21(addr,data)

#define h6280_read_8_io(addr)			cpu_readmem21(addr)
#define h6280_write_8_io(addr,data)		cpu_writemem21(addr,data)
#define h6280_read_8_zeropage(A)		cpu_readmem21(A)
#define h6280_write_8_zeropage(A, V)	cpu_writemem21(A, V)
#define h6280_read_instruction(A)		cpu_readmem21(A)
#define h6280_read_8_immediate(A)		cpu_readmem21(A)
#define h6280_jumping(A)
#define h6280_branching(A)


#else /* H6280_USING_MAME */

#define h6280_jumping(A)
#define h6280_branching(A)


#endif /* H6280_USING_MAME */



/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* H6280__HEADER */
