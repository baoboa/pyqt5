Introduction
============

This is the reference guide for SIP 4.15.3-snapshot-5dc8c370157e.  SIP is a tool for
automatically generating `Python <http://www.python.org>`__ bindings for C and
C++ libraries.  SIP was originally developed in 1998 for
`PyQt <http://www.riverbankcomputing.com/software/pyqt>`__ - the Python
bindings for the Qt GUI toolkit - but is suitable for generating bindings for
any C or C++ library.

This version of SIP generates bindings for Python v2.3 or later, including
Python v3.

There are many other similar tools available.  One of the original such tools
is `SWIG <http://www.swig.org>`__ and, in fact, SIP is so called because it
started out as a small SWIG.  Unlike SWIG, SIP is specifically designed for
bringing together Python and C/C++ and goes to great lengths to make the
integration as tight as possible.

The homepage for SIP is http://www.riverbankcomputing.com/software/sip.  Here
you will always find the latest stable version and the latest version of this
documentation.

SIP can also be downloaded from the
`Mercurial <http://mercurial.selenic.com/>`__ repository at
http://www.riverbankcomputing.com/hg/sip.


License
-------

SIP is licensed under similar terms as Python itself.  SIP is also licensed
under the GPL (both v2 and v3).  It is your choice as to which license you
use.  If you choose the GPL then any bindings you create must be distributed
under the terms of the GPL.


Features
--------

SIP, and the bindings it produces, have the following features:

- bindings are fast to load and minimise memory consumption especially when
  only a small sub-set of a large library is being used

- automatic conversion between standard Python and C/C++ data types

- overloading of functions and methods with different argument signatures

- support for Python's keyword argument syntax

- support for both explicitly specified and automatically generated docstrings

- access to a C++ class's protected methods

- the ability to define a Python class that is a sub-class of a C++ class,
  including abstract C++ classes

- Python sub-classes can implement the :meth:`__dtor__` method which will be
  called from the C++ class's virtual destructor

- support for ordinary C++ functions, class methods, static class methods,
  virtual class methods and abstract class methods

- the ability to re-implement C++ virtual and abstract methods in Python

- support for global and class variables

- support for global and class operators

- support for C++ namespaces

- support for C++ templates

- support for C++ exceptions and wrapping them as Python exceptions

- the automatic generation of complementary rich comparison slots

- support for deprecation warnings

- the ability to define mappings between C++ classes and similar Python data
  types that are automatically invoked

- the ability to automatically exploit any available run time type information
  to ensure that the class of a Python instance object matches the class of the
  corresponding C++ instance

- the ability to change the type and meta-type of the Python object used to
  wrap a C/C++ data type

- full support of the Python global interpreter lock, including the ability to
  specify that a C++ function of method may block, therefore allowing the lock
  to be released and other Python threads to run

- support for consolidated modules where the generated wrapper code for a
  number of related modules may be included in a single, possibly private,
  module

- support for the concept of ownership of a C++ instance (i.e. what part of the
  code is responsible for calling the instance's destructor) and how the
  ownership may change during the execution of an application

- the ability to generate bindings for a C++ class library that itself is built
  on another C++ class library which also has had bindings generated so that
  the different bindings integrate and share code properly

- a sophisticated versioning system that allows the full lifetime of a C++
  class library, including any platform specific or optional features, to be
  described in a single set of specification files 

- the ability to include documentation in the specification files which can be
  extracted and subsequently processed by external tools

- the ability to include copyright notices and licensing information in the
  specification files that is automatically included in all generated source
  code

- a build system, written in Python, that you can extend to configure, compile
  and install your own bindings without worrying about platform specific issues

- support for building your extensions using distutils

- SIP, and the bindings it produces, runs under UNIX, Linux, Windows and
  MacOS/X


SIP Components
--------------

SIP comprises a number of different components.

- The SIP code generator (:program:`sip`).  This processes :file:`.sip`
  specification files and generates C or C++ bindings.  It is covered in detail
  in :ref:`ref-using`.

- The SIP header file (:file:`sip.h`).  This contains definitions and data
  structures needed by the generated C and C++ code.

- The SIP module (:file:`sip.so` or :file:`sip.pyd`).  This is a Python
  extension module that is imported automatically by SIP generated bindings and
  provides them with some common utility functions.  See also
  :ref:`ref-python-api`.

- The SIP build system (:file:`sipconfig.py`).  This is a pure Python module
  that is created when SIP is configured and encapsulates all the necessary
  information about your system including relevant directory names, compiler
  and linker flags, and version numbers.  It also includes several Python
  classes and functions which help you write configuration scripts for your own
  bindings.  It is covered in detail in :ref:`ref-build-system`.

- The SIP distutils extension (:file:`sipdistutils.py`).  This is a distutils
  extension that can be used to build your extension modules using distutils
  and is an alternative to writing configuration scripts with the SIP build
  system.  This can be as simple as adding your .sip files to the list of files
  needed to build the extension module.  It is covered in detail in
  :ref:`ref-distutils`.


Preparing for SIP v5
--------------------

The syntax of a SIP specification file will change in SIP v5.  The command line
options to the SIP code generator will also change.  In order to help users
manage the transition the following approach will be adopted.

- Where possible, all incompatible changes will be first implemented in SIP v4.

- When an incompatible change is implemented, the old syntax will be deprecated
  (with a warning message) but will be supported for the lifetime of v4.


Qt Support
----------

SIP has specific support for the creation of bindings based on Digia's Qt
toolkit.

The SIP code generator understands the signal/slot type safe callback mechanism
that Qt uses to connect objects together.  This allows applications to define
new Python signals, and allows any Python callable object to be used as a slot.

SIP itself does not require Qt to be installed.
