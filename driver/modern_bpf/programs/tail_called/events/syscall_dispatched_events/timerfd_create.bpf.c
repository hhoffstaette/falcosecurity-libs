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
int BPF_PROG(timerfd_create_e, struct pt_regs *regs, long id) {
	struct ringbuf_struct ringbuf;
	if(!ringbuf__reserve_space(&ringbuf, TIMERFD_CREATE_E_SIZE, PPME_SYSCALL_TIMERFD_CREATE_E)) {
		return 0;
	}

	ringbuf__store_event_header(&ringbuf);

	/*=============================== COLLECT PARAMETERS  ===========================*/

	/* Parameter 1: clockid (type: PT_UINT8) */
	/* Like in the old probe we send `0` */
	ringbuf__store_u8(&ringbuf, 0);

	/* Parameter 2: flags (type: PT_UINT8) */
	/* Like in the old probe we send `0` */
	ringbuf__store_u8(&ringbuf, 0);

	/*=============================== COLLECT PARAMETERS  ===========================*/

	ringbuf__submit_event(&ringbuf);

	return 0;
}

/*=============================== ENTER EVENT ===========================*/

/*=============================== EXIT EVENT ===========================*/

SEC("tp_btf/sys_exit")
int BPF_PROG(timerfd_create_x, struct pt_regs *regs, long ret) {
	struct ringbuf_struct ringbuf;
	if(!ringbuf__reserve_space(&ringbuf, TIMERFD_CREATE_X_SIZE, PPME_SYSCALL_TIMERFD_CREATE_X)) {
		return 0;
	}

	ringbuf__store_event_header(&ringbuf);

	/*=============================== COLLECT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_FD) */
	ringbuf__store_s64(&ringbuf, ret);

	/* Parameter 2: clockid (type: PT_UINT8) */
	/* Like in the old probe we send `0` */
	ringbuf__store_u8(&ringbuf, 0);

	/* Parameter 3: flags (type: PT_UINT8) */
	/* Like in the old probe we send `0` */
	ringbuf__store_u8(&ringbuf, 0);

	/*=============================== COLLECT PARAMETERS  ===========================*/

	ringbuf__submit_event(&ringbuf);

	return 0;
}

/*=============================== EXIT EVENT ===========================*/
