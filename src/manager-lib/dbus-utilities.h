/****************************************************************************
**
** Copyright (C) 2015 Pelagicore AG
** Contact: http://www.pelagicore.com/
**
** This file is part of the Pelagicore Application Manager.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Pelagicore Application Manager
** licenses may use this file in accordance with the commercial license
** agreement provided with the Software or, alternatively, in accordance
** with the terms contained in a written agreement between you and
** Pelagicore. For licensing terms and conditions, contact us at:
** http://www.pelagicore.com.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3 requirements will be
** met: http://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#if defined(QT_DBUS_LIB)
#  include <QDBusConnection>
#  include <QProcess>
#  include <QDBusContext>

Q_PID getDBusPeerPid(const QDBusConnection &conn);
#endif

struct DBusPolicy
{
    QList<uint> m_uids;
    QStringList m_executables;
    QStringList m_capabilities;
};

QMap<QByteArray, DBusPolicy> parseDBusPolicy(const QVariantMap &yamlFragment);

#if !defined(QT_DBUS_LIB)
typedef QObject QDBusContext; // evil hack :)
#endif

bool checkDBusPolicy(const QDBusContext *dbusContext, const QMap<QByteArray, DBusPolicy> &dbusPolicy, const QByteArray &function);