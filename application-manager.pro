
MIN_MINOR=4
headless:MIN_MINOR=2

!equals(QT_MAJOR_VERSION, 5)|lessThan(QT_MINOR_VERSION, $$MIN_MINOR):error("This application needs to be built against Qt 5.$${MIN_MINOR}+")

TEMPLATE = subdirs
CONFIG += ordered

include($$BASE_PRI)

bundled-libarchive:SUBDIRS += 3rdparty/libarchive/libarchive.pro
bundled-libyaml:SUBDIRS += 3rdparty/libyaml/libyaml.pro

force-singleprocess:force-multiprocess:error("You cannot both specify force-singleprocess and force-multiprocess")
qtHaveModule(compositor)|headless { check_multi = "yes (auto detect)" } else { check_multi = "no (auto detect)" }
force-singleprocess { check_multi = "no (force-singleprocess)" }
force-multiprocess  { check_multi = "yes (force-multiprocess)" }
CONFIG(debug, debug|release) { check_debug = "debug" } else { check_debug = "release" }

!isEmpty(AM_HARDWARE_ID_FF):check_hwid = "(from file) $$AM_HARDWARE_ID_FF"
else:!isEmpty(AM_HARDWARE_ID):check_hwid = "$$AM_HARDWARE_ID"
else:check_hwid = "auto (derived from network MAC address)"

isEmpty(AM_LIBCRYPTO_DEFINES) { check_libcrypto_defines = "(none)" } else { check_libcrypto_defines = $$AM_LIBCRYPTO_DEFINES }
isEmpty(AM_LIBCRYPTO_INCLUDES) { check_libcrypto_includes = "(system)" } else { check_libcrypto_includes = $$AM_LIBCRYPTO_INCLUDES }

printConfigLine()
printConfigLine("-- Application Manager configuration", , orange)
printConfigLine("Version", $$GIT_VERSION, white)
printConfigLine("Hardware Id", $$check_hwid, auto)
printConfigLine("Qt version", $$[QT_VERSION], white)
printConfigLine("Qt installation", $$[QT_HOST_BINS])
printConfigLine("Debug or release", $$check_debug, white)
printConfigLine("Installation prefix", $$INSTALL_PREFIX, auto)
printConfigLine("Headless", $$yesNo(CONFIG(headless)), auto)
printConfigLine("QtCompositor support", $$yesNo(qtHaveModule(compositor)), auto)
printConfigLine("Multi-process mode", $$check_multi, auto)
printConfigLine("Installer enabled", $$yesNo(!CONFIG(disable-installer)), auto)
printConfigLine("Tests enabled", $$yesNo(CONFIG(enable-tests)), auto)
printConfigLine("Bundled libarchive", $$yesNo(CONFIG(bundled-libarchive)), auto)
printConfigLine("Bundled libyaml", $$yesNo(CONFIG(bundled-libyaml)), auto)
printConfigLine("libcrypto defines", $$check_libcrypto_defines, auto)
printConfigLine("libcrypto includes", $$check_libcrypto_includes, auto)
printConfigLine("SSDP support", $$yesNo(qtHaveModule(pssdp)), auto)
printConfigLine("Shellserver support", $$yesNo(qtHaveModule(pshellserver)), auto)
printConfigLine("Genivi support", $$yesNo(qtHaveModule(geniviextras)), auto)
printConfigLine("Systemd workaround", $$yesNo(CONFIG(systemd-sucks)), auto)

include(doc/doc.pri)

printConfigLine()

force-multiprocess:!qtHaveModule(compositor):error("You forced multi-process mode, but the QtCompositor module is not available")

SUBDIRS += src
enable-tests:SUBDIRS += tests

CONFIG += ordered

OTHER_FILES = \
    INSTALL.md \
    .qmake.conf \
    application-manager.conf \
    doc/application-manager.qdocconf \
    doc/index.qdoc \
    doc/elements.qdoc \
    doc/manifest.qdoc \

global-check-coverage.target = check-coverage
global-check-coverage.depends = coverage
global-check-coverage.commands = ( \
    find . -name \"*.gcov-info\" -print0 | xargs -0 rm -f && \
    cd tests && make check-coverage && cd .. && \
    cd src/common-lib && make check-coverage && cd ../.. && \
    cd src/crypto-lib && make check-coverage && cd ../.. && \
    cd src/installer-lib && make check-coverage && cd ../.. && \
    cd src/manager-lib && make check-coverage && cd ../.. && \
    lcov -o temp.gcov-info `find . -name "*.gcov-info" | xargs -n1 echo -a` && \
    lcov -o application-manager.gcov-info -r temp.gcov-info \"/usr/*\" \"$$[QT_INSTALL_PREFIX]/*\" \"$$[QT_INSTALL_PREFIX/src]/*\" \"tests/*\" \"moc_*\" && \
    rm -f temp.gcov-info && \
    genhtml -o coverage -s -f --legend --no-branch-coverage --demangle-cpp application-manager.gcov-info && echo \"\\n\\nCoverage info is available at file://`pwd`/coverage/index.html\" \
)
global-check-branch-coverage.target = check-branch-coverage
global-check-branch-coverage.depends = coverage
global-check-branch-coverage.commands = ( \
    find . -name \"*.gcov-info\" -print0 | xargs -0 rm -f && \
    cd tests && make check-branch-coverage && cd .. && \
    cd src/common-lib && make check-branch-coverage && cd ../.. && \
    cd src/crypto-lib && make check-branch-coverage && cd ../.. && \
    cd src/installer-lib && make check-branch-coverage && cd ../.. && \
    cd src/manager-lib && make check-branch-coverage && cd ../.. && \
    lcov --rc lcov_branch_coverage=1 -o temp.gcov-info `find . -name "*.gcov-info" | xargs -n1 echo -a` && \
    lcov --rc lcov_branch_coverage=1 -o application-manager.gcov-info -r temp.gcov-info \"/usr/*\" \"$$[QT_INSTALL_PREFIX]/*\" \"$$[QT_INSTALL_PREFIX/src]/*\" \"tests/*\" \"moc_*\" && \
    rm -f temp.gcov-info && \
    genhtml -o branch-coverage -s -f --legend --branch-coverage --rc lcov_branch_coverage=1 --demangle-cpp application-manager.gcov-info && echo \"\\n\\nBranch-Coverage info is available at file://`pwd`/branch-coverage/index.html\" \
)

QMAKE_EXTRA_TARGETS -= sub-check-coverage sub-check-branch-coverage
QMAKE_EXTRA_TARGETS *= global-check-coverage global-check-branch-coverage