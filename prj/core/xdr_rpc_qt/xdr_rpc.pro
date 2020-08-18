#
# file:			xdr_rpc_all.pro
# created on:	2020 Aug 18
# created by:	D. Kalantaryan (davit.kalantaryan@gmail.com)
#

include ($$PWD}/../../common/common_qt/sys_common.pri )

INCLUDEPATH	+= $${PWD}/../../../include

HEADERS		+=	\
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
