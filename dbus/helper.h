// This is the declaration of the helper class for the Qt specific support for
// the standard Python DBus bindings.
//
// Copyright (c) 2013 Riverbank Computing Limited
//
// Licensed under the Academic Free License version 2.1
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


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
