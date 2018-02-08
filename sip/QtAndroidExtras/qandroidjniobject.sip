// This is the SIP interface definition for QAndroidJniObject.
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


%If (Qt_5_2_0 -)

class QAndroidJniObject
{
%TypeHeaderCode
#include <qandroidjniobject.h>
%End

public:
    QAndroidJniObject();
    QAndroidJniObject(const char *className);
    ~QAndroidJniObject();

    int callMethod(const char *methodName) const;
%MethodCode
        sipRes = sipCpp->callMethod<int>(a0);
%End

    static int callStaticMethod(const char *className, const char *methodName);
%MethodCode
        sipRes = QAndroidJniObject::callStaticMethod<int>(a0, a1);
%End

    bool isValid() const;
    QString toString() const;

    static bool isClassAvailable(const char *className);
};

bool operator!=(const QAndroidJniObject &, const QAndroidJniObject &);
bool operator==(const QAndroidJniObject &, const QAndroidJniObject &);

%End
