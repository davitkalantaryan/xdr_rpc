#
# File root_no_gui_common.pri
# File created : 02 Feb 2017
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# This file can be used to produce Makefile for daqadcreceiver application
# for PITZ
#

message("!!! root_no_gui_common.pri")
include( $${PWD}/root_no_gui_no_libs_common.pri )
LIBS += $$system($$MYROOT_SYS_DIR/bin/root-config --libs)
