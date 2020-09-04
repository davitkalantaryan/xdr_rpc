#
# file:			xdr_rpc_all.pro
# created on:	2020 Aug 18
# created by:	D. Kalantaryan (davit.kalantaryan@gmail.com)
#

include ($$PWD}/../../common/common_qt/sys_common.pri )

INCLUDEPATH	+= $${PWD}/../../../include


SOURCES		+=	\
	$${PWD}/../../../src/core/auth_non.c							\
	$${PWD}/../../../src/core/auth_uni.c							\
	$${PWD}/../../../src/core/clnt_gen.c							\
	$${PWD}/../../../src/core/clnt_per.c							\
	$${PWD}/../../../src/core/clnt_per_singlethreaded.c				\
	$${PWD}/../../../src/core/clnt_tcp.c							\
	$${PWD}/../../../src/core/clnt_udp.c							\
	$${PWD}/../../../src/core/mini_xdr_rpc_entry.c					\
	$${PWD}/../../../src/core/mini_xdr_rpc_missing_functions.c		\
	$${PWD}/../../../src/core/pmap_get.c							\
	$${PWD}/../../../src/core/rpc_prot.c							\
	$${PWD}/../../../src/core/xdr.c									\
	$${PWD}/../../../src/core/xdr_array.c							\
	$${PWD}/../../../src/core/xdr_float.c							\
	$${PWD}/../../../src/core/xdr_mem.c								\
	$${PWD}/../../../src/core/xdr_rec.c								\
	$${PWD}/../../../src/core/xdr_reference.c


HEADERS		+=	\
	$${PWD}/../../../src/core/mini_xdr_rpc_src_private.h			\
	$${PWD}/../../../include/rpc/auth.h								\
	$${PWD}/../../../include/rpc/auth_des.h							\
	$${PWD}/../../../include/rpc/auth_unix.h							\
	$${PWD}/../../../include/rpc/clnt.h								\
	$${PWD}/../../../include/rpc/doocs_rpc_unix_like_functions.h		\
	$${PWD}/../../../include/rpc/pmap_clnt.h							\
	$${PWD}/../../../include/rpc/pmap_prot.h							\
	$${PWD}/../../../include/rpc/redirected_win_rpc.h				\
	$${PWD}/../../../include/rpc/rpc.h								\
	$${PWD}/../../../include/rpc/rpc_msg.h							\
	$${PWD}/../../../include/rpc/svc.h								\
	$${PWD}/../../../include/rpc/svc_auth.h							\
	$${PWD}/../../../include/rpc/types.h								\
	$${PWD}/../../../include/rpc/wrpc_first_com_include.h			\
	$${PWD}/../../../include/rpc/xdr.h

OTHER_FILES	+=	\
	$${PWD}/../wasm_xdr_rpc_mkfl/Makefile
