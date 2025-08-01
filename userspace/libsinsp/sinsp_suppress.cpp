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

#include <cstring>

#include <libsinsp/sinsp_suppress.h>
#include <libsinsp/sinsp_exception.h>
#include <libsinsp/logger.h>
#include <driver/ppm_events_public.h>
#include <libscap/scap_const.h>
#include <libscap/scap_assert.h>

void libsinsp::sinsp_suppress::suppress_comm(const std::string &comm) {
	m_suppressed_comms.emplace(comm);
}

void libsinsp::sinsp_suppress::suppress_tid(uint64_t tid) {
	m_suppressed_tids.emplace(tid);
}

void libsinsp::sinsp_suppress::clear_suppress_comm() {
	m_suppressed_comms.clear();
}

void libsinsp::sinsp_suppress::clear_suppress_tid() {
	m_suppressed_tids.clear();
}

bool libsinsp::sinsp_suppress::check_suppressed_comm(uint64_t tid,
                                                     uint64_t parent_tid,
                                                     const std::string &comm) {
	handle_thread(tid, parent_tid, comm);

	if(m_suppressed_comms.find(comm) != m_suppressed_comms.end()) {
		m_suppressed_tids.insert(tid);
		m_num_suppressed_events++;
		return true;
	}
	return false;
}

int32_t libsinsp::sinsp_suppress::process_event(scap_evt *e) {
	if(m_suppressed_tids.empty() && m_suppressed_comms.empty()) {
		// nothing to suppress
		return SCAP_SUCCESS;
	}

	// For events that can create a new tid (fork, vfork, clone),
	// we need to check the comm, which might also update the set
	// of suppressed tids.

	uint64_t tid;
	memcpy(&tid, &e->tid, sizeof(uint64_t));

	switch(e->type) {
	case PPME_SYSCALL_CLONE_20_X:
	case PPME_SYSCALL_FORK_20_X:
	case PPME_SYSCALL_VFORK_20_X:
	case PPME_SYSCALL_EXECVE_19_X:
	case PPME_SYSCALL_EXECVEAT_X:
	case PPME_SYSCALL_CLONE3_X: {
		uint32_t j;
		const char *comm = nullptr;
		uint64_t *ptid_ptr = nullptr;

		auto *lens = (uint16_t *)((char *)e + sizeof(ppm_evt_hdr));
		char *valptr = (char *)lens + e->nparams * sizeof(uint16_t);
		uint16_t scratch = 0;

		ASSERT(e->nparams >= 14);
		if(e->nparams < 14) {
			// SCAP_SUCCESS means "do not suppress this event"
			return SCAP_SUCCESS;
		}

		// For all of these events, the comm is argument 14,
		// so we need to walk the list of params that far to
		// find the comm.
		for(j = 0; j < 13; j++) {
			if(j == 5) {
				ptid_ptr = (uint64_t *)valptr;
			}

			memcpy(&scratch, &lens[j], sizeof(uint16_t));
			valptr += scratch;
		}

		ASSERT(ptid_ptr != nullptr);
		if(ptid_ptr == nullptr) {
			// SCAP_SUCCESS means "do not suppress this event"
			return SCAP_SUCCESS;
		}

		comm = valptr;

		uint64_t ptid;
		memcpy(&ptid, ptid_ptr, sizeof(uint64_t));
		if(is_suppressed_tid(ptid)) {
			m_suppressed_tids.insert(tid);
			m_num_suppressed_events++;
			return SCAP_FILTERED_EVENT;
		}

		if(check_suppressed_comm(tid, ptid, comm)) {
			return SCAP_FILTERED_EVENT;
		}

		return SCAP_SUCCESS;
	}
	case PPME_PROCEXIT_1_E: {
		auto it = m_suppressed_tids.find(tid);
		if(it != m_suppressed_tids.end()) {
			// Given that the process is exiting, we remove the
			// tid from the suppressed tids.
			m_suppressed_tids.erase(it);
		}
		// We don't filter out procexit event otherwise
		// we'll keep stale threadinfo in the threadtable.
		return SCAP_SUCCESS;
	}

	default:
		if(is_suppressed_tid(tid)) {
			m_num_suppressed_events++;
			return SCAP_FILTERED_EVENT;
		} else {
			return SCAP_SUCCESS;
		}
	}
}

bool libsinsp::sinsp_suppress::is_suppressed_tid(uint64_t tid) const {
	if(tid == 0) {
		return false;
	}
	return m_suppressed_tids.find(tid) != m_suppressed_tids.end();
}
void libsinsp::sinsp_suppress::initialize() {
	// Defensive check — we expect m_tids_tree to be a nullptr
	if(m_tids_tree == nullptr) {
		m_tids_tree = std::make_unique<std::map<uint64_t, tid_tree_node>>();
	} else {
		m_tids_tree->clear();
	}
}

void libsinsp::sinsp_suppress::handle_thread(uint64_t tid,
                                             uint64_t parent_tid,
                                             const std::string &comm) {
	if(tid == 0) {
		return;
	}

	// Defensive check — this shouldn't happen under normal conditions.
	if(m_tids_tree == nullptr) {
		return;
	}

	// Add the comm to tid
	(*m_tids_tree)[tid].m_comm = comm;
	(*m_tids_tree)[tid].m_tid = tid;

	// Add child to parent
	(*m_tids_tree)[parent_tid].m_children.push_back(tid);
}

void libsinsp::sinsp_suppress::finalize() {
	// We generate the suppressed tids from the tid tree.
	// The tree is built during the /proc scan, so we can
	// use it to find all the children of a given tid.

	for(auto it = m_tids_tree->begin(); it != m_tids_tree->end(); it++) {
		auto &[_, node] = *it;

		if(is_suppressed_tid(node.m_tid)) {
			for(auto child_tid : node.m_children) {
				suppress_tid(child_tid);
			}
		}
	}
	// delete the map. We don't need it anymore.
	m_tids_tree.reset(nullptr);
}
