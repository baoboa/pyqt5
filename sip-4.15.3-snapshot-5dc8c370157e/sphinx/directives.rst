Directives
==========

In this section we describe each of the directives that can be used in
specification files.  All directives begin with ``%`` as the first
non-whitespace character in a line.

Some directives have arguments or contain blocks of code or documentation.  In
the following descriptions these are shown in *italics*.  Optional arguments
are enclosed in [*brackets*].

Some directives are used to specify handwritten code.  Handwritten code must
not define names that start with the prefix ``sip``.


Revised Directive Syntax
------------------------

.. versionadded:: 4.12

The directive syntax used in older versions has some problems:

- it is inconsistent in places

- it can be problematic to parse

- it is inflexible.

SIP v4.12 introduced a revised directive syntax that addresses these problems
and deprecates the old syntax.  Support for the old syntax will be removed in
SIP v5.

The revised syntax is:

.. parsed-literal::

    %Directive(arg = value, ...)
    {
        %Sub-directive
        ...
    };

A directive may have a number of arguments enclosed in parentheses followed by
a number of sub-directives enclosed in braces.  Individual arguments and
sub-directives may be optional.

Arguments may be specified in any order.  If no arguments are specified then
the parentheses can be omitted.  If a directive has only one compulsory
argument then its value may be specified after the directive name and instead
of the parentheses.

Sub-directives may be specified in any order.  If no sub-directives are
specified then the braces can be omitted.

If a directive is used to specify handwritten code then it may not have
sub-directives.  In this case the syntax is:

.. parsed-literal::

    %Directive(arg = value, ...)
        *code*
    %End

Ordinary C/C++ statements may also have sub-directives.  These will also be
enclosed in braces.

The documentation for each directive describes the revised syntax.  The older
syntax should be used if compatibility with versions of SIP prior to v4.12 is
required.


List of Directives
------------------

.. directive:: %AccessCode

.. parsed-literal::

    %AccessCode
        *code*
    %End

This sub-directive is used in the declaration of an instance of a wrapped class
or structure, or a pointer to such an instance.  You use it to provide
handwritten code that overrides the default behaviour.

For example::

    class Klass;

    Klass *klassInstance
    {
        %AccessCode
            // In this contrived example the C++ library we are wrapping
            // defines klassInstance as Klass ** (which SIP doesn't support) so
            // we explicitly dereference it.
            if (klassInstance && *klassInstance)
                return *klassInstance;

            // This will get converted to None.
            return 0;
        %End
    };

.. seealso:: :directive:`%GetCode`, :directive:`%SetCode`


.. directive:: %API

.. versionadded:: 4.9

.. parsed-literal::

    %API(name = *name*, version = *integer*)

This directive is used to define an API and set its default version number.  A
version number must be greater than or equal to 1.

See :ref:`ref-incompat-apis` for more detail.

For example::

    %API(name=PyQt4, version=1)


.. directive:: %AutoPyName

.. versionadded:: 4.12

.. parsed-literal::

    %AutoPyName(remove_leading = *string*)

This is a sub-directive of the :directive:`%Module` directive used to specify a
rule for automatically providing Python names for classes, enums, functions,
methods, variables and exceptions.  The directive may be specified any number
of times and each rule will be applied in turn.  Rules will not be applied if
an item has been given an explicit Python name.

``remove_leading`` is a string that will be removed from the beginning of any
C++ or C name.

For example::

    %Module PyQt4.QtCore
    {
        %AutoPyName(remove_leading="Q")
    }


.. directive:: %BIGetBufferCode

.. parsed-literal::

    %BIGetBufferCode
        *code*
    %End

This directive (along with :directive:`%BIReleaseBufferCode`) is used to
specify code that implements the buffer interface of Python v3.  If Python v2
is being used then this is ignored.

The following variables are made available to the handwritten code:

Py_buffer \*sipBuffer
    This is a pointer to the Python buffer structure that the handwritten code
    must populate.

*type* \*sipCpp
    This is a pointer to the structure or class instance.  Its *type* is a
    pointer to the structure or class.

int sipFlags
    These are the flags that specify what elements of the ``sipBuffer``
    structure must be populated.

int sipRes
    The handwritten code should set this to 0 if there was no error or -1 if
    there was an error.

PyObject \*sipSelf
    This is the Python object that wraps the structure or class instance, i.e.
    ``self``.


.. directive:: %BIGetCharBufferCode

.. parsed-literal::

    %BIGetCharBufferCode
        *code*
    %End

This directive (along with :directive:`%BIGetReadBufferCode`,
:directive:`%BIGetSegCountCode` and :directive:`%BIGetWriteBufferCode`) is used
to specify code that implements the buffer interface of Python v2.  If Python
v3 is being used then this is ignored.

The following variables are made available to the handwritten code:

*type* \*sipCpp
    This is a pointer to the structure or class instance.  Its *type* is a
    pointer to the structure or class.

void \*\*sipPtrPtr
    This is the pointer used to return the address of the character buffer.

:c:macro:`SIP_SSIZE_T` sipRes
    The handwritten code should set this to the length of the character buffer
    or -1 if there was an error.

:c:macro:`SIP_SSIZE_T` sipSegment
    This is the number of the segment of the character buffer.

PyObject \*sipSelf
    This is the Python object that wraps the structure or class instance, i.e.
    ``self``.


.. directive:: %BIGetReadBufferCode

.. parsed-literal::

    %BIGetReadBufferCode
        *code*
    %End

This directive (along with :directive:`%BIGetCharBufferCode`,
:directive:`%BIGetSegCountCode` and :directive:`%BIGetWriteBufferCode`) is used
to specify code that implements the buffer interface of Python v2.  If
Python v3 is being used then this is ignored.

The following variables are made available to the handwritten code:

*type* \*sipCpp
    This is a pointer to the structure or class instance.  Its *type* is a
    pointer to the structure or class.

void \*\*sipPtrPtr
    This is the pointer used to return the address of the read buffer.

:c:macro:`SIP_SSIZE_T` sipRes
    The handwritten code should set this to the length of the read buffer or
    -1 if there was an error.

:c:macro:`SIP_SSIZE_T` sipSegment
    This is the number of the segment of the read buffer.

PyObject \*sipSelf
    This is the Python object that wraps the structure or class instance, i.e.
    ``self``.


.. directive:: %BIGetSegCountCode

.. parsed-literal::

    %BIGetSegCountCode
        *code*
    %End

This directive (along with :directive:`%BIGetCharBufferCode`,
:directive:`%BIGetReadBufferCode` and :directive:`%BIGetWriteBufferCode`) is
used to specify code that implements the buffer interface of Python v2.  If
Python v3 is being used then this is ignored.

The following variables are made available to the handwritten code:

*type* \*sipCpp
    This is a pointer to the structure or class instance.  Its *type* is a
    pointer to the structure or class.

:c:macro:`SIP_SSIZE_T` \*sipLenPtr
    This is the pointer used to return the total length in bytes of all
    segments of the buffer.

:c:macro:`SIP_SSIZE_T` sipRes
    The handwritten code should set this to the number of segments that make
    up the buffer.

PyObject \*sipSelf
    This is the Python object that wraps the structure or class instance, i.e.
    ``self``.


.. directive:: %BIGetWriteBufferCode

.. parsed-literal::

    %BIGetWriteBufferCode
        *code*
    %End

This directive (along with :directive:`%BIGetCharBufferCode`,
:directive:`%BIGetReadBufferCode` and :directive:`%BIGetSegCountCode` is used
to specify code that implements the buffer interface of Python v2.  If Python
v3 is being used then this is ignored.

The following variables are made available to the handwritten code:

*type* \*sipCpp
    This is a pointer to the structure or class instance.  Its *type* is a
    pointer to the structure or class.

void \*\*sipPtrPtr
    This is the pointer used to return the address of the write buffer.

:c:macro:`SIP_SSIZE_T` sipRes
    The handwritten code should set this to the length of the write buffer or
    -1 if there was an error.

:c:macro:`SIP_SSIZE_T` sipSegment
    This is the number of the segment of the write buffer.

PyObject \*sipSelf
    This is the Python object that wraps the structure or class instance, i.e.
    ``self``.


.. directive:: %BIReleaseBufferCode

.. parsed-literal::

    %BIReleaseBufferCode
        *code*
    %End

This directive (along with :directive:`%BIGetBufferCode`) is used to specify
code that implements the buffer interface of Python v3.  If Python v2 is being
used then this is ignored.

The following variables are made available to the handwritten code:

Py_buffer \*sipBuffer
    This is a pointer to the Python buffer structure.

*type* \*sipCpp
    This is a pointer to the structure or class instance.  Its *type* is a
    pointer to the structure or class.

PyObject \*sipSelf
    This is the Python object that wraps the structure or class instance, i.e.
    ``self``.


.. directive:: %CModule

.. deprecated:: 4.12
    Use the :directive:`%Module` directive with the ``language`` argument set
    to ``"C"`` instead.

.. parsed-literal::

    %CModule *name* [*version*]

This directive is used to identify that the library being wrapped is a C
library and to define the name of the module and it's optional version number.

See the :directive:`%Module` directive for an explanation of the version
number.

For example::

    %CModule dbus 1


.. directive:: %CompositeModule

.. parsed-literal::

    %CompositeModule(name = *dotted-name*)
    {
        [:directive:`%Docstring`]
    };

A composite module is one that merges a number of related SIP generated
modules.  For example, a module that merges the modules ``a_mod``, ``b_mod``
and ``c_mod`` is equivalent to the following pure Python module::

    from a_mod import *
    from b_mod import *
    from c_mod import *

Clearly the individual modules should not define module-level objects with the
same name.

This directive is used to specify the name of a composite module.  Any
subsequent :directive:`%Module` directive is interpreted as defining a
component module.

The optional :directive:`%Docstring` sub-directive is used to specify the
module's docstring.

For example::

    %CompositeModule PyQt4.Qt
    %Include QtCore/QtCoremod.sip
    %Include QtGui/QtGuimod.sip

The main purpose of a composite module is as a programmer convenience as they
don't have to remember which individual module an object is defined in.


.. directive:: %ConsolidatedModule

.. parsed-literal::

    %ConsolidatedModule(name = *dotted-name*)
    {
        [:directive:`%Docstring`]
    };

A consolidated module is one that consolidates the wrapper code of a number of
SIP generated modules (refered to as component modules in this context).

This directive is used to specify the name of a consolidated module.  Any
subsequent :directive:`%Module` directive is interpreted as defining a
component module.

The optional :directive:`%Docstring` sub-directive is used to specify the
module's docstring.

For example::

    %ConsolidatedModule PyQt4._qt
    %Include QtCore/QtCoremod.sip
    %Include QtGui/QtGuimod.sip

A consolidated module is not intended to be explicitly imported by an
application.  Instead it is imported by its component modules when they
themselves are imported.

Normally the wrapper code is contained in the component module and is linked
against the corresponding C or C++ library.  The advantage of a consolidated
module is that it allows all of the wrapped C or C++ libraries to be linked
against a single module.  If the linking is done statically then deployment of
generated modules can be greatly simplified.

It follows that a component module can be built in one of two ways, as a
normal standalone module, or as a component of a consolidated module.  When
building as a component the ``-p`` command line option should be used to
specify the name of the consolidated module.


.. directive:: %ConvertFromTypeCode

.. parsed-literal::

    %ConvertFromTypeCode
        *code*
    %End

This directive is used as part of the :directive:`%MappedType` directive (when
it is required) or of a class specification (when it is optional) to specify
the handwritten code that converts an instance of a C/C++ type to a Python
object.

If used as part of a class specification then instances of the class will be
automatically converted to the Python object, even though the class itself has
been wrapped.  This behaviour can be changed on a temporary basis from an
application by calling the :func:`sip.enableautoconversion` function, or from
handwritten code by calling the :c:func:`sipEnableAutoconversion` function.

The following variables are made available to the handwritten code:

*type* \*sipCpp
    This is a pointer to the C/C++ instance to be converted.  It will never be
    zero as the conversion from zero to ``Py_None`` is handled before the
    handwritten code is called.

PyObject \*sipTransferObj
    This specifies any desired ownership changes to the returned object.  If it
    is ``NULL`` then the ownership should be left unchanged.  If it is
    ``Py_None`` then ownership should be transferred to Python.  Otherwise
    ownership should be transferred to C/C++ and the returned object associated
    with *sipTransferObj*.  The code can choose to interpret these changes in
    any way.  For example, if the code is converting a C++ container of wrapped
    classes to a Python list it is likely that the ownership changes should be
    made to each element of the list.

The handwritten code must explicitly return a ``PyObject *``.  If there was an
error then a Python exception must be raised and ``NULL`` returned.

The following example converts a ``QList<QWidget *>`` instance to a Python
list of ``QWidget`` instances::

    %ConvertFromTypeCode
        PyObject *l;

        // Create the Python list of the correct length.
        if ((l = PyList_New(sipCpp->size())) == NULL)
            return NULL;

        // Go through each element in the C++ instance and convert it to a
        // wrapped QWidget.
        for (int i = 0; i < sipCpp->size(); ++i)
        {
            QWidget *w = sipCpp->at(i);
            PyObject *wobj;

            // Get the Python wrapper for the QWidget instance, creating a new
            // one if necessary, and handle any ownership transfer.
            if ((wobj = sipConvertFromType(w, sipType_QWidget, sipTransferObj)) == NULL)
            {
                // There was an error so garbage collect the Python list.
                Py_DECREF(l);
                return NULL;
            }

            // Add the wrapper to the list.
            PyList_SET_ITEM(l, i, wobj);
        }

        // Return the Python list.
        return l;
    %End


.. directive:: %ConvertToSubClassCode

.. parsed-literal::

    %ConvertToSubClassCode
        *code*
    %End

When SIP needs to wrap a C++ class instance it first checks to make sure it
hasn't already done so.  If it has then it just returns a new reference to the
corresponding Python object.  Otherwise it creates a new Python object of the
appropriate type.  In C++ a function may be defined to return an instance of a
certain class, but can often return a sub-class instead.

This directive is used to specify handwritten code that exploits any available
real-time type information (RTTI) to see if there is a more specific Python
type that can be used when wrapping the C++ instance.  The RTTI may be
provided by the compiler or by the C++ instance itself.

The directive is included in the specification of one of the classes that the
handwritten code handles the type conversion for.  It doesn't matter which
one, but a sensible choice would be the one at the root of that class
hierarchy in the module.

Note that if a class hierarchy extends over a number of modules then this
directive should be used in each of those modules to handle the part of the
hierarchy defined in that module.  SIP will ensure that the different pieces
of code are called in the right order to determine the most specific Python
type to use.

The following variables are made available to the handwritten code:

*type* \*sipCpp
    This is a pointer to the C++ class instance.

void \*\*sipCppRet
    When the sub-class is derived from more than one super-class then it is
    possible that the C++ address of the instance as the sub-class is
    different to that of the super-class.  If so, then this must be set to the
    C++ address of the instance when cast (usually using ``static_cast``)
    from the super-class to the sub-class.

const sipTypeDef \*sipType
    The handwritten code must set this to the SIP generated type structure
    that corresponds to the class instance.  (The type structure for class
    ``Klass`` is ``sipType_Klass``.)  If the RTTI of the class instance isn't
    recognised then ``sipType`` must be set to ``NULL``.  The code doesn't
    have to recognise the exact class, only the most specific sub-class that
    it can.

    The code may also set the value to a type that is apparently unrelated to
    the requested type.  If this happens then the whole conversion process is
    started again using the new type as the requested type.  This is typically
    used to deal with classes that have more than one super-class that are
    subject to this conversion process.  It allows the code for one super-class
    to switch to the code for another (more appropriate) super-class.

sipWrapperType \*sipClass
    .. deprecated:: 4.8
        Use ``sipType`` instead.

    The handwritten code must set this to the SIP generated Python type object
    that corresponds to the class instance.  (The type object for class
    ``Klass`` is ``sipClass_Klass``.)  If the RTTI of the class instance isn't
    recognised then ``sipClass`` must be set to ``NULL``.  The code doesn't
    have to recognise the exact class, only the most specific sub-class that
    it can.

The handwritten code must not explicitly return.

The following example shows the sub-class conversion code for ``QEvent`` based
class hierarchy in PyQt::

    class QEvent
    {
    %ConvertToSubClassCode
        // QEvent sub-classes provide a unique type ID.
        switch (sipCpp->type())
        {
        case QEvent::Timer:
            sipType = sipType_QTimerEvent;
            break;

        case QEvent::KeyPress:
        case QEvent::KeyRelease:
            sipType = sipType_QKeyEvent;
            break;

        // Skip the remaining event types to keep the example short.

        default:
            // We don't recognise the type.
            sipType = NULL;
        }
    %End

        // The rest of the class specification.

    };


.. directive:: %ConvertToTypeCode

.. parsed-literal::

    %ConvertToTypeCode
        *code*
    %End

This directive is used to specify the handwritten code that converts a Python
object to a mapped type instance and to handle any ownership transfers.  It is
used as part of the :directive:`%MappedType` directive and as part of a class
specification.  The code is also called to determine if the Python object is of
the correct type prior to conversion.

When used as part of a class specification it can automatically convert
additional types of Python object.  For example, PyQt uses it in the
specification of the ``QString`` class to allow Python string objects and
unicode objects to be used wherever ``QString`` instances are expected.

The following variables are made available to the handwritten code:

int \*sipIsErr
    If this is ``NULL`` then the code is being asked to check the type of the
    Python object.  The check must not have any side effects.  Otherwise the
    code is being asked to convert the Python object and a non-zero value
    should be returned through this pointer if an error occurred during the
    conversion.

PyObject \*sipPy
    This is the Python object to be converted.

*type* \*\*sipCppPtr
    This is a pointer through which the address of the mapped type instance (or
    zero if appropriate) is returned.  Its value is undefined if ``sipIsErr``
    is ``NULL``.

PyObject \*sipTransferObj
    This specifies any desired ownership changes to *sipPy*.  If it is ``NULL``
    then the ownership should be left unchanged.  If it is ``Py_None`` then
    ownership should be transferred to Python.  Otherwise ownership should be
    transferred to C/C++ and *sipPy* associated with *sipTransferObj*.  The
    code can choose to interpret these changes in any way.

The handwritten code must explicitly return an ``int`` the meaning of which
depends on the value of ``sipIsErr``.

If ``sipIsErr`` is ``NULL`` then a non-zero value is returned if the Python
object has a type that can be converted to the mapped type.  Otherwise zero is
returned.

If ``sipIsErr`` is not ``NULL`` then a combination of the following flags is
returned.

        - :c:macro:`SIP_TEMPORARY` is set to indicate that the returned
          instance is a temporary and should be released to avoid a memory
          leak.

        - :c:macro:`SIP_DERIVED_CLASS` is set to indicate that the type of the
          returned instance is a derived class.  See
          :ref:`ref-derived-classes`.

The following example converts a Python list of ``QPoint`` instances to a
``QList<QPoint>`` instance::

    %ConvertToTypeCode
        // See if we are just being asked to check the type of the Python
        // object.
        if (!sipIsErr)
        {
            // Checking whether or not None has been passed instead of a list
            // has already been done.
            if (!PyList_Check(sipPy))
                return 0;

            // Check the type of each element.  We specify SIP_NOT_NONE to
            // disallow None because it is a list of QPoint, not of a pointer
            // to a QPoint, so None isn't appropriate.
            for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
                if (!sipCanConvertToType(PyList_GET_ITEM(sipPy, i),
                                         sipType_QPoint, SIP_NOT_NONE))
                    return 0;

            // The type is valid.
            return 1;
        }

        // Create the instance on the heap.
        QList<QPoint> *ql = new QList<QPoint>;

        for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
        {
            QPoint *qp;
            int state;

            // Get the address of the element's C++ instance.  Note that, in
            // this case, we don't apply any ownership changes to the list
            // elements, only to the list itself.
            qp = reinterpret_cast<QPoint *>(sipConvertToType(
                                                    PyList_GET_ITEM(sipPy, i),
                                                    sipType_QPoint, 0,
                                                    SIP_NOT_NONE,
                                                    &state, sipIsErr));

            // Deal with any errors.
            if (*sipIsErr)
            {
                sipReleaseType(qp, sipType_QPoint, state);

                // Tidy up.
                delete ql;

                // There is no temporary instance.
                return 0;
            }

            ql->append(*qp);

            // A copy of the QPoint was appended to the list so we no longer
            // need it.  It may be a temporary instance that should be
            // destroyed, or a wrapped instance that should not be destroyed.
            // sipReleaseType() will do the right thing.
            sipReleaseType(qp, sipType_QPoint, state);
        }

        // Return the instance.
        *sipCppPtr = ql;

        // The instance should be regarded as temporary (and be destroyed as
        // soon as it has been used) unless it has been transferred from
        // Python.  sipGetState() is a convenience function that implements
        // this common transfer behaviour.
        return sipGetState(sipTransferObj);
    %End

When used in a class specification the handwritten code replaces the code that
would normally be automatically generated.  This means that the handwritten
code must also handle instances of the class itself and not just the additional
types that are being supported.  This should be done by making calls to
:c:func:`sipCanConvertToType()` to check the object type and
:c:func:`sipConvertToType()` to convert the object.  The
:c:macro:`SIP_NO_CONVERTORS` flag *must* be passed to both these functions to
prevent recursive calls to the handwritten code.


.. directive:: %Copying

.. parsed-literal::

    %Copying
        *text*
    %End

This directive is used to specify some arbitrary text that will be included at
the start of all source files generated by SIP.  It is normally used to include
copyright and licensing terms.

For example::

    %Copying
    Copyright (c) 2013 Riverbank Computing Limited
    %End


.. directive:: %DefaultDocstringFormat

.. versionadded:: 4.13

.. parsed-literal::

    %DefaultDocstringFormat(name = ["raw" | "deindented"])

This directive is used to specify the default formatting of docstrings, i.e.
when the :directive:`%Docstring` directive does not specify an explicit format.

See the :directive:`%Docstring` directive for an explanation of the different
formats.  If the directive is not specified then the default format used is
``"raw"``.

For example::

    %DefaultDocstringFormat "deindented"


.. directive:: %DefaultEncoding

.. parsed-literal::

    %DefaultEncoding(name = ["ASCII" | "Latin-1" | "UTF-8" | "None"])

This directive is used to specify the default encoding used for ``char``,
``const char``, ``char *`` or ``const char *`` values.  An encoding of
``"None"`` means that the value is unencoded.  The default can be overridden
for a particular value using the :aanno:`Encoding` annotation.
    
If the directive is not specified then the default encoding of the last
imported module is used, if any.

For example::

    %DefaultEncoding "Latin-1"


.. directive:: %DefaultMetatype

.. parsed-literal::

    %DefaultMetatype(name = *dotted-name*)

This directive is used to specify the Python type that should be used as the
meta-type for any C/C++ data type defined in the same module, and by importing
modules, that doesn't have an explicit meta-type.

If this is not specified then ``sip.wrappertype`` is used.

You can also use the :canno:`Metatype` class annotation to specify the
meta-type used by a particular C/C++ type.

See the section :ref:`ref-types-metatypes` for more details.

For example::

    %DefaultMetatype PyQt4.QtCore.pyqtWrapperType


.. directive:: %DefaultSupertype

.. parsed-literal::

    %DefaultSupertype(name = *dotted-name*)

This directive is used to specify the Python type that should be used as the
super-type for any C/C++ data type defined in the same module that doesn't have
an explicit super-type.

If this is not specified then ``sip.wrapper`` is used.

You can also use the :canno:`Supertype` class annotation to specify the
super-type used by a particular C/C++ type.

See the section :ref:`ref-types-metatypes` for more details.

For example::

    %DefaultSupertype sip.simplewrapper


.. directive:: %Doc

.. deprecated:: 4.12
    Use the :directive:`%Extract` directive instead.

.. parsed-literal::

    %Doc
        *text*
    %End

This directive is used to specify some arbitrary text that will be extracted
by SIP when the ``-d`` command line option is used.  The directive can be
specified any number of times and SIP will concatenate all the separate pieces
of text in the order that it sees them.

Documentation that is specified using this directive is local to the module in
which it appears.  It is ignored by modules that :directive:`%Import` it.  Use
the :directive:`%ExportedDoc` directive for documentation that should be
included by all modules that :directive:`%Import` this one.

For example::

    %Doc
    <h1>An Example</h1>
    <p>
    This fragment of documentation is HTML and is local to the module in
    which it is defined.
    </p>
    %End


.. directive:: %Docstring

.. versionadded:: 4.10

.. parsed-literal::

    %Docstring(format = ["raw" | "deindented"])
        *text*
    %End

This directive is used to specify explicit docstrings for modules, classes,
functions, methods and properties.

The docstring of a class is made up of the docstring specified for the class
itself, with the docstrings specified for each contructor appended.

The docstring of a function or method is made up of the concatenated docstrings
specified for each of the overloads.

Specifying an explicit docstring will prevent SIP from generating an automatic
docstring that describes the Python signature of a function or method overload.
This means that SIP will generate less informative exceptions (i.e. without a
full signature) when it fails to match a set of arguments to any function or
method overload.

.. versionadded:: 4.13

The format may either be ``"raw"`` or ``"deindented"``.  If it is not specified
then the value specified by any :directive:`%DefaultDocstringFormat` directive
is used.

If the format is ``"raw"`` then the docstring is used as it appears in the
specification file.

If the format is ``"deindented"`` then any leading spaces common to all
non-blank lines of the docstring are removed.

For example::

    class Klass
    {
    %Docstring
    This will be at the start of the class's docstring.
    %End

    public:
        Klass();
    %Docstring deindented
        This will be appended to the class's docstring and will not be indented.

            This will be indented by four spaces.
    %End
    };


.. directive:: %End

This isn't a directive in itself, but is used to terminate a number of
directives that allow a block of handwritten code or text to be specified.


.. directive:: %Exception

.. parsed-literal::

    %Exception *name* [(*base-exception*)]
    {
        [:directive:`%TypeHeaderCode`]
        :directive:`%RaiseCode`
    };

This directive is used to define new Python exceptions, or to provide a stub
for existing Python exceptions.  It allows handwritten code to be provided
that implements the translation between C++ exceptions and Python exceptions.
The arguments to ``throw ()`` specifiers must either be names of classes or the
names of Python exceptions defined by this directive.

*name* is the name of the exception.

*base-exception* is the optional base exception.  This may be either one of
the standard Python exceptions or one defined with a previous
:directive:`%Exception` directive.

The optional :directive:`%TypeHeaderCode` sub-directive is used to specify any
external interface to the exception being defined.

The :directive:`%RaiseCode` sub-directive is used to specify the handwritten
code that converts a reference to the C++ exception to the Python exception.

For example::

    %Exception std::exception(SIP_Exception) /PyName=StdException/
    {
    %TypeHeaderCode
    #include <exception>
    %End
    %RaiseCode
        const char *detail = sipExceptionRef.what();

        SIP_BLOCK_THREADS
        PyErr_SetString(sipException_std_exception, detail);
        SIP_UNBLOCK_THREADS
    %End
    };

In this example we map the standard C++ exception to a new Python exception.
The new exception is called ``StdException`` and is derived from the standard
Python exception ``Exception``.

An exception may be annotated with :xanno:`Default` to specify that it should
be caught by default if there is no ``throw`` clause.


.. directive:: %ExportedDoc

.. deprecated:: 4.12
    Use the :directive:`%Extract` directive instead.

.. parsed-literal::

    %ExportedDoc
        *text*
    %End

This directive is used to specify some arbitrary text that will be extracted
by SIP when the ``-d`` command line option is used.  The directive can be
specified any number of times and SIP will concatenate all the separate pieces
of text in the order that it sees them.

Documentation that is specified using this directive will also be included by
modules that :directive:`%Import` it.

For example::

    %ExportedDoc
    ==========
    An Example
    ==========
    
    This fragment of documentation is reStructuredText and will appear in the
    module in which it is defined and all modules that %Import it.
    %End


.. directive:: %ExportedHeaderCode

.. parsed-literal::

    %ExportedHeaderCode
        *code*
    %End

This directive is used to specify handwritten code, typically the declarations
of types, that is placed in a header file that is included by all generated
code for all modules.  It should not include function declarations because
Python modules should not explicitly call functions in another Python module.

.. seealso:: :directive:`%ModuleCode`, :directive:`%ModuleHeaderCode`


.. directive:: %Extract

.. versionadded:: 4.12

.. parsed-literal::

    %Extract(id = *name* [, order = *integer*])
        *text*
    %End

This directive is used to specify part of an extract.  An extract is a
collection of arbitrary text specified as one or more parts each having the
same ``id``.  SIP places no interpretation on an identifier, or on the
contents of the extract.  Extracts may be used for any purpose, e.g.
documentation, tests etc.

The part's optional ``order`` determines its position relative to the extract's
other parts.  If the order is not specified then the part is appended to the
extract.

An extract is written to a file using the :option:`-X <sip -X>` command line
option.

For example::

    %Extract example
    This will be the last line because there is no explicit order.
    %End

    %Extract(id=example, order=20)
    This will be the second line.
    %End

    %Extract(id=example, order=10)
    This will be the first line.
    %End


.. directive:: %Feature

.. parsed-literal::

    %Feature(name = *name*)

This directive is used to declare a feature.  Features (along with
:directive:`%Platforms` and :directive:`%Timeline`) are used by the
:directive:`%If` directive to control whether or not parts of a specification
are processed or ignored.

Features are mutually independent of each other - any combination of features
may be enabled or disable.  By default all features are enabled.  The
:option:`-x <sip -x>` command line option is used to disable a feature.

If a feature is enabled then SIP will automatically generate a corresponding C
preprocessor symbol for use by handwritten code.  The symbol is the name of
the feature prefixed by ``SIP_FEATURE_``.

For example::

    %Feature FOO_SUPPORT

    %If (FOO_SUPPORT)
    void foo();
    %End


.. directive:: %FinalisationCode

.. versionadded:: 4.15

.. parsed-literal::

    %FinalisationCode
        *code*
    %End

This directive is used to specify handwritten code that is executed one the
instance of a wrapped class has been created.  The handwritten code is passed a
dictionary of any remaining keyword arguments.  It must explicitly return an
integer result which should be ``0`` if there was no error.  If an error
occurred then ``-1`` should be returned and a Python exception raised.

The following variables are made available to the handwritten code:

PyObject \*sipSelf
    This is the Python object that wraps the structure or class instance, i.e.
    ``self``.

*type* \*sipCpp
    This is a pointer to the structure or class instance.  Its *type* is a
    pointer to the structure or class.

PyObject \*sipKwds
    This is an optional dictionary of unused keyword arguments.  It may be
    ``NULL`` or refer to an empty dictionary.  If the handwritten code handles
    any of the arguments then, if ``sipUnused`` is ``NULL``, those arguments
    must be removed from the dictionary.  If ``sipUnused`` is not ``NULL`` then
    the ``sipKwds`` dictionary must not be updated.  Instead a new dictionary
    must be created that contains any remaining unused keyword arguments and
    the address of the new dictionary returned via ``sipUnused``.  This rather
    complicated API ensures that new dictionaries are created only when
    necessary.

PyObject \*\*sipUnused
    This is an optional pointer to where the handwritten code should save the
    address of any new dictionary of unused keyword arguments that it creates.
    If it is ``NULL`` then the handwritten code is allowed to update the
    ``sipKwds`` dictionary.


.. directive:: %GCClearCode

.. parsed-literal::

    %GCClearCode
        *code*
    %End

Python has a cyclic garbage collector which can identify and release unneeded
objects even when their reference counts are not zero.  If a wrapped C
structure or C++ class keeps its own reference to a Python object then, if the
garbage collector is to do its job, it needs to provide some handwritten code
to traverse and potentially clear those embedded references.

See the section `Supporting Cyclic Garbage Collection
<http://docs.python.org/3/c-api/gcsupport.html>`__ in the Python documentation
for the details.

This directive is used to specify the code that clears any embedded references.
(See :directive:`%GCTraverseCode` for specifying the code that traverses any
embedded references.)

The following variables are made available to the handwritten code:

*type* \*sipCpp
    This is a pointer to the structure or class instance.  Its *type* is a
    pointer to the structure or class.

int sipRes
    The handwritten code should set this to the result to be returned.

The following simplified example is taken from PyQt.  The ``QCustomEvent``
class allows arbitary data to be attached to the event.  In PyQt this data is
always a Python object and so should be handled by the garbage collector::

    %GCClearCode
        PyObject *obj;

        // Get the object.
        obj = reinterpret_cast<PyObject *>(sipCpp->data());

        // Clear the pointer.
        sipCpp->setData(0);

        // Clear the reference.
        Py_XDECREF(obj);

        // Report no error.
        sipRes = 0;
    %End


.. directive:: %GCTraverseCode

.. parsed-literal::

    %GCTraverseCode
        *code*
    %End

This directive is used to specify the code that traverses any embedded
references for Python's cyclic garbage collector.  (See
:directive:`%GCClearCode` for a full explanation.)

The following variables are made available to the handwritten code:

*type* \*sipCpp
    This is a pointer to the structure or class instance.  Its *type* is a
    pointer to the structure or class.

visitproc sipVisit
    This is the visit function provided by the garbage collector.

void \*sipArg
    This is the argument to the visit function provided by the garbage
    collector.

int sipRes
    The handwritten code should set this to the result to be returned.

The following simplified example is taken from PyQt's ``QCustomEvent`` class::

    %GCTraverseCode
        PyObject *obj;

        // Get the object.
        obj = reinterpret_cast<PyObject *>(sipCpp->data());

        // Call the visit function if there was an object.
        if (obj)
            sipRes = sipVisit(obj, sipArg);
        else
            sipRes = 0;
    %End


.. directive:: %GetCode

.. parsed-literal::

    %GetCode
        *code*
    %End

This sub-directive is used in the declaration of a C++ class variable or C
structure member to specify handwritten code to convert it to a Python object.
It is usually used to handle types that SIP cannot deal with automatically.

The following variables are made available to the handwritten code:

*type* \*sipCpp
    This is a pointer to the structure or class instance.  Its *type* is a
    pointer to the structure or class.  It is not made available if the
    variable being wrapped is a static class variable.

PyObject \*sipPy
    The handwritten code must set this to the Python representation of the
    class variable or structure member.  If there is an error then the code
    must raise an exception and set this to ``NULL``.

PyObject \*sipPyType
    If the variable being wrapped is a static class variable then this is the
    Python type object of the class from which the variable was referenced
    (*not* the class in which it is defined).  It may be safely cast to a
    PyTypeObject \* or a sipWrapperType \*.

For example::

    struct Entity
    {
        /*
         * In this contrived example the C library we are wrapping actually
         * defines this as char buffer[100] which SIP cannot handle
         * automatically.
         */
        char *buffer
        {
            %GetCode
                sipPy = PyString_FromStringAndSize(sipCpp->buffer, 100);
            %End

            %SetCode
                char *ptr;
                int length;

                if (PyString_AsStringAndSize(sipPy, &ptr, &length) == -1)
                {
                    sipErr = 1;
                }
                else if (length != 100)
                {
                    /*
                     * Raise an exception because the length isn't exactly
                     * right.
                     */

                    PyErr_SetString(PyExc_ValueError,
                            "an Entity.buffer must be exactly 100 bytes");
                    sipErr = 1;
                }
                else
                {
                    memcpy(sipCpp->buffer, ptr, 100);
                }
            %End
        };
    }

.. seealso:: :directive:`%AccessCode`, :directive:`%SetCode`


.. directive:: %If

.. parsed-literal::

    %If (*expression*)
        *specification*
    %End

where

.. parsed-literal::

    *expression* ::= [*ored-qualifiers* | *range*]

    *ored-qualifiers* ::= [*qualifier* | *qualifier* **||** *ored-qualifiers*]

    *qualifier* ::= [**!**] [*feature* | *platform*]

    *range* ::= [*version*] **-** [*version*]

This directive is used in conjunction with features (see
:directive:`%Feature`), platforms (see :directive:`%Platforms`) and versions
(see :directive:`%Timeline`) to control whether or not parts of a specification
are processed or not.

A *range* of versions means all versions starting with the lower bound up to
but excluding the upper bound.  If the lower bound is omitted then it is
interpreted as being before the earliest version.  If the upper bound is
omitted then it is interpreted as being after the latest version.

For example::

    %Feature SUPPORT_FOO
    %Platforms {WIN32_PLATFORM POSIX_PLATFORM MACOS_PLATFORM}
    %Timeline {V1_0 V1_1 V2_0 V3_0}

    %If (!SUPPORT_FOO)
        // Process this if the SUPPORT_FOO feature is disabled.
    %End

    %If (POSIX_PLATFORM || MACOS_PLATFORM)
        // Process this if either the POSIX_PLATFORM or MACOS_PLATFORM
        // platforms are enabled.
    %End

    %If (V1_0 - V2_0)
        // Process this if either V1_0 or V1_1 is enabled.
    %End

    %If (V2_0 - )
        // Process this if either V2_0 or V3_0 is enabled.
    %End

    %If (SIP_4_13 - )
        // SIP v4.13 and later will process this.
    %End

    %If ( - )
        // Always process this.
    %End

Also note that the only way to specify the logical and of qualifiers is to use
nested :directive:`%If` directives.


.. directive:: %Import

.. parsed-literal::

    %Import(name = *filename*)

This directive is used to import the specification of another module.  This is
needed if the current module makes use of any types defined in the imported
module, e.g. as an argument to a function, or to sub-class.

If ``name`` cannot be opened then SIP prepends ``name`` with the name of the
directory containing the current specification file (i.e. the one containing
the :directive:`%Import` directive) and tries again.  If this also fails then
SIP prepends ``name`` with each of the directories, in turn, specified by the
:option:`-I <sip -I>` command line option.

Directory separators must always be ``/``.

For example::

    %Import qt/qtmod.sip


.. directive:: %Include

.. parsed-literal::

    %Include(name = *filename* [, optional = [True | False]])

This directive is used to include contents of another file as part of the
specification of the current module.  It is the equivalent of the C
preprocessor's ``#include`` directive and is used to structure a large module
specification into manageable pieces.

:directive:`%Include` follows the same search process as the
:directive:`%Import` directive when trying to open ``name``.

if ``optional`` is set then SIP will silently continue processing if the file
could not be opened.

Directory separators must always be ``/``.

For example::

    %Include qwidget.sip


.. directive:: %InitialisationCode

.. parsed-literal::

    %InitialisationCode
        *code*
    %End

This directive is used to specify handwritten code that is embedded in-line
in the generated module initialisation code after the SIP module has been
imported but before the module itself has been initialised.

It is typically used to call :c:func:`sipRegisterPyType()`.

For example::

    %InitialisationCode
        // The code will be executed when the module is first imported, after
        // the SIP module has been imported, but before other module-specific
        // initialisation has been completed.
    %End


.. directive:: %InstanceCode

.. versionadded:: 4.14

.. parsed-literal::

    %InstanceCode
        *code*
    %End

There are a number of circumstances where SIP needs to create an instance of a
C++ class but may not be able to do so.  For example the C++ class may be
abstract or may not have an argumentless public constructor.  This directive is
used in the definition of a class or mapped type to specify handwritten code to
create an instance of the C++ class.  For example, if the C++ class is
abstract, then the handwritten code may return an instance of a concrete
sub-class.

The following variable is made available to the handwritten code:

*type* \*sipCpp
    This must be set by the handwritten code to the address of an instance of
    the C++ class.  It doesn't matter if the instance is on the heap or not as
    it will never be explicitly destroyed.


.. directive:: %License

.. parsed-literal::

    %License(type = *string*
            [, licensee = *string*]
            [, signature = *string*]
            [, timestamp = *string*])

This directive is used to specify the contents of an optional license
dictionary.  The license dictionary is called :data:`__license__` and is stored
in the module dictionary.

``type`` is the type of the license and its value in the license dictionary is
accessed using the ``"Type"`` key.  No restrictions are placed on the value.

``licensee`` is the optional name of the licensee and its value in the license
dictionary is accessed using the ``"Licensee"`` key.  No restrictions are
placed on the value.

``signature`` is the license's optional signature and its value in the license
dictionary is accessed using the ``"Signature"`` key.  No restrictions are
placed on the value.

``timestamp`` is the license's optional timestamp and its value in the license
dictionary is accessed using the ``"Timestamp"`` key.  No restrictions are
placed on the value.

Note that this directive isn't an attempt to impose any licensing restrictions
on a module.  It is simply a method for easily embedding licensing information
in a module so that it is accessible to Python scripts.

For example::

    %License "GPL"


.. directive:: %MappedType

.. parsed-literal::

    template<*type-list*>
    %MappedType *type*
    {
        [:directive:`%TypeHeaderCode`]
        [:directive:`%ConvertToTypeCode`]
        [:directive:`%ConvertFromTypeCode`]
    };

    %MappedType *type*
    {
        [:directive:`%TypeHeaderCode`]
        [:directive:`%ConvertToTypeCode`]
        [:directive:`%ConvertFromTypeCode`]
    };

This directive is used to define an automatic mapping between a C or C++ type
and a Python type.  It can be used as part of a template, or to map a specific
type.

When used as part of a template *type* cannot itself refer to a template.  Any
occurrences of any of the type names (but not any ``*`` or ``&``) in
*type-list* will be replaced by the actual type names used when the template is
instantiated.  Template mapped types are instantiated automatically as required
(unlike template classes which are only instantiated using ``typedef``).

Any explicit mapped type will be used in preference to any template that maps
the same type, ie. a template will not be automatically instantiated if there
is an explicit mapped type.

The optional :directive:`%TypeHeaderCode` sub-directive is used to specify the
library interface to the type being mapped.

The optional :directive:`%ConvertToTypeCode` sub-directive is used to specify
the handwritten code that converts a Python object to an instance of the mapped
type.

The optional :directive:`%ConvertFromTypeCode` sub-directive is used to specify
the handwritten code that converts an instance of the mapped type to a Python
object.

For example::

    template<Type *>
    %MappedType QList
    {
    %TypeHeaderCode
    // Include the library interface to the type being mapped.
    #include <qlist.h>
    %End

    %ConvertToTypeCode
        // See if we are just being asked to check the type of the Python
        // object.
        if (sipIsErr == NULL)
        {
            // Check it is a list.
            if (!PyList_Check(sipPy))
                return 0;

            // Now check each element of the list is of the type we expect.
            // The template is for a pointer type so we don't disallow None.
            for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
                if (!sipCanConvertToType(PyList_GET_ITEM(sipPy, i),
                                         sipType_Type, 0))
                    return 0;

            return 1;
        }

        // Create the instance on the heap.
        QList<Type *> *ql = new QList<Type *>;

        for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
        {
            // Use the SIP API to convert the Python object to the
            // corresponding C++ instance.  Note that we apply any ownership
            // transfer to the list itself, not the individual elements.
            Type *t = reinterpret_cast<Type *>(sipConvertToType(
                                                PyList_GET_ITEM(sipPy, i),
                                                sipType_Type, 0, 0, 0,
                                                sipIsErr));

            if (*sipIsErr)
            {
                // Tidy up.
                delete ql;

                // There is nothing on the heap.
                return 0;
            }

            // Add the pointer to the C++ instance.
            ql->append(t);
        }

        // Return the instance on the heap.
        *sipCppPtr = ql;

        // Apply the normal transfer.
        return sipGetState(sipTransferObj);
    %End

    %ConvertFromTypeCode
        PyObject *l;

        // Create the Python list of the correct length.
        if ((l = PyList_New(sipCpp->size())) == NULL)
            return NULL;

        // Go through each element in the C++ instance and convert it to the
        // corresponding Python object.
        for (int i = 0; i < sipCpp->size(); ++i)
        {
            Type *t = sipCpp->at(i);
            PyObject *tobj;

            if ((tobj = sipConvertFromType(t, sipType_Type, sipTransferObj)) == NULL)
            {
                // There was an error so garbage collect the Python list.
                Py_DECREF(l);
                return NULL;
            }

            PyList_SET_ITEM(l, i, tobj);
        }

        // Return the Python list.
        return l;
    %End
    };

Using this we can use, for example, ``QList<QObject *>`` throughout the
module's specification files (and in any module that imports this one).  The
generated code will automatically map this to and from a Python list of QObject
instances when appropriate.


.. directive:: %MethodCode

.. parsed-literal::

    %MethodCode
        *code*
    %End

This directive is used as part of the specification of a global function, class
method, operator, constructor or destructor to specify handwritten code that
replaces the normally generated call to the function being wrapped.  It is
usually used to handle argument types and results that SIP cannot deal with
automatically.

Normally the specified code is embedded in-line after the function's arguments
have been successfully converted from Python objects to their C or C++
equivalents.  In this case the specified code must not include any ``return``
statements.

However if the :fanno:`NoArgParser` annotation has been used then the specified
code is also responsible for parsing the arguments.  No other code is generated
by SIP and the specified code must include a ``return`` statement.

In the context of a destructor the specified code is embedded in-line in the
Python type's deallocation function.  Unlike other contexts it supplements
rather than replaces the normally generated code, so it must not include code
to return the C structure or C++ class instance to the heap.  The code is only
called if ownership of the structure or class is with Python.

The specified code must also handle the Python Global Interpreter Lock (GIL).
If compatibility with SIP v3.x is required then the GIL must be released
immediately before the C++ call and reacquired immediately afterwards as shown
in this example fragment::

    Py_BEGIN_ALLOW_THREADS
    sipCpp->foo();
    Py_END_ALLOW_THREADS

If compatibility with SIP v3.x is not required then this is optional but
should be done if the C++ function might block the current thread or take a
significant amount of time to execute.  (See :ref:`ref-gil` and the
:fanno:`ReleaseGIL` and :fanno:`HoldGIL` annotations.)

If the :fanno:`NoArgParser` annotation has not been used then the following
variables are made available to the handwritten code:

*type* a0
    There is a variable for each argument of the Python signature (excluding
    any ``self`` argument) named ``a0``, ``a1``, etc.  If
    ``use_argument_names`` has been set in the :directive:`%Module` directive
    then the name of the argument is the real name.  The *type* of the variable
    is the same as the type defined in the specification with the following
    exceptions:

    - if the argument is only used to return a value (e.g. it is an ``int *``
      without an :aanno:`In` annotation) then the type has one less level of
      indirection (e.g. it will be an ``int``)
    - if the argument is a structure or class (or a reference or a pointer to a
      structure or class) then *type* will always be a pointer to the structure
      or class.

    Note that handwritten code for destructors never has any arguments.

PyObject \*a0Wrapper
    This variable is made available only if the :aanno:`GetWrapper` annotation
    is specified for the corresponding argument.  The variable is a pointer to
    the Python object that wraps the argument.

    If ``use_argument_names`` has been set in the :directive:`%Module`
    directive then the name of the variable is the real name of the argument
    with ``Wrapper`` appended.

*type* \*sipCpp
    If the directive is used in the context of a class constructor then this
    must be set by the handwritten code to the constructed instance.  If it is
    set to ``0`` and no Python exception is raised then SIP will continue to
    try other Python signatures.
    
    If the directive is used in the context of a method (but not the standard
    binary operator methods, e.g. :meth:`__add__`) or a destructor then this is
    a pointer to the C structure or C++ class instance.
    
    Its *type* is a pointer to the structure or class.
    
    Standard binary operator methods follow the same convention as global
    functions and instead define two arguments called ``a0`` and ``a1``.

sipErrorState sipError
    The handwritten code should set this to either ``sipErrorContinue`` or
    ``sipErrorFail``, and raise an appropriate Python exception, if an error
    is detected.  Its initial value will be ``sipErrorNone``.

    When ``sipErrorContinue`` is used, SIP will remember the exception as the
    reason why the particular overloaded callable could not be invoked.  It
    will then continue to try the next overloaded callable.  It is typically
    used by code that needs to do additional type checking of the callable's
    arguments.

    When ``sipErrorFail`` is used, SIP will report the exception immediately
    and will not attempt to invoke other overloaded callables.

    ``sipError`` is not provided for destructors.

int sipIsErr
    The handwritten code should set this to a non-zero value, and raise an
    appropriate Python exception, if an error is detected.  This is the
    equivalent of setting ``sipError`` to ``sipErrorFail``.  Its initial value
    will be ``0``.

    ``sipIsErr`` is not provided for destructors.

*type* sipRes
    The handwritten code should set this to the result to be returned.  The
    *type* of the variable is the same as the type defined in the Python
    signature in the specification with the following exception:

    - if the argument is a structure or class (or a reference or a pointer to a
      structure or class) then *type* will always be a pointer to the structure
      or class.

    ``sipRes`` is not provided for inplace operators (e.g. ``+=`` or
    :meth:`__imul__`) as their results are handled automatically, nor for class
    constructors or destructors.

PyObject \*sipSelf
    If the directive is used in the context of a class constructor, destructor
    or method then this is the Python object that wraps the structure or class
    instance, i.e. ``self``.

bool sipSelfWasArg
    This is only made available for non-abstract, virtual methods.  It is set
    if ``self`` was explicitly passed as the first argument of the method
    rather than being bound to the method.  In other words, the call was::

        Klass.foo(self, ...)

    rather than::

        self.foo(...)

If the :fanno:`NoArgParser` annotation has been used then only the following
variables are made available to the handwritten code:

PyObject \*sipArgs
    This is the tuple of arguments.

PyObject \*sipKwds
    This is the dictionary of keyword arguments.

The following is a complete example::

    class Klass
    {
    public:
        virtual int foo(SIP_PYTUPLE);
    %MethodCode
            // The C++ API takes a 2 element array of integers but passing a
            // two element tuple is more Pythonic.

            int iarr[2];

            if (PyArg_ParseTuple(a0, "ii", &iarr[0], &iarr[1]))
            {
                Py_BEGIN_ALLOW_THREADS
                sipRes = sipSelfWasArg ? sipCpp->Klass::foo(iarr)
                                       : sipCpp->foo(iarr);
                Py_END_ALLOW_THREADS
            }
            else
            {
                // PyArg_ParseTuple() will have raised the exception.
                sipIsErr = 1;
            }
    %End
    };

As the example is a virtual method [#]_, note the use of ``sipSelfWasArg`` to
determine exactly which implementation of ``foo()`` to call.

If a method is in the ``protected`` section of a C++ class then SIP generates
helpers that provide access to method.  However, these are not available if
the Python module is being built with ``protected`` redefined as ``public``.

The following pattern should be used to cover all possibilities::

    #if defined(SIP_PROTECTED_IS_PUBLIC)
        sipRes = sipSelfWasArg ? sipCpp->Klass::foo(iarr)
                               : sipCpp->foo(iarr);
    #else
        sipRes = sipCpp->sipProtectVirt_foo(sipSelfWasArg, iarr);
    #endif

If a method is in the ``protected`` section of a C++ class but is not virtual
then the pattern should instead be::

    #if defined(SIP_PROTECTED_IS_PUBLIC)
        sipRes = sipCpp->foo(iarr);
    #else
        sipRes = sipCpp->sipProtect_foo(iarr);
    #endif

.. [#] See :directive:`%VirtualCatcherCode` for a description of how SIP
       generated code handles the reimplementation of C++ virtual methods in
       Python.


.. directive:: %Module

.. parsed-literal::

    %Module(name = *dotted-name*
            [, all_raise_py_exception = [True | False]]
            [, call_super_init = [True | False]]
            [, default_VirtualErrorHandler = *name*]
            [, keyword_arguments = ["None" | "All" | "Optional"]]
            [, language = *string*]
            [, use_argument_names = [True | False]]
            [, version = *integer*])
    {
        [:directive:`%AutoPyName`]
        [:directive:`%Docstring`]
    };

This directive is used to specify the name of a module and a number of other
attributes.  ``name`` may contain periods to specify that the module is part of
a Python package.

``all_raise_py_exception`` specifies that all constructors, functions and
methods defined in the module raise a Python exception to indicate that an
error occurred.  It is the equivalent of using the :fanno:`RaisesPyException`
function annotation on every constructor, function and method.

``call_super_init`` specifies that the ``__init__()`` method of a wrapped class
should automatically call it's super-class's ``__init__()`` method passing a
dictionary of any unused keyword arguments.  In other words, wrapped classes
support cooperative multi-inheritance.  This means that sub-classes, and any
mixin classes, should always use call ``super().__init__()`` and not call any
super-class's ``__init__()`` method explicitly.

``default_VirtualErrorHandler`` specifies the handler (defined by the
:directive:`%VirtualErrorHandler` directive) that is called when a Python
re-implementation of any virtual C++ function raises a Python exception.  If no
handler is specified for a virtual C++ function then ``PyErr_Print()`` is
called.

``keyword_arguments`` specifies the default level of support for Python keyword
arguments.  See the :fanno:`KeywordArgs` annotation for an explaination of the
possible values and their effect.  If it is not specified then the value
implied by the (deprecated) :option:`-k <sip -k>` command line option is used.

``language`` specifies the implementation language of the library being
wrapped.  Its value is either ``"C++"`` (the default) or ``"C"``.

When providing handwritten code as part of either the :directive:`%MethodCode`
or :directive:`%VirtualCatcherCode` directives the names of the arguments of
the function or method are based on the number of the argument, i.e. the first
argument is named ``a0``, the second ``a1`` and so on.  ``use_argument_names``
is set to specify that the real name of the argument, if any, should be used
instead.  It also affects the name of the variable created when the
:aanno:`GetWrapper` argument annotation is used.

``version`` is an optional version number that is useful if you (or others)
might create other modules that build on this module, i.e. if another module
might :directive:`%Import` this module.  Under the covers, a module exports an
API that is used by modules that :directive:`%Import` it and the API is given a
version number.  A module built on that module knows the version number of the
API that it is expecting.  If, when the modules are imported at run-time, the
version numbers do not match then a Python exception is raised.  The dependent
module must then be re-built using the correct specification files for the base
module.

The optional :directive:`%AutoPyName` sub-directive is used to specify a rule
for automatically providing Python names.

The optional :directive:`%Docstring` sub-directive is used to specify the
module's docstring.

For example::

    %Module(name=PyQt4.QtCore, version=5)


.. directive:: %ModuleCode

.. parsed-literal::

    %ModuleCode
        *code*
    %End

This directive is used to specify handwritten code, typically the
implementations of utility functions, that can be called by other handwritten
code in the module.

For example::

    %ModuleCode
    // Print an object on stderr for debugging purposes.
    void dump_object(PyObject *o)
    {
        PyObject_Print(o, stderr, 0);
        fprintf(stderr, "\n");
    }
    %End

.. seealso:: :directive:`%ExportedHeaderCode`, :directive:`%ModuleHeaderCode`


.. directive:: %ModuleHeaderCode

.. parsed-literal::

    %ModuleHeaderCode
        *code*
    %End

This directive is used to specify handwritten code, typically the declarations
of utility functions, that is placed in a header file that is included by all
generated code for the same module.

For example::

    %ModuleHeaderCode
    void dump_object(PyObject *o);
    %End

.. seealso:: :directive:`%ExportedHeaderCode`, :directive:`%ModuleCode`


.. directive:: %OptionalInclude

.. parsed-literal::

    %OptionalInclude *filename*

.. deprecated:: 4.12
    Use the :directive:`%Include` directive with the ``optional`` argument set
    to ``True`` instead.

This directive is identical to the :directive:`%Include` directive except that
SIP silently continues processing if *filename* could not be opened.

For example::

    %OptionalInclude license.sip


.. directive:: %PickleCode

.. parsed-literal::

    %PickleCode
        *code*
    %End

This directive is used to specify handwritten code to pickle a C structure or
C++ class instance.

The following variables are made available to the handwritten code:

*type* \*sipCpp
    This is a pointer to the structure or class instance.  Its *type* is a
    pointer to the structure or class.

PyObject \*sipRes
    The handwritten code must set this to a tuple of the arguments that will
    be passed to the type's ``__init__()`` method when the structure or class
    instance is unpickled.  If there is an error then the code must raise an
    exception and set this to ``NULL``.

For example::

    class Point
    {
        Point(int x, y);

        int x() const;
        int y() const;

    %PickleCode
        sipRes = Py_BuildValue("ii", sipCpp->x(), sipCpp->y());
    %End
    }

Note that SIP works around the Python limitation that prevents nested types
being pickled.

Both named and unnamed enums can be pickled automatically without providing any
handwritten code.


.. directive:: %Platforms

.. parsed-literal::

    %Platforms {*name* *name* ...}

This directive is used to declare a set of platforms.  Platforms (along with
:directive:`%Feature` and :directive:`%Timeline`) are used by the
:directive:`%If` directive to control whether or not parts of a specification
are processed or ignored.

Platforms are mutually exclusive - only one platform can be enabled at a time.
By default all platforms are disabled.  The SIP :option:`-t <sip -t>` command
line option is used to enable a platform.

.. versionadded:: 4.14

If a platform is enabled then SIP will automatically generate a corresponding C
preprocessor symbol for use by handwritten code.  The symbol is the name of
the platform prefixed by ``SIP_PLATFORM_``.

For example::

    %Platforms {WIN32_PLATFORM POSIX_PLATFORM MACOS_PLATFORM}

    %If (WIN32_PLATFORM)
    void undocumented();
    %End

    %If (POSIX_PLATFORM)
    void documented();
    %End


.. directive:: %PostInitialisationCode

.. parsed-literal::

    %PostInitialisationCode
        *code*
    %End

This directive is used to specify handwritten code that is embedded in-line
at the very end of the generated module initialisation code.

The following variables are made available to the handwritten code:

PyObject \*sipModule
    This is the module object returned by ``Py_InitModule()``.

PyObject \*sipModuleDict
    This is the module's dictionary object returned by ``Py_ModuleGetDict()``.

For example::

    %PostInitialisationCode
        // The code will be executed when the module is first imported and
        // after all other initialisation has been completed.
    %End


.. directive:: %PreInitialisationCode

.. parsed-literal::

    %PreInitialisationCode
        *code*
    %End

This directive is used to specify handwritten code that is embedded in-line
at the very start of the generated module initialisation code.

For example::

    %PreInitialisationCode
        // The code will be executed when the module is first imported and
        // before other initialisation has been completed.
    %End


.. directive:: %Property

.. versionadded:: 4.12

.. parsed-literal::

    %Property(name = *name*, get = *name* [, set = *name*])
    {
        [:directive:`%Docstring`]
    };

This directive is used to define a Python property.  ``name`` is the name of
the property.

``get`` is the Python name of the getter method and must refer to a method in
the same class.

``set`` is the Python name of the optional setter method and must refer to a
method in the same class.

The optional :directive:`%Docstring` sub-directive is used to specify the
property's docstring.

For example::

    class Klass
    {
    public:
        int get_count() const;
        void set_count();

        %Property(name=count, get=get_count, set=set_count)
    };


.. directive:: %RaiseCode

.. parsed-literal::

    %RaiseCode
        *code*
    %End

This directive is used as part of the definition of an exception using the
:directive:`%Exception` directive to specify handwritten code that raises a
Python exception when a C++ exception has been caught.  The code is embedded
in-line as the body of a C++ ``catch ()`` clause.

The specified code must handle the Python Global Interpreter Lock (GIL) if
necessary.  The GIL must be acquired before any calls to the Python API and
released after the last call as shown in this example fragment::

    SIP_BLOCK_THREADS
    PyErr_SetNone(PyErr_Exception);
    SIP_UNBLOCK_THREADS

Finally, the specified code must not include any ``return`` statements.

The following variable is made available to the handwritten code:

*type* &sipExceptionRef
    This is a reference to the caught C++ exception.  The *type* of the
    reference is the same as the type defined in the ``throw ()`` specifier.

See the :directive:`%Exception` directive for an example.


.. directive:: %SetCode

.. parsed-literal::

    %SetCode
        *code*
    %End

This sub-directive is used in the declaration of a C++ class variable or C
structure member to specify handwritten code to convert it from a Python
object.  It is usually used to handle types that SIP cannot deal with
automatically.

The following variables are made available to the handwritten code:

*type* \*sipCpp
    This is a pointer to the structure or class instance.  Its *type* is a
    pointer to the structure or class.  It is not made available if the
    variable being wrapped is a static class variable.

int sipErr
    If the conversion failed then the handwritten code should raise a Python
    exception and set this to a non-zero value.  Its initial value will be
    automatically set to zero.

PyObject \*sipPy
    This is the Python object that the handwritten code should convert.

PyObject \*sipPyType
    If the variable being wrapped is a static class variable then this is the
    Python type object of the class from which the variable was referenced
    (*not* the class in which it is defined).  It may be safely cast to a
    PyTypeObject \* or a sipWrapperType \*.

.. seealso:: :directive:`%AccessCode`, :directive:`%GetCode`


.. directive:: %Timeline

.. parsed-literal::

    %Timeline {*name* *name* ...}

This directive is used to declare a set of versions released over a period of
time.  Versions (along with :directive:`%Feature` and :directive:`%Platforms`)
are used by the :directive:`%If` directive to control whether or not parts of a
specification are processed or ignored.

Versions are mutually exclusive - only one version can be enabled at a time.
By default all versions are disabled.  The SIP :option:`-t <sip -t>` command
line option is used to enable a version.

The :directive:`%Timeline` directive can be used any number of times in a
module to allow multiple libraries to be wrapped in the same module.

.. versionadded:: 4.12

SIP automatically defines a timeline containing all versions of SIP since
v4.12.  The name of the version is ``SIP_`` followed by the individual parts of
the version number separated by an underscore.  SIP v4.12 is therefore
``SIP_4_12`` and SIP v4.13.2 is ``SIP_4_13_2``.

.. versionadded:: 4.14

If a particular version is enabled then SIP will automatically generate a
corresponding C preprocessor symbol for use by handwritten code.  The symbol is
the name of the version prefixed by ``SIP_TIMELINE_``.

For example::

    %Timeline {V1_0 V1_1 V2_0 V3_0}

    %If (V1_0 - V2_0)
    void foo();
    %End

    %If (V2_0 -)
    void foo(int = 0);
    %End

    %If (- SIP_4_13)
    void bar();
    %End


.. directive:: %TypeCode

.. parsed-literal::

    %TypeCode
        *code*
    %End

This directive is used as part of the specification of a C structure, a C++
class or a :directive:`%MappedType` directive to specify handwritten code,
typically the implementations of utility functions, that can be called by other
handwritten code in the structure or class.

For example::

    class Klass
    {
    %TypeCode
    // Print an instance on stderr for debugging purposes.
    static void dump_klass(const Klass *k)
    {
        fprintf(stderr,"Klass %s at %p\n", k->name(), k);
    }
    %End

        // The rest of the class specification.

    };

Because the scope of the code is normally within the generated file that
implements the type, any utility functions would normally be declared
``static``.  However a naming convention should still be adopted to prevent
clashes of function names within a module in case the SIP ``-j`` command line
option is used.


.. directive:: %TypeHeaderCode

.. parsed-literal::

    %TypeHeaderCode
        *code*
    %End

This directive is used to specify handwritten code that defines the interface
to a C or C++ type being wrapped, either a structure, a class, or a template.
It is used within a class definition or a :directive:`%MappedType` directive.

Normally *code* will be a pre-processor ``#include`` statement.

For example::

    // Wrap the Klass class.
    class Klass
    {
    %TypeHeaderCode
    #include <klass.h>
    %End

        // The rest of the class specification.
    };


.. directive:: %UnitCode

.. parsed-literal::

    %UnitCode
        *code*
    %End

This directive is used to specify handwritten code that is included at the very
start of a generated compilation unit (ie. C or C++ source file).  It is
typically used to ``#include`` a C++ precompiled header file.


.. directive:: %UnitPostIncludeCode

.. versionadded:: 4.11

.. parsed-literal::

    %UnitPostIncludeCode
        *code*
    %End

This directive is used to specify handwritten code that is included following
the ``#include`` of all header files in a generated compilation unit (ie. C or
C++ source file).


.. directive:: %VirtualCatcherCode

.. parsed-literal::

    %VirtualCatcherCode
        *code*
    %End

For most classes there are corresponding :ref:`generated derived classes
<ref-derived-classes>` that contain reimplementations of the class's virtual
methods.  These methods (which SIP calls catchers) determine if there is a
corresponding Python reimplementation and call it if so.  If there is no Python
reimplementation then the method in the original class is called instead.

This directive is used to specify handwritten code that replaces the normally
generated call to the Python reimplementation and the handling of any returned
results.  It is usually used to handle argument types and results that SIP
cannot deal with automatically.

This directive can also be used in the context of a class destructor to
specify handwritten code that is embedded in-line in the internal derived
class's destructor.

In the context of a method the Python Global Interpreter Lock (GIL) is
automatically acquired before the specified code is executed and automatically
released afterwards.

In the context of a destructor the specified code must handle the GIL.  The
GIL must be acquired before any calls to the Python API and released after the
last call as shown in this example fragment::

    SIP_BLOCK_THREADS
    Py_DECREF(obj);
    SIP_UNBLOCK_THREADS

The following variables are made available to the handwritten code in the
context of a method:

*type* a0
    There is a variable for each argument of the C++ signature named ``a0``,
    ``a1``, etc.  If ``use_argument_names`` has been set in the
    :directive:`%Module` directive then the name of the argument is the real
    name.  The *type* of the variable is the same as the type defined in the
    specification.

int a0Key
    There is a variable for each argument of the C++ signature that has a type
    where it is important to ensure that the corresponding Python object is not
    garbage collected too soon.  This only applies to output arguments that
    return ``'\0'`` terminated strings.  The variable would normally be passed
    to :c:func:`sipParseResult()` using either the ``A`` or ``B`` format
    characters.

    If ``use_argument_names`` has been set in the :directive:`%Module`
    directive then the name of the variable is the real name of the argument
    with ``Key`` appended.

int sipIsErr
    The handwritten code should set this to a non-zero value, and raise an
    appropriate Python exception, if an error is detected.

PyObject \*sipMethod
    This object is the Python reimplementation of the virtual C++ method.  It
    is normally passed to :c:func:`sipCallMethod()`.

*type* sipRes
    The handwritten code should set this to the result to be returned.  The
    *type* of the variable is the same as the type defined in the C++ signature
    in the specification.

int sipResKey
    This variable is only made available if the result has a type where it is
    important to ensure that the corresponding Python object is not garbage
    collected too soon.  This only applies to ``'\0'`` terminated strings.  The
    variable would normally be passed to :c:func:`sipParseResult()` using
    either the ``A`` or ``B`` format characters.

sipSimpleWrapper \*sipPySelf
    This variable is only made available if either the ``a0Key`` or
    ``sipResKey`` are made available.  It defines the context within which keys
    are unique.  The variable would normally be passed to
    :c:func:`sipParseResult()` using the ``S`` format character.

No variables are made available in the context of a destructor.

For example::

    class Klass
    {
    public:
        virtual int foo(SIP_PYTUPLE) [int (int *)];
    %MethodCode
            // The C++ API takes a 2 element array of integers but passing a
            // two element tuple is more Pythonic.

            int iarr[2];

            if (PyArg_ParseTuple(a0, "ii", &iarr[0], &iarr[1]))
            {
                Py_BEGIN_ALLOW_THREADS
                sipRes = sipCpp->Klass::foo(iarr);
                Py_END_ALLOW_THREADS
            }
            else
            {
                // PyArg_ParseTuple() will have raised the exception.
                sipIsErr = 1;
            }
    %End
    %VirtualCatcherCode
            // Convert the 2 element array of integers to the two element
            // tuple.

            PyObject *result;

            result = sipCallMethod(&sipIsErr, sipMethod, "ii", a0[0], a0[1]);

            if (result != NULL)
            {
                // Convert the result to the C++ type.
                sipParseResult(&sipIsErr, sipMethod, result, "i", &sipRes);

                Py_DECREF(result);
            }
    %End
    };


.. directive:: %VirtualErrorHandler

.. versionadded:: 4.14

.. parsed-literal::

    %VirtualErrorHandler(name = *name*)
        *code*
    %End

This directive is used to define the handwritten code that implements a handler
that is called when a Python re-implementation of a virtual C++ function raises
a Python exception.  If a virtual C++ function does not have a handler the
``PyErr_Print()`` function is called.

The handler is called after all tidying up has been completed, with the Python
Global Interpreter Lock (GIL) held and from the thread that raised the
exception.  If the handler wants to change the execution path by, for example,
throwing a C++ exception, it must first release the GIL by calling
:c:func:`SIP_RELEASE_GIL`.  It must not call :c:func:`SIP_RELEASE_GIL` if the
execution path is not changed.

The following variables are made available to the handwritten code:

sipSimpleWrapper \*sipPySelf
    This is the class instance containing the Python reimplementation.

sip_gilstate_t sipGILState
    This is an opaque value that must be passed to :c:func:`SIP_RELEASE_GIL` in
    order to release the GIL prior to changing the execution path.

For example::

    %VirtualErrorHandler my_handler
        PyObject *exception, *value, *traceback;

        PyErr_Fetch(&exception, &value, &traceback);

        SIP_RELEASE_GIL(sipGILState);

        throw my_exception(sipPySelf, exception, value, traceback);
    %End

.. seealso:: :fanno:`NoVirtualErrorHandler`, :fanno:`VirtualErrorHandler`, :canno:`VirtualErrorHandler`
