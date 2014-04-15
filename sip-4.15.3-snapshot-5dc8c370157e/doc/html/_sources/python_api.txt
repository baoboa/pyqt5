.. _ref-python-api:

Python API for Applications
===========================

.. module:: sip

The main purpose of the :mod:`sip` module is to provide functionality common to
all SIP generated bindings.  It is loaded automatically and most of the time
you will completely ignore it.  However, it does expose some functionality that
can be used by applications.


.. function:: cast(obj, type) -> object

    This does the Python equivalent of casting a C++ instance to one of its
    sub or super-class types.

    :param obj:
        the Python object.
    :param type:
        the type.
    :return:
        a new Python object is that wraps the same C++ instance as *obj*, but
        has the type *type*.


.. function:: delete(obj)

    For C++ instances this calls the C++ destructor.  For C structures it
    returns the structure's memory to the heap.

    :param obj:
        the Python object.


.. function:: dump(obj)

    This displays various bits of useful information about the internal state
    of the Python object that wraps a C++ instance or C structure.

    :param obj:
        the Python object.


.. function:: enableautoconversion(type, enable) -> bool

    .. versionadded:: 4.14.7

    Instances of some classes may be automatically converted to other Python
    objects even though the class has been wrapped.  This allows that behaviour
    to be suppressed so that an instances of the wrapped class is returned
    instead.

    :param type:
        the Python type object.
    :param enable:
        is ``True`` if auto-conversion should be enabled for the type.  This is
        the default behaviour.
    :return:
        ``True`` or ``False`` depending on whether or not auto-conversion was
        previously enabled for the type.  This allows the previous state to be
        restored later on.


.. function:: getapi(name) -> version

    .. versionadded:: 4.9

    This returns the version number that has been set for an API.  The version
    number is either set explicitly by a call to :func:`sip.setapi` or
    implicitly by importing the module that defines it.

    :param name:
        the name of the API.
    :return:
        The version number that has been set for the API.  An exception will
        be raised if the API is unknown.


.. function:: isdeleted(obj) -> bool

    This checks if the C++ instance or C structure has been deleted and
    returned to the heap.

    :param obj:
        the Python object.
    :return:
        ``True`` if the C/C++ instance has been deleted.


.. function:: ispycreated(obj) -> bool

    .. versionadded:: 4.12.1

    This checks if the C++ instance or C structure was created by Python.  If
    it was then it is possible to call a C++ instance's protected methods.

    :param obj:
        the Python object.
    :return:
        ``True`` if the C/C++ instance was created by Python.


.. function:: ispyowned(obj) -> bool

    This checks if the C++ instance or C structure is owned by Python.

    :param obj:
        the Python object.
    :return:
        ``True`` if the C/C++ instance is owned by Python.


.. function:: setapi(name, version)

    .. versionadded:: 4.9

    This sets the version number of an API.  An exception is raised if a
    different version number has already been set, either explicitly by a
    previous call, or implicitly by importing the module that defines it.

    :param name:
        the name of the API.
    :param version:
        The version number to set for the API.  Version numbers must be
        greater than or equal to 1.


.. function:: setdeleted(obj)

    This marks the C++ instance or C structure as having been deleted and
    returned to the heap so that future references to it raise an exception
    rather than cause a program crash.  Normally SIP handles such things
    automatically, but there may be circumstances where this isn't possible.

    :param obj:
        the Python object.


.. function:: setdestroyonexit(destroy)

    .. versionadded:: 4.14.2

    When the Python interpreter exits it garbage collects those objects that it
    can.  This means that any corresponding C++ instances and C structures
    owned by Python are destroyed.  Unfortunately this happens in an
    unpredictable order and so can cause memory faults within the wrapped
    library.  Calling this function with a value of ``False`` disables the
    automatic destruction of C++ instances and C structures.

    :param destroy:
        ``True`` if all C++ instances and C structures owned by Python should
        be destroyed when the interpreter exits.  This is the default.


.. function:: settracemask(mask)

    If the bindings have been created with SIP's :option:`-r <sip -r>` command
    line option then the generated code will include debugging statements that
    trace the execution of the code.  (It is particularly useful when trying to
    understand the operation of a C++ library's virtual function calls.)

    :param mask:
        the mask that determines which debugging statements are enabled.

    Debugging statements are generated at the following points:

    - in a C++ virtual function (*mask* is ``0x0001``)
    - in a C++ constructor (*mask* is ``0x0002``)
    - in a C++ destructor (*mask* is ``0x0004``)
    - in a Python type's __init__ method (*mask* is ``0x0008``)
    - in a Python type's __del__ method (*mask* is ``0x0010``)
    - in a Python type's ordinary method (*mask* is ``0x0020``).

    By default the trace mask is zero and all debugging statements are
    disabled.


.. class:: simplewrapper

    This is an alternative type object than can be used as the base type of an
    instance wrapped by SIP.  Objects using this are smaller than those that
    use the default :class:`sip.wrapper` type but do not support the concept of
    object ownership.


.. data:: SIP_VERSION

    This is a Python integer object that represents the SIP version number as
    a 3 part hexadecimal number (e.g. v4.0.0 is represented as ``0x040000``).
    It was first implemented in SIP v4.2.


.. data:: SIP_VERSION_STR

    This is a Python string object that defines the SIP version number as
    represented as a string.  For development snapshots it will start with
    ``snapshot-``.  It was first implemented in SIP v4.3.


.. function:: transferback(obj)

    This function is a wrapper around :c:func:`sipTransferBack()`.


.. function:: transferto(obj, owner)

    This function is a wrapper around :c:func:`sipTransferTo()`.


.. function:: unwrapinstance(obj) -> integer

    This returns the address, as an integer, of a wrapped C/C++ structure or
    class instance.

    :param obj:
        the Python object.
    :return:
        an integer that is the address of the C/C++ instance.


.. class:: voidptr

    This is the type object for the type SIP uses to represent a C/C++
    ``void *``.  It may have a size associated with the address in which case
    the Python buffer interface is supported.  The type has the following
    methods.

    .. method:: __init__(address[, size=-1[, writeable=True]])

        :param address:
            the address, either another :class:`sip.voidptr`, ``None``, a
            Python Capsule, a Python CObject, an object that implements the
            buffer protocol or an integer.
        :param size:
            the optional associated size of the block of memory and is negative
            if the size is not known.
        :param writeable:
            set if the memory is writeable.  If it is not specified, and
            *address* is a :class:`sip.voidptr` instance then its value will be
            used.

    .. method:: __int__() -> integer

        This returns the address as an integer.

        :return:
            the integer address.

    .. method:: __getitem__(idx) -> item

        .. versionadded:: 4.12

        This returns the item at a given index.  An exception will be raised if
        the address does not have an associated size.  In this way it behaves
        like a Python ``memoryview`` object.

        :param idx:
            is the index which may either be an integer, an object that
            implements ``__index__()`` or a slice object.
        :return:
            the item.  If the index is an integer then the item will be a
            Python v2 string object or a Python v3 bytes object containing the
            single byte at that index.  If the index is a slice object then the
            item will be a new :class:`voidptr` object defining the subset of
            the memory corresponding to the slice.

    .. method:: __hex__() -> string

        This returns the address as a hexadecimal string.

        :return:
            the hexadecimal string address.

    .. method:: __len__() -> integer

        .. versionadded:: 4.12

        This returns the size associated with the address.
        
        :return:
            the associated size.  An exception will be raised if there is none.

    .. method:: __setitem__(idx, item)

        .. versionadded:: 4.12

        This updates the memory at a given index.  An exception will be raised
        if the address does not have an associated size or is not writable.  In
        this way it behaves like a Python ``memoryview`` object.

        :param idx:
            is the index which may either be an integer, an object that
            implements ``__index__()`` or a slice object.
        :param item:
            is the data that will update the memory defined by the index.  It
            must implement the buffer interface and be the same size as the
            data that is being updated.

    .. method:: ascapsule() -> capsule

        .. versionadded:: 4.10

        This returns the address as an unnamed Python Capsule.  This requires
        Python v3.1 or later or Python v2.7 or later.

        :return:
            the Capsule.

    .. method:: ascobject() -> cObject

        This returns the address as a Python CObject.  This is deprecated with
        Python v3.1 and is not supported with Python v3.2 and later.

        :return:
            the CObject.

    .. method:: asstring([size=-1]) -> string/bytes

        This returns a copy of the block of memory as a Python v2 string object
        or a Python v3 bytes object.
        
        :param size:
            the number of bytes to copy.  If it is negative then the size
            associated with the address is used.  If there is no associated
            size then an exception is raised.
        :return:
            the string or bytes object.

    .. method:: getsize() -> integer

        This returns the size associated with the address.
        
        :return:
            the associated size which will be negative if there is none.

    .. method:: setsize(size)

        This sets the size associated with the address.
        
        :param size:
            the size to associate.  If it is negative then no size is
            associated.

    .. method:: getwriteable() -> bool

        This returns the writeable state of the memory.

        :return:
            ``True`` if the memory is writeable.

    .. method:: setwriteable(writeable)

        This sets the writeable state of the memory.

        :param writeable:
            the writeable state to set.


.. function:: wrapinstance(addr, type) -> object

    This wraps a C structure or C++ class instance in a Python object.  If the
    instance has already been wrapped then a new reference to the existing
    object is returned.
    
    :param addr:
        the address of the instance as a number.
    :param type:
        the Python type of the instance.
    :return:
        the Python object that wraps the instance.


.. class:: wrapper

    This is the type object of the default base type of all instances wrapped
    by SIP.  The :canno:`Supertype` class annotation can be used to specify a
    different base type for a class.


.. class:: wrappertype

    This is the type object of the metatype of the :class:`sip.wrapper` type.
