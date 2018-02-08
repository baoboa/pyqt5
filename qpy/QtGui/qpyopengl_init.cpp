// This is the initialisation support code for the QtGui module.
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


#include <Python.h>

#include "sipAPIQtGui.h"

#if defined(SIP_FEATURE_PyQt_OpenGL)

#include "qpyopengl_api.h"
#include "qpyopengl_data_cache.h"


// Perform any required initialisation.
void qpyopengl_init()
{
    // Initialise the OpenGL data cache type.
    if (!qpyopengl_dataCache_init_type())
        Py_FatalError("PyQt5.QtGui: Failed to initialise dataCache type");

    // Export the private helpers, ie. those that should not be used by
    // external handwritten code.
#if QT_VERSION >= 0x050100
    sipExportSymbol("qpyopengl_value_array", (void *)qpyopengl_value_array);
    sipExportSymbol("qpyopengl_value_array_cached",
            (void *)qpyopengl_value_array_cached);
    sipExportSymbol("qpyopengl_get", (void *)qpyopengl_get);
    sipExportSymbol("qpyopengl_from_GLint", (void *)qpyopengl_from_GLint);
    sipExportSymbol("qpyopengl_from_GLuint", (void *)qpyopengl_from_GLuint);
    sipExportSymbol("qpyopengl_from_GLboolean",
            (void *)qpyopengl_from_GLboolean);
    sipExportSymbol("qpyopengl_from_GLfloat", (void *)qpyopengl_from_GLfloat);
#if defined(SIP_FEATURE_PyQt_Desktop_OpenGL)
    sipExportSymbol("qpyopengl_from_GLdouble",
            (void *)qpyopengl_from_GLdouble);
#endif
#endif
}


#endif
