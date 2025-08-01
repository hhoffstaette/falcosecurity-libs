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
int BPF_PROG(dup3_e, struct pt_regs *regs, long id) {
	struct ringbuf_struct ringbuf;
	if(!ringbuf__reserve_space(&ringbuf, DUP3_E_SIZE, PPME_SYSCALL_DUP3_E)) {
		return 0;
	}

	ringbuf__store_event_header(&ringbuf);

	/*=============================== COLLECT PARAMETERS  ===========================*/

	/* Parameter 1: fd (type: PT_FD) */
	int64_t oldfd = (int64_t)(int32_t)extract__syscall_argument(regs, 0);
	ringbuf__store_s64(&ringbuf, oldfd);

	/*=============================== COLLECT PARAMETERS  ===========================*/

	ringbuf__submit_event(&ringbuf);

	return 0;
}

/*=============================== ENTER EVENT ===========================*/

/*=============================== EXIT EVENT ===========================*/

SEC("tp_btf/sys_exit")
int BPF_PROG(dup3_x, struct pt_regs *regs, long ret) {
	struct ringbuf_struct ringbuf;
	if(!ringbuf__reserve_space(&ringbuf, DUP3_X_SIZE, PPME_SYSCALL_DUP3_X)) {
		return 0;
	}

	ringbuf__store_event_header(&ringbuf);

	/*=============================== COLLECT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_FD) */
	ringbuf__store_s64(&ringbuf, (int64_t)(int32_t)ret);

	/* Parameter 2: oldfd (type: PT_FD) */
	int64_t oldfd = (int64_t)(int32_t)extract__syscall_argument(regs, 0);
	ringbuf__store_s64(&ringbuf, oldfd);

	/* Parameter 3: newfd (type: PT_FD) */
	int64_t newfd = (int64_t)(int32_t)extract__syscall_argument(regs, 1);
	ringbuf__store_s64(&ringbuf, newfd);

	/* Parameter 4: flags (type: PT_FLAGS32) */
	int32_t flags = extract__syscall_argument(regs, 2);
	ringbuf__store_u32(&ringbuf, dup3_flags_to_scap(flags));

	/*=============================== COLLECT PARAMETERS  ===========================*/

	ringbuf__submit_event(&ringbuf);

	return 0;
}

/*=============================== EXIT EVENT ===========================*/
