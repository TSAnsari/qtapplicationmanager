/****************************************************************************
**
** Copyright (C) 2015 Pelagicore AG
** Contact: http://www.qt.io/ or http://www.pelagicore.com/
**
** This file is part of the Pelagicore Application Manager.
**
** $QT_BEGIN_LICENSE:GPL3-PELAGICORE$
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
** SPDX-License-Identifier: GPL-3.0
**
****************************************************************************/

#pragma once

#include "window.h"

#include <QObject>
#include <QPointer>

class InProcessWindow : public Window
{
public:
    InProcessWindow(const Application *app, QQuickItem *surfaceItem);

    bool isInProcess() const { return true; }

    //bool isClosing() const override;

    bool setWindowProperty(const QString &name, const QVariant &value) override;
    QVariant windowProperty(const QString &name) const override;
    QVariantMap windowProperties() const override;

protected:
    bool eventFilter(QObject *o, QEvent *e);

private:
    QVariantMap m_windowProperties;
};
