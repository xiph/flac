; libFLAC - Free Lossless Audio Codec library
; Copyright (C) 2001  Josh Coalson
;
; This library is free software; you can redistribute it and/or
; modify it under the terms of the GNU Library General Public
; License as published by the Free Software Foundation; either
; version 2 of the License, or (at your option) any later version.
;
; This library is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; Library General Public License for more details.
;
; You should have received a copy of the GNU Library General Public
; License along with this library; if not, write to the
; Free Software Foundation, Inc., 59 Temple Place - Suite 330,
; Boston, MA  02111-1307, USA.

	bits 32

%ifdef WIN32
	%define FLAC__PUBLIC_NEEDS_UNDERSCORE
	%idefine code_section section .text align=32 class=CODE use32
	%idefine data_section section .data align=32 class=DATA use32
	%idefine bss_section  section .bss  align=32 class=DATA use32
%elifdef AOUT
	%define FLAC__PUBLIC_NEEDS_UNDERSCORE
	%idefine code_section section .text
	%idefine data_section section .data
	%idefine bss_section  section .bss
%elifdef ELF
	%idefine code_section section .text class=CODE use32
	%idefine data_section section .data class=DATA use32
	%idefine bss_section  section .bss  class=DATA use32
%else
	%error unsupported object format!
%endif

%imacro cglobal 1
	%ifdef FLAC__PUBLIC_NEEDS_UNDERSCORE
		global _%1
	%else
		global %1
	%endif
%endmacro

%imacro cextern 1
	%ifdef FLAC__PUBLIC_NEEDS_UNDERSCORE
		extern _%1
	%else
		extern %1
	%endif
%endmacro
