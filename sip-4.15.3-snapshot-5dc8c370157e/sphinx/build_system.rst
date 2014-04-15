.. _ref-build-system:

The Build System
================

.. module:: sipconfig

The purpose of the build system is to make it easy for you to write
configuration scripts in Python for your own bindings.  The build system takes
care of the details of particular combinations of platform and compiler.  It
supports over 50 different platform/compiler combinations.

The build system is implemented as a pure Python module called :mod:`sipconfig`
that contains a number of classes and functions.  Using this module you can
write bespoke configuration scripts (e.g. PyQt's ``configure.py``) or use it
with other Python based build systems (e.g.
`Distutils <http://www.python.org/sigs/distutils-sig/distutils.html>`_ and
`SCons <http://www.scons.org>`_).

An important feature of SIP is the ability to generate bindings that are built
on top of existing bindings.  For example, both
`PyKDE <http://www.riverbankcomputing.com/software/pykde/>`_ and
`PyQwt <http://pyqwt.sourceforge.net/>`_ are built on top of PyQt but all three
packages are maintained by different developers.  To make this easier PyQt
includes its own configuration module, ``pyqtconfig``, that contains additional
classes intended to be used by the configuration scripts of bindings built on
top of PyQt.  The SIP build system includes facilities that do a lot of the
work of creating these additional configuration modules.


.. function:: create_config_module(module, template, content[, macros=None])

    This creates a configuration module (e.g. ``pyqtconfig``) from a template
    file and a string.

    :param module:
        the name of the configuration module file to create.
    :param template:
        the name of the template file.
    :param content:
        a string which replaces every occurence of the pattern
        ``@SIP_CONFIGURATION@`` in the template file.  The content string is
        usually created from a Python dictionary using
        :func:`sipconfig.create_content()`.  *content* may also be a
        dictionary, in which case :func:`sipconfig.create_content()` is
        automatically called to convert it to a string.
    :param macros:
        an optional dictionary of platform specific build macros.  It is only
        used if :func:`sipconfig.create_content()` is called automatically to
        convert a *content* dictionary to a string.


.. function:: create_content(dict[, macros=None]) -> string

    This converts a Python dictionary to a string that can be parsed by the
    Python interpreter and converted back to an equivalent dictionary.  It is
    typically used to generate the content string for
    :func:`sipconfig.create_config_module()`.

    :param dict:
        the Python dictionary to convert.
    :param macros:
        the optional dictionary of platform specific build macros.
    :return:
        the string representation of the dictionary.


.. function:: create_wrapper(script, wrapper[, gui=0[, use_arch='']]) -> string

    This creates a platform dependent executable wrapper around a Python
    script.

    :param script:
        the full pathname of the script.
    :param wrapper:
        the full pathname of the wrapper to create, excluding any platform
        specific extension.
    :param gui:
        is non-zero if a GUI enabled version of the interpreter should be used
        on platforms that require it.
    :param use_arch:
        is the MacOS/X architecture to invoke python with.
    :return:
        the platform specific name of the wrapper.


.. function:: error(msg)

    This displays an error message on ``stderr`` and calls ``sys.exit(1)``.

    :param msg:
        the text of the message and should not include any newline characters.


.. function:: format(msg[, leftmargin=0[, rightmargin=78]]) -> string

    This formats a message by inserting newline characters at appropriate
    places.

    :param msg:
        the text of the message and should not include any newline characters.
    :param leftmargin:
        the optional position of the left margin.
    :param rightmargin:
        the optional position of the right margin.
    :return:
        the formatted message.


.. function:: inform(msg)

    This displays an information message on ``stdout``.

    :param msg:
        the text of the message and should not include any newline characters.


.. function:: parse_build_macros(filename, names[, overrides=None[, properties=None]]) -> dict

    This parses a ``qmake`` compatible file of build system macros and converts
    it to a dictionary.  A macro is a name/value pair.  Individual macros may
    be augmented or replaced.

    :param filename:
        the name of the file to parse.
    :param names:
        the list of the macro names to extract from the file.
    :param overrides:
        the optional list of macro names and values that modify those found in
        the file.  They are of the form ``name=value`` (in which case the value
        replaces the value found in the file) or ``name+=value`` (in which case
        the value is appended to the value found in the file).
    :param properties:
        the optional dictionary of property name and values that are used to
        resolve any expressions of the form ``$[name]`` in the file.
    :return:
        the dictionary of parsed macros or ``None`` if any of the overrides
        were invalid.


.. function:: read_version(filename, description[, numdefine=None[, strdefine=None]]) -> integer, string

    This extracts version information for a package from a file, usually a C or
    C++ header file.  The version information must each be specified as a
    ``#define`` of a numeric (hexadecimal or decimal) value and/or a string
    value.

    :param filename:
        the name of the file to read.
    :param description:
        a descriptive name of the package used in error messages.
    :param numdefine:
        the optional name of the ``#define`` of the version as a number.  If it
        is ``None`` then the numeric version is ignored.
    :param strdefine:
        the optional name of the ``#define`` of the version as a string.  If it
        is ``None`` then the string version is ignored.
    :return:
        a tuple of the numeric and string versions.  :func:`sipconfig.error()`
        is called if either were required but could not be found.


.. function:: version_to_sip_tag(version, tags, description) -> string

    This converts a version number to a SIP version tag.  SIP uses the
    :directive:`%Timeline` directive to define the chronology of the different
    versions of the C/C++ library being wrapped.  Typically it is not necessary
    to define a version tag for every version of the library, but only for
    those versions that affect the library's API as SIP sees it.

    :param version:
        the numeric version number of the C/C++ library being wrapped.  If it
        is negative then the latest version is assumed.  (This is typically
        useful if a snapshot is indicated by a negative version number.)
    :param tags:
        the dictionary of SIP version tags keyed by the corresponding C/C++
        library version number.  The tag used is the one with the smallest key
        (i.e. earliest version) that is greater than *version*.
    :param description:
        a descriptive name of the C/C++ library used in error messages.
    :return:
        the SIP version tag.  :func:`sipconfig.error()` is called if the C/C++
        library version number did not correspond to a SIP version tag.


.. function:: version_to_string(v) -> string

    This converts a 3 part version number encoded as a hexadecimal value to a
    string.

    :param v:
        the version number.
    :return:
        a string.


.. class:: Configuration

    This class encapsulates configuration values that can be accessed as
    instance objects.  A sub-class may provide a dictionary of additional
    configuration values in its constructor the elements of which will have
    precedence over the super-class's values.

    The following configuration values are provided:

    .. attribute:: default_bin_dir

        The name of the directory where executables should be installed by
        default.

    .. attribute:: default_mod_dir

        The name of the directory where SIP generated modules should be
        installed by default.

    .. attribute:: default_sip_dir

        The name of the base directory where the ``.sip`` files for SIP
        generated modules should be installed by default.  A sub-directory with
        the same name as the module should be created and its ``.sip`` files
        should be installed in the sub-directory.  The ``.sip`` files only need
        to be installed if you might want to build other bindings based on
        them.

    .. attribute:: platform

        The name of the platform/compiler for which the build system has been
        configured for.

    .. attribute:: py_conf_inc_dir

        The name of the directory containing the ``pyconfig.h`` header file.

    .. attribute:: py_inc_dir

        The name of the directory containing the ``Python.h`` header file.

    .. attribute:: py_lib_dir

        The name of the directory containing the Python interpreter library.

    .. attribute:: py_version

        The Python version as a 3 part hexadecimal number (e.g. v2.3.3 is
        represented as ``0x020303``).

    .. attribute:: sip_bin

        The full pathname of the SIP executable.

    .. attribute:: sip_config_args

        The command line passed to ``configure.py`` when SIP was configured.

    .. attribute:: sip_inc_dir

        The name of the directory containing the ``sip.h`` header file.

    .. attribute:: sip_mod_dir

        The name of the directory containing the SIP module.

    .. attribute:: sip_version

        The SIP version as a 3 part hexadecimal number (e.g. v4.0.0 is
        represented as ``0x040000``).

    .. attribute:: sip_version_str

        The SIP version as a string.  For development snapshots it will start
        with ``snapshot-``.

    .. attribute:: universal

        The name of the MacOS/X SDK used when creating universal binaries.

    .. attribute:: arch

        The space separated MacOS/X architectures to build.

    .. attribute:: deployment_target

        The MacOS/X deployment target.

    .. method:: __init__([sub_cfg=None])

        :param sub_cfg:
            an optional list of sub-class configurations.  It should only be
            used by the ``__init__()`` method of a sub-class to append its own
            dictionary of configuration values before passing the list to its
            super-class.

    .. method:: build_macros() -> dict

        Get the dictionary of platform specific build macros.

        :return:
            the macros dictionary.

    .. method:: set_build_macros(macros)

        Set the dictionary of platform specific build macros to be used when
        generating Makefiles.  Normally there is no need to change the default
        macros.

        :param macros:
            the macros dictionary.


.. class:: Makefile

    This class encapsulates a Makefile.  It is intended to be sub-classed to
    generate Makefiles for particular purposes.  It handles all platform and
    compiler specific flags, but allows them to be adjusted to suit the
    requirements of a particular module or program.  These are defined using a
    number of macros which can be accessed as instance attributes.

    The following instance attributes are provided to help in fine tuning the
    generated Makefile:

    .. attribute:: chkdir

        A string that will check for the existence of a directory.

    .. attribute:: config

        A reference to the *configuration* argument that was passed to
        :meth:`Makefile.__init__`.

    .. attribute:: console

        A reference to the *console* argument that was passed to the
        :meth:`Makefile.__init__`.

    .. attribute:: copy

        A string that will copy a file.

    .. attribute:: extra_cflags

        A list of additional flags passed to the C compiler.

    .. attribute:: extra_cxxflags

        A list of additional flags passed to the C++ compiler.

    .. attribute:: extra_defines

        A list of additional macro names passed to the C/C++ preprocessor.

    .. attribute:: extra_include_dirs

        A list of additional include directories passed to the C/C++
        preprocessor.

    .. attribute:: extra_lflags

        A list of additional flags passed to the linker.

    .. attribute:: extra_lib_dirs

        A list of additional library directories passed to the linker.

    .. attribute:: extra_libs

        A list of additional libraries passed to the linker.  The names of the
        libraries must be in platform neutral form (i.e. without any platform
        specific prefixes, version numbers or extensions).

    .. attribute:: generator

        A string that defines the platform specific style of Makefile.  The
        only supported values are ``UNIX``, ``MSVC``, ``MSVC.NET``, ``MINGW``
        and ``BMAKE``.

    .. attribute:: mkdir

        A string that will create a directory.

    .. attribute:: rm

        A string that will remove a file.

    .. method:: __init__(configuration[, console=0[, qt=0[, opengl=0[, python=0[, threaded=0[, warnings=None[, debug=0[, dir=None[, makefile="Makefile"[, installs=None[, universal=None[, arch=None[, deployment_target=None]]]]]]]]]]]]])

        :param configuration:
            the current configuration and is an instance of the
            :class:`Configuration` class or a sub-class.
        :param console:
            is set if the target is a console (rather than GUI) target.  This
            only affects Windows and is ignored on other platforms.
        :param qt:
            is set if the target uses Qt.  For Qt v4 a list of Qt libraries may
            be specified and a simple non-zero value implies QtCore and QtGui.
        :param opengl:
            is set if the target uses OpenGL.
        :param python:
            is set if the target uses Python.h.
        :param threaded:
            is set if the target requires thread support.  It is set
            automatically if the target uses Qt and Qt has thread support
            enabled.
        :param warnings:
            is set if compiler warning messages should be enabled.  The default
            of ``None`` means that warnings are enabled for SIP v4.x and
            disabled for SIP v3.x.
        :param debug:
            is set if debugging symbols should be generated.
        :param dir:
            the name of the directory where build files are read from (if they
            are not absolute file names) and Makefiles are written to.  The
            default of ``None`` means the current directory is used.
        :param makefile:
            the name of the generated Makefile.
        :param installs:
            the list of extra install targets.  Each element is a two part
            list, the first of which is the source and the second is the
            destination.  If the source is another list then it is a list of
            source files and the destination is a directory.
        :param universal:
            the name of the SDK if universal binaries are to be created under
            MacOS/X.  If it is ``None`` then the value is taken from the
            configuration.
        :param arch:
            the space separated MacOS/X architectures to build.  If it is
            ``None`` then the value is taken from the configuration.
        :param deployment_target:
            the MacOS/X deployment target.  If it is ``None`` then the value is
            taken from the configuration.

    .. method:: clean_build_file_objects(mfile, build)

        This generates the Makefile commands that will remove any files
        generated during the build of the default target.

        :param mfile:
            the Python file object of the Makefile.
        :param build:
            the dictionary created from parsing the build file.

    .. method:: finalise()

        This is called just before the Makefile is generated to ensure that it
        is fully configured.  It must be reimplemented by a sub-class.

    .. method:: generate()

        This generates the Makefile.

    .. method:: generate_macros_and_rules(mfile)

        This is the default implementation of the Makefile macros and rules
        generation.

        :param mfile:
            the Python file object of the Makefile.

    .. method:: generate_target_clean(mfile)

        This is the default implementation of the Makefile clean target
        generation.

        :param mfile:
            the Python file object of the Makefile.

    .. method:: generate_target_default(mfile)

        This is the default implementation of the Makefile default target
        generation.

        :param mfile:
            the Python file object of the Makefile.

    .. method:: generate_target_install(mfile)

        This is the default implementation of the Makefile install target
        generation.

        :param mfile:
            the Python file object of the Makefile.

    .. method:: install_file(mfile, src, dst[, strip=0])

        This generates the Makefile commands to install one or more files to a
        directory.

        :param mfile:
            the Python file object of the Makefile.
        :param src:
            the name of a single file to install or a list of a number of files
            to install.
        :param dst:
            the name of the destination directory.
        :param strip:
            is set if the files should be stripped of unneeded symbols after
            having been installed.

    .. method:: optional_list(name) -> list

        This returns an optional Makefile macro as a list.

        :param name:
            the name of the macro.
        :return:
            the macro as a list.

    .. method:: optional_string(name[, default=""])

        This returns an optional Makefile macro as a string.

        :param name:
            the name of the macro.
        :param default:
            the optional default value of the macro.
        :return:
            the macro as a string.

    .. method:: parse_build_file(filename) -> dict

        This parses a build file (created with the :option:`-b <sip -b>` SIP
        command line option) and converts it to a dictionary.  It can also
        validate an existing dictionary created through other means.

        :param filename: is the name of the build file, or is a dictionary to
            be validated.  A valid dictionary will contain the name of the
            target to build (excluding any platform specific extension) keyed
            by ``target``; the names of all source files keyed by ``sources``;
            and, optionally, the names of all header files keyed by
            ``headers``.
        :return:
            a dictionary corresponding to the parsed build file.

    .. method:: platform_lib(clib[, framework=0]) -> string

        This converts a library name to a platform specific form.

        :param clib:
            the name of the library in cannonical form.
        :param framework:
            is set if the library is implemented as a MacOS framework.
        :return:
            the platform specific name.

    .. method:: ready()

        This is called to ensure that the Makefile is fully configured.  It is
        normally called automatically when needed.

    .. method:: required_string(name) -> string

        This returns a required Makefile macro as a string.

        :param name:
            the name of the macro.
        :return:
            the macro as a string.  An exception is raised if the macro does
            not exist or has an empty value.


.. class:: ModuleMakefile

    This class is derived from :class:`sipconfig.Makefile`.

    This class encapsulates a Makefile to build a generic Python extension
    module.

    .. method:: __init__(self, configuration, build_file[, install_dir=None[, static=0[, console=0[, opengl=0[, threaded=0[, warnings=None[, debug=0[, dir=None[, makefile="Makefile"[, installs=None[, strip=1[, export_all=0[, universal=None[, arch=None[, deployment_target=None]]]]]]]]]]]]]]])

        :param configuration:
            see :meth:`sipconfig.Makefile.__init__`.
        :param build_file:
            the name of the build file.  Build files are generated using the
            :option:`-b <sip -b>` SIP command line option.
        :param install_dir:
            the name of the directory where the module will be optionally
            installed.
        :param static:
            is set if the module should be built as a static library (see
            :ref:`ref-builtin`).
        :param console:
            see :meth:`sipconfig.Makefile.__init__`.
        :param qt:
            see :meth:`sipconfig.Makefile.__init__`.
        :param opengl:
            see :meth:`sipconfig.Makefile.__init__`.
        :param threaded:
            see :meth:`sipconfig.Makefile.__init__`.
        :param warnings:
            see :meth:`sipconfig.Makefile.__init__`.
        :param debug:
            see :meth:`sipconfig.Makefile.__init__`.
        :param dir:
            see :meth:`sipconfig.Makefile.__init__`.
        :param makefile:
            see :meth:`sipconfig.Makefile.__init__`.
        :param installs:
            see :meth:`sipconfig.Makefile.__init__`.
        :param strip:
            is set if the module should be stripped of unneeded symbols after
            installation.  It is ignored if either *debug* or *static* is set,
            or if the platform doesn't support it.
        :param export_all:
            is set if all of the module's symbols should be exported rather
            than just the module's initialisation function.  Exporting all
            symbols increases the size of the module and slows down module load
            times but may avoid problems with modules that use C++ exceptions.
            All symbols are exported if either *debug* or *static* is set, or
            if the platform doesn't support it.
        :param universal:
            see :meth:`sipconfig.Makefile.__init__`.
        :param arch:
            see :meth:`sipconfig.Makefile.__init__`.
        :param deployment_target:
            see :meth:`sipconfig.Makefile.__init__`.

    .. method:: finalise()

        This is a reimplementation of :meth:`sipconfig.Makefile.finalise`.

    .. method:: generate_macros_and_rules(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_macros_and_rules`.

    .. method:: generate_target_clean(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_target_clean`.

    .. method:: generate_target_default(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_target_default`.

    .. method:: generate_target_install(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_target_install`.

    .. method:: module_as_lib(mname) -> string

        This gets the name of a SIP v3.x module for when it is used as a
        library to be linked against.  An exception will be raised if it is
        used with SIP v4.x modules.

        :param mname:
            the name of the module.
        :return:
            the corresponding library name.


.. class:: ParentMakefile

    This class is derived from :class:`sipconfig.Makefile`.

    This class encapsulates a Makefile that sits above a number of other
    Makefiles in sub-directories.

    .. method:: __init__(self, configuration, subdirs[, dir=None[, makefile[="Makefile"[, installs=None]]]])

        :param configuration:
            see :meth:`sipconfig.Makefile.__init__`.
        :param subdirs:
            the sequence of sub-directories.
        :param dir:
            see :meth:`sipconfig.Makefile.__init__`.
        :param makefile:
            see :meth:`sipconfig.Makefile.__init__`.
        :param installs:
            see :meth:`sipconfig.Makefile.__init__`.

    .. method:: generate_macros_and_rules(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_macros_and_rules`.

    .. method:: generate_target_clean(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_target_clean`.

    .. method:: generate_target_default(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_target_default`.

    .. method:: generate_target_install(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_target_install`.

.. class:: ProgramMakefile

    This class is derived from :class:`sipconfig.Makefile`.

    This class encapsulates a Makefile to build an executable program.

    .. method:: __init__(configuration[, build_file=None[, install_dir=None[, console=0[, qt=0[, opengl=0[, python=0[, threaded=0[, warnings=None[, debug=0[, dir=None[, makefile="Makefile"[, installs=None[, universal=None[, arch=None[,deployment_target=None]]]]]]]]]]]]]]])

        :param configuration:
            see :meth:`sipconfig.Makefile.__init__`.
        :param build_file:
            the name of the optional build file.  Build files are generated
            using the :option:`-b <sip -b>` SIP command line option.
        :param install_dir:
            the name of the directory where the executable program will be
            optionally installed.
        :param console:
            see :meth:`sipconfig.Makefile.__init__`.
        :param qt:
            see :meth:`sipconfig.Makefile.__init__`.
        :param opengl:
            see :meth:`sipconfig.Makefile.__init__`.
        :param python:
            see :meth:`sipconfig.Makefile.__init__`.
        :param threaded:
            see :meth:`sipconfig.Makefile.__init__`.
        :param warnings:
            see :meth:`sipconfig.Makefile.__init__`.
        :param debug:
            see :meth:`sipconfig.Makefile.__init__`.
        :param dir:
            see :meth:`sipconfig.Makefile.__init__`.
        :param makefile:
            see :meth:`sipconfig.Makefile.__init__`.
        :param installs:
            see :meth:`sipconfig.Makefile.__init__`.
        :param universal:
            see :meth:`sipconfig.Makefile.__init__`.
        :param arch:
            see :meth:`sipconfig.Makefile.__init__`.
        :param deployment_target:
            see :meth:`sipconfig.Makefile.__init__`.

    .. method:: build_command(source) -> string, string

        This creates a single command line that will create an executable
        program from a single source file.

        :param source:
            the name of the source file.
        :return:
            a tuple of the name of the executable that will be created and the
            command line.

    .. method:: finalise()

        This is a reimplementation of :meth:`sipconfig.Makefile.finalise`.

    .. method:: generate_macros_and_rules(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_macros_and_rules`.

    .. method:: generate_target_clean(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_target_clean`.

    .. method:: generate_target_default(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_target_default`.

    .. method:: generate_target_install(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_target_install`.


.. class:: PythonModuleMakefile

    This class is derived from :class:`sipconfig.Makefile`.

    This class encapsulates a Makefile that installs a pure Python module.

    .. method:: __init__(self, configuration, dstdir[, srcdir=None[, dir=None[, makefile="Makefile"[, installs=None]]]])

        :param configuration:
            see :meth:`sipconfig.Makefile.__init__`.
        :param dstdir:
            the name of the directory in which the module's Python code will be
            installed.
        :param srcdir:
            the name of the directory (relative to *dir*) containing the
            module's Python code.  It defaults to the same directory.
        :param dir:
            see :meth:`sipconfig.Makefile.__init__`.
        :param makefile:
            see :meth:`sipconfig.Makefile.__init__`.
        :param installs:
            see :meth:`sipconfig.Makefile.__init__`.

    .. method:: generate_macros_and_rules(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_macros_and_rules`.

    .. method:: generate_target_install(mfile)

        This is a reimplementation of
        :meth:`sipconfig.Makefile.generate_target_install`.


.. class:: SIPModuleMakefile

    This class is derived from :class:`sipconfig.ModuleMakefile`.

    This class encapsulates a Makefile to build a SIP generated Python
    extension module.

    .. method:: __init__(self, configuration, build_file[, install_dir=None[, static=0[, console=0[, opengl=0[, threaded=0[, warnings=None[, debug=0[, dir=None[, makefile="Makefile"[, installs=None[, strip=1[, export_all=0[, universal=None[, arch=None[, prot_is_public=0[, deployment_target=None]]]]]]]]]]]]]]]])

        :param configuration:
            see :meth:`sipconfig.Makefile.__init__`.
        :param build_file:
            see :meth:`sipconfig.ModuleMakefile.__init__`.
        :param install_dir:
            see :meth:`sipconfig.ModuleMakefile.__init__`.
        :param static:
            see :meth:`sipconfig.ModuleMakefile.__init__`.
        :param console:
            see :meth:`sipconfig.Makefile.__init__`.
        :param qt:
            see :meth:`sipconfig.Makefile.__init__`.
        :param opengl:
            see :meth:`sipconfig.Makefile.__init__`.
        :param threaded:
            see :meth:`sipconfig.Makefile.__init__`.
        :param warnings:
            see :meth:`sipconfig.Makefile.__init__`.
        :param debug:
            see :meth:`sipconfig.Makefile.__init__`.
        :param dir:
            see :meth:`sipconfig.Makefile.__init__`.
        :param makefile:
            see :meth:`sipconfig.Makefile.__init__`.
        :param installs:
            see :meth:`sipconfig.Makefile.__init__`.
        :param strip:
            see :meth:`sipconfig.ModuleMakefile.__init__`.
        :param export_all:
            see :meth:`sipconfig.ModuleMakefile.__init__`.
        :param universal:
            see :meth:`sipconfig.Makefile.__init__`.
        :param arch:
            see :meth:`sipconfig.Makefile.__init__`.
        :param prot_is_public:
            is set if ``protected`` should be redefined as ``public`` when
            compiling the generated module.
        :param deployment_target:
            see :meth:`sipconfig.Makefile.__init__`.

    .. method:: finalise()

        This is a reimplementation of :meth:`sipconfig.Makefile.finalise`.
