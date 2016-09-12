Installing PyQt5
================

Both the GPL and commercial versions of PyQt5 can be built from source packages
or installed from binary wheels.


Installing from Wheels
----------------------

Wheels are the standard Python packaging format for pure Python or binary
extension modules such as PyQt5.  PyQt5 wheels are specific to a particular
version of Python.  Only Python v3.5 and later is supported.  Wheels are
provide for 32- and 64-bit Windows, 64-bit OS X and 64-bit Linux.  These
correspond with the platforms for which The Qt Company provide binary
installers.

Wheels are installed using the :program:`pip3` program that is included with
current versions of Python.


Installing the GPL Version
..........................

To install the wheel for the GPL version of PyQt5, run::

    pip3 install pyqt5

This will install the wheel for your platform and your version of Python
(assuming both are supported).  The wheel will be automatically downloaded from
the Python Package Index.

If you get an error message saying that no downloads could be found that
satisfy the rquirement then you are probably using an unsupported version of
Python.

The PyQt5 wheel includes the necessary parts of the LGPL version of Qt.  There
is no need to install Qt yourself.

sip is packaged as a separate wheel which will be downloaded and installed
automatically.

To uninstall the GPL version, run::

    pip3 uninstall pyqt5


Installing the Commercial Version
.................................

.. program:: pyqtlicense

It is not possible to provide wheels for the commercial version in the same way
they are provided for the GPL version:

- the user's license information has to be applied

- it is not possble to distribute a copy of the commercial version of Qt.

Instead *unlicensed* wheels are provided which do not include a copy of Qt.
The program :program:`pyqtlicense` is provided which takes the unlicensed
wheel, the ``pyqt-commercial.sip`` license file and the location of the Qt
installation and generates a *licensed* wheel.  The licensed wheel contains a
copy of the necessary parts of Qt and can be installed using :program:`pip3`.

:program:`pyqtlicense` assumes that the Qt installation has been created from
one of the LGPL or commercial binary installers provided by The Qt Company.  It
may also work with a Qt installation built from source but this is unsupported.

On Windows the binary installer for MSVC 2015 must be used.

The following describes the command line options of :program:`pyqtlicense`.

.. cmdoption:: -h, --help

    Display a help message and exit.

.. cmdoption:: -V, --version

    Display the version number and exit.

.. cmdoption:: --license FILE

    This specifies that ``FILE`` is the license file.

.. cmdoption:: --no-msvc-runtime

    The unlicensed wheels for 32- and 64-bit Python includes ``msvcp140.dll``
    (part of the MSVC2015 C++ runtime).  This specifies that the DLL should be
    omitted from the licensed wheel.

.. cmdoption:: --no-openssl

    The unlicensed wheels for 32- and 64-bit Python includes ``libeay32.dll``
    and ``ssleay32.dll`` (i.e the OpenSSL DLLs).  This specifies that the DLLs
    should be omitted from the licensed wheel.

.. cmdoption:: --openssl DIR

    This specifies that the ``libeay32.dll`` and ``ssleay32.dll`` DLLs included
    in the unlicensed wheels for 32- and 64-bit Python should be replaced by
    the DLLs of the same name in the directory ``DIR``.

.. cmdoption:: --output DIR

    This specifies that the licensed wheel will be written to the directory
    ``DIR``.

.. cmdoption:: --qt DIR

    This specifies that ``DIR`` contains the LGPL or commercial Qt installation
    to be included in the licensed wheel.  It must be specified.

.. cmdoption:: --quiet

    This specifies that all progress messages should be suppressed.

.. cmdoption:: --verbose

    This specifies that additional progress messages should be displayed.

The remaining arguments are the names of the unlicensed wheel files to license.

To uninstall the commercial version, run::

    pip3 uninstall pyqt5-commercial


Building and Installing from Source
-----------------------------------

.. program:: configure.py

Downloading SIP
...............

SIP must be installed before building and using PyQt5.  You can get the latest
release of the SIP source code from
http://www.riverbankcomputing.com/software/sip/download.

The SIP installation instructions can be found at
http://pyqt.sourceforge.net/Docs/sip4/installation.html.


Downloading PyQt5
.................

You can get the latest release of the GPL version of the PyQt5 source code from
http://www.riverbankcomputing.com/software/pyqt/download5.

If you are using the commercial version of PyQt5 then you should use the
download instructions which were sent to you when you made your purchase.  You
must also download your ``pyqt-commercial.sip`` license file.


Configuring PyQt5
.................

After unpacking the source package (either a ``.tar.gz`` or a ``.zip`` file
depending on your platform) you should then check for any :file:`README` files
that relate to your platform.

If you are using the commercial version of PyQt5 then you must copy your
``pyqt-commercial.sip`` license file to the :file:`sip` directory, or to the
directory specified by the :option:`--license-dir <configure.py --license-dir>`
option of :program:`configure.py`.

You need to make sure your environment variables are set properly for your
development environment.

In order to configure the build of PyQt5 you need to run the
:program:`configure.py` script as follows::

    python3 configure.py

This assumes that the Python interpreter is on your path.  Something like the
following may be appropriate on Windows::

    c:\Python35\python configure.py

If you have multiple versions of Python installed then make sure you use the
interpreter for which you wish to build PyQt5 for.

The full set of command line options is:

.. cmdoption:: -h, --help

    Display a help message and exit.

.. cmdoption:: --assume-shared

    Normally Qt is checked to see if it has been built as shared libraries.
    Some Linux distributions configure their Qt builds to make this check
    unreliable.  This option ignores the result of the check and assumes that
    Qt has been built as shared libraries.

.. cmdoption:: --bindir <DIR>

    The :program:`pyuic5`, :program:`pyrcc5` and :program:`pylupdate5`
    utilities will be installed in the directory ``<DIR>``.

.. cmdoption:: --concatenate

    The C++ source files for a Python module will be concatenated.  This
    results in significantly reduced compilation times.  Most, but not all,
    C++ compilers can handle the large files that result.  See also the
    :option:`--concatenate-split` option.

.. cmdoption:: --concatenate-split <N>

    If the :option:`--concatenate` option is used to concatenate the C++ source
    files then this option determines how many files are created.  The default
    is 1.

.. cmdoption:: --configuration <FILE>

    ``<FILE>`` contains the configuration of the PyQt5 build to be used instead
    of dynamically introspecting the system and is typically used when
    cross-compiling.  See :ref:`ref-configuration-files`.

.. cmdoption:: --confirm-license

    Using this confirms that you accept the terms of the PyQt5 license.  If it
    is omitted then you will be asked for confirmation during configuration.

.. cmdoption:: --dbus <DIR>

    The :file:`dbus-python.h` header file of the dbus-python package can be
    found in the directory ``<DIR>/dbus``.

.. cmdoption:: --debug

    The PyQt5 modules will be built with debugging symbols.  On Windows this
    requires that a debug version of Python is installed.

.. cmdoption:: --designer-plugindir <DIR>

    The Python plugin for Qt Designer will be installed in the directory
    ``<DIR>``.

.. cmdoption:: --destdir <DIR>

    The PyQt5 Python package will be installed in the directory ``<DIR>``.  The
    default is the Python installation's :file:`site-packages` directory.  If
    you use this option then the :envvar:`PYTHONPATH` environment variable must
    include ``<DIR>``.

.. cmdoption:: --disable <MODULE>

    .. versionadded:: 5.5.1

    Normally all PyQt5 modules are enabled and are built if the corresponding
    Qt library can be found.  This option will suppress the check for
    ``<MODULE>>``.  The option may be specified any number of times.

.. cmdoption:: --enable <MODULE>

    Normally all PyQt5 modules are enabled and are built if the corresponding
    Qt library can be found.  Using this option only those modules specifically
    enabled will be checked for and built.  The option may be specified any
    number of times.

.. cmdoption:: --license-dir <DIR>

    The license files needed by the commercial version of PyQt5 can be found in
    the directory ``<DIR>``.

.. cmdoption:: --no-designer-plugin

    The Qt Designer plugin will not be built.

.. cmdoption:: --no-docstrings

    The PyQt5 modules will not contain automatically generated docstrings.

.. cmdoption:: --no-python-dbus

    The Qt support for the standard Python DBus bindings is disabled.

.. cmdoption:: --no-qml-plugin

    The :program:`qmlscene` plugin will not be built.

.. cmdoption:: --no-qsci-api

    The :file:`PyQt5.api` QScintilla API file is not installed even if
    QScintilla does appear to be installed.

.. cmdoption:: --no-sip-files

    The ``.sip`` files for the PyQt5 modules will not be installed.

.. cmdoption:: --no-stubs

    .. versionadded:: 5.6

    The PEP 484 type hint stub files for the PyQt5 modules will not be
    installed.  This option is ignored (and the stub files are not installed)
    for versions of Python earlier than v3.5.

.. cmdoption:: --no-tools

    .. versionadded:: 5.3

    The ``pyuic5``, ``pyrcc5`` and ``pylupdate5`` tools will not be built.

.. cmdoption:: --no-timestamp

    Normally the header comments of each generated C/C++ source file includes
    a timestamp corresponding to when the file was generated.  This option
    suppresses the inclusion of the timestamp.

.. cmdoption:: --protected-is-public

    On certain platforms the size of PyQt5 modules can be significantly reduced
    by redefining the C++ ``protected`` keyword as ``public`` during
    compilation.  This option enables this behaviour and is the default on
    Linux and MacOS/X.

.. cmdoption:: --protected-not-public

    The default redefinition of ``protected`` to ``public`` during compilation
    on Linux and MacOS/X is disabled.

.. cmdoption:: --pyuic5-interpreter <FILE>

    ``<FILE>`` is the name of the Python interpreter used in the pyuic5
    wrapper.  The default is platform dependent.

.. cmdoption:: --qmake <FILE>

    Qt's :program:`qmake` program is used to determine how your Qt installation
    is laid out.  Normally :program:`qmake` is found on your :envvar:`PATH`.
    This option can be used to specify a particular instance of
    :program:`qmake` to use.

.. cmdoption:: --qml-plugindir <DIR>

    The Python plugin for :program:`qmlscene` will be installed in the
    directory ``<DIR>``.

.. cmdoption:: --qsci-api

    The :file:`PyQt5.api` QScintilla API file is installed even if QScintilla
    does not appear to be installed.  This option is implied if the
    :option:`--qsci-api-destdir` option is specified.

.. cmdoption:: --qsci-api-destdir <DIR>

    The QScintilla API file will be installed in the :file:`python`
    subdirectory of the :file:`api` subdirectory of the directory ``<DIR>``.

.. cmdoption:: --qtconf-prefix <DIR>

    .. versionadded:: 5.6

    A ``qt.conf`` file is embedded in the :mod:`PyQt5.QtCore` module with
    ``Prefix`` set to ``<DIR>`` which is assumed to be relative to the
    directory that the :mod:`PyQt5.QtCore` module will be installed in.

.. cmdoption:: --sip <FILE>

    The :program:`sip` program is used to generate PyQt5's C++ source code.
    Normally :program:`sip` is found on your :envvar:`PATH`.  This option can
    be used to specify a particular instance of :program:`sip` to use.

.. cmdoption:: --sip-incdir <DIR>

    The ``sip.h`` header file can be found in the directory ``<DIR>``.

.. cmdoption:: --sipdir <DIR>

    The ``.sip`` files for the PyQt5 modules will be installed in the directory
    ``<DIR>``.

.. cmdoption:: --spec <SPEC>

    The argument ``-spec SPEC`` will be passed to :program:`qmake`.  The
    default behaviour is platform specific.  On Windows :program:`configure.py`
    will choose the value that is correct for the version of Python that is
    being used.  (However if you have built Python yourself then you may need
    to explicitly specify ``<SPEC>``.)  On MacOS :program:`configure.py` will
    try and avoid ``macx-xcode`` if possible.)

.. cmdoption:: --static

    The PyQt5 modules will be built as static libraries.  This is useful when
    building a custom interpreter with the PyQt5 modules built in to the
    interpreter.

.. cmdoption:: --stubdir <DIR>

    .. versionadded:: 5.6

    The PEP 484 type hint stub files for the PyQt5 modules will be installed in
    the directory ``<DIR>``.  By default they will be stored in the same
    directory as the corresponding extension modules.  This option is ignored
    (and the stub files are not installed) for versions of Python earlier than
    v3.5.

.. cmdoption:: --sysroot <DIR>

    .. versionadded:: 5.3

    ``<DIR>`` is the name of an optional directory that replaces ``sys.prefix``
    in the names of other directories (specifically those specifying where the
    various PyQt5 components will be installed and where the Python include and
    library directories can be found).  It is typically used when
    cross-compiling or when building a static version of PyQt5.  See
    :ref:`ref-configuration-files`.

.. cmdoption:: --target-py-version <VERSION>

    .. versionadded:: 5.3

    ``<VERSION>`` is the major and minor version (e.g. ``3.4``) of the version
    of Python being targetted.  By default the version of Python being used to
    run the :program:`configure.py` script is used.  It is typically used when
    cross-compiling.  See :ref:`ref-configuration-files`.

.. cmdoption:: --trace

    The generated PyQt5 modules contain additional tracing code that is enabled
    using SIP's :func:`sip.settracemask` function.

.. cmdoption:: --verbose

    Compiler commands and any output issued during configuration is displayed
    instead of being suppressed.  Use this if :program:`configure.py` is having
    problems to see what exactly is going wrong.

.. cmdoption:: --version

    Display the version number and exit.

Any remaining command line arguments are expected to be in the form
``name=value`` or ``name+=value``.  Such arguments are added to any
:program:`qmake` ``.pro`` file created by :program:`configure.py`.


Building PyQt5
..............

The next step is to build PyQt5 by running your platform's :program:`make`
command.  For example::

    make

The final step is to install PyQt5 by running the following command::

    make install

(Depending on your system you may require root or administrator privileges.)

This will install the various PyQt5 components.


.. _ref-configuration-files:

Configuring with Configuration Files
....................................

The :program:`configure.py` script normally introspects the Python installation
of the interpreter running it in order to determine the names of the various
files and directories it needs.  This is fine for a native build of PyQt5 but
isn't appropriate when cross-compiling.  In this case it is possible to supply
a configuration file, specified using the :option:`--configuration` option,
which contains definitions of all the required values.

A configuration file is made up of a number of named sections each of which
contains a number of configuration items.  The format of a configuration file
is as follows:

- a section name is a single line with the name enclosed between ``[`` and
  ``]``

- a configuration item is a single line containing a name/value pair separated
  by ``=``

- values may be extended to lines immediately following if they are indented by
  at least one space

- a value may include another value by embedding the name of that value
  enclosed between ``%(`` and ``)``

- comments begin with ``#`` and continue to the end of the line

- blank lines are ignored.

Those configuration items that appear before the first section name are
automatically added to all sections.

A configuration file defines a section for each version of Qt that requires a
different configuration.  :program:`configure.py` will choose the most
appropriate section according to the version of Qt you are actually using.  For
example, if a configuration file contains sections for Qt v5.3 and Qt v5.1 and
you are using Qt v5.2.1 then the section for Qt v5.1 will be chosen.

:program:`configure.py` provides the following preset values for a
configuration:

``py_major``
    is the major version number of the target Python installation.

``py_minor``
    is the minor version number of the target Python installation.

``sysroot``
    is the name of the system root directory.  This is specified with the
    :option:`--sysroot` option.

The following is an example configuration file::

    # The target Python installation.
    py_platform = linux
    py_inc_dir = %(sysroot)/usr/include/python%(py_major).%(py_minor)
    py_pylib_dir = %(sysroot)/usr/lib/python%(py_major).%(py_minor)/config
    py_pylib_lib = python%(py_major).%(py_minor)mu

    # The target PyQt installation.
    pyqt_module_dir = %(sysroot)/usr/lib/python%(py_major)/dist-packages
    pyqt_bin_dir = %(sysroot)/usr/bin
    pyqt_sip_dir = %(sysroot)/usr/share/sip/PyQt5
    pyuic_interpreter = /usr/bin/python%(py_major).%(py_minor)
    pyqt_disabled_features = PyQt_Desktop_OpenGL PyQt_qreal_double

    # Qt configuration common to all versions.
    qt_shared = True

    [Qt 5.1]
    pyqt_modules = QtCore QtDBus QtDesigner QtGui QtHelp QtMultimedia
        QtMultimediaWidgets QtNetwork QtOpenGL QtPrintSupport QtQml QtQuick
        QtSensors QtSerialPort QtSql QtSvg QtTest QtWebKit QtWebKitWidgets
        QtWidgets QtXmlPatterns _QOpenGLFunctions_ES2

This example contains a section for Qt v5.1.  We have defined a number of
values before the start of the section as they are not specific to any
particular version of Qt.  Note that if you use this configuration with a
version of Qt earlier than v5.1 then you will get an error.

The following values can be specified in the configuration file:

``qt_shared``
    is set if Qt has been built as shared libraries.  The default value is
    ``False``.

``py_platform``
    is the target Python platform.

``py_inc_dir``
    is the target Python include directory, i.e. the directory containing the
    ``Python.h`` file.

``py_pylib_dir``
    is the target Python library directory.

``py_pylib_lib``
    is the target Python interpreter library.  It should not include any
    platform-specific prefix or suffix.

``pyqt_disabled_features``
    is the space separated list of features (as defined by SIP's ``%Feature``
    directive) that should be disabled.

``pyqt_module_dir``
    is the target directory where the PyQt5 modules will be installed.  It can
    be overridden by the :option:`--destdir` option.

``pyqt_modules``
    is the space separated list of PyQt5 modules that will be built.  It can be
    overridden by the :option:`--enable` option.

``pyqt_bin_dir``
    is the name of the target directory where the PyQt5 related executables
    will be installed.  It can be overridden by the :option:`--bindir` option.

``pyqt_sip_dir``
    is the name of the target directory where the PyQt5 ``.sip`` files will be
    installed.  It can be overridden by the :option:`--sipdir` option.

``pyuic_interpreter``
    is the name of the Python interpreter (as it would be called from the
    target system) that will be used to run :program:`pyuic5`.  It can be
    overridden by the :option:`--pyuic5-interpreter` option.


Co-existence with PyQt4
-----------------------

PyQt5 can be installed alongside PyQt4 using the same Python interpreter
without any problems so long as they are built with the same version of SIP.
