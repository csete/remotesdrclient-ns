QT += core gui
QT += network
QT += multimedia
QT += widgets

TARGET = remotesdrclient-ns
TEMPLATE = app

RESOURCES += icons.qrc

# enable pkg-config to find dependencies
CONFIG += link_pkgconfig

# check if codec2 is available
packagesExist(codec2) {
    message("Codec2 support enabled.")
    PKGCONFIG += codec2
    DEFINES += ENABLE_CODEC2
    SOURCES += interface/freedv.cpp
    HEADERS += interface/freedv.h
} else {
    message("Codec2 support disabled.")
}

SOURCES += \
    gui/mainwindow.cpp \
    gui/main.cpp \
    gui/ipeditwidget.cpp \
    gui/editnetdlg.cpp \
    gui/sounddlg.cpp \
    gui/freqctrl.cpp \
    gui/sliderctrl.cpp \
    gui/meter.cpp \
    gui/plotter.cpp  \
    gui/rawiqwidget.cpp \
    gui/sdrdiscoverdlg.cpp \
    gui/transmitdlg.cpp \
    gui/memdialog.cpp \
    gui/chatdialog.cpp \
    dsp/G726.cpp \
    dsp/G711.cpp \
    dsp/fir.cpp \
    interface/soundout.cpp \
    interface/netio.cpp \
    interface/sdrinterface.cpp \
    interface/soundin.cpp

HEADERS  += \
    gui/mainwindow.h \
    gui/editnetdlg.h \
    gui/ipeditwidget.h \
    gui/sounddlg.h \
    gui/freqctrl.h \
    gui/sliderctrl.h \
    gui/meter.h \
    gui/plotter.h \
    gui/rawiqwidget.h \
    gui/sdrdiscoverdlg.h \
    gui/transmitdlg.h \
    gui/memdialog.h \
    gui/chatdialog.h \
    dsp/G711.h \
    dsp/G726.h \
    dsp/fir.h \
    dsp/datatypes.h \
    interface/soundout.h \
    interface/threadwrapper.h \
    interface/netio.h \
    interface/sdrinterface.h \
    interface/ascpmsg.h \
    interface/sdrprotocol.h \
    interface/soundin.h

FORMS += \
    nanoforms/mainwindow.ui \
    nanoforms/ipeditframe.ui \
    nanoforms/editnetdlg.ui \
    nanoforms/sounddlg.ui \
    nanoforms/sdrdiscoverdlg.ui \
    nanoforms/sliderctrl.ui \
    nanoforms/memdialog.ui \
    nanoforms/transmitdlg.ui \
    nanoforms/chatdialog.ui \
    nanoforms/rawiqwidget.ui

OTHER_FILES += \
    changelog.txt \
    resources/remoteclient.rc

win32 {
    RC_FILE = resources/remoteclient.rc
}

macx {
    ICON = RemoteSdrClient.icns
}


