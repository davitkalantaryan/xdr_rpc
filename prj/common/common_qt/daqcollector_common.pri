# File daqcollector_common.pri
# File created : 02 Feb 2017
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# This file can be used to produce Makefile for daqadcreceiver application
# for PITZ
message("!!! daqcollector_common.pri:")
include(../../common/common_qt/root_no_gui_common.pri)
DEFINES += ROOT_APP

# call qmake CONFIG+=test
optionsTest = $$find(CONFIG, "1test")
count(optionsTest, 1):message("!!! test1 version") DEFINES += TEST_VERSION111
options = $$find(CONFIG, "2test")
count(options, 1):message("!!! test2 version") DEFINES += TEST_VERSION112
include(../../common/common_qt/doocs_server_common.pri)
equals(CODENAME,"Boron") { 
    #message ("!!!!! No cpp 11 used")
    DEFINES += no_cpp11
    QMAKE_CXXFLAGS += -std=c++0x
}
else { 
    #message ("!!!!! cpp 11 is used")
    QMAKE_CXXFLAGS += -std=c++0x
}
INCLUDEPATH += ../../../include
INCLUDEPATH += ../../../contrib/cpp-raft/include
INCLUDEPATH += ../../../src/tools

# these two lines are just for inteligence
#INCLUDEPATH += /afs/ifh.de/@sys/products/root64/5.20.00/include
INCLUDEPATH += /doocs/lib/include
SOURCES += \
    $${PWD}/../../../src/server/pitz_daq_collectorproperties.cpp \
    $${PWD}/../../../src/tools/mailsender.cpp \
    $${PWD}/../../../contrib/cpp-raft/common/cpp11+/thread_cpp11.cpp \
    $${PWD}/../../../contrib/cpp-raft/common/cpp11+/mutex_cpp11.cpp \
    $${PWD}/../../../contrib/cpp-raft/common/cpp11+/shared_mutex_cpp14.cpp \
    $${PWD}/../../../src/tools/pitz_daq_data_memory_base.cpp \
    $${PWD}/../../../src/tools/pitz_daq_data_memory_forserver.cpp \
    $${PWD}/../../../src/server/pitz_daq_singleentry.cpp \
    $${PWD}/../../../src/server/pitz_daq_eqfctcollector.cpp \
    $${PWD}/../../../src/server/pitz_daq_collector_global.cpp \
    $${PWD}/../../../src/tools/pitz_daq_data_entryinfo.cpp

HEADERS += \
    $${PWD}/../../../src/server/pitz_daq_collectorproperties.hpp \
    $${PWD}/../../../src/tools/mailsender.h \
    $${PWD}/../../../contrib/cpp-raft/include/cpp11+/thread_cpp11.impl.hpp \
    $${PWD}/../../../contrib/cpp-raft/include/cpp11+/thread_cpp11.hpp \
    $${PWD}/../../../contrib/cpp-raft/include/cpp11+/common_defination.h \
    $${PWD}/../../../contrib/cpp-raft/include/cpp11+/mutex_cpp11.hpp \
    $${PWD}/../../../contrib/cpp-raft/include/common/common_unnamedsemaphorelite.hpp \
    $${PWD}/../../../contrib/cpp-raft/include/common/impl.common_fifofast.hpp \
    $${PWD}/../../../contrib/cpp-raft/include/common/common_fifofast.hpp \
    $${PWD}/../../../contrib/cpp-raft/include/common/lists.hpp \
    $${PWD}/../../../contrib/cpp-raft/include/common/impl.lists.hpp \
    $${PWD}/../../../contrib/cpp-raft/include/cpp11+/shared_mutex_cpp14.hpp \
    $${PWD}/../../../src/server/pitz_daq_singleentry.hpp \
    $${PWD}/../../../src/server/pitz_daq_eqfctcollector.hpp \
    $${PWD}/../../../include/pitz/daq/data/memory/base.hpp \
    $${PWD}/../../../include/pitz/daq/data/memory/forserver.hpp
