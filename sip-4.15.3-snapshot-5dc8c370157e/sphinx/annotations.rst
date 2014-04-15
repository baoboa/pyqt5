Annotations
===========

In this section we describe each of the annotations that can be used in
specification files.

Annotations can either be :ref:`argument annotations <ref-arg-annos>`,
:ref:`class annotations <ref-class-annos>`, :ref:`mapped type annotations
<ref-mapped-type-annos>`, :ref:`enum annotations <ref-enum-annos>`,
:ref:`exception annotations <ref-exception-annos>`, :ref:`function annotations
<ref-function-annos>`, :ref:`typedef annotations <ref-typedef-annos>` or
:ref:`variable annotations <ref-variable-annos>` depending on the context in
which they can be used.

Annotations are placed between forward slashes (``/``).  Multiple annotations
are comma separated within the slashes.

Annotations have a type and, possibly, a value.  The type determines the
format of the value.  The name of an annotation and its value are separated by
``=``.

Annotations can have one of the following types:

*boolean*
    This type of annotation has no value and is implicitly true.

*integer*
    This type of annotation is an integer.  In some cases the value is
    optional.

*name*
    The value is a name that is compatible with a C/C++ identifier.  In some
    cases the value is optional.

*dotted name*
    The value is a name that is compatible with an identifier preceded by a
    Python scope.

*string*
    The value is a double quoted string.

*API range*
    The value is the name of an API (defined using the :directive:`%API`
    directive) separated by a range of version numbers with a colon.

    The range of version numbers is a pair of numbers separated by a hyphen
    specifying the lower and upper bounds of the range.  A version number is
    within the range if it is greater or equal to the lower bound and less
    than the upper bound.  Each bound can be omitted meaning that the range is
    unbounded in that direction.

    For example::

        # This is part of the PyQt4 API up to but excluding v2.
        void hex() /API=PyQt4:-2/

        # This is part of the PyQt4 API starting from v2.
        void hex() /PyName=hex_, API=PyQt4:2-/

The following example shows argument and function annotations::

    void exec(QWidget * /Transfer/) /ReleaseGIL, PyName=call_exec/;


.. _ref-arg-annos:

Argument Annotations
--------------------

.. argument-annotation:: AllowNone

    This boolean annotation specifies that the value of the corresponding
    argument (which should be either :stype:`SIP_PYBUFFER`,
    :stype:`SIP_PYCALLABLE`, :stype:`SIP_PYDICT`, :stype:`SIP_PYLIST`,
    :stype:`SIP_PYSLICE`, :stype:`SIP_PYTUPLE` or :stype:`SIP_PYTYPE`) may be
    ``None``.


.. argument-annotation:: Array

    This boolean annotation specifies that the corresponding argument refers
    to an array.
    
    The argument should be either a pointer to a wrapped type, a ``char *`` or
    a ``unsigned char *``.  If the argument is a character array then the
    annotation also implies the :aanno:`Encoding` annotation with an encoding
    of ``"None"``.

    There must be a corresponding argument with the :aanno:`ArraySize`
    annotation specified.  The annotation may only be specified once in a list
    of arguments.


.. argument-annotation:: ArraySize

    This boolean annotation specifies that the corresponding argument (which
    should be either ``short``, ``unsigned short``, ``int``, ``unsigned``,
    ``long`` or ``unsigned long``) refers to the size of an array.  There must
    be a corresponding argument with the :aanno:`Array` annotation specified.
    The annotation may only be specified once in a list of arguments.


.. argument-annotation:: Constrained

    Python will automatically convert between certain compatible types.  For
    example, if a floating pointer number is expected and an integer supplied,
    then the integer will be converted appropriately.  This can cause problems
    when wrapping C or C++ functions with similar signatures.  For example::

        // The wrapper for this function will also accept an integer argument
        // which Python will automatically convert to a floating point number.
        void foo(double);

        // The wrapper for this function will never get used.
        void foo(int);

    This boolean annotation specifies that the corresponding argument (which
    should be either ``bool``, ``int``, ``float``, ``double``, ``enum`` or a
    wrapped class) must match the type without any automatic conversions.  In
    the context of a wrapped class the invocation of any
    :directive:`%ConvertToTypeCode` is suppressed.

    The following example gets around the above problem::

        // The wrapper for this function will only accept floating point
        // numbers.
        void foo(double /Constrained/);

        // The wrapper for this function will be used for anything that Python
        // can convert to an integer, except for floating point numbers.
        void foo(int);


.. argument-annotation:: DocType

    .. versionadded:: 4.10

    This string annotation specifies the type of the argument as it will appear
    in any generated docstrings.  It is usually used with arguments of type
    :stype:`SIP_PYOBJECT` to provide a more specific type.


.. argument-annotation:: DocValue

    .. versionadded:: 4.10

    This string annotation specifies the default value of the argument as it
    will appear in any generated docstrings.


.. argument-annotation:: Encoding

    This string annotation specifies that the corresponding argument (which
    should be either ``char``, ``const char``, ``char *`` or ``const char *``)
    refers to an encoded character or ``'\0'`` terminated encoded string with
    the specified encoding.  The encoding can be either ``"ASCII"``,
    ``"Latin-1"``, ``"UTF-8"`` or ``"None"``.  An encoding of ``"None"`` means
    that the corresponding argument refers to an unencoded character or string.

    The default encoding is specified by the :directive:`%DefaultEncoding`
    directive.  If the directive is not specified then ``None`` is used.

    Python v3 will use the ``bytes`` type to represent the argument if the
    encoding is ``"None"`` and the ``str`` type otherwise.

    Python v2 will use the ``str`` type to represent the argument if the
    encoding is ``"None"`` and the ``unicode`` type otherwise.


.. argument-annotation:: GetWrapper

    This boolean annotation is only ever used in conjunction with handwritten
    code specified with the :directive:`%MethodCode` directive.  It causes an
    extra variable to be generated for the corresponding argument which is a
    pointer to the Python object that wraps the argument.

    See the :directive:`%MethodCode` directive for more detail.


.. argument-annotation:: In

    This boolean annotation is used to specify that the corresponding argument
    (which should be a pointer type) is used to pass a value to the function.

    For pointers to wrapped C structures or C++ class instances, ``char *`` and
    ``unsigned char *`` then this annotation is assumed unless the :aanno:`Out`
    annotation is specified.

    For pointers to other types then this annotation must be explicitly
    specified if required.  The argument will be dereferenced to obtain the
    actual value.

    Both :aanno:`In` and :aanno:`Out` may be specified for the same argument.


.. argument-annotation:: KeepReference

    This optional integer annotation is used to specify that a reference to the
    corresponding argument should be kept to ensure that the object is not
    garbage collected.  If the method is called again with a new argument then
    the reference to the previous argument is discarded.  Note that ownership
    of the argument is not changed.

    If the function is a method then the reference is kept by the instance,
    i.e. ``self``.  Therefore the extra reference is released when the instance
    is garbage collected.

    If the function is a class method or an ordinary function and it is
    annotated using the :fanno:`Factory` annotation, then the reference is
    kept by the object created by the function.  Therefore the extra reference
    is released when that object is garbage collected.

    Otherwise the reference is not kept by any specific object and will never
    be released.

    If a value is specified then it defines the argument's key.  Arguments of
    different constructors or methods that have the same key are assumed to
    refer to the same value.


.. argument-annotation:: NoCopy

    .. versionadded:: 4.10.1

    This boolean annotation is used with arguments of virtual methods that are
    a ``const`` reference to a class.  Normally, if the class defines a copy
    constructor then a copy of the returned reference is automatically created
    and wrapped before being passed to a Python reimplementation of the method.
    The copy will be owned by Python.  This means that the reimplementation may
    take a reference to the argument without having to make an explicit copy.
    
    If the annotation is specified then the copy is not made and the original
    reference is wrapped instead and will be owned by C++.


.. argument-annotation:: Out

    This boolean annotation is used to specify that the corresponding argument
    (which should be a pointer type) is used by the function to return a value
    as an element of a tuple.

    For pointers to wrapped C structures or C++ class instances, ``char *`` and
    ``unsigned char *`` then this annotation must be explicitly specified if
    required.

    For pointers to other types then this annotation is assumed unless the
    :aanno:`In` annotation is specified.

    Both :aanno:`In` and :aanno:`Out` may be specified for the same argument.


.. argument-annotation:: PyInt

    .. versionadded:: 4.12

    This boolean annotation is used with ``char``, ``signed char`` and
    ``unsigned char`` arguments to specify that they should be interpreted as
    integers rather than strings of one character.


.. argument-annotation:: ResultSize

    This boolean annotation is used with functions or methods that return a
    ``void *`` or ``const void *``.  It identifies an argument that defines the
    size of the block of memory whose address is being returned.  This allows
    the :class:`sip.voidptr` object that wraps the address to support the
    Python buffer protocol.


.. argument-annotation:: SingleShot

    This boolean annotation is used only with arguments of type
    :stype:`SIP_RXOBJ_CON` to specify that the signal connected to the slot
    will only ever be emitted once.  This prevents a certain class of memory
    leaks.


.. argument-annotation:: Transfer

    This boolean annotation is used to specify that ownership of the
    corresponding argument (which should be a wrapped C structure or C++ class
    instance) is transferred from Python to C++.  In addition, if the argument
    is of a class method, then it is associated with the class instance with
    regard to the cyclic garbage collector.

    If the annotation is used with the :aanno:`Array` annotation then the
    array of pointers to the sequence of C structures or C++ class instances
    that is created on the heap is not automatically freed.

    See :ref:`ref-object-ownership` for more detail.


.. argument-annotation:: TransferBack

    This boolean annotation is used to specify that ownership of the
    corresponding argument (which should be a wrapped C structure or C++ class
    instance) is transferred back to Python from C++.  In addition, any
    association of the argument with regard to the cyclic garbage collector
    with another instance is removed.

    See :ref:`ref-object-ownership` for more detail.


.. argument-annotation:: TransferThis

    This boolean annotation is only used in C++ constructors or methods.  In
    the context of a constructor or factory method it specifies that ownership
    of the instance being created is transferred from Python to C++ if the
    corresponding argument (which should be a wrapped C structure or C++ class
    instance) is not ``None``.  In addition, the newly created instance is
    associated with the argument with regard to the cyclic garbage collector.

    In the context of a non-factory method it specifies that ownership of
    ``this`` is transferred from Python to C++ if the corresponding argument is
    not ``None``.  If it is ``None`` then ownership is transferred to Python.

    The annotation may be used more that once, in which case ownership is
    transferred to last instance that is not ``None``.

    See :ref:`ref-object-ownership` for more detail.


.. _ref-class-annos:

Class Annotations
-----------------

.. class-annotation:: Abstract

    This boolean annotation is used to specify that the class has additional
    pure virtual methods that have not been specified and so it cannot be
    instantiated or sub-classed from Python.


.. class-annotation:: AllowNone

    .. versionadded:: 4.8.2

    Normally when a Python object is converted to a C/C++ instance ``None``
    is handled automatically before the class's
    :directive:`%ConvertToTypeCode` is called.  This boolean annotation
    specifies that the handling of ``None`` will be left to the
    :directive:`%ConvertToTypeCode`.  The annotation is ignored if the class
    does not have any :directive:`%ConvertToTypeCode`.


.. class-annotation:: API

    .. versionadded:: 4.9

    This API range annotation is used to specify an API and corresponding
    range of version numbers that the class is enabled for.

    If a class or mapped type has different implementations enabled for
    different ranges of version numbers then those ranges must not overlap.

    Note that sub-classing from a class that has different implementations is
    not currently supported.

    See :ref:`ref-incompat-apis` for more detail.


.. class-annotation:: DelayDtor

    This boolean annotation is used to specify that the class's destructor
    should not be called until the Python interpreter exits.  It would normally
    only be applied to singleton classes.

    When the Python interpreter exits the order in which any wrapped instances
    are garbage collected is unpredictable.  However, the underlying C or C++
    instances may need to be destroyed in a certain order.  If this annotation
    is specified then when the wrapped instance is garbage collected the C or
    C++ instance is not destroyed but instead added to a list of delayed
    instances.  When the interpreter exits then the function
    :c:func:`sipDelayedDtors()` is called with the list of delayed instances.
    :c:func:`sipDelayedDtors()` can then choose to call (or ignore) the
    destructors in any desired order.

    The :c:func:`sipDelayedDtors()` function must be specified using the
    :directive:`%ModuleCode` directive.

.. c:function:: void sipDelayedDtors(const sipDelayedDtor *dd_list)

    :param dd_list:
        the linked list of delayed instances.

.. c:type:: sipDelayedDtor

    This structure describes a particular delayed destructor.

    .. c:member:: const char* dd_name

        This is the name of the class excluding any package or module name.

    .. c:member:: void* dd_ptr

        This is the address of the C or C++ instance to be destroyed.  It's
        exact type depends on the value of :c:member:`dd_isderived`.

    .. c:member:: int dd_isderived

        This is non-zero if the type of :c:member:`dd_ptr` is actually the
        generated derived class.  This allows the correct destructor to be
        called.  See :ref:`ref-derived-classes`.

    .. c:member:: sipDelayedDtor* dd_next

        This is the address of the next entry in the list or zero if this is
        the last one.

    Note that the above applies only to C and C++ instances that are owned by
    Python.


.. class-annotation:: Deprecated

    This boolean annotation is used to specify that the class is deprecated.
    It is the equivalent of annotating all the class's constructors, function
    and methods as being deprecated.


.. class-annotation:: ExportDerived

    .. versionadded:: 4.15

    In many cases SIP generates a derived class for each class being wrapped
    (see :ref:`ref-derived-classes`).  Normally this is used internally.  This
    boolean annotation specifies that the declaration of the class is exported
    and able to be used by handwritten code.


.. class-annotation:: External

    This boolean annotation is used to specify that the class is defined in
    another module.  Declarations of external classes are private to the module
    in which they appear.


.. class-annotation:: Metatype

    This dotted name annotation specifies the name of the Python type object
    (i.e. the value of the ``tp_name`` field) used as the meta-type used when
    creating the type object for this C structure or C++ type.

    See the section :ref:`ref-types-metatypes` for more details.


.. class-annotation:: Mixin

    .. versionadded:: 4.15

    This boolean annotation specifies that the class can be used as a mixin
    with other wrapped classes.
    
    Normally a Python application cannot define a new class that is derived
    from more than one wrapped class.  In C++ this would create a new C++
    class.  This cannot be done from Python.  At best a C++ instance of each of
    the wrapped classes can be created and wrapped as separate Python objects.
    However some C++ classes may function perfectly well with this restriction.
    Such classes are often intended to be used as mixins.

    If this annotation is specified then a separate instance of the class is
    created.  The main instance automatically delegates to the instance of the
    mixin when required.  A mixin class should have the following
    characteristics:

    - Any constructor arguments should be able to be specified using keyword
      arguments.

    - The class should not have any virtual methods.


.. class-annotation:: NoDefaultCtors

    This boolean annotation is used to suppress the automatic generation of
    default constructors for the class.


.. class-annotation:: PyName

    This name annotation specifies an alternative name for the class being
    wrapped which is used when it is referred to from Python.  It is required
    when a class name is the same as a Python keyword.  It may also be used to
    avoid name clashes with other objects (e.g. enums, exceptions, functions)
    that have the same name in the same C++ scope.

    .. seealso:: :directive:`%AutoPyName`


.. class-annotation:: Supertype

    This dotted name annotation specifies the name of the Python type object
    (i.e. the value of the ``tp_name`` field) used as the super-type used when
    creating the type object for this C structure or C++ type.

    See the section :ref:`ref-types-metatypes` for more details.


.. class-annotation:: VirtualErrorHandler

    .. versionadded:: 4.14

    This name annotation specifies the handler (defined by the
    :directive:`%VirtualErrorHandler` directive) that is called when a Python
    re-implementation of any of the class's virtual C++ functions raises a
    Python exception.  If not specified then the handler specified by the
    ``default_VirtualErrorHandler`` argument of the :directive:`%Module`
    directive is used.

    .. seealso:: :fanno:`NoVirtualErrorHandler`, :fanno:`VirtualErrorHandler`, :directive:`%VirtualErrorHandler`


.. _ref-mapped-type-annos:

Mapped Type Annotations
-----------------------

.. mapped-type-annotation:: AllowNone

    Normally when a Python object is converted to a C/C++ instance ``None``
    is handled automatically before the mapped type's
    :directive:`%ConvertToTypeCode` is called.  This boolean annotation
    specifies that the handling of ``None`` will be left to the
    :directive:`%ConvertToTypeCode`.


.. mapped-type-annotation:: API

    .. versionadded:: 4.9

    This API range annotation is used to specify an API and corresponding
    range of version numbers that the mapped type is enabled for.

    If a class or mapped type has different implementations enabled for
    different ranges of version numbers then those ranges must not overlap.

    It should not be used with mapped type templates.

    See :ref:`ref-incompat-apis` for more detail.


.. mapped-type-annotation:: DocType

    .. versionadded:: 4.10

    This string annotation serves the same purpose as the :aanno:`DocType`
    argument annotation when applied to the mapped type being defined.


.. mapped-type-annotation:: NoRelease

    This boolean annotation is used to specify that the mapped type does not
    support the :c:func:`sipReleaseType()` function.  Any
    :directive:`%ConvertToTypeCode` should not create temporary instances of
    the mapped type, i.e. it should not return :c:macro:`SIP_TEMPORARY`.


.. mapped-type-annotation:: PyName

    This name annotation specifies an alternative name for the mapped type
    being wrapped which is used when it is referred to from Python.  The only
    time a Python type is created for a mapped type is when it is used as a
    scope for static methods or enums.
    
    It should not be used with mapped type templates.

    .. seealso:: :directive:`%AutoPyName`


.. _ref-enum-annos:

Enum Annotations
----------------

.. enum-annotation:: NoScope

    .. versionadded:: 4.15

    This boolean annotation specifies the that scope of an enum's members
    should be omitted in the generated code.  Normally this would mean that the
    generated code will not compile.  However it is useful when defining
    pseudo-enums, for example, to wrap global values so that they are defined
    (in Python) within the scope of a class.


.. enum-annotation:: PyName

    This name annotation specifies an alternative name for the enum or enum
    member being wrapped which is used when it is referred to from Python.  It
    is required when an enum or enum member name is the same as a Python
    keyword.  It may also be used to avoid name clashes with other objects
    (e.g. classes, exceptions, functions) that have the same name in the same
    C++ scope.

    .. seealso:: :directive:`%AutoPyName`


.. _ref-exception-annos:

Exception Annotations
---------------------

.. exception-annotation:: Default

    This boolean annotation specifies that the exception being defined will be
    used as the default exception to be caught if a function or constructor
    does not have a ``throw`` clause.

.. exception-annotation:: PyName

    This name annotation specifies an alternative name for the exception being
    defined which is used when it is referred to from Python.  It is required
    when an exception name is the same as a Python keyword.  It may also be
    used to avoid name clashes with other objects (e.g. classes, enums,
    functions) that have the same name.

    .. seealso:: :directive:`%AutoPyName`


.. _ref-function-annos:

Function Annotations
--------------------

.. function-annotation:: API

    .. versionadded:: 4.9

    This API range annotation is used to specify an API and corresponding
    range of version numbers that the function is enabled for.

    See :ref:`ref-incompat-apis` for more detail.


.. function-annotation:: AutoGen

    This optional name annotation is used with class methods to specify that
    the method be automatically included in all sub-classes.  The value is the
    name of a feature (specified using the :directive:`%Feature` directive)
    which must be enabled for the method to be generated.


.. function-annotation:: Default

    This boolean annotation is only used with C++ constructors.  Sometimes SIP
    needs to create a class instance.  By default it uses a constructor with no
    compulsory arguments if one is specified.  (SIP will automatically generate
    a constructor with no arguments if no constructors are specified.)  This
    annotation is used to explicitly specify which constructor to use.  Zero is
    passed as the value of any arguments to the constructor.  This annotation
    is ignored if the class defines :directive:`%InstanceCode`.


.. function-annotation:: Deprecated

    This boolean annotation is used to specify that the constructor or function
    is deprecated.  A deprecation warning is issued whenever the constructor or
    function is called.


.. function-annotation:: DocType

    .. versionadded:: 4.10

    This string annotation serves the same purpose as the :aanno:`DocType`
    argument annotation when applied to the type of the value returned by the
    function.


.. function-annotation:: Encoding

    This string annotation serves the same purpose as the :aanno:`Encoding`
    argument annotation when applied to the type of the value returned by the
    function.


.. function-annotation:: Factory

    This boolean annotation specifies that the value returned by the function
    (which should be a wrapped C structure or C++ class instance) is a newly
    created instance and is owned by Python.

    See :ref:`ref-object-ownership` for more detail.


.. function-annotation:: HoldGIL

    This boolean annotation specifies that the Python Global Interpreter Lock
    (GIL) is not released before the call to the underlying C or C++ function.
    See :ref:`ref-gil` and the :fanno:`ReleaseGIL` annotation.


.. function-annotation:: KeepReference

    .. versionadded:: 4.12.2

    This optional integer annotation serves the same purpose as the
    :aanno:`KeepReference` argument annotation when applied to the type of the
    value returned by the function.


.. function-annotation:: KeywordArgs

    .. versionadded:: 4.10

    This string annotation specifies the level of support the argument parser
    generated for this function will provide for passing the parameters using
    Python's keyword argument syntax.  The value of the annotation can be
    either ``"None"`` meaning that keyword arguments are not supported,
    ``"All"`` meaning that all named arguments can be passed as keyword
    arguments, or ``"Optional"`` meaning that all named optional arguments
    (i.e. those with a default value) can be passed as keyword arguments.

    If the annotation is not used then the value specified by the
    ``keyword_arguments`` argument of the :directive:`%Module` directive is
    used.

    Keyword arguments cannot be used for functions that use an ellipsis to
    designate that the function has a variable number of arguments.

    .. deprecated:: 4.12
        It can also be used as a boolean annotation which is the equivalent of
        specifiying a value of ``"All"``.


.. function-annotation:: __len__

    .. versionadded:: 4.10.3

    This boolean annotation specifies that a ``__len__()`` method should be
    automatically generated that will use the method being annotated to compute
    the value that the ``__len__()`` method will return.


.. function-annotation:: NewThread

    This boolean annotation specifies that the function will create a new
    thread.


.. function-annotation:: NoArgParser

    This boolean annotation is used with methods and global functions to
    specify that the supplied :directive:`%MethodCode` will handle the parsing
    of the arguments.


.. function-annotation:: NoCopy

    .. versionadded:: 4.10.1

    This boolean annotation is used with methods and global functions that
    return a ``const`` reference to a class.  Normally, if the class defines a
    copy constructor then a copy of the returned reference is automatically
    created and wrapped.  The copy will be owned by Python.
    
    If the annotation is specified then the copy is not made and the original
    reference is wrapped instead and will be owned by C++.


.. function-annotation:: NoDerived

    This boolean annotation is only used with C++ constructors.  In many cases
    SIP generates a derived class for each class being wrapped (see
    :ref:`ref-derived-classes`).  This derived class contains constructors with
    the same C++ signatures as the class being wrapped.  Sometimes you may want
    to define a Python constructor that has no corresponding C++ constructor.
    This annotation is used to suppress the generation of the constructor in
    the derived class.


.. function-annotation:: NoKeywordArgs

    .. versionadded:: 4.10

    .. deprecated:: 4.12
        Use the :fanno:`KeywordArgs` annotation with a value of ``"None"``.

    This boolean annotation specifies that the argument parser generated for
    this function will not support passing the parameters using Python's
    keyword argument syntax.  In other words, the argument parser will only
    support normal positional arguments.  This annotation is useful when the
    default setting of allowing keyword arguments has been changed via the
    command line or the :directive:`%Module` directive, but you would still
    like certain functions to only support positional arguments.


.. function-annotation:: NoRaisesPyException

    .. versionadded:: 4.13.1

    This boolean annotation specifies that the function or constructor does not
    raise a Python exception to indicate that an error occurred.

    .. seealso:: :fanno:`RaisesPyException`


.. function-annotation:: NoVirtualErrorHandler

    .. versionadded:: 4.14

    This boolean annotation specifies that when a Python re-implementation of a
    virtual C++ function raises a Python exception then ``PyErr_Print()`` is
    always called.  Any error handler specified by either the
    :fanno:`VirtualErrorHandler` function annotation, the
    :canno:`VirtualErrorHandler` class annotation or the
    ``default_VirtualErrorHandler`` argument of the :directive:`%Module`
    directive is ignored.

    .. seealso:: :fanno:`VirtualErrorHandler`, :canno:`VirtualErrorHandler`, :directive:`%VirtualErrorHandler`


.. function-annotation:: Numeric

    This boolean annotation specifies that the operator should be interpreted
    as a numeric operator rather than a sequence operator.
    
    Python uses the ``+`` operator for adding numbers and concatanating
    sequences, and the ``*`` operator for multiplying numbers and repeating
    sequences.  Unless this or the :fanno:`Sequence` annotation is specified,
    SIP tries to work out which is meant by looking at other operators that
    have been defined for the type.  If it finds either ``-``, ``-=``, ``/``,
    ``/=``, ``%`` or ``%=`` defined then it assumes that ``+``, ``+=``, ``*``
    and ``*=`` should be numeric operators.  Otherwise, if it finds either
    ``[]``, :meth:`__getitem__`, :meth:`__setitem__` or :meth:`__delitem__`
    defined then it assumes that they should be sequence operators.


.. function-annotation:: PostHook

    This name annotation is used to specify the name of a Python builtin that
    is called immediately after the call to the underlying C or C++ function or
    any handwritten code.  The builtin is not called if an error occurred.  It
    is primarily used to integrate with debuggers.


.. function-annotation:: PreHook

    This name annotation is used to specify the name of a Python builtin that
    is called immediately after the function's arguments have been successfully
    parsed and before the call to the underlying C or C++ function or any
    handwritten code.  It is primarily used to integrate with debuggers.


.. function-annotation:: PyName

    This name annotation specifies an alternative name for the function being
    wrapped which is used when it is referred to from Python.  It is required
    when a function or method name is the same as a Python keyword.  It may
    also be used to avoid name clashes with other objects (e.g. classes, enums,
    exceptions) that have the same name in the same C++ scope.

    .. seealso:: :directive:`%AutoPyName`


.. function-annotation:: PyInt

    .. versionadded:: 4.12

    This boolean annotation serves the same purpose as the :aanno:`PyInt`
    argument annotation when applied to the type of the value returned by the
    function.


.. function-annotation:: RaisesPyException

    .. versionadded:: 4.12.1

    This boolean annotation specifies that the function or constructor raises a
    Python exception to indicate that an error occurred.  Any current exception
    is cleared before the function or constructor is called.  It is ignored if
    the :directive:`%MethodCode` directive is used.

    .. seealso:: :fanno:`NoRaisesPyException`


.. function-annotation:: ReleaseGIL

    This boolean annotation specifies that the Python Global Interpreter Lock
    (GIL) is released before the call to the underlying C or C++ function and
    reacquired afterwards.  It should be used for functions that might block or
    take a significant amount of time to execute.  See :ref:`ref-gil` and the
    :fanno:`HoldGIL` annotation.


.. function-annotation:: Sequence

    .. versionadded:: 4.14.7

    This boolean annotation specifies that the operator should be interpreted
    as a sequence operator rather than a numeric operator.

    Python uses the ``+`` operator for adding numbers and concatanating
    sequences, and the ``*`` operator for multiplying numbers and repeating
    sequences.  Unless this or the :fanno:`Numeric` annotation is specified,
    SIP tries to work out which is meant by looking at other operators that
    have been defined for the type.  If it finds either ``-``, ``-=``, ``/``,
    ``/=``, ``%`` or ``%=`` defined then it assumes that ``+``, ``+=``, ``*``
    and ``*=`` should be numeric operators.  Otherwise, if it finds either
    ``[]``, :meth:`__getitem__`, :meth:`__setitem__` or :meth:`__delitem__`
    defined then it assumes that they should be sequence operators.


.. function-annotation:: Transfer

    This boolean annotation specifies that ownership of the value returned by
    the function (which should be a wrapped C structure or C++ class instance)
    is transferred to C++.  It is only used in the context of a class
    constructor or a method.

    In the case of methods returned values (unless they are new references to
    already wrapped values) are normally owned by C++ anyway.  However, in
    addition, an association between the returned value and the instance
    containing the method is created with regard to the cyclic garbage
    collector.

    See :ref:`ref-object-ownership` for more detail.


.. function-annotation:: TransferBack

    This boolean annotation specifies that ownership of the value returned by
    the function (which should be a wrapped C structure or C++ class instance)
    is transferred back to Python from C++.  Normally returned values (unless
    they are new references to already wrapped values) are owned by C++.  In
    addition, any association of the returned value with regard to the cyclic
    garbage collector with another instance is removed.

    See :ref:`ref-object-ownership` for more detail.


.. function-annotation:: TransferThis

    This boolean annotation specifies that ownership of ``this`` is transferred
    from Python to C++.

    See :ref:`ref-object-ownership` for more detail.


.. function-annotation:: VirtualErrorHandler

    .. versionadded:: 4.14

    This name annotation specifies the handler (defined by the
    :directive:`%VirtualErrorHandler` directive) that is called when a Python
    re-implementation of the virtual C++ function raises a Python exception.
    If not specified then the handler specified by the class's
    :canno:`VirtualErrorHandler` is used.

    .. seealso:: :fanno:`NoVirtualErrorHandler`, :canno:`VirtualErrorHandler`, :directive:`%VirtualErrorHandler`


.. _ref-typedef-annos:

Typedef Annotations
-------------------

.. typedef-annotation:: Capsule

    .. versionadded:: 4.14.1

    This boolean annotation may only be used when the base type is ``void *``
    and specifies that a Python capsule object is used to wrap the value rather
    than a :class:`sip.voidptr`.  The advantage of using a capsule is that name
    based type checking is performed using the name of the type being defined.

    For versions of Python that do not support capules :class:`sip.voidptr` is
    used instead and name based type checking is not performed.


.. typedef-annotation:: DocType

    .. versionadded:: 4.10

    This string annotation serves the same purpose as the :aanno:`DocType`
    argument annotation when applied to the mapped type being defined.


.. typedef-annotation:: Encoding

    This string annotation serves the same purpose as the :aanno:`Encoding`
    argument annotation when applied to the mapped type being defined.


.. typedef-annotation:: NoTypeName

    This boolean annotation specifies that the definition of the type rather
    than the name of the type being defined should be used in the generated
    code.

    Normally a typedef would be defined as follows::

        typedef bool MyBool;

    This would result in ``MyBool`` being used in the generated code.

    Specifying the annotation means that ``bool`` will be used in the generated
    code instead.


.. typedef-annotation:: PyInt

    .. versionadded:: 4.12

    This boolean annotation serves the same purpose as the :aanno:`PyInt`
    argument annotation when applied to the type being defined.


.. typedef-annotation:: PyName

    .. versionadded:: 4.13.1

    This name annotation only applies when the typedef is being used to create
    the wrapping for a class defined using a template and specifies an
    alternative name for the class when it is referred to from Python.  It is
    required when a class name is the same as a Python keyword.  It may also
    be used to avoid name clashes with other objects (e.g. enums, exceptions,
    functions) that have the same name in the same C++ scope.

    .. seealso:: :directive:`%AutoPyName`


.. _ref-variable-annos:

Variable Annotations
--------------------

.. variable-annotation:: DocType

    .. versionadded:: 4.10

    This string annotation serves the same purpose as the :aanno:`DocType`
    argument annotation when applied to the type of the variable being defined.


.. variable-annotation:: Encoding

    This string annotation serves the same purpose as the :aanno:`Encoding`
    argument annotation when applied to the type of the variable being defined.


.. variable-annotation:: PyInt

    .. versionadded:: 4.12

    This boolean annotation serves the same purpose as the :aanno:`PyInt`
    argument annotation when applied to the type of the variable being defined.


.. variable-annotation:: PyName

    This name annotation specifies an alternative name for the variable being
    wrapped which is used when it is referred to from Python.  It is required
    when a variable name is the same as a Python keyword.  It may also be used
    to avoid name clashes with other objects (e.g. classes, functions) that
    have the same name in the same C++ scope.

    .. seealso:: :directive:`%AutoPyName`
