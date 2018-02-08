// This defines the interfaces to the Q_ENUMS and Q_FLAGS support.
//
// Copyright (c) 2018 Riverbank Computing Limited <info@riverbankcomputing.com>
// 
// This file is part of PyQt5.
// 
// This file may be used under the terms of the GNU General Public License
// version 3.0 as published by the Free Software Foundation and appearing in
// the file LICENSE included in the packaging of this file.  Please review the
// following information to ensure the GNU General Public License version 3.0
// requirements will be met: http://www.gnu.org/copyleft/gpl.html.
// 
// If you do not wish to use this file under the terms of the GPL version 3.0
// then you may purchase a commercial license.  For more information contact
// info@riverbankcomputing.com.
// 
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


#ifndef _QPYCORE_ENUMS_FLAGS_H
#define _QPYCORE_ENUMS_FLAGS_H


#include <QByteArray>
#include <QHash>
#include <QList>


struct EnumsFlags
{
    EnumsFlags(const char *name_, bool isFlag_) : name(name_), isFlag(isFlag_)
    {
    }

    QByteArray name;
    bool isFlag;
    QHash<QByteArray, int> keys;
};


QList<EnumsFlags> qpycore_get_enums_flags_list();


#endif
