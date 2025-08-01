// SPDX-License-Identifier: Apache-2.0
/*
Copyright (C) 2023 The Falco Authors.

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

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif  // _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits>

#include <libsinsp/sinsp.h>
#include <libsinsp/sinsp_int.h>
#include <libsinsp/parsers.h>
#include <libsinsp/sinsp_errno.h>
#include <libsinsp/filter.h>
#include <libscap/strl.h>
#include <libsinsp/plugin_manager.h>
#include <libsinsp/sinsp_observer.h>
#include <libsinsp/user.h>

sinsp_parser::sinsp_parser(const sinsp_mode &sinsp_mode,
                           const scap_machine_info *const &machine_info,
                           const std::vector<std::string> &event_sources,
                           const size_t syscall_event_source_idx,
                           const sinsp_network_interfaces &network_interfaces,
                           const bool &hostname_and_port_resolution_enabled,
                           const sinsp_threadinfo_factory &threadinfo_factory,
                           const sinsp_fdinfo_factory &fdinfo_factory,
                           const std::shared_ptr<const sinsp_plugin> &input_plugin,
                           const bool &large_envs_enabled,
                           const std::shared_ptr<sinsp_plugin_manager> &plugin_manager,
                           const std::shared_ptr<sinsp_thread_manager> &thread_manager,
                           const std::shared_ptr<sinsp_usergroup_manager> &usergroup_manager,
                           const std::shared_ptr<sinsp_stats_v2> &sinsp_stats_v2,
                           sinsp_observer *const &observer,
                           sinsp_evt &tmp_evt,
                           scap_platform *const &scap_platform):
        m_sinsp_mode{sinsp_mode},
        m_machine_info{machine_info},
        m_event_sources{event_sources},
        m_syscall_event_source_idx{syscall_event_source_idx},
        m_network_interfaces{network_interfaces},
        m_hostname_and_port_resolution_enabled{hostname_and_port_resolution_enabled},
        m_threadinfo_factory{threadinfo_factory},
        m_fdinfo_factory{fdinfo_factory},
        m_input_plugin{input_plugin},
        m_large_envs_enabled{large_envs_enabled},
        m_plugin_manager{plugin_manager},
        m_thread_manager{thread_manager},
        m_usergroup_manager{usergroup_manager},
        m_sinsp_stats_v2{sinsp_stats_v2},
        m_observer{observer},
        m_tmp_evt{tmp_evt},
        m_scap_platform{scap_platform} {}

sinsp_parser::~sinsp_parser() {
	while(!m_tmp_events_buffer.empty()) {
		auto ptr = m_tmp_events_buffer.top();
		free(ptr);
		m_tmp_events_buffer.pop();
	}
}

void sinsp_parser::set_track_connection_status(bool enabled) {
	m_track_connection_status = enabled;
}

///////////////////////////////////////////////////////////////////////////////
// PROCESSING ENTRY POINT
///////////////////////////////////////////////////////////////////////////////
void sinsp_parser::process_event(sinsp_evt &evt, sinsp_parser_verdict &verdict) {
	const uint16_t etype = evt.get_scap_evt()->type;

	//
	// Route the event to the proper function
	//
	switch(etype) {
	case PPME_SYSCALL_OPEN_E:
	case PPME_SYSCALL_CREAT_E:
	case PPME_SYSCALL_OPENAT_E:
	case PPME_SYSCALL_OPENAT_2_E:
	case PPME_SYSCALL_OPENAT2_E:
	case PPME_SYSCALL_EXECVE_19_E:
	case PPME_SYSCALL_EXECVEAT_E:
		store_event(evt);
		break;
	case PPME_SYSCALL_READ_X:
	case PPME_SYSCALL_WRITE_X:
	case PPME_SOCKET_RECV_X:
	case PPME_SOCKET_SEND_X:
	case PPME_SOCKET_RECVFROM_X:
	case PPME_SOCKET_RECVMSG_X:
	case PPME_SOCKET_RECVMMSG_X:
	case PPME_SOCKET_SENDTO_X:
	case PPME_SOCKET_SENDMSG_X:
	case PPME_SOCKET_SENDMMSG_X:
	case PPME_SYSCALL_READV_X:
	case PPME_SYSCALL_WRITEV_X:
	case PPME_SYSCALL_PREAD_X:
	case PPME_SYSCALL_PWRITE_X:
	case PPME_SYSCALL_PREADV_X:
	case PPME_SYSCALL_PWRITEV_X:
		parse_rw_exit(evt, verdict);
		break;
	case PPME_SYSCALL_SENDFILE_X:
		parse_sendfile_exit(evt, verdict);
		break;
	case PPME_SYSCALL_OPEN_X:
	case PPME_SYSCALL_CREAT_X:
	case PPME_SYSCALL_OPENAT_2_X:
	case PPME_SYSCALL_OPENAT2_X:
	case PPME_SYSCALL_OPEN_BY_HANDLE_AT_X:
		parse_open_openat_creat_exit(evt);
		break;
	case PPME_SYSCALL_FCHMOD_X:
	case PPME_SYSCALL_FCHOWN_X:
		parse_fchmod_fchown_exit(evt);
		break;
	case PPME_SYSCALL_OPENAT_X:
		parse_fspath_related_exit(evt);
		parse_open_openat_creat_exit(evt);
		break;
	case PPME_SYSCALL_UNSHARE_X:
	case PPME_SYSCALL_SETNS_X:
		parse_unshare_setns_exit(evt);
		break;
	case PPME_SYSCALL_MEMFD_CREATE_X:
		parse_memfd_create_exit(evt, SCAP_FD_MEMFD);
		break;
	case PPME_SYSCALL_CLONE_20_X:
	case PPME_SYSCALL_FORK_20_X:
	case PPME_SYSCALL_VFORK_20_X:
	case PPME_SYSCALL_CLONE3_X:
		parse_clone_exit(evt, verdict);
		break;
	case PPME_SYSCALL_PIDFD_OPEN_X:
		parse_pidfd_open_exit(evt);
		break;
	case PPME_SYSCALL_PIDFD_GETFD_X:
		parse_pidfd_getfd_exit(evt);
		break;
	case PPME_SYSCALL_EXECVE_19_X:
	case PPME_SYSCALL_EXECVEAT_X:
		parse_execve_exit(evt, verdict);
		break;
	case PPME_PROCEXIT_E:
	case PPME_PROCEXIT_1_E:
		parse_thread_exit(evt, verdict);
		break;
	case PPME_SYSCALL_PIPE_X:
	case PPME_SYSCALL_PIPE2_X:
		parse_pipe_exit(evt);
		break;
	case PPME_SOCKET_SOCKET_X:
		parse_socket_exit(evt);
		break;
	case PPME_SOCKET_BIND_X:
		parse_bind_exit(evt, verdict);
		break;
	case PPME_SOCKET_CONNECT_E:
		parse_connect_enter(evt);
		break;
	case PPME_SOCKET_CONNECT_X:
		parse_connect_exit(evt, verdict);
		break;
	case PPME_SOCKET_ACCEPT_X:
	case PPME_SOCKET_ACCEPT_5_X:
	case PPME_SOCKET_ACCEPT4_X:
	case PPME_SOCKET_ACCEPT4_5_X:
	case PPME_SOCKET_ACCEPT4_6_X:
		parse_accept_exit(evt, verdict);
		break;
	case PPME_SYSCALL_CLOSE_X:
		parse_close_exit(evt, verdict);
		break;
	case PPME_SYSCALL_FCNTL_X:
		parse_fcntl_exit(evt);
		break;
	case PPME_SYSCALL_EVENTFD_X:
	case PPME_SYSCALL_EVENTFD2_X:
		parse_eventfd_eventfd2_exit(evt);
		break;
	case PPME_SYSCALL_CHDIR_X:
		parse_chdir_exit(evt);
		break;
	case PPME_SYSCALL_FCHDIR_X:
		parse_fchdir_exit(evt);
		break;
	case PPME_SYSCALL_GETCWD_X:
		parse_getcwd_exit(evt);
		break;
	case PPME_SOCKET_SHUTDOWN_X:
		parse_shutdown_exit(evt, verdict);
		break;
	case PPME_SYSCALL_DUP_X:
	case PPME_SYSCALL_DUP_1_X:
	case PPME_SYSCALL_DUP2_X:
	case PPME_SYSCALL_DUP3_X:
		parse_dup_exit(evt, verdict);
		break;
	case PPME_SYSCALL_SIGNALFD_X:
	case PPME_SYSCALL_SIGNALFD4_X:
		parse_single_param_fd_exit(evt, SCAP_FD_SIGNALFD);
		break;
	case PPME_SYSCALL_TIMERFD_CREATE_X:
		parse_single_param_fd_exit(evt, SCAP_FD_TIMERFD);
		break;
	case PPME_SYSCALL_INOTIFY_INIT_X:
	case PPME_SYSCALL_INOTIFY_INIT1_X:
		parse_single_param_fd_exit(evt, SCAP_FD_INOTIFY);
		break;
	case PPME_SYSCALL_BPF_2_X:
		parse_single_param_fd_exit(evt, SCAP_FD_BPF);
		break;
	case PPME_SYSCALL_USERFAULTFD_X:
		parse_single_param_fd_exit(evt, SCAP_FD_USERFAULTFD);
		break;
	case PPME_SYSCALL_IO_URING_SETUP_X:
		parse_single_param_fd_exit(evt, SCAP_FD_IOURING);
		break;
	case PPME_SYSCALL_EPOLL_CREATE_X:
	case PPME_SYSCALL_EPOLL_CREATE1_X:
		parse_single_param_fd_exit(evt, SCAP_FD_EVENTPOLL);
		break;
	case PPME_SYSCALL_GETRLIMIT_X:
	case PPME_SYSCALL_SETRLIMIT_X:
		parse_getrlimit_setrlimit_exit(evt);
		break;
	case PPME_SYSCALL_PRLIMIT_X:
		parse_prlimit_exit(evt);
		break;
	case PPME_SOCKET_SOCKETPAIR_X:
		parse_socketpair_exit(evt);
		break;
	case PPME_SCHEDSWITCH_1_E:
	case PPME_SCHEDSWITCH_6_E:
		parse_context_switch(evt);
		break;
	case PPME_SYSCALL_BRK_4_X:
	case PPME_SYSCALL_MMAP_X:
	case PPME_SYSCALL_MMAP2_X:
	case PPME_SYSCALL_MUNMAP_X:
		parse_brk_mmap_mmap2_munmap__exit(evt);
		break;
	case PPME_SYSCALL_SETRESUID_X:
		parse_setresuid_exit(evt);
		break;
	case PPME_SYSCALL_SETREUID_X:
		parse_setreuid_exit(evt);
		break;
	case PPME_SYSCALL_SETRESGID_X:
		parse_setresgid_exit(evt);
		break;
	case PPME_SYSCALL_SETREGID_X:
		parse_setregid_exit(evt);
		break;
	case PPME_SYSCALL_SETUID_X:
		parse_setuid_exit(evt);
		break;
	case PPME_SYSCALL_SETGID_X:
		parse_setgid_exit(evt);
		break;
	case PPME_CPU_HOTPLUG_E:
		parse_cpu_hotplug_enter(evt);
		break;
	case PPME_SYSCALL_CHROOT_X:
		parse_chroot_exit(evt);
		break;
	case PPME_SYSCALL_SETSID_X:
		parse_setsid_exit(evt);
		break;
	case PPME_SOCKET_GETSOCKOPT_X:
		if(evt.get_num_params() > 0) {
			parse_getsockopt_exit(evt, verdict);
		}
		break;
	case PPME_SYSCALL_CAPSET_X:
		parse_capset_exit(evt);
		break;
	case PPME_USER_ADDED_E:
	case PPME_USER_DELETED_E:
		parse_user_evt(evt);
		break;
	case PPME_GROUP_ADDED_E:
	case PPME_GROUP_DELETED_E:
		parse_group_evt(evt);
		break;
	case PPME_SYSCALL_PRCTL_X:
		parse_prctl_exit_event(evt);
		break;
	default:
		break;
	}

	// Check to see if the name changed as a side-effect of
	// parsing this event. Try to avoid the overhead of a string
	// compare for every event.
	if(evt.get_fd_info()) {
		evt.set_fdinfo_name_changed(evt.get_fd_info()->m_name != evt.get_fd_info()->m_oldname);
	}
}

void sinsp_parser::event_cleanup(sinsp_evt &evt) {
	if(evt.get_direction() == SCAP_ED_OUT && evt.get_tinfo() &&
	   evt.get_tinfo()->get_last_event_data()) {
		free_event_buffer(evt.get_tinfo()->get_last_event_data());
		evt.get_tinfo()->set_last_event_data(nullptr);
		evt.get_tinfo()->set_lastevent_data_validity(false);
	}
}

///////////////////////////////////////////////////////////////////////////////
// HELPERS
///////////////////////////////////////////////////////////////////////////////

/*!
 * \brief Indicate if the event is a clone or a clone3 exit event.
 */
static bool is_clone_exit_event(const uint16_t evt_type) {
	return evt_type == PPME_SYSCALL_CLONE_20_X || evt_type == PPME_SYSCALL_CLONE3_X;
}

/*!
 * \brief Indicate if the event is a fork or a vfork exit event.
 */
static bool is_fork_exit_event(const uint16_t evt_type) {
	return evt_type == PPME_SYSCALL_FORK_20_X || evt_type == PPME_SYSCALL_VFORK_20_X;
}

static bool is_procexit_event(const uint16_t evt_type) {
	return evt_type == PPME_PROCEXIT_E || evt_type == PPME_PROCEXIT_1_E;
}

static bool is_schedswitch_event(const uint16_t evt_type) {
	return evt_type == PPME_SCHEDSWITCH_1_E || evt_type == PPME_SCHEDSWITCH_6_E;
}

/*!
 * \brief Indicate if it is allowed to query the operating system for retrieving thread information.
 * If it is an exit clone event or a scheduler event (many kernel thread), it is not needed to query
 * the OS.
 */
static bool can_query_os_for_thread_info(const uint16_t evt_type) {
	// If we received a `procexit` event it means that the process is dead in the kernel, and
	// querying for thread information would generate fake entries.
	return !(is_clone_exit_event(evt_type) || is_fork_exit_event(evt_type) ||
	         is_schedswitch_event(evt_type) || is_procexit_event(evt_type));
}

//
// Called before starting the parsing.
// Returns false in case of issues resetting the state.
//
bool sinsp_parser::reset(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	uint16_t etype = evt.get_type();
	// Before anything can happen, the event needs to be initialized.
	evt.init();

	uint32_t plugin_id = 0;
	if(evt.get_type() == PPME_PLUGINEVENT_E || evt.get_type() == PPME_ASYNCEVENT_E) {
		// Note: async events can potentially encode a non-zero plugin ID to indicate that they've
		// been produced by a plugin with a specific event source. If an async event has a zero
		// plugin ID, then we can assume it being of the "syscall" source. On the other hand, plugin
		// events are not allowed to have a zero plugin ID, so we should be ok on that front.
		plugin_id = evt.get_param(0)->as<uint32_t>();
	}

	if(plugin_id != 0) {
		bool plugin_found = false;
		const auto src_idx = m_plugin_manager->source_idx_by_plugin_id(plugin_id, plugin_found);
		if(!plugin_found) {
			evt.set_source_idx(sinsp_no_event_source_idx);
			evt.set_source_name(sinsp_no_event_source_name);
		} else {
			evt.set_source_idx(src_idx);
			evt.set_source_name(m_event_sources[src_idx].c_str());
		}
	} else {
		// Every other event falls under the "syscall" event source umbrella.
		evt.set_source_idx(m_syscall_event_source_idx);
		evt.set_source_name(m_syscall_event_source_idx != sinsp_no_event_source_idx
		                            ? sinsp_syscall_event_source_name
		                            : sinsp_no_event_source_name);
	}

	evt.set_fdinfo_ref(nullptr);
	evt.set_fd_info(nullptr);
	evt.set_errorcode(0);

	// Ignore events with EF_SKIPPARSERESET flag.
	if(const auto eflags = evt.get_info_flags(); eflags & EF_SKIPPARSERESET) {
		sinsp_threadinfo *tinfo = nullptr;
		if(etype == PPME_PROCINFO_E) {
			tinfo = m_thread_manager->get_thread_ref(evt.get_scap_evt()->tid, false, false).get();
		}
		evt.set_tinfo(tinfo);
		return false;
	}

	// todo(jasondellaluce): should we do this for all meta-events in general?
	if(etype == PPME_CONTAINER_JSON_E || etype == PPME_CONTAINER_JSON_2_E ||
	   etype == PPME_USER_ADDED_E || etype == PPME_USER_DELETED_E || etype == PPME_GROUP_ADDED_E ||
	   etype == PPME_GROUP_DELETED_E || etype == PPME_PLUGINEVENT_E || etype == PPME_ASYNCEVENT_E) {
		// Note: still managing container events cases. They might still be present in existing scap
		// files, even if they are then parsed by the container plugin.
		evt.set_tinfo(nullptr);
		return true;
	}

	const auto tid = evt.get_scap_evt()->tid;
	const bool query_os = can_query_os_for_thread_info(etype);
	const auto tinfo = m_thread_manager->get_thread_ref(tid, query_os, false).get();

	evt.set_tinfo(tinfo);

	if(is_schedswitch_event(etype)) {
		return false;
	}

	if(!tinfo) {
		if(is_clone_exit_event(etype) || is_fork_exit_event(etype)) {
			if(m_sinsp_stats_v2 != nullptr) {
				m_sinsp_stats_v2->m_n_failed_thread_lookups--;
			}
		}
		return false;
	}

	if(query_os) {
		tinfo->m_flags |= PPM_CL_ACTIVE;
	}

	// todo!: at the end of we work we should remove the enter/exit distinction and ideally we
	//   should set the fdinfos directly here and return if they are not present.
	if(PPME_IS_ENTER(etype)) {
		tinfo->m_lastevent_fd = -1;
		tinfo->set_lastevent_type(etype);

		if(evt.uses_fd()) {
			const int fd_location = get_enter_event_fd_location(static_cast<ppm_event_code>(etype));
			ASSERT(evt.get_param_info(fd_location)->type == PT_FD);
			tinfo->m_lastevent_fd = evt.get_param(fd_location)->as<int64_t>();
			evt.set_fd_info(tinfo->get_fd(tinfo->m_lastevent_fd));
		}

		tinfo->m_latency = 0;
		tinfo->m_last_latency_entertime = evt.get_ts();
		return true;
	}

	//
	// Handling section for exit events.
	//

	//
	// event latency
	//
	if(tinfo->m_last_latency_entertime != 0) {
		tinfo->m_latency = evt.get_ts() - tinfo->m_last_latency_entertime;
		ASSERT(static_cast<int64_t>(tinfo->m_latency) >= 0);
	}

	if(etype == PPME_SYSCALL_EXECVE_19_X &&
	   tinfo->get_lastevent_type() == PPME_SYSCALL_EXECVEAT_E) {
		tinfo->set_lastevent_data_validity(true);
	} else if(etype == tinfo->get_lastevent_type() + 1) {
		tinfo->set_lastevent_data_validity(true);
	} else {
		tinfo->set_lastevent_data_validity(false);
		// We cannot be sure that the lastevent_fd is something valid, it could be the socket of
		// the previous `socket` syscall, or it could be something completely unrelated, for now
		// we don't trust it in any case.
		tinfo->m_lastevent_fd = -1;
	}

	//
	// Error detection logic
	//
	if(evt.has_return_value()) {
		if(const int64_t res = evt.get_syscall_return_value(); res < 0) {
			evt.set_errorcode(-static_cast<int32_t>(res));
		}
	}

	if(!evt.uses_fd()) {
		return true;
	}

	//
	// Handling section for events using FDs.
	//

	// sendmmsg and recvmmsg send all data in the exit event, fd included.
	if((etype == PPME_SOCKET_SENDMMSG_X || etype == PPME_SOCKET_RECVMMSG_X) &&
	   evt.get_num_params() > 0) {
		tinfo->m_lastevent_fd = evt.get_param(1)->as<int64_t>();
	}

	// todo!: this should become the unique logic when we'll disable the enter events.
	if(tinfo->m_lastevent_fd == -1) {
		if(const int fd_location = get_exit_event_fd_location(static_cast<ppm_event_code>(etype));
		   fd_location != -1) {
			const auto fd_param = evt.get_param(fd_location);
			// It is possible that the fd_param is empty
			if(!fd_param->empty()) {
				tinfo->m_lastevent_fd = fd_param->as<int64_t>();
			}
		}
	}

	const auto fdinfo = tinfo->get_fd(tinfo->m_lastevent_fd);
	evt.set_fd_info(fdinfo);
	if(fdinfo == nullptr) {
		return false;
	}

	if(evt.get_errorcode() != 0 && m_observer) {
		m_observer->on_error(&evt);
	}

	return true;
}

void sinsp_parser::store_event(sinsp_evt &evt) {
	if(evt.get_tinfo() == nullptr) {
		//
		// No thread in the table. We won't store this event, which mean that
		// we won't be able to parse the corresponding exit event and we'll have
		// to drop the information it carries.
		//
		if(m_sinsp_stats_v2 != nullptr) {
			m_sinsp_stats_v2->m_n_store_evts_drops++;
		}
		return;
	}

	//
	// Make sure the event data is going to fit
	//
	const uint32_t elen = scap_event_getlen(evt.get_scap_evt());

	if(elen > SP_EVT_BUF_SIZE) {
		ASSERT(false);
		return;
	}

	//
	// Copy the data
	//
	auto tinfo = evt.get_tinfo();
	if(tinfo->get_last_event_data() == nullptr) {
		tinfo->set_last_event_data(reserve_event_buffer());
		if(tinfo->get_last_event_data() == nullptr) {
			throw sinsp_exception("cannot reserve event buffer in sinsp_parser::store_event.");
			return;
		}
	}
	memcpy(tinfo->get_last_event_data(), evt.get_scap_evt(), elen);
	tinfo->set_lastevent_cpuid(evt.get_cpuid());

	if(m_sinsp_stats_v2 != nullptr) {
		m_sinsp_stats_v2->m_n_stored_evts++;
	}
}

bool sinsp_parser::retrieve_enter_event(sinsp_evt &enter_evt, sinsp_evt &exit_evt) const {
	//
	// Make sure there's a valid thread info
	//
	if(!exit_evt.get_tinfo()) {
		return false;
	}

	//
	// Retrieve the copy of the enter event and initialize it
	//
	if(!(exit_evt.get_tinfo()->is_lastevent_data_valid() &&
	     exit_evt.get_tinfo()->get_last_event_data())) {
		//
		// This happen especially at the beginning of trace files, where events
		// can be truncated
		//
		if(m_sinsp_stats_v2 != nullptr) {
			m_sinsp_stats_v2->m_n_retrieve_evts_drops++;
		}
		return false;
	}

	enter_evt.init_from_raw(exit_evt.get_tinfo()->get_last_event_data(),
	                        exit_evt.get_tinfo()->get_lastevent_cpuid());

	/* The `execveat` syscall is a wrapper of `execve`, when the call
	 * succeeds the event returned is simply an `execve` exit event.
	 * So if an `execveat` is correctly executed we will have, a
	 * `PPME_SYSCALL_EXECVEAT_E` as enter event and a
	 * `PPME_SYSCALL_EXECVE_..._X` as exit one. So when we retrieve
	 * the enter event in the `parse_execve_exit` method  we cannot
	 * only check for the same syscall event, so `PPME_SYSCALL_EXECVE_..._E`,
	 * we have also to check for the `PPME_SYSCALL_EXECVEAT_E`.
	 */
	if(exit_evt.get_type() == PPME_SYSCALL_EXECVE_19_X &&
	   enter_evt.get_type() == PPME_SYSCALL_EXECVEAT_E) {
		if(m_sinsp_stats_v2 != nullptr) {
			m_sinsp_stats_v2->m_n_retrieved_evts++;
		}
		return true;
	}

	//
	// Make sure that we're using the right enter event, to prevent inconsistencies when events
	// are dropped
	//
	if(enter_evt.get_type() != (exit_evt.get_type() - 1)) {
		// ASSERT(false);
		exit_evt.get_tinfo()->set_lastevent_data_validity(false);
		if(m_sinsp_stats_v2 != nullptr) {
			m_sinsp_stats_v2->m_n_retrieve_evts_drops++;
		}
		return false;
	}
	if(m_sinsp_stats_v2 != nullptr) {
		m_sinsp_stats_v2->m_n_retrieved_evts++;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// PARSERS
///////////////////////////////////////////////////////////////////////////////

void sinsp_parser::parse_clone_exit_caller(sinsp_evt &evt,
                                           sinsp_parser_verdict &verdict,
                                           const int64_t child_tid) const {
	int64_t caller_tid = evt.get_tid();

	/* We have a collision when we force a removal in the thread table because
	 * we have 2 entries with the same tid.
	 */
	int64_t tid_collision = -1;

	/* By default we have a valid caller. `valid_caller==true` means that we can
	 * use the caller info to fill some fields of the child, `valid_caller==false`
	 * means that we will use some info about the child to fill the caller thread info.
	 * We need the caller because it is the most reliable source of info for the child.
	 */
	bool valid_caller = true;

	/* The clone caller exit event has 2 main purposes:
	 * 1. enrich the caller thread info with fresh info or create a new one if it was not there.
	 * 2. create a new thread info for the child if necessary. (resilience to event drops)
	 */

	/*=============================== ENRICH/CREATE ESSENTIAL CALLER STATE
	 * ===========================*/

	/* Let's see if we have some info regarding the caller */
	auto caller_tinfo = m_thread_manager->get_thread_ref(caller_tid, true);

	/* This happens only if we reach the max entries in our table otherwise we should obtain a new
	 * fresh empty thread info to populate even if we are not able to recover any information! If
	 * `caller_tinfo == nullptr` we return, we won't have enough space for the child in the table!
	 */
	if(caller_tinfo == nullptr) {
		/* Invalidate the thread info associated with this event */
		evt.set_tinfo(nullptr);
		return;
	}

	/* We have an invalid thread:
	 * 1. The process is dead and we are not able to find it in /proc.
	 * 2. We have done too much /proc scan and we cannot recover it.
	 */
	if(caller_tinfo->is_invalid()) {
		/* In case of invalid thread we enrich it with fresh info and we obtain a sort of valid
		 * thread info */
		valid_caller = false;

		/* pid. */
		caller_tinfo->m_pid = evt.get_param(4)->as<int64_t>();

		/* ptid */
		caller_tinfo->m_ptid = evt.get_param(5)->as<int64_t>();

		/* vtid & vpid */
		// If one of these parameters is present, so is for the other ones, so just check the
		// presence of one of them.
		const auto vtid_param = evt.get_param(18);
		const auto vpid_param = evt.get_param(19);
		if(!vpid_param->empty()) {
			caller_tinfo->m_vtid = vtid_param->as<int64_t>();
			caller_tinfo->m_vpid = vpid_param->as<int64_t>();
		} else {
			caller_tinfo->m_vtid = caller_tid;
			caller_tinfo->m_vpid = -1;
		}

		/* Create thread groups and parenting relationships */
		m_thread_manager->create_thread_dependencies(caller_tinfo);
	}

	/* Update the evt.get_tinfo() of the caller. */
	evt.set_tinfo(caller_tinfo.get());

	/// todo(@Andreagit97): here we could update `comm` `exe` and `args` with fresh info from the
	/// event

	/*=============================== ENRICH/CREATE ESSENTIAL CALLER STATE
	 * ===========================*/

	/*=============================== CHILD IN CONTAINER CASE ===========================*/

	/* Get `flags` to check if we are in a container.
	 * We should never assign these flags to the caller otherwise if the child is a thread
	 * also the caller will be marked as a thread with the `PPM_CL_CLONE_THREAD` flag.
	 */

	const auto flags = evt.get_param(15)->as<uint32_t>();

	/* PPM_CL_CHILD_IN_PIDNS is true when:
	 * - the caller is running into a container and so the child
	 * Please note: if only the child is running into a container
	 * (so when the child is the init process of the new namespace)
	 * this flag is not set
	 *
	 * PPM_CL_CLONE_NEWPID is true when:
	 * - the child is the init process of a new namespace
	 *
	 * PPM_CL_CLONE_PARENT is set by `runc:[0:PARENT]` when it creates
	 * the first process in the new pid namespace. In new sinsp versions
	 * `PPM_CL_CHILD_IN_PIDNS` is enough but in old scap-files where we don't have
	 * this custom flag, we leave the event to the child parser when `PPM_CL_CLONE_PARENT`
	 * is set since we don't know if the new child is in a pid namespace or not.
	 * Moreover when `PPM_CL_CLONE_PARENT` is set `PPM_CL_CLONE_NEWPID` cannot
	 * be set according to the clone manual.
	 *
	 * When `caller_tid != caller_tinfo->m_vtid` is true we are for
	 * sure in a container, and so is the child.
	 * This is not a strict requirement (leave it here for compatibility with old
	 * scap-files)
	 */
	if(flags & PPM_CL_CHILD_IN_PIDNS || flags & PPM_CL_CLONE_NEWPID ||
	   flags & PPM_CL_CLONE_PARENT || caller_tid != caller_tinfo->m_vtid) {
		return;
	}

	/*=============================== CHILD IN CONTAINER CASE ===========================*/

	/*=============================== CHILD ALREADY THERE ===========================*/

	/* See if the child is already there, if yes and it is valid we return immediately */
	sinsp_threadinfo *existing_child_tinfo =
	        m_thread_manager->get_thread_ref(child_tid, false, true).get();
	if(existing_child_tinfo != nullptr) {
		/* If this was an inverted clone, all is fine, we've already taken care
		 * of adding the thread table entry in the child.
		 * Otherwise, we assume that the entry is there because we missed the proc exit event
		 * for a previous thread and we replace the tinfo.
		 */
		if(existing_child_tinfo->m_flags & PPM_CL_CLONE_INVERTED) {
			return;
		} else {
			m_thread_manager->remove_thread(child_tid);
			tid_collision = child_tid;
		}
	}

	/*=============================== CHILD ALREADY THERE ===========================*/

	/* If we come here it means that we need to create the child thread info */

	/*=============================== CREATE CHILD ===========================*/

	/* Allocate the new thread info and initialize it.
	 * We avoid `malloc` here and get the item from a preallocated list.
	 */
	auto child_tinfo = m_threadinfo_factory.create();

	/* Initialise last exec time to zero (can be overridden in the case of a
	 * thread clone)
	 */
	child_tinfo->m_lastexec_ts = 0;

	/* flags */
	child_tinfo->m_flags = flags;

	/* tid */
	child_tinfo->m_tid = child_tid;

	/* Thread-leader case */
	if(!(child_tinfo->m_flags & PPM_CL_CLONE_THREAD)) {
		/* We populate fdtable, cwd and env only if we are
		 * a new leader thread, all not leader threads will use the same information
		 * of the main thread.
		 */
		if(valid_caller) {
			/* Copy the fd list:
			 * XXX this is a gross oversimplification that will need to be fixed.
			 * What we do is: if the child is NOT a thread, we copy all the parent fds.
			 * The right thing to do is looking at PPM_CL_CLONE_FILES, but there are
			 * syscalls like open and pipe2 that can override PPM_CL_CLONE_FILES with the O_CLOEXEC
			 * flag
			 */
			sinsp_fdtable *fd_table_ptr = caller_tinfo->get_fd_table();
			if(fd_table_ptr != nullptr) {
				child_tinfo->get_fdtable().clear();
				child_tinfo->get_fdtable().set_tid(child_tinfo->m_tid);
				fd_table_ptr->const_loop([&child_tinfo](int64_t fd, const sinsp_fdinfo &info) {
					/* Track down that those are cloned fds */
					auto newinfo = info.clone();
					newinfo->set_is_cloned();
					child_tinfo->get_fdtable().add(fd, std::move(newinfo));
					return true;
				});

				/* It's important to reset the cache of the child thread, to prevent it from
				 * referring to an element in the parent's table.
				 */
				child_tinfo->get_fdtable().reset_cache();
			} else {
				/* This should never happen */
				libsinsp_logger()->format(sinsp_logger::SEV_DEBUG,
				                          "cannot get fd table in sinsp_parser::parse_clone_exit.");
				ASSERT(false);
			}

			/* Not a thread, copy cwd */
			child_tinfo->set_cwd(caller_tinfo->get_cwd());

			/* Not a thread, copy env */
			child_tinfo->m_env = caller_tinfo->m_env;
		}

		/* Create info about the thread group */

		/* pid */
		child_tinfo->m_pid = child_tinfo->m_tid;

		/* The child parent is the calling process */
		child_tinfo->m_ptid = caller_tinfo->m_tid;
	} else /* Simple thread case */
	{
		/* pid */
		child_tinfo->m_pid = caller_tinfo->m_pid;

		/* ptid */
		/* The parent is the parent of the calling process */
		child_tinfo->m_ptid = caller_tinfo->m_ptid;

		/* Please note this is not the right behavior, it is something we do to be compliant with
		 * `/proc` scan.
		 *
		 * In our approximation threads will never have their `fdtable` they will use the main
		 * thread one, for this reason, we keep the main thread alive until we have some threads in
		 * the group.
		 */
		child_tinfo->m_flags |= PPM_CL_CLONE_FILES;

		/* If we are a new thread we keep the same lastexec time of the main thread
		 * If the caller is invalid we are re-initializing this value to 0 again.
		 */
		child_tinfo->m_lastexec_ts = caller_tinfo->m_lastexec_ts;
	}

	/* We are not in a container otherwise we should never reach this point.
	 * We have a previous check in this parser!
	 */

	/* vtid */
	child_tinfo->m_vtid = child_tinfo->m_tid;

	/* vpid */
	child_tinfo->m_vpid = child_tinfo->m_pid;

	/* exe */
	child_tinfo->m_exe = evt.get_param(1)->as<std::string>();

	/* args */
	child_tinfo->set_args(evt.get_param(2)->as<std::vector<std::string>>());

	/* comm */
	if(const auto comm_param = evt.get_param(13); !comm_param->empty()) {
		child_tinfo->m_comm = comm_param->as<std::string>();
	} else {
		child_tinfo->m_comm = child_tinfo->m_exe;
	}

	/* fdlimit */
	child_tinfo->m_fdlimit = evt.get_param(7)->as<int64_t>();

	/* Generic memory info */
	// If one of these parameters is present, so is for the other ones, so just check the
	// presence of one of them.
	const auto pgft_maj_param = evt.get_param(8);
	const auto pgft_min_param = evt.get_param(9);
	const auto vm_size_param = evt.get_param(10);
	const auto vm_rss_param = evt.get_param(11);
	const auto vm_swap_param = evt.get_param(12);
	if(!vm_swap_param->empty()) {
		child_tinfo->m_pfmajor = pgft_maj_param->as<uint64_t>();
		child_tinfo->m_pfminor = pgft_min_param->as<uint64_t>();
		child_tinfo->m_vmsize_kb = vm_size_param->as<uint32_t>();
		child_tinfo->m_vmrss_kb = vm_rss_param->as<uint32_t>();
		child_tinfo->m_vmswap_kb = vm_swap_param->as<uint32_t>();
	}

	/* uid */
	const auto uid = evt.get_param(16)->as<int32_t>();
	child_tinfo->set_user(uid, must_notify_thread_user_update());

	/* gid */
	const auto gid = evt.get_param(17)->as<int32_t>();
	child_tinfo->set_group(gid, must_notify_thread_group_update());

	// Set cgroups
	if(const auto cgroups_param = evt.get_param(14); !cgroups_param->empty()) {
		child_tinfo->set_cgroups(cgroups_param->as<std::vector<std::string>>());
	}

	/* Initialize the thread clone time */
	child_tinfo->m_clone_ts = evt.get_ts();

	/* Get pid namespace start ts - convert monotonic time in ns to epoch ts */
	child_tinfo->m_pidns_init_start_ts = m_machine_info->boot_ts_epoch;

	/* Take some further info from the caller */
	if(valid_caller) {
		/* We should trust the info we obtain from the caller, if it is valid */
		child_tinfo->set_exepath(std::string(caller_tinfo->m_exepath));

		child_tinfo->m_exe_writable = caller_tinfo->m_exe_writable;

		child_tinfo->m_exe_upper_layer = caller_tinfo->m_exe_upper_layer;

		child_tinfo->m_exe_lower_layer = caller_tinfo->m_exe_lower_layer;

		child_tinfo->m_exe_from_memfd = caller_tinfo->m_exe_from_memfd;

		child_tinfo->m_root = caller_tinfo->m_root;

		child_tinfo->m_sid = caller_tinfo->m_sid;

		child_tinfo->m_vpgid = caller_tinfo->m_vpgid;

		child_tinfo->m_pgid = caller_tinfo->m_pgid;

		child_tinfo->m_tty = caller_tinfo->m_tty;

		child_tinfo->set_loginuid(caller_tinfo->m_loginuid);

		child_tinfo->m_cap_permitted = caller_tinfo->m_cap_permitted;

		child_tinfo->m_cap_inheritable = caller_tinfo->m_cap_inheritable;

		child_tinfo->m_cap_effective = caller_tinfo->m_cap_effective;

		child_tinfo->m_exe_ino = caller_tinfo->m_exe_ino;

		child_tinfo->m_exe_ino_ctime = caller_tinfo->m_exe_ino_ctime;

		child_tinfo->m_exe_ino_mtime = caller_tinfo->m_exe_ino_mtime;

		child_tinfo->m_exe_ino_ctime_duration_clone_ts =
		        caller_tinfo->m_exe_ino_ctime_duration_clone_ts;
	} else {
		/* exe */
		caller_tinfo->m_exe = child_tinfo->m_exe;

		/* comm */
		caller_tinfo->m_comm = child_tinfo->m_comm;

		/* args */
		caller_tinfo->set_args(evt.get_param(2)->as<std::vector<std::string>>());
	}

	/*=============================== CREATE CHILD ===========================*/

	/*=============================== ADD THREAD TO THE TABLE ===========================*/

	/* Until we use the shared pointer we need it here, after we can move it at the end */
	auto new_child = m_thread_manager->add_thread(std::move(child_tinfo), false);
	if(!new_child) {
		// note: we expect the thread manager to log a warning already
		return;
	}

	//
	// If there's a listener, add a callback to later invoke it.
	//
	if(m_observer) {
		verdict.add_post_process_cbs(
		        [new_child, tid_collision](sinsp_observer *observer, sinsp_evt *evt) {
			        observer->on_clone(evt, new_child.get(), tid_collision);
		        });
	}

	/* If we had to erase a previous entry for this tid and rebalance the table,
	 * make sure we reinitialize the tinfo pointer for this event, as the thread
	 * generating it might have gone away.
	 */
	if(tid_collision != -1) {
		reset(evt, verdict);
		DBG_SINSP_INFO("tid collision for %" PRIu64 "(%s)",
		               tid_collision,
		               new_child->m_comm.c_str());
	}
	/*=============================== ADD THREAD TO THE TABLE ===========================*/
}

void sinsp_parser::parse_clone_exit_child(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	int64_t child_tid = evt.get_tid();

	int64_t tid_collision = -1;
	bool valid_lookup_thread = true;

	/*=============================== CHILD ALREADY THERE ===========================*/

	/* Before embarking on parsing the event, check if there's already
	 * an entry in the thread table for this process. If there is one, make sure
	 * it was created recently. Otherwise, assume it's an old thread for which
	 * we lost the exit event and remove it from the table.
	 * Please note that the thread info is associated with the event
	 * in `sinsp_parser::reset` method.
	 */
	if(evt.get_tinfo() != nullptr && evt.get_tinfo()->m_clone_ts != 0) {
		if(evt.get_ts() - evt.get_tinfo()->m_clone_ts < CLONE_STALE_TIME_NS) {
			/* This is a valid thread-info, the caller populated it so we
			 * have nothing to do here. Note that if we are in a container the caller
			 * will never generate the child thread-info because it doesn't have
			 * enough info. In all other cases the thread info created by the caller
			 * should be already valid.
			 */
			return;
		}

		/* The info is too old, we remove it and create a new one */
		m_thread_manager->remove_thread(child_tid);
		tid_collision = child_tid;
		evt.set_tinfo(nullptr);
	}

	/*=============================== CHILD ALREADY THERE ===========================*/

	/*=============================== CREATE NEW THREAD-INFO ===========================*/

	/* We take this flow in the following situations:
	 * - clone() returns in the child before then in the caller.
	 *   This usually happens when we use the CAPTURE_SCHED_PROC_FORK logic
	 *   because the child event is generated by the `sched_proc_fork`
	 *   tracepoint. (Default behavior on arm64 and s390x)
	 * - We dropped the clone exit event in the caller.
	 * - The new process lives in a container.
	 */

	/* Allocate the new thread info and initialize it.
	 * We must avoid `malloc` here and get the item from a preallocated list.
	 */
	auto child_tinfo = m_threadinfo_factory.create();

	/* Initialise last exec time to zero (can be overridden in the case of a
	 * thread clone)
	 */
	child_tinfo->m_lastexec_ts = 0;

	/* tid */
	child_tinfo->m_tid = child_tid;

	/* pid */
	child_tinfo->m_pid = evt.get_param(4)->as<int64_t>();

	/* ptid. */
	child_tinfo->m_ptid = evt.get_param(5)->as<int64_t>();

	/* `vtid` and `vpid` */
	// If one of these parameters is present, so is for the other ones, so just check the
	// presence of one of them.
	const auto vtid_param = evt.get_param(18);
	const auto vpid_param = evt.get_param(19);
	if(!vpid_param->empty()) {
		child_tinfo->m_vtid = vtid_param->as<int64_t>();
		child_tinfo->m_vpid = vpid_param->as<int64_t>();
	} else {
		child_tinfo->m_vtid = child_tinfo->m_tid;
		child_tinfo->m_vpid = -1;
	}

	/* flags */
	child_tinfo->m_flags = evt.get_param(15)->as<uint32_t>();

	/* We add this custom `PPM_CL_CLONE_INVERTED` flag.
	 * It means that we received the child event before the caller one and
	 * it will notify the caller that it has to do nothing because we already
	 * populated the thread info in the child.
	 */
	child_tinfo->m_flags |= PPM_CL_CLONE_INVERTED;

	/* Lookup the thread info of the leader thread if we are a new thread while if we are
	 * a new process we copy it from the parent.
	 *
	 * Note that the lookup thread could be different from the caller one!
	 * If they are different we cannot completely trust the info we obtain from lookup thread
	 * becuase they could be stale! For example the caller may have called `prctl` changing its
	 * comm, while the lookup thread still have the old `comm`.
	 */
	int64_t lookup_tid;

	bool is_thread_leader = !(child_tinfo->m_flags & PPM_CL_CLONE_THREAD);
	if(is_thread_leader) {
		/* We need to copy data from the parent */
		lookup_tid = child_tinfo->m_ptid;
	} else {
		/* We need to copy data from the thread leader */
		lookup_tid = child_tinfo->m_pid;

		/* Please note this is not the right behavior, it is something we do to be compliant with
		 * `/proc` scan.
		 *
		 * In our approximation threads will never have their `fdtable` they will use the main
		 * thread one, for this reason, we keep the main thread alive until we have some threads in
		 * the group.
		 */
		child_tinfo->m_flags |= PPM_CL_CLONE_FILES;
	}

	auto lookup_tinfo = m_thread_manager->get_thread_ref(lookup_tid, true);
	/* This happens only if we reach the max entries in our table otherwise we should obtain a new
	 * fresh empty thread info to populate even if we are not able to recover any information! If
	 * `caller_tinfo == nullptr` we return, we won't have enough space for the child in the table!
	 */
	if(lookup_tinfo == nullptr) {
		/* Invalidate the thread_info associated with this event */
		evt.set_tinfo(nullptr);
		return;
	}

	if(lookup_tinfo->is_invalid()) {
		valid_lookup_thread = false;

		if(!is_thread_leader) {
			/* If the main thread was invalid we should be able to recover some info */

			/* pid. */
			/* the new thread pid is the same of the main thread */
			lookup_tinfo->m_pid = child_tinfo->m_pid;

			/* ptid */
			/* the new thread ptid is the same of the main thread */
			lookup_tinfo->m_ptid = child_tinfo->m_ptid;

			/* vpid */
			/* we are in the same thread group, the vpid is the same of the child */
			lookup_tinfo->m_vpid = child_tinfo->m_vpid;

			/* vtid */
			/* we are a main thread so vtid==vpid */
			lookup_tinfo->m_vtid = lookup_tinfo->m_vpid;

			/* Create thread groups and parenting relationships */
			m_thread_manager->create_thread_dependencies(lookup_tinfo);
		}
	}

	/* We need to do this here, in this way we can use this info to populate the lookup thread
	 * if it is invalid.
	 */

	/* exe */
	child_tinfo->m_exe = evt.get_param(1)->as<std::string>();

	/* comm */
	if(const auto comm_param = evt.get_param(13); !comm_param->empty()) {
		child_tinfo->m_comm = comm_param->as<std::string>();
	} else {
		child_tinfo->m_comm = child_tinfo->m_exe;
	}

	/* args */
	child_tinfo->set_args(evt.get_param(2)->as<std::vector<std::string>>());

	if(valid_lookup_thread) {
		/* Please note that these data could be wrong if the lookup thread
		 * is not the caller! for example, if the child is created by a thread
		 * the thread could have different info with respect to the thread leader,
		 * for example `comm` could be different! This is a sort of best effort
		 * enrichment...
		 */

		child_tinfo->set_exepath(std::string(lookup_tinfo->m_exepath));

		child_tinfo->m_exe_writable = lookup_tinfo->m_exe_writable;

		child_tinfo->m_exe_upper_layer = lookup_tinfo->m_exe_upper_layer;

		child_tinfo->m_exe_lower_layer = lookup_tinfo->m_exe_lower_layer;

		child_tinfo->m_exe_from_memfd = lookup_tinfo->m_exe_from_memfd;

		child_tinfo->m_root = lookup_tinfo->m_root;

		child_tinfo->m_sid = lookup_tinfo->m_sid;

		child_tinfo->m_vpgid = lookup_tinfo->m_vpgid;

		child_tinfo->m_pgid = lookup_tinfo->m_pgid;

		child_tinfo->m_tty = lookup_tinfo->m_tty;

		child_tinfo->set_loginuid(lookup_tinfo->m_loginuid);

		child_tinfo->m_cap_permitted = lookup_tinfo->m_cap_permitted;

		child_tinfo->m_cap_inheritable = lookup_tinfo->m_cap_inheritable;

		child_tinfo->m_cap_effective = lookup_tinfo->m_cap_effective;

		child_tinfo->m_exe_ino = lookup_tinfo->m_exe_ino;

		child_tinfo->m_exe_ino_ctime = lookup_tinfo->m_exe_ino_ctime;

		child_tinfo->m_exe_ino_mtime = lookup_tinfo->m_exe_ino_mtime;

		child_tinfo->m_exe_ino_ctime_duration_clone_ts =
		        lookup_tinfo->m_exe_ino_ctime_duration_clone_ts;

		/* We are a new thread leader */
		if(is_thread_leader) {
			/* We populate fdtable, cwd and env only if we are
			 * a new leader thread, all not leader threads will use the same information
			 * of the main thread.
			 */

			/* Copy the fd list:
			 * XXX this is a gross oversimplification that will need to be fixed.
			 * What we do is: if the child is NOT a thread, we copy all the parent fds.
			 * The right thing to do is looking at PPM_CL_CLONE_FILES, but there are
			 * syscalls like open and pipe2 that can override PPM_CL_CLONE_FILES with the O_CLOEXEC
			 * flag
			 */
			sinsp_fdtable *fd_table_ptr = lookup_tinfo->get_fd_table();
			if(fd_table_ptr != nullptr) {
				child_tinfo->get_fdtable().clear();
				child_tinfo->get_fdtable().set_tid(child_tinfo->m_tid);
				fd_table_ptr->const_loop([&child_tinfo](int64_t fd, const sinsp_fdinfo &info) {
					/* Track down that those are cloned fds.
					 * This flag `FLAGS_IS_CLONED` seems to be never used...
					 */
					auto newinfo = info.clone();
					newinfo->set_is_cloned();
					child_tinfo->get_fdtable().add(fd, std::move(newinfo));
					return true;
				});

				/* It's important to reset the cache of the child thread, to prevent it from
				 * referring to an element in the parent's table.
				 */
				child_tinfo->get_fdtable().reset_cache();
			} else {
				/* This should never happen */
				libsinsp_logger()->format(sinsp_logger::SEV_DEBUG,
				                          "cannot get fd table in sinsp_parser::parse_clone_exit.");
				ASSERT(false);
			}

			/* Not a thread, copy cwd */
			child_tinfo->set_cwd(lookup_tinfo->get_cwd());

			/* Not a thread, copy env */
			child_tinfo->m_env = lookup_tinfo->m_env;
		} else {
			/* If we are a new thread we keep the same lastexec time of the main thread */
			child_tinfo->m_lastexec_ts = lookup_tinfo->m_lastexec_ts;
		}
	} else {
		/* Please note that here `comm`, `exe`, ... could be different from our thread, so this is
		 * an approximation */
		if(!is_thread_leader) {
			/* exe */
			lookup_tinfo->m_exe = child_tinfo->m_exe;

			/* comm */
			lookup_tinfo->m_comm = child_tinfo->m_comm;

			/* args */
			lookup_tinfo->set_args(evt.get_param(2)->as<std::vector<std::string>>());
		}
	}

	/* fdlimit */
	child_tinfo->m_fdlimit = evt.get_param(7)->as<int64_t>();

	/* Generic memory info */
	// If one of these parameters is present, so is for the other ones, so just check the
	// presence of one of them.
	const auto pgft_maj_param = evt.get_param(8);
	const auto pgft_min_param = evt.get_param(9);
	const auto vm_size_param = evt.get_param(10);
	const auto vm_rss_param = evt.get_param(11);
	const auto vm_swap_param = evt.get_param(12);
	if(!vm_swap_param->empty()) {
		child_tinfo->m_pfmajor = pgft_maj_param->as<uint64_t>();
		child_tinfo->m_pfminor = pgft_min_param->as<uint64_t>();
		child_tinfo->m_vmsize_kb = vm_size_param->as<uint32_t>();
		child_tinfo->m_vmrss_kb = vm_rss_param->as<uint32_t>();
		child_tinfo->m_vmswap_kb = vm_swap_param->as<uint32_t>();
	}

	/* uid */
	const auto uid = evt.get_param(16)->as<int32_t>();
	child_tinfo->set_user(uid, must_notify_thread_user_update());

	/* gid */
	const auto gid = evt.get_param(17)->as<int32_t>();
	child_tinfo->set_group(gid, must_notify_thread_group_update());

	// Set cgroups
	if(const auto cgroups_param = evt.get_param(14); !cgroups_param->empty()) {
		child_tinfo->set_cgroups(cgroups_param->as<std::vector<std::string>>());
	}

	/* Initialize the thread clone time */
	child_tinfo->m_clone_ts = evt.get_ts();

	/* Get pid namespace start ts - convert monotonic time in ns to epoch ts */
	/* If we are in container! */
	if(child_tinfo->m_flags & PPM_CL_CHILD_IN_PIDNS || child_tinfo->m_flags & PPM_CL_CLONE_NEWPID ||
	   child_tinfo->m_tid != child_tinfo->m_vtid) {
		if(const auto pidns_init_start_ts_param = evt.get_param(20);
		   !pidns_init_start_ts_param->empty()) {
			child_tinfo->m_pidns_init_start_ts =
			        pidns_init_start_ts_param->as<uint64_t>() + m_machine_info->boot_ts_epoch;
		}
	} else {
		child_tinfo->m_pidns_init_start_ts = m_machine_info->boot_ts_epoch;
	}

	/*=============================== CREATE NEW THREAD-INFO ===========================*/

	/* Add the new thread to the table */
	auto new_child = m_thread_manager->add_thread(std::move(child_tinfo), false);
	if(!new_child) {
		// note: we expect the thread manager to log a warning already
		evt.set_tinfo(nullptr);
		return;
	}

	/* Update the evt.get_tinfo() of the child.
	 * We update it here, in this way the `on_clone`
	 * callback will use updated info.
	 */
	evt.set_tinfo(new_child.get());

	//
	// If there's a listener, add a callback to later invoke it.
	//
	if(m_observer) {
		verdict.add_post_process_cbs(
		        [new_child, tid_collision](sinsp_observer *observer, sinsp_evt *evt) {
			        observer->on_clone(evt, new_child.get(), tid_collision);
		        });
	}

	/* If we had to erase a previous entry for this tid and rebalance the table,
	 * make sure we reinitialize the child_tinfo pointer for this event, as the thread
	 * generating it might have gone away.
	 */
	if(tid_collision != -1) {
		reset(evt, verdict);
		/* Right now we have collisions only on the clone() caller */
		DBG_SINSP_INFO("tid collision for %" PRIu64 "(%s)",
		               tid_collision,
		               new_child->m_comm.c_str());
	}

	/*=============================== CREATE NEW THREAD-INFO ===========================*/
}

void sinsp_parser::parse_clone_exit(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	int64_t childtid = evt.get_syscall_return_value();
	/* Please note that if the child is in a namespace different from the init one
	 * we should never use this `childtid` otherwise we will use a thread id referred to
	 * an internal namespace and not to the init one!
	 */
	if(childtid < 0) {
		//
		// clone() failed. Do nothing and keep going.
		//
		return;
	} else if(childtid == 0) {
		parse_clone_exit_child(evt, verdict);
	} else {
		parse_clone_exit_caller(evt, verdict, childtid);
	}
	return;
}

void sinsp_parser::parse_execve_exit(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	/* Some architectures like s390x send a `PPME_SYSCALL_EXECVEAT_X` exit event
	 * when the `execveat` syscall succeeds, for this reason, we need to manage also
	 * this event in the parser.
	 */
	if(evt.get_syscall_return_value() < 0) {
		return;
	}

	//
	// We get here when `execve` or `execveat` return. The thread has already been added by a
	// previous fork or clone, and we just update the entry with the new information.
	//
	if(evt.get_tinfo() == nullptr) {
		//
		// No thread to update?
		// We probably missed the start event, so we will just do nothing
		//
		// fprintf(stderr, "comm = %s, args =
		// %s\n",evt.get_param(1)->data()),evt.get_param(1)->data())); ASSERT(false);
		return;
	}

	/* In some corner cases an execve is thrown by a secondary thread when
	 * the main thread is already dead. In these cases the secondary thread
	 * will become a main thread (it will change its tid) and here we will have
	 * an execve exit event called by a main thread that is dead.
	 * What we need to do is to set the main thread as alive again and then
	 * a new PROC_EXIT event will kill it again.
	 * This is what happens with `stress-ng --exec`.
	 */
	if(evt.get_tinfo()->is_dead()) {
		evt.get_tinfo()->resurrect_thread();
	}

	// Set the exe.
	auto parinfo = evt.get_param(1);
	evt.get_tinfo()->m_exe = parinfo->as<std::string>();
	evt.get_tinfo()->m_lastexec_ts = evt.get_ts();

	// Set the comm.
	if(const auto comm_param = evt.get_param(13); !comm_param->empty()) {
		evt.get_tinfo()->m_comm = comm_param->as<std::string>();
	} else {
		// Old trace files didn't have comm, so just set it to exe.
		evt.get_tinfo()->m_comm = evt.get_tinfo()->m_exe;
	}

	// Set the command arguments.
	evt.get_tinfo()->set_args(evt.get_param(2)->as<std::vector<std::string>>());

	// Set the pid.
	evt.get_tinfo()->m_pid = evt.get_param(4)->as<uint64_t>();

	//
	// In case this thread is a fake entry,
	// try to at least patch the parent, since
	// we have it from the execve event
	//
	if(evt.get_tinfo()->is_invalid()) {
		evt.get_tinfo()->m_ptid = evt.get_param(5)->as<uint64_t>();

		/* We are not in a namespace we recover also vtid and vpid */
		if((evt.get_tinfo()->m_flags & PPM_CL_CHILD_IN_PIDNS) == 0) {
			evt.get_tinfo()->m_vtid = evt.get_tinfo()->m_tid;
			evt.get_tinfo()->m_vpid = evt.get_tinfo()->m_pid;
		}

		auto tinfo = m_thread_manager->get_thread_ref(evt.get_tinfo()->m_tid, false);
		/* Create thread groups and parenting relationships */
		m_thread_manager->create_thread_dependencies(tinfo);
	}

	// Set the fdlimit.
	evt.get_tinfo()->m_fdlimit = evt.get_param(7)->as<int64_t>();

	// If one the following parameters is present, so is for the other ones, so just check the
	// presence of one of them.
	const auto pgft_maj_param = evt.get_param(8);
	const auto pgft_min_param = evt.get_param(9);
	const auto vm_size_param = evt.get_param(10);
	const auto vm_rss_param = evt.get_param(11);
	const auto vm_swap_param = evt.get_param(12);
	if(!vm_swap_param->empty()) {
		evt.get_tinfo()->m_pfmajor = pgft_maj_param->as<uint64_t>();
		evt.get_tinfo()->m_pfminor = pgft_min_param->as<uint64_t>();
		evt.get_tinfo()->m_vmsize_kb = vm_size_param->as<uint32_t>();
		evt.get_tinfo()->m_vmrss_kb = vm_rss_param->as<uint32_t>();
		evt.get_tinfo()->m_vmswap_kb = vm_swap_param->as<uint32_t>();
	}

	// Set the proc env.
	if(const auto env_param = evt.get_param(15); !env_param->empty()) {
		const auto can_load_env_from_proc = is_large_envs_enabled();
		evt.get_tinfo()->set_env(env_param->data(), env_param->len(), can_load_env_from_proc);
	}

	// Set cgroups.
	if(const auto cgroups_param = evt.get_param(14); !cgroups_param->empty()) {
		evt.get_tinfo()->set_cgroups(cgroups_param->as<std::vector<std::string>>());
	}

	// Set tty.
	if(const auto tty_param = evt.get_param(16); !tty_param->empty()) {
		evt.get_tinfo()->m_tty = tty_param->as<uint32_t>();
	}

	// Set the exepath.
	if(!evt.get_param(27)->empty()) {
		/* Parameter 28: trusted_exepath (type: PT_FSPATH) */
		evt.get_tinfo()->set_exepath(evt.get_param(27)->as<std::string>());
	} else {
		/* ONLY VALID FOR OLD SCAP-FILES:
		 * In older event versions we can only rely on our userspace reconstruction
		 */

		// If we are not able to retrieve the enter event we can do nothing.
		sinsp_evt *enter_evt = &m_tmp_evt;
		if(retrieve_enter_event(*enter_evt, evt)) {
			std::string fullpath;

			/* We need to manage the 2 possible cases:
			 * - enter event is an `EXECVE`
			 * - enter event is an `EXECVEAT`
			 */
			if(enter_evt->get_type() == PPME_SYSCALL_EXECVE_19_E) {
				/*
				 * Get filename
				 */
				std::string_view filename = enter_evt->get_param(0)->as<std::string_view>();
				/* This could happen only if we are not able to get the info from the kernel,
				 * because if the syscall was successful the pathname was surely here the problem
				 * is that for some reason we were not able to get it with our instrumentation,
				 * for example when the `bpf_probe_read()` call fails in BPF.
				 */
				if(filename == "<NA>") {
					fullpath = "<NA>";
				} else {
					/* Here the filename can be relative or absolute. */
					fullpath = sinsp_utils::concatenate_paths(evt.get_tinfo()->get_cwd(), filename);
				}
			} else if(enter_evt->get_type() == PPME_SYSCALL_EXECVEAT_E) {
				/*
				 * Get dirfd
				 */
				int64_t dirfd = enter_evt->get_param(0)->as<int64_t>();

				/*
				 * Get flags
				 */
				uint32_t flags = enter_evt->get_param(2)->as<uint32_t>();

				/*
				 * Get pathname
				 */

				/* The pathname could be:
				 * - (1) relative (to dirfd).
				 * - (2) absolute.
				 * - (3) empty in the kernel because the user specified the `AT_EMPTY_PATH` flag.
				 *   In this case, `dirfd` must refer to a file.
				 *   Please note:
				 *   The path is empty in the kernel but in userspace, we will obtain a `<NA>`.
				 * - (4) empty in the kernel because we fail to recover it from the registries.
				 * 	 Please note:
				 *   The path is empty in the kernel but in userspace, we will obtain a `<NA>`.
				 */
				std::string_view pathname = enter_evt->get_param(1)->as<std::string_view>();

				/* If the pathname is `<NA>` here we shouldn't have problems during `parse_dirfd`.
				 * It doesn't start with "/" so it is not considered an absolute path.
				 */
				std::string sdir = parse_dirfd(evt, pathname, dirfd);

				// Update event fdinfo since parse_dirfd is stateless
				if(sdir != "." && sdir != "<UNKNOWN>") {
					evt.set_fd_info(evt.get_tinfo()->get_fd(dirfd));
				}

				/* (4) In this case, we were not able to recover the pathname from the kernel or
				 * we are not able to recover information about `dirfd` in our `sinsp` state.
				 * Fallback to `<NA>`.
				 */
				if((!(flags & PPM_EXVAT_AT_EMPTY_PATH) && pathname == "<NA>") ||
				   sdir == "<UNKNOWN>") {
					fullpath = "<NA>";
				}
				/* (3) In this case we have already obtained the `exepath` and it is `sdir`, we just
				 * need to sanitize it.
				 */
				else if(flags & PPM_EXVAT_AT_EMPTY_PATH) {
					/* In this case `sdir` will always be an absolute path.
					 * concatenate_paths takes care of resolving the path
					 */
					fullpath = sinsp_utils::concatenate_paths("", sdir);

				}
				/* (2)/(1) If it is relative or absolute we craft the `fullpath` as usual:
				 * - `sdir` + `pathname`
				 */
				else {
					fullpath = sinsp_utils::concatenate_paths(sdir, pathname);
				}
			}
			evt.get_tinfo()->set_exepath(std::move(fullpath));
		}
	}

	// Set the vpgid.
	if(const auto vpgid_param = evt.get_param(17); !vpgid_param->empty()) {
		evt.get_tinfo()->m_vpgid = vpgid_param->as<int64_t>();
	}

	// Set the loginuid.
	if(const auto loginuid_param = evt.get_param(18); !loginuid_param->empty()) {
		// Notice: this can potentially set loginuid to UINT32_MAX, which is used to denote an
		// uid invalid value.
		evt.get_tinfo()->set_loginuid(loginuid_param->as<uint32_t>());
	}

	// Set execve/execveat flags.
	if(const auto flags_param = evt.get_param(19); !flags_param->empty()) {
		const auto flags = flags_param->as<uint32_t>();
		evt.get_tinfo()->m_exe_writable = (flags & PPM_EXE_WRITABLE) != 0;
		evt.get_tinfo()->m_exe_upper_layer = (flags & PPM_EXE_UPPER_LAYER) != 0;
		evt.get_tinfo()->m_exe_from_memfd = (flags & PPM_EXE_FROM_MEMFD) != 0;
		evt.get_tinfo()->m_exe_lower_layer = (flags & PPM_EXE_LOWER_LAYER) != 0;
	}

	// Set capabilities.
	const auto cap_inheritable_param = evt.get_param(20);
	const auto cap_permitted_param = evt.get_param(21);
	const auto cap_effective_param = evt.get_param(22);
	// If one capability set is present, so is for the other ones, so just check the
	// presence of one of them.
	if(!cap_effective_param->empty()) {
		evt.get_tinfo()->m_cap_inheritable = cap_inheritable_param->as<uint64_t>();
		evt.get_tinfo()->m_cap_permitted = cap_permitted_param->as<uint64_t>();
		evt.get_tinfo()->m_cap_effective = cap_effective_param->as<uint64_t>();
	}

	// Set exe ino fields.
	const auto exe_ino_param = evt.get_param(23);
	const auto exe_ino_ctime_param = evt.get_param(24);
	const auto exe_ino_mtime_param = evt.get_param(25);
	// If one of these parameters is present, so is for the other ones, so just check the
	// presence of one of them.
	if(!exe_ino_mtime_param->empty()) {
		evt.get_tinfo()->m_exe_ino = exe_ino_param->as<uint64_t>();
		evt.get_tinfo()->m_exe_ino_ctime = exe_ino_ctime_param->as<uint64_t>();
		evt.get_tinfo()->m_exe_ino_mtime = exe_ino_mtime_param->as<uint64_t>();
		if(evt.get_tinfo()->m_clone_ts != 0) {
			evt.get_tinfo()->m_exe_ino_ctime_duration_clone_ts =
			        evt.get_tinfo()->m_clone_ts - evt.get_tinfo()->m_exe_ino_ctime;
		}
		if(evt.get_tinfo()->m_pidns_init_start_ts != 0 &&
		   (evt.get_tinfo()->m_exe_ino_ctime > evt.get_tinfo()->m_pidns_init_start_ts)) {
			evt.get_tinfo()->m_exe_ino_ctime_duration_pidns_start =
			        evt.get_tinfo()->m_exe_ino_ctime - evt.get_tinfo()->m_pidns_init_start_ts;
		}
	}

	// Set uid.
	if(const auto uid_param = evt.get_param(26); !uid_param->empty()) {
		// Notice: this can potentially set uid to UINT32_MAX, which is used to denote an uid
		// invalid value.
		evt.get_tinfo()->set_user(uid_param->as<uint32_t>(), must_notify_thread_user_update());
	}

	// Set pgid.
	int64_t pgid = -1;
	if(const auto pgid_param = evt.get_param(28); !pgid_param->empty()) {
		pgid = pgid_param->as<int64_t>();
	}
	evt.get_tinfo()->m_pgid = pgid;

	// Set gid.
	if(const auto gid_param = evt.get_param(29); !gid_param->empty()) {
		evt.get_tinfo()->set_group(gid_param->as<uint32_t>(), must_notify_thread_group_update());
	}

	//
	// execve starts with a clean fd list, so we get rid of the fd list that clone
	// copied from the parent
	// XXX validate this
	//
	//  scap_fd_free_table(tinfo);

	//
	// Clear the flags for this thread, making sure to propagate the inverted
	// and shell pipe flags
	//

	auto spf =
	        evt.get_tinfo()->m_flags & (PPM_CL_PIPE_SRC | PPM_CL_PIPE_DST | PPM_CL_IS_MAIN_THREAD);
	bool inverted = ((evt.get_tinfo()->m_flags & PPM_CL_CLONE_INVERTED) != 0);

	evt.get_tinfo()->m_flags = PPM_CL_ACTIVE;

	evt.get_tinfo()->m_flags |= spf;
	if(inverted) {
		evt.get_tinfo()->m_flags |= PPM_CL_CLONE_INVERTED;
	}

	//
	// This process' name changed, so we need to include it in the protocol again
	//
	evt.get_tinfo()->m_flags |= PPM_CL_NAME_CHANGED;

	//
	// If there's a listener, add a callback to later invoke it.
	//
	if(m_observer) {
		verdict.add_post_process_cbs(
		        [](sinsp_observer *observer, sinsp_evt *evt) { observer->on_execve(evt); });
	}

	/* If any of the threads in a thread group performs an
	 * execve, then all threads other than the thread group
	 * leader are terminated, and the new program is executed in
	 * the thread group leader.
	 *
	 * if `evt.get_tinfo()->m_tginfo->get_thread_count() > 1` it means
	 * we still have some not leader threads in the group.
	 */
	if(evt.get_tinfo()->m_tginfo != nullptr && evt.get_tinfo()->m_tginfo->get_thread_count() > 1) {
		for(const auto &thread : evt.get_tinfo()->m_tginfo->get_thread_list()) {
			auto thread_ptr = thread.lock().get();
			/* we don't want to remove the main thread since it is the one
			 * running in this parser!
			 *
			 * Also make sure the thread to be removed is not the one
			 * associated with the event. Under normal conditions this
			 * should not happen, since the kernel will reassign tid before
			 * returning from the exec syscall. But there are crash reports,
			 * indicating possibility the original tid is kept in place, but
			 * the syscall still returns a success.
			 *
			 * To handle such cases gracefully, keep the event thread.
			 */
			if(thread_ptr == nullptr || thread_ptr->is_main_thread() ||
			   thread_ptr->m_tid == evt.get_tinfo()->m_tid) {
				continue;
			}
			m_thread_manager->remove_thread(thread_ptr->m_tid);
		}
	}
}

/* Different possible cases:
 * - the pathname is absolute:
 *	 sdir = "."
 * - the pathname is relative:
 *   - if `dirfd` is `PPM_AT_FDCWD` -> sdir = cwd.
 *   - if we have no information about `dirfd` -> sdir = "<UNKNOWN>".
 *   - if `dirfd` has a valid vaule for us -> sdir = path + "/" at the end.
 */
std::string sinsp_parser::parse_dirfd(sinsp_evt &evt,
                                      const std::string_view name,
                                      const int64_t dirfd) {
	bool is_absolute = false;
	/* This should never happen but just to be sure. */
	if(name.data() != nullptr) {
		is_absolute = (!name.empty() && name[0] == '/');
	}

	if(is_absolute) {
		//
		// The path is absolute.
		// Some processes (e.g. irqbalance) actually do this: they pass an invalid fd and
		// and absolute path, and openat succeeds.
		//
		return ".";
	}

	if(evt.get_tinfo() == nullptr) {
		// In this case we can
		// - neither retrieve the cwd when dirfd == PPM_AT_FDCWD
		// - nor attempt to query the threadtable for the dirfd fd_info
		return "<UNKNOWN>";
	}

	if(dirfd == PPM_AT_FDCWD) {
		return evt.get_tinfo()->get_cwd();
	}

	auto fdinfo = evt.get_tinfo()->get_fd(dirfd);
	if(fdinfo == nullptr) {
		return "<UNKNOWN>";
	}

	if(fdinfo->m_name.empty() || fdinfo->m_name.back() == '/') {
		return fdinfo->m_name;
	}
	return fdinfo->m_name + '/';
}

void sinsp_parser::parse_open_openat_creat_exit(sinsp_evt &evt) const {
	int64_t fd;
	std::string_view name;
	std::string_view enter_evt_name;
	uint32_t flags;
	uint32_t enter_evt_flags;
	sinsp_evt *enter_evt = &m_tmp_evt;
	std::string sdir;
	uint16_t etype = evt.get_type();
	uint32_t dev = 0;
	uint64_t ino = 0;
	bool lastevent_retrieved = false;

	if(evt.get_tinfo() == nullptr) {
		return;
	}

	if(etype != PPME_SYSCALL_OPEN_BY_HANDLE_AT_X) {
		//
		// Load the enter event so we can access its arguments
		//
		lastevent_retrieved = retrieve_enter_event(*enter_evt, evt);
	}

	//
	// Check the return value
	//
	fd = evt.get_syscall_return_value();

	//
	// Parse the parameters, based on the event type
	//
	if(etype == PPME_SYSCALL_OPEN_X) {
		name = evt.get_param(1)->as<std::string_view>();
		flags = evt.get_param(2)->as<uint32_t>();

		if(evt.get_num_params() > 4) {
			dev = evt.get_param(4)->as<uint32_t>();
			if(evt.get_num_params() > 5) {
				ino = evt.get_param(5)->as<uint64_t>();
			}
		}

		//
		// Compare with enter event parameters
		//
		if(lastevent_retrieved && enter_evt->get_num_params() >= 2) {
			enter_evt_name = enter_evt->get_param(0)->as<std::string_view>();
			enter_evt_flags = enter_evt->get_param(1)->as<uint32_t>();

			if(enter_evt_name.data() != nullptr && enter_evt_name != "<NA>") {
				name = enter_evt_name;

				// keep flags added by the syscall exit probe if present
				uint32_t mask = ~(PPM_O_F_CREATED - 1);
				uint32_t added_flags = flags & mask;
				flags = enter_evt_flags | added_flags;
			}
		}

		sdir = evt.get_tinfo()->get_cwd();
	} else if(etype == PPME_SYSCALL_CREAT_X) {
		name = evt.get_param(1)->as<std::string_view>();

		flags = 0;

		if(evt.get_num_params() > 3) {
			dev = evt.get_param(3)->as<uint32_t>();
			if(evt.get_num_params() > 4) {
				ino = evt.get_param(4)->as<uint64_t>();
				if(evt.get_num_params() > 5) {
					uint16_t creat_flags = evt.get_param(5)->as<uint16_t>();
					// creat is a special case becuase it has no flags parameter, so the layer info
					// bits arrive from probe in a separate creat_flags parameter and flags need to
					// be constructed from it
					if(creat_flags & PPM_FD_UPPER_LAYER_CREAT) {
						flags |= PPM_FD_UPPER_LAYER;
					} else if(creat_flags & PPM_FD_LOWER_LAYER_CREAT) {
						flags |= PPM_FD_LOWER_LAYER;
					}
				}
			}
		}

		if(lastevent_retrieved && enter_evt->get_num_params() >= 1) {
			enter_evt_name = enter_evt->get_param(0)->as<std::string_view>();
			enter_evt_flags = 0;

			if(enter_evt_name.data() != nullptr && enter_evt_name != "<NA>") {
				name = enter_evt_name;

				flags |= enter_evt_flags;
			}
		}

		sdir = evt.get_tinfo()->get_cwd();
	} else if(etype == PPME_SYSCALL_OPENAT_X) {
		name = enter_evt->get_param(1)->as<std::string_view>();

		flags = enter_evt->get_param(2)->as<uint32_t>();

		int64_t dirfd = enter_evt->get_param(0)->as<int64_t>();

		sdir = parse_dirfd(evt, name, dirfd);
	} else if(etype == PPME_SYSCALL_OPENAT_2_X || etype == PPME_SYSCALL_OPENAT2_X) {
		name = evt.get_param(2)->as<std::string_view>();

		flags = evt.get_param(3)->as<uint32_t>();

		int64_t dirfd = evt.get_param(1)->as<int64_t>();

		if(etype == PPME_SYSCALL_OPENAT_2_X && evt.get_num_params() > 5) {
			dev = evt.get_param(5)->as<uint32_t>();
			if(evt.get_num_params() > 6) {
				ino = evt.get_param(6)->as<uint64_t>();
			}
		} else if(etype == PPME_SYSCALL_OPENAT2_X && evt.get_num_params() > 6) {
			dev = evt.get_param(6)->as<uint32_t>();
			if(evt.get_num_params() > 7) {
				ino = evt.get_param(7)->as<uint64_t>();
			}
		}

		//
		// Compare with enter event parameters
		//
		if(lastevent_retrieved && enter_evt->get_num_params() >= 3) {
			enter_evt_name = enter_evt->get_param(1)->as<std::string_view>();
			enter_evt_flags = enter_evt->get_param(2)->as<uint32_t>();
			int64_t enter_evt_dirfd = enter_evt->get_param(0)->as<int64_t>();

			if(enter_evt_name.data() != nullptr && enter_evt_name != "<NA>") {
				name = enter_evt_name;

				// keep flags added by the syscall exit probe if present
				uint32_t mask = ~(PPM_O_F_CREATED - 1);
				uint32_t added_flags = flags & mask;
				flags = enter_evt_flags | added_flags;

				dirfd = enter_evt_dirfd;
			}
		}

		sdir = parse_dirfd(evt, name, dirfd);
	} else if(etype == PPME_SYSCALL_OPEN_BY_HANDLE_AT_X) {
		flags = evt.get_param(2)->as<uint32_t>();

		name = evt.get_param(3)->as<std::string_view>();

		if(evt.get_num_params() > 4) {
			dev = evt.get_param(4)->as<uint32_t>();
			if(evt.get_num_params() > 5) {
				ino = evt.get_param(5)->as<uint64_t>();
			}
		}

		// The driver implementation always serves an absolute path for open_by_handle_at using
		// dpath traversal; hence there is no need to interpret the path relative to mountfd.
		sdir = "";
	} else {
		ASSERT(false);
		return;
	}

	// XXX not implemented yet
	// parinfo = evt.get_param(2);
	// ASSERT(parinfo->len() == sizeof(uint32_t));
	// mode = *(uint32_t*)parinfo->data());

	std::string fullpath = sinsp_utils::concatenate_paths(sdir, name);

	if(fd >= 0) {
		//
		// Populate the new fdi
		//
		auto fdi = m_fdinfo_factory.create();
		if(flags & PPM_O_DIRECTORY) {
			fdi->m_type = SCAP_FD_DIRECTORY;
		} else {
			fdi->m_type = SCAP_FD_FILE_V2;
		}

		fdi->m_openflags = flags;
		fdi->m_mount_id = 0;
		fdi->m_dev = dev;
		fdi->m_ino = ino;
		fdi->add_filename_raw(name);
		fdi->add_filename(fullpath);
		if(flags & PPM_FD_UPPER_LAYER) {
			fdi->set_overlay_upper();
		}
		if(flags & PPM_FD_LOWER_LAYER) {
			fdi->set_overlay_lower();
		}

		//
		// Add the fd to the table.
		//
		evt.set_fd_info(evt.get_tinfo()->add_fd(fd, std::move(fdi)));
	}

	if(m_observer && !(flags & PPM_O_DIRECTORY)) {
		m_observer->on_file_open(&evt, fullpath, flags);
	}
}

void sinsp_parser::parse_fchmod_fchown_exit(sinsp_evt &evt) {
	// Both of these syscalls act on fds although they do not
	// create them. Take the fd argument and attempt to look up
	// the fd from the thread.
	if(evt.get_tinfo() == nullptr) {
		return;
	}

	ASSERT(evt.get_param_info(1)->type == PT_FD);
	const int64_t fd = evt.get_param(1)->as<int64_t>();
	evt.get_tinfo()->m_lastevent_fd = fd;
	evt.set_fd_info(evt.get_tinfo()->get_fd(fd));
}

//
// Helper function to allocate a socket fd, initialize it by parsing its parameters and add it to
// the fd table of the given thread.
//
inline void sinsp_parser::add_socket(sinsp_evt &evt,
                                     const int64_t fd,
                                     const uint32_t domain,
                                     const uint32_t type,
                                     const uint32_t protocol) const {
	//
	// Populate the new fdi
	//
	auto fdi = m_fdinfo_factory.create();
	memset(&(fdi->m_sockinfo.m_ipv4info), 0, sizeof(fdi->m_sockinfo.m_ipv4info));
	fdi->m_type = SCAP_FD_UNKNOWN;
	fdi->m_sockinfo.m_ipv4info.m_fields.m_l4proto = SCAP_L4_UNKNOWN;

	if(domain == PPM_AF_UNIX) {
		fdi->m_type = SCAP_FD_UNIX_SOCK;
	} else if(domain == PPM_AF_INET || domain == PPM_AF_INET6) {
		fdi->m_type = (domain == PPM_AF_INET) ? SCAP_FD_IPV4_SOCK : SCAP_FD_IPV6_SOCK;

		uint8_t l4proto = SCAP_L4_UNKNOWN;
		if(protocol == IPPROTO_TCP) {
			l4proto = (type == SOCK_RAW) ? SCAP_L4_RAW : SCAP_L4_TCP;
		} else if(protocol == IPPROTO_UDP) {
			l4proto = (type == SOCK_RAW) ? SCAP_L4_RAW : SCAP_L4_UDP;
		} else if(protocol == IPPROTO_IP) {
			//
			// XXX: we mask type because, starting from linux 2.6.27, type can be ORed with
			//      SOCK_NONBLOCK and SOCK_CLOEXEC. We need to validate that byte masking is
			//      acceptable
			//
			if((type & 0xff) == SOCK_STREAM) {
				l4proto = SCAP_L4_TCP;
			} else if((type & 0xff) == SOCK_DGRAM) {
				l4proto = SCAP_L4_UDP;
			} else {
				ASSERT(false);
			}
		} else if(protocol == IPPROTO_ICMP) {
			l4proto = (type == SOCK_RAW) ? SCAP_L4_RAW : SCAP_L4_ICMP;
		} else if(protocol == IPPROTO_RAW) {
			l4proto = SCAP_L4_RAW;
		}

		if(domain == PPM_AF_INET) {
			fdi->m_sockinfo.m_ipv4info.m_fields.m_l4proto = l4proto;
		} else {
			memset(&(fdi->m_sockinfo.m_ipv6info), 0, sizeof(fdi->m_sockinfo.m_ipv6info));
			fdi->m_sockinfo.m_ipv6info.m_fields.m_l4proto = l4proto;
		}
	} else if(domain == PPM_AF_NETLINK) {
		fdi->m_type = SCAP_FD_NETLINK;
	} else {
		if(domain != 10 &&  // IPv6
#ifdef _WIN32
		   domain != AF_INET6 &&  // IPv6 on Windows
#endif
		   domain != 17)  // AF_PACKET, used for packet capture
		{
			// A possible case in which we enter here is when we reproduce an old scap-file like
			// `scap_2013` in our tests. In this case, we have only the exit event of the socket
			// `evt_num=5` because we have just started the capture so we lost the enter event. The
			// result produced by our scap-file converter is a socket with (domain=0, type=0,
			// protocol=0).
			fdi->m_type = SCAP_FD_UNKNOWN;
		}
	}

	if(fdi->m_type == SCAP_FD_UNKNOWN) {
		SINSP_STR_DEBUG("Unknown fd fd=" + std::to_string(fd) +
		                " domain=" + std::to_string(domain) + " type=" + std::to_string(type) +
		                " protocol=" + std::to_string(protocol) +
		                " pid=" + std::to_string(evt.get_tinfo()->m_pid) +
		                " comm=" + evt.get_tinfo()->m_comm);
	}

	//
	// Add the fd to the table.
	//
	evt.set_fd_info(evt.get_tinfo()->add_fd(fd, std::move(fdi)));
}

/**
 * If we receive a call to 'send()/sendto()/sendmsg()' and the event's m_fdinfo is nullptr,
 * then we likely missed the call to 'socket()' that created the file
 * descriptor.  In that case, we'll guess that it's a SOCK_DGRAM/UDP socket
 * and create the fdinfo based on that.
 *
 * Preconditions: evt.get_fd_info() == nullptr and
 *                evt.get_tinfo() != nullptr
 *
 */
inline void sinsp_parser::infer_send_sendto_sendmsg_fdinfo(sinsp_evt &evt) const {
	if((evt.get_fd_info() != nullptr) || (evt.get_tinfo() == nullptr)) {
		return;
	}

	constexpr uint32_t FILE_DESCRIPTOR_PARAM_ID = 2;
	constexpr uint32_t SOCKET_TUPLE_PARAM_ID = 4;

	if(evt.get_syscall_return_value() < 0) {
		// Call to send*() failed so we cannot trust parameters provided by the user.
		return;
	}

	ASSERT(evt.get_param_info(FILE_DESCRIPTOR_PARAM_ID)->type == PT_FD);
	const int64_t fd = evt.get_param(FILE_DESCRIPTOR_PARAM_ID)->as<int64_t>();
	ASSERT(fd >= 0);

	const auto parinfo = evt.get_param(SOCKET_TUPLE_PARAM_ID);
	const auto addr_family = *parinfo->data();

	if((addr_family == AF_INET) || (addr_family == AF_INET6)) {
		const uint32_t domain = (addr_family == AF_INET) ? PPM_AF_INET : PPM_AF_INET6;

#ifndef _WIN32
		SINSP_DEBUG(
		        "Call to send*() with fd=%d; missing socket() "
		        "data. Adding socket %s/SOCK_DGRAM/IPPROTO_UDP "
		        "for command '%s', pid %d",
		        fd,
		        (domain == PPM_AF_INET) ? "PPM_AF_INET" : "PPM_AF_INET6",
		        evt.get_tinfo()->get_comm().c_str(),
		        evt.get_tinfo()->m_pid);
#endif

		// Here we're assuming send*() means SOCK_DGRAM/UDP, but it
		// can be used with TCP.  We have no way to know for sure at
		// this point.
		add_socket(evt, fd, domain, SOCK_DGRAM, IPPROTO_UDP);
	}
}

void sinsp_parser::parse_socket_exit(sinsp_evt &evt) const {
	//
	// NOTE: we don't check the return value of get_param() because we know the arguments we need
	// are there.
	// XXX this extraction would be much faster if we parsed the event manually to extract the
	// parameters in one scan. We don't care too much because we assume that we get here
	// seldom enough that saving few tens of CPU cycles is not important.
	//
	int64_t fd = evt.get_syscall_return_value();

	if(fd < 0) {
		//
		// socket() failed. Nothing to add to the table.
		//
		return;
	}

	if(evt.get_tinfo() == nullptr) {
		return;
	}

	//
	// Extract the arguments
	//
	uint32_t domain = evt.get_param(1)->as<uint32_t>();
	uint32_t type = evt.get_param(2)->as<uint32_t>();
	uint32_t protocol = evt.get_param(3)->as<uint32_t>();

	//
	// Allocate a new fd descriptor, populate it and add it to the thread fd table
	//
	add_socket(evt, fd, domain, type, protocol);
}

void sinsp_parser::parse_bind_exit(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	const sinsp_evt_param *parinfo;
	int64_t retval;
	const char *parstr;

	if(evt.get_fd_info() == nullptr) {
		return;
	}

	retval = evt.get_syscall_return_value();

	if(retval < 0) {
		return;
	}

	parinfo = evt.get_param(1);
	if(parinfo->empty()) {
		//
		// No address, there's nothing we can really do with this.
		//
		return;
	}

	const auto packed_data = reinterpret_cast<const uint8_t *>(parinfo->data());
	const auto family = *packed_data;

	//
	// Update the FD info with this tuple, assume that if port > 0, means that
	// the socket is used for listening
	//
	if(family == PPM_AF_INET) {
		uint32_t ip;
		uint16_t port;
		memcpy(&ip, packed_data + 1, sizeof(ip));
		memcpy(&port, packed_data + 5, sizeof(port));
		if(port > 0) {
			evt.get_fd_info()->m_type = SCAP_FD_IPV4_SERVSOCK;
			evt.get_fd_info()->m_sockinfo.m_ipv4serverinfo.m_ip = ip;
			evt.get_fd_info()->m_sockinfo.m_ipv4serverinfo.m_port = port;
			evt.get_fd_info()->m_sockinfo.m_ipv4serverinfo.m_l4proto =
			        evt.get_fd_info()->m_sockinfo.m_ipv4info.m_fields.m_l4proto;
			evt.get_fd_info()->set_role_server();
		}
	} else if(family == PPM_AF_INET6) {
		const uint8_t *ip = packed_data + 1;
		uint16_t port;
		memcpy(&port, packed_data + 17, sizeof(uint16_t));
		if(port > 0) {
			if(sinsp_utils::is_ipv4_mapped_ipv6(ip)) {
				evt.get_fd_info()->m_type = SCAP_FD_IPV4_SERVSOCK;
				evt.get_fd_info()->m_sockinfo.m_ipv4serverinfo.m_l4proto =
				        evt.get_fd_info()->m_sockinfo.m_ipv6info.m_fields.m_l4proto;
				memcpy(&evt.get_fd_info()->m_sockinfo.m_ipv4serverinfo.m_ip,
				       packed_data + 13,
				       sizeof(uint32_t));
				evt.get_fd_info()->m_sockinfo.m_ipv4serverinfo.m_port = port;
			} else {
				evt.get_fd_info()->m_type = SCAP_FD_IPV6_SERVSOCK;
				evt.get_fd_info()->m_sockinfo.m_ipv6serverinfo.m_port = port;
				memcpy(evt.get_fd_info()->m_sockinfo.m_ipv6serverinfo.m_ip.m_b,
				       ip,
				       sizeof(ipv6addr));
				evt.get_fd_info()->m_sockinfo.m_ipv6serverinfo.m_l4proto =
				        evt.get_fd_info()->m_sockinfo.m_ipv6info.m_fields.m_l4proto;
			}
			evt.get_fd_info()->set_role_server();
		}
	}
	//
	// Update the name of this socket
	//
	evt.get_fd_info()->m_name = evt.get_param_as_str(1, &parstr, sinsp_evt::PF_SIMPLE);

	//
	// If there's a listener, add a callback to later invoke it.
	//
	if(m_observer) {
		verdict.add_post_process_cbs(
		        [](sinsp_observer *observer, sinsp_evt *evt) { observer->on_bind(evt); });
	}
}

/**
 * Register a socket in pending state
 */
void sinsp_parser::parse_connect_enter(sinsp_evt &evt) const {
	const auto fdinfo = evt.get_fd_info();
	if(fdinfo == nullptr) {
		return;
	}

	if(m_track_connection_status) {
		fdinfo->set_socket_pending();
	}

	auto &sockinfo = fdinfo->m_sockinfo;

	const sinsp_evt_param *addr_param = evt.get_param(1);
	if(addr_param->empty()) {
		// Address can be nullptr:
		// sk is a TCP fastopen active socket and
		// TCP_FASTOPEN_CONNECT sockopt is set and
		// we already have a valid cookie for this socket.
		return;
	}

	const auto packed_data = reinterpret_cast<const uint8_t *>(addr_param->data());

	switch(const uint8_t family = *packed_data; family) {
	case PPM_AF_INET: {
		fdinfo->m_type = SCAP_FD_IPV4_SOCK;
		memcpy(&sockinfo.m_ipv4info.m_fields.m_dip, packed_data + 1, sizeof(uint32_t));
		memcpy(&sockinfo.m_ipv4info.m_fields.m_dport, packed_data + 5, sizeof(uint16_t));
		break;
	}
	case PPM_AF_INET6: {
		uint16_t port;
		memcpy(&port, packed_data + 17, sizeof(uint16_t));
		const uint8_t *ip = packed_data + 1;
		if(sinsp_utils::is_ipv4_mapped_ipv6(ip)) {
			fdinfo->m_type = SCAP_FD_IPV4_SOCK;
			memcpy(&sockinfo.m_ipv4info.m_fields.m_dip, packed_data + 13, sizeof(uint32_t));
			sockinfo.m_ipv4info.m_fields.m_dport = port;
		} else {
			fdinfo->m_type = SCAP_FD_IPV6_SOCK;
			sockinfo.m_ipv6info.m_fields.m_dport = port;
			memcpy(sockinfo.m_ipv6info.m_fields.m_dip.m_b, ip, sizeof(ipv6addr));
		}
		break;
	}
	case PPM_AF_UNSPEC: {
		switch(fdinfo->m_type) {
		case SCAP_FD_IPV4_SOCK:
			sockinfo.m_ipv4info.m_fields.m_dip = 0;
			sockinfo.m_ipv4info.m_fields.m_dport = 0;
			break;
		case SCAP_FD_IPV6_SOCK:
			sockinfo.m_ipv6info.m_fields.m_dip = ipv6addr::empty_address;
			sockinfo.m_ipv6info.m_fields.m_dport = 0;
			break;
		default:
			break;
		}
		sinsp_utils::sockinfo_to_str(&sockinfo,
		                             fdinfo->m_type,
		                             &evt.get_paramstr_storage()[0],
		                             (uint32_t)evt.get_paramstr_storage().size(),
		                             m_hostname_and_port_resolution_enabled);

		fdinfo->m_name = &evt.get_paramstr_storage()[0];
		return;
	}
	default: {
		// Add the friendly name to the fd info.
		const char *parstr;
		fdinfo->m_name = evt.get_param_as_str(1, &parstr, sinsp_evt::PF_SIMPLE);
		break;
	}
	}

	// If there's a listener callback, and we're tracking connection status, invoke it.
	if(m_track_connection_status && m_observer) {
		// TODO(ekoops): remove const_cast once we adapt sinsp_observer::on_connect API to accept
		//    const pointers/references.
		m_observer->on_connect(&evt, const_cast<uint8_t *>(packed_data));
	}
}

inline void sinsp_parser::fill_client_socket_info(sinsp_evt &evt,
                                                  const uint8_t *packed_data,
                                                  const bool overwrite_dest,
                                                  const bool can_resolve_hostname_and_port) {
	uint8_t family;
	const char *parstr;
	bool changed;

	//
	// Validate the family
	//
	family = *packed_data;

	//
	// Fill the fd with the socket info
	//
	if(family == PPM_AF_INET || family == PPM_AF_INET6) {
		if(family == PPM_AF_INET6) {
			//
			// Check to see if it's an IPv4-mapped IPv6 address
			// (http://en.wikipedia.org/wiki/IPv6#IPv4-mapped_IPv6_addresses)
			//
			const uint8_t *sip = packed_data + 1;
			const uint8_t *dip = packed_data + 19;

			if(!(sinsp_utils::is_ipv4_mapped_ipv6(sip) && sinsp_utils::is_ipv4_mapped_ipv6(dip))) {
				evt.get_fd_info()->m_type = SCAP_FD_IPV6_SOCK;
				changed = set_ipv6_addresses_and_ports(*evt.get_fd_info(),
				                                       packed_data,
				                                       overwrite_dest);
			} else {
				evt.get_fd_info()->m_type = SCAP_FD_IPV4_SOCK;
				changed = set_ipv4_mapped_ipv6_addresses_and_ports(*evt.get_fd_info(),
				                                                   packed_data,
				                                                   overwrite_dest);
			}
		} else {
			evt.get_fd_info()->m_type = SCAP_FD_IPV4_SOCK;

			//
			// Update the FD info with this tuple
			//
			changed = set_ipv4_addresses_and_ports(*evt.get_fd_info(), packed_data, overwrite_dest);
		}

		if(changed && evt.get_fd_info()->is_role_server() && evt.get_fd_info()->is_udp_socket()) {
			// connect done by a udp server, swap the addresses
			swap_addresses(*evt.get_fd_info());
		}

		//
		// Add the friendly name to the fd info
		//
		sinsp_utils::sockinfo_to_str(&evt.get_fd_info()->m_sockinfo,
		                             evt.get_fd_info()->m_type,
		                             &evt.get_paramstr_storage()[0],
		                             (uint32_t)evt.get_paramstr_storage().size(),
		                             can_resolve_hostname_and_port);

		evt.get_fd_info()->m_name = &evt.get_paramstr_storage()[0];
	} else {
		if(!evt.get_fd_info()->is_unix_socket()) {
			//
			// This should happen only in case of a bug in our code, because I'm assuming that the
			// OS causes a connect with the wrong socket type to fail. Assert in debug mode and just
			// return in release mode.
			//
			ASSERT(false);
			return;
		}

		//
		// Add the friendly name to the fd info
		//
		evt.get_fd_info()->m_name = evt.get_param_as_str(1, &parstr, sinsp_evt::PF_SIMPLE);

		//
		// Update the FD with this tuple
		//
		evt.get_fd_info()->set_unix_info(packed_data);
	}

	if(evt.get_fd_info()->is_role_none()) {
		//
		// Mark this fd as a client
		//
		evt.get_fd_info()->set_role_client();
	}
}

void sinsp_parser::parse_connect_exit(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	if(evt.get_tinfo() == nullptr) {
		return;
	}

	bool force_overwrite_stale_data = false;

	if(evt.get_fd_info() == nullptr) {
		// Perhaps we dropped the connect enter event.
		// try harder to be resilient.
		const int64_t fd = evt.get_param(2)->as<int64_t>();
		if(fd < 0) {
			// Accept failure. Do nothing.
			return;
		}
		evt.get_tinfo()->m_lastevent_fd = fd;
		evt.set_fd_info(evt.get_tinfo()->get_fd(evt.get_tinfo()->m_lastevent_fd));
		if(evt.get_fd_info() == nullptr) {
			// Ok this is a completely new fd; we probably lost too many events.
			// Bye.
			return;
		}
		// ok we got stale data; we probably missed the connect enter event on this thread.
		// Force overwrite existing fdinfo socket data
		force_overwrite_stale_data = true;
	}

	const int64_t retval = evt.get_syscall_return_value();

	if(m_track_connection_status) {
		if(retval == -SE_EINPROGRESS) {
			evt.get_fd_info()->set_socket_pending();
		} else if(retval < 0) {
			evt.get_fd_info()->set_socket_failed();
		} else {
			evt.get_fd_info()->set_socket_connected();
		}
	} else {
		if(retval < 0 && retval != -SE_EINPROGRESS) {
			return;
		}
		evt.get_fd_info()->set_socket_connected();
	}

	const sinsp_evt_param *tuple_param = evt.get_param(1);
	if(tuple_param->empty()) {
		// Address can be nullptr:
		// sk is a TCP fastopen active socket and
		// TCP_FASTOPEN_CONNECT sockopt is set and
		// we already have a valid cookie for this socket.
		return;
	}

	const auto packed_data = reinterpret_cast<const uint8_t *>(tuple_param->data());

	fill_client_socket_info(evt,
	                        packed_data,
	                        force_overwrite_stale_data,
	                        m_hostname_and_port_resolution_enabled);

	// If there's a listener, add a callback to later invoke it.
	if(m_observer) {
		verdict.add_post_process_cbs([packed_data](sinsp_observer *observer, sinsp_evt *evt) {
			// TODO(ekoops): remove const_cast once we adapt sinsp_observer::on_connect API to
			//   accept const pointers/references.
			observer->on_connect(evt, const_cast<uint8_t *>(packed_data));
		});
	}
}

void sinsp_parser::parse_accept_exit(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	// Lookup the thread.
	if(evt.get_tinfo() == nullptr) {
		return;
	}

	// Extract the fd.
	const int64_t fd = evt.get_syscall_return_value();

	if(fd < 0) {
		// Accept failure: do nothing.
		return;
	}

	// Update the last event fd. It's needed by the filtering engine.
	evt.get_tinfo()->m_lastevent_fd = fd;

	// Extract the address.
	const sinsp_evt_param *parinfo = evt.get_param(1);
	if(parinfo->empty()) {
		// No address, there's nothing we can really do with this.
		// This happens for socket types that we don't support, so we have the assertion
		// to make sure that this is not a type of socket that we support.
		return;
	}

	const auto packed_data = reinterpret_cast<const uint8_t *>(parinfo->data());

	// Populate the fd info class.
	std::shared_ptr fdi = m_fdinfo_factory.create();
	if(*packed_data == PPM_AF_INET) {
		set_ipv4_addresses_and_ports(*fdi, packed_data);
		fdi->m_type = SCAP_FD_IPV4_SOCK;
		fdi->m_sockinfo.m_ipv4info.m_fields.m_l4proto = SCAP_L4_TCP;
	} else if(*packed_data == PPM_AF_INET6) {
		// Check to see if it's an IPv4-mapped IPv6 address
		// (http://en.wikipedia.org/wiki/IPv6#IPv4-mapped_IPv6_addresses)
		const uint8_t *sip = packed_data + 1;
		const uint8_t *dip = packed_data + 19;

		if(sinsp_utils::is_ipv4_mapped_ipv6(sip) && sinsp_utils::is_ipv4_mapped_ipv6(dip)) {
			set_ipv4_mapped_ipv6_addresses_and_ports(*fdi, packed_data);
			fdi->m_type = SCAP_FD_IPV4_SOCK;
			fdi->m_sockinfo.m_ipv4info.m_fields.m_l4proto = SCAP_L4_TCP;
		} else {
			set_ipv6_addresses_and_ports(*fdi, packed_data);
			fdi->m_type = SCAP_FD_IPV6_SOCK;
			fdi->m_sockinfo.m_ipv6info.m_fields.m_l4proto = SCAP_L4_TCP;
		}
	} else if(*packed_data == PPM_AF_UNIX) {
		fdi->m_type = SCAP_FD_UNIX_SOCK;
		fdi->set_unix_info(packed_data);
	} else {
		// Unsupported family
		return;
	}

	const char *parstr;
	fdi->m_name = evt.get_param_as_str(1, &parstr, sinsp_evt::PF_SIMPLE);
	fdi->m_flags = 0;

	// If there's a listener, add a callback to later invoke it.
	if(m_observer) {
		verdict.add_post_process_cbs(
		        [fd, packed_data, fdi](sinsp_observer *observer, sinsp_evt *evt) {
			        auto fd_info = evt->get_fd_info();
			        if(fd_info == nullptr) {
				        fd_info = fdi.get();
			        }
			        // TODO(ekoops): remove const_cast once we adapt sinsp_observer::on_accept API
			        //   to accept const pointers/references.
			        observer->on_accept(evt, fd, const_cast<uint8_t *>(packed_data), fd_info);
		        });
	}

	// Mark this fd as a server.
	fdi->set_role_server();

	// Mark this fd as a connected socket.
	fdi->set_socket_connected();

	// Add the entry to the table.
	evt.set_fd_info(evt.get_tinfo()->add_fd(fd, std::move(fdi)));
}

//
// This function takes care of cleaning up the FD and removing it from all the tables
// (process FD table, connection table...).
// It's invoked when a close() or a thread exit happens.
//
void sinsp_parser::erase_fd(erase_fd_params &params, sinsp_parser_verdict &verdict) const {
	// Schedule the fd for removal.
	if(params.m_remove_from_table) {
		verdict.add_fd_to_remove(params.m_tinfo->m_tid, params.m_fd);
	}

	// If there's a listener, invoke the callback.
	// Note: we avoid postponing this to avoid the risk of use-after-free.
	if(m_observer) {
		m_observer->on_erase_fd(&params);
	}
}

void sinsp_parser::parse_close_exit(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	// If the close() was successful, do the cleanup
	if(evt.get_syscall_return_value() >= 0) {
		if(evt.get_fd_info() == nullptr || evt.get_tinfo() == nullptr) {
			return;
		}

		erase_fd_params eparams;
		eparams.m_fd = evt.get_tinfo()->m_lastevent_fd;
		eparams.m_fdinfo = evt.get_fd_info();
		eparams.m_remove_from_table = true;
		eparams.m_tinfo = evt.get_tinfo();
		erase_fd(eparams, verdict);
		return;
	}

	//
	// It is normal when a close fails that the fd lookup failed, so we revert the
	// increment of m_n_failed_fd_lookups (for the enter event too if there's one).
	//
	if(m_sinsp_stats_v2 != nullptr) {
		m_sinsp_stats_v2->m_n_failed_fd_lookups--;
	}
	// TODO(ekoops): remove this once we remove, in sinsp_parser::reset(), the logic setting the
	//   lastevent type upon the `close` enter event reception as well as the logic setting the
	//   lastevent data validity upon the `close` exit event reception.
	if(evt.get_tinfo() && evt.get_tinfo()->is_lastevent_data_valid()) {
		if(m_sinsp_stats_v2 != nullptr) {
			m_sinsp_stats_v2->m_n_failed_fd_lookups--;
		}
	}
}

void sinsp_parser::add_pipe(sinsp_evt &evt,
                            const int64_t fd,
                            const uint64_t ino,
                            const uint32_t openflags) const {
	//
	// lookup the thread info
	//
	if(!evt.get_tinfo()) {
		return;
	}

	//
	// Populate the new fdi
	//
	auto fdi = m_fdinfo_factory.create();
	fdi->m_type = SCAP_FD_FIFO;
	fdi->m_ino = ino;
	fdi->m_openflags = openflags;

	//
	// Add the fd to the table.
	//
	evt.set_fd_info(evt.get_tinfo()->add_fd(fd, std::move(fdi)));
}

void sinsp_parser::parse_socketpair_exit(sinsp_evt &evt) const {
	int64_t fd1, fd2;
	int64_t retval;
	uint64_t source_address;
	uint64_t peer_address;

	retval = evt.get_syscall_return_value();

	if(retval < 0) {
		//
		// socketpair() failed. Nothing to add to the table.
		//
		return;
	}

	if(evt.get_tinfo() == nullptr) {
		// There is nothing we can do here if tinfo is missing
		return;
	}

	fd1 = evt.get_param(1)->as<int64_t>();

	fd2 = evt.get_param(2)->as<int64_t>();

	/*
	** In the case of 2 equal fds we ignore them (e.g. both equal to -1).
	*/
	if(fd1 == fd2) {
		evt.set_fd_info(nullptr);
		return;
	}

	source_address = evt.get_param(3)->as<uint64_t>();

	peer_address = evt.get_param(4)->as<uint64_t>();

	auto fdi1 = m_fdinfo_factory.create();
	fdi1->m_type = SCAP_FD_UNIX_SOCK;
	fdi1->m_sockinfo.m_unixinfo.m_fields.m_source = source_address;
	fdi1->m_sockinfo.m_unixinfo.m_fields.m_dest = peer_address;
	auto fdi2 = fdi1->clone();
	evt.set_fd_info(evt.get_tinfo()->add_fd(fd1, std::move(fdi1)));
	evt.get_tinfo()->add_fd(fd2, std::move(fdi2));
}

void sinsp_parser::parse_pipe_exit(sinsp_evt &evt) const {
	const int64_t retval = evt.get_syscall_return_value();

	if(retval < 0) {
		//
		// pipe() failed. Nothing to add to the table.
		//
		return;
	}

	const auto fd1 = evt.get_param(1)->as<int64_t>();
	const auto fd2 = evt.get_param(2)->as<int64_t>();
	const auto ino = evt.get_param(3)->as<uint64_t>();
	uint32_t openflags = 0;
	if(evt.get_type() == PPME_SYSCALL_PIPE2_X) {
		openflags = evt.get_param(4)->as<uint32_t>();
	}
	add_pipe(evt, fd1, ino, openflags);
	add_pipe(evt, fd2, ino, openflags);
}

void sinsp_parser::parse_thread_exit(sinsp_evt &evt, sinsp_parser_verdict &verdict) {
	/* We set the `m_tinfo` in `reset()`.
	 * If we don't have the thread info we do nothing, this thread is already deleted
	 */
	if(evt.get_tinfo() == nullptr) {
		return;
	}

	/* [Mark thread as dead]
	 * We mark the thread as dead here and we will remove it
	 * from the table during remove_thread().
	 * Please note that the `!evt.get_tinfo()->is_dead()` shouldn't be
	 * necessary at all since here we shouldn't receive dead threads.
	 * This is first place where we mark threads as dead.
	 */
	if(evt.get_tinfo()->m_tginfo != nullptr && !evt.get_tinfo()->is_dead()) {
		evt.get_tinfo()->m_tginfo->decrement_thread_count();
	}
	evt.get_tinfo()->set_dead();

	/* [Store the tid to remove]
	 * We set the current tid to remove. We don't remove it here so we can parse the event
	 */
	verdict.set_tid_to_remove(evt.get_tid());

	/* If this thread has no children we don't send the reaper info from the kernel,
	 * so we do nothing.
	 */
	if(evt.get_tinfo()->m_children.size() == 0) {
		return;
	}

	/* [Set the reaper to the current thread]
	 * We need to set the reaper for this thread
	 */
	if(evt.get_type() == PPME_PROCEXIT_1_E && evt.get_num_params() > 4) {
		evt.get_tinfo()->m_reaper_tid = evt.get_param(4)->as<int64_t>();
	} else {
		evt.get_tinfo()->m_reaper_tid = -1;
	}
}

inline bool sinsp_parser::update_ipv4_addresses_and_ports(sinsp_fdinfo &fdinfo,
                                                          const uint32_t tsip,
                                                          const uint16_t tsport,
                                                          const uint32_t tdip,
                                                          const uint16_t tdport,
                                                          const bool overwrite_dest) {
	if(fdinfo.m_type == SCAP_FD_IPV4_SOCK) {
		if((tsip == fdinfo.m_sockinfo.m_ipv4info.m_fields.m_sip &&
		    tsport == fdinfo.m_sockinfo.m_ipv4info.m_fields.m_sport &&
		    tdip == fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dip &&
		    tdport == fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dport) ||
		   (tdip == fdinfo.m_sockinfo.m_ipv4info.m_fields.m_sip &&
		    tdport == fdinfo.m_sockinfo.m_ipv4info.m_fields.m_sport &&
		    tsip == fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dip &&
		    tsport == fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dport)) {
			return false;
		}
	}

	bool changed = false;

	if(fdinfo.m_sockinfo.m_ipv4info.m_fields.m_sip != tsip) {
		fdinfo.m_sockinfo.m_ipv4info.m_fields.m_sip = tsip;
		changed = true;
	}

	if(fdinfo.m_sockinfo.m_ipv4info.m_fields.m_sport != tsport) {
		fdinfo.m_sockinfo.m_ipv4info.m_fields.m_sport = tsport;
		changed = true;
	}

	if(fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dip == 0 ||
	   (overwrite_dest && fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dip != tdip)) {
		fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dip = tdip;
		changed = true;
	}

	if(fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dport == 0 ||
	   (overwrite_dest && fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dport != tdport)) {
		fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dport = tdport;
		changed = true;
	}

	return changed;
}

bool sinsp_parser::set_ipv4_addresses_and_ports(sinsp_fdinfo &fdinfo,
                                                const uint8_t *packed_data,
                                                const bool overwrite_dest) {
	uint32_t tsip, tdip;
	uint16_t tsport, tdport;

	memcpy(&tsip, packed_data + 1, sizeof(uint32_t));
	memcpy(&tsport, packed_data + 5, sizeof(uint16_t));
	memcpy(&tdip, packed_data + 7, sizeof(uint32_t));
	memcpy(&tdport, packed_data + 11, sizeof(uint16_t));

	return update_ipv4_addresses_and_ports(fdinfo, tsip, tsport, tdip, tdport, overwrite_dest);
}

bool sinsp_parser::set_ipv4_mapped_ipv6_addresses_and_ports(sinsp_fdinfo &fdinfo,
                                                            const uint8_t *packed_data,
                                                            const bool overwrite_dest) {
	uint32_t tsip, tdip;
	uint16_t tsport, tdport;

	memcpy(&tsip, packed_data + 13, sizeof(uint32_t));
	memcpy(&tsport, packed_data + 17, sizeof(uint16_t));
	memcpy(&tdip, packed_data + 31, sizeof(uint32_t));
	memcpy(&tdport, packed_data + 35, sizeof(uint16_t));

	return update_ipv4_addresses_and_ports(fdinfo, tsip, tsport, tdip, tdport, overwrite_dest);
}

bool sinsp_parser::set_ipv6_addresses_and_ports(sinsp_fdinfo &fdinfo,
                                                const uint8_t *packed_data,
                                                const bool overwrite_dest) {
	ipv6addr tsip, tdip;
	uint16_t tsport, tdport;

	memcpy((uint8_t *)tsip.m_b, packed_data + 1, sizeof(tsip.m_b));
	memcpy(&tsport, packed_data + 17, sizeof(tsport));

	memcpy((uint8_t *)tdip.m_b, packed_data + 19, sizeof(tdip.m_b));
	memcpy(&tdport, packed_data + 35, sizeof(tdport));

	if(fdinfo.m_type == SCAP_FD_IPV6_SOCK) {
		if((tsip == fdinfo.m_sockinfo.m_ipv6info.m_fields.m_sip &&
		    tsport == fdinfo.m_sockinfo.m_ipv6info.m_fields.m_sport &&
		    tdip == fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dip &&
		    tdport == fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dport) ||
		   (tdip == fdinfo.m_sockinfo.m_ipv6info.m_fields.m_sip &&
		    tdport == fdinfo.m_sockinfo.m_ipv6info.m_fields.m_sport &&
		    tsip == fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dip &&
		    tsport == fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dport)) {
			return false;
		}
	}

	bool changed = false;

	if(fdinfo.m_sockinfo.m_ipv6info.m_fields.m_sip != tsip) {
		fdinfo.m_sockinfo.m_ipv6info.m_fields.m_sip = tsip;
		changed = true;
	}

	if(fdinfo.m_sockinfo.m_ipv6info.m_fields.m_sport != tsport) {
		fdinfo.m_sockinfo.m_ipv6info.m_fields.m_sport = tsport;
		changed = true;
	}

	if(fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dip == ipv6addr::empty_address ||
	   (overwrite_dest && fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dip != tdip)) {
		fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dip = tdip;
		changed = true;
	}

	if(fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dport == 0 ||
	   (overwrite_dest && fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dport != tdport)) {
		fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dport = tdport;
		changed = true;
	}

	return changed;
}

// Return false if the update didn't happen (for example because the tuple is nullptr)
bool sinsp_parser::update_fd(sinsp_evt &evt, const sinsp_evt_param &parinfo) const {
	if(parinfo.empty()) {
		return false;
	}

	const auto packed_data = reinterpret_cast<const uint8_t *>(parinfo.data());
	const auto family = *packed_data;

	if(family == PPM_AF_INET) {
		if(evt.get_fd_info()->m_type == SCAP_FD_IPV4_SERVSOCK) {
			//
			// If this was previously a server socket, propagate the L4 protocol
			//
			evt.get_fd_info()->m_sockinfo.m_ipv4info.m_fields.m_l4proto =
			        evt.get_fd_info()->m_sockinfo.m_ipv4serverinfo.m_l4proto;
		}

		evt.get_fd_info()->m_type = SCAP_FD_IPV4_SOCK;
		if(set_ipv4_addresses_and_ports(*evt.get_fd_info(), packed_data) == false) {
			return false;
		}
	} else if(family == PPM_AF_INET6) {
		//
		// Check to see if it's an IPv4-mapped IPv6 address
		// (http://en.wikipedia.org/wiki/IPv6#IPv4-mapped_IPv6_addresses)
		//
		auto sip = packed_data + 1;
		auto dip = packed_data + 19;

		if(sinsp_utils::is_ipv4_mapped_ipv6(sip) && sinsp_utils::is_ipv4_mapped_ipv6(dip)) {
			evt.get_fd_info()->m_type = SCAP_FD_IPV4_SOCK;

			if(set_ipv4_mapped_ipv6_addresses_and_ports(*evt.get_fd_info(), packed_data) == false) {
				return false;
			}
		} else {
			// It's not an ipv4-mapped ipv6 address. Extract it as a normal address.
			if(set_ipv6_addresses_and_ports(*evt.get_fd_info(), packed_data) == false) {
				return false;
			}
		}
	} else if(family == PPM_AF_UNIX) {
		evt.get_fd_info()->m_type = SCAP_FD_UNIX_SOCK;
		evt.get_fd_info()->set_unix_info(packed_data);
		evt.get_fd_info()->m_name = ((char *)packed_data) + 17;

		return true;
	}

	//
	// If we reach this point and the protocol is not set yet, we assume this
	// connection is UDP, because TCP would fail if the address is changed in
	// the middle of a connection.
	//
	if(evt.get_fd_info()->m_type == SCAP_FD_IPV4_SOCK) {
		if(evt.get_fd_info()->m_sockinfo.m_ipv4info.m_fields.m_l4proto == SCAP_L4_UNKNOWN) {
			evt.get_fd_info()->m_sockinfo.m_ipv4info.m_fields.m_l4proto = SCAP_L4_UDP;
		}
	} else if(evt.get_fd_info()->m_type == SCAP_FD_IPV6_SOCK) {
		if(evt.get_fd_info()->m_sockinfo.m_ipv6info.m_fields.m_l4proto == SCAP_L4_UNKNOWN) {
			evt.get_fd_info()->m_sockinfo.m_ipv6info.m_fields.m_l4proto = SCAP_L4_UDP;
		}
	}

	//
	// If this is an incomplete tuple, patch it using interface info
	//
	m_network_interfaces.update_fd(*evt.get_fd_info());

	return true;
}

void sinsp_parser::swap_addresses(sinsp_fdinfo &fdinfo) {
	if(fdinfo.m_type == SCAP_FD_IPV4_SOCK) {
		const uint32_t tip = fdinfo.m_sockinfo.m_ipv4info.m_fields.m_sip;
		const uint16_t tport = fdinfo.m_sockinfo.m_ipv4info.m_fields.m_sport;
		fdinfo.m_sockinfo.m_ipv4info.m_fields.m_sip = fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dip;
		fdinfo.m_sockinfo.m_ipv4info.m_fields.m_sport =
		        fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dport;
		fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dip = tip;
		fdinfo.m_sockinfo.m_ipv4info.m_fields.m_dport = tport;
	} else {
		const ipv6addr tip = fdinfo.m_sockinfo.m_ipv6info.m_fields.m_sip;
		const uint16_t tport = fdinfo.m_sockinfo.m_ipv6info.m_fields.m_sport;

		fdinfo.m_sockinfo.m_ipv6info.m_fields.m_sip = fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dip;
		fdinfo.m_sockinfo.m_ipv6info.m_fields.m_sport =
		        fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dport;

		fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dip = tip;
		fdinfo.m_sockinfo.m_ipv6info.m_fields.m_dport = tport;
	}
}

void sinsp_parser::parse_fspath_related_exit(sinsp_evt &evt) const {
	sinsp_evt *enter_evt = &m_tmp_evt;
	if(retrieve_enter_event(*enter_evt, evt)) {
		evt.save_enter_event_params(enter_evt);
	}
}

#ifndef _WIN32
// ppm_cmsghdr is a mirror of the POSIX cmsghdr structure. The fundamental assumption when working
// with it is that actual control message variable-size data follows this (padding-aligned) header.
struct ppm_cmsghdr {
	// Length of ppm_cmsghdr structure plus data following it.
	size_t cmsg_len;
	// Originating protocol.
	int cmsg_level;
	// Protocol specific type.
	int cmsg_type;
};

// PPM_CMSG_* macros definitions. Their purpose is to manipulate ppm_cmsghdr structure and fields.
// Majority of them are equivalent to the corresponding variants without PPM_* prefix, but they
// don't depend on msghdr definition (as we don't need it at the moment).
#define PPM_CMSG_FIRSTHDR(msg_control, msg_controllen) \
	((size_t)msg_controllen >= sizeof(ppm_cmsghdr) ? (ppm_cmsghdr *)msg_control : (ppm_cmsghdr *)0)

#define PPM_CMSG_UNALIGNED_READ(cmsg, field, dest)           \
	(memcpy((void *)&(dest),                                 \
	        ((char *)(cmsg)) + offsetof(ppm_cmsghdr, field), \
	        sizeof((cmsg)->field)))

#define PPM_CMSG_ALIGN(len) (((len) + sizeof(size_t) - 1) & (size_t) ~(sizeof(size_t) - 1))

#define PPM_CMSG_NXTHDR(msg_control, msg_controllen, cmsg) \
	ppm_cmsg_nxthdr(msg_control, msg_controllen, cmsg)
static ppm_cmsghdr *ppm_cmsg_nxthdr(char const *msg_control,
                                    size_t const msg_controllen,
                                    ppm_cmsghdr *cmsg) {
	size_t cmsg_len;
	PPM_CMSG_UNALIGNED_READ(cmsg, cmsg_len, cmsg_len);
	if(cmsg_len < sizeof(ppm_cmsghdr)) {
		return nullptr;
	}

	size_t const cmsg_aligned_len = PPM_CMSG_ALIGN(cmsg_len);
	cmsg = reinterpret_cast<ppm_cmsghdr *>(reinterpret_cast<char *>(cmsg) + cmsg_aligned_len);
	if(reinterpret_cast<char *>(cmsg + 1) > msg_control + msg_controllen ||
	   reinterpret_cast<char *>(cmsg) + cmsg_aligned_len > msg_control + msg_controllen) {
		return nullptr;
	}
	return cmsg;
}

#define PPM_CMSG_DATA(cmsg) ((char *)((ppm_cmsghdr *)(cmsg) + 1))

#define PPM_CMSG_LEN(len) (PPM_CMSG_ALIGN(sizeof(ppm_cmsghdr)) + (len))

inline void sinsp_parser::process_recvmsg_ancillary_data_fds(scap_platform *scap_platform,
                                                             int const *fds,
                                                             size_t const fds_len,
                                                             scap_threadinfo &scap_tinfo) {
	char error[SCAP_LASTERR_SIZE] = {};
	for(size_t i = 0; i < fds_len; i++) {
		if(scap_get_fdinfo(scap_platform, &scap_tinfo, fds[i], error) != SCAP_SUCCESS) {
			libsinsp_logger()->format(
			        sinsp_logger::SEV_DEBUG,
			        "scap_get_fdinfo failed: %s, proc table will not be updated with new fd.",
			        error);
		}
	}
}

inline void sinsp_parser::process_recvmsg_ancillary_data(sinsp_evt &evt,
                                                         const sinsp_evt_param &parinfo) const {
	// Seek for SCM_RIGHTS control message headers and extract passed file descriptors.
	char const *msg_ctrl = parinfo.data();
	size_t const msg_ctrllen = parinfo.len();
	for(ppm_cmsghdr *cmsg = PPM_CMSG_FIRSTHDR(msg_ctrl, msg_ctrllen); cmsg != nullptr;
	    cmsg = PPM_CMSG_NXTHDR(msg_ctrl, msg_ctrllen, cmsg)) {
		// Check for malformed control message buffer:
		if(reinterpret_cast<const char *>(cmsg) < msg_ctrl ||
		   reinterpret_cast<const char *>(cmsg) >= msg_ctrl + msg_ctrllen) {
			libsinsp_logger()->format(sinsp_logger::SEV_DEBUG,
			                          "Malformed ancillary data, skipping.");
			break;
		}
		int cmsg_type;
		PPM_CMSG_UNALIGNED_READ(cmsg, cmsg_type, cmsg_type);
		if(cmsg_type != SCM_RIGHTS) {
			continue;
		}
		// Found SCM_RIGHTS control message. Process it.
		size_t cmsg_len;
		PPM_CMSG_UNALIGNED_READ(cmsg, cmsg_len, cmsg_len);
		unsigned long const data_size = cmsg_len - PPM_CMSG_LEN(0);
		unsigned long const fds_len = data_size / sizeof(int);
#define SCM_MAX_FD 253  // Taken from kernel.
		// Guard against malformed event, by checking that data size is a multiple of
		// sizeof(int) (file descriptor size) and the control message doesn't contain more
		// data than allowed by kernel constraints.
		if(data_size % sizeof(int) || fds_len > SCM_MAX_FD) {
			break;
		}
		scap_threadinfo scap_tinfo{};
		memset(&scap_tinfo, 0, sizeof(scap_tinfo));
		m_thread_manager->thread_to_scap(*evt.get_tinfo(), &scap_tinfo);
		int fds[SCM_MAX_FD];
#undef SCM_MAX_FD
		memcpy(&fds, PPM_CMSG_DATA(cmsg), data_size);
		process_recvmsg_ancillary_data_fds(m_scap_platform, fds, fds_len, scap_tinfo);
	}
}
#endif  // _WIN32

void sinsp_parser::parse_rw_exit(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	const sinsp_evt_param *parinfo;
	int64_t retval;
	int64_t tid = evt.get_tid();
	ppm_event_flags eflags = evt.get_info_flags();
	uint16_t etype = evt.get_scap_evt()->type;

	if((etype == PPME_SOCKET_SENDMMSG_X || etype == PPME_SOCKET_RECVMMSG_X) &&
	   evt.get_num_params() == 0) {
		return;
	}

	if((etype == PPME_SOCKET_SEND_X || etype == PPME_SOCKET_SENDTO_X ||
	    etype == PPME_SOCKET_SENDMSG_X) &&
	   evt.get_fd_info() == nullptr && evt.get_tinfo() != nullptr) {
		infer_send_sendto_sendmsg_fdinfo(evt);
	}

	if(evt.get_fd_info() == nullptr) {
		return;
	}

	//
	// Extract the return value
	//
	retval = evt.get_syscall_return_value();

	//
	// If the operation was successful, validate that the fd exists
	//
	if(retval >= 0) {
		if(evt.get_fd_info()->m_type == SCAP_FD_IPV4_SOCK ||
		   evt.get_fd_info()->m_type == SCAP_FD_IPV6_SOCK) {
			evt.get_fd_info()->set_socket_connected();
		}

		if(eflags & EF_READS_FROM_FD) {
			int32_t tupleparam = -1;

			if(etype == PPME_SOCKET_RECVFROM_X) {
				tupleparam = 2;
			} else if(etype == PPME_SOCKET_RECVMSG_X) {
				tupleparam = 3;
			} else if(etype == PPME_SOCKET_RECVMMSG_X || etype == PPME_SOCKET_RECV_X) {
				tupleparam = 4;
			}

			if(tupleparam != -1 &&
			   (evt.get_fd_info()->m_name.length() == 0 || !evt.get_fd_info()->is_tcp_socket())) {
				//
				// recvfrom contains tuple info.
				// If the fd still doesn't contain tuple info (because the socket is a
				// datagram one or because some event was lost),
				// add it here.
				//
				if(update_fd(evt, *evt.get_param(tupleparam))) {
					const char *parstr;

					scap_fd_type fdtype = evt.get_fd_info()->m_type;

					if(fdtype == SCAP_FD_IPV4_SOCK || fdtype == SCAP_FD_IPV6_SOCK) {
						if(evt.get_fd_info()->is_role_none()) {
							evt.get_fd_info()->set_net_role_by_guessing(*evt.get_tinfo(), true);
						}

						if(evt.get_fd_info()->is_role_client()) {
							swap_addresses(*evt.get_fd_info());
						}

						sinsp_utils::sockinfo_to_str(&evt.get_fd_info()->m_sockinfo,
						                             fdtype,
						                             &evt.get_paramstr_storage()[0],
						                             (uint32_t)evt.get_paramstr_storage().size(),
						                             m_hostname_and_port_resolution_enabled);

						evt.get_fd_info()->m_name = &evt.get_paramstr_storage()[0];
					} else {
						evt.get_fd_info()->m_name =
						        evt.get_param_as_str(tupleparam, &parstr, sinsp_evt::PF_SIMPLE);
					}
				}
			}

			//
			// Extract the data buffer
			//
			if(etype == PPME_SYSCALL_READV_X || etype == PPME_SYSCALL_PREADV_X ||
			   etype == PPME_SOCKET_RECVMSG_X) {
				parinfo = evt.get_param(2);
			} else if(etype == PPME_SOCKET_RECVMMSG_X) {
				parinfo = evt.get_param(3);
			} else {  // PPME_SOCKET_RECVFROM_X, PPME_SOCKET_RECV_X
				parinfo = evt.get_param(1);
			}

			const auto data = parinfo->data();
			const auto data_len = parinfo->len();

			//
			// If there's a listener, add a callback to later invoke it.
			//
			if(m_observer) {
				verdict.add_post_process_cbs(
				        [tid, data, retval, data_len](sinsp_observer *observer, sinsp_evt *evt) {
					        observer->on_read(evt,
					                          tid,
					                          evt->get_tinfo()->m_lastevent_fd,
					                          evt->get_fd_info(),
					                          data,
					                          (uint32_t)retval,
					                          data_len);
				        });
			}

			//
			// Check if recvmsg contains ancillary data. If so, we check for SCM_RIGHTS,
			// which is used to pass FDs between processes, and update the sinsp state
			// accordingly via procfs scan.
			//
#ifndef _WIN32
			if(evt.get_fd_info()->is_unix_socket()) {
				int32_t cmparam = -1;
				if(etype == PPME_SOCKET_RECVMSG_X && evt.get_num_params() >= 5) {
					cmparam = 4;
				} else if(etype == PPME_SOCKET_RECVMMSG_X && evt.get_num_params() >= 6) {
					cmparam = 5;
				}

				if(cmparam != -1) {
					parinfo = evt.get_param(cmparam);
					process_recvmsg_ancillary_data(evt, *parinfo);
				}
			}
#endif

		} else {
			if((etype == PPME_SOCKET_SEND_X || etype == PPME_SOCKET_SENDTO_X ||
			    etype == PPME_SOCKET_SENDMSG_X || etype == PPME_SOCKET_SENDMMSG_X) &&
			   (evt.get_fd_info()->m_name.length() == 0 || !evt.get_fd_info()->is_tcp_socket())) {
				// send, sendto, sendmsg and sendmmsg contain tuple info in the exit event.
				// If the fd still doesn't contain tuple info (because the socket is a datagram one
				// or because some event was lost), add it here.
				constexpr uint32_t SOCKET_TUPLE_PARAM_ID = 4;

				if(update_fd(evt, *evt.get_param(SOCKET_TUPLE_PARAM_ID))) {
					scap_fd_type fdtype = evt.get_fd_info()->m_type;

					if(fdtype == SCAP_FD_IPV4_SOCK || fdtype == SCAP_FD_IPV6_SOCK) {
						if(evt.get_fd_info()->is_role_none()) {
							evt.get_fd_info()->set_net_role_by_guessing(*evt.get_tinfo(), false);
						}

						if(evt.get_fd_info()->is_role_server()) {
							swap_addresses(*evt.get_fd_info());
						}

						sinsp_utils::sockinfo_to_str(&evt.get_fd_info()->m_sockinfo,
						                             fdtype,
						                             &evt.get_paramstr_storage()[0],
						                             (uint32_t)evt.get_paramstr_storage().size(),
						                             m_hostname_and_port_resolution_enabled);

						evt.get_fd_info()->m_name = &evt.get_paramstr_storage()[0];
					} else {
						const char *parstr;
						evt.get_fd_info()->m_name = evt.get_param_as_str(SOCKET_TUPLE_PARAM_ID,
						                                                 &parstr,
						                                                 sinsp_evt::PF_SIMPLE);
					}
				}
			}

			//
			// Extract the data buffer
			//
			if(etype == PPME_SOCKET_SENDMMSG_X) {
				parinfo = evt.get_param(2);
			} else {  // PPME_SOCKET_SEND_X, PPME_SOCKET_SENDTO_X, PPME_SOCKET_SENDMSG_X
				parinfo = evt.get_param(1);
			}

			const auto data = parinfo->data();
			const auto data_len = parinfo->len();

			//
			// If there's a listener, add a callback to later invoke it.
			//
			if(m_observer) {
				verdict.add_post_process_cbs(
				        [tid, data, retval, data_len](sinsp_observer *observer, sinsp_evt *evt) {
					        observer->on_write(evt,
					                           tid,
					                           evt->get_tinfo()->m_lastevent_fd,
					                           evt->get_fd_info(),
					                           data,
					                           (uint32_t)retval,
					                           data_len);
				        });
			}
		}
	} else if(m_track_connection_status) {
		if(evt.get_fd_info()->m_type == SCAP_FD_IPV4_SOCK ||
		   evt.get_fd_info()->m_type == SCAP_FD_IPV6_SOCK) {
			evt.get_fd_info()->set_socket_failed();
			//
			// If there's a listener, add a callback to later invoke it.
			//
			if(m_observer) {
				verdict.add_post_process_cbs([](sinsp_observer *observer, sinsp_evt *evt) {
					observer->on_socket_status_changed(evt);
				});
			}
		}
	}
}

void sinsp_parser::parse_sendfile_exit(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	if(!evt.get_fd_info()) {
		return;
	}

	// If the operation was successful and there's a listener, add a callback to later invoke it.
	const int64_t retval = evt.get_syscall_return_value();
	if(retval < 0 || !m_observer) {
		return;
	}

	const int64_t in_fd = evt.get_param(3)->as<int64_t>();
	verdict.add_post_process_cbs([in_fd, retval](sinsp_observer *observer, sinsp_evt *evt) {
		observer->on_sendfile(evt, in_fd, (uint32_t)retval);
	});
}

void sinsp_parser::parse_eventfd_eventfd2_exit(sinsp_evt &evt) const {
	// Lookup the thread info.
	if(evt.get_tinfo() == nullptr) {
		return;
	}

	const int64_t fd = evt.get_syscall_return_value();
	if(fd < 0) {
		// eventfd() failed. Nothing to add to the table.
		return;
	}

	// Populate the new fd info.
	auto fdi = m_fdinfo_factory.create();
	fdi->m_type = SCAP_FD_EVENT;

	if(evt.get_type() == PPME_SYSCALL_EVENTFD2_X) {
		fdi->m_openflags = evt.get_param(1)->as<uint16_t>();
	}

	// Add the fd to the table.
	evt.set_fd_info(evt.get_tinfo()->add_fd(fd, std::move(fdi)));
}

void sinsp_parser::parse_chdir_exit(sinsp_evt &evt) {
	// In case of success, if the event has an associated thread, update its working directory.
	if(evt.get_tinfo() != nullptr && evt.get_syscall_return_value() >= 0) {
		evt.get_tinfo()->update_cwd(evt.get_param(1)->as<std::string_view>());
	}
}

void sinsp_parser::parse_fchdir_exit(sinsp_evt &evt) {
	//
	// Extract the return value
	//
	const int64_t retval = evt.get_syscall_return_value();

	//
	// In case of success, update the thread working dir
	//
	if(retval >= 0) {
		//
		// Find the fd name
		//
		if(evt.get_fd_info() == nullptr || evt.get_tinfo() == nullptr) {
			return;
		}

		// Update the thread working directory
		evt.get_tinfo()->update_cwd(evt.get_fd_info()->m_name);
	}
}

void sinsp_parser::parse_getcwd_exit(sinsp_evt &evt) {
	//
	// Extract the return value
	//
	const int64_t retval = evt.get_syscall_return_value();

	//
	// Check if the syscall was successful
	//
	if(retval >= 0) {
		if(evt.get_tinfo() == nullptr) {
			//
			// No thread in the table. We won't store this event, which mean that
			// we won't be able to parse the corresponding exit event and we'll have
			// to drop the information it carries.
			//
			return;
		}

		std::string cwd = evt.get_param(1)->as<std::string>();

#ifdef _DEBUG
		if(cwd != "/") {
			if(cwd + "/" != evt.get_tinfo()->get_cwd()) {
				//
				// This shouldn't happen, because we should be able to stay in synch by
				// following chdir(). If it does, it's almost sure there was an event drop.
				// In that case, we use this value to update the thread cwd.
				//
#if !defined(_WIN32)
#ifdef _DEBUG
				int target_res;
				char target_name[1024];
				target_res = readlink((cwd + "/").c_str(), target_name, sizeof(target_name) - 1);

				if(target_res > 0) {
					target_name[target_res] = '\0';
					if(target_name != evt.get_tinfo()->get_cwd()) {
						printf("%s != %s", target_name, evt.get_tinfo()->get_cwd().c_str());
						ASSERT(false);
					}
				}

#endif
#endif
			}
		}
#endif

		evt.get_tinfo()->update_cwd(cwd);
	}
}

void sinsp_parser::parse_shutdown_exit(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	if(evt.get_syscall_return_value() < 0) {
		return;
	}

	// Operation was successful, do the cleanup.

	if(evt.get_fd_info() == nullptr) {
		return;
	}

	// If there's a listener, add a callback to later invoke it.
	if(m_observer) {
		verdict.add_post_process_cbs([](sinsp_observer *observer, sinsp_evt *evt) {
			observer->on_socket_shutdown(evt);
		});
	}
}

void sinsp_parser::parse_dup_exit(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	if(evt.get_tinfo() == nullptr) {
		return;
	}

	//
	// Extract the return value
	//
	const int64_t retval = evt.get_syscall_return_value();

	//
	// Check if the syscall was successful
	//
	if(retval >= 0) {
		//
		// Heuristic to determine if a thread is part of a shell pipe
		//
		if(retval == 0) {
			evt.get_tinfo()->m_flags |= PPM_CL_PIPE_DST;
		}
		if(retval == 1) {
			evt.get_tinfo()->m_flags |= PPM_CL_PIPE_SRC;
		}

		if(evt.get_fd_info() == nullptr) {
			return;
		}
		//
		// If the old FD is in the table, remove it properly.
		// The old FD is:
		// 	- dup(): fd number of a previously closed fd that has not been removed from the fd_table
		//		     and has been reassigned to the newly created fd by dup()(very rare condition);
		//  - dup2(): fd number of an existing fd that we pass to the dup2() as the "newfd". dup2()
		//			  will close the existing one. So we need to clean it up / overwrite;
		//  - dup3(): same as dup2().
		//
		sinsp_fdinfo *oldfdinfo = evt.get_tinfo()->get_fd(retval);

		if(oldfdinfo != nullptr) {
			erase_fd_params eparams;

			eparams.m_fd = retval;
			eparams.m_fdinfo = oldfdinfo;
			eparams.m_remove_from_table = false;
			eparams.m_tinfo = evt.get_tinfo();
			erase_fd(eparams, verdict);
		}

		//
		// If we are handling the dup3() event exit then we add the flags to the new file
		// descriptor.
		//
		if(evt.get_type() == PPME_SYSCALL_DUP3_X) {
			uint32_t flags;

			//
			// Get the flags parameter.
			//
			flags = evt.get_param(3)->as<uint32_t>();

			//
			// We keep the previously flags that has been set on the original file descriptor and
			// just set/reset O_CLOEXEC flag base on the value received by dup3() syscall.
			//
			if(flags) {
				//
				// set the O_CLOEXEC flag.
				//
				evt.get_fd_info()->m_openflags |= flags;
			} else {
				//
				// reset the O_CLOEXEC flag.
				//
				evt.get_fd_info()->m_openflags &= ~PPM_O_CLOEXEC;
			}
		}

		//
		// Add the new fd to the table.
		//
		auto fdi = evt.get_fd_info()->clone();
		evt.set_fd_info(evt.get_tinfo()->add_fd(retval, std::move(fdi)));
	}
}

void sinsp_parser::parse_single_param_fd_exit(sinsp_evt &evt, const scap_fd_type type) const {
	if(evt.get_tinfo() == nullptr) {
		return;
	}

	const int64_t retval = evt.get_syscall_return_value();
	if(retval < 0) {
		return;
	}

	// Populate the new fd info.
	auto fdi = m_fdinfo_factory.create();
	fdi->m_type = type;

	if(evt.get_type() == PPME_SYSCALL_INOTIFY_INIT1_X) {
		fdi->m_openflags = evt.get_param(1)->as<uint16_t>();
	}

	if(evt.get_type() == PPME_SYSCALL_SIGNALFD4_X) {
		fdi->m_openflags = evt.get_param(1)->as<uint16_t>();
	}

	// Add the fd to the table.
	evt.set_fd_info(evt.get_tinfo()->add_fd(retval, std::move(fdi)));
}

void sinsp_parser::parse_getrlimit_setrlimit_exit(sinsp_evt &evt) const {
	if(evt.get_tinfo() == nullptr || evt.get_syscall_return_value() < 0) {
		return;
	}

	// Extract the resource number.
	const uint8_t resource = evt.get_param(3)->as<uint8_t>();
	if(resource != PPM_RLIMIT_NOFILE) {
		return;
	}

	// Extract the current value for the resource.
	int64_t curval = evt.get_param(1)->as<uint64_t>();
	if(curval != -1) {
		auto main_thread = evt.get_tinfo()->get_main_thread();
		if(main_thread == nullptr) {
			return;
		}
		main_thread->m_fdlimit = curval;
	} else {
		ASSERT(false);
	}
}

void sinsp_parser::parse_prlimit_exit(sinsp_evt &evt) const {
	int64_t newcur;
	int64_t tid;

	//
	// Check if the syscall was successful
	//
	if(evt.get_syscall_return_value() != 0) {
		return;
	}

	//
	// Extract the resource number
	//
	const sinsp_evt_param *resource = evt.get_param(6);
	if(resource->empty()) {
		return;
	}

	if(resource->as<uint8_t>() == PPM_RLIMIT_NOFILE) {
		//
		// Extract the current value for the resource
		//
		newcur = evt.get_param(1)->as<uint64_t>();
		if(newcur == -1) {
			return;
		}

		//
		// Extract the tid and look for its process info
		//
		const sinsp_evt_param *tid_evt = evt.get_param(5);
		if(tid_evt->empty()) {
			return;
		}
		tid = tid_evt->as<int64_t>();
		if(tid == 0) {
			tid = evt.get_tid();
		}

		sinsp_threadinfo *ptinfo = m_thread_manager->get_thread_ref(tid, true, true).get();
		/* If the thread info is invalid we cannot recover the main thread because we don't
		 * even have the `pid` of the thread.
		 */
		if(ptinfo == nullptr || ptinfo->is_invalid()) {
			return;
		}

		//
		// update the process fdlimit
		//
		auto main_thread = ptinfo->get_main_thread();
		if(main_thread == nullptr) {
			return;
		}
		main_thread->m_fdlimit = newcur;
	}
}

void sinsp_parser::parse_fcntl_exit(sinsp_evt &evt) const {
	if(evt.get_tinfo() == nullptr) {
		return;
	}

	//
	// Extract the return value
	//
	const int64_t retval = evt.get_syscall_return_value();

	//
	// Extract the command
	//
	const auto cmd = evt.get_param(2)->as<int8_t>();

	//
	// If not a F_DUPFD or F_DUPFD_CLOEXEC command, ignore the event
	//
	if(!(cmd == PPM_FCNTL_F_DUPFD || cmd == PPM_FCNTL_F_DUPFD_CLOEXEC)) {
		return;
	}

	//
	// Check if the syscall was successful
	//
	if(retval >= 0) {
		if(evt.get_fd_info() == nullptr) {
			return;
		}

		//
		// Add the new fd to the table.
		// NOTE: dup2 and dup3 accept an existing FD and in that case they close it.
		//       For us it's ok to just overwrite it.
		//
		evt.set_fd_info(evt.get_tinfo()->add_fd(retval, evt.get_fd_info()->clone()));
	}
}

void sinsp_parser::parse_context_switch(sinsp_evt &evt) {
	if(evt.get_tinfo() == nullptr) {
		return;
	}

	evt.get_tinfo()->m_pfmajor = evt.get_param(1)->as<uint64_t>();

	evt.get_tinfo()->m_pfminor = evt.get_param(2)->as<uint64_t>();

	auto main_tinfo = evt.get_tinfo()->get_main_thread();
	if(main_tinfo) {
		main_tinfo->m_vmsize_kb = evt.get_param(3)->as<uint32_t>();

		main_tinfo->m_vmrss_kb = evt.get_param(4)->as<uint32_t>();

		main_tinfo->m_vmswap_kb = evt.get_param(5)->as<uint32_t>();
	}
}

void sinsp_parser::parse_brk_mmap_mmap2_munmap__exit(sinsp_evt &evt) {
	if(evt.get_tinfo() == nullptr) {
		return;
	}

	evt.get_tinfo()->m_vmsize_kb = evt.get_param(1)->as<uint32_t>();
	evt.get_tinfo()->m_vmrss_kb = evt.get_param(2)->as<uint32_t>();
	evt.get_tinfo()->m_vmswap_kb = evt.get_param(3)->as<uint32_t>();
}

void sinsp_parser::set_evt_thread_user(sinsp_evt &evt, const sinsp_evt_param &euid_param) const {
	if(euid_param.empty()) {
		return;
	}

	sinsp_threadinfo *ti = evt.get_thread_info();
	if(ti == nullptr) {
		return;
	}

	ti->set_user(euid_param.as<uint32_t>(), must_notify_thread_user_update());
}

void sinsp_parser::set_evt_thread_group(sinsp_evt &evt, const sinsp_evt_param &egid_param) const {
	if(egid_param.empty()) {
		return;
	}

	sinsp_threadinfo *ti = evt.get_thread_info();
	if(ti == nullptr) {
		return;
	}

	ti->set_group(egid_param.as<uint32_t>(), must_notify_thread_group_update());
}

void sinsp_parser::parse_setresuid_exit(sinsp_evt &evt) const {
	if(evt.get_syscall_return_value() != 0) {
		return;
	}

	set_evt_thread_user(evt, *evt.get_param(2));
}

void sinsp_parser::parse_setreuid_exit(sinsp_evt &evt) const {
	if(evt.get_syscall_return_value() != 0) {
		return;
	}

	set_evt_thread_user(evt, *evt.get_param(2));
}

void sinsp_parser::parse_setresgid_exit(sinsp_evt &evt) const {
	if(evt.get_syscall_return_value() != 0) {
		return;
	}

	set_evt_thread_group(evt, *evt.get_param(2));
}

void sinsp_parser::parse_setregid_exit(sinsp_evt &evt) const {
	if(evt.get_syscall_return_value() != 0) {
		return;
	}

	set_evt_thread_group(evt, *evt.get_param(2));
}

void sinsp_parser::parse_setuid_exit(sinsp_evt &evt) const {
	if(evt.get_syscall_return_value() != 0) {
		return;
	}

	set_evt_thread_user(evt, *evt.get_param(1));
}

void sinsp_parser::parse_setgid_exit(sinsp_evt &evt) const {
	if(evt.get_syscall_return_value() != 0) {
		return;
	}

	set_evt_thread_group(evt, *evt.get_param(1));
}

void sinsp_parser::parse_user_evt(sinsp_evt &evt) const {
	const auto uid = evt.get_param(0)->as<uint32_t>();
	const auto gid = evt.get_param(1)->as<uint32_t>();
	const auto name = evt.get_param(2)->as<std::string_view>();
	const auto home = evt.get_param(3)->as<std::string_view>();
	const auto shell = evt.get_param(4)->as<std::string_view>();
	const auto container_id = evt.get_param(5)->as<std::string_view>();

	if(evt.get_scap_evt()->type == PPME_USER_ADDED_E) {
		m_usergroup_manager->add_user(std::string(container_id), -1, uid, gid, name, home, shell);
	} else {
		m_usergroup_manager->rm_user(std::string(container_id), uid);
	}
}

void sinsp_parser::parse_group_evt(sinsp_evt &evt) const {
	const auto gid = evt.get_param(0)->as<uint32_t>();
	const auto name = evt.get_param(1)->as<std::string_view>();
	const auto container_id = evt.get_param(2)->as<std::string_view>();

	if(evt.get_scap_evt()->type == PPME_GROUP_ADDED_E) {
		m_usergroup_manager->add_group(container_id.data(), -1, gid, name.data());
	} else {
		m_usergroup_manager->rm_group(container_id.data(), gid);
	}
}

void sinsp_parser::parse_cpu_hotplug_enter(sinsp_evt &evt) const {
	if(m_sinsp_mode.is_live() || is_syscall_plugin_enabled()) {
		throw sinsp_exception("CPU " + evt.get_param_value_str("cpu") +
		                      " configuration change detected. Aborting.");
	}
}

void sinsp_parser::parse_prctl_exit_event(sinsp_evt &evt) {
	/* Parameter 1: res (type: PT_ERRNO) */
	const int64_t retval = evt.get_syscall_return_value();

	if(retval < 0) {
		/* we are not interested in parsing something if the syscall fails */
		return;
	}

	/* prctl could be called by the main thread but also by a secondary thread */
	auto caller_tinfo = evt.get_thread_info();
	/* only invalid threads have `caller_tinfo->m_tginfo == nullptr` */
	if(caller_tinfo == nullptr || caller_tinfo->is_invalid()) {
		return;
	}

	bool child_subreaper = false;

	/* Parameter 2: option (type: PT_ENUMFLAGS32) */
	uint32_t option = evt.get_param(1)->as<uint32_t>();
	switch(option) {
	case PPM_PR_SET_CHILD_SUBREAPER:
		/* Parameter 4: arg2_int (type: PT_INT64) */
		/* If the user provided an arg2 != 0, we set the child_subreaper
		 * attribute for the calling process. If arg2 is zero, unset the attribute
		 */
		child_subreaper = (evt.get_param(3)->as<int64_t>()) != 0 ? true : false;
		caller_tinfo->m_tginfo->set_reaper(child_subreaper);
		break;

	case PPM_PR_GET_CHILD_SUBREAPER:
		/* Parameter 4: arg2_int (type: PT_INT64) */
		/* arg2 != 0 means the calling process is a child_subreaper */
		child_subreaper = (evt.get_param(3)->as<int64_t>()) != 0 ? true : false;
		caller_tinfo->m_tginfo->set_reaper(child_subreaper);
		break;

	default:
		break;
	}
}

uint8_t *sinsp_parser::reserve_event_buffer() {
	if(m_tmp_events_buffer.empty()) {
		return (uint8_t *)malloc(sizeof(uint8_t) * SP_EVT_BUF_SIZE);
	} else {
		auto ptr = m_tmp_events_buffer.top();
		m_tmp_events_buffer.pop();
		return ptr;
	}
}

void sinsp_parser::parse_chroot_exit(sinsp_evt &evt) {
	if(evt.get_tinfo() == nullptr) {
		return;
	}

	const int64_t retval = evt.get_syscall_return_value();
	if(retval == 0) {
		const char *resolved_path;
		auto path = evt.get_param_as_str(1, &resolved_path);
		if(resolved_path[0] == 0) {
			evt.get_tinfo()->m_root = path;
		} else {
			evt.get_tinfo()->m_root = resolved_path;
		}
	}
}

void sinsp_parser::parse_setsid_exit(sinsp_evt &evt) {
	//
	// Extract the return value
	//
	const int64_t retval = evt.get_syscall_return_value();

	if(retval >= 0) {
		if(evt.get_thread_info()) {
			evt.get_thread_info()->m_sid = retval;
		}
	}
}

void sinsp_parser::parse_getsockopt_exit(sinsp_evt &evt, sinsp_parser_verdict &verdict) const {
	const sinsp_evt_param *parinfo;
	int64_t retval;
	int64_t fd;
	int8_t level, optname;

	if(evt.get_tinfo() == nullptr) {
		return;
	}

	fd = evt.get_param(1)->as<int64_t>();

	evt.get_tinfo()->m_lastevent_fd = fd;

	// right now we only parse getsockopt() for SO_ERROR options
	// if that ever changes, move this check inside
	// the `if (level == PPM_SOCKOPT_LEVEL_SOL_SOCKET ...)` block
	if(!m_track_connection_status) {
		return;
	}

	//
	// Extract the return value
	//
	retval = evt.get_syscall_return_value();

	if(retval < 0) {
		return;
	}

	level = evt.get_param(2)->as<int8_t>();

	optname = evt.get_param(3)->as<int8_t>();

	if(level == PPM_SOCKOPT_LEVEL_SOL_SOCKET && optname == PPM_SOCKOPT_SO_ERROR) {
		auto main_thread = evt.get_tinfo()->get_main_thread();
		if(main_thread == nullptr) {
			return;
		}
		evt.set_fd_info(main_thread->get_fd(fd));
		if(!evt.get_fd_info()) {
			return;
		}

		parinfo = evt.get_param(4);
		ASSERT(*parinfo->data() == PPM_SOCKOPT_IDX_ERRNO);
		ASSERT(parinfo->len() == sizeof(int64_t) + 1);
		const auto err = *reinterpret_cast<const int64_t *>(
		        parinfo->data() + 1);  // add 1 byte to skip over PT_DYN param index

		evt.set_errorcode(static_cast<int32_t>(err));
		if(err < 0) {
			evt.get_fd_info()->set_socket_failed();
		} else {
			evt.get_fd_info()->set_socket_connected();
		}
		//
		// If there's a listener, add a callback to later invoke it.
		//
		if(m_observer) {
			verdict.add_post_process_cbs([](sinsp_observer *observer, sinsp_evt *evt) {
				observer->on_socket_status_changed(evt);
			});
		}
	}
}

void sinsp_parser::parse_capset_exit(sinsp_evt &evt) {
	//
	// Extract the return value
	//
	const int64_t retval = evt.get_syscall_return_value();

	if(retval < 0 || evt.get_tinfo() == nullptr) {
		return;
	}

	const auto tinfo = evt.get_tinfo();

	//
	// Extract and update thread capabilities
	//
	tinfo->m_cap_inheritable = evt.get_param(1)->as<uint64_t>();

	tinfo->m_cap_permitted = evt.get_param(2)->as<uint64_t>();

	tinfo->m_cap_effective = evt.get_param(3)->as<uint64_t>();
}

void sinsp_parser::parse_unshare_setns_exit(sinsp_evt &evt) {
	if(evt.get_syscall_return_value() < 0 || evt.get_tinfo() == nullptr) {
		return;
	}

	const auto etype = evt.get_scap_evt()->type;

	// Retrieve flags.
	uint32_t flags = 0;
	if(etype == PPME_SYSCALL_UNSHARE_X) {
		flags = evt.get_param(1)->as<uint32_t>();
	} else if(etype == PPME_SYSCALL_SETNS_X) {
		flags = evt.get_param(2)->as<uint32_t>();
	}

	// Update capabilities.
	if((flags & PPM_CL_CLONE_NEWUSER) == 0) {
		return;
	}

	const auto tinfo = evt.get_tinfo();
	const auto max_caps = sinsp_utils::get_max_caps();
	tinfo->m_cap_inheritable = max_caps;
	tinfo->m_cap_permitted = max_caps;
	tinfo->m_cap_effective = max_caps;
}

void sinsp_parser::free_event_buffer(uint8_t *ptr) {
	if(m_tmp_events_buffer.size() < m_thread_manager->get_threads()->size()) {
		m_tmp_events_buffer.push(ptr);
	} else {
		free(ptr);
	}
}

void sinsp_parser::parse_memfd_create_exit(sinsp_evt &evt, const scap_fd_type type) const {
	int64_t fd;
	uint32_t flags;

	if(evt.get_tinfo() == nullptr) {
		return;
	}

	/* ret (fd) */
	fd = evt.get_syscall_return_value();

	/* name */
	/*
	Suppose you create a memfd named libstest resulting in a fd.name libstest while on disk
	(e.g. ls -l /proc/$PID/fd/$FD_NUM) it may look like /memfd:libstest (deleted)
	*/
	auto name = evt.get_param(1)->as<std::string_view>();

	/* flags */
	flags = evt.get_param(2)->as<uint32_t>();

	auto fdi = m_fdinfo_factory.create();
	if(fd >= 0) {
		fdi->m_type = type;
		fdi->add_filename(name);
		fdi->m_openflags = flags;
	}

	evt.set_fd_info(evt.get_tinfo()->add_fd(fd, std::move(fdi)));
}

void sinsp_parser::parse_pidfd_open_exit(sinsp_evt &evt) const {
	int64_t fd;
	int64_t pid;
	int64_t flags;

	if(evt.get_tinfo() == nullptr) {
		return;
	}

	/* ret (fd) */
	fd = evt.get_syscall_return_value();

	/* pid (fd) */
	ASSERT(evt.get_param_info(1)->type == PT_PID);
	pid = evt.get_param(1)->as<int64_t>();

	/* flags */
	flags = evt.get_param(2)->as<uint32_t>();

	auto fdi = m_fdinfo_factory.create();
	if(fd >= 0) {
		// note: approximating equivalent filename as in:
		// https://man7.org/linux/man-pages/man2/pidfd_getfd.2.html
		std::string fname = std::string(scap_get_host_root()) + "/proc/" + std::to_string(pid);
		fdi->m_type = scap_fd_type::SCAP_FD_PIDFD;
		fdi->add_filename(fname);
		fdi->m_openflags = flags;
		fdi->m_pid = pid;
	}

	evt.set_fd_info(evt.get_tinfo()->add_fd(fd, std::move(fdi)));
}

void sinsp_parser::parse_pidfd_getfd_exit(sinsp_evt &evt) const {
	int64_t fd;
	int64_t pidfd;
	int64_t targetfd;

	if(evt.get_tinfo() == nullptr) {
		return;
	}

	/* ret (fd) */
	fd = evt.get_syscall_return_value();

	/* pidfd */
	ASSERT(evt.get_param_info(1)->type == PT_FD);
	pidfd = evt.get_param(1)->as<int64_t>();

	/* targetfd */
	ASSERT(evt.get_param_info(2)->type == PT_FD);
	targetfd = evt.get_param(2)->as<int64_t>();

	/* flags */
	// currently unused: https://man7.org/linux/man-pages/man2/pidfd_getfd.2.html

	auto pidfd_fdinfo = evt.get_tinfo()->get_fd(pidfd);
	if(pidfd_fdinfo == nullptr || !pidfd_fdinfo->is_pidfd()) {
		return;
	}

	auto pidfd_tinfo = m_thread_manager->get_thread_ref(pidfd_fdinfo->m_pid);
	if(pidfd_tinfo == nullptr) {
		return;
	}

	auto targetfd_fdinfo = pidfd_tinfo->get_fd(targetfd);
	if(targetfd_fdinfo == nullptr) {
		return;
	}
	evt.get_tinfo()->add_fd(fd, targetfd_fdinfo->clone());
}
