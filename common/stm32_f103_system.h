/*
 *  WARNING: THIS FILE IS AUTO-GENERATED. CHANGES WILL BE OVERWRITTEN.
 *  Generated at ${now.strftime("%Y-%m-%d %H:%M:%S")}
 *  Generated by https://github.com/johndoe31415/mcuconfig
 */

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

#ifndef __STM32_F103_SYSTEM_H__
#define __STM32_F103_SYSTEM_H__

#include <stm32f10x_gpio.h>

%for pin in pinmap:
<%
	pin_desc = [ "P%s%d" % (pin["pin"].port, pin["pin"].pin_no), "mode = %s" % (pin["mode"].name) ]
	if pin.get("invert"):
		pin_desc.append("inverted")
	if pin.get("speed") is not None:
		pin_desc.append("speed %s MHz" % (pin["speed"]))
	if pin.get("af") is not None:
		pin_desc.append("alternate function %s" % (pin["af"]))

%>\
// ${pin["name"]}: ${", ".join(pin_desc)}
#define ${pin["name"]}_PORT				GPIO${pin["pin"].port}
#define ${pin["name"]}_PIN				${pin["pin"].pin_no}
#define ${pin["name"]}_MASK				(1 << ${pin["name"]}_PIN)
%if pin["mode"].settable:
#define ${pin["name"]}_set_high()		${pin["name"]}_PORT->BSRR = ${pin["name"]}_MASK
#define ${pin["name"]}_set_low()		${pin["name"]}_PORT->BRR = ${pin["name"]}_MASK
%if not pin.get("invert", False):
#define ${pin["name"]}_set_active()		${pin["name"]}_set_high()
#define ${pin["name"]}_set_inactive()	${pin["name"]}_set_low()
%else:
#define ${pin["name"]}_set_active()		${pin["name"]}_set_low()
#define ${pin["name"]}_set_inactive()	${pin["name"]}_set_high()
%endif
#define ${pin["name"]}_toggle()			${pin["name"]}_PORT->ODR ^= ${pin["name"]}_MASK
%endif
#define ${pin["name"]}_get()			((${pin["name"]}_PORT->IDR >> ${pin["name"]}_PIN) & 1)
#define ${pin["name"]}_is_high()		(${pin["name"]}_get() != 0)
#define ${pin["name"]}_is_low()			(${pin["name"]}_get() == 0)
%if not pin.get("invert", False):
#define ${pin["name"]}_is_active()		${pin["name"]}_is_high()
#define ${pin["name"]}_is_inactive()	${pin["name"]}_is_low()
%else:
#define ${pin["name"]}_is_active()		${pin["name"]}_is_low()
#define ${pin["name"]}_is_inactive()	${pin["name"]}_is_high()
%endif

%endfor

void default_fault_handler(void);
void early_system_init(void);

#endif
