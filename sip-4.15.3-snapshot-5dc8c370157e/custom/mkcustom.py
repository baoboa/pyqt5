"""This Python script uses the SIP build system to create a Makefile for
building a custom Python interpreter.  The script doesn't take any command line
flags - just edit it to suit your needs.  You will also need to edit custom.c
or customw.c.
"""


import sys
import sipconfig


# Get the SIP configuration.
cfg = sipconfig.Configuration()


# This is the name of the interpreter executable (excluding any platform
# specific extension.
InterpreterName = "custom"

# Set this to True to create a non-console interpreter on Windows (ie. a custom
# version of pythonw).  Make sure you make changes to customw.c rather than
# custom.c.  It is ignored on other platforms.
WindowsInterpreter = False

# Set this to the list of the name of modules to be builtin to the custom
# interpreter.  The modules must also be added to custom.c and/or customw.c.
Modules = ["sip"]
#Modules = ["sip", "qt", "qtaxcontainer", "qtcanvas", "qtext", "qtgl",
#           "qtnetwork", "qtsql", "qttable", "qtui", "qtxml"]

# Set this to the name of the directory containing the static modules.
ModuleDirectory = cfg.default_mod_dir

# Set this to the list of additional libraries to link with the custom
# interpreter.
ExtraLibraries = []
#ExtraLibraries = ["qassistantclient"]

# Set this to the list of additional directory names to search for any
# additional libraries.
ExtraLibraryDirectories = []

# Set this to the name of the directory containing the Python library.
PythonLibraryDirectory = cfg.py_lib_dir


# Make platform specific modifications.
if sys.platform.startswith("linux"):
    ExtraLibraries.append("util")


# Create a dictionary describing the target and source files to be passed to
# the SIP build system.
build = {}

if sys.platform == "win32" and WindowsInterpreter:
    build["target"] = InterpreterName + "w"
    build["sources"] = "customw.c"
    console = False
else:
    build["target"] = InterpreterName
    build["sources"] = "custom.c"
    console = True

# Assume Qt support is required if Qt support was enabled in the sip module.
qt = (cfg.qt_version > 0)

# Create the Makefile instance.
mf = sipconfig.ProgramMakefile(cfg, build, python=True, console=console, qt=qt)

# Modify the Makefile according to the values set above.
mf.extra_lib_dirs.extend(ExtraLibraryDirectories)
mf.extra_lib_dirs.append(ModuleDirectory)
mf.extra_lib_dirs.append(PythonLibraryDirectory)

mf.extra_libs.extend(Modules)

if sys.platform == "win32":
    pylib = "python%u%u"
else:
    pylib = "python%u.%u"

mf.extra_libs.append(pylib % ((cfg.py_version >> 16), ((cfg.py_version >> 8) & 0xff)))
mf.extra_libs.extend(ExtraLibraries)

# Generate the Makefile itself.
mf.generate()
