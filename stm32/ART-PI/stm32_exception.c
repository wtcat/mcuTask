/*
 * Copyright 2024 wtcat
 */
#include "tx_api.h"
#include "stm32h7xx.h"

typedef unsigned int __u32;
struct callee_saved {
	__u32 r4;
	__u32 r5;
	__u32 r6;
	__u32 r7;
	__u32 r8;
	__u32 r9;
	__u32 r10;
	__u32 r11;
	__u32 psp;
};

struct excep_frame {
    __u32 r0;
    __u32 r1;
    __u32 r2;
    __u32 r3;
    __u32 r12;
    __u32 r14;
    __u32 r15;
    __u32 xpsr;
};

#define PR_FAULT_INFO PR_EXC
#define PR_EXC(fmt, ...)    printk(fmt"\n", ##__VA_ARGS__)
#define STORE_xFAR(reg_var, reg) __u32 reg_var = (__u32)reg
#define __ASSERT(cond, msg) \
do { \
    if (!(cond)) { \
        printk("%s\n", msg); \
        for ( ; ; ); \
    } \
} while (0)

/* helpers to access memory/bus/usage faults */
#define SCB_CFSR_MEMFAULTSR \
	(__u32)((SCB->CFSR & SCB_CFSR_MEMFAULTSR_Msk) \
		   >> SCB_CFSR_MEMFAULTSR_Pos)
#define SCB_CFSR_BUSFAULTSR \
	(__u32)((SCB->CFSR & SCB_CFSR_BUSFAULTSR_Msk) \
		   >> SCB_CFSR_BUSFAULTSR_Pos)
#define SCB_CFSR_USGFAULTSR \
	(__u32)((SCB->CFSR & SCB_CFSR_USGFAULTSR_Msk) \
		   >> SCB_CFSR_USGFAULTSR_Pos)


static void fault_show(const void *ef, int fault) {
    (void) ef;
	PR_EXC("Fault! EXC #%d", fault);
	PR_EXC("MMFSR: 0x%x, BFSR: 0x%x, UFSR: 0x%x", SCB_CFSR_MEMFAULTSR,
	       SCB_CFSR_BUSFAULTSR, SCB_CFSR_USGFAULTSR);
}

static void mem_manage_fault(struct excep_frame *esf, int from_hard_fault) {
	PR_FAULT_INFO("***** MPU FAULT *****");

	if ((SCB->CFSR & SCB_CFSR_MSTKERR_Msk) != 0) {
		PR_FAULT_INFO("  Stacking error (context area might be"
			" not valid)");
	}
	if ((SCB->CFSR & SCB_CFSR_MUNSTKERR_Msk) != 0) {
		PR_FAULT_INFO("  Unstacking error");
	}
	if ((SCB->CFSR & SCB_CFSR_DACCVIOL_Msk) != 0) {
		PR_FAULT_INFO("  Data Access Violation");
		/* In a fault handler, to determine the true faulting address:
		 * 1. Read and save the MMFAR value.
		 * 2. Read the MMARVALID bit in the MMFSR.
		 * The MMFAR address is valid only if this bit is 1.
		 *
		 * Software must follow this sequence because another higher
		 * priority exception might change the MMFAR value.
		 */
		__u32 temp = SCB->MMFAR;

		if ((SCB->CFSR & SCB_CFSR_MMARVALID_Msk) != 0) {
			PR_EXC("  MMFAR Address: 0x%x", temp);
			if (from_hard_fault != 0) {
				/* clear SCB_MMAR[VALID] to reset */
				SCB->CFSR &= ~SCB_CFSR_MMARVALID_Msk;
			}
		}
	}
	if ((SCB->CFSR & SCB_CFSR_IACCVIOL_Msk) != 0) {
		PR_FAULT_INFO("  Instruction Access Violation");
	}

	if ((SCB->CFSR & SCB_CFSR_MLSPERR_Msk) != 0) {
		PR_FAULT_INFO(
			"  Floating-point lazy state preservation error");
	}


	/* When stack protection is enabled, we need to assess
	 * if the memory violation error is a stack corruption.
	 *
	 * By design, being a Stacking MemManage fault is a necessary
	 * and sufficient condition for a thread stack corruption.
	 * [Cortex-M process stack pointer is always descending and
	 * is never modified by code (except for the context-switch
	 * routine), therefore, a stacking error implies the PSP has
	 * crossed into an area beyond the thread stack.]
	 *
	 * Data Access Violation errors may or may not be caused by
	 * thread stack overflows.
	 */
	if ((SCB->CFSR & SCB_CFSR_MSTKERR_Msk) ||
		(SCB->CFSR & SCB_CFSR_DACCVIOL_Msk)) {

	__ASSERT(!(SCB->CFSR & SCB_CFSR_MSTKERR_Msk),
		"Stacking or Data Access Violation error "
		"without stack guard, user-mode or null-pointer detection\n");
	}

	/* When we were handling this fault, we may have triggered a fp
	 * lazy stacking Memory Manage fault. At the time of writing, this
	 * can happen when printing.  If that's true, we should clear the
	 * pending flag in addition to the clearing the reason for the fault
	 */
	if ((SCB->CFSR & SCB_CFSR_MLSPERR_Msk) != 0)
		SCB->SHCSR &= ~SCB_SHCSR_MEMFAULTPENDED_Msk;

	/* clear MMFSR sticky bits */
	SCB->CFSR |= SCB_CFSR_MEMFAULTSR_Msk;
}

static void bus_fault(struct excep_frame *esf, int from_hard_fault) {
	PR_FAULT_INFO("***** BUS FAULT *****");

	if (SCB->CFSR & SCB_CFSR_STKERR_Msk) {
		PR_FAULT_INFO("  Stacking error");
	}
	if (SCB->CFSR & SCB_CFSR_UNSTKERR_Msk) {
		PR_FAULT_INFO("  Unstacking error");
	}
	if (SCB->CFSR & SCB_CFSR_PRECISERR_Msk) {
		PR_FAULT_INFO("  Precise data bus error");
		/* In a fault handler, to determine the true faulting address:
		 * 1. Read and save the BFAR value.
		 * 2. Read the BFARVALID bit in the BFSR.
		 * The BFAR address is valid only if this bit is 1.
		 *
		 * Software must follow this sequence because another
		 * higher priority exception might change the BFAR value.
		 */
		STORE_xFAR(bfar, SCB->BFAR);

		if ((SCB->CFSR & SCB_CFSR_BFARVALID_Msk) != 0) {
			PR_EXC("  BFAR Address: 0x%x", bfar);
			if (from_hard_fault != 0) {
				/* clear SCB_CFSR_BFAR[VALID] to reset */
				SCB->CFSR &= ~SCB_CFSR_BFARVALID_Msk;
			}
		}
	}
	if (SCB->CFSR & SCB_CFSR_IMPRECISERR_Msk) {
		PR_FAULT_INFO("  Imprecise data bus error");
	}
	if ((SCB->CFSR & SCB_CFSR_IBUSERR_Msk) != 0) {
		PR_FAULT_INFO("  Instruction bus error");

	} else if (SCB->CFSR & SCB_CFSR_LSPERR_Msk) {
		PR_FAULT_INFO("  Floating-point lazy state preservation error");
	} else {
		;
	}

	/* clear BFSR sticky bits */
	SCB->CFSR |= SCB_CFSR_BUSFAULTSR_Msk;
}

static void usage_fault(const struct excep_frame *esf) {

	PR_FAULT_INFO("***** USAGE FAULT *****");
	/* bits are sticky: they stack and must be reset */
	if ((SCB->CFSR & SCB_CFSR_DIVBYZERO_Msk) != 0) {
		PR_FAULT_INFO("  Division by zero");
	}
	if ((SCB->CFSR & SCB_CFSR_UNALIGNED_Msk) != 0) {
		PR_FAULT_INFO("  Unaligned memory access");
	}
	if ((SCB->CFSR & SCB_CFSR_NOCP_Msk) != 0) {
		PR_FAULT_INFO("  No coprocessor instructions");
	}
	if ((SCB->CFSR & SCB_CFSR_INVPC_Msk) != 0) {
		PR_FAULT_INFO("  Illegal load of EXC_RETURN into PC");
	}
	if ((SCB->CFSR & SCB_CFSR_INVSTATE_Msk) != 0) {
		PR_FAULT_INFO("  Illegal use of the EPSR");
	}
	if ((SCB->CFSR & SCB_CFSR_UNDEFINSTR_Msk) != 0) {
		PR_FAULT_INFO("  Attempt to execute undefined instruction");
	}

	/* clear UFSR sticky bits */
	SCB->CFSR |= SCB_CFSR_USGFAULTSR_Msk;
}

static void debug_monitor(struct excep_frame *esf) {
	PR_FAULT_INFO(
		"***** Debug monitor exception *****");
}

static void hard_fault(struct excep_frame *esf) {
	PR_FAULT_INFO("***** HARD FAULT *****");

	if ((SCB->HFSR & SCB_HFSR_VECTTBL_Msk) != 0) {
		PR_EXC("  Bus fault on vector table read");
	} else if ((SCB->HFSR & SCB_HFSR_DEBUGEVT_Msk) != 0) {
		PR_EXC("  Debug event");
	} else if ((SCB->HFSR & SCB_HFSR_FORCED_Msk) != 0) {
		PR_EXC("  Fault escalation (see below)");
		if ((SCB->CFSR & SCB_CFSR_MEMFAULTSR_Msk) != 0) {
			mem_manage_fault(esf, 1);
		} else if ((SCB->CFSR & SCB_CFSR_BUSFAULTSR_Msk) != 0) {
			bus_fault(esf, 1);
		} else if ((SCB->CFSR & SCB_CFSR_USGFAULTSR_Msk) != 0) {
			usage_fault(esf);
		} else {
			__ASSERT(0,
			"Fault escalation without FSR info");
		}
	} else {
		__ASSERT(0,
		"HardFault without HFSR info"
		" Shall never occur");
	}
}

static void reserved_exception(const void *ef, int fault) {
	(void) ef;
	PR_FAULT_INFO("***** %s %d) *****",
	       fault < 16 ? "Reserved Exception (" : "Spurious interrupt (IRQ ",
	       fault - 16);
}

static void fault_handle(struct excep_frame *esf, int fault) {
	switch (fault) {
	case 3:
		hard_fault(esf);
		break;
	case 4:
		mem_manage_fault(esf, 0);
		break;
	case 5:
		bus_fault(esf, 0);
		break;
	case 6:
		usage_fault(esf);
		break;
	case 12:
		debug_monitor(esf);
		break;
	default:
		reserved_exception(esf, fault);
		break;
	}

	fault_show(esf, fault);
}


static void esf_dump(const struct excep_frame *esf, const struct callee_saved *callee, 
    __u32 exec_ret) {
	PR_EXC("r0/a1:  0x%08x  r1/a2:  0x%08x  r2/a3:  0x%08x",
		esf->r0, esf->r1, esf->r2);
	PR_EXC("r3/a4:  0x%08x r12/ip:  0x%08x r14/lr:  0x%08x",
		esf->r3, esf->r12, esf->r14);
	PR_EXC(" xpsr:  0x%08x", esf->xpsr);

    PR_EXC("r4/v1:  0x%08x  r5/v2:  0x%08x  r6/v3:  0x%08x",
        callee->r4, callee->r5, callee->r6);
    PR_EXC("r7/v4:  0x%08x  r8/v5:  0x%08x  r9/v6:  0x%08x",
        callee->r7, callee->r8, callee->r9);
    PR_EXC("r10/v7: 0x%08x  r11/v8: 0x%08x    psp:  0x%08x",
        callee->r10, callee->r11, callee->psp);
	
	PR_EXC("EXC_RETURN: 0x%0x", exec_ret);
	PR_EXC("Faulting instruction address (r15/pc): 0x%08x", esf->r15);
}

static void __rte_used stm32_fault_process(void *msp, void *psp, __u32 exec_ret, 
    const struct callee_saved *callee) {
    int fault = SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;
    struct excep_frame *esf;

    if (exec_ret & (1 << 3))
        esf = psp;
    else
        esf = msp;

    fault_handle(esf, fault);
    esf_dump(esf, callee, exec_ret);
}

void __rte_naked _stm32_exception_handler(void) {
	__asm__ volatile (
        "mrs  r0, msp\n"
        "mrs  r1, psp\n"
        "push {r0, lr}\n"
        "push {r1, r2}\n"
        "push {r4-r11}\n"
        "mov  r3, sp\n"
        "mov  r2, lr\n"
        "bl   stm32_fault_process\n"
        "add  sp, #40\n"
        "pop  {r0, pc}\n"
        :
        :
        :
    );
    for ( ; ; );
}
