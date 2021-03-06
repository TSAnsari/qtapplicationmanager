/****************************************************************************
**
** Copyright (C) 2019 Luxoft Sweden AB
** Copyright (C) 2018 Pelagicore AG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Luxoft Application Manager.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT-QTAS$
** Commercial License Usage
** Licensees holding valid commercial Qt Automotive Suite licenses may use
** this file in accordance with the commercial license agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and The Qt Company.  For
** licensing terms and conditions see https://www.qt.io/terms-conditions.
** For further information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include <QQmlExtensionPlugin>
#include <QThread>


class TerminatorPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override;
};


class Terminator : public QObject
{
    Q_OBJECT

public:
    Terminator(QObject *parent) : QObject(parent) {}

    Q_INVOKABLE void accessIllegalMemory() const;
    Q_INVOKABLE void accessIllegalMemoryInThread();
    Q_INVOKABLE void forceStackOverflow() const;
    Q_INVOKABLE void divideByZero() const;
    Q_INVOKABLE void abort() const;
    Q_INVOKABLE void raise(int sig) const;
    Q_INVOKABLE void throwUnhandledException() const;
    Q_INVOKABLE void exitGracefully() const;
};


class TerminatorThread : public QThread
{
    Q_OBJECT

public:
    explicit TerminatorThread(Terminator *parent);

private:
    void run() override;
    Terminator *m_terminator;
};
