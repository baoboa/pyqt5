// This is the implementation of the ListData class.
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


#include "qpyqml_listdata.h"


// Create a null instance.
ListData::ListData(PyObject *py_type_, PyObject *py_obj_, PyObject *py_list_,
        PyObject *py_append_, PyObject *py_count_, PyObject *py_at_,
        PyObject *py_clear_, QObject *parent) : QObject(parent),
        py_type(py_type_), py_obj(py_obj_), py_list(py_list_),
        py_append(py_append_), py_count(py_count_), py_at(py_at_),
        py_clear(py_clear_)
{
    Py_XINCREF(py_type);
    Py_XINCREF(py_obj);
    Py_XINCREF(py_list);
    Py_XINCREF(py_append);
    Py_XINCREF(py_count);
    Py_XINCREF(py_at);
    Py_XINCREF(py_clear);
}


// Destroy the data.
ListData::~ListData()
{
    Py_XDECREF(py_type);
    Py_XDECREF(py_obj);
    Py_XDECREF(py_list);
    Py_XDECREF(py_append);
    Py_XDECREF(py_count);
    Py_XDECREF(py_at);
    Py_XDECREF(py_clear);
}
