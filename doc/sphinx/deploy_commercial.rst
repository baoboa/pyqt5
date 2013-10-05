.. _ref-deploy-commercial:

Deploying Commercial PyQt5 Applications
=======================================

When deploying commercial PyQt5 applications it is necessary to discourage
users from accessing the underlying PyQt5 modules for themselves.  A user that
used the modules shipped with your application to develop new applications
would themselves be considered a developer and would need their own commercial
PyQt5 license.

One solution to this problem is the `VendorID
<http://www.riverbankcomputing.com/software/vendorid/>`__ package.  This allows
you to build Python extension modules that can only be imported by a digitally
signed custom interpreter.  The package enables you to create such an
interpreter with your application embedded within it.  The result is an
interpreter that can only run your application, and PyQt5 modules that can only
be imported by that interpreter.  You can use the package to similarly restrict
access to any extension module.

In order to build PyQt5 with support for the VendorID package, pass the
:option:`--vendorid <configure.py --vendorid>` command line option to
:program:`configure.py`.
