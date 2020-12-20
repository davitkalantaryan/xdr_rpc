#
# File doocs_server_common.pri
# File created : 12 Feb 2017
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# This file can be used to produce Makefile for daqadcreceiver application
# for PITZ
#

message("!!! doocs_server_common.pri:")

QT -= core
QT -= gui

LIBS += -lEqServer
LIBS += -lCom
LIBS += -lca

include(../../common/common_qt/doocs_client_common.pri)

LIBS += -L$$MYDOOCS/system_arch/$$CODENAME/MAIN/base/lib/linux-x86_64
