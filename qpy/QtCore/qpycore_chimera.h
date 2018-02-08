// This is the interface of the Chimera and related classes.
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


#ifndef _QPYCORE_CHIMERA_H
#define _QPYCORE_CHIMERA_H


#include <Python.h>

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMetaProperty>
#include <QSet>
#include <QVariant>

#include "sipAPIQtCore.h"


// This describes a type that can be understood by Python and C++ (specifically
// Qt's meta-type system).  The implementation assumes that the GIL is held.
class Chimera
{
public:
    // Construct a copy.
    Chimera(const Chimera &other);

    // Destroy the type.
    ~Chimera();

    // Parses a Python object as a type.  Return 0 if there was an error.
    static const Chimera *parse(PyObject *obj);

    // Parses a Qt meta-property as a type.  Return 0 if there was an error.
    static const Chimera *parse(const QMetaProperty &mprop);

    // Parses a C++ type name as a type.  Return 0 if there was an error.
    static const Chimera *parse(const QByteArray &name);

    // The registered QVariant to PyObject convertors.
    typedef bool (*FromQVariantConvertorFn)(const QVariant &, PyObject **);
    typedef QList<FromQVariantConvertorFn> FromQVariantConvertors;
    static FromQVariantConvertors registeredFromQVariantConvertors;

    // Register a function for converting a Python object to a QVariant.
    typedef bool (*ToQVariantConvertorFn)(PyObject *, QVariant &, bool *);
    typedef QList<ToQVariantConvertorFn> ToQVariantConvertors;
    static ToQVariantConvertors registeredToQVariantConvertors;

    // Register a function for converting a Python object to QVariant data.
    typedef bool (*ToQVariantDataConvertorFn)(PyObject *, void *, int, bool *);
    typedef QList<ToQVariantDataConvertorFn> ToQVariantDataConvertors;
    static ToQVariantDataConvertors registeredToQVariantDataConvertors;

    class Signature
    {
    public:
        // Destroy the signature and any result type objects.
        ~Signature();

        // The list of parsed argument types.
        QList<const Chimera *> parsed_arguments;

        // The optional parsed result type.
        const Chimera *result;

        // The original normalised signature (possibly including a method name
        // but excluding a return type).
        QByteArray signature;

        // The signature as declared by the user for use in exceptions.
        QByteArray py_signature;

        // The revision of a slot.  Putting this here is an awful hack.
        int revision;

        // Return the parsed signature wrapped in a Python object.  Ownership
        // of the signature is passed to the Python object.  Return 0 if there
        // was an error.
        static PyObject *toPyObject(Signature *parsed_signature);

        // Return the parsed signature extracted from a Python object.
        static Signature *fromPyObject(PyObject *py);

        // Return the name from the signature.
        QByteArray name() const {return name(signature);}

        // Return the name from a signature.
        static QByteArray name(const QByteArray &signature);

        // Return the arguments from the signature.
        QByteArray arguments() const {return arguments(signature);}

        // Return the arguments from a signature.
        static QByteArray arguments(const QByteArray &signature);

    private:
        friend class Chimera;

        // Set if the parsed arguments are cached.
        bool _cached;

        Signature(const QByteArray &sig, bool cached)
            : result(0), signature(sig), py_signature(sig), _cached(cached) {}

        Signature(const Signature &);
        Signature &operator=(const Signature &);
    };

    // Parses a normalised C++ signature.  Return 0 if there was an error.
    static Signature *parse(const QByteArray &sig, const char *context);

    // Parses a C++ signature given as a Python tuple of types and an optional
    // name.  Return 0 if there was an error.
    static Signature *parse(PyObject *types, const char *name,
            const char *context);

    // Raise an exception after parse() has failed.
    static void raiseParseException(PyObject *type, const char *context = 0);

    class Storage
    {
    public:
        // Destroy the type and any temporary data.
        ~Storage();

        // Return the parsed type.
        const Chimera *type() const {return _parsed_type;}

        // Return the address of the stored value.
        void *address();

        // Convert the stored value to a Python object.
        PyObject *toPyObject() const;

    private:
        friend class Chimera;

        // The parsed type.
        const Chimera *_parsed_type;

        // The storage for fundamental and value types.
        QVariant _value_storage;

        // The storage for pointer types.
        void *_ptr_storage;

        // The state of any temporary stored data.
        int _tmp_state;

        // Set if the value is valid.
        bool _valid;

        Storage(const Chimera *ct);
        Storage(const Chimera *ct, PyObject *py);

        // Return true if the value is valid.
        bool isValid() const {return _valid;}

        // Return true is the value is a pointer type.
        bool isPointerType() const;

        Storage(const Storage &);
        Storage &operator=(const Storage &);
    };

    // Convert a Python object to C++, allocating storage as necessary.  Return
    // 0 if there was an error.
    Storage *fromPyObjectToStorage(PyObject *py) const;

    // Create a storage instance for this type.
    Storage *storageFactory() const;

    // Convert a Python object to C++ at a given address.  Return false if
    // there was an error.
    bool fromPyObject(PyObject *py, void *cpp) const;

    // Convert a Python object to a QVariant.  Return false if there was an
    // error.
    bool fromPyObject(PyObject *py, QVariant *var, bool strict = true) const;

    // Convert a Python object to a QVariant based on the type of the object.
    static QVariant fromAnyPyObject(PyObject *py, int *is_err);

    // Convert a QVariant to a Python object.
    PyObject *toPyObject(const QVariant &var) const;

    // Convert a C++ object at an arbitary address to a Python object.
    PyObject *toPyObject(void *cpp) const;

    // Convert a QVariant to a Python object based on the type of the object.
    static PyObject *toAnyPyObject(const QVariant &var);

    // Returns the Qt meta-type id.  It will be QMetaType::UnknownType if the
    // type isn't known to Qt's meta-type system.
    int metatype() const {return _metatype;}

    // Returns a borrowed reference to the Python type object that was used to
    // define the type (if any).
    PyObject *py_type() const {return (PyObject *)_py_type;}

    // Returns the C++ name of the type.
    const QByteArray &name() const {return _name;}

    // Returns true if the type is a C++ or Python enum (ie. has been passed to
    // Q_ENUMS or Q_FLAGS).
    bool isEnum() const;

    // Returns the SIP generated type structure.
    const sipTypeDef *typeDef() const {return _type;}

    // Register a type as a Python enum.
    static void registerPyEnum(PyObject *enum_type);

private:
    // The generated type structure.  This may be 0 if the type is known by Qt
    // but hasn't been wrapped.
    const sipTypeDef *_type;

    // The Python type object if a Python type (rather than a C++ type string)
    // was given.
    PyTypeObject *_py_type;

    // The Qt meta-type.  This will always be valid but may refer to the
    // PyObject wrapper if the underlying type isn't known to Qt.
    int _metatype;

    // Set if the meta-type needn't be exact.
    bool _inexact;

    // Set if the the type is derived from QFlags.
    bool _is_qflags;

    // The C++ name of the type.
    QByteArray _name;

    // The registered int types.
    static QSet<PyObject *> _py_enum_types;

    // The cache of previously parsed argument type lists.
    static QHash<QByteArray, QList<const Chimera *> > _previously_parsed;

    Chimera();

    bool parse_cpp_type(const QByteArray &type);
    bool parse_py_type(PyTypeObject *type_obj);
    sipAssignFunc get_assign_helper() const;
    void set_qflags();
    bool isCppEnum() const {return (_type && sipTypeIsEnum(_type));}
    bool to_QVariantList(PyObject *py, QVariantList &cpp) const;
    bool to_QVariantMap(PyObject *py, QVariantMap &cpp) const;
    static PyObject *from_QVariantMap(const QVariantMap &qm);
    bool to_QVariantHash(PyObject *py, QVariantHash &cpp) const;
    static bool add_variant_to_dict(PyObject *dict, const QString &key_ref,
            const QVariant &val_ref);
    static void raiseParseCppException(const char *type,
            const char *context = 0);
    static QVariant keep_as_pyobject(PyObject *py);
    static int extract_raw_type(const QByteArray &type, QByteArray &raw_type);
    static QByteArray resolve_types(const QByteArray &type);

    Chimera &operator=(const Chimera &);
};


#endif
