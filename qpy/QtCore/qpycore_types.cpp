// This contains the meta-type used by PyQt.
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
#include <QList>
#include <QMap>
#include <QMetaObject>
#include <QMetaType>
#include <QPair>

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
static int trawl_hierarchy(PyTypeObject *pytype, qpycore_metaobject *qo,
        QMetaObjectBuilder &builder, QList<const qpycore_pyqtSignal *> &psigs,
        QMap<uint, PropertyData> &pprops);
static int trawl_type(PyTypeObject *pytype, qpycore_metaobject *qo,
        QMetaObjectBuilder &builder, QList<const qpycore_pyqtSignal *> &psigs,
        QMap<uint, PropertyData> &pprops);
static qpycore_metaobject *create_dynamic_metaobject(sipWrapperType *wt);
static const QMetaObject *get_scope_qmetaobject(const Chimera *ct);


// The handler invoked whenever a Python sub-class of QObject is defined.
int qpycore_new_user_type_handler(sipWrapperType *wt)
{
    const pyqt5ClassPluginDef *cpd = reinterpret_cast<const pyqt5ClassPluginDef *>(sipTypePluginData(sipTypeFromPyTypeObject((PyTypeObject *)wt)));

    Q_ASSERT(cpd);

    // Create a dynamic meta-object as its base wrapped type has a static Qt
    // meta-object.
    if (cpd->static_metaobject)
    {
        qpycore_metaobject *qo = create_dynamic_metaobject(wt);

        if (!qo)
            return -1;

        sipSetTypeUserData(wt, qo);
    }

    return 0;
}


// Create a dynamic meta-object for a Python type by introspecting its
// attributes.  Note that it leaks if the type is deleted.
static qpycore_metaobject *create_dynamic_metaobject(sipWrapperType *wt)
{
    PyTypeObject *pytype = (PyTypeObject *)wt;
    qpycore_metaobject *qo = new qpycore_metaobject;
    QMetaObjectBuilder builder;

    // Get any class info.
    QList<ClassInfo> class_info_list = qpycore_get_class_info_list();

    // Get any enums/flags.
    QList<EnumsFlags> enums_flags_list = qpycore_get_enums_flags_list();

    // Get the super-type's meta-object.
    PyTypeObject *tp_base;

#if PY_VERSION_HEX >= 0x03040000
    tp_base = reinterpret_cast<PyTypeObject *>(
            PyType_GetSlot(pytype, Py_tp_base));
#else
    tp_base = pytype->tp_base;
#endif

    builder.setSuperClass(qpycore_get_qmetaobject((sipWrapperType *)tp_base));

    // Get the name of the type.  Dynamic types have simple names.
    builder.setClassName(sipPyTypeName(pytype));

    // Go through the class hierarchy getting all PyQt properties, slots and
    // signals.

    QList<const qpycore_pyqtSignal *> psigs;
    QMap<uint, PropertyData> pprops;

    if (trawl_hierarchy(pytype, qo, builder, psigs, pprops) < 0)
        return 0;

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

        QMetaEnumBuilder enum_builder = builder.addEnumerator(ef.name);
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
        const char *prop_name = SIPBytes_AsString(pprop.first);
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
                return 0;
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
                prop_type = sipPyTypeName(
                        (PyTypeObject *)pp->pyqtprop_parsed_type->py_type());
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

        QMetaPropertyBuilder prop_builder = builder.addProperty(
                QByteArray(prop_name), prop_type, notifier_id);

        // Reset the defaults.
        prop_builder.setReadable(false);
        prop_builder.setWritable(false);

        // Enum or flag.
        if (pp->pyqtprop_parsed_type->isEnum())
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

    return qo;
}


// Trawl a type's hierarchy looking for any slots, signals or properties.
static int trawl_hierarchy(PyTypeObject *pytype, qpycore_metaobject *qo,
        QMetaObjectBuilder &builder, QList<const qpycore_pyqtSignal *> &psigs,
        QMap<uint, PropertyData> &pprops)
{
    if (trawl_type(pytype, qo, builder, psigs, pprops) < 0)
        return -1;

    PyObject *tp_bases;

#if PY_VERSION_HEX >= 0x03040000
    if (PyType_HasFeature(pytype, Py_TPFLAGS_HEAPTYPE))
        tp_bases = reinterpret_cast<PyObject *>(
                PyType_GetSlot(pytype, Py_tp_bases));
    else
        tp_bases = 0;
#else
    tp_bases = pytype->tp_bases;
#endif

    if (!tp_bases)
        return 0;

    Q_ASSERT(PyTuple_Check(tp_bases));

    for (Py_ssize_t i = 0; i < PyTuple_Size(tp_bases); ++i)
    {
        PyTypeObject *sup = (PyTypeObject *)PyTuple_GetItem(tp_bases, i);

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
    Py_ssize_t pos = 0;
    PyObject *key, *value, *dict;

    dict = sipPyTypeDict(pytype);

    while (PyDict_Next(dict, &pos, &key, &value))
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
                for (Py_ssize_t i = 0; i < PyList_Size(sig_obj); ++i)
                {
                    // Set up the skeleton slot.
                    PyObject *decoration = PyList_GetItem(sig_obj, i);
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
            if (PyObject_TypeCheck(value, qpycore_pyqtProperty_TypeObject))
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
            else if (PyObject_TypeCheck(value, qpycore_pyqtSignal_TypeObject))
            {
                // It is a signal.

                if (!ascii_key)
                    return -1;

                qpycore_pyqtSignal *ps = (qpycore_pyqtSignal *)value;

                // Make sure the signal has a name.
                qpycore_set_signal_name(ps, sipPyTypeName(pytype), ascii_key);

                // Add all the overloads.
                do
                {
                    psigs.append(ps);
                    ps = ps->next;
                }
                while (ps);

                Py_DECREF(key);
            }
            else
            {
                PyErr_Clear();
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
    if (!ct->typeDef())
        return 0;

    const sipTypeDef *td = sipTypeScope(ct->typeDef());

    if (!td)
        return 0;

    // Check the scope is wrapped by PyQt.
    if (!qpycore_is_pyqt_class(td))
        return 0;

    return qpycore_get_qmetaobject((sipWrapperType *)sipTypeAsPyTypeObject(td));
}


// Return the QMetaObject for a type.
const QMetaObject *qpycore_get_qmetaobject(sipWrapperType *wt,
        const sipTypeDef *base_td)
{
    if (wt)
    {
        qpycore_metaobject *qo = reinterpret_cast<qpycore_metaobject *>(
                sipGetTypeUserData(wt));

        // See if it has a dynamic meta-object.
        if (qo)
            return qo->mo;
    }

    // Get the static meta-object if there is one.
    if (!base_td)
    {
        if (!wt)
            return 0;

        base_td = sipTypeFromPyTypeObject((PyTypeObject *)wt);

        if (!base_td)
            return 0;
    }

    const pyqt5ClassPluginDef *cpd = reinterpret_cast<const pyqt5ClassPluginDef *>(sipTypePluginData(base_td));

    return reinterpret_cast<const QMetaObject *>(cpd->static_metaobject);
}
