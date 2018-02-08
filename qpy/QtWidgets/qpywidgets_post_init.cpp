// This is the initialisation support code for the QtWidgets module.
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


#include "qpywidgets_api.h"
#include "qpywidgets_chimera_helpers.h"

#include "sipAPIQtWidgets.h"


// Perform any required initialisation.
void qpywidgets_post_init()
{
    // Import the Chimera helper registration functions.
    void (*register_to_qvariant_convertor)(ToQVariantConvertorFn);
    register_to_qvariant_convertor = (void (*)(ToQVariantConvertorFn))sipImportSymbol("pyqt5_register_to_qvariant_convertor");
    Q_ASSERT(register_to_qvariant_convertor);
    register_to_qvariant_convertor(qpywidgets_to_qvariant_convertor);
}
