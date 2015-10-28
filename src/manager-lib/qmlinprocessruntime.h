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

#include "abstractruntime.h"

class FakePelagicoreWindow;
class QmlInProcessApplicationInterface;

class QmlInProcessRuntime : public AbstractRuntime
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit QmlInProcessRuntime(QObject *parent = 0);
    ~QmlInProcessRuntime();

    static QString identifier() { return "qml-inprocess"; }

    virtual bool inProcess() const override;

    bool create(const Application *app);

    State state() const;

    void openDocument(const QString &document) override;

    Q_PID applicationPID() const override;


public slots:
    bool start();
    void stop(bool forceKill = false);

signals:
    void aboutToStop(); // used for the ApplicationInterface

private slots:
#if !defined(AM_HEADLESS)
    void onWindowClose();
    void onWindowDestroyed();

    void onEnableFullscreen();
    void onDisableFullscreen();
#endif

private:
    QString m_document;
    QmlInProcessApplicationInterface *m_applicationIf = 0;

#if !defined(AM_HEADLESS)
    // used by FakePelagicoreWindow to register windows
    void addWindow(QQuickItem *window);

    FakePelagicoreWindow *m_mainWindow = 0;
    QList<QQuickItem *> m_windows;

    friend class FakePelagicoreWindow; // for emitting signals on behalf of this class in onComplete
#endif
};