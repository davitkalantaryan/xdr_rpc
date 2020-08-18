#
# File root_no_gui_common.pri
# File created : 02 Feb 2017
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# This file can be used to produce Makefile for daqadcreceiver application
# for PITZ
#

message("!!! root_no_gui_no_libs_common.pri")

include( $${PWD}/sys_common.pri )

MYROOT_SYS_DIR = $$system(env | grep ROOT_SYS_DIR)
isEmpty(MYROOT_SYS_DIR) {
    #MYROOT_SYS_DIR = /afs/ifh.de/amd64_rhel50/products/root64/5.28.00
    #MYROOT_SYS_DIR = /export/doocs/opt/root/6.16.00
    MYROOT_SYS_DIR = /afs/ifh.de/group/pitz/data/ers/sys/$${CODENAME}/opt/root/current
    message("!!! MYROOT_SYS_DIR set in the project file: $$MYROOT_SYS_DIR")
} else {
    message("!!! MYROOT_SYS_DIR comes from environment: $$MYROOT_SYS_DIR")
}

DEFINES += R__NULLPTR
DEFINES += ROOT_APP

ROOTCFLAGS = $$system($$MYROOT_SYS_DIR/bin/root-config --cflags)

QMAKE_CXXFLAGS += $$ROOTCFLAGS
QMAKE_CFLAGS += $$ROOTCFLAGS
optionsCpp11 = $$find(CONFIG, "cpp11")
count(optionsCpp11, 1):QMAKE_CXXFLAGS += -std=c++0x
message("ROOTCFLAGS=$$ROOTCFLAGS")

# this line is not needed for compilation but some
# IDE does not shows ROOT headers properly if this line is not there
INCLUDEPATH += $$MYROOT_SYS_DIR/include
