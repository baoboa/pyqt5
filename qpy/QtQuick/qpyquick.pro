# This is the qmake project file for the QPy support code for the QtQuick
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
TARGET      = qpyquick
TEMPLATE    = lib

SOURCES   = \
            qpyquick_post_init.cpp \
            qpyquick_chimera_helpers.cpp \
            qpyquick_register_type.cpp \
            qpyquickitem.cpp \
            qpyquickpainteditem.cpp

HEADERS   = \
            qpyquick_api.h \
            qpyquick_chimera_helpers.h \
            qpyquick_register_type.h \
            qpyquickitem.h \
            qpyquickpainteditem.h
