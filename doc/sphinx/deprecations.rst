Deprecated Features and Behaviours
==================================

These features and behaviours currenly issue deprecation warnings and will be
removed in PyQt v5.5.  To ensure that the warnings are written to ``stderr``
pass the ``-Wa`` command line option to the Python interpreter.  To run your
application as it would with PyQt v5.5 pass the ``-We`` command line option
instead.


Conversion of Latin-1 Strings to :class:`~PyQt5.QtCore.QByteArray`
------------------------------------------------------------------

The automatic conversion of a Latin-1 encoded string when a
:class:`~PyQt5.QtCore.QByteArray` is expected is deprecated.  In PyQt v5.5 an
exception will be raised instead.


Unhandled Python Exceptions
---------------------------

There are a number of situations where Python code is executed from C++.
Python reimplementations of C++ virtual methods is probably the most common
example.  If the Python code raises an exception then PyQt will call Python's
:c:func:`PyErr_Print` function which will then call :func:`sys.excepthook`.
The default exception hook will then display the exception and any traceback to
``stderr``.  There are number of disadvantages to this default behaviour:

- the application does not terminate, meaning the behaviour is different to
  when exceptions are raised in other situations

- the output written to ``stderr`` may not be seen by the developer or user
  (particularly if it is a GUI application) thereby hiding the fact that the
  application is trying to report a potential bug.

This behaviour is deprecated in PyQt v5.4.  In PyQt v5.5 an unhandled Python
exception will result in a call to Qt's :cpp:func:`qFatal` function.  By
default this will call :c:func:`abort` and the application will terminate.
Note that an application installed exception hook will still take precedence.
