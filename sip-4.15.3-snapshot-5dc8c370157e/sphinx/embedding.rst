Using the C API when Embedding
==============================

The :ref:`C API <ref-c-api>` is intended to be called from handwritten code in
SIP generated modules.  However it is also often necessary to call it from C or
C++ applications that embed the Python interpreter and need to pass C or C++
instances between the application and the interpreter.

The API is exported by the SIP module as a ``sipAPIDef`` data structure
containing a set of function pointers.  The data structure is defined in the
SIP header file ``sip.h``.  When using Python v2.7, or Python v3.1 or later the
data structure is wrapped as a Python ``PyCapsule`` object.  When using other
versions of Python the data structure is wrapped as a Python ``PyCObject``
object.  It is referenced by the name ``_C_API`` in the SIP module dictionary.

Each member of the data structure is a pointer to one of the functions of the
SIP API.  The name of the member can be derived from the function name by
replacing the ``sip`` prefix with ``api`` and converting each word in the
name to lower case and preceding it with an underscore.  For example:

    ``sipExportSymbol`` becomes ``api_export_symbol``

    ``sipWrapperCheck`` becomes ``api_wrapper_check``

Note that the type objects that SIP generates for a wrapped module (see
:ref:`ref-type-structures`, :ref:`ref-enum-type-objects` and
:ref:`ref-exception-objects`) cannot be refered to directly and must be
obtained using the :c:func:`sipFindType()` function.  Of course, the
corresponding modules must already have been imported into the interpreter.

The following code fragment shows how to get a pointer to the ``sipAPIDef``
data structure::

    #include <sip.h>

    const sipAPIDef *get_sip_api()
    {
    #if defined(SIP_USE_PYCAPSULE)
        return (const sipAPIDef *)PyCapsule_Import("sip._C_API", 0);
    #else
        PyObject *sip_module;
        PyObject *sip_module_dict;
        PyObject *c_api;

        /* Import the SIP module. */
        sip_module = PyImport_ImportModule("sip");

        if (sip_module == NULL)
            return NULL;

        /* Get the module's dictionary. */
        sip_module_dict = PyModule_GetDict(sip_module);

        /* Get the "_C_API" attribute. */
        c_api = PyDict_GetItemString(sip_module_dict, "_C_API");

        if (c_api == NULL)
            return NULL;

        /* Sanity check that it is the right type. */
        if (!PyCObject_Check(c_api))
            return NULL;

        /* Get the actual pointer from the object. */
        return (const sipAPIDef *)PyCObject_AsVoidPtr(c_api);
    #endif
    }

The use of :c:macro:`SIP_USE_PYCAPSULE` means that code will run under all
versions of Python.
