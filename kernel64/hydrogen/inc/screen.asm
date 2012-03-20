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
; Screen - Colors and Dimensions
;-------------------------------------------------------------------------------

%define SCREEN_BCOLOR	0		; Black
%define SCREEN_FCOLOR	15		; White
%define SCREEN_ATTR		((SCREEN_BCOLOR << 12) | (SCREEN_FCOLOR & 0x0F) << 8)

%define SCREEN_WIDTH	80
%define SCREEN_HEIGHT	25
%define SCREEN_SIZE		(SCREEN_WIDTH * SCREEN_HEIGHT)

;-------------------------------------------------------------------------------
; Screen - I/O
;-------------------------------------------------------------------------------

%define IO_VIDEO_COMMAND	0x3D4
%define IO_VIDEO_DATA		0x3D5

%define IO_VIDEO_HIGH_CUR	14
%define IO_VIDEO_LOW_CUR	15

;-------------------------------------------------------------------------------
; Screen - Special characters
;-------------------------------------------------------------------------------

%define CHAR_NL				0xA
%define CHAR_CR				0xD
