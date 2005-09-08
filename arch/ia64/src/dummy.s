#
# Copyright (C) 2005 Jakub Jermar
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

.text

.global calibrate_delay_loop
.global asm_delay_loop
.global userspace
.global before_thread_runs_arch
.global arch_late_init
.global cpu_identify
.global cpu_print_report
.global cpu_priority_high
.global cpu_priority_low
.global cpu_priority_read
.global cpu_priority_restore
.global cpu_sleep
.global frame_arch_init
.global dummy
.global fpu_enable
.global fpu_disable
.global fpu_init

before_thread_runs_arch:
userspace:
calibrate_delay_loop:
asm_delay_loop:
arch_late_init:
cpu_identify:
cpu_print_report:
cpu_priority_high:
cpu_priority_low:
cpu_priority_read:
cpu_priority_restore:
cpu_sleep:
frame_arch_init:
fpu_init:
fpu_enable:
fpu_disable:	

dummy:
	br.ret.sptk.many b0

