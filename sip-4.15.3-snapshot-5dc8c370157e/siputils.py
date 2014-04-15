# This module is intended to be used by the build/installation scripts of
# extension modules created with SIP.  It provides information about file
# locations, version numbers etc., and provides some classes and functions.
#
# Copyright (c) 2013 Riverbank Computing Limited <info@riverbankcomputing.com>
#
# This file is part of SIP.
#
# This copy of SIP is licensed for use under the terms of the SIP License
# Agreement.  See the file LICENSE for more details.
#
# This copy of SIP may also used under the terms of the GNU General Public
# License v2 or v3 as published by the Free Software Foundation which can be
# found in the files LICENSE-GPL2 and LICENSE-GPL3 included in this package.
#
# SIP is supplied WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


import sys
import os
import stat
import string
import re


# These are installation specific values created when SIP was configured.
# @SIP_CONFIGURATION@

# The stack of configuration dictionaries.
_config_stack = []


class Configuration(object):
    """The class that represents SIP configuration values.
    """
    def __init__(self, sub_cfg=None):
        """Initialise an instance of the class.

        sub_cfg is the list of sub-class configurations.  It should be None
        when called normally.
        """
        # Find the build macros in the closest imported module from where this
        # was originally defined.
        self._macros = None

        for cls in self.__class__.__mro__:
            if cls is object:
                continue

            mod = sys.modules[cls.__module__]

            if hasattr(mod, "_default_macros"):
                self._macros = mod._default_macros
                break

        if sub_cfg:
            cfg = sub_cfg
        else:
            cfg = []

        cfg.append(_pkg_config)

        global _config_stack
        _config_stack = cfg

    def __getattr__(self, name):
        """Allow configuration values and user options to be handled as
        instance variables.

        name is the name of the configuration value or user option.
        """
        for cfg in _config_stack:
            try:
                return cfg[name]
            except KeyError:
                pass

        raise AttributeError("\"%s\" is not a valid configuration value or user option" % name)

    def build_macros(self):
        """Return the dictionary of platform specific build macros.
        """
        return self._macros

    def set_build_macros(self, macros):
        """Set the dictionary of build macros to be use when generating
        Makefiles.

        macros is the dictionary of platform specific build macros.
        """
        self._macros = macros


class _UniqueList:
    """A limited list that ensures all its elements are unique.
    """
    def __init__(self, value=None):
        """Initialise the instance.

        value is the initial value of the list.
        """
        if value is None:
            self._list = []
        else:
            self._list = value

    def append(self, value):
        """Append a value to the list if it isn't already present.

        value is the value to append.
        """
        if value not in self._list:
            self._list.append(value)

    def lextend(self, value):
        """A normal list extend ignoring the uniqueness.

        value is the list of elements to append.
        """
        self._list.extend(value)

    def extend(self, value):
        """Append each element of a value to a list if it isn't already
        present.

        value is the list of elements to append.
        """
        for el in value:
            self.append(el)

    def as_list(self):
        """Return the list as a raw list.
        """
        return self._list


class _Macro:
    """A macro that can be manipulated as a list.
    """
    def __init__(self, name, value):
        """Initialise the instance.

        name is the name of the macro.
        value is the initial value of the macro.
        """
        self._name = name
        self.set(value)

    def set(self, value):
        """Explicitly set the value of the macro.

        value is the new value.  It may be a string, a list of strings or a
        _UniqueList instance.
        """
        self._macro = []

        if isinstance(value, _UniqueList):
            value = value.as_list()

        if type(value) == list:
            self.extend(value)
        else:
            self.append(value)

    def append(self, value):
        """Append a value to the macro.

        value is the value to append.
        """
        if value:
            self._macro.append(value)

    def extend(self, value):
        """Append each element of a value to the macro.

        value is the list of elements to append.
        """
        for el in value:
            self.append(el)

    def remove(self, value): 
        """Remove a value from the macro.  It doesn't matter if the value 
        wasn't present. 
 
        value is the value to remove. 
        """ 
        try: 
            self._macro.remove(value) 
        except: 
            pass 

    def as_list(self):
        """Return the macro as a list.
        """
        return self._macro


class Makefile:
    """The base class for the different types of Makefiles.
    """
    def __init__(self, configuration, console=0, qt=0, opengl=0, python=0,
                 threaded=0, warnings=1, debug=0, dir=None,
                 makefile="Makefile", installs=None, universal=None,
                 arch=None, deployment_target=None):
        """Initialise an instance of the target.  All the macros are left
        unchanged allowing scripts to manipulate them at will.

        configuration is the current configuration.
        console is set if the target is a console (rather than windows) target.
        qt is set if the target uses Qt.  For Qt v4 a list of Qt libraries may
        be specified and a simple non-zero value implies QtCore and QtGui.
        opengl is set if the target uses OpenGL.
        python is set if the target #includes Python.h.
        debug is set to generated a debugging version of the target.
        threaded is set if the target requires thread support.  It is
        automatically set if the target uses Qt and Qt has thread support
        enabled.
        warnings is set if compiler warning messages are required.
        debug is set if debugging symbols should be generated.
        dir is the directory for build files and Makefiles.
        makefile is the name of the Makefile.
        installs is a list of extra install targets.  Each element is a two
        part list, the first of which is the source and the second is the
        destination.  If the source is another list then it is a set of source
        files and the destination is a directory.
        universal is the name of the SDK if the target is a MacOS/X universal
        binary.  If it is None then the value is taken from the configuration.
        arch is the space separated MacOS/X architectures to build.  If it is
        None then it is taken from the configuration.
        deployment_target MacOS/X deployment target.  If it is None then it is
        taken from the configuration.
        """
        if qt:
            if not hasattr(configuration, "qt_version"):
                error("The target uses Qt but pyqtconfig has not been imported.")

            # For Qt v4 interpret Qt support as meaning link against the core
            # and GUI libraries (which corresponds to the default qmake
            # configuration).  Also allow a list of Qt v4 modules to be
            # specified.
            if configuration.qt_version >= 0x040000:
                if type(qt) != list:
                    qt = ["QtCore", "QtGui"]

            self._threaded = configuration.qt_threaded
        else:
            self._threaded = threaded

        self.config = configuration
        self.console = console
        self._qt = qt
        self._opengl = opengl
        self._python = python
        self._warnings = warnings
        self._debug = debug
        self._makefile = makefile
        self._installs = installs
        self._infix = ""

        # Make sure the destination directory is an absolute path.
        if dir:
            self.dir = os.path.abspath(dir)
        else:
            self.dir = os.path.curdir

        # Assume we are building in the source tree.
        self._src_dir = self.dir

        if universal is None:
            self._universal = configuration.universal
        else:
            self._universal = universal

        if arch is None:
            self._arch = configuration.arch
        else:
            self._arch = arch

        if deployment_target is None:
            self._deployment_target = configuration.deployment_target
        else:
            self._deployment_target = deployment_target

        self._finalised = 0

        # Copy the macros and convert them all to instance lists.
        macros = configuration.build_macros()

        for m in list(macros.keys()):
            # Allow the user to override the default.
            try:
                val = getattr(configuration, m)
            except AttributeError:
                val = macros[m]

            # These require special handling as they are (potentially) a set of
            # space separated values rather than a single value that might
            # contain spaces.
            if m in ("DEFINES", "CONFIG") or m[:6] in ("INCDIR", "LIBDIR"):
                val = val.split()

            # We also want to treat lists of libraries in the same way so that
            # duplicates get eliminated.
            if m[:4] == "LIBS":
                val = val.split()

            self.__dict__[m] = _Macro(m, val)

        # This is used to alter the configuration more significantly than can
        # be done with just configuration files.
        self.generator = self.optional_string("MAKEFILE_GENERATOR", "UNIX")

        # These are what configuration scripts normally only need to change.
        self.extra_cflags = []
        self.extra_cxxflags = []
        self.extra_defines = []
        self.extra_include_dirs = []
        self.extra_lflags = []
        self.extra_lib_dirs = []
        self.extra_libs = []

        # Get these once and make them available to sub-classes.
        if sys.platform == "win32":
            def_copy = "copy"
            def_rm = "del"
            def_mkdir = "mkdir"
            def_chk_dir_exists = "if not exist"
        else:
            def_copy = "cp -f"
            def_rm = "rm -f"
            def_mkdir = "mkdir -p"
            def_chk_dir_exists = "test -d"

        self.copy = self.optional_string("COPY", def_copy)
        self.rm = self.optional_string("DEL_FILE", def_rm)
        self.mkdir = self.optional_string("MKDIR", def_mkdir)
        self.chkdir = self.optional_string("CHK_DIR_EXISTS", def_chk_dir_exists)


    def finalise(self):
        """Finalise the macros by doing any consolidation that isn't specific
        to a Makefile.
        """
        # Extract the things we might need from the Windows Qt configuration.
        # Note that we used to think that if Qt was built with exceptions, RTTI
        # and STL support enabled then anything that linked against it also
        # needed the same flags.  However, detecting this was broken for some
        # time and nobody complained.  For the moment we'll leave the code in
        # but it will never be used.
        if self._qt:
            wcfg = self.config.qt_winconfig.split()
            win_shared = ("shared" in wcfg)
            win_exceptions = ("exceptions" in wcfg)
            win_rtti = ("rtti" in wcfg)
            win_stl = ("stl" in wcfg)

            qt_version = self.config.qt_version
        else:
            win_shared = 1
            win_exceptions = 0
            win_rtti = 0
            win_stl = 0

            qt_version = 0

        # Get what we are going to transform.
        cflags = _UniqueList()
        cflags.extend(self.extra_cflags)
        cflags.extend(self.optional_list("CFLAGS"))

        cxxflags = _UniqueList()
        cxxflags.extend(self.extra_cxxflags)
        cxxflags.extend(self.optional_list("CXXFLAGS"))

        defines = _UniqueList()
        defines.extend(self.extra_defines)
        defines.extend(self.optional_list("DEFINES"))

        incdir = _UniqueList(["."])
        incdir.extend(self.extra_include_dirs)
        incdir.extend(self.optional_list("INCDIR"))

        lflags = _UniqueList()
        lflags.extend(self.extra_lflags)
        lflags.extend(self.optional_list("LFLAGS"))

        libdir = _UniqueList()
        libdir.extend(self.extra_lib_dirs)
        libdir.extend(self.optional_list("LIBDIR"))

        # Handle MacOS/X specific configuration.
        if sys.platform == 'darwin':
            mac_cflags = []
            mac_lflags = []

            for a in self._arch.split():
                aflag = '-arch ' + a
                mac_cflags.append(aflag)
                mac_lflags.append(aflag)

            if self._universal:
                mac_cflags.append('-isysroot %s' % self._universal)
                mac_lflags.append('-Wl,-syslibroot,%s' % self._universal)

            cflags.lextend(mac_cflags)
            cxxflags.lextend(mac_cflags)
            lflags.lextend(mac_lflags)

        # Don't use a unique list as libraries may need to be searched more
        # than once.  Also MacOS/X uses the form "-framework lib" so we don't
        # want to lose the multiple "-framework".
        libs = []

        for l in self.extra_libs:
            libs.append(self.platform_lib(l))

            if self._qt:
                libs.extend(self._dependent_libs(l))

        libs.extend(self.optional_list("LIBS"))

        rpaths = _UniqueList()

        for l in self.extra_lib_dirs:
            l_dir = os.path.dirname(l)

            # This is a hack to ignore PyQt's internal support libraries.
            if '/qpy/' in l_dir:
                continue

            # Ignore relative directories.  This is really a hack to handle
            # SIP v3 inter-module linking.
            if l_dir in ("", ".", ".."):
                continue

            rpaths.append(l)

        if self._python:
            incdir.append(self.config.py_inc_dir)
            incdir.append(self.config.py_conf_inc_dir)

            if sys.platform == "cygwin":
                libdir.append(self.config.py_lib_dir)

                py_lib = "python%u.%u" % ((self.config.py_version >> 16), ((self.config.py_version >> 8) & 0xff))
                libs.append(self.platform_lib(py_lib))
            elif sys.platform == "win32":
                libdir.append(self.config.py_lib_dir)

                py_lib = "python%u%u" % ((self.config.py_version >> 16), ((self.config.py_version >> 8) & 0xff))

                # For Borland use the OMF version of the Python library if it
                # exists, otherwise assume that Python was built with Borland
                # and use the normal library.
                if self.generator == "BMAKE":
                    bpy_lib = py_lib + "_bcpp"
                    bpy_lib_path = os.path.join(self.config.py_lib_dir, self.platform_lib(bpy_lib))

                    if os.access(bpy_lib_path, os.F_OK):
                        py_lib = bpy_lib

                if self._debug:
                    py_lib = py_lib + "_d"

                    if self.generator != "MINGW":
                        cflags.append("/D_DEBUG")
                        cxxflags.append("/D_DEBUG")

                libs.append(self.platform_lib(py_lib))

        if self.generator in ("MSVC", "MSVC.NET", "MSBUILD", "BMAKE"):
            if win_exceptions:
                cflags_exceptions = "CFLAGS_EXCEPTIONS_ON"
                cxxflags_exceptions = "CXXFLAGS_EXCEPTIONS_ON"
            else:
                cflags_exceptions = "CFLAGS_EXCEPTIONS_OFF"
                cxxflags_exceptions = "CXXFLAGS_EXCEPTIONS_OFF"

            cflags.extend(self.optional_list(cflags_exceptions))
            cxxflags.extend(self.optional_list(cxxflags_exceptions))

            if win_rtti:
                cflags_rtti = "CFLAGS_RTTI_ON"
                cxxflags_rtti = "CXXFLAGS_RTTI_ON"
            else:
                cflags_rtti = "CFLAGS_RTTI_OFF"
                cxxflags_rtti = "CXXFLAGS_RTTI_OFF"

            cflags.extend(self.optional_list(cflags_rtti))
            cxxflags.extend(self.optional_list(cxxflags_rtti))

            if win_stl:
                cflags_stl = "CFLAGS_STL_ON"
                cxxflags_stl = "CXXFLAGS_STL_ON"
            else:
                cflags_stl = "CFLAGS_STL_OFF"
                cxxflags_stl = "CXXFLAGS_STL_OFF"

            cflags.extend(self.optional_list(cflags_stl))
            cxxflags.extend(self.optional_list(cxxflags_stl))

        if self._debug:
            if win_shared:
                cflags_mt = "CFLAGS_MT_DLLDBG"
                cxxflags_mt = "CXXFLAGS_MT_DLLDBG"
            else:
                cflags_mt = "CFLAGS_MT_DBG"
                cxxflags_mt = "CXXFLAGS_MT_DBG"

            cflags_debug = "CFLAGS_DEBUG"
            cxxflags_debug = "CXXFLAGS_DEBUG"
            lflags_debug = "LFLAGS_DEBUG"
        else:
            if win_shared:
                cflags_mt = "CFLAGS_MT_DLL"
                cxxflags_mt = "CXXFLAGS_MT_DLL"
            else:
                cflags_mt = "CFLAGS_MT"
                cxxflags_mt = "CXXFLAGS_MT"

            cflags_debug = "CFLAGS_RELEASE"
            cxxflags_debug = "CXXFLAGS_RELEASE"
            lflags_debug = "LFLAGS_RELEASE"

        if self.generator in ("MSVC", "MSVC.NET", "MSBUILD", "BMAKE"):
            if self._threaded:
                cflags.extend(self.optional_list(cflags_mt))
                cxxflags.extend(self.optional_list(cxxflags_mt))

            if self.console:
                cflags.extend(self.optional_list("CFLAGS_CONSOLE"))
                cxxflags.extend(self.optional_list("CXXFLAGS_CONSOLE"))

        cflags.extend(self.optional_list(cflags_debug))
        cxxflags.extend(self.optional_list(cxxflags_debug))
        lflags.extend(self.optional_list(lflags_debug))

        if self._warnings:
            cflags_warn = "CFLAGS_WARN_ON"
            cxxflags_warn = "CXXFLAGS_WARN_ON"
        else:
            cflags_warn = "CFLAGS_WARN_OFF"
            cxxflags_warn = "CXXFLAGS_WARN_OFF"

        cflags.extend(self.optional_list(cflags_warn))
        cxxflags.extend(self.optional_list(cxxflags_warn))

        if self._threaded:
            cflags.extend(self.optional_list("CFLAGS_THREAD"))
            cxxflags.extend(self.optional_list("CXXFLAGS_THREAD"))
            lflags.extend(self.optional_list("LFLAGS_THREAD"))

        if self._qt:
            # Get the name of the mkspecs directory.
            try:
                specd_base = self.config.qt_data_dir
            except AttributeError:
                specd_base = self.config.qt_dir

            mkspecs = os.path.join(specd_base, "mkspecs")

            if self.generator != "UNIX" and win_shared:
                defines.append("QT_DLL")

            if not self._debug:
                defines.append("QT_NO_DEBUG")

            if qt_version >= 0x040000:
                for mod in self._qt:
                    # Note that qmake doesn't define anything for QtHelp.
                    if mod == "QtCore":
                        defines.append("QT_CORE_LIB")
                    elif mod == "QtDeclarative":
                        defines.append("QT_DECLARATIVE_LIB")
                    elif mod == "QtGui":
                        defines.append("QT_GUI_LIB")
                    elif mod == "QtMultimedia":
                        defines.append("QT_MULTIMEDIA_LIB")
                    elif mod == "QtNetwork":
                        defines.append("QT_NETWORK_LIB")
                    elif mod == "QtOpenGL":
                        defines.append("QT_OPENGL_LIB")
                    elif mod == "QtScript":
                        defines.append("QT_SCRIPT_LIB")
                    elif mod == "QtScriptTools":
                        defines.append("QT_SCRIPTTOOLS_LIB")
                    elif mod == "QtSql":
                        defines.append("QT_SQL_LIB")
                    elif mod == "QtTest":
                        defines.append("QT_TEST_LIB")
                    elif mod == "QtWebKit":
                        defines.append("QT_WEBKIT_LIB")
                    elif mod == "QtXml":
                        defines.append("QT_XML_LIB")
                    elif mod == "QtXmlPatterns":
                        defines.append("QT_XMLPATTERNS_LIB")
                    elif mod == "phonon":
                        defines.append("QT_PHONON_LIB")

                    if qt_version >= 0x050000:
                        if mod == "QtTest":
                            defines.append("QT_GUI_LIB")

                        if mod in ("QtSql", "QtTest"):
                            defines.append("QT_WIDGETS_LIB")

            elif self._threaded:
                defines.append("QT_THREAD_SUPPORT")

            # Handle library directories.
            libdir_qt = self.optional_list("LIBDIR_QT")
            libdir.extend(libdir_qt)
            rpaths.extend(libdir_qt)

            if qt_version >= 0x040000:
                # Try and read QT_LIBINFIX from qconfig.pri.
                qconfig = os.path.join(mkspecs, "qconfig.pri")
                self._infix = self._extract_value(qconfig, "QT_LIBINFIX")

                # For Windows: the macros that define the dependencies on
                # Windows libraries.
                wdepmap = {
                    "QtCore":       "LIBS_CORE",
                    "QtGui":        "LIBS_GUI",
                    "QtNetwork":    "LIBS_NETWORK",
                    "QtOpenGL":     "LIBS_OPENGL",
                    "QtWebKit":     "LIBS_WEBKIT"
                }

                # For Windows: the dependencies between Qt libraries.
                qt5_depmap = {
                    "QtDeclarative":    ("QtXmlPatterns", "QtNetwork", "QtSql", "QtScript", "QtWidgets", "QtGui", "QtCore"),
                    "QtGui":            ("QtPrintSupport", "QtWidgets", "QtCore"),
                    "QtHelp":           ("QtNetwork", "QtSql", "QtWidgets", "QtGui", "QtCore"),
                    "QtMultimedia":     ("QtGui", "QtCore"),
                    "QtNetwork":        ("QtCore", ),
                    "QtOpenGL":         ("QtWidgets", "QtGui", "QtCore"),
                    "QtScript":         ("QtCore", ),
                    "QtScriptTools":    ("QtScript", "QtGui", "QtCore"),
                    "QtSql":            ("QtCore", ),
                    "QtSvg":            ("QtXml", "QtWidgets", "QtGui", "QtCore"),
                    "QtTest":           ("QtGui", "QtCore"),
                    "QtWebKit":         ("QtNetwork", "QtWebKitWidgets", "QtWidgets", "QtGui", "QtCore"),
                    "QtXml":            ("QtCore", ),
                    "QtXmlPatterns":    ("QtNetwork", "QtCore"),
                    "QtDesigner":       ("QtGui", "QtCore"),
                    "QAxContainer":     ("Qt5AxBase", "QtWidgets", "QtGui", "QtCore")
                }

                qt4_depmap = {
                    "QtAssistant":      ("QtNetwork", "QtGui", "QtCore"),
                    "QtDeclarative":    ("QtNetwork", "QtGui", "QtCore"),
                    "QtGui":            ("QtCore", ),
                    "QtHelp":           ("QtSql", "QtGui", "QtCore"),
                    "QtMultimedia":     ("QtGui", "QtCore"),
                    "QtNetwork":        ("QtCore", ),
                    "QtOpenGL":         ("QtGui", "QtCore"),
                    "QtScript":         ("QtCore", ),
                    "QtScriptTools":    ("QtScript", "QtGui", "QtCore"),
                    "QtSql":            ("QtCore", ),
                    "QtSvg":            ("QtXml", "QtGui", "QtCore"),
                    "QtTest":           ("QtGui", "QtCore"),
                    "QtWebKit":         ("QtNetwork", "QtGui", "QtCore"),
                    "QtXml":            ("QtCore", ),
                    "QtXmlPatterns":    ("QtNetwork", "QtCore"),
                    "phonon":           ("QtGui", "QtCore"),
                    "QtDesigner":       ("QtGui", "QtCore"),
                    "QAxContainer":     ("QtGui", "QtCore")
                }

                if qt_version >= 0x050000:
                    qt_depmap = qt5_depmap
                else:
                    qt_depmap = qt4_depmap

                # The QtSql .prl file doesn't include QtGui as a dependency (at
                # least on Linux) so we explcitly set the dependency here for
                # everything.
                if "QtSql" in self._qt:
                    if "QtGui" not in self._qt:
                        self._qt.append("QtGui")

                # With Qt v4.2.0, the QtAssistantClient library is now a shared
                # library on UNIX. The QtAssistantClient .prl file doesn't
                # include QtGui and QtNetwork as a dependency any longer.  This
                # seems to be a bug in Qt v4.2.0.  We explicitly set the
                # dependencies here.
                if qt_version >= 0x040200 and "QtAssistant" in self._qt:
                    if "QtGui" not in self._qt:
                        self._qt.append("QtGui")
                    if "QtNetwork" not in self._qt:
                        self._qt.append("QtNetwork")

                for mod in self._qt:
                    lib = self._qt_module_to_lib(mod)
                    libs.append(self.platform_lib(lib, self._is_framework(mod)))

                    if sys.platform == "win32":
                        # On Windows the dependent libraries seem to be in
                        # qmake.conf rather than the .prl file and the
                        # inter-dependencies between Qt libraries don't seem to
                        # be anywhere.
                        deps = _UniqueList()

                        if mod in list(wdepmap.keys()):
                            deps.extend(self.optional_list(wdepmap[mod]))

                        if mod in list(qt_depmap.keys()):
                            for qdep in qt_depmap[mod]:
                                # Ignore the dependency if it is explicitly
                                # linked.
                                if qdep not in self._qt:
                                    libs.append(self.platform_lib(self._qt_module_to_lib(qdep)))

                                    if qdep in list(wdepmap.keys()):
                                        deps.extend(self.optional_list(wdepmap[qdep]))

                        libs.extend(deps.as_list())
                    else:
                        libs.extend(self._dependent_libs(lib, self._is_framework(mod)))
            else:
                # Windows needs the version number appended if Qt is a DLL.
                qt_lib = self.config.qt_lib

                if self.generator in ("MSVC", "MSVC.NET", "MSBUILD", "BMAKE") and win_shared:
                    qt_lib = qt_lib + version_to_string(qt_version).replace(".", "")

                    if self.config.qt_edition == "non-commercial":
                        qt_lib = qt_lib + "nc"

                libs.append(self.platform_lib(qt_lib, self.config.qt_framework))
                libs.extend(self._dependent_libs(self.config.qt_lib))

            # Handle header directories.
            specd = os.path.join(mkspecs, "default")

            if not os.access(specd, os.F_OK):
                specd = os.path.join(mkspecs, self.config.platform)

            incdir.append(specd)

            qtincdir = self.optional_list("INCDIR_QT")

            if qtincdir:
                if qt_version >= 0x040000:
                    for mod in self._qt:
                        if mod == "QAxContainer":
                            incdir.append(os.path.join(qtincdir[0], "ActiveQt"))
                        elif self._is_framework(mod):
                            idir = libdir_qt[0]

                            if mod == "QtAssistant" and qt_version < 0x040202:
                                mod = "QtAssistantClient"

                            incdir.append(os.path.join(idir,
                                    mod + ".framework", "Headers"))

                            if qt_version >= 0x050000:
                                if mod == "QtGui":
                                    incdir.append(os.path.join(idir,
                                            "QtWidgets.framework", "Headers"))
                                    incdir.append(os.path.join(idir,
                                            "QtPrintSupport.framework",
                                            "Headers"))
                                elif mod == "QtWebKit":
                                    incdir.append(os.path.join(idir,
                                            "QtWebKitWidgets.framework",
                                            "Headers"))
                        else:
                            idir = qtincdir[0]

                            incdir.append(os.path.join(idir, mod))

                            if qt_version >= 0x050000:
                                if mod == "QtGui":
                                    incdir.append(os.path.join(idir,
                                            "QtWidgets"))
                                    incdir.append(os.path.join(idir,
                                            "QtPrintSupport"))
                                elif mod == "QtWebKit":
                                    incdir.append(os.path.join(idir,
                                            "QtWebKitWidgets"))

                # This must go after the module include directories.
                incdir.extend(qtincdir)

        if self._opengl:
            incdir.extend(self.optional_list("INCDIR_OPENGL"))
            lflags.extend(self.optional_list("LFLAGS_OPENGL"))
            libdir.extend(self.optional_list("LIBDIR_OPENGL"))
            libs.extend(self.optional_list("LIBS_OPENGL"))

        if self._qt or self._opengl:
            if qt_version < 0x040000 or self._opengl or "QtGui" in self._qt:
                incdir.extend(self.optional_list("INCDIR_X11"))
                libdir.extend(self.optional_list("LIBDIR_X11"))
                libs.extend(self.optional_list("LIBS_X11"))

        if self._threaded:
            libs.extend(self.optional_list("LIBS_THREAD"))
            libs.extend(self.optional_list("LIBS_RTMT"))
        else:
            libs.extend(self.optional_list("LIBS_RT"))

        if self.console:
            libs.extend(self.optional_list("LIBS_CONSOLE"))

        libs.extend(self.optional_list("LIBS_WINDOWS"))

        lflags.extend(self._platform_rpaths(rpaths.as_list()))

        # Save the transformed values.
        self.CFLAGS.set(cflags)
        self.CXXFLAGS.set(cxxflags)
        self.DEFINES.set(defines)
        self.INCDIR.set(incdir)
        self.LFLAGS.set(lflags)
        self.LIBDIR.set(libdir)
        self.LIBS.set(libs)

        # Don't do it again because it has side effects.
        self._finalised = 1

    def _add_manifest(self, target=None):
        """Add the link flags for creating a manifest file.
        """
        if target is None:
            target = "$(TARGET)"

        self.LFLAGS.append("/MANIFEST")
        self.LFLAGS.append("/MANIFESTFILE:%s.manifest" % target)

    def _is_framework(self, mod):
        """Return true if the given Qt module is a framework.
        """
        return (self.config.qt_framework and (self.config.qt_version >= 0x040200 or mod != "QtAssistant"))

    def _qt_module_to_lib(self, mname):
        """Return the name of the Qt library corresponding to a module.

        mname is the name of the module.
        """
        qt_version = self.config.qt_version

        if mname == "QtAssistant":
            if qt_version >= 0x040202 and sys.platform == "darwin":
                lib = mname
            else:
                lib = "QtAssistantClient"
        else:
            lib = mname

        lib += self._infix

        if self._debug:
            if sys.platform == "win32":
                lib = lib + "d"
            elif sys.platform == "darwin":
                if not self._is_framework(mname):
                    lib = lib + "_debug"
            elif qt_version < 0x040200:
                lib = lib + "_debug"

        qt5_rename = False

        if sys.platform == "win32" and "shared" in self.config.qt_winconfig.split():
            if (mname in ("QtCore", "QtDeclarative", "QtDesigner", "QtGui",
                          "QtHelp", "QtMultimedia", "QtNetwork", "QtOpenGL",
                          "QtScript", "QtScriptTools", "QtSql", "QtSvg",
                          "QtTest", "QtWebKit", "QtXml", "QtXmlPatterns",
                          "phonon", "QAxContainer", "QtPrintSupport",
                          "QtWebKitWidgets", "QtWidgets") or
                (qt_version >= 0x040200 and mname == "QtAssistant")):
                if mname == "QAxContainer":
                    if qt_version >= 0x050000:
                        lib = "Qt5" + lib[1:]
                elif qt_version >= 0x050000:
                    qt5_rename = True
                else:
                    lib = lib + "4"
        elif sys.platform.startswith("linux") and qt_version >= 0x050000:
            qt5_rename = True

        if qt5_rename:
            lib = "Qt5" + lib[2:]

        return lib

    def optional_list(self, name):
        """Return an optional Makefile macro as a list.

        name is the name of the macro.
        """
        return self.__dict__[name].as_list()

    def optional_string(self, name, default=""):
        """Return an optional Makefile macro as a string.

        name is the name of the macro.
        default is the default value
        """
        s = ' '.join(self.optional_list(name))

        if not s:
            s = default

        return s

    def required_string(self, name):
        """Return a required Makefile macro as a string.

        name is the name of the macro.
        """
        s = self.optional_string(name)

        if not s:
            raise ValueError("\"%s\" must have a non-empty value" % name)

        return s

    def _platform_rpaths(self, rpaths):
        """Return a list of platform specific rpath flags.

        rpaths is the cannonical list of rpaths.
        """
        flags = []
        prefix = self.optional_string("RPATH")

        if prefix == "":
            # This was renamed in Qt v4.7.
            prefix = self.optional_string("LFLAGS_RPATH")

        if prefix != "":
            for r in rpaths:
                flags.append(_quote(prefix + r))

        return flags

    def platform_lib(self, clib, framework=0):
        """Return a library name in platform specific form.

        clib is the library name in cannonical form.
        framework is set of the library is implemented as a MacOS framework.
        """
        if self.generator in ("MSVC", "MSVC.NET", "MSBUILD", "BMAKE"):
            plib = clib + ".lib"
        elif sys.platform == "darwin" and framework:
            plib = "-framework " + clib
        else:
            plib = "-l" + clib

        return plib

    def _dependent_libs(self, clib, framework=0):
        """Return a list of additional libraries (in platform specific form)
        that must be linked with a library.

        clib is the library name in cannonical form.
        framework is set of the library is implemented as a MacOS framework.
        """
        if self.generator in ("MSVC", "MSVC.NET", "MSBUILD", "BMAKE"):
            prl_name = os.path.join(self.config.qt_lib_dir, clib + ".prl")
        elif sys.platform == "darwin" and framework:
            prl_name = os.path.join(self.config.qt_lib_dir, clib + ".framework", clib + ".prl")
        else:
            prl_name = os.path.join(self.config.qt_lib_dir, "lib" + clib + ".prl")

        libs = self._extract_value(prl_name, "QMAKE_PRL_LIBS").split()

        if self.config.qt_version >= 0x050000:
            xtra_libs = []

            if clib in ("QtGui", "Qt5Gui"):
                xtra_libs.append("QtWidgets")
                xtra_libs.append("QtPrintSupport")
            elif clib in ("QtWebKit", "Qt5WebKit"):
                xtra_libs.append("QtWebKitWidgets")

            for xtra in xtra_libs:
                libs.extend(
                        self.platform_lib(
                                self._qt_module_to_lib(xtra), framework).split())

        return libs

    def _extract_value(self, fname, vname):
        """Return the stripped value from a name=value line in a file.

        fname is the name of the file.
        vname is the name of the value.
        """
        value = ""

        if os.access(fname, os.F_OK):
            try:
                f = open(fname, "r")
            except IOError:
                error("Unable to open \"%s\"" % fname)

            line = f.readline()
            while line:
                line = line.strip()
                if line and line[0] != "#":
                    eq = line.find("=")
                    if eq > 0 and line[:eq].strip() == vname:
                        value = line[eq + 1:].strip()
                        break

                line = f.readline()

            f.close()

        return value

    def parse_build_file(self, filename):
        """
        Parse a build file and return the corresponding dictionary.

        filename is the name of the build file.  If it is a dictionary instead
        then its contents are validated.
        """
        if type(filename) == dict:
            bfname = "dictionary"
            bdict = filename
        else:
            if os.path.isabs(filename):
                # We appear to be building out of the source tree.
                self._src_dir = os.path.dirname(filename)
                bfname = filename
            else:
                bfname = os.path.join(self.dir, filename)

            bdict = {}

            try:
                f = open(bfname, "r")
            except IOError:
                error("Unable to open \"%s\"" % bfname)

            line_nr = 1
            line = f.readline()

            while line:
                line = line.strip()

                if line and line[0] != "#":
                    eq = line.find("=")

                    if eq <= 0:
                        error("\"%s\" line %d: Line must be in the form 'name = value value...'." % (bfname, line_nr))

                    bdict[line[:eq].strip()] = line[eq + 1:].strip()

                line_nr = line_nr + 1
                line = f.readline()

            f.close()

        # Check the compulsory values.
        for i in ("target", "sources"):
            try:
                bdict[i]
            except KeyError:
                error("\"%s\" is missing from \"%s\"." % (i, bfname))

        # Get the optional values.
        for i in ("headers", "moc_headers"):
            try:
                bdict[i]
            except KeyError:
                bdict[i] = ""

        # Generate the list of objects.
        if self.generator in ("MSVC", "MSVC.NET", "MSBUILD", "BMAKE"):
            ext = ".obj"
        else:
            ext = ".o"

        olist = []

        for f in bdict["sources"].split():
            root, discard = os.path.splitext(f)
            olist.append(root + ext)

        for f in bdict["moc_headers"].split():
            if not self._qt:
                error("\"%s\" defines \"moc_headers\" for a non-Qt module." % bfname)

            root, discard = os.path.splitext(f)
            olist.append("moc_" + root + ext)

        bdict["objects"] = ' '.join(olist)

        return bdict

    def clean_build_file_objects(self, mfile, build):
        """Generate the clean target.

        mfile is the file object.
        build is the dictionary created from the build file.
        """
        mfile.write("\t-%s $(TARGET)\n" % self.rm)

        for f in build["objects"].split():
            mfile.write("\t-%s %s\n" % (self.rm, f))

        for f in build["moc_headers"].split():
            root, discard = os.path.splitext(f)
            mfile.write("\t-%s moc_%s.cpp\n" % (self.rm, root))

    def ready(self):
        """The Makefile is now ready to be used.
        """
        if not self._finalised:
            self.finalise()

    def generate(self):
        """Generate the Makefile.
        """
        self.ready()

        # Make sure the destination directory exists.
        try:
            os.makedirs(self.dir)
        except:
            pass

        mfname = os.path.join(self.dir, self._makefile)

        try:
            mfile = open(mfname, "w")
        except IOError:
            error("Unable to create \"%s\"" % mfname)

        self.generate_macros_and_rules(mfile)
        self.generate_target_default(mfile)
        self.generate_target_install(mfile)

        if self._installs:
            if type(self._installs) != list:
                self._installs = [self._installs]

            for src, dst in self._installs:
                self.install_file(mfile, src, dst)

        self.generate_target_clean(mfile)

        mfile.close()

    def generate_macros_and_rules(self, mfile):
        """The default implementation of the macros and rules generation.

        mfile is the file object.
        """
        if self._deployment_target:
            mfile.write("export MACOSX_DEPLOYMENT_TARGET = %s\n" % self._deployment_target)

        mfile.write("CC = %s\n" % self.required_string("CC"))
        mfile.write("CXX = %s\n" % self.required_string("CXX"))
        mfile.write("LINK = %s\n" % self.required_string("LINK"))

        cppflags = []

        if not self._debug:
            cppflags.append("-DNDEBUG")

        for f in self.optional_list("DEFINES"):
            cppflags.append("-D" + f)

        for f in self.optional_list("INCDIR"):
            cppflags.append("-I" + _quote(f))

        libs = []

        if self.generator in ("MSVC", "MSVC.NET", "MSBUILD"):
            libdir_prefix = "/LIBPATH:"
        else:
            libdir_prefix = "-L"

        for ld in self.optional_list("LIBDIR"):
            if sys.platform == "darwin" and self.config.qt_framework:
                fflag = "-F" + _quote(ld)
                libs.append(fflag)
                cppflags.append(fflag)

            libs.append(libdir_prefix + _quote(ld))

        libs.extend(self.optional_list("LIBS"))

        mfile.write("CPPFLAGS = %s\n" % ' '.join(cppflags))

        mfile.write("CFLAGS = %s\n" % self.optional_string("CFLAGS"))
        mfile.write("CXXFLAGS = %s\n" % self.optional_string("CXXFLAGS"))
        mfile.write("LFLAGS = %s\n" % self.optional_string("LFLAGS"))

        mfile.write("LIBS = %s\n" % ' '.join(libs))

        if self._qt:
            mfile.write("MOC = %s\n" % _quote(self.required_string("MOC")))

        if self._src_dir != self.dir:
            mfile.write("VPATH = %s\n\n" % self._src_dir)

        # These probably don't matter.
        if self.generator == "MINGW":
            mfile.write(".SUFFIXES: .cpp .cxx .cc .C .c\n\n")
        elif self.generator == "UNIX":
            mfile.write(".SUFFIXES: .c .o .cpp .cc .cxx .C\n\n")
        else:
            mfile.write(".SUFFIXES: .c .cpp .cc .cxx .C\n\n")

        if self.generator in ("MSVC", "MSVC.NET", "MSBUILD"):
            mfile.write("""
{.}.cpp{}.obj::
\t$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -Fo @<<
\t$<
<<

{.}.cc{}.obj::
\t$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -Fo @<<
\t$<
<<

{.}.cxx{}.obj::
\t$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -Fo @<<
\t$<
<<

{.}.C{}.obj::
\t$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -Fo @<<
\t$<
<<

{.}.c{}.obj::
\t$(CC) -c $(CFLAGS) $(CPPFLAGS) -Fo @<<
\t$<
<<
""")
        elif self.generator == "BMAKE":
            mfile.write("""
.cpp.obj:
\t$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o$@ $<

.cc.obj:
\t$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o$@ $<

.cxx.obj:
\t$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o$@ $<

.C.obj:
\t$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o$@ $<

.c.obj:
\t$(CC) -c $(CFLAGS) $(CPPFLAGS) -o$@ $<
""")
        else:
            mfile.write("""
.cpp.o:
\t$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

.cc.o:
\t$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

.cxx.o:
\t$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

.C.o:
\t$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

.c.o:
\t$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<
""")

    def generate_target_default(self, mfile):
        """The default implementation of the default target.

        mfile is the file object.
        """
        mfile.write("\nall:\n")

    def generate_target_install(self, mfile):
        """The default implementation of the install target.

        mfile is the file object.
        """
        mfile.write("\ninstall:\n")

    def generate_target_clean(self, mfile):
        """The default implementation of the clean target.

        mfile is the file object.
        """
        mfile.write("\nclean:\n")

    def install_file(self, mfile, src, dst, strip=0):
        """Install one or more files in a directory.

        mfile is the file object.
        src is the name of a single file to install, or the list of a number of
        files to install.
        dst is the name of the destination directory.
        strip is set if the files should be stripped after been installed.
        """
        # Help package builders.
        if self.generator == "UNIX":
            dst = "$(DESTDIR)" + dst

        mfile.write("\t@%s %s " % (self.chkdir, _quote(dst)))

        if self.generator == "UNIX":
            mfile.write("|| ")

        mfile.write("%s %s\n" % (self.mkdir, _quote(dst)))

        if type(src) != list:
            src = [src]

        # Get the strip command if needed.
        if strip:
            strip_cmd = self.optional_string("STRIP")

            if not strip_cmd:
                strip = 0

        for sf in src:
            target = _quote(os.path.join(dst, os.path.basename(sf)))

            mfile.write("\t%s %s %s\n" % (self.copy, _quote(sf), target))

            if strip:
                mfile.write("\t%s %s\n" % (strip_cmd, target))


class ParentMakefile(Makefile):
    """The class that represents a parent Makefile.
    """
    def __init__(self, configuration, subdirs, dir=None, makefile="Makefile",
                 installs=None):
        """Initialise an instance of a parent Makefile.

        subdirs is the sequence of subdirectories.
        """
        Makefile.__init__(self, configuration, dir=dir, makefile=makefile, installs=installs)

        self._subdirs = subdirs

    def generate_macros_and_rules(self, mfile):
        """Generate the macros and rules.

        mfile is the file object.
        """
        # We don't want them.
        pass

    def generate_target_default(self, mfile):
        """Generate the default target.

        mfile is the file object.
        """
        self._subdir_target(mfile)

    def generate_target_install(self, mfile):
        """Generate the install target.

        mfile is the file object.
        """
        self._subdir_target(mfile, "install")

    def generate_target_clean(self, mfile):
        """Generate the clean target.

        mfile is the file object.
        """
        self._subdir_target(mfile, "clean")

    def _subdir_target(self, mfile, target="all"):
        """Create a target for a list of sub-directories.

        mfile is the file object.
        target is the name of the target.
        """
        if target == "all":
            tname = ""
        else:
            tname = " " + target

        mfile.write("\n" + target + ":\n")

        for d in self._subdirs:
            if self.generator == "MINGW":
                mfile.write("\t@$(MAKE) -C %s%s\n" % (d, tname))
            elif self.generator == "UNIX":
                mfile.write("\t@(cd %s; $(MAKE)%s)\n" % (d, tname))
            else:
                mfile.write("\tcd %s\n" % d)
                mfile.write("\t$(MAKE)%s\n" % tname)
                mfile.write("\t@cd ..\n")


class PythonModuleMakefile(Makefile):
    """The class that represents a Python module Makefile.
    """
    def __init__(self, configuration, dstdir, srcdir=None, dir=None,
                 makefile="Makefile", installs=None):
        """Initialise an instance of a parent Makefile.

        dstdir is the name of the directory where the module's Python code will
        be installed.
        srcdir is the name of the directory (relative to the directory in which
        the Makefile will be created) containing the module's Python code.  It
        defaults to the same directory.
        """
        Makefile.__init__(self, configuration, dir=dir, makefile=makefile, installs=installs)

        if not srcdir:
            srcdir = "."

        if dir:
            self._moddir = os.path.join(dir, srcdir)
        else:
            self._moddir = srcdir

        self._srcdir = srcdir
        self._dstdir = dstdir

    def generate_macros_and_rules(self, mfile):
        """Generate the macros and rules.

        mfile is the file object.
        """
        # We don't want them.
        pass

    def generate_target_install(self, mfile):
        """Generate the install target.

        mfile is the file object.
        """
        Makefile.generate_target_install(self, mfile)

        for root, dirs, files in os.walk(self._moddir):
            # Do not recurse into certain directories.
            for skip in (".svn", "CVS"):
                if skip in dirs:
                    dirs.remove(skip)

            tail = root[len(self._moddir):]
            flist = []

            for f in files:
                if f == "Makefile":
                    continue

                if os.path.isfile(os.path.join(root, f)):
                    flist.append(os.path.join(self._srcdir + tail, f))

            self.install_file(mfile, flist, self._dstdir + tail)


class ModuleMakefile(Makefile):
    """The class that represents a Python extension module Makefile
    """
    def __init__(self, configuration, build_file, install_dir=None, static=0,
                 console=0, qt=0, opengl=0, threaded=0, warnings=1, debug=0,
                 dir=None, makefile="Makefile", installs=None, strip=1,
                 export_all=0, universal=None, arch=None,
                 deployment_target=None):
        """Initialise an instance of a module Makefile.

        build_file is the file containing the target specific information.  If
        it is a dictionary instead then its contents are validated.
        install_dir is the directory the target will be installed in.
        static is set if the module should be built as a static library.
        strip is set if the module should be stripped of unneeded symbols when
        installed.  The default is 1.
        export_all is set if all the module's symbols should be exported rather
        than just the module's initialisation function.  Exporting all symbols
        increases the size of the module and slows down module load times but
        may avoid problems with modules that use exceptions.  The default is 0.
        """
        Makefile.__init__(self, configuration, console, qt, opengl, 1, threaded, warnings, debug, dir, makefile, installs, universal, arch, deployment_target)

        self._build = self.parse_build_file(build_file)
        self._install_dir = install_dir
        self.static = static

        self._manifest = ("embed_manifest_dll" in self.optional_list("CONFIG"))

        # Don't strip or restrict the exports if this is a debug or static
        # build.
        if debug or static:
            self._strip = 0
            self._limit_exports = 0
        else:
            self._strip = strip
            self._limit_exports = not export_all

        # Save the target name for later.
        self._target = self._build["target"]

        # The name of the module entry point is Python version specific.
        if self.config.py_version >= 0x030000:
            self._entry_point = "PyInit_%s" % self._target
        else:
            self._entry_point = "init%s" % self._target

        if sys.platform != "win32" and static:
            self._target = "lib" + self._target

        if sys.platform == "win32" and debug:
            self._target = self._target + "_d"

    def finalise(self):
        """Finalise the macros common to all module Makefiles.
        """
        if self.console:
            lflags_console = "LFLAGS_CONSOLE"
        else:
            lflags_console = "LFLAGS_WINDOWS"

        if self.static:
            self.DEFINES.append("SIP_STATIC_MODULE")
        else:
            self.CFLAGS.extend(self.optional_list("CFLAGS_SHLIB"))
            self.CXXFLAGS.extend(self.optional_list("CXXFLAGS_SHLIB"))

            lflags_dll = self.optional_list("LFLAGS_DLL")

            if lflags_dll:
                self.LFLAGS.extend(lflags_dll)
            elif self.console:
                lflags_console = "LFLAGS_CONSOLE_DLL"
            else:
                lflags_console = "LFLAGS_WINDOWS_DLL"

            if self._manifest:
                self._add_manifest()

            # We use this to explictly create bundles on MacOS.  Apple's Python
            # can handle extension modules that are bundles or dynamic
            # libraries, but python.org versions need bundles (unless built
            # with DYNLOADFILE=dynload_shlib.o).
            if sys.platform == "darwin":
                lflags_plugin = ["-bundle"]
            else:
                lflags_plugin = self.optional_list("LFLAGS_PLUGIN")

            if not lflags_plugin:
                lflags_plugin = self.optional_list("LFLAGS_SHLIB")

            self.LFLAGS.extend(lflags_plugin)

        self.LFLAGS.extend(self.optional_list(lflags_console))

        if sys.platform == "darwin":
            from distutils.sysconfig import get_python_inc

            # The Python include directory seems to be the only one that uses
            # the real path even when using a virtual environment (eg. pyvenv).
            # Note that I can't remember why we need a framework build.
            dl = get_python_inc().split(os.sep)

            if "Python.framework" not in dl:
                error("SIP requires Python to be built as a framework")

            self.LFLAGS.append("-undefined dynamic_lookup")

        Makefile.finalise(self)

        if not self.static:
            if self.optional_string("AIX_SHLIB"):
                # AIX needs a lot of special handling.
                if self.required_string('LINK') == 'g++':
                    # g++ is used for linking.
                    # For SIP v4 and g++:
                    # 1.) Import the python symbols
                    aix_lflags = ['-Wl,-bI:%s/python.exp' % self.config.py_lib_dir]

                    if self._limit_exports:
                        aix_lflags.append('-Wl,-bnoexpall')
                        aix_lflags.append('-Wl,-bnoentry')
                        aix_lflags.append('-Wl,-bE:%s.exp' % self._target)
                else:
                    # IBM VisualAge C++ is used for linking.
                    # For SIP v4 and xlC:
                    # 1.) Create a shared object
                    # 2.) Import the python symbols
                    aix_lflags = ['-qmkshrobj',
                                  '-bI:%s/python.exp' % self.config.py_lib_dir]

                    if self._limit_exports:
                        aix_lflags.append('-bnoexpall')
                        aix_lflags.append('-bnoentry')
                        aix_lflags.append('-bE:%s.exp' % self._target)

                self.LFLAGS.extend(aix_lflags)
            else:
                if self._limit_exports:
                    if sys.platform[:5] == 'linux':
                        self.LFLAGS.extend(['-Wl,--version-script=%s.exp' % self._target])
                    elif sys.platform[:5] == 'sunos':
                        if self.required_string('LINK') == 'g++':
                            self.LFLAGS.extend(['-Wl,-z,noversion', '-Wl,-M,%s.exp' % self._target])
                        else:
                            self.LFLAGS.extend(['-z' 'noversion', '-M', '%s.exp' % self._target])
                    elif sys.platform[:5] == 'hp-ux':
                        self.LFLAGS.extend(['-Wl,+e,%s' % self._entry_point])
                    elif sys.platform[:5] == 'irix' and self.required_string('LINK') != 'g++':
                        # Doesn't work when g++ is used for linking on IRIX.
                        self.LFLAGS.extend(['-Wl,-exported_symbol,%s' % self._entry_point])

                # Force the shared linker if there is one.
                link_shlib = self.optional_list("LINK_SHLIB")

                if link_shlib:
                    self.LINK.set(link_shlib)

        # This made an appearence in Qt v4.4rc1 and breaks extension modules so
        # remove it.  It was removed at my request but some stupid distros may
        # have kept it.
        self.LFLAGS.remove('-Wl,--no-undefined') 

    def module_as_lib(self, mname):
        """Return the name of a SIP v3.x module when it is used as a library.
        This will raise an exception when used with SIP v4.x modules.

        mname is the name of the module.
        """
        raise ValueError("module_as_lib() can only be used with SIP v3.x")

    def generate_macros_and_rules(self, mfile):
        """Generate the macros and rules generation.

        mfile is the file object.
        """
        if self.static:
            if sys.platform == "win32":
                ext = "lib"
            else:
                ext = "a"
        else:
            if sys.platform == "win32":
                ext = "pyd"
            elif sys.platform == "darwin":
                ext = "so"
            elif sys.platform == "cygwin":
                ext = "dll"
            else:
                ext = self.optional_string("EXTENSION_PLUGIN")
                if not ext:
                    ext = self.optional_string("EXTENSION_SHLIB", "so")

        mfile.write("TARGET = %s\n" % (self._target + "." + ext))
        mfile.write("OFILES = %s\n" % self._build["objects"])
        mfile.write("HFILES = %s %s\n" % (self._build["headers"], self._build["moc_headers"]))
        mfile.write("\n")

        if self.static:
            if self.generator in ("MSVC", "MSVC.NET", "MSBUILD", "BMAKE"):
                mfile.write("LIB = %s\n" % self.required_string("LIB"))
            elif self.generator == "MINGW":
                mfile.write("AR = %s\n" % self.required_string("LIB"))
                self._ranlib = None
            else:
                mfile.write("AR = %s\n" % self.required_string("AR"))

                self._ranlib = self.optional_string("RANLIB")

                if self._ranlib:
                    mfile.write("RANLIB = %s\n" % self._ranlib)

        Makefile.generate_macros_and_rules(self, mfile)

    def generate_target_default(self, mfile):
        """Generate the default target.

        mfile is the file object.
        """
        # Do these first so that it's safe for a sub-class to append additional
        # commands to the real target, but make sure the default is correct.
        mfile.write("\nall: $(TARGET)\n")
        mfile.write("\n$(OFILES): $(HFILES)\n")

        for mf in self._build["moc_headers"].split():
            root, discard = os.path.splitext(mf)
            cpp = "moc_" + root + ".cpp"

            mfile.write("\n%s: %s\n" % (cpp, mf))
            mfile.write("\t$(MOC) -o %s %s\n" % (cpp, mf))

        mfile.write("\n$(TARGET): $(OFILES)\n")

        if self.generator in ("MSVC", "MSVC.NET", "MSBUILD"):
            if self.static:
                mfile.write("\t$(LIB) /OUT:$(TARGET) @<<\n")
                mfile.write("\t  $(OFILES)\n")
                mfile.write("<<\n")
            else:
                mfile.write("\t$(LINK) $(LFLAGS) /OUT:$(TARGET) @<<\n")
                mfile.write("\t  $(OFILES) $(LIBS)\n")
                mfile.write("<<\n")

                if self._manifest:
                    mfile.write("\tmt -nologo -manifest $(TARGET).manifest -outputresource:$(TARGET);2\n")
        elif self.generator == "BMAKE":
            if self.static:
                mfile.write("\t-%s $(TARGET)\n" % (self.rm))
                mfile.write("\t$(LIB) $(TARGET) @&&|\n")

                for of in self._build["objects"].split():
                    mfile.write("+%s \\\n" % (of))

                mfile.write("|\n")
            else:
                mfile.write("\t$(LINK) @&&|\n")
                mfile.write("\t$(LFLAGS) $(OFILES) ,$(TARGET),,$(LIBS),%s\n" % (self._target))
                mfile.write("|\n")

                # Create the .def file that renames the entry point.
                defname = os.path.join(self.dir, self._target + ".def")

                try:
                    dfile = open(defname, "w")
                except IOError:
                    error("Unable to create \"%s\"" % defname)

                dfile.write("EXPORTS\n")
                dfile.write("%s=_%s\n" % (self._entry_point, self._entry_point))

                dfile.close()

        else:
            if self.static:
                mfile.write("\t-%s $(TARGET)\n" % self.rm)
                mfile.write("\t$(AR) $(TARGET) $(OFILES)\n")

                if self._ranlib:
                    mfile.write("\t$(RANLIB) $(TARGET)\n")
            else:
                if self._limit_exports:
                    # Create an export file for AIX, Linux and Solaris.
                    if sys.platform[:5] == 'linux':
                        mfile.write("\t@echo '{ global: %s; local: *; };' > %s.exp\n" % (self._entry_point, self._target))
                    elif sys.platform[:5] == 'sunos':
                        mfile.write("\t@echo '{ global: %s; local: *; };' > %s.exp\n" % (self._entry_point, self._target))
                    elif sys.platform[:3] == 'aix':
                        mfile.write("\t@echo '#!' >%s.exp" % self._target)
                        mfile.write("; \\\n\t echo '%s' >>%s.exp\n" % (self._entry_point, self._target))

                mfile.write("\t$(LINK) $(LFLAGS) -o $(TARGET) $(OFILES) $(LIBS)\n")

    def generate_target_install(self, mfile):
        """Generate the install target.

        mfile is the file object.
        """
        if self._install_dir is None:
            self._install_dir = self.config.default_mod_dir

        mfile.write("\ninstall: $(TARGET)\n")
        self.install_file(mfile, "$(TARGET)", self._install_dir, self._strip)

    def generate_target_clean(self, mfile):
        """Generate the clean target.

        mfile is the file object.
        """
        mfile.write("\nclean:\n")
        self.clean_build_file_objects(mfile, self._build)

        if self._manifest and not self.static:
            mfile.write("\t-%s $(TARGET).manifest\n" % self.rm)

        # Remove any export file on AIX, Linux and Solaris.
        if self._limit_exports and (sys.platform[:5] == 'linux' or
                                    sys.platform[:5] == 'sunos' or
                                    sys.platform[:3] == 'aix'):
            mfile.write("\t-%s %s.exp\n" % (self.rm, self._target))


class SIPModuleMakefile(ModuleMakefile):
    """The class that represents a SIP generated module Makefile.
    """
    def __init__(self, configuration, build_file, install_dir=None, static=0,
                 console=0, qt=0, opengl=0, threaded=0, warnings=1, debug=0,
                 dir=None, makefile="Makefile", installs=None, strip=1,
                 export_all=0, universal=None, arch=None, prot_is_public=0,
                 deployment_target=None):
        """Initialise an instance of a SIP generated module Makefile.

        prot_is_public is set if "protected" is to be redefined as "public".
        If the platform's C++ ABI allows it this can significantly reduce the
        size of the generated code.

        For all other arguments see ModuleMakefile.
        """
        ModuleMakefile.__init__(self, configuration, build_file, install_dir,
                static, console, qt, opengl, threaded, warnings, debug, dir,
                makefile, installs, strip, export_all, universal, arch,
                deployment_target)

        self._prot_is_public = prot_is_public

    def finalise(self):
        """Finalise the macros for a SIP generated module Makefile.
        """
        if self._prot_is_public:
            self.DEFINES.append('SIP_PROTECTED_IS_PUBLIC')
            self.DEFINES.append('protected=public')

        self.INCDIR.append(self.config.sip_inc_dir)

        ModuleMakefile.finalise(self)


class ProgramMakefile(Makefile):
    """The class that represents a program Makefile.
    """
    def __init__(self, configuration, build_file=None, install_dir=None,
                 console=0, qt=0, opengl=0, python=0, threaded=0, warnings=1,
                 debug=0, dir=None, makefile="Makefile", installs=None,
                 universal=None, arch=None, deployment_target=None):
        """Initialise an instance of a program Makefile.

        build_file is the file containing the target specific information.  If
        it is a dictionary instead then its contents are validated.
        install_dir is the directory the target will be installed in.
        """
        Makefile.__init__(self, configuration, console, qt, opengl, python, threaded, warnings, debug, dir, makefile, installs, universal, arch, deployment_target)

        self._install_dir = install_dir

        self._manifest = ("embed_manifest_exe" in self.optional_list("CONFIG"))
        self._target = None

        if build_file:
            self._build = self.parse_build_file(build_file)
        else:
            self._build = None

    def build_command(self, source):
        """Create a command line that will build an executable.  Returns a
        tuple of the name of the executable and the command line.

        source is the name of the source file.
        """
        # The name of the executable.
        self._target, _ = os.path.splitext(source)

        if sys.platform in ("win32", "cygwin"):
            exe = self._target + ".exe"
        else:
            exe = self._target

        self.ready()

        # The command line.
        build = []

        build.append(self.required_string("CXX"))

        for a in self._arch.split():
            build.append('-arch ' + a)

        for f in self.optional_list("DEFINES"):
            build.append("-D" + f)

        for f in self.optional_list("INCDIR"):
            build.append("-I" + _quote(f))

        build.extend(self.optional_list("CXXFLAGS"))

        # This is for Qt5.
        build.extend(self.optional_list("CXXFLAGS_APP"))

        # Borland requires all flags to precede all file names.
        if self.generator != "BMAKE":
            build.append(source)

        if self.generator in ("MSVC", "MSVC.NET", "MSBUILD"):
            build.append("-Fe")
            build.append("/link")
            libdir_prefix = "/LIBPATH:"
        elif self.generator == "BMAKE":
            build.append("-e" + exe)
            libdir_prefix = "-L"
        else:
            build.append("-o")
            build.append(exe)
            libdir_prefix = "-L"

        for ld in self.optional_list("LIBDIR"):
            if sys.platform == "darwin" and self.config.qt_framework:
                build.append("-F" + _quote(ld))

            build.append(libdir_prefix + _quote(ld))

        lflags = self.optional_list("LFLAGS")

        # This is a huge hack demonstrating my lack of understanding of how the
        # Borland compiler works.
        if self.generator == "BMAKE":
            blflags = []

            for lf in lflags:
                for f in lf.split():
                    # Tell the compiler to pass the flags to the linker.
                    if f[-1] == "-":
                        f = "-l-" + f[1:-1]
                    elif f[0] == "-":
                        f = "-l" + f[1:]

                    # Remove any explicit object files otherwise the compiler
                    # will complain that they can't be found, but they don't
                    # seem to be needed.
                    if f[-4:].lower() != ".obj":
                        blflags.append(f)

            lflags = blflags

        build.extend(lflags)

        build.extend(self.optional_list("LIBS"))

        if self.generator == "BMAKE":
            build.append(source)

        return (exe, ' '.join(build))

    def finalise(self):
        """Finalise the macros for a program Makefile.
        """
        if self.generator in ("MSVC", "MSVC.NET", "MSBUILD"):
            self.LFLAGS.append("/INCREMENTAL:NO")

        if self._manifest:
            self._add_manifest(self._target)

        if self.console:
            lflags_console = "LFLAGS_CONSOLE"
        else:
            lflags_console = "LFLAGS_WINDOWS"

        self.LFLAGS.extend(self.optional_list(lflags_console))

        Makefile.finalise(self)

    def generate_macros_and_rules(self, mfile):
        """Generate the macros and rules generation.

        mfile is the file object.
        """
        if not self._build:
            raise ValueError("pass a filename as build_file when generating a Makefile")

        target = self._build["target"]

        if sys.platform in ("win32", "cygwin"):
            target = target + ".exe"

        mfile.write("TARGET = %s\n" % target)
        mfile.write("OFILES = %s\n" % self._build["objects"])
        mfile.write("HFILES = %s\n" % self._build["headers"])
        mfile.write("\n")

        Makefile.generate_macros_and_rules(self, mfile)

    def generate_target_default(self, mfile):
        """Generate the default target.

        mfile is the file object.
        """
        # Do these first so that it's safe for a sub-class to append additional
        # commands to the real target, but make sure the default is correct.
        mfile.write("\nall: $(TARGET)\n")
        mfile.write("\n$(OFILES): $(HFILES)\n")

        for mf in self._build["moc_headers"].split():
            root, _ = os.path.splitext(mf)
            cpp = "moc_" + root + ".cpp"

            if self._src_dir != self.dir:
                mf = os.path.join(self._src_dir, mf)

            mfile.write("\n%s: %s\n" % (cpp, mf))
            mfile.write("\t$(MOC) -o %s %s\n" % (cpp, mf))

        mfile.write("\n$(TARGET): $(OFILES)\n")

        if self.generator in ("MSVC", "MSVC.NET", "MSBUILD"):
            mfile.write("\t$(LINK) $(LFLAGS) /OUT:$(TARGET) @<<\n")
            mfile.write("\t  $(OFILES) $(LIBS)\n")
            mfile.write("<<\n")
        elif self.generator == "BMAKE":
            mfile.write("\t$(LINK) @&&|\n")
            mfile.write("\t$(LFLAGS) $(OFILES) ,$(TARGET),,$(LIBS),,\n")
            mfile.write("|\n")
        else:
            mfile.write("\t$(LINK) $(LFLAGS) -o $(TARGET) $(OFILES) $(LIBS)\n")

        if self._manifest:
            mfile.write("\tmt -nologo -manifest $(TARGET).manifest -outputresource:$(TARGET);1\n")

    def generate_target_install(self, mfile):
        """Generate the install target.

        mfile is the file object.
        """
        if self._install_dir is None:
            self._install_dir = self.config.default_bin_dir

        mfile.write("\ninstall: $(TARGET)\n")
        self.install_file(mfile, "$(TARGET)", self._install_dir)

    def generate_target_clean(self, mfile):
        """Generate the clean target.

        mfile is the file object.
        """
        mfile.write("\nclean:\n")
        self.clean_build_file_objects(mfile, self._build)

        if self._manifest:
            mfile.write("\t-%s $(TARGET).manifest\n" % self.rm)


def _quote(s):
    """Return a string surrounded by double quotes it if contains a space.

    s is the string.
    """
    # On Qt5 paths often includes forward slashes so convert them.
    if sys.platform == "win32":
        s = s.replace("/", "\\")

    if s.find(" ") >= 0:
        s = '"' + s + '"'

    return s


def version_to_string(v):
    """Convert a 3 part version number encoded as a hexadecimal value to a
    string.
    """
    return "%u.%u.%u" % (((v >> 16) & 0xff), ((v >> 8) & 0xff), (v & 0xff))


def read_version(filename, description, numdefine=None, strdefine=None):
    """Read the version information for a package from a file.  The information
    is specified as #defines of a numeric (hexadecimal or decimal) value and/or
    a string value.

    filename is the name of the file.
    description is the descriptive name of the package.
    numdefine is the name of the #define of the numeric version.  It is ignored
    if it is None.
    strdefine is the name of the #define of the string version.  It is ignored
    if it is None.

    Returns a tuple of the version as a number and as a string.
    """
    need_num = numdefine is not None
    need_str = strdefine is not None

    vers = None
    versstr = None

    f = open(filename)
    l = f.readline()

    while l and (need_num or need_str):
        wl = l.split()
        if len(wl) >= 3 and wl[0] == "#define":
            if need_num and wl[1] == numdefine:
                v = wl[2]

                if v[0:2] == "0x":
                    vers = int(v, 16)
                else:
                    dec = int(v)
                    maj = dec / 100
                    min = (dec % 100) / 10
                    bug = (dec % 10)
                    vers = (maj << 16) + (min << 8) + bug

                need_num = 0

            if need_str and wl[1] == strdefine:
                # Take account of embedded spaces.
                versstr = ' '.join(wl[2:])[1:-1]
                need_str = 0

        l = f.readline()

    f.close()

    if need_num or need_str:
        error("The %s version number could not be determined by parsing %s." % (description, filename))

    return (vers, versstr)


def create_content(cdict, macros=None):
    """Convert a dictionary to a string (typically to use as the content to a
    call to create_config_module()).  Dictionary values that are strings are
    quoted.  Dictionary values that are lists are converted to quoted strings.

    dict is the dictionary.
    macros is the optional dictionary of platform specific build macros.
    """
    content = "_pkg_config = {\n"

    keys = list(cdict.keys())
    keys.sort()

    # Format it nicely.
    width = 0

    for k in keys:
        klen = len(k)

        if width < klen:
            width = klen

    for k in keys:
        val = cdict[k]
        vtype = type(val)
        delim = None

        if val is None:
            val = "None"
        elif vtype == list:
            val = ' '.join(val)
            delim = "'"
        elif vtype == int:
            if k.find("version") >= 0:
                # Assume it's a hexadecimal version number.  It doesn't matter
                # if it isn't, we are just trying to make it look pretty.
                val = "0x%06x" % val
            else:
                val = str(val)
        else:
            val = str(val)
            delim = "'"

        if delim:
            if "'" in val:
                delim = "'''"

            val = delim + val + delim

        content = content + "    '" + k + "':" + (" " * (width - len(k) + 2)) + val.replace("\\", "\\\\")

        if k != keys[-1]:
            content = content + ","

        content = content + "\n"

    content = content + "}\n\n"

    # Format the optional macros.
    content = content + "_default_macros = "

    if macros:
        content = content + "{\n"

        names = list(macros.keys())
        names.sort()

        width = 0
        for c in names:
            clen = len(c)
            if width < clen:
                width = clen

        for c in names:
            if c == names[-1]:
                sep = ""
            else:
                sep = ","

            val = macros[c]
            if "'" in val:
                delim = "'''"
            else:
                delim = "'"

            k = "'" + c + "':"
            content = content + "    %-*s  %s%s%s%s\n" % (1 + width + 2, k, delim, val.replace("\\", "\\\\"), delim, sep)

        content = content + "}\n"
    else:
        content = content + "None\n"

    return content


def create_config_module(module, template, content, macros=None):
    """Create a configuration module by replacing "@" followed by
    "SIP_CONFIGURATION" followed by "@" in a template file with a content
    string.

    module is the name of the module file.
    template is the name of the template file.
    content is the content string.  If it is a dictionary it is first converted
    to a string using create_content().
    macros is an optional dictionary of platform specific build macros.  It is
    only used if create_content() is called to convert the content to a string.
    """
    if type(content) == dict:
        content = create_content(content, macros)

    # Allow this file to used as a template.
    key = "@" + "SIP_CONFIGURATION" + "@"

    df = open(module, "w")
    sf = open(template, "r")

    line = sf.readline()
    while line:
        if line.find(key) >= 0:
            line = content

        df.write(line)

        line = sf.readline()

    df.close()
    sf.close()


def version_to_sip_tag(version, tags, description):
    """Convert a version number to a SIP tag.

    version is the version number.  If it is negative then the latest version
    is assumed.  (This is typically useful if a snapshot is indicated by a
    negative version number.)
    tags is the dictionary of tags keyed by version number.  The tag used is
    the one with the smallest key (ie. earliest version) that is greater than
    the given version number.
    description is the descriptive name of the package used for error messages.

    Returns the corresponding tag.
    """
    vl = list(tags.keys())
    vl.sort()

    # For a snapshot use the latest tag.
    if version < 0:
        tag = tags[vl[-1]]
    else:
        for v in vl:
            if version < v:
                tag = tags[v]
                break
        else:
            error("Unsupported %s version: 0x%06x." % (description, version))

    return tag


def error(msg):
    """Display an error message and terminate.

    msg is the text of the error message.
    """
    sys.stderr.write(format("Error: " + msg) + "\n")
    sys.exit(1)

 
def inform(msg):
    """Display an information message.

    msg is the text of the error message.
    """
    sys.stdout.write(format(msg) + "\n")


def format(msg, leftmargin=0, rightmargin=78):
    """Format a message by inserting line breaks at appropriate places.

    msg is the text of the message.
    leftmargin is the position of the left margin.
    rightmargin is the position of the right margin.

    Return the formatted message.
    """
    curs = leftmargin
    fmsg = " " * leftmargin

    for w in msg.split():
        l = len(w)
        if curs != leftmargin and curs + l > rightmargin:
            fmsg = fmsg + "\n" + (" " * leftmargin)
            curs = leftmargin

        if curs > leftmargin:
            fmsg = fmsg + " "
            curs = curs + 1

        fmsg = fmsg + w
        curs = curs + l

    return fmsg


def parse_build_macros(filename, names, overrides=None, properties=None):
    """Parse a qmake compatible file of build system macros and convert it to a
    dictionary.  A macro is a name/value pair.  The dictionary is returned or
    None if any of the overrides was invalid.

    filename is the name of the file to parse.
    names is a list of the macro names to extract from the file.
    overrides is an optional list of macro names and values that modify those
    found in the file.  They are of the form "name=value" (in which case the
    value replaces the value found in the file) or "name+=value" (in which case
    the value is appended to the value found in the file).
    properties is an optional dictionary of property name and values that are
    used to resolve any expressions of the form "$[name]" in the file.
    """
    # Validate and convert the overrides to a dictionary.
    orides = {}

    if overrides is not None:
        for oride in overrides:
            prefix = ""
            name_end = oride.find("+=")

            if name_end >= 0:
                prefix = "+"
                val_start = name_end + 2
            else:
                name_end = oride.find("=")

                if name_end >= 0:
                    val_start = name_end + 1
                else:
                    return None

            name = oride[:name_end]

            if name not in names:
                return None

            orides[name] = prefix + oride[val_start:]

    # This class defines a file like object that handles the nested include()
    # directives in qmake files.
    class qmake_build_file_reader:
        def __init__(self, filename):
            self.filename = filename
            self.currentfile = None
            self.filestack = []
            self.pathstack = []
            self.cond_fname = None
            self._openfile(filename)

        def _openfile(self, filename):
            try:
                f = open(filename, 'r')
            except IOError:
                # If this file is conditional then don't raise an error.
                if self.cond_fname == filename:
                    return

                error("Unable to open %s" % filename)

            if self.currentfile:
                self.filestack.append(self.currentfile)
                self.pathstack.append(self.path)

            self.currentfile = f
            self.path = os.path.dirname(filename)

        def readline(self):
            line = self.currentfile.readline()
            sline = line.strip()

            if self.cond_fname and sline == '}':
                # The current condition is closed.
                self.cond_fname = None
                line = self.currentfile.readline()
            elif sline.startswith('exists(') and sline.endswith('{'):
                # A new condition is opened so extract the filename.
                self.cond_fname = self._normalise(sline[:-1].strip()[7:-1].strip())
                line = self.currentfile.readline()
            elif sline.startswith('include('):
                nextfile = self._normalise(sline[8:-1].strip())
                self._openfile(nextfile)
                return self.readline()

            if not line:
                self.currentfile.close()

                if self.filestack:
                    self.currentfile = self.filestack.pop()
                    self.path = self.pathstack.pop()
                    return self.readline()

            return line

        # Normalise a filename by expanding any environment variables and
        # making sure it is absolute.
        def _normalise(self, fname):
            if "$(" in fname:
                fname = os.path.normpath(self._expandvars(fname))

            if not os.path.isabs(fname):
                fname = os.path.join(self.path, fname)

            return fname

        # Expand the environment variables in a filename.
        def _expandvars(self, fname):
            i = 0
            while True:
                m = re.search(r'\$\((\w+)\)', fname[i:])
                if not m:
                    break

                i, j = m.span(0)
                name = m.group(1)
                if name in os.environ:
                    tail = fname[j:]
                    fname = fname[:i] + os.environ[name]
                    i = len(fname)
                    fname += tail
                else:
                    i = j

            return fname

    f = qmake_build_file_reader(filename)

    # Get everything into a dictionary.
    raw = {
        "DIR_SEPARATOR":        os.sep,
        "LITERAL_WHITESPACE":   " ",
        "LITERAL_DOLLAR":       "$",
        "LITERAL_HASH":         "#"
    }

    line = f.readline()
    while line:
        # Handle line continuations.
        while len(line) > 1 and line[-2] == "\\":
            line = line[:-2]

            next = f.readline()

            if next:
                line = line + next
            else:
                break

        line = line.strip()

        # Ignore comments.
        if line and line[0] != "#":
            assstart = line.find("+")
            if assstart > 0 and line[assstart + 1] == '=':
                adding = True
                assend = assstart + 1
            else:
                adding = False
                assstart = line.find("=")
                assend = assstart

            if assstart > 0:
                lhs = line[:assstart].strip()
                rhs = line[assend + 1:].strip()

                # Remove the escapes for any quotes.
                rhs = rhs.replace(r'\"', '"').replace(r"\'", "'")

                if adding and rhs != "":
                    orig_rhs = raw.get(lhs)
                    if orig_rhs is not None:
                        rhs = orig_rhs + " " + rhs

                raw[lhs] = _expand_macro_value(raw, rhs, properties)

        line = f.readline()

    # Go through the raw dictionary extracting the macros we need and
    # resolving any macro expansions.  First of all, make sure every macro has
    # a value.
    refined = {}

    for m in names:
        refined[m] = ""

    macro_prefix = "QMAKE_"

    for lhs in list(raw.keys()):
        # Strip any prefix.
        if lhs.startswith(macro_prefix):
            reflhs = lhs[len(macro_prefix):]
        else:
            reflhs = lhs

        # See if we are interested in this one.
        if reflhs not in names:
            continue

        rhs = raw[lhs]

        # Expand any POSIX style environment variables.
        pleadin = ["$$(", "$("]

        for pl in pleadin:
            estart = rhs.find(pl)

            if estart >= 0:
                nstart = estart + len(pl)
                break
        else:
            estart = -1

        while estart >= 0:
            eend = rhs[nstart:].find(")")

            if eend < 0:
                break

            eend = nstart + eend

            name = rhs[nstart:eend]

            try:
                env = os.environ[name]
            except KeyError:
                env = ""

            rhs = rhs[:estart] + env + rhs[eend + 1:]

            for pl in pleadin:
                estart = rhs.find(pl)

                if estart >= 0:
                    nstart = estart + len(pl)
                    break
            else:
                estart = -1

        # Expand any Windows style environment variables.
        estart = rhs.find("%")

        while estart >= 0:
            eend = rhs[estart + 1:].find("%")

            if eend < 0:
                break

            eend = estart + 1 + eend

            name = rhs[estart + 1:eend]

            try:
                env = os.environ[name]
            except KeyError:
                env = ""

            rhs = rhs[:estart] + env + rhs[eend + 1:]

            estart = rhs.find("%")

        refined[reflhs] = rhs

    # Handle the user overrides.
    for lhs in list(orides.keys()):
        rhs = refined[lhs]
        oride = orides[lhs]

        if oride.find("+") == 0:
            if rhs:
                rhs = rhs + " " + oride[1:]
            else:
                rhs = oride[1:]
        else:
            rhs = oride

        refined[lhs] = rhs

    return refined


def _expand_macro_value(macros, rhs, properties):
    """Expand the value of a macro based on ones seen so far."""
    estart = rhs.find("$$(")
    mstart = rhs.find("$$")

    while mstart >= 0 and mstart != estart:
        rstart = mstart + 2
        if rstart < len(rhs) and rhs[rstart] == "{":
            rstart = rstart + 1
            term = "}"
        elif rstart < len(rhs) and rhs[rstart] == "[":
            rstart = rstart + 1
            term = "]"
        else:
            term = string.whitespace

        mend = rstart
        while mend < len(rhs) and rhs[mend] not in term:
            mend = mend + 1

        lhs = rhs[rstart:mend]

        if term in "}]":
            mend = mend + 1

        if term == "]":
            # Assume a missing property expands to an empty string.
            if properties is None:
                value = ""
            else:
                value = properties.get(lhs, "")
        else:
            # We used to treat a missing value as an error, but Qt v4.3.0 has
            # at least one case that refers to an undefined macro.  If qmake
            # handles it then this must be the correct behaviour.
            value = macros.get(lhs, "")

        rhs = rhs[:mstart] + value + rhs[mend:]
        estart = rhs.find("$$(")
        mstart = rhs.find("$$")

    return rhs


def create_wrapper(script, wrapper, gui=0, use_arch=''):
    """Create a platform dependent executable wrapper around a Python script.

    script is the full pathname of the script.
    wrapper is the name of the wrapper file to create.
    gui is non-zero if a GUI enabled version of the interpreter should be used.
    use_arch is the MacOS/X architecture to invoke python with.

    Returns the platform specific name of the wrapper.
    """
    if sys.platform == "win32":
        wrapper = wrapper + ".bat"

    wf = open(wrapper, "w")

    if sys.platform == "win32":
        exe = sys.executable

        if gui:
            exe = exe[:-4] + "w.exe"

        wf.write("@\"%s\" \"%s\" %%1 %%2 %%3 %%4 %%5 %%6 %%7 %%8 %%9\n" % (exe, script))
    elif sys.platform == "darwin":
        # The installation of MacOS's python is a mess that changes from
        # version to version and where sys.executable is useless.

        if gui:
            exe = "pythonw"
        else:
            exe = "python"

        version = sys.version_info
        exe = "%s%d.%d" % (exe, version[0], version[1])

        if use_arch:
            # Note that this may not work with the "standard" interpreter but
            # should with the "pythonX.Y" version.
            exe = "arch -%s %s" % (use_arch, exe)

        wf.write("#!/bin/sh\n")
        wf.write("exec %s %s ${1+\"$@\"}\n" % (exe, script))
    else:
        wf.write("#!/bin/sh\n")
        wf.write("exec %s %s ${1+\"$@\"}\n" % (sys.executable, script))

    wf.close()

    if sys.platform != "win32":
        sbuf = os.stat(wrapper)
        mode = sbuf.st_mode
        mode |= (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

        os.chmod(wrapper, mode)

    return wrapper
