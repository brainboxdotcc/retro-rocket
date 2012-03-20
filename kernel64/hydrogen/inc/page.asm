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
; Paging - Flags
;-------------------------------------------------------------------------------

%define PAGE_FLAG_PRESENT			(1 << 0)
%define PAGE_FLAG_WRITABLE			(1 << 1)
%define PAGE_FLAG_USER				(1 << 2)
%define PAGE_FLAG_GLOBAL			(1 << 6)
%define PAGE_FLAG_PS				(1 << 7)

%define PAGE_FLAG_PW				PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE

%define PAGE_PHYSICAL_MASK			~0x1FF
