#include "../../event_class/event_class.h"

#ifdef __NR_umount2

#include <sys/mount.h>

TEST(SyscallExit, umount2X) {
	auto evt_test = get_syscall_event_test(__NR_umount2, EXIT_EVENT);

	evt_test->enable_capture();

	/*=============================== TRIGGER SYSCALL ===========================*/

	const char* target = "//**null-file-path**//";
	unsigned long flags = MNT_FORCE;
	assert_syscall_state(SYSCALL_FAILURE, "umount2", syscall(__NR_umount2, target, flags));
	int64_t errno_value = -errno;

	/*=============================== TRIGGER SYSCALL ===========================*/

	evt_test->disable_capture();

	evt_test->assert_event_presence();

	if(HasFatalFailure()) {
		return;
	}

	evt_test->parse_event();

	evt_test->assert_header();

	/*=============================== ASSERT PARAMETERS  ===========================*/

	/* Parameter 1: res (type: PT_ERRNO) */
	evt_test->assert_numeric_param(1, (int64_t)errno_value);

	/* Parameter 2: name (type: PT_FSPATH) */
	evt_test->assert_charbuf_param(2, target);

	/* Parameter 3: flags (type: PT_FLAGS32) */
	evt_test->assert_numeric_param(3, (uint32_t)(PPM_MNT_FORCE));

	/*=============================== ASSERT PARAMETERS  ===========================*/

	evt_test->assert_num_params_pushed(3);
}
#endif
