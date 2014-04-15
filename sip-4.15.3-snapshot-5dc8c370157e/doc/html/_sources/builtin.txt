.. _ref-builtin:

Builtin Modules and Custom Interpreters
=======================================

Sometimes you want to create a custom Python interpreter with some modules
built in to the interpreter itself rather than being dynamically loaded.  To
do this the module must be created as a static library and linked with a
custom stub and the normal Python library.

To build the SIP module as a static library you must pass the ``-k`` command
line option to ``configure.py``.  You should then build and install SIP as
normal.  (Note that, because the module is now a static library, you will not
be able to import it.)

To build a module you have created for your own library you must modify your
own configuration script to pass a non-zero value as the ``static`` argument
of the ``__init__()`` method of the :class:`sipconfig.ModuleMakefile` class (or
any derived class you have created).  Normally you would make this configurable
using a command line option in the same way that SIP's ``configure.py`` handles
it.

The next stage is to create a custom stub and a Makefile.  The SIP distribution
contains a directory called ``custom`` which contains example stubs and a
Python script that will create a correct Makefile.  Note that, if your copy of
SIP was part of a standard Linux distribution, the ``custom`` directory may
not be installed on your system.

The ``custom`` directory contains the following files.  They are provided as
examples - each needs to be modified according to your particular
requirements.

    - ``mkcustom.py`` is a Python script that will create a Makefile which is
      then used to build the custom interpreter.  Comments in the file describe
      how it should be modified.

    - ``custom.c`` is a stub for a custom interpreter on Linux/UNIX.  It
      should also be used for a custom console interpreter on Windows (i.e.
      like ``python.exe``).  Comments in the file describe how it should be
      modified.

    - ``customw.c`` is a stub for a custom GUI interpreter on Windows (i.e.
      like ``pythonw.exe``).  Comments in the file describe how it should be
      modified.

Note that this technique does not restrict how the interpreter can be used.
For example, it still allows users to write their own applications that can
import your builtin modules.  If you want to prevent users from doing that,
perhaps to protect a proprietary API, then take a look at the
`VendorID <http://www.riverbankcomputing.com/software/vendorid/>`__ package.
