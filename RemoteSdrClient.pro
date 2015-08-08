#-------------------------------------------------
#
# Project created by QtCreator 2012-12-11T09:45:38
#
#-------------------------------------------------
QT += core gui
QT += network
QT += multimedia
QT += widgets

TARGET = RemoteSdrClient
TEMPLATE = app


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
	gui/aboutdlg.cpp \
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
	interface/soundin.cpp \

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
	gui/aboutdlg.h \
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
	interface/soundin.h \

win32 {
FORMS  += \
	winforms/aboutdlg.ui \
	winforms/mainwindow.ui \
	winforms/ipeditframe.ui \
	winforms/editnetdlg.ui \
	winforms/sounddlg.ui \
	winforms/sdrdiscoverdlg.ui \
	winforms/sliderctrl.ui \
	winforms/transmitdlg.ui \
	winforms/memdialog.ui \
	winforms/chatdialog.ui \
	winforms/rawiqwidget.ui

}
macx {
FORMS  += \
	macforms/aboutdlg.ui \
	macforms/mainwindow.ui \
	macforms/ipeditframe.ui \
	macforms/editnetdlg.ui \
	macforms/sounddlg.ui \
	macforms/sdrdiscoverdlg.ui \
	macforms/sliderctrl.ui \
	macforms/memdialog.ui \
	macforms/transmitdlg.ui \
	macforms/chatdialog.ui \
	macforms/rawiqwidget.ui
}

unix:!macx {
FORMS += \
	unixforms/aboutdlg.ui \
	unixforms/mainwindow.ui \
	unixforms/ipeditframe.ui \
	unixforms/editnetdlg.ui \
	unixforms/sounddlg.ui \
	unixforms/sdrdiscoverdlg.ui \
	unixforms/sliderctrl.ui \
	unixforms/memdialog.ui \
	unixforms/transmitdlg.ui \
	unixforms/chatdialog.ui \
	unixforms/rawiqwidget.ui
}

OTHER_FILES += \
    resources/remoteclient.rc

win32 {
    RC_FILE = resources/remoteclient.rc
}

macx {
    ICON = RemoteSdrClient.icns
}


