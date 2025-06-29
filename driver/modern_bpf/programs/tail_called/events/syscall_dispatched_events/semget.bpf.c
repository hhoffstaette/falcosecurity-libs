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
int BPF_PROG(semget_e, struct pt_regs *regs, long id) {
	struct ringbuf_struct ringbuf;
	if(!ringbuf__reserve_space(&ringbuf, SEMGET_E_SIZE, PPME_SYSCALL_SEMGET_E)) {
		return 0;
	}

	ringbuf__store_event_header(&ringbuf);

	/*=============================== COLLECT PARAMETERS  ===========================*/

	/* Parameter 1: key (type: PT_INT32) */
	int32_t key = (int32_t)extract__syscall_argument(regs, 0);
	ringbuf__store_s32(&ringbuf, key);

	/* Parameter 2: nsems (type: PT_INT32) */
	int32_t nsems = (int32_t)extract__syscall_argument(regs, 1);
	ringbuf__store_s32(&ringbuf, nsems);

	/* Parameter 3: semflg (type: PT_FLAGS32) */
	uint32_t semflg = (uint32_t)extract__syscall_argument(regs, 2);
	ringbuf__store_u32(&ringbuf, semget_flags_to_scap(semflg));

	/*=============================== COLLECT PARAMETERS  ===========================*/

	ringbuf__submit_event(&ringbuf);

	return 0;
}

SEC("tp_btf/sys_exit")
int BPF_PROG(semget_x, struct pt_regs *regs, long ret) {
	struct ringbuf_struct ringbuf;
	if(!ringbuf__reserve_space(&ringbuf, SEMGET_X_SIZE, PPME_SYSCALL_SEMGET_X)) {
		return 0;
	}

	ringbuf__store_event_header(&ringbuf);

	/*=============================== COLLECT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	ringbuf__store_s64(&ringbuf, (int64_t)ret);

	/* Parameter 2: key (type: PT_INT32) */
	int32_t key = (int32_t)extract__syscall_argument(regs, 0);
	ringbuf__store_s32(&ringbuf, key);

	/* Parameter 3: nsems (type: PT_INT32) */
	int32_t nsems = (int32_t)extract__syscall_argument(regs, 1);
	ringbuf__store_s32(&ringbuf, nsems);

	/* Parameter 4: semflg (type: PT_FLAGS32) */
	uint32_t semflg = (uint32_t)extract__syscall_argument(regs, 2);
	ringbuf__store_u32(&ringbuf, semget_flags_to_scap(semflg));

	/*=============================== COLLECT PARAMETERS  ===========================*/

	ringbuf__submit_event(&ringbuf);

	return 0;
}
