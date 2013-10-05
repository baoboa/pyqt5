# This is the qmake project file for the QPy support code for the QtCore
# module.
#
# Copyright (c) 2013 Riverbank Computing Limited <info@riverbankcomputing.com>
# 
# This file is part of PyQt5.
# 
# This file may be used under the terms of the GNU General Public License
# version 3.0 as published by the Free Software Foundation and appearing in
# the file LICENSE included in the packaging of this file.  Please review the
# following information to ensure the GNU General Public License version 3.0
# requirements will be met: http://www.gnu.org/copyleft/gpl.html.
# 
# If you do not wish to use this file under the terms of the GPL version 3.0
# then you may purchase a commercial license.  For more information contact
# info@riverbankcomputing.com.
# 
# This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
# WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


CONFIG      += static warn_on
TARGET      = qpycore
TEMPLATE    = lib

# Python's type system relies on type punning.
!win32: QMAKE_CXXFLAGS += -fno-strict-aliasing

# This seems to be necessary for Qt v4.5.2.
win32: INCLUDEPATH += .

SOURCES   = \
            qpycore_chimera.cpp \
            qpycore_chimera_signature.cpp \
            qpycore_chimera_storage.cpp \
            qpycore_classinfo.cpp \
            qpycore_decorators.cpp \
            qpycore_init.cpp \
            qpycore_misc.cpp \
            qpycore_post_init.cpp \
            qpycore_public_api.cpp \
            qpycore_pyqtboundsignal.cpp \
            qpycore_pyqtconfigure.cpp \
            qpycore_pyqtmethodproxy.cpp \
            qpycore_pyqtproperty.cpp \
            qpycore_pyqtpyobject.cpp \
            qpycore_pyqtsignal.cpp \
            qpycore_pyqtslot.cpp \
            qpycore_pyqtslotproxy.cpp \
            qpycore_qmessagelogger.cpp \
            qpycore_qmetaobject.cpp \
            qpycore_qmetaobject_helpers.cpp \
            qpycore_qobject_getattr.cpp \
            qpycore_qobject_helpers.cpp \
            qpycore_qstring.cpp \
            qpycore_qstringlist.cpp \
            qpycore_types.cpp \
            qpycore_qvariant.cpp \
            qpycore_qvariant_value.cpp

HEADERS   = \
            qpycore_api.h \
            qpycore_chimera.h \
            qpycore_classinfo.h \
            qpycore_misc.h \
            qpycore_namespace.h \
            qpycore_public_api.h \
            qpycore_pyqtboundsignal.h \
            qpycore_pyqtproperty.h \
            qpycore_pyqtpyobject.h \
            qpycore_pyqtsignal.h \
            qpycore_pyqtslot.h \
            qpycore_pyqtslotproxy.h \
            qpycore_pyqtmethodproxy.h \
            qpycore_qmetaobjectbuilder.h \
            qpycore_qobject_helpers.h \
            qpycore_sip.h \
            qpycore_types.h
