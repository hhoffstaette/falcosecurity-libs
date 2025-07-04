// SPDX-License-Identifier: GPL-2.0-only OR MIT
/*

Copyright (C) 2023 The Falco Authors.

This file is dual licensed under either the MIT or GPL 2. See MIT.txt
or GPL2.txt for full copies of the license.

*/
#ifndef __FILLER_HELPERS_H
#define __FILLER_HELPERS_H

#include <linux/compat.h>
#include <net/compat.h>
#include <net/sock.h>
#include <net/inet_sock.h>
#include <net/af_unix.h>
#include <linux/in.h>
#include <linux/fdtable.h>
#include <linux/net.h>

#include "ppm_flag_helpers.h"
#include "builtins.h"
#include "missing_definitions.h"

/* Helper used to please the verifier with operations on the number of arguments */
#define SAFE_ARG_NUMBER(x) x &(PPM_MAX_EVENT_PARAMS - 1)

/* This enum is used to tell our helpers if they have to
 * read from kernel or user memory.
 */
enum read_memory {
	USER = 0,
	KERNEL = 1,
};

static __always_inline struct inode *get_file_inode(struct file *file) {
	if(file) {
		return _READ(file->f_inode);
	}
	return NULL;
}

static __always_inline bool in_port_range(uint16_t port, uint16_t min, uint16_t max) {
	return port >= min && port <= max;
}

static __always_inline struct file *bpf_fget(int fd) {
	struct task_struct *task;
	struct files_struct *files;
	struct fdtable *fdt;
	int max_fds;
	struct file **fds;
	struct file *fil;

	task = (struct task_struct *)bpf_get_current_task();
	if(!task)
		return NULL;

	files = _READ(task->files);
	if(!files)
		return NULL;

	fdt = _READ(files->fdt);
	if(!fdt)
		return NULL;

	max_fds = _READ(fdt->max_fds);
	if(fd >= max_fds)
		return NULL;

	fds = _READ(fdt->fd);
	fil = _READ(fds[fd]);

	return fil;
}

static __always_inline uint32_t bpf_get_fd_fmode_created(int fd) {
	if(fd < 0) {
		return 0;
	}

/* FMODE_CREATED flag was introduced in kernel 4.19 and it's not present in earlier versions */
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 19, 0)
	struct file *file;
	file = bpf_fget(fd);
	if(file) {
		fmode_t fmode = _READ(file->f_mode);
		if(fmode & FMODE_CREATED)
			return PPM_O_F_CREATED;
	}
#endif
	return 0;
}

/* In this kernel version the instruction limit was bumped from 131072 to 1000000.
 * For this reason we use different values of `MAX_NUM_COMPONENTS`
 * according to the kernel version.
 */
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0))
#define MAX_NUM_COMPONENTS 48
#else
#define MAX_NUM_COMPONENTS 24
#endif

/* We must always leave at least 4096 bytes free in our tmp scratch space
 * to please the verifier since we set the max component len to 4096 bytes.
 * We start writing our exepath from half of the tmp scratch space. The
 * whole space is 256 KB, we start at 128 KB.
 *
 *       128 KB           128 KB (Free space to please the verifier)
 * |----------------|----------------|
 *                  ^
 *                  |
 *        We start here and write backward
 *        as we find components of the path
 *
 * As a bitmask we use `SAFE_TMP_SCRATCH_ACCESS` (128*1024 - 1).
 * This helps the verifier to understand that our offset never overcomes
 * 128 KB.
 */
#define MAX_COMPONENT_LEN 4096
#define MAX_TMP_SCRATCH_LEN (SCRATCH_SIZE >> 1)
#define SAFE_TMP_SCRATCH_ACCESS(x) (x) & (MAX_TMP_SCRATCH_LEN - 1)

/* Please note: Kernel 5.10 introduced a new bpf_helper called `bpf_d_path`
 * to extract a file path starting from a struct* file but it can be used only
 * with specific hooks:
 *
 * https://github.com/torvalds/linux/blob/e0dccc3b76fb35bb257b4118367a883073d7390e/kernel/trace/bpf_trace.c#L915-L929.
 *
 * So we need to do it by hand emulating its behavior.
 * This brings some limitations:
 * 1. the number of path components is limited to `MAX_NUM_COMPONENTS`.
 * 2. we cannot use locks so we can face race conditions during the path reconstruction.
 * 3. reconstructed path could be slightly different from the one returned by `d_path`.
 *    See pseudo_filesystem prefixes or the " (deleted)" suffix.
 *
 * Take a look at the research that led to this implementation:
 * https://github.com/falcosecurity/libs/issues/1111
 */
static __always_inline char *bpf_d_path_approx(struct filler_data *data, struct path *path) {
	struct path f_path = {};
	bpf_probe_read_kernel(&f_path, sizeof(struct path), path);
	struct dentry *dentry = f_path.dentry;
	struct vfsmount *vfsmnt = f_path.mnt;
	struct mount *mnt_p = container_of(vfsmnt, struct mount, mnt);
	struct mount *mnt_parent_p = NULL;
	bpf_probe_read_kernel(&mnt_parent_p, sizeof(struct mount *), &(mnt_p->mnt_parent));
	struct dentry *mnt_root_p = NULL;
	bpf_probe_read_kernel(&mnt_root_p, sizeof(struct dentry *), &(vfsmnt->mnt_root));

	/* This is the max length of the buffer in which we will write the full path. */
	uint32_t max_buf_len = MAX_TMP_SCRATCH_LEN;

	/* Populated inside the loop */
	struct dentry *d_parent = NULL;
	struct qstr d_name = {};
	uint32_t current_off = 0;
	int effective_name_len = 0;
	char slash = '/';
	char terminator = '\0';

#pragma unroll
	for(int i = 0; i < MAX_NUM_COMPONENTS; i++) {
		bpf_probe_read_kernel(&d_parent, sizeof(struct dentry *), &(dentry->d_parent));
		if(dentry == d_parent && dentry != mnt_root_p) {
			/* We reached the root (dentry == d_parent)
			 * but not the mount root...there is something weird, stop here.
			 */
			break;
		}

		if(dentry == mnt_root_p) {
			if(mnt_p != mnt_parent_p) {
				/* We reached root, but not global root - continue with mount point path */
				bpf_probe_read_kernel(&dentry, sizeof(struct dentry *), &mnt_p->mnt_mountpoint);
				bpf_probe_read_kernel(&mnt_p, sizeof(struct mount *), &mnt_p->mnt_parent);
				bpf_probe_read_kernel(&mnt_parent_p, sizeof(struct mount *), &mnt_p->mnt_parent);
				vfsmnt = &mnt_p->mnt;
				bpf_probe_read_kernel(&mnt_root_p, sizeof(struct dentry *), &(vfsmnt->mnt_root));
				continue;
			} else {
				/* We have the full path, stop here */
				break;
			}
		}

		/* Get the dentry name */
		bpf_probe_read_kernel(&d_name, sizeof(struct qstr), &(dentry->d_name));

		/* +1 for the terminator that is not considered in d_name.len.
		 * Reserve space for the name trusting the len
		 * written in `qstr` struct
		 */
		current_off = max_buf_len - (d_name.len + 1);

		effective_name_len = bpf_probe_read_kernel_str(
		        &(data->tmp_scratch[SAFE_TMP_SCRATCH_ACCESS(current_off)]),
		        MAX_COMPONENT_LEN,
		        (void *)d_name.name);

		/* This check shouldn't be necessary, right now we
		 * keep it just to be extra safe. Unfortunately, it causes
		 * verifier issues on s390x (5.15.0-75-generic Ubuntu s390x)
		 */
#ifndef CONFIG_S390
		if(effective_name_len <= 1) {
			/* If effective_name_len is 0 or 1 we have an error
			 * (path can't be null nor an empty string)
			 */
			break;
		}
#endif
		/* 1. `max_buf_len -= 1` point to the `\0` of the just written name.
		 * 2. We replace it with a `/`. Note that we have to use `bpf_probe_read_kernel`
		 *    to please some old verifiers like (Oracle Linux 4.14).
		 * 3. Then we set `max_buf_len` to the last written char.
		 */
		max_buf_len -= 1;
		bpf_probe_read_kernel(&(data->tmp_scratch[SAFE_TMP_SCRATCH_ACCESS(max_buf_len)]),
		                      1,
		                      &slash);
		max_buf_len -= (effective_name_len - 1);

		dentry = d_parent;
	}

	if(max_buf_len == MAX_TMP_SCRATCH_LEN) {
		/* memfd files have no path in the filesystem so we never decremented the `max_buf_len` */
		bpf_probe_read_kernel(&d_name, sizeof(struct qstr), &(dentry->d_name));
		bpf_probe_read_kernel_str(&(data->tmp_scratch[0]), MAX_COMPONENT_LEN, (void *)d_name.name);
		return data->tmp_scratch;
	}

	/* Add leading slash */
	max_buf_len -= 1;
	bpf_probe_read_kernel(&(data->tmp_scratch[SAFE_TMP_SCRATCH_ACCESS(max_buf_len)]), 1, &slash);

	/* Null terminate the path string.
	 * Replace the first `/` we added in the loop with `\0`
	 */
	bpf_probe_read_kernel(&(data->tmp_scratch[SAFE_TMP_SCRATCH_ACCESS(MAX_TMP_SCRATCH_LEN - 1)]),
	                      1,
	                      &terminator);

	return &(data->tmp_scratch[SAFE_TMP_SCRATCH_ACCESS(max_buf_len)]);
}

static __always_inline struct socket *bpf_sockfd_lookup(struct filler_data *data, int fd) {
	struct file *file;
	const struct file_operations *fop;
	struct socket *sock;

	if(!data->settings->socket_file_ops)
		return NULL;

	file = bpf_fget(fd);
	if(!file)
		return NULL;

	fop = _READ(file->f_op);
	if(fop != data->settings->socket_file_ops)
		return NULL;

	sock = _READ(file->private_data);
	return sock;
}

static __always_inline unsigned long bpf_encode_dev(dev_t dev) {
	unsigned int major = MAJOR(dev);
	unsigned int minor = MINOR(dev);

	return (minor & 0xff) | (major << 8) | ((minor & ~0xff) << 12);
}

static __always_inline void bpf_get_ino_from_fd(int fd, unsigned long *ino) {
	struct super_block *sb;
	struct inode *inode;
	struct file *file;
	dev_t kdev;

	if(fd < 0)
		return;

	file = bpf_fget(fd);
	if(!file)
		return;

	inode = _READ(file->f_inode);
	if(!inode)
		return;

	*ino = _READ(inode->i_ino);
}

static __always_inline enum ppm_overlay get_overlay_layer(struct file *file) {
	if(!file) {
		return PPM_NOT_OVERLAY_FS;
	}
	struct dentry *dentry = NULL;
	bpf_probe_read_kernel(&dentry, sizeof(dentry), &file->f_path.dentry);
	struct super_block *sb = (struct super_block *)_READ(dentry->d_sb);
	unsigned long sb_magic = _READ(sb->s_magic);

	if(sb_magic != PPM_OVERLAYFS_SUPER_MAGIC) {
		return PPM_NOT_OVERLAY_FS;
	}

	char *vfs_inode = (char *)_READ(dentry->d_inode);
	struct dentry *upper_dentry = NULL;
	bpf_probe_read_kernel(&upper_dentry,
	                      sizeof(upper_dentry),
	                      (char *)vfs_inode + sizeof(struct inode));
	if(!upper_dentry) {
		return PPM_OVERLAY_LOWER;
	}

	struct inode *upper_ino = _READ(upper_dentry->d_inode);
	if(_READ(upper_ino->i_ino) != 0) {
		return PPM_OVERLAY_UPPER;
	} else {
		return PPM_OVERLAY_LOWER;
	}
}

static __always_inline void bpf_get_dev_ino_overlay_from_fd(int fd,
                                                            unsigned long *dev,
                                                            unsigned long *ino,
                                                            enum ppm_overlay *ol) {
	struct super_block *sb;
	struct inode *inode;
	dev_t kdev;
	struct file *file;

	if(fd < 0)
		return;

	file = bpf_fget(fd);
	if(!file)
		return;

	*ol = get_overlay_layer(file);

	inode = _READ(file->f_inode);
	if(!inode)
		return;

	sb = _READ(inode->i_sb);
	if(!sb)
		return;

	kdev = _READ(sb->s_dev);
	*dev = bpf_encode_dev(kdev);

	*ino = _READ(inode->i_ino);
}

static __always_inline bool bpf_ipv6_addr_any(const struct in6_addr *a) {
	const unsigned long *ul = (const unsigned long *)a;

	return (ul[0] | ul[1]) == 0UL;
}

static __always_inline bool bpf_getsockname(struct socket *sock,
                                            struct sockaddr_storage *addr,
                                            int peer) {
	struct sock *sk;
	sa_family_t family;

	sk = _READ(sock->sk);
	if(!sk)
		return false;

	family = _READ(sk->sk_family);

	switch(family) {
	case AF_INET: {
		struct inet_sock *inet = (struct inet_sock *)sk;
		struct sockaddr_in *sin = (struct sockaddr_in *)addr;

		sin->sin_family = AF_INET;
		if(peer) {
			sin->sin_port = _READ(inet->inet_dport);
			sin->sin_addr.s_addr = _READ(inet->inet_daddr);
		} else {
			uint32_t addr = _READ(inet->inet_rcv_saddr);

			if(!addr)
				addr = _READ(inet->inet_saddr);
			sin->sin_port = _READ(inet->inet_sport);
			sin->sin_addr.s_addr = addr;
		}

		break;
	}
	case AF_INET6: {
		struct sockaddr_in6 *sin = (struct sockaddr_in6 *)addr;
		struct inet_sock *inet = (struct inet_sock *)sk;
		struct ipv6_pinfo {
			struct in6_addr saddr;
		};
		struct ipv6_pinfo *np = (struct ipv6_pinfo *)_READ(inet->pinet6);

		sin->sin6_family = AF_INET6;
		if(peer) {
			sin->sin6_port = _READ(inet->inet_dport);
			sin->sin6_addr = _READ(sk->sk_v6_daddr);
		} else {
			sin->sin6_addr = _READ(sk->sk_v6_rcv_saddr);
			if(bpf_ipv6_addr_any(&sin->sin6_addr))
				sin->sin6_addr = _READ(np->saddr);
			sin->sin6_port = _READ(inet->inet_sport);
		}

		break;
	}
	case AF_UNIX: {
		struct sockaddr_un *sunaddr = (struct sockaddr_un *)addr;
		struct unix_sock *u;
		struct unix_address *addr;

		if(peer)
			sk = _READ(((struct unix_sock *)sk)->peer);

		u = (struct unix_sock *)sk;
		addr = _READ(u->addr);
		if(u && addr) {
			unsigned int len = _READ(addr->len);

			if(len > sizeof(struct sockaddr_storage))
				len = sizeof(struct sockaddr_storage);

#ifdef BPF_FORBIDS_ZERO_ACCESS
			if(len > 0)
				bpf_probe_read_kernel(sunaddr, ((len - 1) & 0xff) + 1, addr->name);
#else
			bpf_probe_read_kernel(sunaddr, len, addr->name);
#endif
		} else {
			sunaddr->sun_family = AF_UNIX;
			sunaddr->sun_path[0] = 0;
			// The first byte to 0 can be confused with an `abstract socket address` for this reason
			// we put also the second byte to 0 to comunicate to the caller that the address is not
			// valid.
			sunaddr->sun_path[1] = 0;
		}

		break;
	}
	default:
		return false;
	}

	return true;
}

static __always_inline int bpf_addr_to_kernel(void *uaddr, int ulen, struct sockaddr *kaddr) {
	int len = ulen & 0xfff; /* required by BPF verifier */

	if(len < 0 || len > sizeof(struct sockaddr_storage))
		return -EINVAL;
	if(len == 0)
		return 0;

#ifdef BPF_FORBIDS_ZERO_ACCESS
	if(bpf_probe_read_user(kaddr, ((len - 1) & 0xff) + 1, uaddr))
#else
	if(bpf_probe_read_user(kaddr, len & 0xff, uaddr))
#endif
		return -EFAULT;

	return 0;
}

#define get_buf(x) data->buf[(data->state->tail_ctx.curoff + (x)) & SCRATCH_SIZE_HALF]

static __always_inline uint32_t bpf_compute_snaplen(struct filler_data *data,
                                                    uint32_t lookahead_size) {
	uint32_t res = data->settings->snaplen;

	if(!data->settings->do_dynamic_snaplen)
		return res;

	// We set this in the previous syscall-specific logic
	if(data->fd == -1)
		return res;

	struct socket *socket = bpf_sockfd_lookup(data, data->fd);
	if(!socket)
		return res;

	struct sock *sk = _READ(socket->sk);
	if(!sk)
		return res;

	uint16_t port_local = 0;
	uint16_t port_remote = 0;

	uint16_t socket_family = _READ(sk->sk_family);
	if(socket_family == AF_INET || socket_family == AF_INET6) {
		struct inet_sock *inet = (struct inet_sock *)sk;
		port_local = _READ(inet->inet_sport);
		port_remote = _READ(sk->__sk_common.skc_dport);
		port_local = ntohs(port_local);
		port_remote = ntohs(port_remote);
		struct sockaddr *sockaddr = NULL;

		switch(data->state->tail_ctx.evt_type) {
		case PPME_SOCKET_SENDTO_X:
		case PPME_SOCKET_RECVFROM_X:
			sockaddr = (struct sockaddr *)bpf_syscall_get_argument(data, 4);
			break;

		case PPME_SOCKET_RECVMSG_X:
		case PPME_SOCKET_SENDMSG_X: {
			unsigned long mh_p = bpf_syscall_get_argument(data, 1);
#ifdef CONFIG_COMPAT
			if(bpf_in_ia32_syscall()) {
				struct compat_msghdr compat_mh = {};
				if(likely(bpf_probe_read_user(&compat_mh, sizeof(compat_mh), (void *)mh_p) == 0)) {
					sockaddr = (struct sockaddr *)(unsigned long)(compat_mh.msg_name);
				}
				// in any case we break the switch.
				break;
			}
#endif
			struct user_msghdr mh = {};
			if(bpf_probe_read_user(&mh, sizeof(mh), (void *)mh_p) == 0) {
				sockaddr = (struct sockaddr *)mh.msg_name;
			}
		} break;

		default:
			break;
		}

		if(port_remote == 0 && sockaddr != NULL) {
			if(socket_family == AF_INET) {
				struct sockaddr_in sockaddr_in = {};
				bpf_probe_read_user(&sockaddr_in, sizeof(sockaddr_in), sockaddr);
				port_remote = ntohs(sockaddr_in.sin_port);
			} else {
				struct sockaddr_in6 sockaddr_in6 = {};
				bpf_probe_read_user(&sockaddr_in6, sizeof(sockaddr_in6), sockaddr);
				port_remote = ntohs(sockaddr_in6.sin6_port);
			}
		}
	}

	uint16_t min_port = data->settings->fullcapture_port_range_start;
	uint16_t max_port = data->settings->fullcapture_port_range_end;

	if(max_port > 0 && (in_port_range(port_local, min_port, max_port) ||
	                    in_port_range(port_remote, min_port, max_port))) {
		return res > SNAPLEN_FULLCAPTURE_PORT ? res : SNAPLEN_FULLCAPTURE_PORT;
	} else if(port_remote == data->settings->statsd_port) {
		return res > SNAPLEN_EXTENDED ? res : SNAPLEN_EXTENDED;
	} else if(port_remote == PPM_PORT_DNS) {
		return res > SNAPLEN_DNS_UDP ? res : SNAPLEN_DNS_UDP;
	} else if((port_local == PPM_PORT_MYSQL || port_remote == PPM_PORT_MYSQL) &&
	          lookahead_size >= 5) {
		if((get_buf(0) == 3 || get_buf(1) == 3 || get_buf(2) == 3 || get_buf(3) == 3 ||
		    get_buf(4) == 3) ||
		   (get_buf(2) == 0 && get_buf(3) == 0)) {
			return res > SNAPLEN_EXTENDED ? res : SNAPLEN_EXTENDED;
		}
	} else if((port_local == PPM_PORT_POSTGRES || port_remote == PPM_PORT_POSTGRES) &&
	          lookahead_size >= 7) {
		if((get_buf(0) == 'Q' && get_buf(1) == 0) || /* SimpleQuery command */
		   (get_buf(0) == 'P' && get_buf(1) == 0) || /* Prepare statement command */
		   (get_buf(4) == 0 && get_buf(5) == 3 && get_buf(6) == 0) || /* startup command */
		   (get_buf(0) == 'E' && get_buf(1) == 0)                     /* error or execute command */
		) {
			return res > SNAPLEN_EXTENDED ? res : SNAPLEN_EXTENDED;
		}
	} else if((port_local == PPM_PORT_MONGODB || port_remote == PPM_PORT_MONGODB) ||
	          (lookahead_size >= 16 &&
	           (*(int32_t *)&get_buf(12) == 1 || /* matches header */
	            *(int32_t *)&get_buf(12) == 2001 || *(int32_t *)&get_buf(12) == 2002 ||
	            *(int32_t *)&get_buf(12) == 2003 || *(int32_t *)&get_buf(12) == 2004 ||
	            *(int32_t *)&get_buf(12) == 2005 || *(int32_t *)&get_buf(12) == 2006 ||
	            *(int32_t *)&get_buf(12) == 2007))) {
		return res > SNAPLEN_EXTENDED ? res : SNAPLEN_EXTENDED;
	} else if(lookahead_size >= 5) {
		uint32_t buf = *(uint32_t *)&get_buf(0);

#ifdef CONFIG_S390
		buf = __builtin_bswap32(buf);
#endif
		if(buf == BPF_HTTP_GET || buf == BPF_HTTP_POST || buf == BPF_HTTP_PUT ||
		   buf == BPF_HTTP_DELETE || buf == BPF_HTTP_TRACE || buf == BPF_HTTP_CONNECT ||
		   buf == BPF_HTTP_OPTIONS ||
		   (buf == BPF_HTTP_PREFIX &&
		    data->buf[(data->state->tail_ctx.curoff + 4) & SCRATCH_SIZE_HALF] == '/')) {  // "HTTP/"
			return res > SNAPLEN_EXTENDED ? res : SNAPLEN_EXTENDED;
		}
	}
	return res;
}

static __always_inline int unix_socket_path(char *dest, const char *user_ptr, size_t size) {
	int res = bpf_probe_read_kernel_str(dest, size, user_ptr);
	/*
	 * Extract from: https://man7.org/linux/man-pages/man7/unix.7.html
	 * an abstract socket address is distinguished (from a
	 * pathname socket) by the fact that sun_path[0] is a null byte
	 * ('\0').  The socket's address in this namespace is given by
	 * the additional bytes in sun_path that are covered by the
	 * specified length of the address structure.
	 */
	if(res == 1) {
		res = bpf_probe_read_kernel_str(dest,
		                                size - 1,  // account for '@'
		                                user_ptr + 1);
	}
	return res;
}

static __always_inline uint16_t bpf_pack_addr(struct filler_data *data,
                                              struct sockaddr *usrsockaddr,
                                              int ulen) {
	uint32_t ip;
	uint16_t port;
	sa_family_t family = usrsockaddr->sa_family;
	struct sockaddr_in *usrsockaddr_in;
	struct sockaddr_in6 *usrsockaddr_in6;
	struct sockaddr_un *usrsockaddr_un;
	uint16_t size;
	char *dest;
	int res;

	switch(family) {
	case AF_INET:
		/*
		 * Map the user-provided address to a sockaddr_in
		 */
		usrsockaddr_in = (struct sockaddr_in *)usrsockaddr;

		/*
		 * Retrieve the src address
		 */
		ip = usrsockaddr_in->sin_addr.s_addr;
		port = ntohs(usrsockaddr_in->sin_port);

		/*
		 * Pack the tuple info in the temporary buffer
		 */
		size = 1 + 4 + 2; /* family + ip + port */

		data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF] = socket_family_to_scap(family);
		memcpy(&data->buf[(data->state->tail_ctx.curoff + 1) & SCRATCH_SIZE_HALF], &ip, 4);
		memcpy(&data->buf[(data->state->tail_ctx.curoff + 5) & SCRATCH_SIZE_HALF], &port, 2);

		break;
	case AF_INET6:
		/*
		 * Map the user-provided address to a sockaddr_in
		 */
		usrsockaddr_in6 = (struct sockaddr_in6 *)usrsockaddr;

		/*
		 * Retrieve the src address
		 */
		port = ntohs(usrsockaddr_in6->sin6_port);

		/*
		 * Pack the tuple info in the temporary buffer
		 */
		size = 1 + 16 + 2; /* family + ip + port */

		data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF] = socket_family_to_scap(family);
		memcpy(&data->buf[(data->state->tail_ctx.curoff + 1) & SCRATCH_SIZE_HALF],
		       usrsockaddr_in6->sin6_addr.s6_addr,
		       16);
		memcpy(&data->buf[(data->state->tail_ctx.curoff + 17) & SCRATCH_SIZE_HALF], &port, 2);

		break;
	case AF_UNIX:
		/*
		 * Map the user-provided address to a sockaddr_in
		 */
		usrsockaddr_un = (struct sockaddr_un *)usrsockaddr;

		/*
		 * Put a 0 at the end of struct sockaddr_un because
		 * the user might not have considered it in the length
		 */
		if(ulen == sizeof(struct sockaddr_storage))
			((char *)usrsockaddr_un)[(ulen - 1) & SCRATCH_SIZE_MAX] = 0;
		else
			((char *)usrsockaddr_un)[ulen & SCRATCH_SIZE_MAX] = 0;

		/*
		 * Pack the data into the target buffer
		 */
		size = 1;

		data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF] = socket_family_to_scap(family);

		res = unix_socket_path(&data->buf[(data->state->tail_ctx.curoff + 1) & SCRATCH_SIZE_HALF],
		                       usrsockaddr_un->sun_path,
		                       UNIX_PATH_MAX);

		size += res;

		break;
	default:
		size = 0;
		break;
	}

	return size;
}

static __always_inline long bpf_fd_to_socktuple(struct filler_data *data,
                                                int fd,
                                                struct sockaddr *usrsockaddr,
                                                int ulen,
                                                bool use_userdata,
                                                bool is_inbound,
                                                char *tmp_area) {
	struct sockaddr_storage *sock_address;
	struct sockaddr_storage *peer_address;
	unsigned short family;
	struct socket *sock;
	struct sock *sk;
	long size = 0;
	struct in6_addr in6 = {0};

	sock = bpf_sockfd_lookup(data, fd);
	if(!sock)
		return 0;

	sock_address = (struct sockaddr_storage *)tmp_area;
	peer_address = (struct sockaddr_storage *)tmp_area + 1;

	if(!bpf_getsockname(sock, sock_address, 0))
		return 0;

	sk = _READ(sock->sk);
	if(!sk)
		return 0;

	family = _READ(sk->sk_family);

	switch(family) {
	case AF_INET: {
		uint32_t sip;
		uint32_t dip;
		uint16_t sport;
		uint16_t dport;

		if(!use_userdata) {
			if(bpf_getsockname(sock, peer_address, 1)) {
				if(is_inbound) {
					sip = ((struct sockaddr_in *)peer_address)->sin_addr.s_addr;
					sport = ntohs(((struct sockaddr_in *)peer_address)->sin_port);
					dip = ((struct sockaddr_in *)sock_address)->sin_addr.s_addr;
					dport = ntohs(((struct sockaddr_in *)sock_address)->sin_port);
				} else {
					sip = ((struct sockaddr_in *)sock_address)->sin_addr.s_addr;
					sport = ntohs(((struct sockaddr_in *)sock_address)->sin_port);
					dip = ((struct sockaddr_in *)peer_address)->sin_addr.s_addr;
					dport = ntohs(((struct sockaddr_in *)peer_address)->sin_port);
				}
			} else {
				sip = 0;
				sport = 0;
				dip = 0;
				dport = 0;
			}
		} else {
			struct sockaddr_in *usrsockaddr_in = (struct sockaddr_in *)usrsockaddr;

			if(is_inbound) {
				/* To take peer address info we try to use the kernel where possible.
				 * TCP allows us to obtain the right information, while the kernel doesn't fill
				 * `sk->__sk_common.skc_daddr` for UDP connection.
				 * Instead of having a custom logic for each protocol we try to read from
				 * kernel structs and if we don't find valid data we fallback to userspace
				 * structs.
				 */
				bpf_probe_read_kernel(&sport, sizeof(sport), &sk->__sk_common.skc_dport);
				if(sport != 0) {
					/* We can read from the kernel */
					bpf_probe_read_kernel(&sip, sizeof(sip), &sk->__sk_common.skc_daddr);
					sport = ntohs(sport);
				} else {
					/* Fallback to userspace struct */
					sip = usrsockaddr_in->sin_addr.s_addr;
					sport = ntohs(usrsockaddr_in->sin_port);
				}
				dip = ((struct sockaddr_in *)sock_address)->sin_addr.s_addr;
				dport = ntohs(((struct sockaddr_in *)sock_address)->sin_port);
			} else {
				sip = ((struct sockaddr_in *)sock_address)->sin_addr.s_addr;
				sport = ntohs(((struct sockaddr_in *)sock_address)->sin_port);
				dip = usrsockaddr_in->sin_addr.s_addr;
				dport = ntohs(usrsockaddr_in->sin_port);
			}
		}

		size = 1 + 4 + 4 + 2 + 2;

		data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF] = socket_family_to_scap(family);
		memcpy(&data->buf[(data->state->tail_ctx.curoff + 1) & SCRATCH_SIZE_HALF], &sip, 4);
		memcpy(&data->buf[(data->state->tail_ctx.curoff + 5) & SCRATCH_SIZE_HALF], &sport, 2);
		memcpy(&data->buf[(data->state->tail_ctx.curoff + 7) & SCRATCH_SIZE_HALF], &dip, 4);
		memcpy(&data->buf[(data->state->tail_ctx.curoff + 11) & SCRATCH_SIZE_HALF], &dport, 2);

		break;
	}
	case AF_INET6: {
		uint8_t *sip6;
		uint8_t *dip6;
		uint16_t sport;
		uint16_t dport;

		if(!use_userdata) {
			if(bpf_getsockname(sock, peer_address, 1)) {
				if(is_inbound) {
					sip6 = ((struct sockaddr_in6 *)peer_address)->sin6_addr.s6_addr;
					sport = ntohs(((struct sockaddr_in6 *)peer_address)->sin6_port);
					dip6 = ((struct sockaddr_in6 *)sock_address)->sin6_addr.s6_addr;
					dport = ntohs(((struct sockaddr_in6 *)sock_address)->sin6_port);
				} else {
					sip6 = ((struct sockaddr_in6 *)sock_address)->sin6_addr.s6_addr;
					sport = ntohs(((struct sockaddr_in6 *)sock_address)->sin6_port);
					dip6 = ((struct sockaddr_in6 *)peer_address)->sin6_addr.s6_addr;
					dport = ntohs(((struct sockaddr_in6 *)peer_address)->sin6_port);
				}
			} else {
				memset(peer_address, 0, 16);
				sip6 = (uint8_t *)peer_address;
				dip6 = (uint8_t *)peer_address;
				sport = 0;
				dport = 0;
			}
		} else {
			/*
			 * Map the user-provided address to a sockaddr_in6
			 */
			struct sockaddr_in6 *usrsockaddr_in6 = (struct sockaddr_in6 *)usrsockaddr;

			if(is_inbound) {
				bpf_probe_read_kernel(&sport, sizeof(sport), &sk->__sk_common.skc_dport);
				if(sport != 0) {
					/* We can read from the kernel */
					bpf_probe_read_kernel(&in6, sizeof(in6), &sk->__sk_common.skc_v6_daddr);
					sip6 = in6.in6_u.u6_addr8;
					sport = ntohs(sport);
				} else {
					/* Fallback to userspace struct */
					sip6 = usrsockaddr_in6->sin6_addr.s6_addr;
					sport = ntohs(usrsockaddr_in6->sin6_port);
				}
				dip6 = ((struct sockaddr_in6 *)sock_address)->sin6_addr.s6_addr;
				dport = ntohs(((struct sockaddr_in6 *)sock_address)->sin6_port);
			} else {
				sip6 = ((struct sockaddr_in6 *)sock_address)->sin6_addr.s6_addr;
				sport = ntohs(((struct sockaddr_in6 *)sock_address)->sin6_port);
				dip6 = usrsockaddr_in6->sin6_addr.s6_addr;
				dport = ntohs(usrsockaddr_in6->sin6_port);
			}
		}

		/*
		 * Pack the tuple info in the temporary buffer
		 */
		size = 1 + 16 + 16 + 2 + 2; /* family + sip + dip + sport + dport */

		data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF] = socket_family_to_scap(family);
		memcpy(&data->buf[(data->state->tail_ctx.curoff + 1) & SCRATCH_SIZE_HALF], sip6, 16);
		memcpy(&data->buf[(data->state->tail_ctx.curoff + 17) & SCRATCH_SIZE_HALF], &sport, 2);
		memcpy(&data->buf[(data->state->tail_ctx.curoff + 19) & SCRATCH_SIZE_HALF], dip6, 16);
		memcpy(&data->buf[(data->state->tail_ctx.curoff + 35) & SCRATCH_SIZE_HALF], &dport, 2);

		break;
	}
	case AF_UNIX: {
		/*
		 * Retrieve the addresses
		 */
		struct unix_sock *us = (struct unix_sock *)sk;
		struct sock *speer = _READ(us->peer);
		struct sockaddr_un *usrsockaddr_un;
		char *us_name = NULL;

		data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF] = socket_family_to_scap(family);

		if(is_inbound) {
			memcpy(&data->buf[(data->state->tail_ctx.curoff + 1) & SCRATCH_SIZE_HALF], &us, 8);
			memcpy(&data->buf[(data->state->tail_ctx.curoff + 1 + 8) & SCRATCH_SIZE_HALF],
			       &speer,
			       8);
			us_name = ((struct sockaddr_un *)sock_address)->sun_path;
		} else {
			memcpy(&data->buf[(data->state->tail_ctx.curoff + 1) & SCRATCH_SIZE_HALF], &speer, 8);
			memcpy(&data->buf[(data->state->tail_ctx.curoff + 1 + 8) & SCRATCH_SIZE_HALF], &us, 8);
			bpf_getsockname(sock, peer_address, 1);
			us_name = ((struct sockaddr_un *)peer_address)->sun_path;
			if(us_name[0] == '\0' && us_name[1] == '\0' && usrsockaddr != NULL) {
				usrsockaddr_un = (struct sockaddr_un *)usrsockaddr;
				us_name = usrsockaddr_un->sun_path;
			}
		}

		int res = unix_socket_path(
		        &data->buf[(data->state->tail_ctx.curoff + 1 + 8 + 8) & SCRATCH_SIZE_HALF],
		        us_name,
		        UNIX_PATH_MAX);
		size = 1 + 8 + 8 + res;
		break;
	}
	}

	return size;
}

static __always_inline int __bpf_read_val_into(struct filler_data *data,
                                               unsigned long curoff_bounded,
                                               unsigned long val,
                                               volatile uint16_t read_size,
                                               enum read_memory mem) {
	int rc;
	int read_size_bound;

#ifdef BPF_FORBIDS_ZERO_ACCESS
	if(read_size == 0)
		return -1;

	read_size_bound = ((read_size - 1) & SCRATCH_SIZE_HALF) + 1;
#else
	read_size_bound = read_size & SCRATCH_SIZE_HALF;
#endif

	if(mem == KERNEL)
		rc = bpf_probe_read_kernel(&data->buf[curoff_bounded], read_size_bound, (void *)val);
	else
		rc = bpf_probe_read_user(&data->buf[curoff_bounded], read_size_bound, (void *)val);

	return rc;
}

static __always_inline int __bpf_val_to_ring(struct filler_data *data,
                                             unsigned long val,
                                             unsigned long val_len,
                                             enum ppm_param_type type,
                                             uint8_t dyn_idx,
                                             bool enforce_snaplen,
                                             enum read_memory mem) {
	unsigned int len_dyn = 0;
	unsigned int len = 0;
	unsigned long curoff_bounded = 0;

	curoff_bounded = data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF;
	if(data->state->tail_ctx.curoff > SCRATCH_SIZE_HALF) {
		return PPM_FAILURE_FRAME_SCRATCH_MAP_FULL;
	}

	if(dyn_idx != (uint8_t)-1) {
		*((uint8_t *)&data->buf[curoff_bounded]) = dyn_idx;
		len_dyn = sizeof(uint8_t);
		data->state->tail_ctx.curoff += len_dyn;
		data->state->tail_ctx.len += len_dyn;
	}

	curoff_bounded = data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF;
	if(data->state->tail_ctx.curoff > SCRATCH_SIZE_HALF) {
		return PPM_FAILURE_FRAME_SCRATCH_MAP_FULL;
	}

	switch(type) {
	case PT_CHARBUF:
	case PT_FSPATH:
	case PT_FSRELPATH: {
		if(!data->curarg_already_on_frame) {
			int res = -1;

			if(val)
				/* Return `res<0` only in case of error. */
				res = (mem == KERNEL) ? bpf_probe_read_kernel_str(&data->buf[curoff_bounded],
				                                                  PPM_MAX_ARG_SIZE,
				                                                  (const void *)val)
				                      : bpf_probe_read_user_str(&data->buf[curoff_bounded],
				                                                PPM_MAX_ARG_SIZE,
				                                                (const void *)val);
			if(res >= 0) {
				len = res;
			} else {
				/* This should be already `0`, but just to be future-proof. */
				len = 0;
			}
		} else {
			len = val_len;
		}
		break;
	}
	case PT_BYTEBUF: {
		if(data->curarg_already_on_frame || (val && val_len)) {
			len = val_len;

			if(enforce_snaplen) {
				uint32_t dpi_lookahead_size = DPI_LOOKAHEAD_SIZE;
				unsigned int sl;

				if(dpi_lookahead_size > len) {
					dpi_lookahead_size = len;
				}

				if(!data->curarg_already_on_frame) {
					/* We need to read the first `dpi_lookahead_size` bytes.
					 * If we are not able to read at least `dpi_lookahead_size`
					 * we send an empty param `len=0`.
					 */
					volatile uint16_t read_size = dpi_lookahead_size;
					int rc = 0;

					rc = __bpf_read_val_into(data, curoff_bounded, val, read_size, mem);
					if(rc) {
						len = 0;
						break;
					}
				}

				/* If `curarg` was already on frame, we are interested only in this computation,
				 * so we can understand how many bytes of the `curarg` we have to consider.
				 */
				sl = bpf_compute_snaplen(data, dpi_lookahead_size);
				if(len > sl) {
					len = sl;
				}
			}

			if(len > PPM_MAX_ARG_SIZE)
				len = PPM_MAX_ARG_SIZE;

			if(!data->curarg_already_on_frame) {
				volatile uint16_t read_size = len;
				int rc = 0;

				curoff_bounded = data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF;
				if(data->state->tail_ctx.curoff > SCRATCH_SIZE_HALF) {
					return PPM_FAILURE_FRAME_SCRATCH_MAP_FULL;
				}

				rc = __bpf_read_val_into(data, curoff_bounded, val, read_size, mem);
				if(rc) {
					len = 0;
					break;
				}
			}
		} else {
			/* Handle NULL pointers */
			len = 0;
		}
		break;
	}
	case PT_SOCKADDR:
	case PT_SOCKTUPLE:
	case PT_FDLIST:
		if(data->curarg_already_on_frame) {
			len = val_len;
			break;
		}
		/* Cases in which we don't have the tuple and
		 * we want to send an empty param.
		 */
		else if(val == 0) {
			len = 0;
			break;
		}
		bpf_printk("expected arg already on frame: evt_type %d, curarg %d, type %d\n",
		           data->state->tail_ctx.evt_type,
		           data->state->tail_ctx.curarg,
		           type);
		return PPM_FAILURE_BUG;

	case PT_FLAGS8:
	case PT_ENUMFLAGS8:
	case PT_UINT8:
	case PT_SIGTYPE:
		*((uint8_t *)&data->buf[curoff_bounded]) = val;
		len = sizeof(uint8_t);
		break;
	case PT_FLAGS16:
	case PT_ENUMFLAGS16:
	case PT_UINT16:
	case PT_SYSCALLID:
		*((uint16_t *)&data->buf[curoff_bounded]) = val;
		len = sizeof(uint16_t);
		break;
	case PT_FLAGS32:
	case PT_MODE:
	case PT_UINT32:
	case PT_UID:
	case PT_GID:
	case PT_SIGSET:
	case PT_ENUMFLAGS32:
		*((uint32_t *)&data->buf[curoff_bounded]) = val;
		len = sizeof(uint32_t);
		break;
	case PT_RELTIME:
	case PT_ABSTIME:
	case PT_UINT64:
		*((uint64_t *)&data->buf[curoff_bounded]) = val;
		len = sizeof(uint64_t);
		break;
	case PT_INT8:
		*((int8_t *)&data->buf[curoff_bounded]) = val;
		len = sizeof(int8_t);
		break;
	case PT_INT16:
		*((int16_t *)&data->buf[curoff_bounded]) = val;
		len = sizeof(int16_t);
		break;
	case PT_INT32:
		*((int32_t *)&data->buf[curoff_bounded]) = val;
		len = sizeof(int32_t);
		break;
	case PT_INT64:
	case PT_ERRNO:
	case PT_FD:
	case PT_PID:
		*((int64_t *)&data->buf[curoff_bounded]) = val;
		len = sizeof(int64_t);
		break;
	default: {
		bpf_printk("unhandled type in bpf_val_to_ring: evt_type %d, curarg %d, type %d\n",
		           data->state->tail_ctx.evt_type,
		           data->state->tail_ctx.curarg,
		           type);
		return PPM_FAILURE_BUG;
	}
	}
	if(len_dyn + len > PPM_MAX_ARG_SIZE) {
		return PPM_FAILURE_FRAME_SCRATCH_MAP_FULL;
	}

	fixup_evt_arg_len(data->buf, data->state->tail_ctx.curarg, len_dyn + len);
	data->state->tail_ctx.curoff += len;
	data->state->tail_ctx.len += len;
	data->curarg_already_on_frame = false;
	++data->state->tail_ctx.curarg;

	return PPM_SUCCESS;
}

static __always_inline int bpf_push_empty_param(struct filler_data *data) {
	/* We push 0 in the length array */
	fixup_evt_arg_len(data->buf, data->state->tail_ctx.curarg, 0);
	data->curarg_already_on_frame = false;

	/* We increment the current argument - to make verifier happy, properly check it */
	data->state->tail_ctx.curarg = SAFE_ARG_NUMBER(data->state->tail_ctx.curarg + 1);
	return PPM_SUCCESS;
}

static __always_inline enum read_memory param_type_to_mem(enum ppm_param_type type) {
	/* __bpf_val_to_ring() uses bpf_probe_read_* functions for particular types
	 * only.  Instead of changing all places, let's keep it simple and try to
	 * spot the correct address space by type.
	 */

	switch(type) {
	case PT_CHARBUF:
	case PT_FSPATH:
	case PT_FSRELPATH:
	case PT_BYTEBUF:
		/* Those types typically read memory from user space pointers.
		 * If not, explicit use the respective helper with the _mem()
		 * suffix to specify the memory to read from.
		 *
		 * See also the usage below in the helpers.
		 */
		return USER;
	default:
		return KERNEL;
	}
}

static __always_inline int bpf_val_to_ring_mem(struct filler_data *data,
                                               unsigned long val,
                                               enum read_memory mem) {
	const struct ppm_param_info *param_info;

	if(data->state->tail_ctx.curarg >= PPM_MAX_EVENT_PARAMS) {
		bpf_printk("invalid curarg: %d\n", data->state->tail_ctx.curarg);
		return PPM_FAILURE_BUG;
	}

	param_info = &data->evt->params[data->state->tail_ctx.curarg & (PPM_MAX_EVENT_PARAMS - 1)];

	return __bpf_val_to_ring(data, val, 0, param_info->type, -1, false, mem);
}

/// TODO: @Andreagit97 these functions should return void
static __always_inline int bpf_push_s64_to_ring(struct filler_data *data, int64_t val) {
	/// TODO: @Andreagit97 this could be removed in a second iteration.
	if(data->state->tail_ctx.curoff > SCRATCH_SIZE_HALF) {
		return PPM_FAILURE_FRAME_SCRATCH_MAP_FULL;
	}
	const unsigned int len = sizeof(int64_t);
	*((int64_t *)&data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF]) = val;
	/// TODO: @Andreagit97 this could be simplified
	fixup_evt_arg_len(data->buf, data->state->tail_ctx.curarg, len);
	data->state->tail_ctx.curoff += len;
	data->state->tail_ctx.len += len;
	data->curarg_already_on_frame = false;
	/* We increment the current argument - to make verifier happy, properly check it */
	data->state->tail_ctx.curarg = SAFE_ARG_NUMBER(data->state->tail_ctx.curarg + 1);
	return PPM_SUCCESS;
}

static __always_inline int bpf_push_u64_to_ring(struct filler_data *data, uint64_t val) {
	if(data->state->tail_ctx.curoff > SCRATCH_SIZE_HALF) {
		return PPM_FAILURE_FRAME_SCRATCH_MAP_FULL;
	}
	const unsigned int len = sizeof(uint64_t);
	*((uint64_t *)&data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF]) = val;
	fixup_evt_arg_len(data->buf, data->state->tail_ctx.curarg, len);
	data->state->tail_ctx.curoff += len;
	data->state->tail_ctx.len += len;
	data->curarg_already_on_frame = false;
	/* We increment the current argument - to make verifier happy, properly check it */
	data->state->tail_ctx.curarg = SAFE_ARG_NUMBER(data->state->tail_ctx.curarg + 1);
	return PPM_SUCCESS;
}

static __always_inline int bpf_push_u32_to_ring(struct filler_data *data, uint32_t val) {
	if(data->state->tail_ctx.curoff > SCRATCH_SIZE_HALF) {
		return PPM_FAILURE_FRAME_SCRATCH_MAP_FULL;
	}
	const unsigned int len = sizeof(uint32_t);
	*((uint32_t *)&data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF]) = val;
	fixup_evt_arg_len(data->buf, data->state->tail_ctx.curarg, len);
	data->state->tail_ctx.curoff += len;
	data->state->tail_ctx.len += len;
	data->curarg_already_on_frame = false;
	/* We increment the current argument - to make verifier happy, properly check it */
	data->state->tail_ctx.curarg = SAFE_ARG_NUMBER(data->state->tail_ctx.curarg + 1);
	return PPM_SUCCESS;
}

static __always_inline int bpf_push_s32_to_ring(struct filler_data *data, int32_t val) {
	if(data->state->tail_ctx.curoff > SCRATCH_SIZE_HALF) {
		return PPM_FAILURE_FRAME_SCRATCH_MAP_FULL;
	}
	const unsigned int len = sizeof(int32_t);
	*((int32_t *)&data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF]) = val;
	fixup_evt_arg_len(data->buf, data->state->tail_ctx.curarg, len);
	data->state->tail_ctx.curoff += len;
	data->state->tail_ctx.len += len;
	data->curarg_already_on_frame = false;
	/* We increment the current argument - to make verifier happy, properly check it */
	data->state->tail_ctx.curarg = SAFE_ARG_NUMBER(data->state->tail_ctx.curarg + 1);
	return PPM_SUCCESS;
}

static __always_inline int bpf_push_u16_to_ring(struct filler_data *data, uint16_t val) {
	if(data->state->tail_ctx.curoff > SCRATCH_SIZE_HALF) {
		return PPM_FAILURE_FRAME_SCRATCH_MAP_FULL;
	}
	const unsigned int len = sizeof(uint16_t);
	*((uint16_t *)&data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF]) = val;
	fixup_evt_arg_len(data->buf, data->state->tail_ctx.curarg, len);
	data->state->tail_ctx.curoff += len;
	data->state->tail_ctx.len += len;
	data->curarg_already_on_frame = false;
	/* We increment the current argument - to make verifier happy, properly check it */
	data->state->tail_ctx.curarg = SAFE_ARG_NUMBER(data->state->tail_ctx.curarg + 1);
	return PPM_SUCCESS;
}

static __always_inline int bpf_push_s16_to_ring(struct filler_data *data, int16_t val) {
	if(data->state->tail_ctx.curoff > SCRATCH_SIZE_HALF) {
		return PPM_FAILURE_FRAME_SCRATCH_MAP_FULL;
	}
	const unsigned int len = sizeof(int16_t);
	*((int16_t *)&data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF]) = val;
	fixup_evt_arg_len(data->buf, data->state->tail_ctx.curarg, len);
	data->state->tail_ctx.curoff += len;
	data->state->tail_ctx.len += len;
	data->curarg_already_on_frame = false;
	/* We increment the current argument - to make verifier happy, properly check it */
	data->state->tail_ctx.curarg = SAFE_ARG_NUMBER(data->state->tail_ctx.curarg + 1);
	return PPM_SUCCESS;
}

static __always_inline int bpf_push_u8_to_ring(struct filler_data *data, uint8_t val) {
	if(data->state->tail_ctx.curoff > SCRATCH_SIZE_HALF) {
		return PPM_FAILURE_FRAME_SCRATCH_MAP_FULL;
	}
	const unsigned int len = sizeof(uint8_t);
	*((uint8_t *)&data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF]) = val;
	fixup_evt_arg_len(data->buf, data->state->tail_ctx.curarg, len);
	data->state->tail_ctx.curoff += len;
	data->state->tail_ctx.len += len;
	data->curarg_already_on_frame = false;
	/* We increment the current argument - to make verifier happy, properly check it */
	data->state->tail_ctx.curarg = SAFE_ARG_NUMBER(data->state->tail_ctx.curarg + 1);
	return PPM_SUCCESS;
}

static __always_inline int bpf_push_s8_to_ring(struct filler_data *data, int16_t val) {
	if(data->state->tail_ctx.curoff > SCRATCH_SIZE_HALF) {
		return PPM_FAILURE_FRAME_SCRATCH_MAP_FULL;
	}
	const unsigned int len = sizeof(int8_t);
	*((int8_t *)&data->buf[data->state->tail_ctx.curoff & SCRATCH_SIZE_HALF]) = val;
	fixup_evt_arg_len(data->buf, data->state->tail_ctx.curarg, len);
	data->state->tail_ctx.curoff += len;
	data->state->tail_ctx.len += len;
	data->curarg_already_on_frame = false;
	/* We increment the current argument - to make verifier happy, properly check it */
	data->state->tail_ctx.curarg = SAFE_ARG_NUMBER(data->state->tail_ctx.curarg + 1);
	return PPM_SUCCESS;
}

static __always_inline int bpf_val_to_ring(struct filler_data *data, unsigned long val) {
	const struct ppm_param_info *param_info;

	/// TODO this is something we want to enforce at test time, not runtime
	if(data->state->tail_ctx.curarg >= PPM_MAX_EVENT_PARAMS) {
		bpf_printk("invalid curarg: %d\n", data->state->tail_ctx.curarg);
		return PPM_FAILURE_BUG;
	}

	param_info = &data->evt->params[data->state->tail_ctx.curarg & (PPM_MAX_EVENT_PARAMS - 1)];

	return __bpf_val_to_ring(data,
	                         val,
	                         0,
	                         param_info->type,
	                         -1,
	                         false,
	                         param_type_to_mem(param_info->type));
}

static __always_inline int bpf_val_to_ring_len(struct filler_data *data,
                                               unsigned long val,
                                               unsigned long val_len) {
	const struct ppm_param_info *param_info;

	if(data->state->tail_ctx.curarg >= PPM_MAX_EVENT_PARAMS) {
		bpf_printk("invalid curarg: %d\n", data->state->tail_ctx.curarg);
		return PPM_FAILURE_BUG;
	}

	param_info = &data->evt->params[data->state->tail_ctx.curarg & (PPM_MAX_EVENT_PARAMS - 1)];

	return __bpf_val_to_ring(data,
	                         val,
	                         val_len,
	                         param_info->type,
	                         -1,
	                         false,
	                         param_type_to_mem(param_info->type));
}

static __always_inline int bpf_val_to_ring_dyn(struct filler_data *data,
                                               unsigned long val,
                                               enum ppm_param_type type,
                                               uint8_t dyn_idx) {
	return __bpf_val_to_ring(data, val, 0, type, dyn_idx, false, param_type_to_mem(type));
}

static __always_inline int bpf_val_to_ring_type_mem(struct filler_data *data,
                                                    unsigned long val,
                                                    enum ppm_param_type type,
                                                    enum read_memory mem) {
	return __bpf_val_to_ring(data, val, 0, type, -1, false, mem);
}

static __always_inline int bpf_val_to_ring_type(struct filler_data *data,
                                                unsigned long val,
                                                enum ppm_param_type type) {
	return __bpf_val_to_ring(data, val, 0, type, -1, false, param_type_to_mem(type));
}

static __always_inline pid_t bpf_push_pgid(struct filler_data *data, struct task_struct *task) {
	pid_t pgid = 0;
	// this is like calling in the kernel:
	//
	// struct pid *grp = task_pgrp(current);
	// int pgrp = pid_nr(grp);
#ifdef HAS_TASK_PIDS_FIELD
	struct task_struct *leader = (struct task_struct *)_READ(task->group_leader);
	if(leader) {
		struct pid_link link = _READ(leader->pids[PIDTYPE_PGID]);
		struct pid *pid_struct = link.pid;
		if(pid_struct) {
			pgid = _READ(pid_struct->numbers[0].nr);
		}
	}
#else
	struct signal_struct *signal = (struct signal_struct *)_READ(task->signal);
	if(signal) {
		struct pid *pid_struct = _READ(signal->pids[PIDTYPE_PGID]);
		if(pid_struct) {
			pgid = _READ(pid_struct->numbers[0].nr);
		}
	}
#endif
	return bpf_push_s64_to_ring(data, (int64_t)pgid);
}

#endif

/* Legacy-probe-specific replacement for `socket_family_to_scap` helper. As encoding the socket
 * family using the `socket_family_to_scap` helper breaks the verifier on old kernel versions, just
 * send `PPM_AF_UNSPEC` if the user-provided socket family is negative, and leave it as is
 * otherwise. This solution relies on the fact that `AF_*` and corresponding `PPM_AF_*` macros map
 * to the same values. */
static __always_inline uint8_t bpf_socket_family_to_scap(int8_t family) {
	if(family < 0) {
		family = PPM_AF_UNSPEC;
	}
	return (uint8_t)family;
}
