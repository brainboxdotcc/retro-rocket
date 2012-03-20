; Hydrogen Operating System
; Copyright (C) 2011 Lukas Heidemann
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;-------------------------------------------------------------------------------
; LAPIC - Registers
;-------------------------------------------------------------------------------

%define LAPIC_ID_OFFSET			0x020
%define LAPIC_VERSION_OFFSET	0x030
%define	LAPIC_EOI_OFFSET		0x0B0
%define LAPIC_SVR_OFFSET		0x0F0
%define LAPIC_ICR_LOW_OFFSET	0x300
%define LAPIC_ICR_HIGH_OFFSET	0x310
%define LAPIC_LVT_TIMER_OFFSET	0x320
%define LAPIC_TIMER_INIT_OFFSET	0x380
%define LAPIC_TIMER_CUR_OFFSET	0x390
%define LAPIC_TIMER_DIV_OFFSET	0x3E0

;-------------------------------------------------------------------------------
; LAPIC - ICR
;-------------------------------------------------------------------------------

%define LAPIC_DELIVERY_FIXED		000b
%define LAPIC_DELIVERY_LOW_PRIO		001b
%define LAPIC_DELIVERY_SMI			010b
%define LAPIC_DELIVERY_NMI			100b
%define LAPIC_DELIVERY_INIT			101b
%define LAPIC_DELIVERY_STARTUP   	110b

%define LAPIC_SHORT_NONE		00b
%define LAPIC_SHORT_SELF		01b
%define LAPIC_SHORT_ALL_INCL    10b
%define LAPIC_SHORT_ALL_EXCL	11b

%define LAPIC_LEVEL_DEASSERT	0
%define LAPIC_LEVEL_ASSERT		1

%define LAPIC_TRIGGER_EDGE		0
%define LAPIC_TRIGGER_LEVEL		1

%define LAPIC_MODE_PHYSICAL		0
%define LAPIC_MODE_LOGICAL		1

;-------------------------------------------------------------------------------
; LAPIC - Timer
;-------------------------------------------------------------------------------

%define LAPIC_TIMER_VECTOR		0x40

%define LAPIC_TIMER_MODE_OFFSET 	17
%define LAPIC_TIMER_MODE_PERIODIC	1
%define LAPIC_TIMER_MODE_ONESHOT	0
