/*
 * This file is the basis of a custom Python interpreter.  Use it for Windows
 * (non-console).  You will also need to edit mkcustom.py.
 */


#define	WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <Python.h>


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	/*
	 * Declare the module initialisation function for each module you want
	 * to be a builtin in the custom interpreter.  The name of the function
	 * will be the name of the module with "init" prepended.  The modules
	 * must be built as static libraries (using the -k flag to configure.py
	 * for SIP and PyQt).
	 */

	/* The sip module will be builtin. */
	extern void initsip(void);

	/*
	 * Uncomment these (and in the structure below) to include the PyQt
	 * modules as builtins.
	 */
/*	extern void initqt(void);*/
/*	extern void initqtaxcontainer(void);*/
/*	extern void initqtcanvas(void);*/
/*	extern void initqtext(void);*/
/*	extern void initqtgl(void);*/
/*	extern void initqtnetwork(void);*/
/*	extern void initqtsql(void);*/
/*	extern void initqttable(void);*/
/*	extern void initqtui(void);*/
/*	extern void initqtxml(void);*/

	/*
	 * This structure specifies the names and initialisation functions of
	 * the builtin modules.
	 */
	struct _inittab builtin_modules[] = {
		{"sip", initsip},
/*		{"qt", initqt},*/
/*		{"qtaxcontainer", initqtaxcontainer},*/
/*		{"qtcanvas", initqtcanvas},*/
/*		{"qtext", initqtext},*/
/*		{"qtgl", initqtgl},*/
/*		{"qtnetwork", initqtnetwork},*/
/*		{"qtsql", initqtsql},*/
/*		{"qttable", initqttable},*/
/*		{"qtui", initqtui},*/
/*		{"qtxml", initqtxml},*/
		{NULL, NULL}
	};

	PyImport_ExtendInittab(builtin_modules);

	return Py_Main(__argc, __argv);
}
