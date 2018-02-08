// This is the initialisation support code for the QtQuick module.
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


#include "qpyquick_api.h"
#include "qpyquick_chimera_helpers.h"
#include "qpyquick_register_type.h"

#include "sipAPIQtQuick.h"


// Imports from QtCore.
pyqt5_qtquick_err_print_t pyqt5_qtquick_err_print;


// Perform any required initialisation.
void qpyquick_post_init()
{
    // Import the Chimera helper registration functions.
    void (*register_from_qvariant_convertor)(FromQVariantConvertorFn);
    register_from_qvariant_convertor = (void (*)(FromQVariantConvertorFn))sipImportSymbol("pyqt5_register_from_qvariant_convertor");
    Q_ASSERT(register_from_qvariant_convertor);
    register_from_qvariant_convertor(qpyquick_from_qvariant_convertor);

    void (*register_to_qvariant_convertor)(ToQVariantConvertorFn);
    register_to_qvariant_convertor = (void (*)(ToQVariantConvertorFn))sipImportSymbol("pyqt5_register_to_qvariant_convertor");
    Q_ASSERT(register_to_qvariant_convertor);
    register_to_qvariant_convertor(qpyquick_to_qvariant_convertor);

    void (*register_to_qvariant_data_convertor)(ToQVariantDataConvertorFn);
    register_to_qvariant_data_convertor = (void (*)(ToQVariantDataConvertorFn))sipImportSymbol("pyqt5_register_to_qvariant_data_convertor");
    Q_ASSERT(register_to_qvariant_data_convertor);
    register_to_qvariant_data_convertor(qpyquick_to_qvariant_data_convertor);

    // Other QtCore imports.
    pyqt5_qtquick_err_print = (pyqt5_qtquick_err_print_t)sipImportSymbol(
            "pyqt5_err_print");
    Q_ASSERT(pyqt5_qtquick_err_print);

    // Export the qml_register_type() helper.
    sipExportSymbol("qtquick_register_item", (void *)qpyquick_register_type);
}
