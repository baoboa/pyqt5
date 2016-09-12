// This is the SIP interface definition for the QList based mapped types
// specific to the QtMultimedia module.
//
// Copyright (c) 2016 Riverbank Computing Limited <info@riverbankcomputing.com>
// 
// This file is part of PyQt5.
// 
// This file may be used under the terms of the GNU General Public License
// version 3.0 as published by the Free Software Foundation and appearing in
// the file LICENSE included in the packaging of this file.  Please review the
// following information to ensure the GNU General Public License version 3.0
// requirements will be met: http://www.gnu.org/copyleft/gpl.html.
// 
// If you do not wish to use this file under the terms of the GPL version 3.0
// then you may purchase a commercial license.  For more information contact
// info@riverbankcomputing.com.
// 
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


%MappedType QList<QVideoFrame::PixelFormat>
        /TypeHintIn="Iterable[QVideoFrame.PixelFormat]",
        TypeHintOut="List[QVideoFrame.PixelFormat]", TypeHintValue="[]"/
{
%TypeHeaderCode
#include <qvideoframe.h>
%End

%ConvertFromTypeCode
    PyObject *l = PyList_New(sipCpp->size());

    if (!l)
        return 0;

    for (int i = 0; i < sipCpp->size(); ++i)
    {
        PyObject *eobj = sipConvertFromEnum(sipCpp->at(i),
                sipType_QVideoFrame_PixelFormat);

        if (!eobj)
        {
            Py_DECREF(l);

            return 0;
        }

        PyList_SET_ITEM(l, i, eobj);
    }

    return l;
%End

%ConvertToTypeCode
    PyObject *iter = PyObject_GetIter(sipPy);

    if (!sipIsErr)
    {
        Py_XDECREF(iter);

        return (iter
#if PY_MAJOR_VERSION < 3
                && !PyString_Check(sipPy)
#endif
                && !PyUnicode_Check(sipPy));
    }

    if (!iter)
    {
        *sipIsErr = 1;

        return 0;
    }

    QList<QVideoFrame::PixelFormat> *ql = new QList<QVideoFrame::PixelFormat>;

    for (Py_ssize_t i = 0; ; ++i)
    {
        PyErr_Clear();
        PyObject *itm = PyIter_Next(iter);

        if (!itm)
        {
            if (PyErr_Occurred())
            {
                delete ql;
                Py_DECREF(iter);
                *sipIsErr = 1;

                return 0;
            }

            break;
        }

        if (!sipCanConvertToEnum(itm, sipType_QVideoFrame_PixelFormat))
        {
            PyErr_Format(PyExc_TypeError,
                    "index %zd has type '%s' but 'QVideoFrame.PixelFormat' is expected",
                    i, Py_TYPE(itm)->tp_name);

            Py_DECREF(itm);
            delete ql;
            Py_DECREF(iter);
            *sipIsErr = 1;

            return 0;
        }

        ql->append(static_cast<QVideoFrame::PixelFormat>(SIPLong_AsLong(itm)));

        Py_DECREF(itm);
    }

    Py_DECREF(iter);

    *sipCppPtr = ql;

    return sipGetState(sipTransferObj);
%End
};


%MappedType QList<QAudioFormat::Endian>
        /TypeHintIn="Iterable[QAudioFormat.Endian]",
        TypeHintOut="List[QAudioFormat.Endian]", TypeHintValue="[]"/
{
%TypeHeaderCode
#include <qaudioformat.h>
%End

%ConvertFromTypeCode
    PyObject *l = PyList_New(sipCpp->size());

    if (!l)
        return 0;

    for (int i = 0; i < sipCpp->size(); ++i)
    {
        PyObject *eobj = sipConvertFromEnum(sipCpp->at(i),
                sipType_QAudioFormat_Endian);

        if (!eobj)
        {
            Py_DECREF(l);

            return 0;
        }

        PyList_SET_ITEM(l, i, eobj);
    }

    return l;
%End

%ConvertToTypeCode
    PyObject *iter = PyObject_GetIter(sipPy);

    if (!sipIsErr)
    {
        Py_XDECREF(iter);

        return (iter
#if PY_MAJOR_VERSION < 3
                && !PyString_Check(sipPy)
#endif
                && !PyUnicode_Check(sipPy));
    }

    if (!iter)
    {
        *sipIsErr = 1;

        return 0;
    }

    QList<QAudioFormat::Endian> *ql = new QList<QAudioFormat::Endian>;

    for (Py_ssize_t i = 0; ; ++i)
    {
        PyErr_Clear();
        PyObject *itm = PyIter_Next(iter);

        if (!itm)
        {
            if (PyErr_Occurred())
            {
                delete ql;
                Py_DECREF(iter);
                *sipIsErr = 1;

                return 0;
            }

            break;
        }

        if (!sipCanConvertToEnum(itm, sipType_QAudioFormat_Endian))
        {
            PyErr_Format(PyExc_TypeError,
                    "index %zd has type '%s' but 'QAudioFormat.Endian' is expected",
                    i, Py_TYPE(itm)->tp_name);

            Py_DECREF(itm);
            delete ql;
            Py_DECREF(iter);
            *sipIsErr = 1;

            return 0;
        }

        ql->append(static_cast<QAudioFormat::Endian>(SIPLong_AsLong(itm)));

        Py_DECREF(itm);
    }

    Py_DECREF(iter);

    *sipCppPtr = ql;

    return sipGetState(sipTransferObj);
%End
};


%MappedType QList<QAudioFormat::SampleType>
        /TypeHintIn="Iterable[QAudioFormat.SampleType]",
        TypeHintOut="List[QAudioFormat.SampleType]", TypeHintValue="[]"/
{
%TypeHeaderCode
#include <qaudioformat.h>
%End

%ConvertFromTypeCode
    PyObject *l = PyList_New(sipCpp->size());

    if (!l)
        return 0;

    for (int i = 0; i < sipCpp->size(); ++i)
    {
        PyObject *eobj = sipConvertFromEnum(sipCpp->at(i),
                sipType_QAudioFormat_SampleType);

        if (!eobj)
        {
            Py_DECREF(l);

            return 0;
        }

        PyList_SET_ITEM(l, i, eobj);
    }

    return l;
%End

%ConvertToTypeCode
    PyObject *iter = PyObject_GetIter(sipPy);

    if (!sipIsErr)
    {
        Py_XDECREF(iter);

        return (iter
#if PY_MAJOR_VERSION < 3
                && !PyString_Check(sipPy)
#endif
                && !PyUnicode_Check(sipPy));
    }

    if (!iter)
    {
        *sipIsErr = 1;

        return 0;
    }

    QList<QAudioFormat::SampleType> *ql = new QList<QAudioFormat::SampleType>;

    for (Py_ssize_t i = 0; ; ++i)
    {
        PyErr_Clear();
        PyObject *itm = PyIter_Next(iter);

        if (!itm)
        {
            if (PyErr_Occurred())
            {
                delete ql;
                Py_DECREF(iter);
                *sipIsErr = 1;

                return 0;
            }

            break;
        }

        if (!sipCanConvertToEnum(itm, sipType_QAudioFormat_SampleType))
        {
            PyErr_Format(PyExc_TypeError,
                    "index %zd has type '%s' but 'QAudioFormat.SampleType' is expected",
                    i, Py_TYPE(itm)->tp_name);

            Py_DECREF(itm);
            delete ql;
            Py_DECREF(iter);
            *sipIsErr = 1;

            return 0;
        }

        ql->append(static_cast<QAudioFormat::SampleType>(SIPLong_AsLong(itm)));

        Py_DECREF(itm);
    }

    Py_DECREF(iter);

    *sipCppPtr = ql;

    return sipGetState(sipTransferObj);
%End
};


%If (Qt_5_6_0 -)

%MappedType QList<QAudio::Role>
        /TypeHintIn="Iterable[QAudio.Role]",
        TypeHintOut="List[QAudio.Role]", TypeHintValue="[]"/
{
%TypeHeaderCode
#include <qaudio.h>
%End

%ConvertFromTypeCode
    PyObject *l = PyList_New(sipCpp->size());

    if (!l)
        return 0;

    for (int i = 0; i < sipCpp->size(); ++i)
    {
        PyObject *eobj = sipConvertFromEnum(sipCpp->at(i),
                sipType_QAudio_Role);

        if (!eobj)
        {
            Py_DECREF(l);

            return 0;
        }

        PyList_SET_ITEM(l, i, eobj);
    }

    return l;
%End

%ConvertToTypeCode
    PyObject *iter = PyObject_GetIter(sipPy);

    if (!sipIsErr)
    {
        Py_XDECREF(iter);

        return (iter
#if PY_MAJOR_VERSION < 3
                && !PyString_Check(sipPy)
#endif
                && !PyUnicode_Check(sipPy));
    }

    if (!iter)
    {
        *sipIsErr = 1;

        return 0;
    }

    QList<QAudio::Role> *ql = new QList<QAudio::Role>;

    for (Py_ssize_t i = 0; ; ++i)
    {
        PyErr_Clear();
        PyObject *itm = PyIter_Next(iter);

        if (!itm)
        {
            if (PyErr_Occurred())
            {
                delete ql;
                Py_DECREF(iter);
                *sipIsErr = 1;

                return 0;
            }

            break;
        }

        if (!sipCanConvertToEnum(itm, sipType_QAudio_Role))
        {
            PyErr_Format(PyExc_TypeError,
                    "index %zd has type '%s' but 'QAudio.Role' is expected",
                    i, Py_TYPE(itm)->tp_name);

            Py_DECREF(itm);
            delete ql;
            Py_DECREF(iter);
            *sipIsErr = 1;

            return 0;
        }

        ql->append(static_cast<QAudio::Role>(SIPLong_AsLong(itm)));

        Py_DECREF(itm);
    }

    Py_DECREF(iter);

    *sipCppPtr = ql;

    return sipGetState(sipTransferObj);
%End
};

%End
