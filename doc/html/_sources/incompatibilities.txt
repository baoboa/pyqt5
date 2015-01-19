Potential Incompatibilities with Earlier Versions
=================================================

PyQt v5.3
---------

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


Qt Signals with Default Arguments
*********************************

In previous versions Qt signals with default arguments were exposed as multiple
signals each with one additional default argument.  For example
``QAbstractButton::clicked(bool checked = false)`` was exposed as
``QAbstractButton::clicked(bool checked)`` and ``QAbstractButton::clicked()``
where the former was the default signal.  It was therefore possible to index
the latter by using an empty tuple as the key - although there was no benefit
in doing so.

In this version only the signal with all arguments supplied is exposed.
However the signal's ``emit()`` method still supports the default argument,
i.e. when used normally the change should not be noticed.
