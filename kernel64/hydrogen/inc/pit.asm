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
; PIT
;-------------------------------------------------------------------------------

%define PIT_FREQ				100  		; 10 ms, in Hz
%define PIT_FREQ_BASE			1193180		; in Hz

%define PIT_IO_COMMAND			0x43
%define PIT_IO_COMMAND_DIVIDER	0x36
%define PIT_IO_DATA				0x40
