Potential Incompatibilities with Earlier Versions
=================================================

This section describes incompatibilities introduced by particular versions of
SIP.  Normally these are the removal of previously deprecated features.


SIP v4.14.4
-----------

Prior to this version, the handwritten code defined by the
:directive:`%VirtualErrorHandler` directive was called without the Python
Global Interpreter Lock (GIL) being held and from an arbitrary thread.

Starting with this version the code is called with the GIL being held and from
the thread that raised the error.  In addition the code is provided a value
called ``sipGILState`` that may be passed to :c:func:`SIP_RELEASE_GIL` in order
to release the GIL.  This must be done if the code changes the execution path
(e.g. by throwing a C++ exception).


SIP v4.12.3
-----------

Prior to this version, when SIP searches a class hierachy to see if there is a
Python reimplementation of a virtual C++ method, it ignored any objects that
were not Python functions or methods.

Starting with this version such an object is not ignored and will be called.
If it is not callable then a Python exception will be raised.  For example,
the following code will now raise an excepton because the ``Mixin.event``
attribute will now be called as it is assumed to be a valid reimplementation of
``QObject.event()``::

    class Mixin:
        event = False

    class MyObject(QObject, Mixin):
        pass


SIP v4.12
---------

Prior to this version several directives ignored any enclosing :directive:`%If`
directive.  Starting with this version all directives are affected by the
:directive:`%If` directive.


SIP v4.10.1
-----------

Newly Deprecated Features
*************************

The following parts of the :ref:`C API <ref-c-api>` are now deprecated (but
still supported).

- The ``D`` format character of :c:func:`sipParseResult()`.


SIP v4.8
--------

__truediv__
***********

Prior to this version the :meth:`__div__` special method implicitly defined the
:meth:`__truediv__` special method.  From this version the :meth:`__truediv__`
special method must be explicitly defined.


sipWrapper user Member
**********************

Prior to this version the :c:type:`sipWrapper` structure had a member called
:c:type:`user` which is available for handwritten code to use.  From this
version :c:type:`user` is a member of the :c:type:`sipSimpleWrapper` structure.

:c:type:`sipWrapper` pointers can be safely cast to :c:type:`sipSimpleWrapper`
pointers, so if your code does something like::

    ((sipWrapper *)obj)->user = an_object_reference;

then you just need to change it to::

    ((sipSimpleWrapper *)obj)->user = an_object_reference;


Removal of Previously Deprecated Features
*****************************************

The following parts of the :ref:`C API <ref-c-api>` have been removed.

- The ``a``, ``A``, ``M``, ``N``, ``O``, ``P`` and ``T`` format characters
  from :c:func:`sipBuildResult()` and :c:func:`sipCallMethod()`.

- The ``a``, ``A``, ``L`` and ``M`` format characters from
  :c:func:`sipParseResult()`.

- :c:func:`sipConvertToCpp()`

- :c:func:`sipIsSubClassInstance()`

- :c:func:`sipTransfer()`

- The :func:`transfer` function of the :mod:`sip` module.

- The old-style generated type convertors.

In addition the :option:`-a` command line option to :file:`configure.py` has
been removed.


Removal of PyQt-specific Features
*********************************

The following PyQt-specific support functions have been removed.

- :c:func:`sipConnectRx()`

- :c:func:`sipDisconnectRx()`

- :c:func:`sipEmitSlot()`

- :c:func:`sipGetSender()`


Newly Deprecated Features
*************************

The following parts of the :ref:`C API <ref-c-api>` are now deprecated (but
still supported).

- The :ref:`ref-type-objects`.

- The :ref:`ref-enum-type-objects`.

- :c:func:`sipConvertFromInstance()`

- :c:func:`sipConvertFromMappedType()`

- :c:func:`sipConvertFromNamedEnum()`

- :c:func:`sipConvertFromNewInstance()`

- :c:func:`sipCanConvertToInstance()`

- :c:func:`sipCanConvertToMappedType()`

- :c:func:`sipConvertToInstance()`

- :c:func:`sipConvertToMappedType()`

- :c:func:`sipForceConvertToInstance()`

- :c:func:`sipForceConvertToMappedType()`

- :c:func:`sipClassName()`

- :c:func:`sipFindClass()`

- :c:func:`sipFindNamedEnum()`

- :c:func:`sipFindMappedType()`

- :c:func:`sipGetWrapper()`

- :c:func:`sipReleaseInstance()`

- :c:func:`sipReleaseMappedType()`

- :c:func:`sipWrapper_Check()`

- The ``B``, ``C`` and ``E`` format characters of :c:func:`sipBuildResult()`
  and :c:func:`sipCallMethod()`.

- The ``s``, ``C`` and ``E`` format characters of :c:func:`sipParseResult()`.


SIP v4.7.8
----------

Automatic int to Enum Conversions
*********************************

This version allows a Python ``int`` object to be passed whenever an enum is
expected.  This can mean that two signatures that were different with prior
versions are now the same as far as Python is concerned.

The :aanno:`Constrained` argument annotation can now be applied to an enum
argument to revert to the earlier behaviour.


SIP v4.7.3
----------

Complementary Comparison Operators
**********************************

Prior to this version SIP did not automatically generate missing complementary
comparison operators.  Typically this was worked around by adding them
explicitly to the .sip files, even though they weren't implemented in C++ and
relied on the C++ compiler calling the complementary operator that was
implemented.

A necessary change to the code generator meant that this not longer worked and
so SIP was changed to automatically generate any missing complementary
operators.  If you have added such operators explicitly then you should remove
them or make them dependent on the particular version of SIP.


SIP v4.4
--------

%ConvertFromTypeCode and %ConvertToTypeCode
*******************************************

Handwritten :directive:`%ConvertFromTypeCode` and
:directive:`%ConvertToTypeCode` now have the responsibility for implementing
the :aanno:`Transfer` and :aanno:`TransferBack` annotations.


SIP_BUILD
*********

The :c:macro:`SIP_BUILD` C preprocessor symbol has been removed.


Newly Deprecated Features
*************************

The following parts of the :ref:`C API <ref-c-api>` are now deprecated (but
still supported).

- The old-style generated type convertors.

- :c:func:`sipConvertToCpp()`

- :c:func:`sipIsSubClassInstance()`
