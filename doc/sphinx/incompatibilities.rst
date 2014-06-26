Potential Incompatibilities with Earlier Versions
=================================================

PyQt5 v5.3
----------

Execution of Python Slots
*************************

In previous versions, when a signal was emitted to a Python slot
that was not decorated with :func:`~PyQt5.QtCore.pyqtSlot`, it would not check
that the underlying C++ receiver instance still existed.  This matched the
PyQt4 behaviour at the time that PyQt5 v5.0 was released, but doesn't reflect
the standard C++ behaviour.

The lack of a check meant that an object could connect its
:func:`~PyQt5.QtCore.QObject.destroyed` signal to itself so that it could
monitor when its underlying C++ instance was destroyed.  Unfortunately this
turned out to be a potential source of obscure bugs for more common code.

In this version the check has been introduced - hence creating an
incompatibility for any code that relies on the earlier behaviour.  As a
workaround for this the ``no_receiver_check`` argument has been added to
:func:`~PyQt5.QtCore.QObject.connect` which allows the check to be suppressed
on a per connection basis.
