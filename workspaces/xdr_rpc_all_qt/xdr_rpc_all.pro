#   
# file:			xdr_rpc.pro
# created on:	2020 Aug 18
# created by:	D. Kalantaryan (davit.kalantaryan@gmail.com)  
#  

TEMPLATE = subdirs
#CONFIG += ordered

SUBDIRS		+=	$${PWD}/../../prj/core/xdr_rpc_qt/xdr_rpc.pro

win32 {
	contains(QMAKE_TARGET.arch, x86_64) {
	    #SUBDIRS		+=  $${PWD}/../../contrib/service/prj/core/emp_psa_service_qt/emp_psa_service.pro
	}
} else {
	#SUBDIRS		+=  $${PWD}/../../contrib/service/prj/core/emp_psa_service_qt/emp_psa_service.pro
}


debug{
#SUBDIRS		+=	\
#	$${PWD}/../../prj/ui/activity_tracker_qt/activity_tracker.pro					\
#	$${PWD}/../../prj/tests/browsing_history_test_qt/browsing_history_test.pro		\
#	$${PWD}/../../prj/tests/nativeeventfilter_test_qt/nativeeventfilter_test.pro
}
#debug: macx: SUBDIRS += $${PWD}/../../prj/tests/mac_browser_monitor_test_qt/mac_browser_monitor_test.pro


#$${PWD}/../../prj/ui/activity_tracker_qt/activity_tracker.pro.depends = $${PWD}/../../prj/tests/browsing_history_test_qt/browsing_history_test.pro	

	#$${PWD}/../../scripts/host_init_for_compilation.sh
	#$${PWD}/../../scripts/initialize_environment.sh

OTHER_FILES	+=	\
	$${PWD}/../../.gitignore														\
	$${PWD}/../../README.md
