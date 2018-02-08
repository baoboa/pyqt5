// This defines the API exposed by the Python extension module.
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


#if !defined(_PYLUPDATE_H)
#define _PYLUPDATE_H

#include "metatranslator.h"
#include "proparser.h"


void fetchtr_py(const char *fileName, MetaTranslator *tor,
        const char *defaultContext, bool mustExist, const char *codecForSource,
        const char *tr_func, const char *translate_func);

void fetchtr_ui(const char *fileName, MetaTranslator *tor,
        const char *defaultContext, bool mustExist);

void merge(const MetaTranslator *tor, const MetaTranslator *virginTor,
        MetaTranslator *out, bool noObsolete, bool verbose,
        const QString &filename);

#endif
