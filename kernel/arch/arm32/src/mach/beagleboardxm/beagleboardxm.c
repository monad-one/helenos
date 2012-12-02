/*
 * Copyright (c) 2012 Jan Vesely
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** @addtogroup arm32beagleboardxm
 * @{
 */
/** @file
 *  @brief BeagleBoard-xM platform driver.
 */

#include <arch/exception.h>
#include <arch/mach/beagleboardxm/beagleboardxm.h>
#include <genarch/drivers/amdm37x_irc/amdm37x_irc.h>
#include <genarch/drivers/amdm37x_uart/amdm37x_uart.h>
#include <genarch/drivers/amdm37x_gpt/amdm37x_gpt.h>
#include <genarch/drivers/amdm37x_dispc/amdm37x_dispc.h>
#include <genarch/fb/fb.h>
#include <genarch/srln/srln.h>
#include <interrupt.h>
#include <mm/km.h>
#include <ddi/ddi.h>
#include <ddi/device.h>

static void bbxm_init(void);
static void bbxm_timer_irq_start(void);
static void bbxm_cpu_halt(void);
static void bbxm_get_memory_extents(uintptr_t *start, size_t *size);
static void bbxm_irq_exception(unsigned int exc_no, istate_t *istate);
static void bbxm_frame_init(void);
static void bbxm_output_init(void);
static void bbxm_input_init(void);
static size_t bbxm_get_irq_count(void);
static const char *bbxm_get_platform_name(void);

#define BBXM_MEMORY_START	0x80000000	/* physical */
#define BBXM_MEMORY_SIZE	0x20000000	/* 512 MB */

static struct beagleboard {
	amdm37x_dispc_regs_t *dispc;
	amdm37x_irc_regs_t *irc_addr;
	amdm37x_uart_t uart;
	amdm37x_gpt_t timer;
} beagleboard;

struct arm_machine_ops bbxm_machine_ops = {
	.machine_init = bbxm_init,
	.machine_timer_irq_start = bbxm_timer_irq_start,
	.machine_cpu_halt = bbxm_cpu_halt,
	.machine_get_memory_extents = bbxm_get_memory_extents,
	.machine_irq_exception = bbxm_irq_exception,
	.machine_frame_init = bbxm_frame_init,
	.machine_output_init = bbxm_output_init,
	.machine_input_init = bbxm_input_init,
	.machine_get_irq_count = bbxm_get_irq_count,
	.machine_get_platform_name = bbxm_get_platform_name
};

static irq_ownership_t bb_timer_irq_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void bbxm_setup_fb(unsigned width, unsigned height, unsigned bpp)
{
	const unsigned pixel_bytes = (bpp / 8);
	const size_t size = ALIGN_UP(width * height * pixel_bytes, FRAME_SIZE);
	const unsigned frames = size / FRAME_SIZE;
	unsigned order = 0;
	unsigned frame = 1;
	while (frame < frames) {
		frame *= 2;
		++order;
	}
	/* prefer highmem as we don't care about virtual mapping. */
	void *buffer = frame_alloc(order, FRAME_LOWMEM);
	if (!buffer) {
		printf("Failed to allocate framebuffer.\n");
		return;
	}

	amdm37x_dispc_setup_fb(beagleboard.dispc, width, height, bpp,
	    (uintptr_t) buffer);

	fb_properties_t prop = {
		.addr = (uintptr_t)buffer,
		.offset = 0,
		.x = width,
		.y = height,
		.scan = width * pixel_bytes,
		.visual = VISUAL_RGB_5_6_5_LE
	};
	switch (bpp)
	{
	case 8:
		prop.visual = VISUAL_INDIRECT_8; break;
	case 16:
		prop.visual = VISUAL_RGB_5_6_5_LE; break;
	case 24:
		prop.visual = VISUAL_BGR_8_8_8; break;
	case 32:
		prop.visual = VISUAL_RGB_8_8_8_0; break;
	default:
		printf("Invalid framebuffer bit depth: bailing out.\n");
		return;
	}
	outdev_t *fb_dev = fb_init(&prop);
	if (fb_dev)
		stdout_wire(fb_dev);

}

static void bb_timer_irq_handler(irq_t *irq)
{
        /*
         * We are holding a lock which prevents preemption.
         * Release the lock, call clock() and reacquire the lock again.
         */
	amdm37x_gpt_irq_ack(&beagleboard.timer);
	spinlock_unlock(&irq->lock);
	clock();
	spinlock_lock(&irq->lock);
}

static void bbxm_init(void)
{
	/* Initialize interrupt controller */
	beagleboard.irc_addr =
	    (void *) km_map(AMDM37x_IRC_BASE_ADDRESS, AMDM37x_IRC_SIZE,
	    PAGE_NOT_CACHEABLE);
	ASSERT(beagleboard.irc_addr);
	amdm37x_irc_init(beagleboard.irc_addr);

	/* Map display controller */
	beagleboard.dispc = (void*) km_map(AMDM37x_DISPC_BASE_ADDRESS,
	    AMDM37x_DISPC_SIZE, PAGE_NOT_CACHEABLE);
	ASSERT(beagleboard.dispc);

	/* Initialize timer. Use timer1, because it is in WKUP power domain
	 * (always on) and has special capabilities for precise 1ms ticks */
	amdm37x_gpt_timer_ticks_init(&beagleboard.timer,
	    AMDM37x_GPT1_BASE_ADDRESS, AMDM37x_GPT1_SIZE, HZ);
}

static void bbxm_timer_irq_start(void)
{
	/* Initialize timer IRQ */
	static irq_t timer_irq;
	irq_initialize(&timer_irq);
	timer_irq.devno = device_assign_devno();
	timer_irq.inr = AMDM37x_GPT1_IRQ;
	timer_irq.claim = bb_timer_irq_claim;
	timer_irq.handler = bb_timer_irq_handler;
	irq_register(&timer_irq);

	/* Enable timer interrupt */
	amdm37x_irc_enable(beagleboard.irc_addr, AMDM37x_GPT1_IRQ);

	/* Start timer here */
	amdm37x_gpt_timer_ticks_start(&beagleboard.timer);
}

static void bbxm_cpu_halt(void)
{
	while (1);
}

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address (physical).
 * @param size		Place to store memory size.
 */
static void bbxm_get_memory_extents(uintptr_t *start, size_t *size)
{
	*start = BBXM_MEMORY_START;
	*size = BBXM_MEMORY_SIZE;
}

static void bbxm_irq_exception(unsigned int exc_no, istate_t *istate)
{
	const unsigned inum = amdm37x_irc_inum_get(beagleboard.irc_addr);
	amdm37x_irc_irq_ack(beagleboard.irc_addr);

	irq_t *irq = irq_dispatch_and_lock(inum);
	if (irq) {
		/* The IRQ handler was found. */
		irq->handler(irq);
		spinlock_unlock(&irq->lock);
	} else {
		/* Spurious interrupt.*/
		printf("cpu%d: spurious interrupt (inum=%d)\n",
		    CPU->id, inum);
	}
}

static void bbxm_frame_init(void)
{
}

static void bbxm_output_init(void)
{
#ifdef CONFIG_FB
	bbxm_setup_fb(CONFIG_BFB_WIDTH, CONFIG_BFB_HEIGHT, CONFIG_BFB_BPP);
#else
	(void)bbxm_setup_fb;
#endif
	/* UART3 is wired to external RS232 connector */
	const bool ok = amdm37x_uart_init(&beagleboard.uart,
	    AMDM37x_UART3_IRQ, AMDM37x_UART3_BASE_ADDRESS, AMDM37x_UART3_SIZE);
	if (ok) {
		stdout_wire(&beagleboard.uart.outdev);
	}
}

static void bbxm_input_init(void)
{
	srln_instance_t *srln_instance = srln_init();
	if (srln_instance) {
		indev_t *sink = stdin_wire();
		indev_t *srln = srln_wire(srln_instance, sink);
		amdm37x_uart_input_wire(&beagleboard.uart, srln);
		amdm37x_irc_enable(beagleboard.irc_addr, AMDM37x_UART3_IRQ);
	}
}

size_t bbxm_get_irq_count(void)
{
	return AMDM37x_IRC_IRQ_COUNT;
}

const char *bbxm_get_platform_name(void)
{
	return "beagleboardxm";
}

/**
 * @}
 */