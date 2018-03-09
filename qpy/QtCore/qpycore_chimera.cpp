// This is the implementation of the Chimera class.
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

#include <QByteArray>
#include <QMetaObject>
#include <QMetaType>

#include "qpycore_chimera.h"
#include "qpycore_misc.h"
#include "qpycore_pyqtpyobject.h"
#include "qpycore_types.h"

#include "sipAPIQtCore.h"


// The registered Python enum types.
QSet<PyObject *> Chimera::_py_enum_types;

// The cache of previously parsed argument type lists.
QHash<QByteArray, QList<const Chimera *> > Chimera::_previously_parsed;

// The registered QVariant to PyObject convertors.
Chimera::FromQVariantConvertors Chimera::registeredFromQVariantConvertors;

// The registered PyObject to QVariant convertors.
Chimera::ToQVariantConvertors Chimera::registeredToQVariantConvertors;

// The registered PyObject to QVariant data convertors.
Chimera::ToQVariantDataConvertors Chimera::registeredToQVariantDataConvertors;

const int UnknownType = QMetaType::UnknownType;

// Construct an invalid type.
Chimera::Chimera()
    : _type(0), _py_type(0), _metatype(UnknownType), _inexact(false),
      _is_qflags(false)
{
}


// Construct a copy.
Chimera::Chimera(const Chimera &other)
{
    _type = other._type;

    _py_type = other._py_type;
    Py_XINCREF((PyObject *)_py_type);
    
    _metatype = other._metatype;
    _inexact = other._inexact;
    _is_qflags = other._is_qflags;
    _name = other._name;
}


// Destroy the type.
Chimera::~Chimera()
{
    Py_XDECREF((PyObject *)_py_type);
}


// Register the type of a Python enum.
void Chimera::registerPyEnum(PyObject *enum_type)
{
    Py_INCREF(enum_type);
    _py_enum_types.insert(enum_type);
}


// Parse an object as a type.
const Chimera *Chimera::parse(PyObject *obj)
{
    Chimera *ct = new Chimera;
    bool parse_ok;

    if (PyType_Check(obj))
    {
        // Parse the type object.
        parse_ok = ct->parse_py_type((PyTypeObject *)obj);

        if (!parse_ok)
            raiseParseException(obj);
    }
    else
    {
        const char *cpp_type_name = sipString_AsASCIIString(&obj);

        if (cpp_type_name)
        {
            // Always use normalised type names so that we have a consistent
            // standard.
            QByteArray norm_name = QMetaObject::normalizedType(cpp_type_name);
            Py_DECREF(obj);

            // Parse the type name.
            parse_ok = ct->parse_cpp_type(norm_name);

            if (!parse_ok)
                raiseParseCppException(cpp_type_name);
        }
        else
        {
            parse_ok = false;
        }
    }

    if (!parse_ok)
    {
        delete ct;
        return 0;
    }

    return ct;
}


// Parse a C++ type name as a type.
const Chimera *Chimera::parse(const QByteArray &name)
{
    Chimera *ct = new Chimera;

    if (!ct->parse_cpp_type(name))
    {
        delete ct;

        raiseParseCppException(name.constData());

        return 0;
    }

    return ct;
}


// Parse a meta-property as a type.
const Chimera *Chimera::parse(const QMetaProperty &mprop)
{
    Chimera *ct = new Chimera;
    const char *type_name = mprop.typeName();

    ct->_type = sipFindType(type_name);
    ct->_metatype = mprop.userType();
    ct->_inexact = true;
    ct->_is_qflags = mprop.isFlagType();
    ct->_name = type_name;

    return ct;
}


// Parse a normalised C++ signature as a list of types.
Chimera::Signature *Chimera::parse(const QByteArray &sig, const char *context)
{
    // Extract the argument list.
    int start_idx = sig.indexOf('(');

    if (start_idx < 0)
        start_idx = 0;
    else
        ++start_idx;

    int end_idx = sig.lastIndexOf(')');

    int len;

    if (end_idx < start_idx)
        len = -1;
    else
        len = end_idx - start_idx;

    // Parse each argument type.
    Chimera::Signature *parsed_sig = new Chimera::Signature(sig, true);

    if (len > 0)
    {
        QByteArray args_str = sig.mid(start_idx, len);

        // Check we haven't already done it.
        QList<const Chimera *> parsed_args = _previously_parsed.value(args_str);

        if (parsed_args.isEmpty())
        {
            int i, arg_start, template_level;

            i = arg_start = template_level = 0;

            // Extract each argument allowing for commas in templates.
            for (;;)
            {
                char ch = (i < args_str.size() ? args_str.at(i) : '\0');
                QByteArray arg;

                switch (ch)
                {
                case '<':
                    ++template_level;
                    break;

                case '>':
                    --template_level;
                    break;

                case '\0':
                    arg = args_str.mid(arg_start, i - arg_start);
                    break;

                case ',':
                    if (template_level == 0)
                    {
                        arg = args_str.mid(arg_start, i - arg_start);
                        arg_start = i + 1;
                    }

                    break;
                }

                if (!arg.isEmpty())
                {
                    Chimera *ct = new Chimera;

                    if (!ct->parse_cpp_type(arg))
                    {
                        delete ct;
                        delete parsed_sig;
                        qDeleteAll(parsed_args.constBegin(),
                                parsed_args.constEnd());

                        raiseParseCppException(arg.constData(), context);

                        return 0;
                    }

                    parsed_args.append(ct);

                    if (ch == '\0')
                        break;
                }

                ++i;
            }

            // Only parse once.
            _previously_parsed.insert(args_str, parsed_args);
        }

        parsed_sig->parsed_arguments = parsed_args;
    }

    return parsed_sig;
}


// Parses a C++ signature given as a Python tuple of types and an optional
// name.  Return 0 if there was an error.
Chimera::Signature *Chimera::parse(PyObject *types, const char *name,
        const char *context)
{
    if (!name)
        name = "";

    Chimera::Signature *parsed_sig = new Chimera::Signature(name, false);

    parsed_sig->signature.append('(');
    parsed_sig->py_signature.append('[');

    for (Py_ssize_t i = 0; i < PyTuple_Size(types); ++i)
    {
        PyObject *type = PyTuple_GetItem(types, i);
        const Chimera *parsed_type = parse(type);

        if (!parsed_type)
        {
            delete parsed_sig;

            raiseParseException(type, context);

            return 0;
        }

        parsed_sig->parsed_arguments.append(parsed_type);

        if (i > 0)
        {
            parsed_sig->signature.append(',');
            parsed_sig->py_signature.append(", ");
        }

        parsed_sig->signature.append(parsed_type->name());

        if (parsed_type->_py_type)
            parsed_sig->py_signature.append(sipPyTypeName(parsed_type->_py_type));
        else
            parsed_sig->py_signature.append(parsed_type->name());
    }

    parsed_sig->signature.append(')');
    parsed_sig->py_signature.append(']');

    return parsed_sig;
}


// Raise an exception after parse() of a Python type has failed.
void Chimera::raiseParseException(PyObject *type, const char *context)
{
    if (PyType_Check(type))
    {
        if (context)
            PyErr_Format(PyExc_TypeError,
                    "Python type '%s' is not supported as %s type",
                    sipPyTypeName((PyTypeObject *)type), context);
        else
            PyErr_Format(PyExc_TypeError, "unknown Python type '%s'",
                    sipPyTypeName((PyTypeObject *)type));
    }
    else
    {
        const char *cpp_type_name = sipString_AsASCIIString(&type);

        if (cpp_type_name)
        {
            raiseParseCppException(cpp_type_name, context);
            Py_DECREF(type);
        }
    }
}


// Raise an exception after parse() of a C++ type has failed.
void Chimera::raiseParseCppException(const char *type, const char *context)
{
    if (context)
        PyErr_Format(PyExc_TypeError,
                "C++ type '%s' is not supported as %s type", type, context);
    else
        PyErr_Format(PyExc_TypeError, "unknown C++ type '%s'", type);
}


// Parse the given Python type object.
bool Chimera::parse_py_type(PyTypeObject *type_obj)
{
    const sipTypeDef *td = sipTypeFromPyTypeObject(type_obj);

    if (td)
    {
        if (sipTypeIsNamespace(td))
            return false;

        _type = td;
        _name = sipTypeName(td);

        if (sipTypeIsClass(td))
            set_qflags();

        if (sipTypeIsEnum(td) || _is_qflags)
        {
            _metatype = QMetaType::Int;
        }
        else
        {
            // If there is no assignment helper then assume it is a
            // pointer-type.
            if (!get_assign_helper())
                _name.append('*');

            _metatype = QMetaType::type(_name.constData());

            // If it is a user type then it must be a type that SIP knows
            // about but was registered by Qt.
            if (_metatype < QMetaType::User)
            {
                if (PyType_IsSubtype(type_obj, sipTypeAsPyTypeObject(sipType_QObject)))
                {
                    _metatype = QMetaType::QObjectStar;
                }
                else if (sipIsUserType((sipWrapperType *)type_obj))
                {
                    // It is a non-QObject Python sub-class so make sure it
                    // gets wrapped in a PyQt_PyObject.
                    _type = 0;
                    _metatype = PyQt_PyObject::metatype;
                    _name.clear();
                }
            }
        }
    }
    else if (_py_enum_types.contains((PyObject *)type_obj))
    {
        _metatype = QMetaType::Int;
        _name = sipPyTypeName(type_obj);
    }
    else if (type_obj == &PyList_Type)
    {
        _metatype = QMetaType::QVariantList;
    }
#if PY_MAJOR_VERSION >= 3
    else if (type_obj == &PyUnicode_Type)
    {
        _type = sipType_QString;
        _metatype = QMetaType::QString;
    }
#else
    else if (type_obj == &PyString_Type || type_obj == &PyUnicode_Type)
    {
        // In this case we accept that the reverse conversion will result in an
        // object of a different type (i.e. a QString rather than a Python
        // string).
        _type = sipType_QString;
        _metatype = QMetaType::QString;
    }
#endif
    else if (type_obj == &PyBool_Type)
    {
        _metatype = QMetaType::Bool;
    }
#if PY_MAJOR_VERSION < 3
    else if (type_obj == &PyInt_Type)
    {
        // We choose to map to a C++ int, even though a Python int is
        // potentially much larger, as it represents the most common usage in
        // Qt.  However we will allow a larger type to be used if the context
        // is right.
        _metatype = QMetaType::Int;
        _inexact = true;
    }
#endif
    else if (type_obj == &PyLong_Type)
    {
        // We choose to map to a C++ int for the same reasons as above and to
        // be consistent with Python3 where everything is a long object.  If
        // this isn't appropriate the user can always use a string to specify
        // the exact C++ type they want.
        _metatype = QMetaType::Int;
        _inexact = true;
    }
    else if (type_obj == &PyFloat_Type)
    {
        _metatype = QMetaType::Double;
    }
    else if (type_obj == sipVoidPtr_Type)
    {
        _metatype = QMetaType::VoidStar;
        _name = "void*";
    }

    // Fallback to using a PyQt_PyObject.
    if (_metatype == UnknownType)
        _metatype = PyQt_PyObject::metatype;

    // If there is no name so far then use the meta-type name.
    if (_name.isEmpty())
        _name = QMetaType::typeName(_metatype);

    _py_type = type_obj;
    Py_INCREF((PyObject *)_py_type);

    return true;
}


// Set the internal QFlags flag.
void Chimera::set_qflags()
{
    if (qpycore_is_pyqt_class(_type))
        _is_qflags = ((pyqt5ClassPluginDef *)sipTypePluginData(_type))->flags & 0x01;
}


// Update a C++ type so that any typedefs are resolved.
QByteArray Chimera::resolve_types(const QByteArray &type)
{
    // Split into a base type and a possible list of template arguments.
    QByteArray resolved = type.simplified();

    // Get the raw type, ie. without any "const", "&" or "*".
    QByteArray raw_type;
    int original_raw_start;

    if (resolved.startsWith("const "))
        original_raw_start = 6;
    else
        original_raw_start = 0;

    raw_type = resolved.mid(original_raw_start);

    while (raw_type.endsWith('&') || raw_type.endsWith('*') || raw_type.endsWith(' '))
        raw_type.chop(1);

    int original_raw_len = raw_type.size();

    if (original_raw_len == 0)
        return QByteArray();

    // Get any template arguments.
    QList<QByteArray> args;
    int tstart = raw_type.indexOf('<');

    if (tstart >= 0)
    {
        // Make sure the template arguments are terminated.
        if (!raw_type.endsWith('>'))
            return QByteArray();

        // Split the template arguments taking nested templates into account.
        int depth = 1, arg_start = tstart + 1;

        for (int i = arg_start; i < raw_type.size(); ++i)
        {
            int arg_end = -1;
            char ch = raw_type.at(i);

            if (ch == '<')
            {
                ++depth;
            }
            else if (ch == '>')
            {
                --depth;

                if (depth < 0)
                    return QByteArray();

                if (depth == 0)
                    arg_end = i;
            }
            else if (ch == ',' && depth == 1)
            {
                arg_end = i;
            }

            if (arg_end >= 0)
            {
                QByteArray arg = resolve_types(raw_type.mid(arg_start, arg_end - arg_start));

                if (arg.isEmpty())
                    return QByteArray();

                args.append(arg);

                arg_start = arg_end + 1;
            }
        }

        if (depth != 0)
            return QByteArray();

        // Remove the template arguments.
        raw_type.truncate(tstart);
    }

    // Expand any typedef.
    const char *base_type = sipResolveTypedef(raw_type.constData());

    if (base_type)
        raw_type = base_type;

    // Add any (now resolved) template arguments.
    if (args.count() > 0)
    {
        raw_type.append('<');

        for (QList<QByteArray>::const_iterator it = args.begin();;)
        {
            raw_type.append(*it);

            if (++it == args.end())
                break;

            raw_type.append(',');
        }

        if (raw_type.endsWith('>'))
            raw_type.append(' ');

        raw_type.append('>');
    }

    // Replace the original raw type with the resolved one.
    resolved.replace(original_raw_start, original_raw_len, raw_type);

    return resolved;
}


// Parse the given C++ type name.
bool Chimera::parse_cpp_type(const QByteArray &type)
{
    _name = type;

    // Resolve any types.
    QByteArray resolved = resolve_types(type);

    if (resolved.isEmpty())
        return false;

    // See if the type is known to Qt.
    _metatype = QMetaType::type(resolved.constData());

    // If not then use the PyQt_PyObject wrapper.
    if (_metatype == UnknownType)
        _metatype = PyQt_PyObject::metatype;

    // See if the type (without a pointer) is known to SIP.
    bool is_ptr = resolved.endsWith('*');

    if (is_ptr)
    {
        resolved.chop(1);

        if (resolved.endsWith('*'))
            return false;
    }

    _type = sipFindType(resolved.constData());

    if (!_type)
    {
        // This is the only fundamental pointer type recognised by Qt.
        if (_metatype == QMetaType::VoidStar)
            return true;

        // This is 'int', 'bool', etc.
        if (_metatype != PyQt_PyObject::metatype && !is_ptr)
            return true;

        if ((resolved == "char" || resolved == "const char") && is_ptr)
        {
            // This is a special value meaning a (hopefully) '\0' terminated
            // string.
            _metatype = -1;

            return true;
        }

        // This is an explicit 'PyQt_PyObject'.
        if (resolved == "PyQt_PyObject" && !is_ptr)
            return true;

        return false;
    }

    if (sipTypeIsNamespace(_type))
        return false;

    if (sipTypeIsClass(_type))
    {
        set_qflags();

        if (is_ptr)
        {
            PyTypeObject *type_obj = sipTypeAsPyTypeObject(_type);

            if (PyType_IsSubtype(type_obj, sipTypeAsPyTypeObject(sipType_QObject)))
            {
                _metatype = QMetaType::QObjectStar;
            }
        }
    }

    // We don't support pointers to enums.
    if (sipTypeIsEnum(_type) && is_ptr)
        _type = 0;

    if (sipTypeIsEnum(_type) || _is_qflags)
        _metatype = QMetaType::Int;

    return true;
}


// Convert a Python object to C++ at a given address.  This has a lot in common
// with the method that converts to a QVariant.  However, unlike that method,
// we have no control over the size of the destination storage and so must
// convert exactly as requested.
bool Chimera::fromPyObject(PyObject *py, void *cpp) const
{
    // Let any registered convertors have a go first.
    for (int i = 0; i < registeredToQVariantDataConvertors.count(); ++i)
    {
        bool ok;

        if (registeredToQVariantDataConvertors.at(i)(py, cpp, _metatype, &ok))
            return ok;
    }

    int iserr = 0;

    PyErr_Clear();

    switch (_metatype)
    {
    case QMetaType::Bool:
        *reinterpret_cast<bool *>(cpp) = PyLong_AsLong(py);
        break;

    case QMetaType::Int:
        // Truncate it if necessary to fit into a C++ int.  This will
        // automatically handle enums and flag types as Python knows how to
        // convert them to ints.
#if PY_MAJOR_VERSION >= 3
        *reinterpret_cast<int *>(cpp) = PyLong_AsLong(py);
#else
        *reinterpret_cast<int *>(cpp) = PyInt_AsLong(py);
#endif
        break;

    case QMetaType::UInt:
        *reinterpret_cast<unsigned int *>(cpp) = sipLong_AsUnsignedLong(py);
        break;

    case QMetaType::Double:
        *reinterpret_cast<double *>(cpp) = PyFloat_AsDouble(py);
        break;

    case QMetaType::VoidStar:
        *reinterpret_cast<void **>(cpp) = sipConvertToVoidPtr(py);
        break;

    case QMetaType::Long:
        *reinterpret_cast<long *>(cpp) = PyLong_AsLong(py);
        break;

    case QMetaType::LongLong:
        *reinterpret_cast<qlonglong *>(cpp) = PyLong_AsLongLong(py);
        break;

    case QMetaType::Short:
        *reinterpret_cast<short *>(cpp) = PyLong_AsLong(py);
        break;

    case QMetaType::Char:
        if (SIPBytes_Check(py) && SIPBytes_Size(py) == 1)
            *reinterpret_cast<char *>(cpp) = *SIPBytes_AsString(py);
        else
            iserr = 1;
        break;

    case QMetaType::ULong:
        *reinterpret_cast<unsigned long *>(cpp) = sipLong_AsUnsignedLong(py);
        break;

    case QMetaType::ULongLong:
        *reinterpret_cast<qulonglong *>(cpp) = static_cast<qulonglong>(PyLong_AsUnsignedLongLong(py));
        break;

    case QMetaType::UShort:
        *reinterpret_cast<unsigned short *>(cpp) = sipLong_AsUnsignedLong(py);
        break;

    case QMetaType::UChar:
        if (SIPBytes_Check(py) && SIPBytes_Size(py) == 1)
            *reinterpret_cast<unsigned char *>(cpp) = *SIPBytes_AsString(py);
        else
            iserr = 1;
        break;

    case QMetaType::Float:
        *reinterpret_cast<float *>(cpp) = PyFloat_AsDouble(py);
        break;

    case QMetaType::QObjectStar:
        *reinterpret_cast<void **>(cpp) = sipForceConvertToType(py,
                sipType_QObject, 0, SIP_NO_CONVERTORS, 0, &iserr);
        break;

    case QMetaType::QVariantList:
        {
            QVariantList ql;

            if (to_QVariantList(py, ql))
                *reinterpret_cast<QVariantList *>(cpp) = ql;
            else
                iserr = 1;

            break;
        }

    case QMetaType::QVariantMap:
        {
            QVariantMap qm;

            if (to_QVariantMap(py, qm))
                *reinterpret_cast<QVariantMap *>(cpp) = qm;
            else
                iserr = 1;

            break;
        }

    case QMetaType::QVariantHash:
        {
            QVariantHash qh;

            if (to_QVariantHash(py, qh))
                *reinterpret_cast<QVariantHash *>(cpp) = qh;
            else
                iserr = 1;

            break;
        }

    case -1:
        {
            const char **ptr = reinterpret_cast<const char **>(cpp);

            if (SIPBytes_Check(py))
                *ptr = SIPBytes_AsString(py);
            else if (py == Py_None)
                *ptr = 0;
            else
                iserr = 1;

            break;
        }

    default:
        if (_type)
        {
            if (_name.endsWith('*'))
            {
                // This must be a pointer-type.

                *reinterpret_cast<void **>(cpp) = sipForceConvertToType(py,
                        _type, 0, SIP_NO_CONVERTORS, 0, &iserr);
            }
            else
            {
                // This must be a value-type.

                sipAssignFunc assign = get_assign_helper();

                if (assign)
                {
                    int state;
                    void *value_class;

                    value_class = sipForceConvertToType(py, _type, 0,
                            SIP_NOT_NONE, &state, &iserr);

                    if (!iserr)
                        assign(cpp, 0, value_class);

                    sipReleaseType(value_class, _type, state);
                }
                else
                {
                    iserr = 1;
                }
            }
        }
        else
        {
            iserr = 1;
        }
    }

    if (iserr || PyErr_Occurred())
    {
        PyErr_Format(PyExc_TypeError,
                "unable to convert a Python '%s' object to a C++ '%s' instance",
                sipPyTypeName(Py_TYPE(py)), _name.constData());

        return false;
    }

    return true;
}


// Return the assignment helper for the type, if any.
sipAssignFunc Chimera::get_assign_helper() const
{
    if (sipTypeIsClass(_type))
        return ((sipClassTypeDef *)_type)->ctd_assign;

    if (sipTypeIsMapped(_type))
        return ((sipMappedTypeDef *)_type)->mtd_assign;

    return 0;
}


// Convert a Python object to a QVariant.
bool Chimera::fromPyObject(PyObject *py, QVariant *var, bool strict) const
{
    // Deal with the simple case of wrapping the Python object rather than
    // converting it.
    if (_type != sipType_QVariant && _metatype == PyQt_PyObject::metatype)
    {
        // If the type was specified by a Python type (as opposed to
        // 'PyQt_PyObject') then check the object is an instance of it.
        if (_py_type && !PyObject_TypeCheck(py, _py_type))
            return false;

        *var = keep_as_pyobject(py);
        return true;
    }

    // Let any registered convertors have a go first.  However don't invoke
    // then for None because that is effectively reserved for representing a
    // null QString.
    if (py != Py_None)
    {
        for (int i = 0; i < registeredToQVariantConvertors.count(); ++i)
        {
            bool ok;

            if (registeredToQVariantConvertors.at(i)(py, *var, &ok))
                return ok;
        }
    }

    int iserr = 0, value_class_state;
    void *ptr_class, *value_class = 0;

    // Temporary storage for different types.
    union {
        bool tmp_bool;
        int tmp_int;
        unsigned int tmp_unsigned_int;
        double tmp_double;
        void *tmp_void_ptr;
        long tmp_long;
        qlonglong tmp_qlonglong;
        short tmp_short;
        char tmp_char;
        unsigned long tmp_unsigned_long;
        qulonglong tmp_qulonglong;
        unsigned short tmp_unsigned_short;
        unsigned char tmp_unsigned_char;
        float tmp_float;
    } tmp_storage;

    void *variant_data = &tmp_storage;

    PyErr_Clear();

    QVariant variant;
    int metatype_used = _metatype;

    switch (_metatype)
    {
    case QMetaType::Bool:
        tmp_storage.tmp_bool = PyLong_AsLong(py);
        break;

    case QMetaType::Int:
        if (!_inexact || isEnum())
        {
            // It must fit into a C++ int.
#if PY_MAJOR_VERSION >= 3
            tmp_storage.tmp_int = PyLong_AsLong(py);
#else
            tmp_storage.tmp_int = PyInt_AsLong(py);
#endif
        }
        else
        {
            // Fit it into the smallest C++ type we can.

            qlonglong qll = PyLong_AsLongLong(py);

            if (PyErr_Occurred())
            {
                // Try again in case the value is unsigned and will fit with
                // the extra bit.

                PyErr_Clear();

                qulonglong qull = static_cast<qulonglong>(PyLong_AsUnsignedLongLong(py));

                if (PyErr_Occurred())
                {
                    // It won't fit into any C++ type so pass it as a Python
                    // object.

                    PyErr_Clear();

                    *var = keep_as_pyobject(py);
                    metatype_used = UnknownType;
                }
                else
                {
                    tmp_storage.tmp_qulonglong = qull;
                    metatype_used = QMetaType::ULongLong;
                }
            }
            else if ((qlonglong)(int)qll == qll)
            {
                // It fits in a C++ int.
                tmp_storage.tmp_int = qll;
            }
            else if ((qulonglong)(unsigned int)qll == (qulonglong)qll)
            {
                // The extra bit is enough for it to fit.
                tmp_storage.tmp_unsigned_int = qll;
                metatype_used = QMetaType::UInt;
            }
            else
            {
                // This fits.
                tmp_storage.tmp_qlonglong = qll;
                metatype_used = QMetaType::LongLong;
            }
        }

        break;

    case QMetaType::UInt:
        tmp_storage.tmp_unsigned_int = sipLong_AsUnsignedLong(py);
        break;

    case QMetaType::Double:
        tmp_storage.tmp_double = PyFloat_AsDouble(py);
        break;

    case QMetaType::VoidStar:
        tmp_storage.tmp_void_ptr = sipConvertToVoidPtr(py);
        break;

    case QMetaType::Long:
        tmp_storage.tmp_long = PyLong_AsLong(py);
        break;

    case QMetaType::LongLong:
        tmp_storage.tmp_qlonglong = PyLong_AsLongLong(py);
        break;

    case QMetaType::Short:
        tmp_storage.tmp_short = PyLong_AsLong(py);
        break;

    case QMetaType::Char:
        if (SIPBytes_Check(py) && SIPBytes_Size(py) == 1)
            tmp_storage.tmp_char = *SIPBytes_AsString(py);
        else
            iserr = 1;
        break;

    case QMetaType::ULong:
        tmp_storage.tmp_unsigned_long = sipLong_AsUnsignedLong(py);
        break;

    case QMetaType::ULongLong:
        tmp_storage.tmp_qulonglong = static_cast<qulonglong>(PyLong_AsUnsignedLongLong(py));
        break;

    case QMetaType::UShort:
        tmp_storage.tmp_unsigned_short = sipLong_AsUnsignedLong(py);
        break;

    case QMetaType::UChar:
        if (SIPBytes_Check(py) && SIPBytes_Size(py) == 1)
            tmp_storage.tmp_unsigned_char = *SIPBytes_AsString(py);
        else
            iserr = 1;
        break;

    case QMetaType::Float:
        tmp_storage.tmp_float = PyFloat_AsDouble(py);
        break;

    case QMetaType::QObjectStar:
        tmp_storage.tmp_void_ptr = sipForceConvertToType(py, sipType_QObject,
                0, SIP_NO_CONVERTORS, 0, &iserr);
        break;

    case QMetaType::QVariantList:
        {
            QVariantList ql;

            if (to_QVariantList(py, ql))
            {
                *var = QVariant(ql);
                metatype_used = UnknownType;
            }
            else
            {
                iserr = 1;
            }

            break;
        }

    case QMetaType::QVariantMap:
        {
            QVariantMap qm;

            if (to_QVariantMap(py, qm))
            {
                *var = QVariant(qm);
                metatype_used = UnknownType;
            }
            else if (strict)
            {
                iserr = 1;
            }
            else
            {
                // Assume the failure is because the key was the wrong type.
                PyErr_Clear();

                *var = keep_as_pyobject(py);
                metatype_used = UnknownType;
            }

            break;
        }

    case QMetaType::QVariantHash:
        {
            QVariantHash qh;

            if (to_QVariantHash(py, qh))
            {
                *var = QVariant(qh);
                metatype_used = UnknownType;
            }
            else
            {
                iserr = 1;
            }

            break;
        }

    case -1:
        metatype_used = QMetaType::VoidStar;

        if (SIPBytes_Check(py))
            tmp_storage.tmp_void_ptr = SIPBytes_AsString(py);
        else if (py == Py_None)
            tmp_storage.tmp_void_ptr = 0;
        else
            iserr = 1;

        break;

    default:
        if (_type)
        {
            if (_name.endsWith('*'))
            {
                ptr_class = sipForceConvertToType(py, _type, 0,
                        SIP_NO_CONVERTORS, 0, &iserr);

                variant_data = &ptr_class;
            }
            else
            {
                value_class = sipForceConvertToType(py, _type, 0,
                    SIP_NOT_NONE, &value_class_state, &iserr);

                variant_data = value_class;
            }
        }
        else
        {
            // This is a class we don't recognise.
            iserr = 1;
        }
    }

    if (iserr || PyErr_Occurred())
    {
        PyErr_Format(PyExc_TypeError,
                "unable to convert a Python '%s' object to a C++ '%s' instance",
                sipPyTypeName(Py_TYPE(py)), _name.constData());

        iserr = 1;
    }
    else if (_type == sipType_QVariant)
    {
        *var = QVariant(*reinterpret_cast<QVariant *>(variant_data));
    }
    else if (metatype_used != UnknownType)
    {
        *var = QVariant(metatype_used, variant_data);
    }

    // Release any temporary value-class instance now that QVariant will have
    // made a copy.
    if (value_class)
        sipReleaseType(value_class, _type, value_class_state);

    return (iserr == 0);
}


// Convert a Python object to a QVariant based on the type of the object.
QVariant Chimera::fromAnyPyObject(PyObject *py, int *is_err)
{
    QVariant variant;

    if (py != Py_None)
    {
        // Let any registered convertors have a go first.
        for (int i = 0; i < registeredToQVariantConvertors.count(); ++i)
        {
            QVariant var;
            bool ok;

            if (registeredToQVariantConvertors.at(i)(py, var, &ok))
            {
                *is_err = !ok;

                return var;
            }
        }

        Chimera ct;

        if (ct.parse_py_type(Py_TYPE(py)))
        {
            // If the type is a dict then try and convert it to a QVariantMap
            // if possible.
            if (Py_TYPE(py) == &PyDict_Type)
                ct._metatype = QMetaType::QVariantMap;

            // The conversion is non-strict in case the type was a dict and we
            // can't convert it to a QVariantMap.
            if (!ct.fromPyObject(py, &variant, false))
            {
                *is_err = 1;
            }
        }
        else
        {
            *is_err = 1;
        }
    }

    return variant;
}


// Convert a QVariant to Python.
PyObject *Chimera::toPyObject(const QVariant &var) const
{
    if (_type != sipType_QVariant)
    {
        // For some reason (see qvariant_p.h) an invalid QVariant can be
        // returned when a QMetaType::Void is expected.
        if (!var.isValid() && _metatype == QMetaType::Void)
        {
            Py_INCREF(Py_None);
            return Py_None;
        }

        // Handle the reverse of non-strict conversions of dict to QVariantMap,
        // ie. we want a dict but we have a QVariantMap.
        if (_metatype == PyQt_PyObject::metatype && _py_type == &PyDict_Type && var.type() == QVariant::Map)
            return from_QVariantMap(var.toMap());

        // A sanity check.
        if (_metatype != var.userType())
        {
            PyErr_Format(PyExc_TypeError,
                    "unable to convert a QVariant of type %d to a QMetaType of type %d",
                    var.userType(), _metatype);

            return 0;
        }

        // Deal with the simple case of unwrapping a Python object rather than
        // converting it.
        if (_metatype == PyQt_PyObject::metatype)
        {
            PyQt_PyObject pyobj_wrapper = var.value<PyQt_PyObject>();

            if (!pyobj_wrapper.pyobject)
            {
                PyErr_SetString(PyExc_TypeError,
                        "unable to convert a QVariant back to a Python object");

                return 0;
            }

            Py_INCREF(pyobj_wrapper.pyobject);

            return pyobj_wrapper.pyobject;
        }
    }

    // Let any registered convertors have a go first.
    for (int i = 0; i < registeredFromQVariantConvertors.count(); ++i)
    {
        PyObject *py;

        if (registeredFromQVariantConvertors.at(i)(var, &py))
            return py;
    }

    return toPyObject(const_cast<void *>(var.data()));
}


// Convert a C++ object at an arbitary address to Python.
PyObject *Chimera::toPyObject(void *cpp) const
{
    if (_metatype == PyQt_PyObject::metatype)
    {
        if (_type)
        {
            // SIP knows the type (therefore it isn't really wrapped in a
            // PyQt_PyObject) but it's not registered with Qt.
            if (_name.endsWith('*'))
                cpp = *reinterpret_cast<void **>(cpp);

            return sipConvertFromType(cpp, _type, 0);
        }
        else
        {
            // Otherwise unwrap the Python object.
            PyQt_PyObject *pyobj_wrapper = reinterpret_cast<PyQt_PyObject *>(cpp);

            if (!pyobj_wrapper->pyobject)
            {
                PyErr_SetString(PyExc_TypeError,
                        "unable to convert a QVariant back to a Python object");

                return 0;
            }

            Py_INCREF(pyobj_wrapper->pyobject);

            return pyobj_wrapper->pyobject;
        }
    }

    PyObject *py = 0;

    switch (_metatype)
    {
    case QMetaType::Bool:
        py = PyBool_FromLong(*reinterpret_cast<bool *>(cpp));
        break;

    case QMetaType::Int:
        if (_is_qflags)
        {
            py = sipConvertFromType(cpp, _type, 0);
        }
        else if (isCppEnum())
        {
            py = sipConvertFromEnum(*reinterpret_cast<int *>(cpp), _type);
        }
        else
        {
            py = SIPLong_FromLong(*reinterpret_cast<int *>(cpp));
        }

        break;

    case QMetaType::UInt:
        {
            long ui = *reinterpret_cast<unsigned int *>(cpp);

            if (ui < 0)
                py = PyLong_FromUnsignedLong((unsigned long)ui);
            else
                py = SIPLong_FromLong(ui);

            break;
        }

    case QMetaType::Double:
        py = PyFloat_FromDouble(*reinterpret_cast<double *>(cpp));
        break;

    case QMetaType::VoidStar:
        py = sipConvertFromVoidPtr(*reinterpret_cast<void **>(cpp));
        break;

    case QMetaType::Long:
        py = SIPLong_FromLong(*reinterpret_cast<long *>(cpp));
        break;

    case QMetaType::LongLong:
        py = PyLong_FromLongLong(*reinterpret_cast<qlonglong *>(cpp));
        break;

    case QMetaType::Short:
        py = SIPLong_FromLong(*reinterpret_cast<short *>(cpp));
        break;

    case QMetaType::Char:
    case QMetaType::UChar:
        py = SIPBytes_FromStringAndSize(reinterpret_cast<char *>(cpp), 1);
        break;

    case QMetaType::ULong:
        py = PyLong_FromUnsignedLong(*reinterpret_cast<unsigned long *>(cpp));
        break;

    case QMetaType::ULongLong:
        py = PyLong_FromUnsignedLongLong(*reinterpret_cast<qulonglong *>(cpp));
        break;

    case QMetaType::UShort:
        py = SIPLong_FromLong(*reinterpret_cast<unsigned short *>(cpp));
        break;

    case QMetaType::Float:
        py = PyFloat_FromDouble(*reinterpret_cast<float *>(cpp));
        break;

    case QMetaType::QObjectStar:
        py = sipConvertFromType(*reinterpret_cast<void **>(cpp),
                sipType_QObject, 0);
        break;

    case QMetaType::QVariantList:
        {
            QVariantList *ql = reinterpret_cast<QVariantList *>(cpp);

            py = PyList_New(ql->size());

            if (py)
            {
                for (int i = 0; i < ql->size(); ++i)
                {
                    PyObject *val_obj = toAnyPyObject(ql->at(i));

                    if (!val_obj)
                    {
                        Py_DECREF(py);
                        py = 0;

                        break;
                    }

                    PyList_SetItem(py, i, val_obj);
                }
            }

            break;
        }

    case QMetaType::QVariantMap:
        py = from_QVariantMap(*reinterpret_cast<QVariantMap *>(cpp));
        break;

    case QMetaType::QVariantHash:
        {
            py = PyDict_New();

            if (py)
            {
                QVariantHash *qh = reinterpret_cast<QVariantHash *>(cpp);

                for (QVariantHash::const_iterator it = qh->constBegin(); it != qh->constEnd(); ++it)
                    if (!add_variant_to_dict(py, it.key(), it.value()))
                    {
                        Py_DECREF(py);
                        py = 0;

                        break;
                    }
            }

            break;
        }

    case -1:
        {
            char *s = *reinterpret_cast<char **>(cpp);

            if (s)
            {
                py = SIPBytes_FromString(s);
            }
            else
            {
                Py_INCREF(Py_None);
                py = Py_None;
            }

            break;
        }

    default:
        if (_type)
        {
            if (sipTypeIsEnum(_type))
            {
                py = sipConvertFromEnum(*reinterpret_cast<int *>(cpp), _type);
            }
            else if (_name.endsWith('*'))
            {
                py = sipConvertFromType(*reinterpret_cast<void **>(cpp),
                        _type, 0);
            }
            else
            {
                // Make a copy as it is a value type.
                void *copy = QMetaType::create(_metatype, cpp);

                py = sipConvertFromNewType(copy, _type, 0);

                if (!py)
                    QMetaType::destroy(_metatype, copy);
            }
        }
        else if (_name.contains("_QMLTYPE_"))
        {
            // These correspond to objects defined in QML.  We assume that they
            // are all sub-classes of QObject.  If this proves not to be the
            // case then we will have to look at the first part of _name (and
            // possibly move this code the the QtQml module).
            py = sipConvertFromType(*reinterpret_cast<void **>(cpp),
                    sipType_QObject, 0);
        }
    }

    if (!py)
        PyErr_Format(PyExc_TypeError,
                "unable to convert a C++ '%s' instance to a Python object",
                _name.constData());

    return py;
}


// Add a QVariant to a Python dict with a QString key.
bool Chimera::add_variant_to_dict(PyObject *dict, const QString &key_ref,
        const QVariant &val_ref)
{
    QString *key = new QString(key_ref);
    PyObject *key_obj = sipConvertFromNewType(key, sipType_QString, 0);

    PyObject *val_obj = toAnyPyObject(val_ref);

    if (!key_obj || !val_obj || PyDict_SetItem(dict, key_obj, val_obj) < 0)
    {
        if (key_obj)
            Py_DECREF(key_obj);
        else
            delete key;

        Py_XDECREF(val_obj);

        return false;
    }

    Py_DECREF(key_obj);
    Py_DECREF(val_obj);

    return true;
}


// Convert a QVariant to a Python object based on the type of the object.
PyObject *Chimera::toAnyPyObject(const QVariant &var)
{
    if (!var.isValid())
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    const char *type_name = var.typeName();

    // Qt v5.8.0 changed the way it was handling null in QML.  We treat it as a
    // special case though there may be other implications still to be
    // discovered.
    if (qstrcmp(type_name, "std::nullptr_t") == 0)
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    const sipTypeDef *td = sipFindType(type_name);
    Chimera *ct = new Chimera;

    ct->_type = td;
    ct->_name = type_name;
    ct->_metatype = var.userType();

    if (td && sipTypeIsClass(td))
        ct->set_qflags();

    PyObject *py = ct->toPyObject(var);
    delete ct;

    return py;
}


// Wrap a Python object in a QVariant without any conversion.
QVariant Chimera::keep_as_pyobject(PyObject *py)
{
    PyQt_PyObject pyobj_wrapper(py);

    return QVariant(PyQt_PyObject::metatype, &pyobj_wrapper);
}


// Convert a Python list object to a QVariantList and return true if there was
// no error.
bool Chimera::to_QVariantList(PyObject *py, QVariantList &cpp) const
{
    Q_ASSERT(PyList_CheckExact(py));

    for (Py_ssize_t i = 0; i < PyList_Size(py); ++i)
    {
        PyObject *val_obj = PyList_GetItem(py, i);

        if (!val_obj)
            return false;

        int val_state, iserr = 0;

        QVariant *val = reinterpret_cast<QVariant *>(sipForceConvertToType(
                val_obj, sipType_QVariant, 0, SIP_NOT_NONE, &val_state,
                &iserr));

        if (iserr)
            return false;

        cpp.append(*val);

        sipReleaseType(val, sipType_QVariant, val_state);
    }

    return true;
}


// Convert a Python object to a QVariantMap and return true if there was no
// error.
bool Chimera::to_QVariantMap(PyObject *py, QVariantMap &cpp) const
{
    Q_ASSERT(PyDict_CheckExact(py));

    PyObject *key_obj, *val_obj;
    Py_ssize_t i;

    i = 0;
    while (PyDict_Next(py, &i, &key_obj, &val_obj))
    {
        int key_state, val_state, iserr = 0;

        QString *key = reinterpret_cast<QString *>(sipForceConvertToType(
                key_obj, sipType_QString, NULL, SIP_NOT_NONE, &key_state,
                &iserr));

        QVariant *val = reinterpret_cast<QVariant *>(sipForceConvertToType(
                val_obj, sipType_QVariant, NULL, SIP_NOT_NONE, &val_state,
                &iserr));

        if (iserr)
            return false;

        cpp.insert(*key, *val);

        sipReleaseType(key, sipType_QString, key_state);
        sipReleaseType(val, sipType_QVariant, val_state);
    }

    return true;
}


// Convert a QVariantMap to a Python object.
PyObject *Chimera::from_QVariantMap(const QVariantMap &qm)
{
    PyObject *py = PyDict_New();

    if (!py)
        return 0;

    for (QVariantMap::const_iterator it = qm.constBegin(); it != qm.constEnd(); ++it)
        if (!add_variant_to_dict(py, it.key(), it.value()))
        {
            Py_DECREF(py);
            return 0;
        }

    return py;
}


// Convert a Python object to a QVariantHash and return true if there was no
// error.
bool Chimera::to_QVariantHash(PyObject *py, QVariantHash &cpp) const
{
    Q_ASSERT(PyDict_CheckExact(py));

    PyObject *key_obj, *val_obj;
    Py_ssize_t i;

    i = 0;
    while (PyDict_Next(py, &i, &key_obj, &val_obj))
    {
        int key_state, val_state, iserr = 0;

        QString *key = reinterpret_cast<QString *>(sipForceConvertToType(
                key_obj, sipType_QString, NULL, SIP_NOT_NONE, &key_state,
                &iserr));

        QVariant *val = reinterpret_cast<QVariant *>(sipForceConvertToType(
                val_obj, sipType_QVariant, NULL, SIP_NOT_NONE, &val_state,
                &iserr));

        if (iserr)
            return false;

        cpp.insert(*key, *val);

        sipReleaseType(key, sipType_QString, key_state);
        sipReleaseType(val, sipType_QVariant, val_state);
    }

    return true;
}


// Return true if the type is either a C++ or Python enum.
bool Chimera::isEnum() const
{
    if (isCppEnum())
        return true;

    if (_is_qflags)
        return true;

    return (_py_type ? _py_enum_types.contains((PyObject *)_py_type) : false);
}


// Convert a Python object to C++, allocating storage as necessary.
Chimera::Storage *Chimera::fromPyObjectToStorage(PyObject *py) const
{
    Chimera::Storage *st = new Chimera::Storage(this, py);

    if (!st->isValid())
    {
        delete st;
        st = 0;
    }

    return st;
}


// Create the storage for a type.
Chimera::Storage *Chimera::storageFactory() const
{
    return new Chimera::Storage(this);
}
