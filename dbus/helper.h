// This is the declaration of the helper class for the Qt specific support for
// the standard Python DBus bindings.
//
// Copyright (c) 2014 Riverbank Computing Limited
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <QHash>
#include <QList>
#include <QMultiHash>
#include <QObject>
#include <QPointer>
#include <QSocketNotifier>

#include <dbus/dbus.h>


class QTimerEvent;


class pyqt5DBusHelper : public QObject
{
    Q_OBJECT

public:
    struct Watcher
    {
        Watcher() : watch(0), read(0), write(0) {}

        DBusWatch *watch;
        QPointer<QSocketNotifier> read;
        QPointer<QSocketNotifier> write;
    };

    typedef QMultiHash<int, Watcher> Watchers;
    typedef QHash<int, DBusTimeout *> Timeouts;
    typedef QList<DBusConnection *>Connections;

    pyqt5DBusHelper();

    Watchers watchers;
    Timeouts timeouts;
    Connections connections;

public slots:
    void readSocket(int fd);
    void writeSocket(int fd);
    void dispatch();

protected:
    void timerEvent(QTimerEvent *e);
};
