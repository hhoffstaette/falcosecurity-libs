// SPDX-License-Identifier: Apache-2.0

/*
Copyright (C) 2024 The Falco Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#include <test_utils.h>
#include <scap_files/scap_file_test.h>

// Use `sudo sysdig -r <scap-file> -S -q` to check the number of events in the scap file.
// When you find a specific event to assert use
// `sudo sysdig -r <> -d "evt.num=<>" -p "ts=%evt.rawtime, tid=%thread.tid, args=%evt.args"`

TEST_F(scap_file_test, same_number_of_events) {
	open_filename("scap_2013.scap");
	assert_num_event_types({
	        {PPME_SYSCALL_READ_E, 24956},
	        {PPME_SYSCALL_READ_X, 24957},
	        {PPME_SOCKET_ACCEPT_E, 3817},
	        {PPME_SOCKET_ACCEPT_5_X, 3816},
	        // Add further checks regarding the expected number of events in this scap file here.
	});

	open_filename("kexec_arm64.scap");
	assert_num_event_types({
	        {PPME_SYSCALL_PREAD_E, 3216},      {PPME_SYSCALL_PREAD_X, 3216},
	        {PPME_SOCKET_LISTEN_E, 1},         {PPME_SOCKET_LISTEN_X, 1},
	        {PPME_SYSCALL_SETUID_E, 2},        {PPME_SYSCALL_SETUID_X, 2},
	        {PPME_SOCKET_RECVFROM_E, 82},      {PPME_SOCKET_RECVFROM_X, 82},
	        {PPME_SOCKET_SENDTO_E, 162},       {PPME_SOCKET_SENDTO_X, 162},
	        {PPME_SOCKET_SHUTDOWN_E, 9},       {PPME_SOCKET_SHUTDOWN_X, 9},
	        {PPME_SOCKET_SOCKETPAIR_E, 114},   {PPME_SOCKET_SOCKETPAIR_X, 114},
	        {PPME_SOCKET_SENDMSG_E, 26},       {PPME_SOCKET_SENDMSG_X, 26},
	        {PPME_SOCKET_RECVMSG_E, 522},      {PPME_SOCKET_RECVMSG_X, 522},
	        {PPME_SYSCALL_IOCTL_3_E, 1164},    {PPME_SYSCALL_IOCTL_3_X, 1164},
	        {PPME_SYSCALL_FSTAT_E, 1962},      {PPME_SYSCALL_FSTAT_X, 1962},
	        {PPME_SYSCALL_BRK_4_E, 659},       {PPME_SYSCALL_BRK_4_X, 659},
	        {PPME_SYSCALL_GETRLIMIT_E, 2},     {PPME_SYSCALL_GETRLIMIT_X, 2},
	        {PPME_SYSCALL_CLOSE_E, 19860},     {PPME_SYSCALL_CLOSE_X, 19860},
	        {PPME_SYSCALL_MUNMAP_E, 2965},     {PPME_SYSCALL_MUNMAP_X, 2965},
	        {PPME_SYSCALL_GETDENTS64_E, 1870}, {PPME_SYSCALL_GETDENTS64_X, 1870},
	        {PPME_SYSCALL_PPOLL_E, 1093},      {PPME_SYSCALL_PPOLL_X, 1093},
	        {PPME_SYSCALL_UNSHARE_E, 1},       {PPME_SYSCALL_UNSHARE_X, 1},
	        {PPME_SYSCALL_UNSHARE_E, 1},       {PPME_SYSCALL_UNSHARE_X, 1},
	        {PPME_SYSCALL_SECCOMP_E, 18},      {PPME_SYSCALL_SECCOMP_X, 18},
	        {PPME_SYSCALL_EPOLL_CREATE1_E, 5}, {PPME_SYSCALL_EPOLL_CREATE1_X, 5},
	        {PPME_SYSCALL_KILL_E, 156},        {PPME_SYSCALL_KILL_X, 156},
	        {PPME_SYSCALL_TGKILL_E, 1010},     {PPME_SYSCALL_TGKILL_X, 1010},
	        {PPME_SOCKET_ACCEPT4_6_E, 207},    {PPME_SOCKET_ACCEPT4_6_X, 207},
	        {PPME_SYSCALL_SPLICE_E, 253},      {PPME_SYSCALL_SPLICE_X, 253},
	        {PPME_SYSCALL_LSEEK_E, 329},       {PPME_SYSCALL_LSEEK_X, 329},
	        {PPME_SYSCALL_WRITEV_E, 5},        {PPME_SYSCALL_WRITEV_X, 5},
	        {PPME_SYSCALL_FCNTL_E, 9817},      {PPME_SYSCALL_FCNTL_X, 9817},
	        {PPME_SOCKET_CONNECT_E, 238},      {PPME_SOCKET_CONNECT_X, 238},
	        {PPME_SYSCALL_EXECVE_19_E, 202},   {PPME_SYSCALL_EXECVE_19_X, 203},
	        {PPME_SYSCALL_EXECVEAT_E, 1},      {PPME_SYSCALL_EXECVEAT_X, 0},
	        // Add further checks regarding the expected number of events in this scap file here.
	});

	open_filename("kexec_x86.scap");
	assert_num_event_types({
	        {PPME_SYSCALL_EPOLLWAIT_E, 2051}, {PPME_SYSCALL_EPOLLWAIT_X, 2051},
	        {PPME_SYSCALL_POLL_E, 2682},      {PPME_SYSCALL_POLL_X, 2683},
	        {PPME_SYSCALL_SETNS_E, 5},        {PPME_SYSCALL_SETNS_X, 5},
	        {PPME_SYSCALL_SETPGID_E, 4},      {PPME_SYSCALL_SETPGID_X, 4},
	        {PPME_SYSCALL_SETGID_E, 3},       {PPME_SYSCALL_SETGID_X, 3},
	        {PPME_SYSCALL_SETRLIMIT_E, 1},    {PPME_SYSCALL_SETRLIMIT_X, 1},
	        {PPME_SYSCALL_MMAP_E, 2123},      {PPME_SYSCALL_MMAP_X, 2123},
	        {PPME_SYSCALL_SETRESGID_E, 10},   {PPME_SYSCALL_SETRESGID_X, 10},
	        {PPME_SYSCALL_SETRESUID_E, 17},   {PPME_SYSCALL_SETRESUID_X, 17},
	        {PPME_SYSCALL_MOUNT_E, 2},        {PPME_SYSCALL_MOUNT_X, 2},
	        {PPME_SYSCALL_ACCESS_E, 350},     {PPME_SYSCALL_ACCESS_X, 350},
	        {PPME_SYSCALL_MPROTECT_E, 584},   {PPME_SYSCALL_MPROTECT_X, 584},
	        {PPME_SYSCALL_UMOUNT2_E, 2},      {PPME_SYSCALL_UMOUNT2_X, 2},
	        {PPME_SYSCALL_INOTIFY_INIT_E, 1}, {PPME_SYSCALL_INOTIFY_INIT_X, 1},
	        {PPME_SYSCALL_PRLIMIT_E, 173},    {PPME_SYSCALL_PRLIMIT_X, 173},
	        // Add further checks regarding the expected number of events in this scap file here.
	});

	open_filename("ptrace.scap");
	assert_num_event_types({
	        {PPME_SYSCALL_PTRACE_E, 3},
	        {PPME_SYSCALL_PTRACE_X, 3},
	        // Add further checks regarding the expected number of events in this scap file here.
	});

	open_filename("mkdir.scap");
	assert_num_event_types({
	        {PPME_SYSCALL_MKDIR_2_E, 1},
	        {PPME_SYSCALL_MKDIR_2_X, 1},
	        // Add further checks regarding the expected number of events in this scap file here.
	});

	open_filename("sample.scap");
	assert_num_event_types({
	        {PPME_SYSCALL_FUTEX_E, 15},
	        {PPME_SYSCALL_FUTEX_X, 15},
	        {PPME_SYSCALL_NANOSLEEP_E, 38},
	        {PPME_SYSCALL_NANOSLEEP_X, 38},
	        // Add further checks regarding the expected number of events in this scap file here.
	});

	open_filename("fchdir.scap");
	assert_num_event_types({
	        {PPME_SYSCALL_FCHDIR_E, 1},
	        {PPME_SYSCALL_FCHDIR_X, 1},
	        // Add further checks regarding the expected number of events in this scap file here.
	});
	// Add further checks for the expected number of events in unhandled scap files here.
}

////////////////////////////
// CLOSE
////////////////////////////

TEST_F(scap_file_test, close_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `907955` is the following:
	// - type=PPME_SYSCALL_CLOSE_X
	// - ts=1687966734290924121
	// - tid=115186
	// - args=res=0
	//
	// And its corresponding enter event `907954` is the following:
	// - type=PPME_SYSCALL_CLOSE_E
	// - ts=1687966734290922537
	// - tid=115186
	// - args=fd=13(<6>)
	//
	// Let's see the new PPME_SYSCALL_CLOSE_X event!
	constexpr uint64_t ts = 1687966734290924121;
	constexpr int64_t tid = 115186;
	constexpr int64_t res = 0;
	constexpr int64_t fd = 13;
	assert_event_presence(create_safe_scap_event(ts, tid, PPME_SYSCALL_CLOSE_X, 2, res, fd));
}

////////////////////////////
// READ
////////////////////////////

TEST_F(scap_file_test, read_x_check_final_converted_event) {
	open_filename("scap_2013.scap");

	// Inside the scap-file the event `430682` is the following:
	// - type=PPME_SYSCALL_READ_X
	// - ts=1380933088076148247
	// - tid=44106
	// - args=res=270 data=HTTP/1.1 302 Found\0Date: Sat, 05 Oct 2013 00:31:28 GMT\0Server:
	// Apache/2.4.4 (U
	//
	// And its corresponding enter event `430681` is the following:
	// - type=PPME_SYSCALL_READ_E
	// - ts=1380933088076145348
	// - tid=44106,
	// - args=fd=33(<4t>127.0.0.1:38308->127.0.0.1:80) size=8192
	//
	// Let's see the new PPME_SYSCALL_READ_X event!
	constexpr uint64_t ts = 1380933088076148247;
	constexpr int64_t tid = 44106;
	constexpr int64_t res = 270;
	// this is NULL termiinated so we have 81 bytes but in the scap-file we want only 80 bytes
	// without the NULL terminator
	constexpr char read_buf[] = {
	        "HTTP/1.1 302 Found\r\nDate: Sat, 05 Oct 2013 00:31:28 GMT\r\nServer: Apache/2.4.4 "
	        "(U"};
	constexpr int64_t fd = 33;
	constexpr uint32_t size = 8192;
	assert_event_presence(
	        create_safe_scap_event(ts,
	                               tid,
	                               PPME_SYSCALL_READ_X,
	                               4,
	                               res,
	                               scap_const_sized_buffer{read_buf, sizeof(read_buf) - 1},
	                               fd,
	                               size));
}

////////////////////////////
// PREAD
////////////////////////////

TEST_F(scap_file_test, pread_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `862450` is the following:
	// - type=PPME_SYSCALL_PREAD_X,
	// - ts=1687966733234634809
	// - tid=552
	// - args=res=400
	// data=...._...tty1............................tty1LOGIN...............................
	//
	// And its corresponding enter event `862449` is the following:
	// - type=PPME_SYSCALL_PREAD_E
	// - ts=1687966733234634235
	// - tid=552
	// - args=fd=19(<f>/var/run/utmp) size=400 pos=800
	//
	// Let's see the new PPME_SYSCALL_PREAD_X event!
	constexpr uint64_t ts = 1687966733234634809;
	constexpr int64_t tid = 552;
	constexpr int64_t res = 400;
	constexpr uint8_t read_buf[] = {
	        6,   0,   0,   0,   '_', 2,   0,   0,   't', 't', 'y', '1', 0, 0, 0, 0, 0, 0, 0, 0,
	        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0,
	        't', 't', 'y', '1', 'L', 'O', 'G', 'I', 'N', 0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0,
	        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0};
	constexpr int64_t fd = 19;
	constexpr uint32_t size = 400;
	constexpr int64_t pos = 800;
	assert_event_presence(
	        create_safe_scap_event(ts,
	                               tid,
	                               PPME_SYSCALL_PREAD_X,
	                               5,
	                               res,
	                               scap_const_sized_buffer{read_buf, sizeof(read_buf)},
	                               fd,
	                               size,
	                               pos));
}

////////////////////////////
// SIGNALFD
////////////////////////////

// We don't have scap-files with SIGNALFD events. Add it if we face a failure.

////////////////////////////
// KILL
////////////////////////////

TEST_F(scap_file_test, kill_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `900011` is the following:
	// - type=PPME_SYSCALL_KILL_X,
	// - ts=1687966733867745508
	// - tid=132533
	// - args=res=0
	//
	// And its corresponding enter event `900010` is the following:
	// - type=PPME_SYSCALL_KILL_E
	// - ts=1687966733867740060
	// - tid=132533
	// - args=pid=121080(cpptools) sig=0
	//
	// Let's see the new PPME_SYSCALL_KILL_X event!
	constexpr uint64_t ts = 1687966733867745508;
	constexpr int64_t tid = 132533;
	constexpr int64_t res = 0;
	constexpr int64_t pid = 121080;
	constexpr uint8_t sig = 0;

	assert_event_presence(create_safe_scap_event(ts, tid, PPME_SYSCALL_KILL_X, 3, res, pid, sig));
}

////////////////////////////
// TKILL
////////////////////////////

// We don't have scap-files with TKILL events. Add it if we face a failure.

////////////////////////////
// TGKILL
////////////////////////////

TEST_F(scap_file_test, tgkill_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `907496` is the following:
	// - type=PPME_SYSCALL_TGKILL_X,
	// - ts=1687966734248771138
	// - tid=114087
	// - args=res=0
	//
	// And its corresponding enter event `907489` is the following:
	// - type=PPME_SYSCALL_TGKILL_E
	// - ts=1687966734248751978
	// - tid=114087
	// - args=pid=672 tid=678 sig=23(SIGURG)
	//
	// Let's see the new PPME_SYSCALL_TGKILL_X event!
	constexpr uint64_t ts = 1687966734248771138;
	constexpr int64_t tid = 114087;
	constexpr int64_t res = 0;
	constexpr int64_t pid = 672;
	constexpr int64_t tid_param = 678;
	constexpr uint8_t sig = 23;

	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_TGKILL_X, 4, res, pid, tid_param, sig));
}

////////////////////////////
// NANOSLEEP
////////////////////////////

TEST_F(scap_file_test, nanosleep_x_check_final_converted_event) {
	open_filename("sample.scap");

	// Inside the scap-file the event `559` is the following:
	// - type=PPME_SYSCALL_NANOSLEEP_X,
	// - ts=1690557263532873130
	// - tid=762
	// - args=res=0
	//
	// And its corresponding enter event `543` is the following:
	// - type=PPME_SYSCALL_NANOSLEEP_E
	// - ts=1690557263532781075
	// - tid=762
	// - args=interval=0(0s)
	//
	// Let's see the new PPME_SYSCALL_NANOSLEEP_X event!
	constexpr uint64_t ts = 1690557263532873130;
	constexpr int64_t tid = 762;
	constexpr int64_t res = 0;
	constexpr uint64_t interval = 0;

	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_NANOSLEEP_X, 2, res, interval));
}

////////////////////////////
// TIMERFD_CREATE
////////////////////////////

// We don't have scap-files with TIMERFD_CREATE events. Add it if we face a failure.

////////////////////////////
// INOTIFY_INIT
////////////////////////////

TEST_F(scap_file_test, inotify_init_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `161340` is the following:
	// - type=PPME_SYSCALL_INOTIFY_INIT_X,
	// - ts=1687889193632611044
	// - tid=107370
	// - args=res=4(<i>)
	//
	// And its corresponding enter event `161339` is the following:
	// - type=PPME_SYSCALL_INOTIFY_INIT_E
	// - ts=1687889193632606569
	// - tid=107370
	// - args=flags=0
	//
	// Let's see the new PPME_SYSCALL_INOTIFY_INIT_X event!
	constexpr uint64_t ts = 1687889193632611044;
	constexpr int64_t tid = 107370;
	constexpr int64_t res = 4;
	constexpr uint8_t flags = 0;

	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_INOTIFY_INIT_X, 2, res, flags));
}

////////////////////////////
// GETRLIMIT
////////////////////////////

TEST_F(scap_file_test, getrlimit_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `64634` is the following:
	// - type=PPME_SYSCALL_GETRLIMIT_X,
	// - ts=1687966709997975281
	// - tid=141446
	// - args=res=0 cur=1048576 max=1048576
	//
	// And its corresponding enter event `64633` is the following:
	// - type=PPME_SYSCALL_GETRLIMIT_E
	// - ts=1687966709997974370
	// - tid=141446
	// - args=resource=7(RLIMIT_NOFILE)
	//
	// Let's see the new PPME_SYSCALL_GETRLIMIT_X event!
	constexpr uint64_t ts = 1687966709997975281;
	constexpr int64_t tid = 141446;
	constexpr int64_t res = 0;
	constexpr int64_t cur = 1048576;
	constexpr int64_t max = 1048576;
	constexpr uint8_t resource = 7;

	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_GETRLIMIT_X, 4, res, cur, max, resource));
}

////////////////////////////
// SETRLIMIT
////////////////////////////

TEST_F(scap_file_test, setrlimit_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `297081` is the following:
	// - type=PPME_SYSCALL_SETRLIMIT_X,
	// - ts=1687889196237251155
	// - tid=107391
	// - args=res=0 cur=1048576 max=1048576
	//
	// And its corresponding enter event `297080` is the following:
	// - type=PPME_SYSCALL_SETRLIMIT_E
	// - ts=1687889196237250150
	// - tid=107391
	// - args=resource=7(RLIMIT_NOFILE)
	//
	// Let's see the new PPME_SYSCALL_SETRLIMIT_X event!
	constexpr uint64_t ts = 1687889196237251155;
	constexpr int64_t tid = 107391;
	constexpr int64_t res = 0;
	constexpr int64_t cur = 1048576;
	constexpr int64_t max = 1048576;
	constexpr uint8_t resource = 7;

	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_SETRLIMIT_X, 4, res, cur, max, resource));
}

////////////////////////////
// PRLIMIT
////////////////////////////

TEST_F(scap_file_test, prlimit_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `513076` is the following:
	// - type=PPME_SYSCALL_PRLIMIT_X,
	// - ts=1687889198695794034
	// - tid=107452
	// - args=res=0 newcur=-1 newmax=-1 oldcur=8388608 oldmax=-1
	//
	// And its corresponding enter event `513075` is the following:
	// - type=PPME_SYSCALL_PRLIMIT_E
	// - ts=1687889198695793141
	// - tid=107452
	// - args=pid=0 resource=3(RLIMIT_STACK)
	//
	// Let's see the new PPME_SYSCALL_PRLIMIT_X event!
	constexpr uint64_t ts = 1687889198695794034;
	constexpr int64_t tid = 107452;
	constexpr int64_t res = 0;
	constexpr int64_t newcur = -1;
	constexpr int64_t newmax = -1;
	constexpr int64_t oldcur = 8388608;
	constexpr int64_t oldmax = -1;
	constexpr int64_t pid = 0;
	constexpr uint8_t resource = 3;

	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SYSCALL_PRLIMIT_X,
	                                             7,
	                                             res,
	                                             newcur,
	                                             newmax,
	                                             oldcur,
	                                             oldmax,
	                                             pid,
	                                             resource));
}

////////////////////////////
// FCNTL
////////////////////////////

TEST_F(scap_file_test, fcntl_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `906671` is the following:
	// - type=PPME_SYSCALL_FCNTL_X,
	// - ts=1687966734198994052
	// - tid=114093
	// - args=res=0(<f>/dev/null)
	//
	// And its corresponding enter event `906670` is the following:
	// - type=PPME_SYSCALL_FCNTL_E
	// - ts=1687966734198993412
	// - tid=114093
	// - args=fd=19(<f>/sys/fs/cgroup/kubelet.slice/kubelet-kubepods.slice/kubelet-kubepods-
	// besteffort.slice/kubelet-kubepods-besteffort-pod03e86e4b_ac6e_4488_883e_e4b50b1be176.
	// slice/cgroup.procs)
	// cmd=5(F_SETFL)
	//
	// Let's see the new PPME_SYSCALL_FCNTL_X event!
	constexpr uint64_t ts = 1687966734198994052;
	constexpr int64_t tid = 114093;
	constexpr int64_t res = 0;
	constexpr int64_t fd = 19;
	constexpr uint8_t cmd = 5;

	assert_event_presence(create_safe_scap_event(ts, tid, PPME_SYSCALL_FCNTL_X, 3, res, fd, cmd));
}

////////////////////////////
///// BRK
////////////////////////////

TEST_F(scap_file_test, brk_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `897489` is the following:
	// - type=PPME_SYSCALL_BRK_4_X,
	// - ts=1687966733729257738
	// - tid=141707
	// - args=res=AAAB08F60000 vm_size=2208 vm_rss=788 vm_swap=0
	//
	// And its corresponding enter event `897487` is the following:
	// - type=PPME_SYSCALL_BRK_4_E
	// - ts=1687966733729256163
	// - tid=141707
	// - args=addr=AAAB08F60000
	//
	// Let's see the new PPME_SYSCALL_POLL_X event!

	constexpr uint64_t ts = 1687966733729257738;
	constexpr int64_t tid = 141707;
	constexpr int64_t res = 0xAAAB08F60000;
	constexpr uint32_t vm_size = 2208;
	constexpr uint32_t vm_rss = 788;
	constexpr uint32_t vm_swap = 0;
	constexpr uint64_t addr = 0xAAAB08F60000;

	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SYSCALL_BRK_4_X,
	                                             5,
	                                             res,
	                                             vm_size,
	                                             vm_rss,
	                                             vm_swap,
	                                             addr));
}

////////////////////////////
///// EXECVE
////////////////////////////

TEST_F(scap_file_test, execve_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `897489` is the following:
	// - type=PPME_SYSCALL_EXECVE_19_X,
	// - ts=1687966733728778931
	// - tid=141707
	// - args=res=0 exe=cat args=/proc/129520/stat. tid=141707(cat) pid=141707(cat)
	// ptid=141698(cpuUsage.sh) cwd=<NA> fdlimit=1048576 pgft_maj=0 pgft_min=56 vm_size=364 vm_rss=4
	// vm_swap=0 comm=cat
	// cgroups=cpuset=/user.slice.cpu=/user.slice.cpuacct=/.io=/user.slice.memory=/user.slic...
	// env=SHELL=/bin/zsh.VSCODE_VERBOSE_LOGGING=true.LC_ADDRESS=it_IT.UTF-8.LC_NAME=it_... tty=0
	// vpgid=118459 loginuid=1000(ubuntu) flags=0 cap_inheritable=0 cap_permitted=0 cap_effective=0
	// exe_ino=1522 exe_ino_ctime=2023-03-03 03:16:21.531741984 exe_ino_mtime=2022-02-07
	// 17:03:08.000000000 uid=1000(ubuntu)
	//
	// And its corresponding enter event `897487` is the following:
	// - type=PPME_SYSCALL_EXECVE_19_E
	// - ts=1687966733728593747
	// - tid=141707
	// - args=filename=/usr/bin/cat
	//
	// Let's see the new PPME_SYSCALL_EXECVE_19_X event!

	constexpr uint64_t ts = 1687966733728778931;
	constexpr int64_t tid_hdr = 141707;
	constexpr int64_t res = 0;
	constexpr char exe[] = "cat";
	constexpr char args_data[] = "/proc/129520/stat";
	const scap_const_sized_buffer args{args_data, sizeof(args_data)};
	constexpr int64_t tid = 141707;
	constexpr int64_t pid = 141707;
	constexpr int64_t ptid = 141698;
	constexpr auto cwd = empty_value<char *>();
	constexpr uint64_t fdlimit = 1048576;
	constexpr uint64_t pgft_maj = 0;
	constexpr uint64_t pgft_min = 56;
	constexpr uint32_t vm_size = 364;
	constexpr uint32_t vm_rss = 4;
	constexpr uint32_t vm_swap = 0;
	constexpr char comm[] = "cat";
	constexpr char cgroups_data[] =
	        "cpuset=/user.slice\0cpu=/user.slice\0cpuacct=/\0io=/user.slice\0memory=/user.slice/"
	        "user-1000.slice/session-21.scope\0devices=/\0freezer=/\0net_cls=/\0perf_event=/"
	        "user.slice/user-1000.slice/session-21.scope\0net_prio=/\0hugetlb=/user.slice\0pids=/"
	        "user.slice/user-1000.slice/session-21.scope\0rdma=/user.slice\0misc=/user.slice";
	const scap_const_sized_buffer cgroups{cgroups_data, sizeof(cgroups_data)};
	constexpr char env_data[] =
	        "SHELL=/bin/"
	        "zsh\0VSCODE_VERBOSE_LOGGING=true\0LC_ADDRESS=it_IT.UTF-8\0LC_NAME=it_IT.UTF-8\0VSCODE_"
	        "PIPE_LOGGING=true\0LC_MONETARY=it_IT.UTF-8\0VSCODE_AMD_ENTRYPOINT=vs/platform/"
	        "terminal/node/ptyHostMain\0PWD=/home/"
	        "ubuntu\0LOGNAME=ubuntu\0XDG_SESSION_TYPE=tty\0MOTD_SHOWN=pam\0HOME=/home/"
	        "ubuntu\0LC_PAPER=it_IT.UTF-8\0LANG=C.UTF-8\0VSCODE_AGENT_FOLDER=/home/ubuntu/"
	        ".vscode-server\0SSH_CONNECTION=151.38.160.202 36824 172.31.24.0 "
	        "22\0XDG_SESSION_CLASS=user\0VSCODE_HANDLES_SIGPIPE=true\0LC_IDENTIFICATION=it_IT.UTF-"
	        "8\0USER=ubuntu\0SHLVL=1\0LC_TELEPHONE=it_IT.UTF-8\0LC_MEASUREMENT=it_IT.UTF-8\0VSCODE_"
	        "CWD=/home/ubuntu\0XDG_SESSION_ID=21\0VSCODE_PARENT_PID=118516\0XDG_RUNTIME_DIR=/run/"
	        "user/1000\0SSH_CLIENT=151.38.160.202 36824 "
	        "22\0LC_TIME=it_IT.UTF-8\0PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/"
	        "sbin:/bin:/usr/games:/usr/local/games:/snap/bin\0DBUS_SESSION_BUS_ADDRESS=unix:path=/"
	        "run/user/1000/bus\0LC_NUMERIC=it_IT.UTF-8\0OLDPWD=/home/ubuntu\0_=/usr/bin/cat";
	const scap_const_sized_buffer env{env_data, sizeof(env_data)};
	constexpr uint32_t tty = 0;
	constexpr int64_t vpgid = 118459;
	constexpr uint32_t loginuid = 1000;
	constexpr uint32_t flags = 0;
	constexpr uint64_t cap_inheritable = 0;
	constexpr uint64_t cap_permitted = 0;
	constexpr uint64_t cap_effective = 0;
	constexpr uint64_t exe_ino = 1522;
	constexpr int64_t exe_ino_ctime = 1677809781531741984;
	constexpr int64_t exe_ino_mtime = 1644249788000000000;
	constexpr uint32_t uid = 1000;
	constexpr auto trusted_exepath = empty_value<char *>();
	constexpr auto pgid = empty_value<int64_t>();
	constexpr auto gid = empty_value<uint32_t>();

	SCAP_EMPTY_PARAMS_SET(empty_params_set, 6, 27, 28, 29);
	assert_event_presence(create_safe_scap_event_with_empty_params(ts,
	                                                               tid_hdr,
	                                                               PPME_SYSCALL_EXECVE_19_X,
	                                                               &empty_params_set,
	                                                               30,
	                                                               res,
	                                                               exe,
	                                                               args,
	                                                               tid,
	                                                               pid,
	                                                               ptid,
	                                                               cwd,
	                                                               fdlimit,
	                                                               pgft_maj,
	                                                               pgft_min,
	                                                               vm_size,
	                                                               vm_rss,
	                                                               vm_swap,
	                                                               comm,
	                                                               cgroups,
	                                                               env,
	                                                               tty,
	                                                               vpgid,
	                                                               loginuid,
	                                                               flags,
	                                                               cap_inheritable,
	                                                               cap_permitted,
	                                                               cap_effective,
	                                                               exe_ino,
	                                                               exe_ino_ctime,
	                                                               exe_ino_mtime,
	                                                               uid,
	                                                               trusted_exepath,
	                                                               pgid,
	                                                               gid));
}

////////////////////////////
// SOCKET
////////////////////////////

TEST_F(scap_file_test, socket_x_check_final_converted_event) {
	open_filename("scap_2013.scap");

	// Inside the scap-file the event `515881` is the following:
	// - type=PPME_SOCKET_SOCKET_X
	// - ts=1380933088295552884
	// - tid=44106,
	// - args=fd=19(<4>)
	//
	// And its corresponding enter event `511520` is the following:
	// - type=PPME_SOCKET_SOCKET_E
	// - ts=1380933088295478275
	// - tid=44106
	// - args=domain=2(AF_INET) type=524289 proto=0
	//
	constexpr uint64_t ts = 1380933088295552884;
	constexpr int64_t tid = 44106;
	constexpr int64_t fd = 19;
	constexpr uint32_t domain = 2;
	constexpr uint32_t type = 524289;
	constexpr uint32_t proto = 0;

	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SOCKET_SOCKET_X, 4, fd, domain, type, proto));
}

////////////////////////////
// CONNECT
////////////////////////////

// Compile out this test if test_utils helpers are not defined.
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
TEST_F(scap_file_test, connect_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `907953` is the following:
	// - type=PPME_SOCKET_CONNECT_X
	// - ts=1687966734290916318
	// - tid=115186
	// - args=res=-101(ENETUNREACH) tuple=:::47437->2001:4860:4860::8888:53 fd=13(<6>)
	//
	// And its corresponding enter event `907952` is the following:
	// - type=PPME_SOCKET_CONNECT_E
	// - ts=1687966734290903534
	// - tid=115186
	// - args=fd=13(<6>) addr=2001:4860:4860::8888:53
	//
	constexpr uint64_t ts = 1687966734290916318;
	constexpr int64_t tid = 115186;
	constexpr int64_t res = -101;
	constexpr int64_t fd = 13;
	sockaddr_in6 client_sockaddr = test_utils::fill_sockaddr_in6(47437, "::");
	sockaddr_in6 server_sockaddr = test_utils::fill_sockaddr_in6(53, "2001:4860:4860::8888");
	const std::vector<uint8_t> tuple =
	        test_utils::pack_socktuple(reinterpret_cast<struct sockaddr *>(&client_sockaddr),
	                                   reinterpret_cast<struct sockaddr *>(&server_sockaddr));
	const std::vector<uint8_t> packed_server_sockaddr =
	        test_utils::pack_sockaddr(reinterpret_cast<sockaddr *>(&server_sockaddr));

	assert_event_presence(create_safe_scap_event(
	        ts,
	        tid,
	        PPME_SOCKET_CONNECT_X,
	        4,
	        res,
	        scap_const_sized_buffer{tuple.data(), tuple.size()},
	        fd,
	        scap_const_sized_buffer{packed_server_sockaddr.data(), packed_server_sockaddr.size()}));
}
#endif

////////////////////////////
// LISTEN
////////////////////////////

TEST_F(scap_file_test, listen_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `57008` is the following:
	// - type=PPME_SOCKET_LISTEN_X,
	// - ts=1687966709944348874
	// - tid=141291
	// - args=res=0
	//
	// And its corresponding enter event `57007` is the following:
	// - type=PPME_SOCKET_LISTEN_E
	// - ts=1687966709944347577
	// - tid=141291
	// - args=fd=25(<u>/tmp/pty1908604488/pty.sock) backlog=4096
	//
	// Let's see the new PPME_SOCKET_LISTEN_X event!

	constexpr uint64_t ts = 1687966709944348874;
	constexpr int64_t tid = 141291;
	constexpr int64_t res = 0;
	constexpr int64_t fd = 25;
	constexpr int32_t backlog = 4096;
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SOCKET_LISTEN_X, 3, res, fd, backlog));
}

////////////////////////////
// ACCEPT
////////////////////////////

// Compile out this test if test_utils helpers are not defined.
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
TEST_F(scap_file_test, accept_x_check_final_converted_event) {
	open_filename("scap_2013.scap");

	// Inside the scap-file the event `519217` is the following:
	// - type=PPME_SOCKET_ACCEPT_X,
	// - ts=1380933088302022447
	// - tid=43625
	// - args=fd=13(<4t>127.0.0.1:38873->127.0.0.1:80) tuple=127.0.0.1:38873->127.0.0.1:80
	// queuepct=37 queuepct=37
	//
	// And its corresponding enter event `519211` is the following:
	// - type=PPME_SOCKET_ACCEPT_E
	// - ts=1380933088302013474
	// - tid=43625
	// - args=
	//
	// Let's see the new PPME_SOCKET_ACCEPT_5_X event!

	constexpr uint64_t ts = 1380933088302022447;
	constexpr int64_t tid = 43625;
	constexpr int64_t fd = 13;
	sockaddr_in client_sockaddr = test_utils::fill_sockaddr_in(38873, "127.0.0.1");
	sockaddr_in server_sockaddr = test_utils::fill_sockaddr_in(80, "127.0.0.1");
	const std::vector<uint8_t> tuple =
	        test_utils::pack_socktuple(reinterpret_cast<struct sockaddr *>(&client_sockaddr),
	                                   reinterpret_cast<struct sockaddr *>(&server_sockaddr));
	constexpr int32_t queuepct = 37;
	constexpr int32_t queuelen = 0;
	constexpr int32_t queuemax = 0;
	assert_event_presence(
	        create_safe_scap_event(ts,
	                               tid,
	                               PPME_SOCKET_ACCEPT_5_X,
	                               5,
	                               fd,
	                               scap_const_sized_buffer{tuple.data(), tuple.size()},
	                               queuepct,
	                               queuelen,
	                               queuemax));
}
#endif

////////////////////////////
// WRITE
////////////////////////////

TEST_F(scap_file_test, write_x_check_final_converted_event) {
	open_filename("scap_2013.scap");

	// Inside the scap-file the event `511534` is the following:
	// - type=PPME_SYSCALL_WRITE_X
	// - ts=1380933088286397273
	// - tid=44106
	// - args=res=77 data=GET / HTTP/1.0..Host: 127.0.0.1..User-Agent: ApacheBench/2.3..Accept:
	// */*...
	//
	// And its corresponding enter event `511520` is the following:
	// - type=PPME_SYSCALL_WRITE_E
	// - ts=1380933088286362703
	// - tid=44106,
	// - args=fd=13(<4t>127.0.0.1:38904->127.0.0.1:80) size=77
	//
	constexpr uint64_t ts = 1380933088286397273;
	constexpr int64_t tid = 44106;
	constexpr int64_t res = 77;
	// this is NULL termiinated so we have 81 bytes but in the scap-file we want only 80 bytes
	// without the NULL terminator
	constexpr char buf[] = {
	        "GET / HTTP/1.0\r\nHost: 127.0.0.1\r\nUser-Agent: ApacheBench/2.3\r\nAccept: "
	        "*/*\r\n\r\n"};
	constexpr int64_t fd = 13;
	constexpr uint32_t size = 77;
	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SYSCALL_WRITE_X,
	                                             4,
	                                             res,
	                                             scap_const_sized_buffer{buf, sizeof(buf) - 1},
	                                             fd,
	                                             size));
}

////////////////////////////
// PWRITE
////////////////////////////

// We don't have scap-files with PWRITE events. Add it if we face a failure.

////////////////////////////
// READV
////////////////////////////

// We don't have scap-files with READV events. Add it if we face a failure.

////////////////////////////
// WRITEV
////////////////////////////

TEST_F(scap_file_test, writev_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `881318` is the following:
	// - type=PPME_SYSCALL_WRITEV_X,
	// - ts=1687966733260040207
	// - tid=129563
	// - args=res=168 data=1687966733.2580034733
	// _p9k_vcs_resume 1.1./home/ubuntu/libs.f45051394208e04ef35f
	//
	// And its corresponding enter event `881317` is the following:
	// - type=PPME_SYSCALL_WRITEV_E
	// - ts=1687966733260035546
	// - tid=129563
	// - args=fd=1(<p>pipe:[339160]) size=168
	//
	// Let's see the new PPME_SYSCALL_WRITEV_X event!

	constexpr uint64_t ts = 1687966733260040207;
	constexpr int64_t tid = 129563;
	constexpr int64_t res = 168;
	constexpr char data[] =
	        "1687966733.2580034733 _p9k_vcs_resume 1"
	        "\x1f"
	        "1"
	        "\x1f"
	        "/home/ubuntu/libs"
	        "\x1f"
	        "f45051394208e04ef35f";
	constexpr int64_t fd = 1;
	constexpr uint32_t size = 168;
	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SYSCALL_WRITEV_X,
	                                             4,
	                                             res,
	                                             scap_const_sized_buffer{data, sizeof(data) - 1},
	                                             fd,
	                                             size));
}

////////////////////////////
// PREADV
////////////////////////////

// We don't have scap-files with PREADV events. Add it if we face a failure.

////////////////////////////
// PWRITEV
////////////////////////////

// We don't have scap-files with PWRITEV events. Add it if we face a failure.

////////////////////////////
// SETRESUID
////////////////////////////

TEST_F(scap_file_test, setresuid_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `293991` is the following:
	// - type=PPME_SYSCALL_SETRESUID_X,
	// - ts=1687889196229754428
	// - tid=107389
	// - args=res=0
	//
	// And its corresponding enter event `293990` is the following:
	// - type=PPME_SYSCALL_SETRESUID_E
	// - ts=1687889196229752468
	// - tid=107389
	// - args=ruid=1000(ubuntu) euid=-1(<NONE>) suid=-1(<NONE>)
	//
	// Let's see the new PPME_SYSCALL_SETRESUID_X event!

	constexpr uint64_t ts = 1687889196229754428;
	constexpr int64_t tid = 107389;
	constexpr int64_t res = 0;
	constexpr uint32_t ruid = 1000;
	constexpr uint32_t euid = static_cast<uint32_t>(-1);
	constexpr uint32_t suid = static_cast<uint32_t>(-1);

	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_SETRESUID_X, 4, res, ruid, euid, suid));
}

////////////////////////////
// SETUID
////////////////////////////

TEST_F(scap_file_test, setuid_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `61288` is the following:
	// - type=PPME_SYSCALL_SETUID_X,
	// - ts=1687966709959025387
	// - tid=141446
	// - args=res=0
	//
	// And its corresponding enter event `61285` is the following:
	// - type=PPME_SYSCALL_SETUID_E
	// - ts=1687966709959015344
	// - tid=141446
	// - args=uid=0(<NA>)
	//
	// Let's see the new PPME_SYSCALL_SETUID_X event!

	constexpr uint64_t ts = 1687966709959025387;
	constexpr int64_t tid = 141446;
	constexpr int64_t res = 0;
	constexpr int32_t uid = 0;
	assert_event_presence(create_safe_scap_event(ts, tid, PPME_SYSCALL_SETUID_X, 2, res, uid));
}

////////////////////////////
// RECV
////////////////////////////

// We don't have scap-files with RECV events. Add it if we face a failure.

////////////////////////////
// RECVFROM
////////////////////////////

// Compile out this test if test_utils helpers are not defined.
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
TEST_F(scap_file_test, recvfrom_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `593051` is the following:
	// - type=PPME_SOCKET_RECVFROM_X,
	// - ts=1687966725502692767
	// - tid=141633
	// - args=res=89 data=.............ip-172-31-24-0.eu-central-1.compute.internal..............,..
	//   ...... tuple=127.0.0.53:53->127.0.0.1:47288
	//
	// And its corresponding enter event `593050` is the following:
	// - type=PPME_SOCKET_RECVFROM_E
	// - ts=1687966725502689763
	// - args=fd=6(<4u>127.0.0.1:47288->127.0.0.53:53) size=2048
	//
	// Let's see the new PPME_SOCKET_RECVFROM_X event!

	constexpr uint64_t ts = 1687966725502692767;
	constexpr int64_t tid = 141633;
	constexpr int64_t res = 89;
	constexpr uint8_t data[] = {
	        0xe5, 0xa9, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0e, 'i',
	        'p',  '-',  '1',  '7',  '2',  '-',  '3',  '1',  '-',  '2',  '4',  '-',  '0',  0x0c,
	        'e',  'u',  '-',  'c',  'e',  'n',  't',  'r',  'a',  'l',  '-',  '1',  0x07, 'c',
	        'o',  'm',  'p',  'u',  't',  'e',  0x08, 'i',  'n',  't',  'e',  'r',  'n',  'a',
	        'l',  0x00, 0x00, 0x01, 0x00, 0x01, 0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
	        0x00, ',',  0x00, 0x04, 0xac, 0x1f, 0x18, 0x00, 0x00, 0x00};
	sockaddr_in client_sockaddr = test_utils::fill_sockaddr_in(53, "127.0.0.53");
	sockaddr_in server_sockaddr = test_utils::fill_sockaddr_in(47288, "127.0.0.1");
	const std::vector<uint8_t> tuple =
	        test_utils::pack_socktuple(reinterpret_cast<struct sockaddr *>(&client_sockaddr),
	                                   reinterpret_cast<struct sockaddr *>(&server_sockaddr));
	constexpr int64_t fd = 6;
	constexpr int32_t size = 2048;
	assert_event_presence(
	        create_safe_scap_event(ts,
	                               tid,
	                               PPME_SOCKET_RECVFROM_X,
	                               5,
	                               res,
	                               scap_const_sized_buffer{data, sizeof(data)},
	                               scap_const_sized_buffer{tuple.data(), tuple.size()},
	                               fd,
	                               size));
}
#endif

////////////////////////////
// SEND
////////////////////////////

// We don't have scap-files with SEND events. Add it if we face a failure.

////////////////////////////
// SENDTO
////////////////////////////

TEST_F(scap_file_test, sendto_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `857231` is the following:
	// - type=PPME_SOCKET_SENDTO_X
	// - ts=1687966733172651252
	// - tid=114093
	// - args=res=17 data="\x11\x0\x0\x0\x16\x0\x1\x3\x1\x0\x0\x0\x0\x0\x0\x0"
	//
	// And its corresponding enter event `857230` is the following:
	// - type=PPME_SOCKET_SENDTO_E
	// - ts=1687966733172634128
	// - tid=114093
	// - args=fd=22(<n>) size=17 tuple=NULL
	//
	// Let's see the new PPME_SOCKET_SENDTO_X event!

	constexpr uint64_t ts = 1687966733172651252;
	constexpr int64_t tid = 114093;
	constexpr int64_t res = 17;
	constexpr char data[] = "\x11\x0\x0\x0\x16\x0\x1\x3\x1\x0\x0\x0\x0\x0\x0\x0";
	constexpr uint32_t size = sizeof(data);
	constexpr int64_t fd = 22;
	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SOCKET_SENDTO_X,
	                                             5,
	                                             res,
	                                             scap_const_sized_buffer{data, size},
	                                             fd,
	                                             size,
	                                             scap_const_sized_buffer{nullptr, 0}));
}

////////////////////////////
// SHUTDOWN
////////////////////////////
TEST_F(scap_file_test, shutdown_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `861764` is the following:
	// - type=PPME_SOCKET_SHUTDOWN_X
	// - ts=1687966733231918487
	// - tid=112954
	// - args=res=-107(ENOTCONN)
	//
	// And its corresponding enter event `861763` is the following:
	// - type=PPME_SOCKET_SHUTDOWN_E
	// - ts=1687966733231918028
	// - tid=112954
	// - args=fd=13(<4t>127.0.0.1:33566->127.0.0.1:42891) how=1(SHUT_WR)
	//
	// Let's see the new PPME_SOCKET_SHUTDOWN_X event!

	constexpr uint64_t ts = 1687966733231918487;
	constexpr int64_t tid = 112954;
	constexpr int64_t res = -107;
	constexpr int64_t fd = 13;
	constexpr uint8_t how = 1;
	assert_event_presence(create_safe_scap_event(ts, tid, PPME_SOCKET_SHUTDOWN_X, 3, res, fd, how));
}

////////////////////////////
// SOCKETPAIR
////////////////////////////

TEST_F(scap_file_test, socketpair_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `839802` is the following:
	// - type=PPME_SOCKET_SOCKETPAIR_X
	// - ts=1687966732709347847
	// - tid=118552
	// - args=res=0 fd1=28(<u>) fd2=29(<u>) source=FFFF0003C2F15C00 peer=FFFF0003C2F16C00
	//
	// And its corresponding enter event `839801` is the following:
	// - type=PPME_SOCKET_SOCKETPAIR_E
	// - ts=1687966732709343195
	// - tid=118552
	// - args=domain=1(AF_LOCAL|AF_UNIX) type=524289 proto=0
	//
	// Let's see the new PPME_SOCKET_SOCKETPAIR_X event!

	constexpr uint64_t ts = 1687966732709347847;
	constexpr int64_t tid = 118552;
	constexpr int64_t res = 0;
	constexpr int64_t fd1 = 28;
	constexpr int64_t fd2 = 29;
	constexpr uint64_t source = 0xFFFF0003C2F15C00;
	constexpr uint64_t peer = 0xFFFF0003C2F16C00;
	constexpr uint32_t domain = AF_UNIX;
	constexpr uint32_t type = 524289;
	constexpr uint32_t proto = 0;
	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SOCKET_SOCKETPAIR_X,
	                                             8,
	                                             res,
	                                             fd1,
	                                             fd2,
	                                             source,
	                                             peer,
	                                             domain,
	                                             type,
	                                             proto));
}

////////////////////////////
// SENDMSG
////////////////////////////

// Compile out this test if test_utils helpers are not defined.
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
TEST_F(scap_file_test, sendmsg_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `593037` is the following:
	// - type=PPME_SOCKET_SENDMSG_X
	// - ts=1687966725502664007
	// - tid=493
	// - args=res=89
	// data=.............ip-172-31-24-0.eu-central-1.compute.internal..............,........
	//
	// And its corresponding enter event `593036` is the following:
	// - type=PPME_SOCKET_SENDMSG_E
	// - ts=1687966725502632237
	// - tid=493
	// - args=fd=13(<4u>127.0.0.1:47288->127.0.0.53:53) size=89 tuple=127.0.0.53:53->127.0.0.1:47288
	//
	// Let's see the new PPME_SOCKET_SENDMSG_X event!

	constexpr uint64_t ts = 1687966725502664007;
	constexpr int64_t tid = 493;

	constexpr int64_t res = 89;
	constexpr uint8_t data[] = {
	        0xe5, 0xa9, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0e, 'i',
	        'p',  '-',  '1',  '7',  '2',  '-',  '3',  '1',  '-',  '2',  '4',  '-',  '0',  0x0c,
	        'e',  'u',  '-',  'c',  'e',  'n',  't',  'r',  'a',  'l',  '-',  '1',  0x07, 'c',
	        'o',  'm',  'p',  'u',  't',  'e',  0x08, 'i',  'n',  't',  'e',  'r',  'n',  'a',
	        'l',  0x00, 0x00, 0x01, 0x00, 0x01, 0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
	        0x00, ',',  0x00, 0x04, 0xac, 0x1f, 0x18, 0x00, 0x00, 0x00};
	constexpr int64_t fd = 13;
	constexpr int32_t size = 89;
	sockaddr_in client_sockaddr = test_utils::fill_sockaddr_in(53, "127.0.0.53");
	sockaddr_in server_sockaddr = test_utils::fill_sockaddr_in(47288, "127.0.0.1");
	const std::vector<uint8_t> tuple =
	        test_utils::pack_socktuple(reinterpret_cast<struct sockaddr *>(&client_sockaddr),
	                                   reinterpret_cast<struct sockaddr *>(&server_sockaddr));
	assert_event_presence(
	        create_safe_scap_event(ts,
	                               tid,
	                               PPME_SOCKET_SENDMSG_X,
	                               5,
	                               res,
	                               scap_const_sized_buffer{data, sizeof(data)},
	                               fd,
	                               size,
	                               scap_const_sized_buffer{tuple.data(), tuple.size()}));
}
#endif

////////////////////////////
// RECVMSG
////////////////////////////

// Compile out this test if test_utils helpers are not defined.
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
TEST_F(scap_file_test, recvmsg_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `593019` is the following:
	// - type=PPME_SOCKET_RECVMSG_X
	// - ts=1687966725502520629
	// - tid=493
	// args=res=73 size=73 data=...
	// .........ip-172-31-24-0.eu-central-1.compute.internal.......)........
	// tuple=127.0.0.1:47288->127.0.0.53:53
	//
	// And its corresponding enter event `593018` is the following:
	// - type=PPME_SOCKET_RECVMSG_E
	// - ts=1687966725502515632
	// - tid=493
	// - args=fd=13(<4u>127.0.0.1:40646->127.0.0.53:53)
	//
	// Let's see the new PPME_SOCKET_RECVMSG_X event!

	constexpr uint64_t ts = 1687966725502520629;
	constexpr int64_t tid = 493;

	constexpr int64_t res = 73;
	constexpr int32_t size = res;
	constexpr uint8_t data[] = {0xe5, 0xa9, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	                            0x01, 0x0e, 'i',  'p',  '-',  '1',  '7',  '2',  '-',  '3',  '1',
	                            '-',  '2',  '4',  '-',  '0',  0x0c, 'e',  'u',  '-',  'c',  'e',
	                            'n',  't',  'r',  'a',  'l',  '-',  '1',  0x07, 'c',  'o',  'm',
	                            'p',  'u',  't',  'e',  0x08, 'i',  'n',  't',  'e',  'r',  'n',
	                            'a',  'l',  0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x29, 0x04,
	                            0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	sockaddr_in client_sockaddr = test_utils::fill_sockaddr_in(47288, "127.0.0.1");
	sockaddr_in server_sockaddr = test_utils::fill_sockaddr_in(53, "127.0.0.53");
	const std::vector<uint8_t> tuple =
	        test_utils::pack_socktuple(reinterpret_cast<struct sockaddr *>(&client_sockaddr),
	                                   reinterpret_cast<struct sockaddr *>(&server_sockaddr));
	constexpr int64_t fd = 13;

	assert_event_presence(
	        create_safe_scap_event(ts,
	                               tid,
	                               PPME_SOCKET_RECVMSG_X,
	                               6,
	                               res,
	                               size,
	                               scap_const_sized_buffer{data, sizeof(data)},
	                               scap_const_sized_buffer{tuple.data(), tuple.size()},
	                               scap_const_sized_buffer{nullptr, 0},
	                               fd));
}
#endif

////////////////////////////
///// EVENTFD
////////////////////////////

// We don't have scap-files with EVENTFD events. Add it if we face a failure.

////////////////////////////
// MKDIR
////////////////////////////

TEST_F(scap_file_test, mkdir_x_check_final_converted_event) {
	open_filename("mkdir.scap");

	// Inside the scap-file the event `465` is the following:
	// - type=PPME_SYSCALL_MKDIR_2_X,
	// - ts=1749017847850665826
	// - tid=1163259
	// - args=res=-13(EACCES) path=/hello
	//
	// And its corresponding enter event `464` is the following:
	// - type=PPME_SYSCALL_MKDIR_2_E
	// - ts=1749017847850637066
	// - tid=1163259
	// - args=mode=1FF
	//
	// Let's see the new PPME_SYSCALL_MKDIR_2_X event!

	constexpr uint64_t ts = 1749017847850665826;
	constexpr int64_t tid = 1163259;
	constexpr int64_t res = -13;
	constexpr char path[] = "/hello";
	constexpr uint32_t mode = 0x1ff;  // 0777 in octal
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_MKDIR_2_X, 3, res, path, mode));
}

////////////////////////////
// UNSHARE
////////////////////////////

TEST_F(scap_file_test, unshare_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `61225` is the following:
	// - type=PPME_SYSCALL_UNSHARE_X,
	// - ts=1687966709958564213
	// - tid=141445
	// - args=res=0
	//
	// And its corresponding enter event `61224` is the following:
	// - type=PPME_SYSCALL_UNSHARE_E
	// - ts=1687966709958563138
	// - tid=141445
	// - args=flags=0
	//
	// Let's see the new PPME_SYSCALL_UNSHARE_X event!

	constexpr uint64_t ts = 1687966709958564213;
	constexpr int64_t tid = 141445;
	constexpr int64_t res = 0;
	constexpr uint32_t flags = 0;
	assert_event_presence(create_safe_scap_event(ts, tid, PPME_SYSCALL_UNSHARE_X, 2, res, flags));
}

////////////////////////////
///// FUTEX
////////////////////////////

TEST_F(scap_file_test, futex_x_check_final_converted_event) {
	open_filename("sample.scap");

	// Inside the scap-file the event `177` is the following:
	// - type=PPME_SYSCALL_FUTEX_X,
	// - ts=1690557262892768316
	// - tid=418
	// - args=res=0
	//
	// And its corresponding enter event `176` is the following:
	// - type=PPME_SYSCALL_FUTEX_E
	// - ts=1690557262892767595
	// - tid=418
	// - args=addr=5600C32351E0 op=129(FUTEX_PRIVATE_FLAG|FUTEX_WAKE) val=1
	//
	// Let's see the new PPME_SYSCALL_FUTEX_X event!

	constexpr uint64_t ts = 1690557262892768316;
	constexpr int64_t tid = 418;
	constexpr int64_t res = 0;
	constexpr uint64_t addr = 0x5600C32351E0;
	constexpr uint16_t op = 129;
	constexpr uint64_t val = 1;
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_FUTEX_X, 4, res, addr, op, val));
}

////////////////////////////
///// FSTAT
////////////////////////////

TEST_F(scap_file_test, fstat_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `899035` is the following:
	// - type=PPME_SYSCALL_FSTAT_X,
	// - ts=1687966733785539028
	// - tid=114100
	// - args=res=0
	//
	// And its corresponding enter event `899034` is the following:
	// - type=PPME_SYSCALL_FSTAT_E
	// - ts=1687966733785538273
	// - tid=114100
	// -
	// args=fd=19(<f>/sys/fs/cgroup/kubelet.slice/kubelet-kubepods.slice/kubelet-kubepods-besteffort.slice/kubelet-kubepods-besteffort-pod506a980e_4d84_43bf_9c8f_c8811e541ce2.slice/cgroup.controllers)
	//
	// Let's see the new PPME_SYSCALL_FSTAT_X event!

	constexpr uint64_t ts = 1687966733785539028;
	constexpr int64_t tid = 114100;
	constexpr int64_t res = 0;
	constexpr int64_t fd = 19;
	assert_event_presence(create_safe_scap_event(ts, tid, PPME_SYSCALL_FSTAT_X, 2, res, fd));
}

////////////////////////////
///// EPOLL_WAIT
////////////////////////////

TEST_F(scap_file_test, epoll_wait_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `522235` is the following:
	// - type=PPME_SYSCALL_EPOLLWAIT_X,
	// - ts=1687889198957001006
	// - tid=1385
	// - args=res=0
	//
	// And its corresponding enter event `522236` is the following:
	// - type=PPME_SYSCALL_EPOLLWAIT_E
	// - ts=1687889198956999803
	// - tid=1385
	// - args=maxevents=1024
	//
	// Let's see the new PPME_SYSCALL_EPOLLWAIT_X event!

	constexpr uint64_t ts = 1687889198957001006;
	constexpr int64_t tid = 1385;
	constexpr int64_t res = 0;
	constexpr int64_t maxevents = 1024;
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_EPOLLWAIT_X, 2, res, maxevents));
}

////////////////////////////
///// POLL
////////////////////////////

TEST_F(scap_file_test, poll_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `520415` is the following:
	// - type=PPME_SYSCALL_POLL_X,
	// - ts=1687889198874896483
	// - tid=99525
	// - args=res=1 fds=22:i1
	//
	// And its corresponding enter event `520414` is the following:
	// - type=PPME_SYSCALL_POLL_E
	// - ts=1687889198874895459
	// - tid=99525
	// - args=fds=20:p1 22:i1 timeout=500
	//
	// Let's see the new PPME_SYSCALL_POLL_X event!

	constexpr uint64_t ts = 1687889198874896483;
	constexpr int64_t tid = 99525;
	constexpr int64_t res = 1;
	constexpr uint8_t fds[] = {0x1, 0x0, 0x16, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0};
	constexpr int64_t timeout = 500;
	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SYSCALL_POLL_X,
	                                             3,
	                                             res,
	                                             scap_const_sized_buffer{fds, sizeof(fds)},
	                                             timeout));
}

////////////////////////////
///// LSEEK
////////////////////////////

TEST_F(scap_file_test, lseek_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `896635` is the following:
	// - type=PPME_SYSCALL_LSEEK_X,
	// - ts=1687966733725654218
	// - tid=141698
	// - args=res=971
	//
	// And its corresponding enter event `896634` is the following:
	// - type=PPME_SYSCALL_LSEEK_E
	// - ts=1687966733725652701
	// - tid=141698
	// -
	// args=fd=255(<f>/home/ubuntu/.vscode-server/bin/695af097c7bd098fbf017ce3ac85e09bbc5dda06/out/vs/base/node/cpuUsage.sh)
	// offset=-751 whence=1(SEEK_CUR)
	//
	// Let's see the new PPME_SYSCALL_LSEEK_X event!

	constexpr uint64_t ts = 1687966733725654218;
	constexpr int64_t tid = 141698;
	constexpr int64_t res = 971;
	constexpr int64_t fd = 255;
	constexpr uint64_t offset = -751;
	constexpr uint8_t whence = 1;
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_LSEEK_X, 4, res, fd, offset, whence));
}

////////////////////////////
// LLSEEK
////////////////////////////

// We don't have scap-files with LLSEEK events. Add it if we face a failure.

////////////////////////////
///// IOCTL
////////////////////////////

TEST_F(scap_file_test, ioctl_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `903005` is the following:
	// - type=PPME_SYSCALL_IOCTL_3_X,
	// - ts=1687966733986436903
	// - tid=118552
	// - args=res=0
	//
	// And its corresponding enter event `903004` is the following:
	// - type=PPME_SYSCALL_IOCTL_3_E
	// - ts=1687966733986427631
	// - tid=118552
	// - args=fd=21(<f>/dev/ptmx) request=5414 argument=FFFFD297F908
	//
	// Let's see the new PPME_SYSCALL_IOCTL_3_X event!

	constexpr uint64_t ts = 1687966733986436903;
	constexpr int64_t tid = 118552;
	constexpr int64_t res = 0;
	constexpr int64_t fd = 21;
	constexpr uint64_t request = 0x5414;
	constexpr uint64_t argument = 0xFFFFD297F908;
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_IOCTL_3_X, 4, res, fd, request, argument));
}

////////////////////////////
// MMAP
////////////////////////////

TEST_F(scap_file_test, mmap_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `513124` is the following:
	// - type=PPME_SYSCALL_MMAP_X,
	// - ts=1687889198695960021
	// - tid=107452
	// - args=res=139631788478464 vm_size=5908 vm_rss=1024 vm_swap=0
	//
	// And its corresponding enter event `513123` is the following:
	// - type=PPME_SYSCALL_MMAP_E
	// - ts=1687889198695957637
	// - tid=107452
	// - args=addr=0 length=139264 prot=3(PROT_READ|PROT_WRITE) flags=10(MAP_PRIVATE|MAP_ANONYMOUS)
	//   fd=-1(EPERM) offset=0
	//
	// Let's see the new PPME_SYSCALL_MMAP_X event!

	constexpr uint64_t ts = 1687889198695960021;
	constexpr int64_t tid = 107452;
	constexpr int64_t res = 139631788478464;
	constexpr uint32_t vm_size = 5908;
	constexpr uint32_t vm_rss = 1024;
	constexpr uint32_t vm_swap = 0;
	constexpr uint64_t addr = 0;
	constexpr uint64_t length = 139264;
	constexpr uint32_t prot = 3;
	constexpr uint32_t flags = 10;
	constexpr int64_t fd = -1;
	constexpr uint64_t offset = 0;
	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SYSCALL_MMAP_X,
	                                             10,
	                                             res,
	                                             vm_size,
	                                             vm_rss,
	                                             vm_swap,
	                                             addr,
	                                             length,
	                                             prot,
	                                             flags,
	                                             fd,
	                                             offset));
}

////////////////////////////
// MMAP2
////////////////////////////

// We don't have scap-files with MMAP2 events. Add it if we face a failure.

////////////////////////////
// MUNMAP
////////////////////////////

TEST_F(scap_file_test, munmap_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `897614` is the following:
	// - type=PPME_SYSCALL_MUNMAP_X,
	// - ts=1687966733729451471
	// - tid=141707
	// - args=res=0 vm_size=5188 vm_rss=1380 vm_swap=0
	//
	// And its corresponding enter event `897613` is the following:
	// - type=PPME_SYSCALL_MUNMAP_E
	// - ts=1687966733729445022
	// - tid=141707
	// - args=addr=FFFFA778E000 length=139264
	//
	// Let's see the new PPME_SYSCALL_MUNMAP_X event!

	constexpr uint64_t ts = 1687966733729451471;
	constexpr int64_t tid = 141707;
	constexpr int64_t res = 0;
	constexpr uint32_t vm_size = 5188;
	constexpr uint32_t vm_rss = 1380;
	constexpr uint32_t vm_swap = 0;
	constexpr uint64_t addr = 0xFFFFA778E000;
	constexpr uint64_t length = 139264;
	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SYSCALL_MUNMAP_X,
	                                             6,
	                                             res,
	                                             vm_size,
	                                             vm_rss,
	                                             vm_swap,
	                                             addr,
	                                             length));
}

////////////////////////////
// SPLICE
////////////////////////////

TEST_F(scap_file_test, splice_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `862252` is the following:
	// - type=PPME_SYSCALL_SPLICE_X,
	// - ts=1687966733234462655
	// - tid=112950
	// - args=res=0
	//
	// And its corresponding enter event `862251` is the following:
	// - type=PPME_SYSCALL_SPLICE_E
	// - ts=1687966733234461540
	// - tid=112950
	// - args=fd_in=5(<4t>172.18.0.1:40470->172.18.0.2:6443) fd_out=12(<p>pipe:[268409])
	// size=4194304 flags=2(SPLICE_F_NONBLOCK)
	//
	// Let's see the new PPME_SYSCALL_SPLICE_X event!

	constexpr uint64_t ts = 1687966733234462655;
	constexpr int64_t tid = 112950;
	constexpr int64_t res = 0;
	constexpr int64_t fd_in = 5;
	constexpr int64_t fd_out = 12;
	constexpr uint64_t size = 4194304;
	constexpr uint32_t flags = 2;
	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SYSCALL_SPLICE_X,
	                                             5,
	                                             res,
	                                             fd_in,
	                                             fd_out,
	                                             size,
	                                             flags));
}

////////////////////////////
// PTRACE
////////////////////////////

TEST_F(scap_file_test, ptrace_x_check_final_converted_event) {
	open_filename("ptrace.scap");

	// Inside the scap-file the event `464` is the following:
	// - type=PPME_SYSCALL_PTRACE_X,
	// - ts=1747834548577695347
	// - tid=368860
	// - args=res=0 addr=78 data=3B
	//
	// And its corresponding enter event `463` is the following:
	// - type=PPME_SYSCALL_PTRACE_E
	// - ts=1747834548577692897
	// - tid=368860
	// - args=request=4(PTRACE_PEEKUSR) pid=368861(mystrace)
	//
	// Let's see the new PPME_SYSCALL_PTRACE_X event!

	constexpr uint64_t ts = 1747834548577695347;
	constexpr int64_t tid = 368860;
	constexpr int64_t res = 0;
	constexpr uint8_t addr[] = {0x00, 0x78, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	constexpr uint8_t data[] = {0x00, 0x3b, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	constexpr uint16_t request = 4;
	constexpr int64_t pid = 368861;
	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SYSCALL_PTRACE_X,
	                                             5,
	                                             res,
	                                             scap_const_sized_buffer{addr, sizeof(addr)},
	                                             scap_const_sized_buffer{data, sizeof(data)},
	                                             request,
	                                             pid));
}

////////////////////////////
// SENDFILE
////////////////////////////

// We don't have scap-files with SENDFILE events. Add it if we face a failure.

////////////////////////////
// QUOTACTL
////////////////////////////

// We don't have scap-files with QUOTACTL events. Add it if we face a failure.

////////////////////////////
// GETDENTS
////////////////////////////

// We don't have scap-files with GETDENTS events (scap_2013 contains only PPME_GENERIC_* events
// originated from unsupported getdents events, but since they formally have another event type, we
// cannot leverage them) . Add it if we face a failure.

////////////////////////////
// GETDENTS64
////////////////////////////

TEST_F(scap_file_test, getdents64_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `902321` is the following:
	// - type=PPME_SYSCALL_GETDENTS64_X,
	// - ts=1687966733947098286
	// - tid=114095
	// - args=res=144
	//
	// And its corresponding enter event `902320` is the following:
	// - type=PPME_SYSCALL_GETDENTS64_E
	// - ts=1687966733947092756
	// - tid=114095
	// -
	// args=fd=19(<f>/var/lib/kubelet/pods/506a980e-4d84-43bf-9c8f-c8811e541ce2/volumes/kubernetes.io~projected/kube-api-access-hknbt/..2023_06_28_15_37_47.388872689
	//
	// Let's see the new PPME_SYSCALL_GETDENTS64_X event!

	constexpr uint64_t ts = 1687966733947098286;
	constexpr int64_t tid = 114095;
	constexpr int64_t res = 144;
	constexpr int64_t fd = 19;
	assert_event_presence(create_safe_scap_event(ts, tid, PPME_SYSCALL_GETDENTS64_X, 2, res, fd));
}

////////////////////////////
// SETNS
////////////////////////////

TEST_F(scap_file_test, setns_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `156638` is the following:
	// - type=PPME_SYSCALL_SETNS_X,
	// - ts=1687889193606963670
	// - tid=107363
	// - args=res=0
	//
	// And its corresponding enter event `156637` is the following:
	// - type=PPME_SYSCALL_SETNS_E
	// - ts=1687889193606959614
	// - tid=107363
	// - args=fd=6(<f>/proc/1993/ns/ipc) nstype=8(CLONE_NEWIPC)
	//
	// Let's see the new PPME_SYSCALL_SETNS_X event!

	constexpr uint64_t ts = 1687889193606963670;
	constexpr int64_t tid = 107363;
	constexpr int64_t res = 0;
	constexpr int64_t fd = 6;
	constexpr uint32_t nstype = 8;  // CLONE_NEWIPC
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_SETNS_X, 3, res, fd, nstype));
}

////////////////////////////
// FLOCK
////////////////////////////

// We don't have scap-files with FLOCK events. Add it if we face a failure.

////////////////////////////
// SEMOP
////////////////////////////

// We don't have scap-files with SEMOP events (scap_2013 contains only PPME_GENERIC_* events
// originated from unsupported semop events, but since they formally have another event type, we
// cannot leverage them) . Add it if we face a failure.

////////////////////////////
// PPOLL
////////////////////////////

TEST_F(scap_file_test, ppoll_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `903273` is the following:
	// - type=PPME_SYSCALL_PPOLL_X,
	// - ts=1687966733988132906
	// - tid=129339
	// - args=res=1 fds=4:44
	//
	// And its corresponding enter event `903272` is the following:
	// - type=PPME_SYSCALL_PPOLL_E
	// - ts=1687966733988129698
	// - tid=129339
	// - args=fds=4:41 4:44 10:p1 12:p1 7:41 11:41 timeout=0(0s) sigmask=
	//
	// Let's see the new PPME_SYSCALL_PPOLL_X event!

	constexpr uint64_t ts = 1687966733988132906;
	constexpr int64_t tid = 129339;
	constexpr int64_t res = 1;
	constexpr uint8_t fds[] = {0x1, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4, 0x0};
	constexpr uint64_t timeout = 0;
	constexpr uint32_t sigmask = 0;
	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SYSCALL_PPOLL_X,
	                                             4,
	                                             res,
	                                             scap_const_sized_buffer{fds, sizeof(fds)},
	                                             timeout,
	                                             sigmask));
}

////////////////////////////
// MOUNT
////////////////////////////

TEST_F(scap_file_test, mount_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `155639` is the following:
	// - type=PPME_SYSCALL_MOUNT_X,
	// - ts=1687889193604667914
	// - tid=107361
	// - args=res=0 dev=
	// dir=/run/containerd/runc/k8s.io/d7717c36108697b040257e6d78a8980a763d3b22e437cf199477b9a142537c67/runc.J8eCT9
	// type=
	//
	// And its corresponding enter event `155638` is the following:
	// - type=PPME_SYSCALL_MOUNT_E
	// - ts=1687889193604651991
	// - tid=107361
	// - args=flags=4129(RDONLY|REMOUNT|BIND)
	//
	// Let's see the new PPME_SYSCALL_MOUNT_X event!

	constexpr uint64_t ts = 1687889193604667914;
	constexpr int64_t tid = 107361;
	constexpr int64_t res = 0;
	constexpr char dev[] = "";
	constexpr char dir[] =
	        "/run/containerd/runc/k8s.io/"
	        "d7717c36108697b040257e6d78a8980a763d3b22e437cf199477b9a142537c67/runc.J8eCT9";
	constexpr char fstype[] = "";
	constexpr uint32_t flags = 4129;
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_MOUNT_X, 5, res, dev, dir, fstype, flags));
}

////////////////////////////
// SEMCTL
////////////////////////////

// We don't have scap-files with SEMCTL events. Add it if we face a failure.

////////////////////////////
// SEMGET
////////////////////////////

// We don't have scap-files with SEMGET events. Add it if we face a failure.

////////////////////////////
// ACCESS
////////////////////////////

TEST_F(scap_file_test, access_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `513024` is the following:
	// - type=PPME_SYSCALL_ACCESS_X,
	// - ts=1687889198695606972
	// - tid=107452
	// - args=res=-2(ENOENT) name=/etc/ld.so.preload
	//
	// And its corresponding enter event `513023` is the following:
	// - type=PPME_SYSCALL_ACCESS_E
	// - ts=1687889198695603284
	// - tid=107452
	// - args=mode=4(R_OK)
	//
	// Let's see the new PPME_SYSCALL_ACCESS_X event!

	constexpr uint64_t ts = 1687889198695606972;
	constexpr int64_t tid = 107452;
	constexpr int64_t res = -2;
	constexpr char name[] = "/etc/ld.so.preload";
	constexpr uint32_t mode = 4;
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_ACCESS_X, 3, res, name, mode));
}

////////////////////////////
// FCHDIR
////////////////////////////

TEST_F(scap_file_test, fchdir_x_check_final_converted_event) {
	open_filename("fchdir.scap");

	// Inside the scap-file the event `370` is the following:
	// - type=PPME_SYSCALL_FCHDIR_X,
	// - ts=1749117249748433380
	// - tid=1377498
	// - args=res=-9(EBADF)
	//
	// And its corresponding enter event `369` is the following:
	// - type=PPME_SYSCALL_FCHDIR_E,
	// - ts=1749117249748432840
	// - tid=1377498
	// - args=fd=25
	//
	// Let's see the new PPME_SYSCALL_FCHDIR_X event!

	constexpr uint64_t ts = 1749117249748433380;
	constexpr int64_t tid = 1377498;
	constexpr int64_t res = -9;
	constexpr int64_t fd = 25;
	assert_event_presence(create_safe_scap_event(ts, tid, PPME_SYSCALL_FCHDIR_X, 2, res, fd));
}

////////////////////////////
// SETPGID
////////////////////////////

TEST_F(scap_file_test, setpgid_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `123127` is the following:
	// - type=PPME_SYSCALL_SETPGID_X,
	// - ts=1687889193490376726
	// - tid=107344
	// - args=res=0
	//
	// And its corresponding enter event `123126` is the following:
	// - type=PPME_SYSCALL_SETPGID_E
	// - ts=1687889193490374360
	// - tid=107344
	// - args=pid=0 pgid=107344(zsh)
	//
	// Let's see the new PPME_SYSCALL_SETPGID_X event!

	constexpr uint64_t ts = 1687889193490376726;
	constexpr int64_t tid = 107344;
	constexpr int64_t res = 0;
	constexpr int64_t pid = 0;
	constexpr int64_t pgid = 107344;  // zsh process ID
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_SETPGID_X, 3, res, pid, pgid));
}

////////////////////////////
// SECCOMP
////////////////////////////

TEST_F(scap_file_test, seccomp_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `62555` is the following:
	// - type=PPME_SYSCALL_SECCOMP_X,
	// - ts=1687966709992615590
	// - tid=141446
	// - args=res=-14(EFAULT)
	//
	// And its corresponding enter event `62554` is the following:
	// - type=PPME_SYSCALL_SECCOMP_E
	// - ts=1687966709992615023
	// - tid=141446
	// - args=op=1
	//
	// Let's see the new PPME_SYSCALL_SECCOMP_X event!

	constexpr uint64_t ts = 1687966709992615590;
	constexpr int64_t tid = 141446;
	constexpr int64_t res = -14;
	constexpr uint64_t op = 1;
	constexpr uint64_t flags = 0;  // Defaulted.
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_SECCOMP_X, 3, res, op, flags));
}

////////////////////////////
// MPROTECT
////////////////////////////

TEST_F(scap_file_test, mprotect_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `513074` is the following:
	// - type=PPME_SYSCALL_MPROTECT_X,
	// - ts=1687889198695780437
	// - tid=107452
	// - args=res=0
	//
	// And its corresponding enter event `513073` is the following:
	// - type=PPME_SYSCALL_MPROTECT_E
	// - ts=1687889198695776877
	// - tid=107452
	// - args=addr=7EFE8F2D7000 length=8192 prot=1(PROT_READ)
	//
	// Let's see the new PPME_SYSCALL_MPROTECT_X event!

	constexpr uint64_t ts = 1687889198695780437;
	constexpr int64_t tid = 107452;
	constexpr int64_t res = 0;
	constexpr uint64_t addr = 0x7EFE8F2D7000;
	constexpr uint64_t len = 8192;
	constexpr uint32_t proto = 1;
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_MPROTECT_X, 4, res, addr, len, proto));
}

////////////////////////////
// EXECVEAT
////////////////////////////

// We don't have scap-files with both enter and exit EXECVEAT events. Add it if we face a failure.

////////////////////////////
// COPY_FILE_RANGE
////////////////////////////

// We don't have scap-files with COPY_FILE_RANGE events. Add it if we face a failure.

////////////////////////////
// EPOLL_CREATE
////////////////////////////

// We don't have scap-files with EPOLL_CREATE events. Add it if we face a failure.

////////////////////////////
// EPOLL_CREATE1
////////////////////////////

TEST_F(scap_file_test, epoll_create1_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `597328` is the following:
	// - type=PPME_SYSCALL_EPOLL_CREATE1_X,
	// - ts=1687966725514490462
	// - tid=141635
	// - args=res=7
	//
	// And its corresponding enter event `597327` is the following:
	// - type=PPME_SYSCALL_EPOLL_CREATE1_E
	// - ts=1687966725514488017
	// - tid=141635
	// - args=flags=1(EPOLL_CLOEXEC)
	//
	// Let's see the new PPME_SYSCALL_EPOLL_CREATE1_X event!

	constexpr uint64_t ts = 1687966725514490462;
	constexpr int64_t tid = 141635;
	constexpr int64_t res = 7;
	constexpr uint32_t flags = 1;
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_EPOLL_CREATE1_X, 2, res, flags));
}

////////////////////////////
// SETGID
////////////////////////////

TEST_F(scap_file_test, setgid_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `160719` is the following:
	// - type=PPME_SYSCALL_SETGID_X,
	// - ts=1687889193630645846
	// - tid=107364
	// - args=res=0
	//
	// And its corresponding enter event `160716` is the following:
	// - type=PPME_SYSCALL_SETGID_E
	// - ts=1687889193630644057
	// - tid=107364
	// - args=gid=0(<NA>)
	//
	// Let's see the new PPME_SYSCALL_SETPGID_X event!

	constexpr uint64_t ts = 1687889193630645846;
	constexpr int64_t tid = 107364;
	constexpr int64_t res = 0;
	constexpr uint32_t gid = 0;
	assert_event_presence(create_safe_scap_event(ts, tid, PPME_SYSCALL_SETGID_X, 2, res, gid));
}

////////////////////////////
// SETRESGID
////////////////////////////

TEST_F(scap_file_test, setresgid_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `293989` is the following:
	// - type=PPME_SYSCALL_SETRESGID_X,
	// - ts=1687889196229751724
	// - tid=107389
	// - args=res=0
	//
	// And its corresponding enter event `293988` is the following:
	// - type=PPME_SYSCALL_SETRESGID_E
	// - ts=1687889196229749397
	// - tid=107389
	// - args=rgid=0(<NA>) egid=0(<NA>) sgid=0(<NA>)
	//
	// Let's see the new PPME_SYSCALL_SETRESGID_X event!

	constexpr uint64_t ts = 1687889196229751724;
	constexpr int64_t tid = 107389;
	constexpr int64_t res = 0;
	constexpr uint32_t rgid = static_cast<uint32_t>(-1);
	constexpr uint32_t egid = 1000;
	constexpr uint32_t sgid = static_cast<uint32_t>(-1);
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_SETRESGID_X, 4, res, rgid, egid, sgid));
}

////////////////////////////
// ACCEPT4
////////////////////////////

TEST_F(scap_file_test, accept4_x_check_final_converted_event) {
	open_filename("kexec_arm64.scap");

	// Inside the scap-file the event `901231` is the following:
	// - type=PPME_SOCKET_ACCEPT4_6_X,
	// - ts=1687966733926550576
	// - tid=115187
	// - args=fd=-11(EAGAIN) tuple=NULL queuepct=0 queuelen=0 queuemax=0
	//
	// And its corresponding enter event `901230` is the following:
	// - type=PPME_SOCKET_ACCEPT4_6_E
	// - ts=1687966733926547983
	// - tid=115187
	// - args=flags=0
	//
	// Let's see the new PPME_SOCKET_ACCEPT4_6_X event!

	constexpr uint64_t ts = 1687966733926550576;
	constexpr int64_t tid = 115187;
	constexpr int64_t fd = -11;
	constexpr uint8_t queuepct = 0;
	constexpr uint32_t queuelen = 0;
	constexpr uint32_t queuemax = 0;
	constexpr int32_t flags = 0;
	assert_event_presence(create_safe_scap_event(ts,
	                                             tid,
	                                             PPME_SOCKET_ACCEPT4_6_X,
	                                             6,
	                                             fd,
	                                             scap_const_sized_buffer{nullptr, 0},
	                                             queuepct,
	                                             queuelen,
	                                             queuemax,
	                                             flags));
}

////////////////////////////
// UMOUNT2
////////////////////////////

TEST_F(scap_file_test, umount2_x_check_final_converted_event) {
	open_filename("kexec_x86.scap");

	// Inside the scap-file the event `156249` is the following:
	// - type=PPME_SYSCALL_UMOUNT2_X,
	// - ts=1687889193605753138
	// - tid=100562
	// - args=res=-2(ENOENT)
	// name=/run/credentials/run-containerd-runc-k8s.io-d7717c36108697b040257e6d78a8980a763d3b22e437cf199477b9a142537c67-runc.J8eCT9.mount
	//
	// And its corresponding enter event `156248` is the following:
	// - type=PPME_SYSCALL_UMOUNT2_E
	// - ts=1687889193605748887
	// - tid=100562
	// - args=flags=10(DETACH|NOFOLLOW)
	//
	// Let's see the new PPME_SYSCALL_UMOUNT2_X event!

	constexpr uint64_t ts = 1687889193605753138;
	constexpr int64_t tid = 100562;
	constexpr int64_t res = -2;
	constexpr char name[] =
	        "/run/credentials/"
	        "run-containerd-runc-k8s.io-"
	        "d7717c36108697b040257e6d78a8980a763d3b22e437cf199477b9a142537c67-runc.J8eCT9.mount";
	constexpr uint32_t flags = 10;
	assert_event_presence(
	        create_safe_scap_event(ts, tid, PPME_SYSCALL_UMOUNT2_X, 3, res, name, flags));
}

////////////////////////////
// EVENTFD2
////////////////////////////

// We don't have scap-files with EVENTFD2 events. Add it if we face a failure.

////////////////////////////
// SIGNALFD4
////////////////////////////

// We don't have scap-files with SIGNALFD4 events. Add it if we face a failure.
