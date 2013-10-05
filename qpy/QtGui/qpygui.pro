# This is the qmake project file for the QPy support code for the QtGui module.
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
TARGET      = qpygui
TEMPLATE    = lib

SOURCES   = \
            qpyopengl_add_constants.c \
            qpyopengl_array_convertors.cpp \
            qpyopengl_attribute_array.cpp \
            qpyopengl_data_cache.cpp \
            qpyopengl_init.cpp \
            qpyopengl_uniform_value_array.cpp \
            qpyopengl_value_array.cpp \
            qpyopengl_version_functions.cpp

HEADERS   = \
            qpyopengl_api.h \
            qpyopengl_data_cache.h
