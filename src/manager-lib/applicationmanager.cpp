/****************************************************************************
**
** Copyright (C) 2016 Pelagicore AG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Pelagicore Application Manager.
**
** $QT_BEGIN_LICENSE:GPL-QTAS$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
** SPDX-License-Identifier: GPL-3.0
**
****************************************************************************/

#include <QCoreApplication>
#include <QUrl>
#include <QFileInfo>
#include <QProcess>
#include <QDir>
#include <QTimer>
#include <QMimeDatabase>
#include <QDesktopServices>

#include "global.h"

#include "applicationdatabase.h"
#include "applicationmanager.h"
#include "application.h"
#include "runtimefactory.h"
#include "containerfactory.h"
#include "quicklauncher.h"
#include "abstractruntime.h"
#include "abstractcontainer.h"
#include "dbus-policy.h"
#include "qml-utilities.h"
#include "ipcproxyobject.h"


#define AM_AUTHENTICATE_DBUS(RETURN_TYPE) \
    do { \
        if (!checkDBusPolicy(this, d->dbusPolicy, __FUNCTION__)) \
            return RETURN_TYPE(); \
    } while (false);


/*!
    \class ApplicationManager
    \internal
*/

/*!
    \qmltype ApplicationManager
    \inqmlmodule QtApplicationManager 1.0
    \brief The ApplicationManager singleton

    This singleton class is the core of the application manager. It provides both
    a DBus and a QML API for all of its functionality.

    To make QML programmers lifes easier, the class is derived from \c QAbstractListModel,
    so you can directly use this singleton as a model in your app-grid views.

    \keyword ApplicationManager Roles

    The following roles are available in this model:

    \table
    \header
        \li \c Role name
        \li \c Type
        \li Description
    \row
        \li \c id
        \li \c string
        \li The unique of an application represented as a string in reverse-dns form (e.g.
            \c com.pelagicore.foo)
    \row
        \li \c name
        \li \c string
        \li The name of the application. If possible already translated to the current locale.
    \row
        \li \c icon
        \li \c string
        \li The URL of the application's icon.

    \row
        \li \c isRunning
        \li \c bool
        \li A boolean value representing the run-state of the application.
    \row
        \li \c isStarting
        \li \c bool
        \li A boolean value telling if the application was started, but is not fully operational yet.
    \row
        \li \c isActive
        \li \c bool
        \li A boolean value describing if the application is currently the active foreground application.
    \row
        \li \c isBlocked
        \li \c bool
        \li A boolean value that only gets set if the application manager needs to block the application
        from running: this is normally only the case while an update is applied.
    \row
        \li \c isUpdating
        \li \c bool
        \li A boolean value telling if the application is currently being installed or updated. If this
            is the case, then \c updateProgress can be used to track the actual progress.
    \row
        \li \c isRemovable
        \li \c bool
        \li A boolean value telling if this application is user-removable. This will be \c true for all
            dynamically installed 3rd-party applications and \c false for all system applications.

    \row
        \li \c updateProgress
        \li \c real
        \li In case IsUpdating is \c true, querying this role will give the actual progress as a floating-point
            value in the \c 0.0 to \c 1.0 range.

    \row
        \li \c codeFilePath
        \li \c string
        \li The filesystem path to the main "executable". The format of this file is dependent on the
        actual runtime though: for QML applications the "executable" is the \c main.qml file.

    \row
        \li \c categories
        \li \c list<string>
        \li The categories this application is registered for via its meta-dat file (this currently work in progress).

    \row
        \li \c version
        \li \c string
        \li The currently installed version of this application.
    \endtable

    Please note, that the index-based API is currently not available via DBus. The same functionality
    is however provided by the id-based API.

    The QML import for this singleton is

    \c{import QtApplicationManager 1.0}

    After importing, you can just use the ApplicationManager singleton like so:

    \qml
    import QtQuick 2.0
    import QtApplicationManager 1.0

    ListView {
        id: appList
        model: ApplicationManager

        delegate: Text {
            text: name + "(" + id + ")"

            MouseArea {
                anchors.fill: parent
                onClick: ApplicationManager.startApplication(id)
            }
        }
    }
    \endqml
*/

/*!
    \qmlproperty int ApplicationManager::count

    This property holds the number of applications available.
*/

/*!
    \qmlsignal ApplicationManager::applicationWasReactivated(string id)

    This signal is emitted when an application is already running (in the background) and is
    started via the ApplicationManager API again.
    The window manager should take care of raising the application's window in this case.
*/


class ApplicationManagerPrivate
{
public:
    bool securityChecksEnabled;
    QVariantMap additionalConfiguration;
    ApplicationDatabase *database;

    QMap<QByteArray, DBusPolicy> dbusPolicy;

    QVector<const Application *> apps;

    QString currentLocale;
    QHash<int, QByteArray> roleNames;

    QMap<const Application *, QVariantMap> applicationState;

    QVector<IpcProxyObject *> interfaceExtensions;

    ApplicationManagerPrivate()
        : securityChecksEnabled(true)
        , database(0)
    {
        currentLocale = QLocale::system().name(); //TODO: language changes

        roleNames.insert(ApplicationManager::Id, "applicationId");
        roleNames.insert(ApplicationManager::Name, "name");
        roleNames.insert(ApplicationManager::Icon, "icon");
        roleNames.insert(ApplicationManager::IsRunning, "isRunning");
        roleNames.insert(ApplicationManager::IsStarting, "isStarting");
        roleNames.insert(ApplicationManager::IsActive, "isActive");
        roleNames.insert(ApplicationManager::IsBlocked, "isLocked");
        roleNames.insert(ApplicationManager::IsUpdating, "isUpdating");
        roleNames.insert(ApplicationManager::IsRemovable, "isRemovable");
        roleNames.insert(ApplicationManager::UpdateProgress, "updateProgress");
        roleNames.insert(ApplicationManager::CodeFilePath, "codeFilePath");
        roleNames.insert(ApplicationManager::RuntimeName, "runtimeName");
        roleNames.insert(ApplicationManager::RuntimeParameters, "runtimeParameters");
        roleNames.insert(ApplicationManager::BackgroundMode, "backgroundMode");
        roleNames.insert(ApplicationManager::Capabilities, "capabilities");
        roleNames.insert(ApplicationManager::Categories, "categories");
        roleNames.insert(ApplicationManager::Importance, "importance");
        roleNames.insert(ApplicationManager::Preload, "preload");
        roleNames.insert(ApplicationManager::Version, "version");
    }

    ~ApplicationManagerPrivate()
    {
        delete database;
    }
};

ApplicationManager *ApplicationManager::s_instance = 0;

ApplicationManager *ApplicationManager::createInstance(ApplicationDatabase *adb, QString *error)
{
    if (s_instance) {
        if (error)
            *error = qL1S("ApplicationManager::createInstance() was called a second time.");
        return 0;
    }

    QScopedPointer<ApplicationManager> am(new ApplicationManager(adb, QCoreApplication::instance()));

    try {
        if (adb)
            am->d->apps = adb->read();
        am->registerMimeTypes();
    } catch (const Exception &e) {
        if (error)
            *error = e.what();
        return 0;
    }

    return s_instance = am.take();
}

ApplicationManager *ApplicationManager::instance()
{
    if (!s_instance)
        qFatal("ApplicationManager::instance() was called before createInstance().");
    return s_instance;
}

QObject *ApplicationManager::instanceForQml(QQmlEngine *qmlEngine, QJSEngine *)
{
    if (qmlEngine)
        retakeSingletonOwnershipFromQmlEngine(qmlEngine, instance());
    return instance();
}

ApplicationManager::ApplicationManager(ApplicationDatabase *adb, QObject *parent)
    : QAbstractListModel(parent)
    , d(new ApplicationManagerPrivate())
{
    d->database = adb;
    connect(this, &QAbstractItemModel::rowsInserted, this, &ApplicationManager::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &ApplicationManager::countChanged);
    connect(this, &QAbstractItemModel::layoutChanged, this, &ApplicationManager::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &ApplicationManager::countChanged);

    QTimer::singleShot(0, this, &ApplicationManager::preload);
}

ApplicationManager::~ApplicationManager()
{
    delete d;
}

bool ApplicationManager::securityChecksEnabled() const
{
    return d->securityChecksEnabled;
}

void ApplicationManager::setSecurityChecksEnabled(bool enabled)
{
    d->securityChecksEnabled = enabled;
}

QVariantMap ApplicationManager::additionalConfiguration() const
{
    return d->additionalConfiguration;
}

void ApplicationManager::setAdditionalConfiguration(const QVariantMap &map)
{
    d->additionalConfiguration = map;
}

bool ApplicationManager::setDBusPolicy(const QVariantMap &yamlFragment)
{
    static const QVector<QByteArray> functions {
        QT_STRINGIFY(applicationIds),
        QT_STRINGIFY(get),
        QT_STRINGIFY(startApplication),
        QT_STRINGIFY(stopApplication),
        QT_STRINGIFY(openUrl),
        QT_STRINGIFY(capabilities),
        QT_STRINGIFY(identifyApplication),
        QT_STRINGIFY(applicationState)
    };

    d->dbusPolicy = parseDBusPolicy(yamlFragment);

    foreach (const QByteArray &f, d->dbusPolicy.keys()) {
       if (!functions.contains(f))
           return false;
    }
    return true;
}

QVector<const Application *> ApplicationManager::applications() const
{
    return d->apps;
}

const Application *ApplicationManager::fromId(const QString &id) const
{
    foreach (const Application *app, d->apps) {
        if (app->id() == id)
            return app;
    }
    return 0;
}

const Application *ApplicationManager::fromProcessId(qint64 pid) const
{
    if (!pid)
        return 0;

    foreach (const Application *app, d->apps) {
        if (app->currentRuntime() && (app->currentRuntime()->applicationProcessId() == pid))
            return app;
    }
    return 0;
}

const Application *ApplicationManager::fromSecurityToken(const QByteArray &securityToken) const
{
    if (securityToken.size() != AbstractRuntime::SecurityTokenSize)
        return 0;

    foreach (const Application *app, d->apps) {
        if (app->currentRuntime() && app->currentRuntime()->securityToken() == securityToken)
            return app;
    }
    return 0;
}

const Application *ApplicationManager::schemeHandler(const QString &scheme) const
{
    foreach (const Application *app, d->apps) {
        foreach (const QString &mime, app->supportedMimeTypes()) {
            int pos = mime.indexOf(QLatin1Char('/'));

            if ((pos > 0)
                    && (mime.left(pos) == qL1S("x-scheme-handler"))
                    && (mime.mid(pos + 1) == scheme)) {
                return app;
            }
        }
    }
    return 0;
}

const Application *ApplicationManager::mimeTypeHandler(const QString &mimeType) const
{
    foreach (const Application *app, d->apps) {
        if (app->supportedMimeTypes().contains(mimeType))
            return app;
    }
    return 0;
}

void ApplicationManager::registerMimeTypes()
{
    QVector<QString> schemes;
    schemes << qSL("file") << qSL("http") << qSL("https");

    foreach (const Application *a, d->apps) {
        if (a->isAlias())
            continue;

        foreach (const QString &mime, a->supportedMimeTypes()) {
            int pos = mime.indexOf(QLatin1Char('/'));

            if ((pos > 0) && (mime.left(pos) == qL1S("x-scheme-handler")))
                schemes << mime.mid(pos + 1);
        }
    }
    foreach (const QString &scheme, schemes)
        QDesktopServices::setUrlHandler(scheme, this, "openUrlRelay");
}

bool ApplicationManager::startApplication(const Application *app, const QString &documentUrl)
{
    if (!app) {
        qCWarning(LogSystem) << "Application *app is NULL";
        return false;
    }
    if (app->isLocked()) {
        qCWarning(LogSystem) << "Application *app (" << app->id() << ") is blocked";
        return false;
    }
    AbstractRuntime *runtime = app->currentRuntime();

    if (runtime) {
        switch (runtime->state()) {
        case AbstractRuntime::Startup:
        case AbstractRuntime::Active:
            if (!documentUrl.isNull())
                runtime->openDocument(documentUrl);
            else if (!app->documentUrl().isNull())
                runtime->openDocument(app->documentUrl());

            emit applicationWasReactivated(app->isAlias() ? app->nonAliased()->id() : app->id());
            return true;

        case AbstractRuntime::Shutdown:
            return false;

        case AbstractRuntime::Inactive:
            break;
        }
    }

    bool inProcess = RuntimeFactory::instance()->manager(app->runtimeName())->inProcess();
    AbstractContainer *container = nullptr;
    QString containerId = qSL("process"); //TODO: ask SystemUI or use config file
    bool attachRuntime = false;

    if (!runtime) {
        if (!inProcess) {
            // check quicklaunch pool
            QPair<AbstractContainer *, AbstractRuntime *> quickLaunch =
                    QuickLauncher::instance()->take(containerId, app->m_runtimeName);
            container = quickLaunch.first;
            runtime = quickLaunch.second;

            qCDebug(LogSystem) << "Found a quick-launch entry for container" << containerId
                               << "and runtime" << app->m_runtimeName << "->" << container << runtime;

            if (!container && runtime) {
                runtime->deleteLater();
                qCCritical(LogSystem) << "ERROR: QuickLauncher provided a runtime without a container.";
                return false;
            }

            if (!container)
                container = ContainerFactory::instance()->create(containerId);
            if (!container) {
                qCCritical(LogSystem) << "ERROR: Couldn't create Container for Application (" << app->id() <<")!";
                return false;
            }
            if (runtime)
                attachRuntime = true;
        }
        if (!runtime)
            runtime = RuntimeFactory::instance()->create(container, app);

        if (runtime && inProcess)  {
            // evil hook to get in-process mode working transparently
            emit inProcessRuntimeCreated(runtime);
        }
    }

    if (!runtime) {
        qCCritical(LogSystem) << "ERROR: Couldn't create Runtime for Application (" << app->id() <<")!";
        return false;
    }

    if (!documentUrl.isNull())
        runtime->openDocument(documentUrl);
    else if (!app->documentUrl().isNull())
        runtime->openDocument(app->documentUrl());

    qCDebug(LogSystem) << "app:" << app->id() << "; document:" << documentUrl << "; runtime: " << runtime;

    if (inProcess) {
        bool ok = runtime->start();
        if (!ok)
            runtime->deleteLater();
        return ok;
    } else {
        auto f = [=]() {
            qCDebug(LogSystem) << "Container ready for app (" << app->id() <<")!";
            bool successfullyStarted = attachRuntime ? runtime->attachApplicationToQuickLauncher(app)
                                                     : runtime->start();
            if (!successfullyStarted)
                runtime->deleteLater(); // ~Runtime() will clean app->m_runtime

            return successfullyStarted;
        };

        if (container->isReady()) {
            // Since the container is already ready, start the app immediately
            return f();
        }
        else {
            // We postpone the starting of the application to a later point in time since the container is not ready yet
            connect(container, &AbstractContainer::ready, f);
            return true;       // we return true for now, since we don't know at this point in time whether the container will be able to start the application. TODO : fix
        }
    }
}

void ApplicationManager::stopApplication(const Application *app, bool forceKill)
{
    if (!app)
        return;
    AbstractRuntime *rt = app->currentRuntime();
    if (rt) {
        rt->stop(forceKill);
        rt->deleteLater(); // ~Runtime() will clean app->m_runtime
    }
}

/*!
    \qmlmethod bool ApplicationManager::startApplication(string id, string document)

    Tells the application manager to start the application identified by its unique \a id. The
    optional argument \a documentUrl will be supplied to the application as is - most commonly this
    is used to refer to a document to display.
    Returns \c true if the application id is valid and the application manager was able to start
    the runtime plugin. Returns \c false otherwise. Please note, that even though this call may
    indicate success, the application may still later fail to start correctly, since the actual
    startup within the runtime plugin may be asynchronous.
*/
bool ApplicationManager::startApplication(const QString &id, const QString &documentUrl)
{
    AM_AUTHENTICATE_DBUS(bool)

    return startApplication(fromId(id), documentUrl);
}

/*!
    \qmlmethod ApplicationManager::stopApplication(string id, bool forceKill)

    Tells the application manager to stop an application identified by its unique \a id. The
    meaning of the \a forceKill parameter is runtime dependent, but in general you should always try
    to stop an application with \a forceKill set to \c false first in order to allow a clean
    shutdown. Use \a forceKill set to \c true only as last resort to kill hanging applications.
*/
void ApplicationManager::stopApplication(const QString &id, bool forceKill)
{
    AM_AUTHENTICATE_DBUS(void)

    return stopApplication(fromId(id), forceKill);
}

/*!
    \qmlmethod bool ApplicationManager::openUrl(string url)

    Tries to match the supplied \a url against the internal MIME database. If a match is found,
    the corresponding application is started via startApplication() and \a url is supplied as a
    document to open.

    The function will return the result of startApplication() or \c false if no application was
    registered for the type and/or scheme of \a url.
*/
bool ApplicationManager::openUrl(const QString &urlStr)
{
    AM_AUTHENTICATE_DBUS(bool)

    QUrl url(urlStr);
    const Application *app = 0;
    if (url.isValid()) {
        QString scheme = url.scheme();
        if (scheme != qL1S("file"))
            app = schemeHandler(scheme);

        if (!app) {
            QMimeDatabase mdb;
            QMimeType mt = mdb.mimeTypeForUrl(url);

            app = mimeTypeHandler(mt.name());
        }
    }
    if (app)
        return startApplication(app, urlStr);
    return false;
}

/*!
    \qmlmethod list<string> ApplicationManager::capabilities(string id)

    Returns a list of all capabilities granted by the user to the application identified by \a id.
    Will return an empty list, if the application \a id is not valid.
*/
QStringList ApplicationManager::capabilities(const QString &id)
{
    AM_AUTHENTICATE_DBUS(QStringList)

    const Application *app = fromId(id);

    if (!app)
        return QStringList();

    return app->capabilities();
}

/*!
    \qmlmethod string ApplicationManager::identifyApplication(int pid, string securityToken)

    Calling this function will validate that the process running with process-identifier \a pid
    was indeed started by the application manager.
    Will return the application's \c id on success or an empty string otherwise.
*/
QString ApplicationManager::identifyApplication(qint64 pid)
{
    AM_AUTHENTICATE_DBUS(QString)

    const Application *app1 = fromProcessId(pid);

    if (app1)
        return app1->id();
    else
        return QString();
}

bool ApplicationManager::lockApplication(const QString &id)
{
    const Application *app = fromId(id);
    if (!app)
        return false;
    if (!app->lock())
        return false;
    emitDataChanged(app);
    stopApplication(app, true);
    emitDataChanged(app);
    return true;
}

bool ApplicationManager::unlockApplication(const QString &id)
{
    const Application *app = fromId(id);
    if (!app)
        return false;
    if (!app->unlock())
        return false;
    emitDataChanged(app);
    return true;
}

bool ApplicationManager::startingApplicationInstallation(Application *installApp)
{
    if (!installApp || installApp->id().isEmpty())
        return false;
    const Application *app = fromId(installApp->id());

    if (app) { // update
        if (!lockApplication(app->id()))
            return false;
        installApp->mergeInto(const_cast<Application *>(app));
        app->m_state = Application::BeingUpdated;
        app->m_progress = 0;
        emitDataChanged(app);
    } else { // installation
        installApp->m_locked.ref();
        installApp->m_state = Application::BeingInstalled;
        installApp->m_progress = 0;
        beginInsertRows(QModelIndex(), d->apps.count(), d->apps.count());
        d->apps << installApp;
        endInsertRows();
        try {
            if (d->database)
                d->database->write(d->apps);
        } catch (const Exception &) {
            beginRemoveRows(QModelIndex(), d->apps.count() - 1, d->apps.count() - 1);
            d->apps.removeLast();
            endRemoveRows();
            delete installApp;
            return false;
        }
        emitDataChanged(app);
    }
    return true;
}

bool ApplicationManager::startingApplicationRemoval(const QString &id)
{
    const Application *app = fromId(id);
    if (!app || app->m_locked.load() || (app->m_state != Application::Installed))
        return false;

    if (app->isBuiltIn())
        return false;

    if (!lockApplication(id))
        return false;

    app->m_state = Application::BeingRemoved;
    app->m_progress = 0;
    emitDataChanged(app);
    return true;
}

void ApplicationManager::progressingApplicationInstall(const QString &id, qreal progress)
{
    const Application *app = fromId(id);
    if (!app || app->m_state == Application::Installed)
        return;
    app->m_progress = progress;
    emitDataChanged(app);
}

bool ApplicationManager::finishedApplicationInstall(const QString &id)
{
    const Application *app = fromId(id);
    if (!app)
        return false;

    switch (app->m_state) {
    case Application::Installed:
        return false;

    case Application::BeingInstalled:
    case Application::BeingUpdated:
        app->m_state = Application::Installed;
        app->m_progress = 0;
        emitDataChanged(app);

        unlockApplication(id);
        break;

    case Application::BeingRemoved: {
        int row = d->apps.indexOf(app);
        if (row >= 0) {
            beginRemoveRows(QModelIndex(), row, row);
            d->apps.removeAt(row);
            endRemoveRows();
        }
        delete app;
        try {
            if (d->database)
                d->database->write(d->apps);
        } catch (const Exception &e) {
            qCCritical(LogSystem) << "ERROR: Application was removed successfully, but we could not save the app database:" << e.what();
            return false;
        }
        break;
    }
    }

    return true;
}

bool ApplicationManager::canceledApplicationInstall(const QString &id)
{
    const Application *app = fromId(id);
    if (!app)
        return false;

    switch (app->m_state) {
    case Application::Installed:
        return false;

    case Application::BeingInstalled: {
        int row = d->apps.indexOf(app);
        if (row >= 0) {
            beginRemoveRows(QModelIndex(), row, row);
            d->apps.removeAt(row);
            endRemoveRows();
        }
        delete app;
        break;
    }
    case Application::BeingUpdated:
    case Application::BeingRemoved:
        app->m_state = Application::Installed;
        app->m_progress = 0;
        emitDataChanged(app);

        unlockApplication(id);
        break;
    }
    return true;
}

void ApplicationManager::preload()
{
    bool forcePreload = d->database && d->database->isTemporary();

    foreach (const Application *app, d->apps) {
        if (forcePreload || app->isPreloaded()) {
            if (!startApplication(app)) {
                qCWarning(LogSystem) << "WARNING: unable to start preload-enabled application" << app->id();
            }
        }
    }
}

void ApplicationManager::openUrlRelay(const QUrl &url)
{
    openUrl(url.toString());
}

void ApplicationManager::emitDataChanged(const Application *app)
{
    int row = d->apps.indexOf(app);
    if (row >= 0)
        emit dataChanged(index(row), index(row));
}

// item model part

int ApplicationManager::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return d->apps.count();
}

QVariant ApplicationManager::data(const QModelIndex &index, int role) const
{
    if (index.parent().isValid() || !index.isValid())
        return QVariant();

    const Application *app = d->apps.at(index.row());

    switch (role) {
    case Id:
        return app->id();
    case Name: {
        QString name;
        if (!app->names().isEmpty()) {
            name = app->name(d->currentLocale);
            if (name.isEmpty())
                name = app->name(qSL("en"));
            if (name.isEmpty())
                name = app->name(qSL("en_US"));
            if (name.isEmpty())
                name = *app->names().constBegin();
        } else {
            name = app->id();
        }
        return name;
    }
    case Icon:
        return QUrl::fromLocalFile(app->icon());

    case IsRunning:
        return app->currentRuntime() ? (app->currentRuntime()->state() == AbstractRuntime::Active) : false;
    case IsStarting:
        return app->currentRuntime() ? (app->currentRuntime()->state() == AbstractRuntime::Startup) : false;
    case IsActive:
        return false;
    case IsBlocked:
        return app->isLocked();
    case IsUpdating:
        return app->state() != Application::Installed;
    case UpdateProgress:
        return app->progress();
    case IsRemovable:
        return !app->isBuiltIn();

    case CodeFilePath:
        return app->absoluteCodeFilePath();
    case RuntimeName:
        return app->runtimeName();
    case RuntimeParameters:
        return app->runtimeParameters();
    case BackgroundMode: {
        switch (app->backgroundMode()) {
        case Application::Auto:
            return qL1S("Auto");
        case Application::Never:
            return qL1S("Never");
        case Application::ProvidesVoIP:
            return qL1S("ProvidesVoIP");
        case Application::PlaysAudio:
            return qL1S("PlaysAudio");
        case Application::TracksLocation:
            return qL1S("TracksLocation");
        }
        return QString();
    }
    case Capabilities:
        return app->capabilities();
    case Categories:
        return app->categories();
    case Importance:
        return app->importance();
    case Preload:
        return app->isPreloaded();
    case Version:
        return app->version();
    }
    return QVariant();
}

QHash<int, QByteArray> ApplicationManager::roleNames() const
{
    return d->roleNames;
}

int ApplicationManager::count() const
{
    return rowCount();
}

/*!
    \qmlmethod object ApplicationManager::get(int row) const

    Retrieves the model data at \a row as a JavaScript object. Please see the
    \l {ApplicationManager Roles}{role names} for the expected object fields.

    Will return an empty object, if the specified \a row is invalid.
*/
QVariantMap ApplicationManager::get(int row) const
{
    if (row < 0 || row >= count()) {
        qCWarning(LogSystem) << "invalid index:" << row;
        return QVariantMap();
    }

    QVariantMap map;
    QHash<int, QByteArray> roles = roleNames();
    for (auto it = roles.begin(); it != roles.end(); ++it)
        map.insert(it.value(), data(index(row), it.key()));
    return map;
}

/*!
    \qmlmethod int ApplicationManager::indexFromId(string id) const

    Maps the application \a id to its position within the model.

    Will return \c -1, if the specified \a id is invalid.
*/
int ApplicationManager::indexFromId(const QString &id) const
{
    for (int i = 0; i < d->apps.size(); ++i) {
        if (d->apps.at(i)->id() == id)
            return i;
    }
    return -1;
}

/*!
    \qmlmethod list<string> ApplicationManager::applicationIds()

    Returns a list of all available application ids. This can be used to further query for specific
    information via get().
*/
QStringList ApplicationManager::applicationIds() const
{
    AM_AUTHENTICATE_DBUS(QStringList)

    QStringList ids;
    ids.reserve(d->apps.size());
    for (int i = 0; i < d->apps.size(); ++i)
        ids << d->apps.at(i)->id();
    return ids;
}

/*!
    \qmlmethod object ApplicationManager::get(string id)

    Retrieves the model data for the application identified by \a id as a JavaScript object. Please
    see the \l {ApplicationManager Roles}{role names} for the expected object fields.

    Will return an empty object, if the specified \a id is invalid.
*/
QVariantMap ApplicationManager::get(const QString &id) const
{
    AM_AUTHENTICATE_DBUS(QVariantMap)

    int index = indexFromId(id);
    if (index >= 0)
        return get(index);
    return QVariantMap();
}


void ApplicationManager::setApplicationAudioFocus(const QString &id, AudioFocus audioFocus)
{
    int index = indexFromId(id);
    if (index < 0)
        return;
    QString audioFocusName = qSL("audioFocus");
    QString audioFocusState = qSL("none");
    switch (audioFocus) {
    case FullscreenFocus : audioFocusState = qSL("fullscreen"); break;
    case SplitscreenFocus: audioFocusState = qSL("splitscreen"); break;
    case BackgroundFocus : audioFocusState = qSL("background"); break;
    case NoFocus         :
    default              : break;
    }
    d->applicationState[d->apps.at(index)][audioFocusName] = audioFocusState;
    QVariantMap map;
    map.insert(audioFocusName, audioFocusState);
    emit applicationStateChanged(id, map);
}

/*!
    \qmlmethod bool ApplicationManager::registerApplicationInterfaceExtension(object interface, string name, var filter)

    Registers an IPC \a interface object to extend the communication API between applications and
    the Application Manager itself. The \a interface object is a normal QtObject item, that needs
    to stay valid during the whole lifetime of the system-UI. The \a name of the interface has to
    adhere to D-Bus standards, so it needs to at least contain one \c . character (e.g. \c{io.qt.test}).
    The interface is available to all applications matching the \a filter criteria (see below)
    on the private Peer-To-Peer D-Bus as a standard, typed D-Bus interface.

    Since there is no way to add type information to the parameters of JavaScript functions, the
    \a interface is scanned for special annotation properties that are only used to deduce type
    information, but are not exported to the applications:

    \c{readonly property var _decltype_<function-name>: { "<return type>": [ "<param 1 type>", "<param 2 type>", ...] }}

    The following types are supported:

    \table
    \header
        \li Type
        \li DBus Type
        \li Note
    \row
        \li \c void
        \li -
        \li Only valid as return type.
    \row
        \li \c int
        \li \c INT32
        \li
    \row
        \li \c string, \c url
        \li \c STRING
        \li Urls are mapped to plain strings.
    \row
        \li \c bool
        \li \c BOOLEAN
        \li
    \row
        \li \c real, \c double
        \li \c DOUBLE
        \li
    \row
        \li \c var, \c variant
        \li \c VARIANT
        \li Can also be used for lists and maps.

    \endtable

    This is a simple example showing how to add these annotations to a function definition:

    \code
    readonly property var _decltype_testFunction: { "var": [ "int", "string" ] }
    function testFunction(ivar, strvar) {
        return { "strvar": strvar, "intvar": intvar }
    }
    \endcode

    You can restrict the availability of the interface in applications via the \a filter parameter:
    either pass an empty JavaScript object (\c{{}}) or use any combination of these available
    field names:

    \table
    \header
        \li Key
        \li Value
        \li Description
    \row
        \li \c applicationIds
        \li \c list<string>
        \li A list of applications ids that are allowed to use this interface.
    \row
        \li \c categories
        \li \c list<string>
        \li A list of category names (see info.yaml). Any application that is part of one of these
            categories is allowed to use this interface.
    \row
        \li \c capabilities
        \li \c list<string>
        \li A list of applications capabilities (see info.yaml). Any application that has on of these
            capabilities is allowed to use this interface.
    \endtable

    All of the filter fields have to match, but only fields that are actually set are taken into
    consideration.

    \qml
    import QtQuick 2.0
    import QtApplicationManager 1.0

    QtObject {
        id: extension

        property bool pbool: true
        property double pdouble: 3.14

        signal testSignal(string str, variant list)

        readonly property var _decltype_testFunction: { "void": [ "int", "string" ] }
        function testFunction(foo, bar) {
            console.log("testFunction was called: " + foo + " " + bar)
        }
    }
    \endqml
    \code
    ...

    ApplicationManager.registerApplicationInterfaceExtension(extension, "io.qt.test.interface",
                                                             { "capabilities": [ "media", "camera" ] })

    \endcode

    \note Currently only implemented for multi-process setups.

    Will return \c true if the registration was successful or \c false otherwise.
*/
bool ApplicationManager::registerApplicationInterfaceExtension(QObject *object, const QString &name,
                                                               const QVariantMap &filter)
{
    if (!object || name.isEmpty()) {
        qCWarning(LogQmlIpc) << "Application interface extension needs a valid object as well as interface name";
        return false;
    }
    if (!name.contains(QLatin1Char('.'))) {
        qCWarning(LogQmlIpc) << "Application interface extension name" << name << "is not a valid D-Bus interface name";
        return false;
    }
    foreach (const auto &ext, d->interfaceExtensions) {
        if (ext->interfaceName() == name) {
            qCWarning(LogQmlIpc) << "Application interface extension name" << name << "was already registered";
            return false;
        }
    }
    IpcProxyObject *ipo = new IpcProxyObject(object, QString(), qSL("/ExtensionInterfaces"), name, filter);
    d->interfaceExtensions.append(ipo);
    return true;
}

QVector<IpcProxyObject *> ApplicationManager::applicationInterfaceExtensions() const
{
    return d->interfaceExtensions;
}

QVariantMap ApplicationManager::applicationState(const QString &id) const
{
    AM_AUTHENTICATE_DBUS(QVariantMap)

    int index = indexFromId(id);
    if (index < 0)
        return QVariantMap();
    return d->applicationState.value(d->apps.at(index));
}
