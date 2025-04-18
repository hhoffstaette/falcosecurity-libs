// SPDX-License-Identifier: GPL-2.0-only OR MIT
/*
 * Copyright (C) 2023 The Falco Authors.
 *
 * This file is dual licensed under either the MIT or GPL 2. See MIT.txt
 * or GPL2.txt for full copies of the license.
 */

#include <helpers/interfaces/fixed_size_event.h>

/*=============================== ENTER EVENT ===========================*/

SEC("tp_btf/sys_enter")
int BPF_PROG(setrlimit_e, struct pt_regs *regs, long id) {
	struct ringbuf_struct ringbuf;
	if(!ringbuf__reserve_space(&ringbuf, SETRLIMIT_E_SIZE, PPME_SYSCALL_SETRLIMIT_E)) {
		return 0;
	}

	ringbuf__store_event_header(&ringbuf);

	/*=============================== COLLECT PARAMETERS  ===========================*/

	/* Parameter 1: resource (type: PT_ENUMFLAGS8) */
	uint32_t resource = extract__syscall_argument(regs, 0);
	ringbuf__store_u8(&ringbuf, rlimit_resource_to_scap(resource));

	/*=============================== COLLECT PARAMETERS  ===========================*/

	ringbuf__submit_event(&ringbuf);

	return 0;
}

/*=============================== ENTER EVENT ===========================*/

/*=============================== EXIT EVENT ===========================*/

SEC("tp_btf/sys_exit")
int BPF_PROG(setrlimit_x, struct pt_regs *regs, long ret) {
	struct ringbuf_struct ringbuf;
	if(!ringbuf__reserve_space(&ringbuf, SETRLIMIT_X_SIZE, PPME_SYSCALL_SETRLIMIT_X)) {
		return 0;
	}

	ringbuf__store_event_header(&ringbuf);

	/*=============================== COLLECT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO)*/
	ringbuf__store_s64(&ringbuf, ret);

	struct rlimit rl = {0};
	unsigned long rlimit_pointer = extract__syscall_argument(regs, 1);
	bpf_probe_read_user((void *)&rl, bpf_core_type_size(struct rlimit), (void *)rlimit_pointer);

	/* Parameter 2: cur (type: PT_INT64)*/
	ringbuf__store_s64(&ringbuf, rl.rlim_cur);

	/* Parameter 3: max (type: PT_INT64)*/
	ringbuf__store_s64(&ringbuf, rl.rlim_max);

	/* Parameter 4: resource (type: PT_ENUMFLAGS8) */
	uint32_t resource = extract__syscall_argument(regs, 0);
	ringbuf__store_u8(&ringbuf, rlimit_resource_to_scap(resource));

	/*=============================== COLLECT PARAMETERS  ===========================*/

	ringbuf__submit_event(&ringbuf);

	return 0;
}

/*=============================== EXIT EVENT ===========================*/
