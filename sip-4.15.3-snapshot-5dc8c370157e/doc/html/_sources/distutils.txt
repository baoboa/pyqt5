.. _ref-distutils:

Building Your Extension with distutils
======================================

To build the example in :ref:`ref-simple-c++-example` using distutils, it is
sufficient to create a standard ``setup.py``, listing ``word.sip`` among the
files to build, and hook-up SIP into distutils::

    from distutils.core import setup, Extension
    import sipdistutils

    setup(
      name = 'word',
      versione = '1.0',
      ext_modules=[
        Extension("word", ["word.sip", "word.cpp"]),
        ],

      cmdclass = {'build_ext': sipdistutils.build_ext}
    )

As we can see, the above is a normal distutils setup script, with just a
special line which is needed so that SIP can see and process ``word.sip``.
Then, running ``setup.py build`` will build our extension module.

If you want to use any of sip's command-line options described in
:ref:`ref-command-line`, there is a new option available for the
``build_ext`` command in distutils: ``--sip-opts``. So you can either invoke
distutils as follows::

     $ python setup.py build_ext --sip-opts="-e -g" build

or you can leverage distutils' config file support by creating a ``setup.cfg``
file in the supported system or local paths (eg: in the same directory of
``setup.py``) with these contents::

     [build_ext]
     sip-opts = -e -g

and then run ``setup.py build`` as usual.

If ``sip-opts`` has not been specified then any ``swig_opts`` defined when
creating the ``Extension`` will be used.
