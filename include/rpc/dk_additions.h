/*
 *	File: <sys/file.h> For WINDOWS MFC
 *
 *	Created on: Aug 17, 2016
 *	Author    : Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *
 */

#ifndef __win_rpc_dk_addition_h___
#define __win_rpc_dk_addition_h___

#define	RPC_SVC_MTMODE_SET			0
#define	RPC_SVC_CONNMAXREC_SET		8

#include <sdef_gem_windows.h>
#include <win_raw_socket.h>

#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

#if !defined(__uint_t_defined) & !defined(uint_t_defined)
typedef unsigned int uint_t;
#define __uint_t_defined
#define uint_t_defined
#endif  // #if !defined(__uint_t_defined) & !defined(uint_t_defined)

#if !defined(__t_scalar_t_defined) & !defined(t_scalar_t_defined)
typedef int t_scalar_t;
#define __t_scalar_t_defined
#define t_scalar_t_defined
#endif  // #if !defined(__t_scalar_t_defined) & !defined(t_scalar_t_defined)

# define SUN_LEN(ptr) ((size_t) (((struct sockaddr_un *) 0)->sun_path)	      \
		      + strlen ((ptr)->sun_path))

struct __rpc_sockinfo {
	int si_af;
	int si_proto;
	int si_socktype;
	int si_alen;
};

struct netbuf {
	unsigned int	maxlen;
	unsigned int	len;
	void	*buf;
};

struct t_bind {
	struct netbuf	addr;
	unsigned int	qlen;
};

__BEGIN_DECLS

bool_t rpc_control(int op, void *info);
int __rpc_nconf2sockinfo(const struct netconfig *nconf, struct __rpc_sockinfo *sip);
int __rpc_nconf2fd(const struct netconfig *nconf);
SVCXPRT *svc_tli_create(const int fildes, const struct netconfig *netconf, const struct t_bind *bind_addr, const uint_t sendsz, const uint_t recvsz);
int __rpc_fd2sockinfo(int fd, struct __rpc_sockinfo *sip);
int svc_reg(const SVCXPRT*xprt,const rpcprog_t prog,const rpcvers_t vers,
	void(*dispatch)(struct svc_req*,SVCXPRT*),const struct netconfig*nconf);
char * taddr2uaddr(const struct netconfig *nconf, const struct netbuf *nbuf);

__END_DECLS



#endif  // #ifndef __win_rpc_dk_addition_h___
