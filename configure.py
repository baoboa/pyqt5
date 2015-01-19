# This script generates the Makefiles for building PyQt5.
#
# Copyright (c) 2014 Riverbank Computing Limited <info@riverbankcomputing.com>
# 
# This file is part of PyQt5.
# 
# This file may be used under the terms of the GNU General Public License
# version 3.0 as published by the Free Software Foundation and appearing in
# the file LICENSE included in the packaging of this file.  Please review the
# following information to ensure the GNU General Public License version 3.0
# requirements will be met: http://www.gnu.org/copyleft/gpl.html.
# 
# If you do not wish to use this file under the terms of the GPL version 3.0
# then you may purchase a commercial license.  For more information contact
# info@riverbankcomputing.com.
# 
# This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
# WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


import glob
import optparse
import os
import shutil
import stat
import sys

try:
    import sysconfig
except ImportError:
    from distutils import sysconfig


# Initialise the constants.
PYQT_VERSION_STR = "5.4"

SIP_MIN_VERSION = '4.16.4'


class ModuleMetadata:
    """ This class encapsulates the meta-data about a PyQt5 module. """

    def __init__(self, qmake_QT=None, qmake_TARGET='', qpy_lib=False, cpp11=False):
        """ Initialise the meta-data. """

        # The values to update qmake's QT variable.
        self.qmake_QT = [] if qmake_QT is None else qmake_QT

        # The value to set qmake's TARGET variable to.  It defaults to the name
        # of the module.
        self.qmake_TARGET = qmake_TARGET

        # Set if there is a qpy support library.
        self.qpy_lib = qpy_lib

        # Set if C++11 support is required.
        self.cpp11 = cpp11


# The module meta-data.
MODULE_METADATA = {
    'dbus':                 ModuleMetadata(qmake_QT=['-gui'],
                                    qmake_TARGET='pyqt5'),
    'QAxContainer':         ModuleMetadata(qmake_QT=['axcontainer']),
    'Qt':                   ModuleMetadata(qmake_QT=['-core', '-gui']),
    'QtBluetooth':          ModuleMetadata(qmake_QT=['bluetooth']),
    'QtCore':               ModuleMetadata(qmake_QT=['-gui'], qpy_lib=True),
    'QtDBus':               ModuleMetadata(qmake_QT=['dbus', '-gui'],
                                    qpy_lib=True),
    'QtDesigner':           ModuleMetadata(qmake_QT=['designer'],
                                    qpy_lib=True),
    'Enginio':              ModuleMetadata(qmake_QT=['enginio']),
    'QtGui':                ModuleMetadata(qpy_lib=True),
    'QtHelp':               ModuleMetadata(qmake_QT=['help']),
    'QtMacExtras':          ModuleMetadata(qmake_QT=['macextras']),
    'QtMultimedia':         ModuleMetadata(qmake_QT=['multimedia']),
    'QtMultimediaWidgets':  ModuleMetadata(
                                    qmake_QT=['multimediawidgets',
                                            'multimedia']),
    'QtNetwork':            ModuleMetadata(qmake_QT=['network', '-gui']),
    'QtOpenGL':             ModuleMetadata(qmake_QT=['opengl']),
    'QtPositioning':        ModuleMetadata(qmake_QT=['positioning']),
    'QtPrintSupport':       ModuleMetadata(qmake_QT=['printsupport']),
    'QtQml':                ModuleMetadata(qmake_QT=['qml'], qpy_lib=True),
    'QtQuick':              ModuleMetadata(qmake_QT=['quick'], qpy_lib=True),
    'QtQuickWidgets':       ModuleMetadata(qmake_QT=['quickwidgets']),
    'QtSensors':            ModuleMetadata(qmake_QT=['sensors']),
    'QtSerialPort':         ModuleMetadata(qmake_QT=['serialport']),
    'QtSql':                ModuleMetadata(qmake_QT=['sql', 'widgets']),
    'QtSvg':                ModuleMetadata(qmake_QT=['svg']),
    'QtTest':               ModuleMetadata(qmake_QT=['testlib', 'widgets']),
    'QtWebChannel':         ModuleMetadata(qmake_QT=['webchannel', 'network']),
    'QtWebEngineWidgets':   ModuleMetadata(
                                    qmake_QT=['webenginewidgets', 'network',
                                            'widgets'],
                                    cpp11=True),
    'QtWebKit':             ModuleMetadata(qmake_QT=['webkit', 'network']),
    'QtWebKitWidgets':      ModuleMetadata(
                                    qmake_QT=['webkitwidgets',
                                            'printsupport']),
    'QtWebSockets':         ModuleMetadata(qmake_QT=['websockets']),
    'QtWidgets':            ModuleMetadata(qmake_QT=['widgets']),
    'QtWinExtras':          ModuleMetadata(qmake_QT=['winextras', 'widgets']),
    'QtX11Extras':          ModuleMetadata(qmake_QT=['x11extras']),
    'QtXml':                ModuleMetadata(qmake_QT=['xml', '-gui']),
    'QtXmlPatterns':        ModuleMetadata(
                                    qmake_QT=['xmlpatterns', '-gui',
                                            'network']),

    # The OpenGL wrappers.
    '_QOpenGLFunctions_1_0':                ModuleMetadata(),
    '_QOpenGLFunctions_1_1':                ModuleMetadata(),
    '_QOpenGLFunctions_1_2':                ModuleMetadata(),
    '_QOpenGLFunctions_1_3':                ModuleMetadata(),
    '_QOpenGLFunctions_1_4':                ModuleMetadata(),
    '_QOpenGLFunctions_1_5':                ModuleMetadata(),
    '_QOpenGLFunctions_2_0':                ModuleMetadata(),
    '_QOpenGLFunctions_2_1':                ModuleMetadata(),
    '_QOpenGLFunctions_3_0':                ModuleMetadata(),
    '_QOpenGLFunctions_3_1':                ModuleMetadata(),
    '_QOpenGLFunctions_3_2_Compatibility':  ModuleMetadata(),
    '_QOpenGLFunctions_3_2_Core':           ModuleMetadata(),
    '_QOpenGLFunctions_3_3_Compatibility':  ModuleMetadata(),
    '_QOpenGLFunctions_3_3_Core':           ModuleMetadata(),
    '_QOpenGLFunctions_4_0_Compatibility':  ModuleMetadata(),
    '_QOpenGLFunctions_4_0_Core':           ModuleMetadata(),
    '_QOpenGLFunctions_4_1_Compatibility':  ModuleMetadata(),
    '_QOpenGLFunctions_4_1_Core':           ModuleMetadata(),
    '_QOpenGLFunctions_4_2_Compatibility':  ModuleMetadata(),
    '_QOpenGLFunctions_4_2_Core':           ModuleMetadata(),
    '_QOpenGLFunctions_4_3_Compatibility':  ModuleMetadata(),
    '_QOpenGLFunctions_4_3_Core':           ModuleMetadata(),
    '_QOpenGLFunctions_ES2':                ModuleMetadata()
}


# The component modules that make up the composite Qt module.  SIP is broken in
# its handling of composite module in that a component module must be %Included
# before it is first %Imported.  In other words, a module must appear before
# any modules that depend on it.
COMPOSITE_COMPONENTS = (
    'QtCore',
    'QtDBus', 'QtGui', 'QtNetwork', 'QtSensors', 'QtSerialPort',
    'QtMultimedia', 'QtQml', 'QtWebKit', 'QtWidgets', 'QtXml', 'QtXmlPatterns',
    'QtAxContainer', 'QtDesigner', 'QtHelp', 'QtMultimediaWidgets', 'QtOpenGL',
        'QtPrintSupport', 'QtQuick', 'QtSql', 'QtSvg', 'QtTest',
    'QtWebKitWidgets', 'QtBluetooth', 'QtMacExtras', 'QtPositioning',
        'QtWinExtras', 'QtX11Extras', 'QtQuickWidgets', 'QtWebSockets',
        'Enginio', 'QtWebChannel', 'QtWebEngineWidgets'
)


def error(msg):
    """ Display an error message and terminate.  msg is the text of the error
    message.
    """

    sys.stderr.write(format("Error: " + msg) + "\n")
    sys.exit(1)


def inform(msg):
    """ Display an information message.  msg is the text of the error message.
    """

    sys.stdout.write(format(msg) + "\n")


def format(msg, left_margin=0, right_margin=78):
    """ Format a message by inserting line breaks at appropriate places.  msg
    is the text of the message.  left_margin is the position of the left
    margin.  right_margin is the position of the right margin.  Returns the
    formatted message.
    """

    curs = left_margin
    fmsg = " " * left_margin

    for w in msg.split():
        l = len(w)
        if curs != left_margin and curs + l > right_margin:
            fmsg = fmsg + "\n" + (" " * left_margin)
            curs = left_margin

        if curs > left_margin:
            fmsg = fmsg + " "
            curs = curs + 1

        fmsg = fmsg + w
        curs = curs + l

    return fmsg


def version_to_sip_tag(version):
    """ Convert a version number to a SIP tag.  version is the version number.
    """

    # Anything after Qt v5 is assumed to be Qt v6.0.
    if version > 0x060000:
        version = 0x060000

    major = (version >> 16) & 0xff
    minor = (version >> 8) & 0xff
    patch = version & 0xff

    return 'Qt_%d_%d_%d' % (major, minor, patch)


def version_to_string(version, parts=3):
    """ Convert an n-part version number encoded as a hexadecimal value to a
    string.  version is the version number.  Returns the string.
    """

    part_list = [str((version >> 16) & 0xff)]

    if parts > 1:
        part_list.append(str((version >> 8) & 0xff))

        if parts > 2:
            part_list.append(str(version & 0xff))

    return '.'.join(part_list)


class ConfigurationFileParser:
    """ A parser for configuration files. """

    def __init__(self, config_file):
        """ Read and parse a configuration file. """

        self._config = {}
        self._extrapolating = []

        cfg = open(config_file)
        line_nr = 0
        last_name = None

        section = ''
        section_config = {}
        self._config[section] = section_config

        for l in cfg:
            line_nr += 1

            # Strip comments.
            l = l.split('#')[0]

            # See if this might be part of a multi-line.
            multiline = (last_name is not None and len(l) != 0 and l[0] == ' ')

            l = l.strip()

            if l == '':
                last_name = None
                continue

            # See if this is a new section.
            if l[0] == '[' and l[-1] == ']':
                section = l[1:-1].strip()
                if section == '':
                    error(
                            "%s:%d: Empty section name." % (
                                    config_file, line_nr))

                if section in self._config:
                    error(
                            "%s:%d: Section '%s' defined more than once." % (
                                    config_file, line_nr, section))

                section_config = {}
                self._config[section] = section_config

                last_name = None
                continue

            parts = l.split('=', 1)
            if len(parts) == 2:
                name = parts[0].strip()
                value = parts[1].strip()
            elif multiline:
                name = last_name
                value = section_config[last_name]
                value += ' ' + l
            else:
                name = value = ''

            if name == '' or value == '':
                error("%s:%d: Invalid line." % (config_file, line_nr))

            section_config[name] = value
            last_name = name

        cfg.close()

    def sections(self):
        """ Return the list of sections, excluding the default one. """

        return [s for s in self._config.keys() if s != '']

    def preset(self, name, value):
        """ Add a preset value to the configuration. """

        self._config[''][name] = value

    def get(self, section, name, default=None):
        """ Get a configuration value while extrapolating. """

        # Get the name from the section, or the default section.
        value = self._config[section].get(name)
        if value is None:
            value = self._config[''].get(name)
            if value is None:
                if default is None:
                    error(
                            "Configuration file references non-existent name "
                            "'%s'." % name)

                return default

        # Handle any extrapolations.
        parts = value.split('%(', 1)
        while len(parts) == 2:
            prefix, tail = parts

            parts = tail.split(')', 1)
            if len(parts) != 2:
                error(
                        "Configuration file contains unterminated "
                        "extrapolated name '%s'." % tail)

            xtra_name, suffix = parts

            if xtra_name in self._extrapolating:
                error(
                        "Configuration file contains a recursive reference to "
                        "'%s'." % xtra_name)

            self._extrapolating.append(xtra_name)
            xtra_value = self.get(section, xtra_name)
            self._extrapolating.pop()

            value = prefix + xtra_value + suffix

            parts = value.split('%(', 1)

        return value

    def getboolean(self, section, name, default):
        """ Get a boolean configuration value while extrapolating. """

        value = self.get(section, name, default)

        # In case the default was returned.
        if isinstance(value, bool):
            return value

        if value in ('True', 'true', '1'):
            return True

        if value in ('False', 'false', '0'):
            return False

        error(
                "Configuration file contains invalid boolean value for "
                "'%s'." % name)

    def getlist(self, section, name, default):
        """ Get a list configuration value while extrapolating. """

        value = self.get(section, name, default)

        # In case the default was returned.
        if isinstance(value, list):
            return value

        return value.split()


class HostPythonConfiguration:
    """ A container for the host Python configuration. """

    def __init__(self):
        """ Initialise the configuration. """

        self.platform = sys.platform
        self.version = sys.hexversion >> 8

        if hasattr(sysconfig, 'get_path'):
            # The modern API.
            self.inc_dir = sysconfig.get_path('include')
            self.module_dir = sysconfig.get_path('platlib')
        else:
            # The legacy distutils API.
            self.inc_dir = sysconfig.get_python_inc(plat_specific=1)
            self.module_dir = sysconfig.get_python_lib(plat_specific=1)

        if sys.platform == 'win32':
            self.bin_dir = sys.exec_prefix
            self.data_dir = sys.prefix
            self.lib_dir = sys.prefix + '\\libs'
        else:
            self.bin_dir = sys.exec_prefix + '/bin'
            self.data_dir = sys.prefix + '/share'
            self.lib_dir = sys.prefix + '/lib'

        # The name of the interpreter used by the pyuic5 wrapper.
        if sys.platform == 'darwin':
            # The installation of MacOS's python is a mess that changes from
            # version to version and where sys.executable is useless.

            py_major = self.version >> 16
            py_minor = (self.version >> 8) & 0xff

            # In Python v3.4 and later there is no pythonw.
            if (py_major == 3 and py_minor >= 4) or py_major >= 4:
                exe = "python"
            else:
                exe = "pythonw"

            self.pyuic_interpreter = '%s%d.%d' % (exe, py_major, py_minor)
        else:
            self.pyuic_interpreter = sys.executable


class TargetQtConfiguration:
    """ A container for the target Qt configuration. """

    def __init__(self, qmake):
        """ Initialise the configuration.  qmake is the full pathname of the
        qmake executable that will provide the configuration.
        """

        inform("Querying qmake about your Qt installation...")

        pipe = os.popen(' '.join([qmake, '-query']))

        for l in pipe:
            l = l.strip()

            tokens = l.split(':', 1)
            if isinstance(tokens, list):
                if len(tokens) != 2:
                    error("Unexpected output from qmake: '%s'\n" % l)

                name, value = tokens
            else:
                name = tokens
                value = None

            name = name.replace('/', '_')

            setattr(self, name, value)

        pipe.close()


class TargetConfiguration:
    """ A container for configuration information about the target. """

    def __init__(self):
        """ Initialise the configuration with default values. """

        # Values based on the host Python configuration.
        py_config = HostPythonConfiguration()
        self.py_inc_dir = py_config.inc_dir
        self.py_lib_dir = py_config.lib_dir
        self.py_platform = py_config.platform
        self.py_version = py_config.version
        self.pyqt_bin_dir = py_config.bin_dir
        self.pyqt_module_dir = py_config.module_dir
        self.pyqt_sip_dir = os.path.join(py_config.data_dir, 'sip', 'PyQt5')
        self.pyuic_interpreter = py_config.pyuic_interpreter

        # The qmake spec we want to use.
        if self.py_platform == 'win32':
            if self.py_version >= 0x030300:
                self.qmake_spec = 'win32-msvc2010'
            elif self.py_version >= 0x020600:
                self.qmake_spec = 'win32-msvc2008'
            elif self.py_version >= 0x020400:
                self.qmake_spec = 'win32-msvc.net'
            else:
                self.qmake_spec = 'win32-msvc'
        else:
            # Use the Qt default.  (We may update it for OS/X later.)
            self.qmake_spec = ''

        self.default_qmake_spec = ''

        # Remaining values.
        self.dbus_inc_dirs = []
        self.dbus_lib_dirs = []
        self.dbus_libs = []
        self.debug = False
        self.designer_plugin_dir = ''
        self.no_designer_plugin = False
        self.no_docstrings = False
        self.no_qml_plugin = False
        self.no_tools = False
        self.prot_is_public = (self.py_platform.startswith('linux') or self.py_platform == 'darwin')
        self.qmake = self._find_exe('qmake')
        self.qmake_variables = []
        self.py_pylib_dir = ''
        self.py_pylib_lib = ''
        self.py_pyshlib = ''
        self.pydbus_inc_dir = ''
        self.pydbus_module_dir = ''
        self.pyqt_disabled_features = []
        self.pyqt_modules = []
        self.qml_plugin_dir = ''
        self.qsci_api = False
        self.qsci_api_dir = ''
        self.qt_licensee = ''
        self.qt_shared = False
        self.qt_version = 0
        self.sip = self._find_exe('sip')
        self.sip_inc_dir = ''
        self.static = False
        self.sysroot = ''
        self.vend_enabled = False
        self.vend_inc_dir = ''
        self.vend_lib_dir = ''

    def from_configuration_file(self, config_file):
        """ Initialise the configuration with values from a file.  config_file
        is the name of the configuration file.
        """

        inform("Reading configuration from %s..." % config_file)

        parser = ConfigurationFileParser(config_file)

        # Populate some presets from the command line.
        version = version_to_string(self.py_version).split('.')
        parser.preset('py_major', version[0])
        parser.preset('py_minor', version[1])

        parser.preset('sysroot', self.sysroot)

        # Find the section corresponding to the version of Qt.
        qt_major = self.qt_version >> 16
        section = None
        latest_section = -1

        for name in parser.sections():
            parts = name.split()
            if len(parts) != 2 or parts[0] != 'Qt':
                continue

            section_qt_version = version_from_string(parts[1])
            if section_qt_version is None:
                continue

            # Major versions must match.
            if section_qt_version >> 16 != self.qt_version >> 16:
                continue

            # It must be no later that the version of Qt.
            if section_qt_version > self.qt_version:
                continue

            # Save it if it is the latest so far.
            if section_qt_version > latest_section:
                section = name
                latest_section = section_qt_version

        if section is None:
            error("%s does not define a section that covers Qt v%s." % (config_file, version_to_string(self.qt_version)))

        self.py_platform = parser.get(section, 'py_platform', self.py_platform)
        self.py_inc_dir = parser.get(section, 'py_inc_dir', self.py_inc_dir)
        self.py_pylib_dir = parser.get(section, 'py_pylib_dir',
                self.py_pylib_dir)
        self.py_pylib_lib = parser.get(section, 'py_pylib_lib',
                self.py_pylib_lib)
        self.py_pyshlib = parser.get(section, 'py_pyshlib', self.py_pyshlib)

        self.qt_shared = parser.getboolean(section, 'qt_shared',
                self.qt_shared)

        self.pyqt_disabled_features = parser.getlist(section,
                'pyqt_disabled_features', self.pyqt_disabled_features)
        self.pyqt_modules = parser.getlist(section, 'pyqt_modules',
                self.pyqt_modules)
        self.pyqt_module_dir = parser.get(section, 'pyqt_module_dir',
                self.pyqt_module_dir)
        self.pyqt_bin_dir = parser.get(section, 'pyqt_bin_dir',
                self.pyqt_bin_dir)
        self.pyqt_sip_dir = parser.get(section, 'pyqt_sip_dir',
                self.pyqt_sip_dir)
        self.pyuic_interpreter = parser.get(section, 'pyuic_interpreter',
                self.pyuic_interpreter)
 
    def from_introspection(self, verbose, debug):
        """ Initialise the configuration by introspecting the system. """

        inform("Determining the details of your Qt installation...")

        out_file = 'qtdetail.out'

        source = '''#include <QCoreApplication>
#include <QFile>
#include <QLibraryInfo>
#include <QTextStream>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QFile outf("%s");

    if (!outf.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text))
        return 1;

    QTextStream out(&outf);

    out << QLibraryInfo::licensee() << '\\n';

#if defined(QT_SHARED) || defined(QT_DLL)
    out << "shared\\n";
#else
    out << "static\\n";
#endif

    // Determine which features should be disabled.

#if defined(QT_NO_ACCESSIBILITY)
    out << "PyQt_Accessibility\\n";
#endif

#if defined(QT_NO_SESSIONMANAGER)
    out << "PyQt_SessionManager\\n";
#endif

#if defined(QT_NO_SSL)
    out << "PyQt_SSL\\n";
#endif

#if defined(QT_NO_PRINTDIALOG)
    out << "PyQt_PrintDialog\\n";
#endif

#if defined(QT_NO_PRINTER)
    out << "PyQt_Printer\\n";
#endif

#if defined(QT_NO_PRINTPREVIEWDIALOG)
    out << "PyQt_PrintPreviewDialog\\n";
#endif

#if defined(QT_NO_PRINTPREVIEWWIDGET)
    out << "PyQt_PrintPreviewWidget\\n";
#endif

#if defined(QT_NO_RAWFONT)
    out << "PyQt_RawFont\\n";
#endif

#if defined(QT_NO_OPENGL)
    out << "PyQt_OpenGL\\n";
    out << "PyQt_Desktop_OpenGL\\n";
#elif defined(QT_OPENGL_ES_2)
    out << "PyQt_Desktop_OpenGL\\n";
#endif

#if QT_VERSION < 0x050200
// This is the test used in qglobal.h in Qt prior to v5.2.  In v5.2 and later
// qreal is always double.
#if defined(QT_NO_FPU) || defined(Q_PROCESSOR_ARM) || defined(Q_OS_WINCE)
    out << "PyQt_qreal_double\\n";
#endif
#endif

    return 0;
}
''' % out_file

        cmd = compile_qt_program(self, verbose, 'qtdetail', source, 'QtCore',
                debug=debug)

        if cmd is None:
            error("Failed to determine the detail of your Qt installation. Try again using the --verbose flag to see more detail about the problem.")

        # Create the output file, first making sure it doesn't exist.
        remove_file(out_file)
        run_command(cmd, verbose)

        if not os.access(out_file, os.F_OK):
            error("%s failed to create %s. Make sure your Qt installation is correct." % (cmd, out_file))

        # Read the details.
        f = open(out_file)
        lines = f.read().strip().split('\n')
        f.close()

        self.qt_licensee = lines[0]
        self.qt_shared = (lines[1] == 'shared')
        self.pyqt_disabled_features = lines[2:]

        # Get the details of the Python interpreter library.
        py_major = self.py_version >> 16
        py_minor = (self.py_version >> 8) & 0x0ff

        if sys.platform == 'win32':
            debug_suffix = get_win32_debug_suffix(debug)
            pylib_lib = 'python%d%d%s' % (py_major, py_minor, debug_suffix)

            pylib_dir = self.py_lib_dir

            # Assume Python is a DLL on Windows.
            pyshlib = pylib_lib
        else:
            abi = getattr(sys, 'abiflags', '')
            pylib_lib = 'python%d.%d%s' % (py_major, py_minor, abi)
            pylib_dir = pyshlib = ''

            # Use distutils to get the additional configuration.
            from distutils.sysconfig import get_config_vars
            ducfg = get_config_vars()

            config_args = ducfg.get('CONFIG_ARGS', '')

            dynamic_pylib = '--enable-shared' in config_args
            if not dynamic_pylib:
                dynamic_pylib = '--enable-framework' in config_args

            if dynamic_pylib:
                pyshlib = ducfg.get('LDLIBRARY', '')

                exec_prefix = ducfg['exec_prefix']
                multiarch = ducfg.get('MULTIARCH', '')
                libdir = ducfg['LIBDIR']

                if glob.glob('%s/lib/libpython%d.%d*' % (exec_prefix, py_major, py_minor)):
                    pylib_dir = exec_prefix + '/lib'
                elif multiarch != '' and glob.glob('%s/lib/%s/libpython%d.%d*' % (exec_prefix, multiarch, py_major, py_minor)):
                    pylib_dir = exec_prefix + '/lib/' + multiarch
                elif glob.glob('%s/libpython%d.%d*' % (libdir, py_major, py_minor)):
                    pylib_dir = libdir

        self.py_pylib_dir = pylib_dir
        self.py_pylib_lib = pylib_lib
        self.py_pyshlib = pyshlib

        # Apply sysroot where necessary.
        if self.sysroot != '':
            self.py_inc_dir = self._apply_sysroot(self.py_inc_dir)
            self.py_pylib_dir = self._apply_sysroot(self.py_pylib_dir)
            self.pyqt_bin_dir = self._apply_sysroot(self.pyqt_bin_dir)
            self.pyqt_module_dir = self._apply_sysroot(self.pyqt_module_dir)
            self.pyqt_sip_dir = self._apply_sysroot(self.pyqt_sip_dir)

    def _apply_sysroot(self, dir_name):
        """ Replace any leading sys.prefix of a directory name with sysroot.
        """

        if dir_name.startswith(sys.prefix):
            dir_name = self.sysroot + dir_name[len(sys.prefix):]

        return dir_name

    def get_qt_configuration(self):
        """ Get the Qt configuration that can be extracted from qmake. """

        # Query qmake.
        qt_config = TargetQtConfiguration(self.qmake)

        self.qt_version = 0
        try:
            qt_version_str = qt_config.QT_VERSION
            for v in qt_version_str.split('.'):
                self.qt_version *= 256
                self.qt_version += int(v)
        except AttributeError:
            qt_version_str = "3"

        # Check the Qt version number as soon as possible.
        if self.qt_version < 0x050000:
            if sys.platform == 'win32':
                error(
                        "PyQt5 requires Qt v5.0 or later. You seem to be "
                        "using v%s. Make sure the correct version of qmake is "
                        "on your PATH." % qt_version_str)
            else:
                error(
                        "PyQt5 requires Qt v5.0 or later. You seem to be "
                        "using v%s. Use the --qmake flag to specify the "
                        "correct version of qmake." % qt_version_str)

        self.designer_plugin_dir = qt_config.QT_INSTALL_PLUGINS + '/designer'
        self.qml_plugin_dir = qt_config.QT_INSTALL_PLUGINS + '/PyQt5'

        if self.sysroot == '':
            self.sysroot = qt_config.QT_SYSROOT

        # By default, install the API file if QScintilla seems to be installed
        # in the default location.
        self.qsci_api_dir = os.path.join(qt_config.QT_INSTALL_DATA, 'qsci')
        self.qsci_api = os.path.isdir(self.qsci_api_dir)

        # Save the default qmake spec. and finalise the value we want to use.
        self.default_qmake_spec = qt_config.QMAKE_SPEC

        if self.qmake_spec == '':
            self.qmake_spec = self.default_qmake_spec

        # The binary OS/X Qt installer defaults to XCode.  If this is what we
        # might have then use macx-clang.
        if self.qmake_spec == 'macx-xcode':
            # This will exist (and we can't check anyway).
            self.qmake_spec = 'macx-clang'

    def post_configuration(self):
        """ Handle any remaining default configuration after having read a
        configuration file or introspected the system.
        """

        # The platform may have changed so update the default.
        if self.py_platform.startswith('linux') or self.py_platform == 'darwin':
            self.prot_is_public = True

        self.sip_inc_dir = self.py_inc_dir
        self.vend_inc_dir = self.py_inc_dir
        self.vend_lib_dir = self.py_lib_dir

    def apply_pre_options(self, opts):
        """ Apply options from the command line that influence subsequent
        configuration.  opts are the command line options.
        """

        # Get the target Python version.
        if opts.target_py_version is not None:
            self.py_version = opts.target_py_version

        # Get the system root.
        if opts.sysroot is not None:
            self.sysroot = opts.sysroot

        # Determine how to run qmake.
        try:
            qmake = opts.qmake
        except AttributeError:
            # Windows.
            qmake = None

        if qmake is not None:
            self.qmake = qmake
        elif self.qmake is None:
            # Under Windows qmake and the Qt DLLs must be on the system PATH
            # otherwise the dynamic linker won't be able to resolve the
            # symbols.  On other systems we assume we can just run qmake by
            # using its full pathname.
            if sys.platform == 'win32':
                error("Make sure you have a working Qt qmake on your PATH.")
            else:
                error(
                        "Use the --qmake argument to explicitly specify a "
                        "working Qt qmake.")

        if opts.qmakespec is not None:
            self.qmake_spec = opts.qmakespec

    def apply_post_options(self, opts):
        """ Apply options from the command line that override the previous
        configuration.  opts are the command line options.
        """

        if opts.assumeshared:
            self.qt_shared = True

        if opts.bindir is not None:
            self.pyqt_bin_dir = opts.bindir

        if opts.debug:
            self.debug = True

        if opts.designerplugindir is not None:
            self.designer_plugin_dir = opts.designerplugindir

        if opts.destdir is not None:
            self.pyqt_module_dir = opts.destdir

        if len(opts.modules) > 0:
            self.pyqt_modules = opts.modules

        if opts.nodesignerplugin:
            self.no_designer_plugin = True

        if opts.nodocstrings:
            self.no_docstrings = True

        if opts.noqmlplugin:
            self.no_qml_plugin = True

        if opts.notools:
            self.no_tools = True

        if opts.protispublic is not None:
            self.prot_is_public = opts.protispublic

        if opts.pydbusincdir is not None:
            self.pydbus_inc_dir = opts.pydbusincdir

        if opts.pyuicinterpreter is not None:
            self.pyuic_interpreter = opts.pyuicinterpreter

        if opts.qsciapidir is not None:
            self.qsci_api_dir = opts.qsciapidir

            # Assume we want to install the API file if we have provided an
            # installation directory.
            self.qsci_api = True

        if opts.qsciapi is not None:
            self.qsci_api = opts.qsciapi

        if opts.qsciapidir is not None:
            self.qsci_api_dir = opts.qsciapidir

        if opts.sip is not None:
            self.sip = opts.sip

        if opts.sipdir is not None:
            self.pyqt_sip_dir = opts.sipdir
        elif not opts.install_sipfiles:
            self.pyqt_sip_dir = ''

        if opts.sipincdir is not None:
            self.sip_inc_dir = opts.sipincdir

        if opts.static:
            self.static = True

        if opts.vendenabled:
            self.vend_enabled = True

        if opts.vendincdir is not None:
            self.vend_inc_dir = opts.vendincdir

        if opts.vendlibdir is not None:
            self.vend_lib_dir = opts.vendlibdir

        # Handle any conflicts.
        if not self.qt_shared:
            if not self.static:
                error(
                        "Qt has been built as static libraries so the "
                        "--static argument should be used.")

        if self.vend_enabled and self.static:
            error(
                    "Using the VendorID package when building static "
                    "libraries makes no sense.")

    def get_pylib_link_arguments(self):
        """ Return a string to append to qmake's LIBS macro to link against the
        Python interpreter library.
        """

        return qmake_quote('-L' + self.py_pylib_dir) + ' -l' + self.py_pylib_lib

    @staticmethod
    def _find_exe(exe):
        """ Find an executable, ie. the first on the path. """

        try:
            path = os.environ['PATH']
        except KeyError:
            path = ''

        if sys.platform == 'win32':
            exe = exe + '.exe'

        for d in path.split(os.pathsep):
            exe_path = os.path.join(d, exe)

            if os.path.isfile(exe_path) and os.access(exe_path, os.X_OK):
                return exe_path

        return None


def create_optparser(target_config):
    """ Create the parser for the command line.  target_config is the target
    configuration containing default values.
    """

    def store_abspath(option, opt_str, value, parser):
        setattr(parser.values, option.dest, os.path.abspath(value))

    def store_abspath_dir(option, opt_str, value, parser):
        if not os.path.isdir(value):
            raise optparse.OptionValueError("'%s' is not a directory" % value)
        setattr(parser.values, option.dest, os.path.abspath(value))

    def store_abspath_exe(option, opt_str, value, parser):
        if not os.access(value, os.X_OK):
            raise optparse.OptionValueError("'%s' is not an executable" % value)
        setattr(parser.values, option.dest, os.path.abspath(value))

    def store_abspath_file(option, opt_str, value, parser):
        if not os.path.isfile(value):
            raise optparse.OptionValueError("'%s' is not a file" % value)
        setattr(parser.values, option.dest, os.path.abspath(value))

    def store_version(option, opt_str, value, parser):
        version = version_from_string(value)
        if version is None:
            raise optparse.OptionValueError(
                    "'%s' is not a valid version number" % value)
        setattr(parser.values, option.dest, version)

    p = optparse.OptionParser(usage="python %prog [opts] [name=value] "
            "[name+=value]", version=PYQT_VERSION_STR)

    # Note: we don't use %default to be compatible with Python 2.3.
    p.add_option("--static", "-k", dest='static', default=False,
            action='store_true',
            help="build modules as static libraries")
    p.add_option("--no-docstrings", dest='nodocstrings', default=False,
            action='store_true',
            help="disable the generation of docstrings")
    p.add_option("--trace", "-r", dest='tracing', default=False,
            action='store_true',
            help="build modules with tracing enabled")
    p.add_option("--debug", "-u", dest='debug', default=False,
            action='store_true',
            help="build modules with debugging symbols")
    p.add_option("--verbose", "-w", dest='verbose', default=False,
            action='store_true',
            help="enable verbose output during configuration")

    p.add_option("--concatenate", "-c", dest='concat', default=False,
            action='store_true',
            help="concatenate each module's C++ source files")
    p.add_option("--concatenate-split", "-j", dest='split', type='int',
            default=1, metavar="N",
            help="split the concatenated C++ source files into N pieces "
                    "[default: 1]")

    # Configuration.
    g = optparse.OptionGroup(p, title="Configuration")
    g.add_option("--confirm-license", dest='license_confirmed', default=False,
            action='store_true',
            help="confirm acceptance of the license")
    g.add_option("--target-py-version", dest='target_py_version',
            type='string', action='callback', callback=store_version,
            metavar="VERSION",
            help="the major.minor version of the target Python [default: "
                    "%s]" % version_to_string(target_config.py_version,
                            parts=2))
    g.add_option("--sysroot", dest='sysroot', type='string', action='callback',
            callback=store_abspath_dir, metavar="DIR",
            help="DIR is the target system root directory")
    g.add_option("--spec", dest='qmakespec', default=None, action='store',
            metavar="SPEC",
            help="pass -spec SPEC to qmake [default: %s]" % "don't pass -spec" if target_config.qmake_spec == '' else target_config.qmake_spec)
    g.add_option("--enable", "-e", dest='modules', default=[], action='append',
            metavar="MODULE",
            help="enable checks for the specified MODULE [default: checks for "
                    "all modules will be enabled]")
    g.add_option("--no-designer-plugin", dest='nodesignerplugin',
            default=False, action='store_true',
            help="disable the building of the Python plugin for Qt Designer "
                    "[default: enabled]")
    g.add_option("--no-qml-plugin", dest='noqmlplugin', default=False,
            action='store_true',
            help="disable the building of the Python plugin for qmlscene "
                    "[default: enabled]")
    g.add_option("--assume-shared", dest='assumeshared', default=False,
            action='store_true',
            help="assume that the Qt libraries have been built as shared "
                    "libraries [default: check]")
    g.add_option("--no-timestamp", "-T", dest='notimestamp', default=False,
            action='store_true',
            help="suppress timestamps in the header comments of generated "
                    "code [default: include timestamps]")
    g.add_option("--configuration", dest='config_file', type='string',
            action='callback', callback=store_abspath_file, metavar="FILE",
            help="FILE contains the target configuration")

    g.add_option("--protected-is-public", dest='protispublic', default=None,
            action='store_true',
            help="enable building with 'protected' redefined as 'public' "
                    "[default: %s]" %
                            "enabled" if target_config.prot_is_public
                            else "disabled")
    g.add_option("--protected-not-public", dest='protispublic', default=None,
            action='store_false',
            help="disable building with 'protected' redefined as 'public'")

    g.add_option("--pyuic5-interpreter", dest='pyuicinterpreter',
            type='string', default=None, action='callback',
            callback=store_abspath_exe, metavar="FILE",
            help="the name of the Python interpreter to run the pyuic5 "
                    "wrapper is FILE [default: %s]" %
                            target_config.pyuic_interpreter)

    if sys.platform != 'win32':
        g.add_option("--qmake", "-q", dest='qmake', type='string',
                default=None, action='callback', callback=store_abspath_exe,
                metavar="FILE",
                help="the pathname of qmake is FILE [default: "
                        "%s]" % (target_config.qmake or "search PATH"))

    g.add_option("--sip", dest='sip', type='string', default=None,
            action='callback', callback=store_abspath_exe, metavar="FILE",
            help="the pathname of sip is FILE [default: "
                    "%s]" % (target_config.sip or "None"))
    g.add_option("--sip-incdir", dest='sipincdir', type='string',
            default=None, action='callback', callback=store_abspath_dir,
            metavar="DIR",
            help="the directory containing the sip.h header file is DIR "
                    "[default: %s]" % target_config.sip_inc_dir)

    g.add_option("--dbus", "-s", dest='pydbusincdir', type='string',
            default=None, action='callback', callback=store_abspath_dir,
            metavar="DIR",
            help="the directory containing the dbus/dbus-python.h header is "
            "DIR [default: supplied by pkg-config]")
    p.add_option_group(g)

    # Installation.
    g = optparse.OptionGroup(p, title="Installation")
    g.add_option("--bindir", "-b", dest='bindir', type='string', default=None,
            action='callback', callback=store_abspath, metavar="DIR",
            help="install pyuic5, pyrcc5 and pylupdate5 in DIR [default: "
                    "%s]" % target_config.pyqt_bin_dir)
    g.add_option("--destdir", "-d", dest='destdir', type='string',
            default=None, action='callback', callback=store_abspath,
            metavar="DIR",
            help="install the PyQt5 Python package in DIR [default: "
                    "%s]" % target_config.pyqt_module_dir)
    g.add_option("--designer-plugindir", dest='designerplugindir',
            type='string', default=None, action='callback',
            callback=store_abspath, metavar="DIR",
            help="install the Python plugin for Qt Designer in DIR "
                    "[default: QT_INSTALL_PLUGINS/designer]")
    g.add_option("--qml-plugindir", dest='qmlplugindir', type='string',
            default=None, action='callback', callback=store_abspath,
            metavar="DIR",
            help="install the Python plugin for qmlscene in DIR "
                    "[default: QT_INSTALL_PLUGINS/PyQt5]")
    g.add_option("--no-sip-files", action="store_false", default=True,
            dest="install_sipfiles", help="disable the installation of the "
            ".sip files [default: enabled]")
    g.add_option("--sipdir", "-v", dest='sipdir', type='string', default=None,
            action='callback', callback=store_abspath, metavar="DIR",
            help="install the PyQt5 .sip files in DIR [default: %s]" %
                    target_config.pyqt_sip_dir)
    g.add_option("--no-tools", action="store_true", default=False,
            dest="notools",
            help="disable the building of pyuic5, pyrcc5 and pylupdate5 "
                    "[default: enabled]")
    p.add_option_group(g)

    # Vendor ID.
    g = optparse.OptionGroup(p, title="VendorID support")
    g.add_option("--vendorid", "-i", dest='vendenabled', default=False,
            action='store_true',
            help="enable checking of signed interpreters using the VendorID "
                    "package [default: %s]" %
                    "enabled" if target_config.vend_enabled else "disabled")
    g.add_option("--vendorid-incdir", "-l", dest='vendincdir', type='string',
            default=None, action='callback', callback=store_abspath_dir,
            metavar="DIR",
            help="the VendorID header file is installed in DIR [default: "
                    "%s]" % target_config.vend_inc_dir)
    g.add_option("--vendorid-libdir", "-m", dest='vendlibdir', type='string',
            default=None, action='callback', callback=store_abspath_dir,
            metavar="DIR",
            help="the VendorID library is installed in DIR [default: "
                    "%s]" % target_config.vend_lib_dir)
    p.add_option_group(g)

    # QScintilla.
    g = optparse.OptionGroup(p, title="QScintilla support")
    g.add_option("--qsci-api", "-a", dest='qsciapi', default=None,
            action='store_true',
            help="always install the PyQt API file for QScintilla [default: "
                    "install only if QScintilla installed]")
    g.add_option("--no-qsci-api", dest='qsciapi', default=None,
            action='store_false',
            help="do not install the PyQt API file for QScintilla [default: "
                    "install only if QScintilla installed]")
    g.add_option("--qsci-api-destdir", "-n", dest='qsciapidir', type='string',
            default=None, action='callback', callback=store_abspath,
            metavar="DIR",
            help="install the PyQt5 API file for QScintilla in DIR [default: "
                    "QT_INSTALL_DATA/qsci]")
    p.add_option_group(g)

    return p


def check_modules(target_config, verbose):
    """ Check which modules can be built and update the target configuration
    accordingly.  target_config is the target configuration.  verbose is set if
    the output is to be displayed.
    """

    target_config.pyqt_modules.append('QtCore')

    check_module(target_config, verbose, 'QtGui', 'qfont.h', 'new QFont()')
    check_module(target_config, verbose, 'QtHelp', 'qhelpengine.h',
            'new QHelpEngine("foo")')
    check_module(target_config, verbose, 'QtMultimedia', 'QAudioDeviceInfo',
            'new QAudioDeviceInfo()')
    check_module(target_config, verbose, 'QtMultimediaWidgets', 'QVideoWidget',
            'new QVideoWidget()')
    check_module(target_config, verbose, 'QtNetwork', 'qhostaddress.h',
            'new QHostAddress()')
    check_module(target_config, verbose, 'QtOpenGL', 'qgl.h',
            'new QGLWidget()')
    check_module(target_config, verbose, 'QtPrintSupport', 'qprinter.h',
            'new QPrinter()')
    check_module(target_config, verbose, 'QtQml', 'qjsengine.h',
            'new QJSEngine()')
    check_module(target_config, verbose, 'QtQuick', 'qquickwindow.h',
            'new QQuickWindow()')
    check_module(target_config, verbose, 'QtSql', 'qsqldatabase.h',
            'new QSqlDatabase()')
    check_module(target_config, verbose, 'QtSvg', 'qsvgwidget.h',
            'new QSvgWidget()')
    check_module(target_config, verbose, 'QtTest', 'QtTest',
            'QTest::qSleep(0)')
    check_module(target_config, verbose, 'QtWebKit', 'qwebkitglobal.h',
            'qWebKitVersion()')
    check_module(target_config, verbose, 'QtWebKitWidgets', 'qwebpage.h',
            'new QWebPage()')
    check_module(target_config, verbose, 'QtWidgets', 'qwidget.h',
            'new QWidget()')
    check_module(target_config, verbose, 'QtXml', 'qdom.h',
            'new QDomDocument()')
    check_module(target_config, verbose, 'QtXmlPatterns', 'qxmlname.h',
            'new QXmlName()')

    if target_config.qt_shared:
        check_module(target_config, verbose, 'QtDesigner', 'QExtensionFactory',
                'new QExtensionFactory()')
    else:
        inform("QtDesigner module disabled with static Qt libraries.")

    check_module(target_config, verbose, 'QAxContainer', 'qaxobject.h',
            'new QAxObject()')

    check_module(target_config, verbose, 'QtDBus', 'qdbusconnection.h',
            'QDBusConnection::systemBus()')

    if target_config.qt_version >= 0x050100:
        check_5_1_modules(target_config, verbose)

    if target_config.qt_version >= 0x050200:
        check_5_2_modules(target_config, verbose)

    if target_config.qt_version >= 0x050300:
        check_5_3_modules(target_config, verbose)

    if target_config.qt_version >= 0x050400:
        check_5_4_modules(target_config, verbose)


def check_5_1_modules(target_config, verbose):
    """ Check which modules introduced in Qt v5.1 can be built and update the
    target configuration accordingly.  target_config is the target
    configuration.  verbose is set if the output is to be displayed.
    """

    # Check the OpenGL functions.
    if 'PyQt_OpenGL' in target_config.pyqt_disabled_features:
        pass
    elif 'PyQt_Desktop_OpenGL' in target_config.pyqt_disabled_features:
        check_module(target_config, verbose, '_QOpenGLFunctions_ES2',
                'qopenglfunctions_es2.h', 'new QOpenGLFunctions_ES2()')
    else:
        desktop_versions = (
                '1_0', '1_1', '1_2', '1_3', '1_4', '1_5',
                '2_0', '2_1',
                '3_0', '3_1',
                '3_2_Compatibility', '3_2_Core',
                '3_3_Compatibility', '3_3_Core',
                '4_0_Compatibility', '4_0_Core',
                '4_1_Compatibility', '4_1_Core',
                '4_2_Compatibility', '4_2_Core',
                '4_3_Compatibility', '4_3_Core')

        for ogl in desktop_versions:
            ogl_module = '_QOpenGLFunctions_' + ogl
            ogl_h = 'qopenglfunctions_' + ogl.lower() + '.h'
            ogl_ctor = 'new QOpenGLFunctions_' + ogl + '()'

            check_module(target_config, verbose, ogl_module, ogl_h, ogl_ctor)

    check_module(target_config, verbose, 'QtSensors', 'qsensor.h',
            'new QSensor(QByteArray())')
    check_module(target_config, verbose, 'QtSerialPort', 'qserialport.h',
            'new QSerialPort()')
    check_module(target_config, verbose, 'QtX11Extras', 'QX11Info',
            'QX11Info::display()')


def check_5_2_modules(target_config, verbose):
    """ Check which modules introduced in Qt v5.2 can be built and update the
    target configuration accordingly.  target_config is the target
    configuration.  verbose is set if the output is to be displayed.
    """

    check_module(target_config, verbose, 'QtBluetooth', 'qbluetoothaddress.h',
            'new QBluetoothAddress()')
    check_module(target_config, verbose, 'QtMacExtras', 'qmacpasteboardmime.h',
            'class Foo : public QMacPasteboardMime {}')
    check_module(target_config, verbose, 'QtPositioning', 'qgeoaddress.h',
            'new QGeoAddress()')
    check_module(target_config, verbose, 'QtWinExtras', 'QtWin',
            'QtWin::isCompositionEnabled()')


def check_5_3_modules(target_config, verbose):
    """ Check which modules introduced in Qt v5.3 can be built and update the
    target configuration accordingly.  target_config is the target
    configuration.  verbose is set if the output is to be displayed.
    """

    check_module(target_config, verbose, 'QtQuickWidgets', 'qquickwidget.h',
            'new QQuickWidget()')
    check_module(target_config, verbose, 'QtWebSockets', 'qwebsocket.h',
            'new QWebSocket()')
    check_module(target_config, verbose, 'Enginio', 'enginioclient.h',
            'new EnginioClient()')


def check_5_4_modules(target_config, verbose):
    """ Check which modules introduced in Qt v5.4 can be built and update the
    target configuration accordingly.  target_config is the target
    configuration.  verbose is set if the output is to be displayed.
    """

    check_module(target_config, verbose, 'QtWebChannel', 'qwebchannel.h',
            'new QWebChannel()')
    check_module(target_config, verbose, 'QtWebEngineWidgets',
            'qwebengineview.h', 'new QWebEngineView()')


def generate_makefiles(target_config, verbose, no_timestamp, parts, tracing):
    """ Generate the makefiles to build everything.  target_config is the
    target configuration.  verbose is set if the output is to be displayed.
    no_timestamp is set if the .sip files should exclude the timestamp.  parts
    is the number of parts the generated code should be split into.  tracing is
    set if the generated code should include tracing calls.
    """

    # For the top-level .pro file.
    toplevel_pro = 'PyQt5.pro'
    subdirs = []

    # Set the SIP platform, version and feature flags.
    sip_flags = get_sip_flags(target_config)

    # Go through the modules.
    for mname in target_config.pyqt_modules:
        metadata = MODULE_METADATA[mname]

        if metadata.qpy_lib:
            sp_qpy_dir = source_path('qpy', mname)

            qpy_c_sources = [os.path.relpath(f, mname)
                    for f in glob.glob(os.path.join(sp_qpy_dir, '*.c'))]
            qpy_cpp_sources = [os.path.relpath(f, mname)
                    for f in glob.glob(os.path.join(sp_qpy_dir, '*.cpp'))]
            qpy_headers = [os.path.relpath(f, mname)
                    for f in glob.glob(os.path.join(sp_qpy_dir, '*.h'))]

            qpy_sources = qpy_c_sources + qpy_cpp_sources
        else:
            qpy_sources = []
            qpy_headers = []

        generate_sip_module_code(target_config, verbose, no_timestamp, parts,
                tracing, mname, sip_flags, qpy_sources, qpy_headers)
        subdirs.append(mname)

    # Generate the composite module.
    qtmod_sipdir = os.path.join('sip', 'Qt')
    mk_clean_dir(qtmod_sipdir)

    qtmod_sipfile = os.path.join(qtmod_sipdir, 'Qtmod.sip')
    f = open_for_writing(qtmod_sipfile)

    f.write('''%CompositeModule PyQt5.Qt

''')

    for mname in COMPOSITE_COMPONENTS:
        if mname in target_config.pyqt_modules:
            f.write('%%Include %s/%smod.sip\n' % (mname, mname))

    f.close()

    generate_sip_module_code(target_config, verbose, no_timestamp, parts,
            tracing, 'Qt', sip_flags)
    subdirs.append('Qt')

    if not target_config.no_tools:
        # Generate pylupdate5 and pyrcc5.
        for tool in ('pylupdate', 'pyrcc'):
            generate_application_makefile(target_config, verbose, tool)
            subdirs.append(tool)

        # Generate the pyuic5 wrapper.
        pyuic_wrapper = generate_pyuic5_wrapper(target_config)

    # Generate the Qt Designer plugin.
    if not target_config.no_designer_plugin and 'QtDesigner' in target_config.pyqt_modules:
        if generate_plugin_makefile(target_config, verbose, 'designer', target_config.designer_plugin_dir, "Qt Designer"):
            subdirs.append('designer')

    # Generate the qmlscene plugin.
    if not target_config.no_qml_plugin and 'QtQml' in target_config.pyqt_modules:
        if generate_plugin_makefile(target_config, verbose, 'qmlscene', target_config.qml_plugin_dir, "qmlscene"):
            subdirs.append('qmlscene')

            rewrite_qmldir(target_config, 'Charts',
                    source_path('examples', 'quick', 'tutorials', 'extending',
                            'chapter6-plugins'))

    # Generate the QScintilla API file.
    if target_config.qsci_api:
        inform("Generating the QScintilla API file...")
        f = open_for_writing('PyQt5.api')

        for mname in target_config.pyqt_modules:
            api = open(mname + '.api')

            for l in api:
                f.write('PyQt5.' + l)

            api.close()
            os.remove(mname + '.api')

        f.close()

    # Generate the Python dbus module.
    if target_config.pydbus_module_dir != '':
        mname = 'dbus'

        mk_dir(mname)
        sp_src_dir = source_path(mname)

        includepath = ' '.join(target_config.dbus_inc_dirs)

        lib_dirs = ['-L' + l for l in target_config.dbus_lib_dirs]
        lib_names = ['-l' + l for l in target_config.dbus_libs]
        libs = ' '.join(lib_dirs + lib_names)

        generate_module_makefile(target_config, verbose, mname,
                includepath=includepath, libs=libs,
                install_path=target_config.pydbus_module_dir,
                src_dir=sp_src_dir)

        subdirs.append(mname)

    # Generate the top-level .pro file.
    inform("Generating the top-level .pro file...")
    out_f = open_for_writing(toplevel_pro)

    out_f.write('''TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = %s

init_py.files = %s
init_py.path = %s/PyQt5
INSTALLS += init_py
''' % (' '.join(subdirs), source_path('__init__.py'), target_config.pyqt_module_dir))

    # Install the uic module and the pyuic5 wrapper.
    out_f.write('''
uic_package.files = %s
uic_package.path = %s/PyQt5
INSTALLS += uic_package
''' % (source_path('pyuic', 'uic'), target_config.pyqt_module_dir))

    if not target_config.no_tools:
        out_f.write('''
pyuic5.files = %s
pyuic5.path = %s
INSTALLS += pyuic5
''' % (pyuic_wrapper, target_config.pyqt_bin_dir))

    # Install the QScintilla .api file.
    if target_config.qsci_api:
        out_f.write('''
qscintilla_api.files = PyQt5.api
qscintilla_api.path = %s/api/python
INSTALLS += qscintilla_api
''' % target_config.qsci_api_dir)

    out_f.close()

    # Make the pyuic5 wrapper executable on platforms that support it.  This
    # means that qmake will try and strip it after installing it resulting in
    # an (ignored) error.  However if we did it after running qmake then (on
    # Linux) the execute bits would be stripped on installation.
    if not target_config.no_tools and target_config.py_platform != 'win32':
        inform("Making the %s wrapper executable..." % pyuic_wrapper)

        sbuf = os.stat(pyuic_wrapper)
        mode = sbuf.st_mode
        mode |= (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
        os.chmod(pyuic_wrapper, mode)

    # Generate the makefiles.
    inform("Generating the Makefiles...")
    run_qmake(target_config, verbose, toplevel_pro, recursive=True)


def generate_plugin_makefile(target_config, verbose, plugin_dir, install_dir, plugin_name):
    """ Generate the makefile for a plugin that embeds the Python interpreter.
    target_config is the target configuration.  verbose is set if the output is
    to be displayed.  plugin_dir is the name of the directory containing the
    plugin implementation.  install_dir is the name of the directory that the
    plugin will be installed in.  plugin_name is a descriptive name of the
    plugin to be used in user messages.  Returns True if the makefile could be
    generated.
    """

    # Check we have a shared interpreter library.
    if target_config.py_pyshlib == '':
        inform("The %s plugin was disabled because a dynamic Python library couldn't be found." % plugin_name)
        return False

    # Create the qmake project file.
    inform("Generating the %s plugin .pro file..." % plugin_name)

    sp_plugin_dir = source_path(plugin_dir)

    fin = open(os.path.join(sp_plugin_dir, '%s.pro-in' % plugin_dir))
    prj = fin.read()
    fin.close()

    prj = prj.replace('@QTCONFIG@',
            'debug' if target_config.debug else 'release')
    prj = prj.replace('@PYINCDIR@', qmake_quote(target_config.py_inc_dir))
    prj = prj.replace('@SIPINCDIR@', qmake_quote(target_config.sip_inc_dir))
    prj = prj.replace('@PYLINK@', target_config.get_pylib_link_arguments())
    prj = prj.replace('@PYSHLIB@', target_config.py_pyshlib)
    prj = prj.replace('@QTPLUGINDIR@', qmake_quote(install_dir))

    pro_name = os.path.join(plugin_dir, '%s.pro' % plugin_dir)

    mk_dir(plugin_dir)
    fout = open_for_writing(pro_name)
    fout.write(prj)

    if sp_plugin_dir != plugin_dir:
        fout.write('''
INCLUDEPATH += %s
VPATH = %s
''' % (qmake_quote(sp_plugin_dir), qmake_quote(sp_plugin_dir)))

    fout.write('\n'.join(target_config.qmake_variables) + '\n')

    fout.close()

    return True


def generate_application_makefile(target_config, verbose, src_dir):
    """ Create the makefile for a QtXml based application.  target_config is
    the target configuration.  verbose is set if the output is to be displayed.
    src_dir is the name of the directory containing the source files.  (It is
    assumed that it is not a path.)
    """

    mk_dir(src_dir)
    sp_src_dir = source_path(src_dir)

    # The standard naming convention.
    app = src_dir + '5'

    inform("Generating the .pro file for %s..." % app)

    pro_lines = ['TARGET = %s' % app]
    pro_lines.extend(['TEMPLATE = app', 'QT -= gui', 'QT += xml'])
    pro_lines.append(
            'CONFIG += warn_on %s' %
                    ('debug' if target_config.debug else 'release'))

    if target_config.py_platform == 'win32':
        pro_lines.append('CONFIG += console')
    elif target_config.py_platform == 'darwin':
        pro_lines.append('CONFIG -= app_bundle')

    # Work around QTBUG-39300.
    pro_lines.append('CONFIG -= android_install')

    pro_lines.append('target.path = %s' % target_config.pyqt_bin_dir)
    pro_lines.append('INSTALLS += target')

    if sp_src_dir != src_dir:
        pro_lines.append('INCLUDEPATH += %s' % qmake_quote(sp_src_dir))
        pro_lines.append('VPATH = %s' % qmake_quote(sp_src_dir))

    pro_lines.extend(pro_sources(sp_src_dir))
    pro_lines.extend(target_config.qmake_variables)

    pro_name = os.path.join(src_dir, src_dir + '.pro')

    pro = open_for_writing(pro_name)
    pro.write('\n'.join(pro_lines))
    pro.close()


def pro_sources(src_dir, other_headers=None, other_sources=None):
    """ Return the HEADERS and SOURCES variables for a .pro file by
    introspecting a directory.  src_dir is the name of the directory.
    other_headers is an optional list of other header files.  other_sources is
    an optional list of other source files.
    """

    if other_headers is None:
        other_headers = []

    if other_sources is None:
        other_sources = []

    pro_lines = []

    headers = [os.path.basename(f) for f in glob.glob('%s/*.h' % src_dir)]
    if len(headers) != 0:
        pro_lines.append('HEADERS = %s' % ' '.join(headers + other_headers))

    c_sources = [os.path.basename(f) for f in glob.glob('%s/*.c' % src_dir)]
    cpp_sources = [os.path.basename(f) for f in glob.glob('%s/*.cpp' % src_dir)]
    pro_lines.append(
            'SOURCES = %s' % ' '.join(c_sources + cpp_sources + other_sources))

    return pro_lines


def generate_pyuic5_wrapper(target_config):
    """ Create a platform dependent executable wrapper for the pyuic.py script.
    target_config is the target configuration.  Returns the platform specific
    name of the wrapper.
    """

    wrapper = 'pyuic5.bat' if target_config.py_platform == 'win32' else 'pyuic5'

    inform("Generating the %s wrapper..." % wrapper)

    exe = quote(target_config.pyuic_interpreter)

    wf = open_for_writing(wrapper)

    if target_config.py_platform == 'win32':
        wf.write('@%s -m PyQt5.uic.pyuic %%1 %%2 %%3 %%4 %%5 %%6 %%7 %%8 %%9\n' % exe)
    else:
        wf.write('#!/bin/sh\n')
        wf.write('exec %s -m PyQt5.uic.pyuic ${1+"$@"}\n' % exe)

    wf.close()

    return wrapper


def rewrite_qmldir(target_config, module, module_dir):
    """ Re-write a qmldir file for a module that used the qmlscene plugin.
    target_config is the target configuration.  module is the name of the QML
    module.  module_dir is the name of the directory containing the QML module.
    """

    qmldir_fn = os.path.join(module_dir, module, 'qmldir')

    inform("Re-writing %s..." % qmldir_fn)

    qmldir = open_for_writing(qmldir_fn)
    qmldir.write('module %s\nplugin pyqt5qmlplugin %s\n' % (module, target_config.qml_plugin_dir))
    qmldir.close()


def quote(path):
    """ Return a path with quotes added if it contains spaces.  path is the
    path.
    """

    if ' ' in path:
        path = '"%s"' % path

    return path


def qmake_quote(path):
    """ Return a path quoted for qmake if it contains spaces.  path is the
    path.
    """

    if ' ' in path:
        path = '$$quote(%s)' % path

    return path


def inform_user(target_config, sip_version):
    """ Tell the user the values that are going to be used.  target_config is
    the target configuration.  sip_version is the SIP version string.
    """

    if target_config.qt_licensee == '':
        detail = ''
    elif target_config.qt_licensee == 'Open Source':
        detail = " (Open Source)"
    else:
        detail = " licensed to %s" % target_config.qt_licensee

    inform("Qt v%s%s is being used." % (
            version_to_string(target_config.qt_version), detail))

    inform("The qmake executable is %s." % target_config.qmake)

    inform(
            "Qt is built as a %s library." % (
                    "shared" if target_config.qt_shared else "static"))

    if target_config.sysroot != '':
        inform("The system root directory is %s." % target_config.sysroot)

    inform("SIP %s is being used." % sip_version)
    inform("The sip executable is %s." % target_config.sip)
    inform("These PyQt5 modules will be built: %s." % ', '.join(target_config.pyqt_modules))
    inform("The PyQt5 Python package will be installed in %s." % target_config.pyqt_module_dir)

    if target_config.no_docstrings:
        inform("PyQt5 is being built without generated docstrings.")
    else:
        inform("PyQt5 is being built with generated docstrings.")

    if target_config.prot_is_public:
        inform("PyQt5 is being built with 'protected' redefined as 'public'.")

    if target_config.no_designer_plugin:
        inform("The Designer plugin will not be built.")
    else:
        inform("The Designer plugin will be installed in %s." %
                target_config.designer_plugin_dir)

    if target_config.no_qml_plugin:
        inform("The qmlscene plugin will not be built.")
    else:
        inform("The qmlscene plugin will be installed in %s." %
                target_config.qml_plugin_dir)

    if target_config.qsci_api:
        inform(
                "The QScintilla API file will be installed in %s." %
                        os.path.join(
                                target_config.qsci_api_dir, 'api', 'python'))

    if target_config.pydbus_module_dir:
        inform(
                "The dbus support module will be installed in %s." %
                        target_config.pydbus_module_dir)

    if target_config.pyqt_sip_dir:
        inform("The PyQt5 .sip files will be installed in %s." %
                target_config.pyqt_sip_dir)

    if target_config.no_tools:
        inform("pyuic5, pyrcc5 and pylupdate5 will not be built.")
    else:
        inform("pyuic5, pyrcc5 and pylupdate5 will be installed in %s." %
                target_config.pyqt_bin_dir)

        inform("The interpreter used by pyuic5 is %s." %
                target_config.pyuic_interpreter)

    if target_config.vend_enabled:
        inform("PyQt5 will only be usable with signed interpreters.")


def run_qmake(target_config, verbose, pro_name, makefile_name='', fatal=True, recursive=False):
    """ Run qmake against a .pro file.  target_config is the target
    configuration.  verbose is set if the output is to be displayed.  pro_name
    is the name of the .pro file.  makefile_name is the name of the makefile
    to generate (and defaults to Makefile).  fatal is set if a qmake failure is
    considered a fatal error, otherwise False is returned if qmake fails.
    recursive is set to use the -recursive flag.
    """

    # qmake doesn't behave consistently if it is not run from the directory
    # containing the .pro file - so make sure it is.
    pro_dir, pro_file = os.path.split(pro_name)
    if pro_dir != '':
        cwd = os.getcwd()
        os.chdir(pro_dir)
    else:
        cwd = None

    mf = makefile_name if makefile_name != '' else 'Makefile'

    remove_file(mf)

    args = [quote(target_config.qmake)]

    if target_config.qmake_spec != target_config.default_qmake_spec:
        args.append('-spec')
        args.append(target_config.qmake_spec)

    if makefile_name != '':
        args.append('-o')
        args.append(makefile_name)

    if recursive:
        args.append('-recursive')

    args.append(pro_file)

    run_command(' '.join(args), verbose)

    if not os.access(mf, os.F_OK):
        if fatal:
            error(
                    "%s failed to create a makefile from %s." %
                            (target_config.qmake, pro_name))

        return False

    # Restore the current directory.
    if cwd is not None:
        os.chdir(cwd)

    return True


def run_make(target_config, verbose, exe, makefile_name):
    """ Run make against a makefile to create an executable.  target_config is
    the target configuration.  verbose is set if the output is to be displayed.
    exe is the platform independent name of the executable that will be
    created.  makefile_name is the name of the makefile.  Returns the platform
    specific name of the executable, or None if an executable wasn't created.
    """

    # Guess the name of make and set the default target and platform specific
    # name of the executable.
    if target_config.py_platform == 'win32':
        if target_config.qmake_spec == 'win32-borland':
            make = 'bmake'
        elif target_config.qmake_spec == 'win32-g++':
            make = 'mingw32-make'
        else:
            make = 'nmake'

        if target_config.debug:
            makefile_target = 'debug'
            platform_exe = os.path.join('debug', exe + '.exe')
        else:
            makefile_target = 'release'
            platform_exe = os.path.join('release', exe + '.exe')
    else:
        make = 'make'
        makefile_target = ''

        if target_config.py_platform == 'darwin':
            platform_exe = os.path.join(exe + '.app', 'Contents', 'MacOS', exe)
        else:
            platform_exe = os.path.join('.', exe)

    remove_file(platform_exe)

    args = [make, '-f', makefile_name]

    if makefile_target != '':
        args.append(makefile_target)

    run_command(' '.join(args), verbose)

    return platform_exe if os.access(platform_exe, os.X_OK) else None


def run_command(cmd, verbose):
    """ Run a command and display the output if requested.  cmd is the command
    to run.  verbose is set if the output is to be displayed.
    """

    if verbose:
        sys.stdout.write(cmd + "\n")

    fout = get_command_output(cmd, and_stderr=True)

    # Read stdout and stderr until there is no more output.
    lout = fout.readline()
    while lout:
        if verbose:
            if sys.hexversion >= 0x03000000:
                sys.stdout.write(str(lout, encoding=sys.stdout.encoding))
            else:
                sys.stdout.write(lout)

        lout = fout.readline()

    fout.close()

    try:
        os.wait()
    except:
        pass


def remove_file(fname):
    """ Remove a file which may or may not exist.  fname is the name of the
    file.
    """

    try:
        os.remove(fname)
    except OSError:
        pass


def check_vendorid(target_config):
    """ See if the VendorID library and include file can be found.
    target_config is the target configuration.
    """

    if target_config.py_version >= 0x030000:
        # VendorID doesn't support Python v3.
        target_config.vend_enabled = False
    elif target_config.vend_enabled:
        if os.access(os.path.join(target_config.vend_inc_dir, 'vendorid.h'), os.F_OK):
            if glob.glob(os.path.join(target_config.vend_lib_dir, '*vendorid*')):
                inform("The VendorID package was found.")
            else:
                target_config.vend_enabled = False
                inform(
                        "The VendorID library could not be found in %s and so "
                        "signed interpreter checking will be disabled. If the "
                        "VendorID package is installed then use the "
                        "--vendorid-libdir argument to explicitly specify the "
                        "correct directory." % target_config.vend_lib_dir)
        else:
            target_config.vend_enabled = False
            inform(
                    "vendorid.h could not be found in %s and so signed "
                    "interpreter checking will be disabled. If the VendorID "
                    "package is installed then use the --vendorid-incdir "
                    "argument to explicitly specify the correct directory." %
                            target_config.vend_inc_dir)


def get_command_output(cmd, and_stderr=False):
    """ Return a pipe from which a command's output can be read.  cmd is the
    command.  and_stderr is set if the output should include stderr as well as
    stdout.
    """

    try:
        import subprocess
    except ImportError:
        if and_stderr:
            _, sout = os.popen4(cmd)
        else:
            _, sout, _ = os.popen3(cmd)

        return sout

    if and_stderr:
        stderr = subprocess.STDOUT
    else:
        stderr = subprocess.PIPE

    p = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=stderr)

    return p.stdout


def source_path(*names):
    """ Return the native path for a list of components rooted at the directory
    containing this script.  names is the sequence of component names.
    """

    path = [os.path.dirname(os.path.abspath(__file__))] + list(names)

    return os.path.join(*path)


def check_dbus(target_config, verbose):
    """ See if the DBus support module should be built and update the target
    configuration accordingly.  target_config is the target configuration.
    verbose is set if the output is to be displayed.
    """

    if not os.path.isdir(source_path('dbus')):
        return

    inform("Checking to see if the dbus support module should be built...")

    cmd = 'pkg-config --cflags-only-I --libs dbus-1'

    if verbose:
        sys.stdout.write(cmd + "\n")

    sout = get_command_output(cmd)
    iflags = sout.read().strip()

    if not iflags:
        inform("DBus v1 does not seem to be installed.")
        return

    if sys.hexversion >= 0x03000000:
        iflags = iflags.decode()

    for f in iflags.split():
        if f.startswith('-I'):
            target_config.dbus_inc_dirs.append(f[2:])
        elif f.startswith('-L'):
            target_config.dbus_lib_dirs.append(f[2:])
        elif f.startswith('-l'):
            target_config.dbus_libs.append(f[2:])

    try:
        import dbus.mainloop
    except:
        inform("The Python dbus module doesn't seem to be installed.")
        return

    target_config.pydbus_module_dir = dbus.mainloop.__path__[0]

    # Try and find dbus-python.h.  We don't use pkg-config because it is broken
    # for dbus-python (at least for versions up to and including v0.81.0).
    # Instead we look where DBus itself is installed - which in most cases will
    # be where dbus-python is also installed.
    if target_config.pydbus_inc_dir != '':
        dlist = [target_config.pydbus_inc_dir]
    else:
        dlist = target_config.dbus_inc_dirs

    for d in dlist:
        if os.access(os.path.join(d, 'dbus', 'dbus-python.h'), os.F_OK):
            if d not in target_config.dbus_inc_dirs:
                target_config.dbus_inc_dirs.append(d)

            break
    else:
        inform(
                "dbus/dbus-python.h could not be found and so the DBus "
                "support module will be disabled. If dbus-python v0.80 or "
                "later is installed then use the --dbus argument to "
                "explicitly specify the directory containing "
                "dbus/dbus-python.h.")
        target_config.pydbus_module_dir = ''


def check_module(target_config, verbose, mname, incfile, test):
    """ See if a module can be built and, if so, add it to the target
    configurations list of modules.  target_config is the target configuration.
    verbose is set if the output is to be displayed.  mname is the name of the
    module.  incfile is the name of the include file needed for the test.  test
    is a C++ statement being used for the test.
    """

    # Check the module's main .sip file exists.
    if not os.access(source_path('sip', mname, mname + 'mod.sip'), os.F_OK):
        return

    inform("Checking to see if the %s module should be built..." % mname)

    source = '''#include <%s>

int main(int, char **)
{
    %s;
}
''' % (incfile, test)

    if compile_qt_program(target_config, verbose, 'cfgtest_' + mname, source, mname) is not None:
        target_config.pyqt_modules.append(mname)


def compile_qt_program(target_config, verbose, name, source, mname, debug=None):
    """ Compile the source of a Qt program and return the name of the
    executable or None if it couldn't be created.  target_config is the target
    configuration.  verbose is set if the output is to be displayed.  name is
    the root name of the program from which all program-specific file names
    will be derived.  source is the C++ source of the program.  mname is the
    name of the Qt module that the program uses.  debug is set if debug, rather
    than release, mode is to be used.  If it is None then the mode is taken
    from the target configuration.
    """

    metadata = MODULE_METADATA[mname]

    # The derived file names.
    name_pro = name + '.pro'
    name_makefile = name + '.mk'
    name_source = name + '.cpp'

    # Create the .pro file.
    pro_lines = []
    pro_add_qt_dependencies(target_config, metadata, pro_lines, debug)
    pro_lines.append('TARGET = %s' % name)
    pro_lines.append('SOURCES = %s' % name_source)

    f = open_for_writing(name_pro)
    f.write('\n'.join(pro_lines))
    f.close()

    # Create the source file.
    f = open_for_writing(name_source)
    f.write(source)
    f.close()

    if not run_qmake(target_config, verbose, name_pro, name_makefile, fatal=False):
        return None

    return run_make(target_config, verbose, name, name_makefile)


def pro_add_qt_dependencies(target_config, metadata, pro_lines, debug=None):
    """ Add the Qt dependencies of a module to a .pro file.  target_config is
    the target configuration.  metadata is the module's meta-data.  pro_lines
    is the list of lines making up the .pro file that is updated.  debug is set
    if debug, rather than release, mode is to be used.  If it is None then the
    mode is taken from the target configuration.
    """

    if debug is None:
        debug = target_config.debug

    add = []
    remove = []
    for qt in metadata.qmake_QT:
        if qt.startswith('-'):
            remove.append(qt[1:])
        else:
            add.append(qt)

    if len(remove) != 0:
        pro_lines.append('QT -= %s' % ' '.join(remove))

    if len(add) != 0:
        pro_lines.append('QT += %s' % ' '.join(add))

    pro_lines.append(
            'CONFIG += %s' % ('debug qml_debug' if debug else 'release'))

    if metadata.cpp11:
        pro_lines.append('CONFIG += c++11')

    pro_lines.extend(target_config.qmake_variables)


def get_sip_flags(target_config):
    """ Return the SIP platform, version and feature flags.  target_config is
    the target configuration.
    """

    sip_flags = []

    # If we don't check for signed interpreters, we exclude the 'VendorID'
    # feature
    if target_config.py_version < 0x030000 and not target_config.vend_enabled:
        sip_flags.append('-x')
        sip_flags.append('VendorID')

    # Handle the platform tag.
    if target_config.py_platform == 'win32':
        plattag = 'WS_WIN'
    elif target_config.py_platform == 'darwin':
        plattag = 'WS_MACX'
    else:
        plattag = 'WS_X11'

    sip_flags.append('-t')
    sip_flags.append(plattag)

    # Handle the Qt version tag.
    sip_flags.append('-t')
    sip_flags.append(version_to_sip_tag(target_config.qt_version))

    # Handle any feature flags.
    for xf in target_config.pyqt_disabled_features:
        sip_flags.append('-x')
        sip_flags.append(xf)

    # Handle the version specific Python features.
    if target_config.py_version < 0x030000:
        sip_flags.append('-x')
        sip_flags.append('Py_v3')

    return ' '.join(sip_flags)


def mk_clean_dir(name):
    """ Create a clean (ie. empty) directory.  name is the name of the
    directory.
    """

    try:
        shutil.rmtree(name)
    except:
        pass

    try:
        os.makedirs(name)
    except:
        error("Unable to create the %s directory." % name)


def mk_dir(name):
    """ Ensure a directory exists, creating it if necessary.  name is the name
    of the directory.
    """

    try:
        os.makedirs(name)
    except:
        pass


def generate_sip_module_code(target_config, verbose, no_timestamp, parts, tracing, mname, sip_flags, qpy_sources=None, qpy_headers=None):
    """ Generate the code for a module.  target_config is the target
    configuration.  verbose is set if the output is to be displayed.
    no_timestamp is set if the .sip files should exclude the timestamp.  parts
    is the number of parts the generated code should be split into.  tracing is
    set if the generated code should include tracing calls.  mname is the name
    of the module to generate the code for.  sip_flags is the string of flags
    to pass to sip.  qpy_sources is the optional list of QPy support code
    source files.  qpy_headers is the optional list of QPy support code header
    files.
    """

    inform("Generating the C++ source for the %s module..." % mname)

    mk_clean_dir(mname)

    # Build the SIP command line.
    argv = [quote(target_config.sip), '-w', sip_flags]

    # Make sure any unknown Qt version gets treated as the latest Qt v5.
    argv.append('-B')
    argv.append('Qt_6_0_0')

    if no_timestamp:
        argv.append('-T')

    if not target_config.no_docstrings:
        argv.append('-o');

    if target_config.prot_is_public:
        argv.append('-P');

    if parts != 0:
        argv.append('-j')
        argv.append(str(parts))

    if tracing:
        argv.append('-r')

    if target_config.qsci_api and mname != 'Qt':
        argv.append('-a')
        argv.append(mname + '.api')

    # Pass the absolute pathname so that #line files are absolute.
    argv.append('-c')
    argv.append(os.path.abspath(mname))

    argv.append('-I')
    argv.append('sip')

    sp_sip_dir = source_path('sip')
    if sp_sip_dir != 'sip':
        # SIP assumes POSIX style separators.
        sp_sip_dir = sp_sip_dir.replace(os.pathsep, '/')
        argv.append('-I')
        argv.append(sp_sip_dir)

    # The .sip files for the Qt modules will be in the out-of-tree directory.
    if mname == 'Qt':
        sip_dir = 'sip'
    else:
        sip_dir = sp_sip_dir

    # Add the name of the .sip file.
    argv.append('%s/%s/%smod.sip' % (sip_dir, mname, mname))

    run_command(' '.join(argv), verbose)

    # Check the result.
    if mname == 'Qt':
        file_check = 'sip%scmodule.c' % mname
    else:
        file_check = 'sipAPI%s.h' % mname

    if not os.access(os.path.join(mname, file_check), os.F_OK):
        error("Unable to create the C++ code.")

    # Embed the sip flags.
    if mname == 'QtCore':
        inform("Embedding sip flags...")

        in_f = open(source_path('qpy', 'QtCore', 'qpycore_post_init.cpp.in'))
        out_f = open_for_writing(
                os.path.join('QtCore', 'qpycore_post_init.cpp'))

        for line in in_f:
            line = line.replace('@@PYQT_SIP_FLAGS@@', sip_flags)
            out_f.write(line)

        in_f.close()
        out_f.close()

    # Generate the makefile.
    includepath = libs = ''
    if target_config.vend_enabled:
        if mname == 'QtCore':
            includepath = target_config.vend_inc_dir
            libs = '-L%s -lvendorid' % target_config.vend_lib_dir

    generate_module_makefile(target_config, verbose, mname,
            includepath=includepath, libs=libs, qpy_sources=qpy_sources,
            qpy_headers=qpy_headers)


def generate_module_makefile(target_config, verbose, mname, includepath='', libs='', install_path='', src_dir='', qpy_sources=None, qpy_headers=None):
    """ Generate the makefile for a module.  target_config is the target
    configuration.  verbose is set if the output is to be displayed.  mname is
    the name of the module.  includepath is an optional additional value of
    INCLUDEPATH.  libs is an optional additional value of LIBS.  install_path
    is the optional name of the directory that the module will be installed in.
    src_dir is the optional source directory (by default the sources are
    assumed to be in the module directory).  qpy_sources is the optional list
    of QPy support code source files.  qpy_headers is the optional list of QPy
    support code header files.
    """

    inform("Generating the .pro file for the %s module..." % mname)

    if src_dir == '':
        src_dir = mname

    target_name = mname

    metadata = MODULE_METADATA[mname]

    if metadata.qmake_TARGET != '':
        target_name = metadata.qmake_TARGET

    pro_lines = ['TEMPLATE = lib']

    pro_lines.append('CONFIG += warn_on %s' % ('staticlib' if target_config.static else 'plugin'))

    pro_add_qt_dependencies(target_config, metadata, pro_lines)

    # Work around QTBUG-39300.
    pro_lines.append('CONFIG -= android_install')

    if not target_config.static:
        debug_suffix = get_win32_debug_suffix(target_config.debug)
        link = target_config.get_pylib_link_arguments()

        shared = '''
win32 {
    PY_MODULE = %s%s.pyd
    target.files = %s%s.pyd
    LIBS += %s
} else {
    PY_MODULE = %s.so
    target.files = %s.so
}
''' % (target_name, debug_suffix, target_name, debug_suffix, link, target_name, target_name)

        pro_lines.extend(shared.split('\n'))

        # Without the 'no_check_exist' magic the target.files must exist when
        # qmake is run otherwise the install and uninstall targets are not
        # generated.
        pro_lines.append('target.CONFIG = no_check_exist')

    if install_path == '':
        install_path = target_config.pyqt_module_dir + '/PyQt5'

    pro_lines.append('target.path = %s' % install_path)
    pro_lines.append('INSTALLS += target')

    if target_config.pyqt_sip_dir:
        sip_files = [os.path.relpath(f, mname)
                for f in glob.glob(source_path('sip', mname, '*.sip'))]
        if len(sip_files) != 0:
            pro_lines.append('sip.path = %s/%s' % (target_config.pyqt_sip_dir, mname))
            pro_lines.append('sip.files = %s' % ' '.join(sip_files))
            pro_lines.append('INSTALLS += sip')

    if 'g++' in target_config.qmake_spec or 'clang' in target_config.qmake_spec:
        pro_lines.append('QMAKE_CXXFLAGS += -fno-exceptions')

    # This optimisation could apply to other platforms.
    if 'linux' in target_config.qmake_spec and not target_config.static:
        if target_config.py_version >= 0x030000:
            entry_point = 'PyInit_%s' % target_name
        else:
            entry_point = 'init%s' % target_name

        exp = open_for_writing(os.path.join(mname, target_name + '.exp'))
        exp.write('{ global: %s; local: *; };' % entry_point)
        exp.close()

        pro_lines.append('QMAKE_LFLAGS += -Wl,--version-script=%s.exp' % target_name)

    if target_config.prot_is_public:
        pro_lines.append('DEFINES += SIP_PROTECTED_IS_PUBLIC protected=public')

    # This is needed for Windows.
    pro_lines.append('INCLUDEPATH += .')

    # Make sure the SIP include directory is searched before the Python include
    # directory if they are different.
    pro_lines.append('INCLUDEPATH += %s' % target_config.sip_inc_dir)
    if target_config.py_inc_dir != target_config.sip_inc_dir:
        pro_lines.append('INCLUDEPATH += %s' % target_config.py_inc_dir)

    pro_add_qpy(mname, metadata, pro_lines)

    if includepath != '':
        pro_lines.append('INCLUDEPATH += %s' % includepath)

    if libs != '':
        pro_lines.append('LIBS += %s' % libs)

    if not target_config.static:
        shared = '''
win32 {
    QMAKE_POST_LINK = $(COPY_FILE) $(DESTDIR_TARGET) $$PY_MODULE
} else {
    QMAKE_POST_LINK = $(COPY_FILE) $(TARGET) $$PY_MODULE
}
macx {
    QMAKE_LFLAGS += "-undefined dynamic_lookup"
    QMAKE_LFLAGS += "-install_name $$absolute_path($$PY_MODULE, $$target.path)"
}
'''

        pro_lines.extend(shared.split('\n'))

    pro_lines.append('TARGET = %s' % target_name)

    if src_dir != mname:
        pro_lines.append('INCLUDEPATH += %s' % qmake_quote(src_dir))
        pro_lines.append('VPATH = %s' % qmake_quote(src_dir))

    pro_lines.extend(pro_sources(src_dir, qpy_headers, qpy_sources))

    pro_name = os.path.join(mname, mname + '.pro')

    pro = open_for_writing(pro_name)
    pro.write('\n'.join(pro_lines))
    pro.write('\n')
    pro.close()


def pro_add_qpy(mname, metadata, pro_lines):
    """ Add the qpy dependencies of a module to a .pro file.  mname is the
    module's name.  metadata is the module's meta-data.  pro_lines is the list
    of lines making up the .pro file that is updated.
    """ 

    if metadata.qpy_lib:
        pro_lines.append('INCLUDEPATH += %s' %
                qmake_quote(os.path.relpath(source_path('qpy', mname), mname)))


def fix_license(src_lfile, dst_lfile):
    """ Fix the license file, if there is one, so that it conforms to the SIP
    v5 syntax.  src_lfile is the name of the license file.  dst_lfile is the
    name of the fixed license file.
    """

    f = open(src_lfile)
    f5 = open_for_writing(dst_lfile)

    for line in f:
        if line.startswith('%License'):
            anno_start = line.find('/')
            anno_end = line.rfind('/')

            if anno_start < 0 or anno_end < 0 or anno_start == anno_end:
                error("%s has missing annotations." % name)

            annos = line[anno_start + 1:anno_end].split(', ')
            annos5 = [anno[0].lower() + anno[1:] for anno in annos]

            f5.write('%License(')
            f5.write(', '.join(annos5))
            f5.write(')\n')
        else:
            f5.write(line)

    f5.close()
    f.close()


def check_license(target_config, license_confirmed, introspecting):
    """ Handle the validation of the PyQt5 license.  target_config is the
    target configuration.  license_confirmed is set if the user has already
    accepted the license.  introspecting is set if the configuration is being
    determined by introspection.
    """

    try:
        import license
        ltype = license.LicenseType
        lname = license.LicenseName

        try:
            lfile = license.LicenseFile
        except AttributeError:
            lfile = None
    except ImportError:
        ltype = None

    if ltype is None:
        ltype = 'GPL'
        lname = "GNU General Public License"
        lfile = 'pyqt-gpl.sip'

    inform(
            "This is the %s version of PyQt %s (licensed under the %s) for "
            "Python %s on %s." %
                    (ltype, PYQT_VERSION_STR, lname, sys.version.split()[0],
                            sys.platform))

    # Common checks.
    if introspecting and target_config.qt_licensee != 'Open Source' and ltype == 'GPL':
        error(
                "This version of PyQt5 and the commercial version of Qt have "
                "incompatible licenses.")

    # Confirm the license if not already done.
    if not license_confirmed:
        loptions = """
Type 'L' to view the license.
"""

        sys.stdout.write(loptions)
        sys.stdout.write("""Type 'yes' to accept the terms of the license.
Type 'no' to decline the terms of the license.

""")

        while 1:
            sys.stdout.write("Do you accept the terms of the license? ")
            sys.stdout.flush()

            try:
                resp = sys.stdin.readline()
            except KeyboardInterrupt:
                raise SystemExit
            except:
                resp = ""

            resp = resp.strip().lower()

            if resp == "yes":
                break

            if resp == "no":
                sys.exit(0)

            if resp == 'l':
                os.system('more LICENSE')

    # Check that the license file exists and fix its syntax.
    sip_dir = 'sip'
    mk_dir(sip_dir)

    sp_sip_dir = source_path(sip_dir)
    src_lfile = os.path.join(sp_sip_dir, lfile)

    if os.access(src_lfile, os.F_OK):
        inform("Found the license file %s." % lfile)
        fix_license(src_lfile, os.path.join(sip_dir, lfile + '5'))
    else:
        error("Please copy the license file %s to %s." % (lfile, sp_sip_dir))


def check_qt(target_config):
    """ Check the Qt installation.  target_config is the target configuration.
    """

    # Starting with v4.7, Qt (when built with MinGW) assumes that stack frames
    # are 16 byte aligned because it uses SSE.  However the Python Windows
    # installers are built with 4 byte aligned stack frames.  We therefore need
    # to tweak the g++ flags to deal with it.
    if target_config.qmake_spec == 'win32-g++':
        target_config.qmake_variables.append('QMAKE_CFLAGS += -mstackrealign')
        target_config.qmake_variables.append('QMAKE_CXXFLAGS += -mstackrealign')


def check_python(target_config):
    """ Check the Python installation.  target_config is the target
    configuration.
    """

    # Check the Python version number.  This allows us to assume relative
    # imports and ElemenTree are available.
    if target_config.py_version < 0x020600:
        error("PyQt5 requires Python v2.6 or later.")


def check_sip(target_config):
    """ Check that the version of sip is good enough and return its version.
    target_config is the target configuration.
    """

    if target_config.sip is None:
        error(
                "Make sure you have a working sip on your PATH or use the "
                "--sip argument to explicitly specify a working sip.")

    pipe = os.popen(' '.join([target_config.sip, '-V']))

    for l in pipe:
        version_str = l.strip()
        break
    else:
        error("'%s -V' did not generate any output." % target_config.sip)

    pipe.close()

    if 'snapshot' not in version_str:
        version = version_from_string(version_str)
        if version is None:
            error(
                    "'%s -V' generated unexpected output: '%s'." % (
                            target_config.sip, version_str))

        min_version = version_from_string(SIP_MIN_VERSION)
        if version < min_version:
            error(
                    "This version of PyQt5 requires sip %s or later." %
                            SIP_MIN_VERSION)

    return version_str


def version_from_string(version_str):
    """ Convert a version string of the form m.n or m.n.o to an encoded version
    number (or None if it was an invalid format).  version_str is the version
    string.
    """

    parts = version_str.split('.')
    if not isinstance(parts, list):
        return None

    if len(parts) == 2:
        parts.append('0')

    if len(parts) != 3:
        return None

    version = 0

    for part in parts:
        try:
            v = int(part)
        except ValueError:
            return None

        version = (version << 8) + v

    return version


def open_for_writing(fname):
    """ Return a file opened for writing while handling the most common problem
    of not having write permission on the directory.  fname is the name of the
    file to open.
    """
    try:
        return open(fname, 'w')
    except IOError:
        error(
                "There was an error creating %s.  Make sure you have write "
                "permission on the parent directory." % fname)


def get_win32_debug_suffix(debug):
    """ Return the debug-dependent suffix appended to the name of Windows
    libraries.  debug is set if the debug version is to be used.
    """

    return '_d' if debug else ''


def main(argv):
    """ Create the configuration module module.  argv is the list of command
    line arguments.
    """

    # Create the default target configuration.
    target_config = TargetConfiguration()

    # Parse the command line.
    parser = create_optparser(target_config)
    opts, target_config.qmake_variables = parser.parse_args()

    target_config.apply_pre_options(opts)

    # Query qmake for the basic configuration information.
    target_config.get_qt_configuration()

    # Update the target configuration.
    if opts.config_file is not None:
        target_config.from_configuration_file(opts.config_file)
    else:
        target_config.from_introspection(opts.verbose, opts.debug)

    target_config.post_configuration()

    target_config.apply_post_options(opts)

    # Check the licenses are compatible.
    check_license(target_config, opts.license_confirmed,
            (opts.config_file is None))

    # Check Python is what we need.
    check_python(target_config)

    # Check SIP is what we need.
    sip_version = check_sip(target_config)

    # Check Qt is what we need.
    check_qt(target_config)

    # Check for the VendorID package.
    check_vendorid(target_config)

    # Check which modules to build if we haven't been told.
    if len(target_config.pyqt_modules) == 0:
        check_modules(target_config, opts.verbose)

    check_dbus(target_config, opts.verbose)

    # Tell the user what's been found.
    inform_user(target_config, sip_version)

    # Generate the makefiles.
    generate_makefiles(target_config, opts.verbose, opts.notimestamp,
            opts.split if opts.concat else 0, opts.tracing)


###############################################################################
# The script starts here.
###############################################################################

if __name__ == '__main__':
    try:
        main(sys.argv)
    except SystemExit:
        raise
    except:
        sys.stderr.write(
"""An internal error occured.  Please report all the output from the program,
including the following traceback, to support@riverbankcomputing.com.
""")
        raise
