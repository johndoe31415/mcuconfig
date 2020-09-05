/**
 *	mcuconfig - Generation of microcontroller build setups.
 *	Copyright (C) 2019-2020 Johannes Bauer
 *
 *	This file is part of mcuconfig.
 *
 *	mcuconfig is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; this program is ONLY licensed under
 *	version 3 of the License, later versions are explicitly excluded.
 *
 *	mcuconfig is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with mcuconfig; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	Johannes Bauer <JohannesBauer@gmx.de>
**/

#include <stdbool.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_flash.h>
#include "system.h"

void default_fault_handler(void) {
	while (true);
}

%if conf["clocksrc"] in [ "hse-pll", "hsi-pll", "hse" ]:
<%
	if conf["clocksrc"] in [ "hsi", "hsi-pll" ]:
		base_clock = 8000000
	else:
		base_clock = conf["hse-freq"]

	cfgr_flags = set()
	if conf["clocksrc"] in [ "hse-pll", "hsi-pll" ]:
		if conf["clocksrc"] == "hse-pll":
			src_clk = base_clock
			cfgr_flags.add("PLLSRC")
		else:
			src_clk = base_clock / 2

		pll_muls = { mul: "RCC_CFGR_PLLMULL%d" % (mul) for mul in range(2, 17) }
		ideal_multiplier = conf["target_clk"] / src_clk
		(multiplier, flag) = h.get_multiplier_flag(ideal_multiplier, pll_muls)
		cfgr_flags.add(flag)
		sys_clock = src_clk * multiplier
	else:
		sys_clock = base_clock

	if sys_clock > 36e6:
		cfgr_flags.add("RCC_CFGR_PPRE1_DIV2")

%>\
static void clock_switch(void) {
%if conf["clocksrc"] in [ "hse-pll", "hse" ]:
	/* Enable HSE oscillator, ${"%.3f" % (base_clock / 1e6)} MHz */
	RCC->CR |= RCC_CR_HSEON;

	/* Wait for HSE to become ready */
	while (!(RCC->CR & RCC_CR_HSERDY));

	%endif
%if conf["clocksrc"] == "hse-pll":
	/* Source for PLL is HSE (base ${"%.3f" % (base_clock / 1e6)} MHz */
%elif conf["clocksrc"] == "hsi-pll":
	/* Source for PLL is HSI (base ${"%.3f" % (base_clock / 1e6)} MHz */
%endif
%if conf["clocksrc"] in [ "hse-pll", "hsi-pll" ]:
	/* Source clock for PLL ${"%.3f" % (src_clk / 1e6)} MHz * ${multiplier} = ${"%.3f" % (sys_clock / 1e6)} MHz */
%endif
%if sys_clock > 36e6:
	/* APB1 prescaler needs to be /2, APB1 clock is 36 MHz maximum */
%endif
	RCC->CFGR = ${" | ".join(sorted(cfgr_flags))};

	%if conf["clocksrc"] in [ "hse-pll", "hsi-pll" ]:
	/* Enable the PLL */
	RCC->CR |= RCC_CR_PLLON;

	/* Wait for PLL to become ready */
	while (!(RCC->CR & RCC_CR_PLLRDY));
	%endif

	%if sys_clock <= 24e6:
	/* No Flash wait states needed below 24 MHz SYSCLK (SYSCLK = ${"%.3f" % (sys_clock / 1e6)} MHz) */
	%elif sys_clock <= 48e6:
	/* One Flash wait state needed between 24 MHz and 48 MHz SYSCLK (SYSCLK = ${"%.3f" % (sys_clock / 1e6)} MHz) */
	FLASH_SetLatency(FLASH_Latency_1);
	%else:
	/* Two Flash wait state needed between 48 MHz and 72 MHz SYSCLK (SYSCLK = ${"%.3f" % (sys_clock / 1e6)} MHz) */
	FLASH_SetLatency(FLASH_Latency_2);
	%endif

	%if conf["clocksrc"] in [ "hse-pll", "hsi-pll" ]:
	/* Switch clock source to PLL */
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;

	/* Wait for PLL to become active clock */
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
	%elif conf["clocksrc"] == "hse":
	/* Switch clock source to HSE */
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSE;

	/* Wait for HSE to become active */
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE);
	%endif
	%if conf["clocksrc"] in [ "hse-pll", "hse" ]:

	/* Disable HSI to save power */
	RCC->CR &= ~RCC_CR_HSION;
	%endif
}
%elif conf["clocksrc"] == "hsi":
static void clock_switch(void) {
	/* Using HSI oscillator, ${"%.3f" % (base_clock / 1e6)} MHz */
}
%else:
error("Unsupported clock source: %s" % (conf["clocksrc"]))
%endif

void early_system_init(void) {
%if conf["clocksrc"] in [ "hse-pll", "hsi-pll", "hse", "hsi" ]:
	clock_switch();
%endif
}
