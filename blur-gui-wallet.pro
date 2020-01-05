#qml components require at least QT 5.7.0
#lessThan (QT_MAJOR_VERSION, 5) | lessThan (QT_MINOR_VERSION, 7) {
#  error("Can't build with Qt $${QT_VERSION}. Use at least Qt 5.7.0")
#}

TEMPLATE = app

QT += qml quick widgets concurrent

WALLET_ROOT=$$PWD/monero

CONFIG += c++11 link_pkgconfig
!win32 {
    QMAKE_CXXFLAGS += -fPIC -fstack-protector -fstack-protector-strong
    QMAKE_LFLAGS += -fstack-protector -fstack-protector-strong
}

# cleaning "auto-generated" bitmonero directory on "make distclean"
QMAKE_DISTCLEAN += -r $$WALLET_ROOT

INCLUDEPATH +=  $$WALLET_ROOT/include \
                $$PWD/src/libwalletqt \
                $$PWD/src/QR-Code-generator \
                $$PWD/src \
                $$WALLET_ROOT/src

HEADERS += \
    filter.h \
    clipboardAdapter.h \
    oscursor.h \
    src/libwalletqt/WalletManager.h \
    src/libwalletqt/Wallet.h \
    src/libwalletqt/PendingTransaction.h \
    src/libwalletqt/TransactionHistory.h \
    src/libwalletqt/TransactionInfo.h \
    src/libwalletqt/QRCodeImageProvider.h \
    src/libwalletqt/Transfer.h \
    src/NetworkType.h \
    oshelper.h \
    TranslationManager.h \
    src/model/TransactionHistoryModel.h \
    src/model/TransactionHistorySortFilterModel.h \
    src/QR-Code-generator/BitBuffer.hpp \
    src/QR-Code-generator/QrCode.hpp \
    src/QR-Code-generator/QrSegment.hpp \
    src/model/AddressBookModel.h \
    src/libwalletqt/AddressBook.h \
    src/model/SubaddressModel.h \
    src/libwalletqt/Subaddress.h \
    src/zxcvbn-c/zxcvbn.h \
    src/libwalletqt/UnsignedTransaction.h \
    Logger.h \
    MainApp.h

SOURCES += main.cpp \
    filter.cpp \
    clipboardAdapter.cpp \
    oscursor.cpp \
    src/libwalletqt/WalletManager.cpp \
    src/libwalletqt/Wallet.cpp \
    src/libwalletqt/PendingTransaction.cpp \
    src/libwalletqt/TransactionHistory.cpp \
    src/libwalletqt/TransactionInfo.cpp \
    src/libwalletqt/QRCodeImageProvider.cpp \
    oshelper.cpp \
    TranslationManager.cpp \
    src/model/TransactionHistoryModel.cpp \
    src/model/TransactionHistorySortFilterModel.cpp \
    src/QR-Code-generator/BitBuffer.cpp \
    src/QR-Code-generator/QrCode.cpp \
    src/QR-Code-generator/QrSegment.cpp \
    src/model/AddressBookModel.cpp \
    src/libwalletqt/AddressBook.cpp \
    src/model/SubaddressModel.cpp \
    src/libwalletqt/Subaddress.cpp \
    src/zxcvbn-c/zxcvbn.c \
    src/libwalletqt/UnsignedTransaction.cpp \
    Logger.cpp \
    MainApp.cpp

CONFIG(DISABLE_PASS_STRENGTH_METER) {
    HEADERS -= src/zxcvbn-c/zxcvbn.h
    SOURCES -= src/zxcvbn-c/zxcvbn.c
    DEFINES += "DISABLE_PASS_STRENGTH_METER"
}

!ios {
    HEADERS += src/daemon/DaemonManager.h
    SOURCES += src/daemon/DaemonManager.cpp
}

lupdate_only {
SOURCES = *.qml \
          components/*.qml \
          pages/*.qml \
          pages/settings/*.qml \
          wizard/*.qml \
          wizard/*js
}


ios:armv7 {
    message("target is armv7")
    LIBS += \
        -L$$PWD/../ofxiOSBoost/build/libs/boost/lib/armv7 \
}
ios:arm64 {
    message("target is arm64")
    LIBS += \
        -L$$PWD/../ofxiOSBoost/build/libs/boost/lib/arm64 \
}
!ios:!android {
LIBS += -L$$WALLET_ROOT/lib \
        -lwallet_merged \
        -llmdb \
        -lepee \
        -lsodium \
        -leasylogging
}

android {
    message("Host is Android")
    LIBS += -L$$WALLET_ROOT/lib \
        -lwallet_merged \
        -llmdb \
        -lepee \
        -lsodium \
        -leasylogging
}



QMAKE_CXXFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=1 -Wformat -Wformat-security
QMAKE_CFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=1 -Wformat -Wformat-security

ios {
    message("Host is IOS")

    QMAKE_LFLAGS += -v
    QMAKE_IOS_DEVICE_ARCHS = arm64
    CONFIG += arm64
    LIBS += -L$$WALLET_ROOT/lib-ios \
        -lwallet_merged \
        -llmdb \
        -lepee \
        -lsodium \
        -leasylogging

    LIBS+= \
        -L$$PWD/../OpenSSL-for-iPhone/lib \
        -L$$PWD/../ofxiOSBoost/build/libs/boost/lib/arm64 \
        -lboost_serialization \
        -lboost_thread \
        -lboost_system \
        -lboost_date_time \
        -lboost_filesystem \
        -lboost_regex \
        -lboost_chrono \
        -lboost_program_options \
        -lssl \
        -lcrypto \
        -ldl
}

CONFIG(WITH_SCANNER) {
    if( greaterThan(QT_MINOR_VERSION, 5) ) {
        message("using camera scanner")
        QT += multimedia
        DEFINES += "WITH_SCANNER"
        INCLUDEPATH += $$PWD/src/QR-Code-scanner
        HEADERS += \
            src/QR-Code-scanner/QrScanThread.h \
            src/QR-Code-scanner/QrCodeScanner.h
        SOURCES += \
            src/QR-Code-scanner/QrScanThread.cpp \
            src/QR-Code-scanner/QrCodeScanner.cpp
        android {
            INCLUDEPATH += $$PWD/../ZBar/include
            LIBS += -lzbarjni -liconv
        } else {
            LIBS += -lzbar
        }
    } else {
        message("Skipping camera scanner because of Incompatible Qt Version !")
    }
}


# currently we only support x86 build as qt.io only provides prebuilt qt for x86 mingw

win32 {

    # QMAKE_HOST.arch is unreliable, will allways report 32bit if mingw32 shell is run.
    # Obtaining arch through uname should be reliable. This also fixes building the project in Qt creator without changes.
    MSYS_HOST_ARCH = $$system(uname -a | grep -o "x86_64")

    # WIN64 Host settings
    contains(MSYS_HOST_ARCH, x86_64) {
        message("Host is 64bit")
        MSYS_ROOT_PATH=c:/msys64

    # WIN32 Host settings
    } else {
        message("Host is 32bit")
        MSYS_ROOT_PATH=c:/msys32
    }

    # WIN64 Target settings
    contains(QMAKE_HOST.arch, x86_64) {
        MSYS_MINGW_PATH=/mingw64

    # WIN32 Target settings
    } else {
        MSYS_MINGW_PATH=/mingw32
    }
    
    MSYS_PATH=$$MSYS_ROOT_PATH$$MSYS_MINGW_PATH

    # boost root path
    BOOST_PATH=$$MSYS_PATH/boost
    BOOST_MINGW_PATH=$$MSYS_MINGW_PATH/boost

    LIBS+=-L$$MSYS_PATH/lib
    LIBS+=-L$$MSYS_MINGW_PATH/lib
    LIBS+=-L$$BOOST_PATH/lib
    LIBS+=-L$$BOOST_MINGW_PATH/lib
    
    LIBS+= \
        -Wl,-Bstatic \
        -lboost_serialization-mt \
        -lboost_thread-mt \
        -lboost_system-mt \
        -lboost_date_time-mt \
        -lboost_filesystem-mt \
        -lboost_regex-mt \
        -lboost_chrono-mt \
        -lboost_program_options-mt \
        -lboost_locale-mt \
        -licuio \
        -licuin \
        -licuuc \
        -licudt \
        -licutu \
        -liconv \
        -lssl \
        -lsodium \
        -lcrypto \
        -Wl,-Bdynamic \
        -lwinscard \
        -lws2_32 \
        -lwsock32 \
        -lIphlpapi \
        -lcrypt32 \
        -lgdi32
    
    !contains(QMAKE_TARGET.arch, x86_64) {
        message("Target is 32bit")
        ## Windows x86 (32bit) specific build here
        ## there's 2Mb stack in libwallet allocated internally, so we set stack=4Mb
        ## this fixes app crash for x86 Windows build
        QMAKE_LFLAGS += -Wl,--stack,4194304
    } else {
        message("Target is 64bit")
    }

    QMAKE_LFLAGS += -Wl,--dynamicbase -Wl,--nxcompat
}

linux {
    CONFIG(static) {
        message("using static libraries")
        LIBS+= -Wl,-Bstatic    
        QMAKE_LFLAGS += -static-libgcc -static-libstdc++
   #     contains(QT_ARCH, x86_64) {
            LIBS+=
   #     }
    } else {
      # On some distro's we need to add dynload
      LIBS+= -ldl
    }

    LIBS+= \
        -lboost_serialization \
        -lboost_thread \
        -lboost_system \
        -lboost_date_time \
        -lboost_filesystem \
        -lboost_regex \
        -lboost_chrono \
        -lboost_program_options \
        -lssl \
        -llmdb \
        -lsodium \
        -lcrypto

    if(!android) {
        LIBS+= \
            -Wl,-Bdynamic \
            -lGL
    }

    QMAKE_LFLAGS += -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
}

macx {
    # mixing static and shared libs are not supported on mac
    # CONFIG(static) {
    #     message("using static libraries")
    #     LIBS+= -Wl,-Bstatic
    # }
    LIBS+= \
        -L/usr/local/lib \
        -L/usr/local/opt/openssl/lib \
        -L/usr/local/opt/boost/lib \
        -lboost_serialization-mt \
        -lboost_thread-mt \
        -lboost_system-mt \
        -lboost_date_time-mt \
        -lboost_filesystem-mt \
        -lboost_regex \
        -lboost_chrono-mt \
        -lboost_program_options-mt \
        -lssl \
        -lsodium \
        -lcrypto \
        -ldl
    LIBS+=

    QMAKE_LFLAGS +=
}


# translation stuff
TRANSLATIONS = $$files($$PWD/translations/monero-core_*.ts)

CONFIG(release, debug|release) {
    DESTDIR = release/bin
    LANGUPD_OPTIONS = -locations relative -no-ui-lines
    LANGREL_OPTIONS = -compress -nounfinished -removeidentical

} else {
    DESTDIR = debug/bin
    LANGUPD_OPTIONS =
#    LANGREL_OPTIONS = -markuntranslated "MISS_TR "
}

TRANSLATION_TARGET_DIR = $$OUT_PWD/translations

!ios {
    isEmpty(QMAKE_LUPDATE) {
        win32:LANGUPD = $$[QT_INSTALL_BINS]\lupdate.exe
        else:LANGUPD = $$[QT_INSTALL_BINS]/lupdate
    }

    isEmpty(QMAKE_LRELEASE) {
        win32:LANGREL = $$[QT_INSTALL_BINS]\lrelease.exe
        else:LANGREL = $$[QT_INSTALL_BINS]/lrelease
    }

#    langupd.command = \
#        $$LANGUPD $$LANGUPD_OPTIONS $$shell_path($$_PRO_FILE) -ts $$_PRO_FILE_PWD/$$TRANSLATIONS



    langrel.depends = langupd
    langrel.input = TRANSLATIONS
    langrel.output = $$TRANSLATION_TARGET_DIR/${QMAKE_FILE_BASE}.qm
    langrel.commands = \
        $$LANGREL $$LANGREL_OPTIONS ${QMAKE_FILE_IN} -qm $$TRANSLATION_TARGET_DIR/${QMAKE_FILE_BASE}.qm
    langrel.CONFIG += no_link

    QMAKE_EXTRA_TARGETS += langupd deploy deploy_win
    QMAKE_EXTRA_COMPILERS += langrel

    # Compile an initial version of translation files when running qmake
    # the first time and generate the resource file for translations.
#    !exists($$TRANSLATION_TARGET_DIR) {
#        mkpath($$TRANSLATION_TARGET_DIR)
#    }
#    qrc_entry = "<RCC>"
#    qrc_entry += '  <qresource prefix="/">'
#    write_file($$TRANSLATION_TARGET_DIR/translations.qrc, qrc_entry)
#    for(tsfile, TRANSLATIONS) {
#        qmfile = $$TRANSLATION_TARGET_DIR/$$basename(tsfile)
#        qmfile ~= s/.ts$/.qm/
#        system($$LANGREL $$LANGREL_OPTIONS $$tsfile -qm $$qmfile)
#        qrc_entry = "    <file>$$basename(qmfile)</file>"
#        write_file($$TRANSLATION_TARGET_DIR/translations.qrc, qrc_entry, append)
#    }
#    qrc_entry = "  </qresource>"
#    qrc_entry += "</RCC>"
#    write_file($$TRANSLATION_TARGET_DIR/translations.qrc, qrc_entry, append)
#    RESOURCES += $$TRANSLATION_TARGET_DIR/translations.qrc
}


# Update: no issues with the "slow link process" anymore,
# for development, just build debug version of libwallet_merged lib
# by invoking 'get_libwallet_api.sh Debug'
# so we update translations everytime even for debug build

PRE_TARGETDEPS += langupd compiler_langrel_make_all

RESOURCES += qml.qrc
CONFIG += qtquickcompiler

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)
macx {
    deploy.commands += macdeployqt $$sprintf("%1/%2/%3.app", $$OUT_PWD, $$DESTDIR, $$TARGET) -qmldir=$$PWD
}

win32 {
    deploy.commands += windeployqt $$sprintf("%1/%2/%3.exe", $$OUT_PWD, $$DESTDIR, $$TARGET) -release -no-translations -qmldir=$$PWD
    # Win64 msys2 deploy settings
    contains(QMAKE_HOST.arch, x86_64) {
        deploy.commands += $$escape_expand(\n\t) $$PWD/windeploy_helper.sh $$DESTDIR
    }
}

linux:!android {
    deploy.commands += $$escape_expand(\n\t) $$PWD/linuxdeploy_helper.sh $$DESTDIR $$TARGET
}

android{
    deploy.commands += make install INSTALL_ROOT=$$DESTDIR && androiddeployqt --input android-libmonero-wallet-gui.so-deployment-settings.json --output $$DESTDIR --deployment bundled --android-platform android-21 --jdk /usr/lib/jvm/java-8-openjdk-amd64 -qmldir=$$PWD
}


OTHER_FILES += \
    .gitignore \
    $$TRANSLATIONS

DISTFILES += \
    notes.txt \
    monero/src/wallet/CMakeLists.txt \
    components/MobileHeader.qml


# windows application icon
RC_ICONS = images/appicon.ico

# mac Info.plist & application icon
QMAKE_INFO_PLIST = $$PWD/share/Info.plist
ICON = $$PWD/images/appicon.icns

