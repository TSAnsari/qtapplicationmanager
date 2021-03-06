TEMPLATE = lib
TARGET = QtAppManManager
MODULE = appman_manager

load(am-config)

QT = core network qml
!headless:QT *= gui gui-private quick qml-private quick-private
QT_FOR_PRIVATE *= \
    appman_common-private \
    appman_application-private \
    appman_notification-private \
    appman_plugininterfaces-private \
    appman_intent_server-private \
    appman_intent_client-private \

CONFIG *= static internal_module

multi-process {
    QT *= dbus
    LIBS *= -ldl

    HEADERS += \
        nativeruntime.h \
        nativeruntime_p.h \
        processcontainer.h \

    SOURCES += \
        nativeruntime.cpp \
        processcontainer.cpp \

    CONFIG = dbus-adaptors-xml $$CONFIG

    ADAPTORS_XML = \
        ../dbus-lib/io.qt.applicationmanager.intentinterface.xml
}

HEADERS += \
    application.h \
    applicationmanager.h \
    applicationmodel.h \
    applicationdatabase.h \
    notificationmanager.h \
    abstractcontainer.h \
    containerfactory.h \
    plugincontainer.h \
    abstractruntime.h \
    runtimefactory.h \
    quicklauncher.h \
    applicationipcmanager.h \
    applicationipcinterface.h \
    applicationipcinterface_p.h \
    applicationmanager_p.h \
    systemreader.h \
    debugwrapper.h \
    amnamespace.h \
    intentaminterface.h

linux:HEADERS += \
    sysfsreader.h \

!headless:HEADERS += \
    qmlinprocessapplicationmanagerwindow.h \
    inprocesssurfaceitem.h

qtHaveModule(qml):HEADERS += \
    qmlinprocessruntime.h \
    qmlinprocessapplicationinterface.h \

SOURCES += \
    application.cpp \
    applicationmanager.cpp \
    applicationmodel.cpp \
    applicationdatabase.cpp \
    notificationmanager.cpp \
    abstractcontainer.cpp \
    containerfactory.cpp \
    plugincontainer.cpp \
    abstractruntime.cpp \
    runtimefactory.cpp \
    quicklauncher.cpp \
    applicationipcmanager.cpp \
    applicationipcinterface.cpp \
    systemreader.cpp \
    debugwrapper.cpp \
    intentaminterface.cpp

linux:SOURCES += \
    sysfsreader.cpp \

!headless:SOURCES += \
    qmlinprocessapplicationmanagerwindow.cpp \
    inprocesssurfaceitem.cpp

qtHaveModule(qml):SOURCES += \
    qmlinprocessruntime.cpp \
    qmlinprocessapplicationinterface.cpp \

# we have an external plugin interface with signals, so we need to
# compile the moc-data into the exporting binary (appman itself)
HEADERS += ../plugin-interfaces/containerinterface.h

load(qt_module)
