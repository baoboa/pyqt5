// This contains the meta-type used by PyQt.
//
// Copyright (c) 2014 Riverbank Computing Limited <info@riverbankcomputing.com>
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
#include <QList>
#include <QMap>
#include <QMetaObject>
#include <QMetaType>
#include <QPair>

// Defining PYQT_QOBJECT_GUARD will enable code that tries to detect when a
// QObject created by Qt (ie. where we don't have a generated virtual dtor) is
// destroyed thus allowing us to raise an exception rather than crashing.
// However, there are problems:
//
// 1. The obvious choice to implement this is QWeakPointer.  However it cannot
//    be used across threads.  The code checks to see if QObject is in the
//    current thread, but can't do anything about the QObject being moved to
//    another thread later on.
//
// 2. A QObject dtor may invoke a Python reimplementation that causes the
//    QObject to be wrapped.  QWeakPointer will fail an assertion if it is
//    being created for a QObject that is being deleted (rather than the more
//    sensible behaviour of creating a null QWeakPointer).
//
// For the moment we disable the code until we have a better solution.  (If you
// connect the destroyed signal of a QObject then move the QObject to a
// different thread, will the signal be delivered properly?)
#if defined(PYQT_QOBJECT_GUARD)
#include <QThread>
#include <QWeakPointer>
#endif

#include "qpycore_chimera.h"
#include "qpycore_classinfo.h"
#include "qpycore_enums_flags.h"
#include "qpycore_misc.h"
#include "qpycore_objectified_strings.h"
#include "qpycore_pyqtproperty.h"
#include "qpycore_pyqtsignal.h"
#include "qpycore_pyqtslot.h"
#include "qpycore_qmetaobjectbuilder.h"
#include "qpycore_types.h"

#include "sipAPIQtCore.h"


// A tuple of the property name and definition.
typedef QPair<PyObject *, PyObject *> PropertyData;


// Forward declarations.
extern "C" {
static int pyqtWrapperType_init(pyqtWrapperType *self, PyObject *args,
        PyObject *kwds);
#if defined(PYQT_QOBJECT_GUARD)
static PyObject *pyqtWrapperType_call(PyObject *type, PyObject *args,
        PyObject *kwds);
static void *qpointer_access_func(sipSimpleWrapper *w, AccessFuncOp op);
#endif
}

static int trawl_hierarchy(PyTypeObject *pytype, qpycore_metaobject *qo,
        QMetaObjectBuilder &builder, QList<const qpycore_pyqtSignal *> &psigs,
        QMap<uint, PropertyData> &pprops);
static int trawl_type(PyTypeObject *pytype, qpycore_metaobject *qo,
        QMetaObjectBuilder &builder, QList<const qpycore_pyqtSignal *> &psigs,
        QMap<uint, PropertyData> &pprops);
static int create_dynamic_metaobject(pyqtWrapperType *pyqt_wt);
static const QMetaObject *get_scope_qmetaobject(const Chimera *ct);
static const QMetaObject *get_qmetaobject(pyqtWrapperType *pyqt_wt);


// The meta-type for PyQt classes.
PyTypeObject qpycore_pyqtWrapperType_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    SIP_TPNAME_CAST("PyQt5.QtCore.pyqtWrapperType"),    /* tp_name */
    sizeof (pyqtWrapperType),   /* tp_basicsize */
    0,                      /* tp_itemsize */
    0,                      /* tp_dealloc */
    0,                      /* tp_print */
    0,                      /* tp_getattr */
    0,                      /* tp_setattr */
    0,                      /* tp_compare */
    0,                      /* tp_repr */
    0,                      /* tp_as_number */
    0,                      /* tp_as_sequence */
    0,                      /* tp_as_mapping */
    0,                      /* tp_hash */
#if defined(PYQT_QOBJECT_GUARD)
    pyqtWrapperType_call,   /* tp_call */
#else
    0,                      /* tp_call */
#endif
    0,                      /* tp_str */
    0,                      /* tp_getattro */
    0,                      /* tp_setattro */
    0,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    0,                      /* tp_doc */
    0,                      /* tp_traverse */
    0,                      /* tp_clear */
    0,                      /* tp_richcompare */
    0,                      /* tp_weaklistoffset */
    0,                      /* tp_iter */
    0,                      /* tp_iternext */
    0,                      /* tp_methods */
    0,                      /* tp_members */
    0,                      /* tp_getset */
    0,                      /* tp_base */
    0,                      /* tp_dict */
    0,                      /* tp_descr_get */
    0,                      /* tp_descr_set */
    0,                      /* tp_dictoffset */
    (initproc)pyqtWrapperType_init, /* tp_init */
    0,                      /* tp_alloc */
    0,                      /* tp_new */
    0,                      /* tp_free */
    0,                      /* tp_is_gc */
    0,                      /* tp_bases */
    0,                      /* tp_mro */
    0,                      /* tp_cache */
    0,                      /* tp_subclasses */
    0,                      /* tp_weaklist */
    0,                      /* tp_del */
    0,                      /* tp_version_tag */
#if PY_VERSION_HEX >= 0x03040000
    0,                      /* tp_finalize */
#endif
};


// The type init slot.
static int pyqtWrapperType_init(pyqtWrapperType *self, PyObject *args,
        PyObject *kwds)
{
    // Let the super-type complete the basic initialisation.
    if (sipWrapperType_Type->tp_init((PyObject *)self, args, kwds) < 0)
        return -1;

    pyqt5ClassTypeDef *pyqt_td = (pyqt5ClassTypeDef *)((sipWrapperType *)self)->type;

    if (pyqt_td && !sipIsExactWrappedType((sipWrapperType *)self))
    {
        // Create a dynamic meta-object as its base wrapped type has a static
        // Qt meta-object.
        if (pyqt_td->static_metaobject && create_dynamic_metaobject(self) < 0)
            return -1;
    }

    return 0;
}


// Create a dynamic meta-object for a Python type by introspecting its
// attributes.  Note that it leaks if the type is deleted.
static int create_dynamic_metaobject(pyqtWrapperType *pyqt_wt)
{
    PyTypeObject *pytype = (PyTypeObject *)pyqt_wt;
    qpycore_metaobject *qo = new qpycore_metaobject;
    QMetaObjectBuilder builder;

    // Get any class info.
    QList<ClassInfo> class_info_list = qpycore_get_class_info_list();

    // Get any enums/flags.
    QList<EnumsFlags> enums_flags_list = qpycore_get_enums_flags_list();

    // Get the super-type's meta-object.
    builder.setSuperClass(get_qmetaobject((pyqtWrapperType *)pytype->tp_base));

    // Get the name of the type.  Dynamic types have simple names.
    builder.setClassName(pytype->tp_name);

    // Go through the class hierarchy getting all PyQt properties, slots and
    // signals.

    QList<const qpycore_pyqtSignal *> psigs;
    QMap<uint, PropertyData> pprops;

    if (trawl_hierarchy(pytype, qo, builder, psigs, pprops) < 0)
        return -1;

    qo->nr_signals = psigs.count();

    // Initialise the header section of the data table.  Note that Qt v4.5
    // introduced revision 2 which added constructors.  However the design is
    // broken in that the static meta-call function doesn't provide enough
    // information to determine which Python sub-class of a Qt class is to be
    // created.  So we stick with revision 1 (and don't allow pyqtSlot() to
    // decorate __init__).

    // Set up any class information.
    for (int i = 0; i < class_info_list.count(); ++i)
    {
        const ClassInfo &ci = class_info_list.at(i);

        builder.addClassInfo(ci.first, ci.second);
    }

    // Set up any enums/flags.
    for (int i = 0; i < enums_flags_list.count(); ++i)
    {
        const EnumsFlags &ef = enums_flags_list.at(i);

        QByteArray scoped_name(pytype->tp_name);
        scoped_name.append("::");
        scoped_name.append(ef.name);
        QMetaEnumBuilder enum_builder = builder.addEnumerator(scoped_name);

        enum_builder.setIsFlag(ef.isFlag);

        QHash<QByteArray, int>::const_iterator it = ef.keys.constBegin();

        while (it != ef.keys.constEnd())
        {
            enum_builder.addKey(it.key(), it.value());
            ++it;
        }
    }

    // Add the signals to the meta-object.
    for (int g = 0; g < qo->nr_signals; ++g)
    {
        const qpycore_pyqtSignal *ps = psigs.at(g);

        QMetaMethodBuilder signal_builder = builder.addSignal(
                ps->parsed_signature->signature.mid(1));

        if (ps->parameter_names)
            signal_builder.setParameterNames(*ps->parameter_names);

        signal_builder.setRevision(ps->revision);
    }

    // Add the slots to the meta-object.
    for (int s = 0; s < qo->pslots.count(); ++s)
    {
        const Chimera::Signature *slot_signature = qo->pslots.at(s)->slotSignature();
        const QByteArray &sig = slot_signature->signature;

        QMetaMethodBuilder slot_builder = builder.addSlot(sig);

        // Add any type.
        if (slot_signature->result)
            slot_builder.setReturnType(slot_signature->result->name());

        slot_builder.setRevision(slot_signature->revision);
    }

    // Add the properties to the meta-object.
    QMapIterator<uint, PropertyData> it(pprops);

    for (int p = 0; it.hasNext(); ++p)
    {
        it.next();

        const PropertyData &pprop = it.value();
        const char *prop_name = SIPBytes_AS_STRING(pprop.first);
        qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)pprop.second;
        int notifier_id;

        if (pp->pyqtprop_notify)
        {
            qpycore_pyqtSignal *ps = (qpycore_pyqtSignal *)pp->pyqtprop_notify;
            const QByteArray &sig = ps->parsed_signature->signature;

            notifier_id = builder.indexOfSignal(sig.mid(1));

            if (notifier_id < 0)
            {
                PyErr_Format(PyExc_TypeError,
                        "the notify signal '%s' was not defined in this class",
                        sig.constData() + 1);

                // Note that we leak the property name.
                return -1;
            }
        }
        else
        {
            notifier_id = -1;
        }

        // A Qt v5 revision 7 meta-object holds the QMetaType::Type of the type
        // or its name if it is unresolved (ie. not known to the type system).
        // In Qt v4 both are held.  For QObject sub-classes Chimera will fall
        // back to the QMetaType::QObjectStar if there is no specific meta-type
        // for the sub-class.  This means that, for Qt v4,
        // QMetaProperty::read() can handle the type.  However, Qt v5 doesn't
        // know that the unresolved type is a QObject sub-class.  Therefore we
        // have to tell it that the property is a QObject, rather than the
        // sub-class.  This means that QMetaProperty.typeName() will always
        // return "QObject*".
        QByteArray prop_type;

        if (pp->pyqtprop_parsed_type->metatype() == QMetaType::QObjectStar)
        {
            // However, if the type is a Python sub-class of QObject then we
            // use the name of the Python type.  This anticipates that the type
            // is one that will be proxied by QML at some point.
            if (pp->pyqtprop_parsed_type->typeDef() == sipType_QObject)
            {
                prop_type = ((PyTypeObject *)pp->pyqtprop_parsed_type->py_type())->tp_name;
                prop_type.append('*');
            }
            else
            {
                prop_type = "QObject*";
            }
        }
        else
        {
            prop_type = pp->pyqtprop_parsed_type->name();
        }

        QMetaPropertyBuilder prop_builder = builder.addProperty(prop_name,
                prop_type, notifier_id);

        // Reset the defaults.
        prop_builder.setReadable(false);
        prop_builder.setWritable(false);

        // Enum or flag.
        if (pp->pyqtprop_parsed_type->isEnum() || pp->pyqtprop_parsed_type->isFlag())
        {
            prop_builder.setEnumOrFlag(true);
        }

        if (pp->pyqtprop_get && PyCallable_Check(pp->pyqtprop_get))
        {
            // Readable.
            prop_builder.setReadable(true);
        }

        if (pp->pyqtprop_set && PyCallable_Check(pp->pyqtprop_set))
        {
            // Writable.
            prop_builder.setWritable(true);

            // See if the name of the setter follows the Designer convention.
            // If so tell the UI compilers not to use setProperty().
            PyObject *setter_name_obj = PyObject_GetAttr(pp->pyqtprop_set,
                    qpycore_dunder_name);

            if (setter_name_obj)
            {
                PyObject *ascii_obj = setter_name_obj;
                const char *ascii = sipString_AsASCIIString(&ascii_obj);
                Py_DECREF(setter_name_obj);

                if (ascii)
                {
                    if (qstrlen(ascii) > 3 && ascii[0] == 's' &&
                            ascii[1] == 'e' && ascii[2] == 't' &&
                            ascii[3] == toupper(prop_name[0]) &&
                            qstrcmp(&ascii[4], &prop_name[1]) == 0)
                        prop_builder.setStdCppSet(true);
                }

                Py_DECREF(ascii_obj);
            }

            PyErr_Clear();
        }

        if (pp->pyqtprop_reset && PyCallable_Check(pp->pyqtprop_reset))
        {
            // Resetable.
            prop_builder.setResettable(true);
        }

        // Add the property flags.  Note that Qt4 always seems to have
        // ResolveEditable set but QMetaObjectBuilder doesn't provide an API
        // call to do it.
        prop_builder.setDesignable(pp->pyqtprop_flags & 0x00001000);
        prop_builder.setScriptable(pp->pyqtprop_flags & 0x00004000);
        prop_builder.setStored(pp->pyqtprop_flags & 0x00010000);
        prop_builder.setUser(pp->pyqtprop_flags & 0x00100000);
        prop_builder.setConstant(pp->pyqtprop_flags & 0x00000400);
        prop_builder.setFinal(pp->pyqtprop_flags & 0x00000800);

        prop_builder.setRevision(pp->pyqtprop_revision);

        // Save the property data for qt_metacall().  (We already have a
        // reference.)
        qo->pprops.append(pp);

        // We've finished with the property name.
        Py_DECREF(pprop.first);
    }

    // Initialise the rest of the meta-object.
    qo->mo = builder.toMetaObject();

    // Save the meta-object.
    pyqt_wt->metaobject = qo;

    return 0;
}


// Trawl a type's hierarchy looking for any slots, signals or properties.
static int trawl_hierarchy(PyTypeObject *pytype, qpycore_metaobject *qo,
        QMetaObjectBuilder &builder, QList<const qpycore_pyqtSignal *> &psigs,
        QMap<uint, PropertyData> &pprops)
{
    if (trawl_type(pytype, qo, builder, psigs, pprops) < 0)
        return -1;

    if (!pytype->tp_bases)
        return 0;

    Q_ASSERT(PyTuple_Check(pytype->tp_bases));

    for (SIP_SSIZE_T i = 0; i < PyTuple_GET_SIZE(pytype->tp_bases); ++i)
    {
        PyTypeObject *sup = (PyTypeObject *)PyTuple_GET_ITEM(pytype->tp_bases, i);

#if PY_MAJOR_VERSION < 3
        /* Ignore classic classes as mixins. */
        if (PyClass_Check((PyObject *)sup))
            continue;
#endif

        if (PyType_IsSubtype(sup, sipTypeAsPyTypeObject(sipType_QObject)))
            continue;

        if (trawl_hierarchy(sup, qo, builder, psigs, pprops) < 0)
            return -1;
    }

    return 0;
}


// Trawl a type's dict looking for any slots, signals or properties.
static int trawl_type(PyTypeObject *pytype, qpycore_metaobject *qo,
        QMetaObjectBuilder &builder, QList<const qpycore_pyqtSignal *> &psigs,
        QMap<uint, PropertyData> &pprops)
{
    SIP_SSIZE_T pos = 0;
    PyObject *key, *value;

    while (PyDict_Next(pytype->tp_dict, &pos, &key, &value))
    {
        // See if it is a slot, ie. it has been decorated with pyqtSlot().
        PyObject *sig_obj = PyObject_GetAttr(value,
                qpycore_dunder_pyqtsignature);

        if (sig_obj)
        {
            // Make sure it is a list and not some legitimate attribute that
            // happens to use our special name.
            if (PyList_Check(sig_obj))
            {
                for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sig_obj); ++i)
                {
                    // Set up the skeleton slot.
                    PyObject *decoration = PyList_GET_ITEM(sig_obj, i);
                    Chimera::Signature *slot_signature = Chimera::Signature::fromPyObject(decoration);

                    PyQtSlot *slot = new PyQtSlot(value, (PyObject *)pytype,
                            slot_signature);;

                    qo->pslots.append(slot);
                }
            }

            Py_DECREF(sig_obj);
        }
        else
        {
            PyErr_Clear();

            // Make sure the key is an ASCII string.  Delay the error checking
            // until we know we actually need it.
            const char *ascii_key = sipString_AsASCIIString(&key);

            // See if the value is of interest.
            if (PyObject_TypeCheck(value, &qpycore_pyqtProperty_Type))
            {
                // It is a property.

                if (!ascii_key)
                    return -1;

                Py_INCREF(value);

                qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)value;

                pprops.insert(pp->pyqtprop_sequence, PropertyData(key, value));

                // See if the property has a scope.  If so, collect all
                // QMetaObject pointers that are not in the super-class
                // hierarchy.
                const QMetaObject *mo = get_scope_qmetaobject(pp->pyqtprop_parsed_type);

                if (mo)
                    builder.addRelatedMetaObject(mo);
            }
            else if (PyObject_TypeCheck(value, &qpycore_pyqtSignal_Type))
            {
                // It is a signal.

                if (!ascii_key)
                    return -1;

                qpycore_pyqtSignal *ps = (qpycore_pyqtSignal *)value;

                // Make sure the signal has a name.
                qpycore_set_signal_name(ps, pytype->tp_name, ascii_key);

                // Add all the overloads.
                do
                {
                    psigs.append(ps);
                    ps = ps->next;
                }
                while (ps);

                Py_DECREF(key);
            }
        }
    }

    return 0;
}


// Return the QMetaObject for an enum type's scope.
static const QMetaObject *get_scope_qmetaobject(const Chimera *ct)
{
    // Check it is an enum.
    if (!ct->isEnum())
        return 0;

    // Check it has a scope.
    const sipTypeDef *td = sipTypeScope(ct->typeDef());

    if (!td)
        return 0;

    // Check the scope is wrapped by PyQt.
    if (!qpycore_is_pyqt_class(td))
        return 0;

    return get_qmetaobject((pyqtWrapperType *)sipTypeAsPyTypeObject(td));
}


// Return the QMetaObject for a type.
static const QMetaObject *get_qmetaobject(pyqtWrapperType *pyqt_wt)
{
    // See if it's a sub-type of a wrapped type.
    if (pyqt_wt->metaobject)
        return pyqt_wt->metaobject->mo;

    // It's a wrapped type.
    return reinterpret_cast<const QMetaObject *>(((pyqt5ClassTypeDef *)((sipWrapperType *)pyqt_wt)->type)->static_metaobject);
}


#if defined(PYQT_QOBJECT_GUARD)

// Reimplemented to wrap any QObjects created internally by Qt so that we can
// detect if they get destroyed.
static PyObject *pyqtWrapperType_call(PyObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *self = sipWrapperType_Type->tp_call(type, args, kwds);

    // See if the object was created and it is a QObject sub-class.
    if (self && PyObject_TypeCheck(self, sipTypeAsPyTypeObject(sipType_QObject)))
    {
        sipSimpleWrapper *w = (sipSimpleWrapper *)self;

        // See if it is created internally by Qt and doesn't already have an
        // access function (e.g. qApp).
        if (!sipIsDerived(w) && !w->access_func)
        {
            QObject *qobj = reinterpret_cast<QObject *>(w->data);

            // We can only guard objects in the same thread.
            if (qobj->thread() == QThread::currentThread())
            {
                QWeakPointer<QObject> *guard = new QWeakPointer<QObject>(qobj);

                w->data = guard;
                w->access_func = qpointer_access_func;
            }
        }
    }

    return self;
}


// The access function for guarded QObject pointers.
static void *qpointer_access_func(sipSimpleWrapper *w, AccessFuncOp op)
{
    QWeakPointer<QObject> *guard = reinterpret_cast<QWeakPointer<QObject> *>(w->data);
    void *addr;

    switch (op)
    {
    case UnguardedPointer:
        addr = guard->data();
        break;

    case GuardedPointer:
        addr = guard->isNull() ? 0 : guard->data();
        break;

    case ReleaseGuard:
        delete guard;
        addr = 0;
        break;
    }

    return addr;
}

#endif
