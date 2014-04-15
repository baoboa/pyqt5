# This script handles the SIP configuration and generates the Makefiles.
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
import glob
import optparse
from distutils import sysconfig

try:
    from importlib import invalidate_caches
except ImportError:
    invalidate_caches = lambda: None

import siputils


# Initialise the globals.
sip_version = 0x040f03
sip_version_str = "4.15.3-snapshot-5dc8c370157e"
py_version = sys.hexversion >> 8
plat_py_site_dir = None
plat_py_inc_dir = None
plat_py_conf_inc_dir = None
plat_py_lib_dir = None
plat_sip_dir = None
plat_bin_dir = None
platform_specs = []
src_dir = os.path.dirname(os.path.abspath(__file__))
sip_module_base = None

# Constants.
DEFAULT_MACOSX_ARCH = 'i386 ppc'
MACOSX_SDK_DIRS = ('/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs', '/Developer/SDKs')

# Command line options.
default_platform = None
default_sipbindir = None
default_sipmoddir = None
default_sipincdir = None
default_sipsipdir = None

# The names of build macros extracted from the platform specific configuration
# files.
build_macro_names = [
    "DEFINES", "CONFIG",
    "CC",
    "CFLAGS",
    "CFLAGS_RELEASE", "CFLAGS_DEBUG",
    "CFLAGS_CONSOLE", "CFLAGS_SHLIB", "CFLAGS_APP", "CFLAGS_THREAD",
    "CFLAGS_MT", "CFLAGS_MT_DBG", "CFLAGS_MT_DLL", "CFLAGS_MT_DLLDBG",
    "CFLAGS_EXCEPTIONS_ON", "CFLAGS_EXCEPTIONS_OFF",
    "CFLAGS_RTTI_ON", "CFLAGS_RTTI_OFF",
    "CFLAGS_STL_ON", "CFLAGS_STL_OFF",
    "CFLAGS_WARN_ON", "CFLAGS_WARN_OFF",
    "CHK_DIR_EXISTS", "COPY",
    "CXX",
    "CXXFLAGS",
    "CXXFLAGS_RELEASE", "CXXFLAGS_DEBUG",
    "CXXFLAGS_CONSOLE", "CXXFLAGS_SHLIB", "CXXFLAGS_APP", "CXXFLAGS_THREAD",
    "CXXFLAGS_MT", "CXXFLAGS_MT_DBG", "CXXFLAGS_MT_DLL", "CXXFLAGS_MT_DLLDBG",
    "CXXFLAGS_EXCEPTIONS_ON", "CXXFLAGS_EXCEPTIONS_OFF",
    "CXXFLAGS_RTTI_ON", "CXXFLAGS_RTTI_OFF",
    "CXXFLAGS_STL_ON", "CXXFLAGS_STL_OFF",
    "CXXFLAGS_WARN_ON", "CXXFLAGS_WARN_OFF",
    "DEL_FILE",
    "EXTENSION_SHLIB", "EXTENSION_PLUGIN",
    "INCDIR", "INCDIR_X11", "INCDIR_OPENGL",
    "LIBS_CORE", "LIBS_GUI", "LIBS_NETWORK", "LIBS_OPENGL", "LIBS_WEBKIT",
    "LINK", "LINK_SHLIB", "AIX_SHLIB", "LINK_SHLIB_CMD",
    "LFLAGS", "LFLAGS_CONSOLE", "LFLAGS_CONSOLE_DLL", "LFLAGS_DEBUG",
    "LFLAGS_DLL",
    "LFLAGS_PLUGIN", "LFLAGS_RELEASE", "LFLAGS_SHLIB", "LFLAGS_SONAME",
    "LFLAGS_THREAD", "LFLAGS_WINDOWS", "LFLAGS_WINDOWS_DLL", "LFLAGS_OPENGL",
    "LIBDIR", "LIBDIR_X11", "LIBDIR_OPENGL",
    "LIBS", "LIBS_CONSOLE", "LIBS_RT",
    "LIBS_RTMT", "LIBS_THREAD", "LIBS_WINDOWS", "LIBS_X11",
    "MAKEFILE_GENERATOR",
    "MKDIR",
    "RPATH", "LFLAGS_RPATH",
    "AR", "RANLIB", "LIB", "STRIP"
]


def show_platforms():
    """Display the different platform/compilers.
    """
    sys.stdout.write("""
The following platform/compiler configurations are supported:

""")

    platform_specs.sort()
    sys.stdout.write(siputils.format(", ".join(platform_specs), leftmargin=2))
    sys.stdout.write("\n\n")


def show_macros():
    """Display the different build macros.
    """
    sys.stdout.write("""
The following options may be used to adjust the compiler configuration:

""")

    build_macro_names.sort()
    sys.stdout.write(siputils.format(", ".join(build_macro_names), leftmargin=2))
    sys.stdout.write("\n\n")


def set_defaults():
    """Set up the defaults for values that can be set on the command line.
    """
    global default_platform, default_sipbindir, default_sipmoddir
    global default_sipincdir, default_sipsipdir

    # Set the platform specific default specification.
    platdefaults = {
        "aix":          "aix-xlc",
        "bsd":          "bsdi-g++",
        "cygwin":       "cygwin-g++",
        "darwin":       "macx-g++",
        "dgux":         "dgux-g++",
        "freebsd":      "freebsd-g++",
        "gnu":          "hurd-g++",
        "hp-ux":        "hpux-acc",
        "irix":         "irix-cc",
        "linux":        "linux-g++",
        "lynxos":       "lynxos-g++",
        "netbsd":       "netbsd-g++",
        "openbsd":      "openbsd-g++",
        "openunix":     "unixware-cc",
        "osf1":         "tru64-cxx",
        "qnx":          "qnx-g++",
        "reliantunix":  "reliant-cds",
        "sco_sv":       "sco-cc",
        "sinix":        "reliant-cds",
        "sunos5":       "solaris-cc",
        "ultrix":       "ultrix-g++",
        "unix_sv":      "unixware-g++",
        "unixware":     "unixware-cc"
    }

    default_platform = "none"

    if sys.platform == "win32":
        if py_version >= 0x030300:
            default_platform = "win32-msvc2010"
        elif py_version >= 0x020600:
            default_platform = "win32-msvc2008"
        elif py_version >= 0x020400:
            default_platform = "win32-msvc.net"
        else:
            default_platform = "win32-msvc"
    else:
        for pd in list(platdefaults.keys()):
            if sys.platform[:len(pd)] == pd:
                default_platform = platdefaults[pd]
                break

    default_sipbindir = plat_bin_dir
    default_sipmoddir = plat_py_site_dir
    default_sipincdir = plat_py_inc_dir
    default_sipsipdir = plat_sip_dir


def inform_user():
    """Tell the user the option values that are going to be used.
    """
    siputils.inform("The SIP code generator will be installed in %s." % opts.sipbindir)
    siputils.inform("The %s module will be installed in %s." % (sip_module_base, opts.sipmoddir))
    siputils.inform("The sip.h header file will be installed in %s." % opts.sipincdir)
    siputils.inform("The default directory to install .sip files in is %s." % opts.sipsipdir)
    siputils.inform("The platform/compiler configuration is %s." % opts.platform)

    if opts.arch:
        siputils.inform("MacOS/X binaries will be created for %s." % (", ".join(opts.arch.split())))

    if opts.universal:
        siputils.inform("MacOS/X universal binaries will be created using %s." % opts.universal)

    if opts.deployment_target:
        siputils.inform("MacOS/X deployment target is %s." % opts.deployment_target)


def set_platform_directories():
    """Initialise the global variables relating to platform specific
    directories.
    """
    global plat_py_site_dir, plat_py_inc_dir, plat_py_conf_inc_dir
    global plat_bin_dir, plat_py_lib_dir, plat_sip_dir

    # We trust distutils for some stuff.
    plat_py_site_dir = sysconfig.get_python_lib(plat_specific=1)
    plat_py_inc_dir = sysconfig.get_python_inc()
    plat_py_conf_inc_dir = os.path.dirname(sysconfig.get_config_h_filename())

    if sys.platform == "win32":
        plat_py_lib_dir = sys.prefix + "\\libs"
        plat_bin_dir = sys.exec_prefix
        plat_sip_dir = sys.prefix + "\\sip"
    else:
        lib_dir = sysconfig.get_python_lib(plat_specific=1, standard_lib=1)

        plat_py_lib_dir = lib_dir + "/config"
        plat_bin_dir = sys.exec_prefix + "/bin"
        plat_sip_dir = sys.prefix + "/share/sip"


def patch_files():
    """Patch any files that need it."""

    patched = (
        ("siplib", "sip.h"),
        ("siplib", "siplib.c"),
        ("siplib", "siplib.sbf")
    )

    # The siplib directory may not exist if we are building away from the
    # source directory.
    try:
        os.mkdir("siplib")
    except OSError:
        pass

    for f in patched:
        dst_fn = os.path.join(*f)
        src_fn = os.path.join(src_dir, dst_fn + ".in")

        siputils.inform("Creating %s..." % dst_fn)

        dst = open(dst_fn, "w")
        src = open(src_fn)

        for line in src:
            line = line.replace("@CFG_MODULE_NAME@", opts.sip_module)
            line = line.replace("@CFG_MODULE_BASENAME@", sip_module_base)

            dst.write(line)

        dst.close()
        src.close()


def create_config(module, template, macros):
    """Create the SIP configuration module so that it can be imported by build
    scripts.

    module is the module file name.
    template is the template file name.
    macros is the dictionary of build macros.
    """
    siputils.inform("Creating %s..." % module)

    content = {
        "sip_config_args":  sys.argv[1:],
        "sip_version":      sip_version,
        "sip_version_str":  sip_version_str,
        "platform":         opts.platform,
        "sip_bin":          os.path.join(opts.sipbindir, "sip"),
        "sip_inc_dir":      opts.sipincdir,
        "sip_mod_dir":      opts.sipmoddir,
        "default_bin_dir":  plat_bin_dir,
        "default_mod_dir":  plat_py_site_dir,
        "default_sip_dir":  opts.sipsipdir,
        "py_version":       py_version,
        "py_inc_dir":       plat_py_inc_dir,
        "py_conf_inc_dir":  plat_py_conf_inc_dir,
        "py_lib_dir":       plat_py_lib_dir,
        "universal":        opts.universal,
        "arch":             opts.arch,
        "deployment_target":    opts.deployment_target
    }

    siputils.create_config_module(module, template, content, macros)


def create_makefiles(macros):
    """Create the Makefiles.

    macros is the dictionary of platform specific build macros.
    """
    # Bootstrap.  Make sure we get the right one.
    sys.path.insert(0, os.path.curdir)
    invalidate_caches()
    import sipconfig

    cfg = sipconfig.Configuration()

    cfg.set_build_macros(macros)

    sipconfig.inform("Creating top level Makefile...")

    sipconfig.ParentMakefile(
        configuration=cfg,
        subdirs=["sipgen", "siplib"],
        installs=(["sipconfig.py", os.path.join(src_dir, "sipdistutils.py")],
                cfg.sip_mod_dir)
    ).generate()

    sipconfig.inform("Creating sip code generator Makefile...")

    sipconfig.ProgramMakefile(
        configuration=cfg,
        build_file=os.path.join(src_dir, "sipgen", "sipgen.sbf"),
        dir="sipgen",
        install_dir=os.path.dirname(cfg.sip_bin),
        console=1,
        warnings=0,
        universal=opts.universal,
        arch=opts.arch,
        deployment_target=opts.deployment_target
    ).generate()

    sipconfig.inform("Creating sip module Makefile...")

    makefile = sipconfig.ModuleMakefile(
        configuration=cfg,
        build_file=os.path.join(src_dir, "siplib", "siplib.sbf"),
        dir="siplib",
        install_dir=cfg.sip_mod_dir,
        installs=([os.path.join(src_dir, "siplib", "sip.h")], cfg.sip_inc_dir),
        console=1,
        warnings=0,
        static=opts.static,
        debug=opts.debug,
        universal=opts.universal,
        arch=opts.arch,
        deployment_target=opts.deployment_target
    )

    makefile.generate()


def create_optparser(sdk_dir):
    """Create the parser for the command line.
    """
    def store_abspath(option, opt_str, value, parser):
        setattr(parser.values, option.dest, os.path.abspath(value))

    p = optparse.OptionParser(usage="python %prog [opts] [macro=value] "
            "[macro+=value]", version=sip_version_str)

    # Note: we don't use %default to be compatible with Python 2.3.
    p.add_option("-k", "--static", action="store_true", default=False,
            dest="static", help="build the SIP module as a static library")
    p.add_option("-p", "--platform", action="store",
            default=default_platform, type="string", metavar="PLATFORM",
            dest="platform", help="the platform/compiler configuration "
            "[default: %s]" % default_platform)
    p.add_option("-u", "--debug", action="store_true", default=False,
            help="build with debugging symbols")
    p.add_option("--sip-module", action="store", default="sip", type="string",
            metavar="NAME", dest="sip_module", help="the package.module name "
            "of the sip module [default: sip]")

    if sys.platform == 'darwin':
        # Get the latest SDK to use as the default.
        sdks = glob.glob(sdk_dir + '/MacOSX*.sdk')
        if len(sdks) > 0:
            sdks.sort()
            _, default_sdk = os.path.split(sdks[-1])
        else:
            default_sdk = 'MacOSX10.4u.sdk'

        g = optparse.OptionGroup(p, title="MacOS X Configuration")
        g.add_option("--arch", action="append", default=[], dest="arch",
                choices=["i386", "x86_64", "ppc"],
                help="build for architecture ARCH")
        g.add_option("--deployment-target", action="store", default='',
                metavar="VERSION", dest="deployment_target",
                help="set the value of the MACOSX_DEPLOYMENT_TARGET "
                        "environment variable in generated Makefiles")
        g.add_option("-n", "--universal", action="store_true", default=False,
                dest="universal",
                help="build the SIP code generator and module as universal "
                        "binaries")
        g.add_option("-s", "--sdk", action="store", default=default_sdk,
                type="string", metavar="SDK", dest="sdk",
                help="the name of the SDK used when building universal "
                        "binaries [default: %s]" % default_sdk)
        p.add_option_group(g)

    # Querying.
    g = optparse.OptionGroup(p, title="Query")
    g.add_option("--show-platforms", action="store_true", default=False,
            dest="show_platforms", help="show the list of supported "
            "platform/compiler configurations")
    g.add_option("--show-build-macros", action="store_true", default=False,
            dest="show_build_macros", help="show the list of supported build "
            "macros")
    p.add_option_group(g)

    # Installation.
    g = optparse.OptionGroup(p, title="Installation")
    g.add_option("-b", "--bindir", action="callback",
            default=default_sipbindir, type="string", metavar="DIR",
            dest="sipbindir", callback=store_abspath, help="where the SIP "
            "code generator will be installed [default: %s]" %
            default_sipbindir)
    g.add_option("-d", "--destdir", action="callback",
            default=default_sipmoddir, type="string", metavar="DIR",
            dest="sipmoddir", callback=store_abspath, help="where the SIP "
            "module will be installed [default: %s]" % default_sipmoddir)
    g.add_option("-e", "--incdir", action="callback",
            default=default_sipincdir, type="string", metavar="DIR",
            dest="sipincdir", callback=store_abspath, help="where the SIP "
            "header file will be installed [default: %s]" % default_sipincdir)
    g.add_option("-v", "--sipdir", action="callback",
            default=default_sipsipdir, type="string", metavar="DIR",
            dest="sipsipdir", callback=store_abspath, help="where .sip files "
            "are normally installed [default: %s]" % default_sipsipdir)
    p.add_option_group(g)

    return p


def main(argv):
    """Create the configuration module module.

    argv is the list of command line arguments.
    """
    siputils.inform("This is SIP %s for Python %s on %s." % (sip_version_str, sys.version.split()[0], sys.platform))

    if py_version < 0x020300:
        siputils.error("This version of SIP requires Python v2.3 or later.")

    # Basic initialisation.
    set_platform_directories()

    # Build up the list of valid specs.
    for s in os.listdir(os.path.join(src_dir, "specs")):
        platform_specs.append(s)

    # Determine the directory containing the default OS/X SDK.
    if sys.platform == 'darwin':
        for sdk_dir in MACOSX_SDK_DIRS:
            if os.path.isdir(sdk_dir):
                break
        else:
            sdk_dir = MACOSX_SDK_DIRS[0]
    else:
        sdk_dir = ''

    # Parse the command line.
    global opts

    set_defaults()
    p = create_optparser(sdk_dir)
    opts, args = p.parse_args()

    # Make sure MacOS specific options get initialised.
    if sys.platform != 'darwin':
        opts.universal = ''
        opts.arch = []
        opts.sdk = ''
        opts.deployment_target = ''

    # Handle the query options.
    if opts.show_platforms or opts.show_build_macros:
        if opts.show_platforms:
            show_platforms()

        if opts.show_build_macros:
            show_macros()

        sys.exit()

    # Convert the list 'arch' option to a string.  Multiple architectures
    # imply a universal binary.
    if len(opts.arch) > 1:
        opts.universal = True

    opts.arch = ' '.join(opts.arch)

    # Convert the boolean 'universal' option to a string.
    if opts.universal:
        if '/' in opts.sdk:
            opts.universal = os.path.abspath(opts.sdk)
        else:
            opts.universal = sdk_dir + '/' + opts.sdk

        if not os.path.isdir(opts.universal):
            siputils.error("Unable to find the SDK directory %s. Use the --sdk flag to specify the name of the SDK or its full path." % opts.universal)

        if opts.arch == '':
            opts.arch = DEFAULT_MACOSX_ARCH
    else:
        opts.universal = ''

    # Get the platform specific macros for building.
    macros = siputils.parse_build_macros(
            os.path.join(src_dir, "specs", opts.platform), build_macro_names,
            args)

    if macros is None:
        siputils.error("Unsupported macro name specified. Use the --show-build-macros flag to see a list of supported macros.")
        sys.exit(2)

    # Fix the name of the sip module.
    global sip_module_base

    module_path = opts.sip_module.split(".")
    sip_module_base = module_path[-1]

    if len(module_path) > 1:
        del module_path[-1]
        module_path.insert(0, opts.sipmoddir)
        opts.sipmoddir = os.path.join(*module_path)

    # Tell the user what's been found.
    inform_user()

    # Patch any files that need it.
    patch_files()

    # Install the configuration module.
    create_config("sipconfig.py", os.path.join(src_dir, "siputils.py"),
            macros)

    # Create the Makefiles.
    create_makefiles(macros)


###############################################################################
# The script starts here.
###############################################################################

if __name__ == "__main__":
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
