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
; PIC - IO Constants
;-------------------------------------------------------------------------------

; see http://http://wiki.osdev.org/PIC

%define IO_PIC1                     0x20
%define IO_PIC2                     0xA0
%define IO_PIC1_COMMAND             IO_PIC1
%define IO_PIC1_DATA                (IO_PIC1 + 1)
%define IO_PIC2_COMMAND             IO_PIC2
%define IO_PIC2_DATA                (IO_PIC2 + 1)

%define PIC_ICW1_ICW4	            0x01		; ICW4 (not) needed
%define PIC_ICW1_SINGLE	            0x02		; Single (cascade) mode
%define PIC_ICW1_INTERVAL4          0x04		; Call address interval 4 (8)
%define PIC_ICW1_LEVEL              0x08		; Level triggered (edge) mode
%define PIC_ICW1_INIT               0x10		; Initialization - required!

%define PIC_ICW4_8086               0x01		; 8086/88 (MCS-80/85) mode
%define PIC_ICW4_AUTO               0x02		; Auto (normal) EOI
%define PIC_ICW4_BUF_SLAVE          0x08		; Buffered mode/slave
%define PIC_ICW4_BUF_MASTER         0x0C		; Buffered mode/master
%define PIC_ICW4_SFNM               0x10		; Special fully nested (not)
