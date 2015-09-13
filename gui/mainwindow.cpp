/* ==========================================================================*/
/* - - - - - - - - - -  m a i n w i n d o w . c p p  - - - - - - - - - - - - */
/* ==========================================================================*/
/*	Created   2012-12-11   M. Wheatley                                       */
/*	Modified  2013-10-02   added tx stuff MW                                 */
/*	Modified  2013-10-13   fixed  bugs, extended frequency range, added      */
/*                         New Squelch audio packet                          */
/*	Modified  2013-10-15   fixed checkbox bug                                */
/*	Modified  2013-12-17   fixed netsork reconnect and stayontop issues      */
/*	Modified  2014-03-15   Added NCO spur null button                        */
/*	Modified  2014-03-15   Added compression mode byte                       */
/*                            to video and audio packets                     */
/*	Modified  2014-04-06   Added new icon                                    */
/*	Modified  2014-06-24   Added Memory Feature                              */
/*	Modified  2014-06-28   Fixed default WFM setting, Memory Dlg title bug,  */
/*							Added ifdef for Linux/windows difference in      */
/*                          StayOn Top logic (tnx Alex OZ9AEC)               */
/*	Modified  2014-07-07   added NULL test to  m_pSdrInterface pointer       */
/*	Modified  2014-07-09   Fixed broken memory dlg button on OSX form        */
/*                          Fixed issue with min/max BW changes              */
/*	Modified  2014-07-17   Changed span limit range and click res logic      */
/*	Modified  2014-07-18   Fixed span step bug                               */
/*							Added pixel smoothing function                   */
/*	Modified  2014-08-03  Fixed demod audio filter bug for SAM mode          */
/*	Modified  2014-08-28  Fixed Transmit frequency setting                   */
/*	Modified  2014-09-20  Added WAM mode,extended span range                 */
/*	Modified  2014-10-24  Added Autoscale and smoothing functions            */
/*	Modified  2015-01-01  Modified spectrum amplitude range limits           */
/*	Modified  2015-02-04  Added List server fields                           */
/*	Modified  2015-02-26  Added digital mode support                         */
/*	Modified  2015-03-12  Added Raw I/Q mode support                         */
/*.........................................................................  */
/* GUI entry point and main GUI event processing module                      */
/* ==========================================================================*/
//=============================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2013 Moe Wheatley. All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are
//permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//	  conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//	  of conditions and the following disclaimer in the documentation and/or other materials
//	  provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
//WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//The views and conclusions contained in the software and documentation are those of the
//authors and should not be interpreted as representing official policies, either expressed
//or implied, of Moe Wheatley.
//=============================================================================

/*---------------------------------------------------------------------------*/
/*------------------------> I N C L U D E S <--------------------------------*/
/*---------------------------------------------------------------------------*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editnetdlg.h"
#include "sounddlg.h"
#include "aboutdlg.h"
#include "transmitdlg.h"
#include "interface/sdrprotocol.h"
#include <QDebug>
#include <QList>
#include <QSpinBox>
#include <QSettings>

#define PROGRAM_TITLE_VERSION "RemoteSdrClient 1.11-ns4"


#define DOWNCONVERTER_TRANSITION_FREQ 56000000	//frequency where transition between direct and downconverter mode occurs

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);		//initialize all the GUI elements
    m_InhibitUpdate = true;	//use to keep controls from updating on initialization
	m_ProgramExeName = QFileInfo(QApplication::applicationFilePath()).fileName();
	m_ProgramExeName.remove(".exe", Qt::CaseInsensitive);
    setWindowTitle(PROGRAM_TITLE_VERSION);

	m_pMemDialog = new CMemDialog(this, Qt::WindowTitleHint );

	if(!g_pChatDialog)
		g_pChatDialog = new CChatDialog(this, Qt::WindowTitleHint );

	if(!g_pRawIQWidget)
		g_pRawIQWidget = new CRawIQWidget(this, Qt::WindowTitleHint );

	connect(m_pMemDialog, SIGNAL(GetCurrentSetting(bool)), this, SLOT(OnGetCurrentSetting(bool)));
	connect(m_pMemDialog, SIGNAL(SetCurrentSettingMemory()), this, SLOT(OnSetCurrentSetting()));

	InitDemodSettings();	//preload structures with fixed settings
	readSettings();			//Get persistent settings data

	QFile FileTest(m_MemoryFilePath);
	if( !FileTest.exists() )
		m_MemoryFilePath = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first()
								+ "/RemoteSdrClient/memory.ini";
	m_pMemDialog->Init(m_MemoryFilePath);

    m_TxActive = false;
	//create CSdrInterface and connect its signals
	m_pSdrInterface = new CSdrInterface(this);
	//give GUI plotter access to the sdr interface object pointer
	ui->framePlot->SetSdrInterface(m_pSdrInterface);
	g_pChatDialog->SetSdrInterface(m_pSdrInterface);

	connect(m_pSdrInterface, SIGNAL(NewSdrStatus(int)), this, SLOT(NewSdrStatus(int)));
	connect(m_pSdrInterface, SIGNAL(NewSMeterValue(qint16)), this, SLOT(NewSMeterValue(qint16)));
	connect(m_pSdrInterface, SIGNAL(NewVideoData()), this, SLOT(OnNewVideoData()));
	connect(m_pSdrInterface, SIGNAL(NewFftAvePwr(qint16)), this, SLOT(OnNewFftAvePwr(qint16)));


	//create status timer and connect its signal
	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	//connect GUI signals
	connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(OnExit()));
	connect(ui->actionNetwork, SIGNAL(triggered()), this, SLOT(OnNetworkDlg()));
	connect(ui->actionSoundCard, SIGNAL(triggered()), this, SLOT(OnSoundCardDlg()));
	connect(ui->actionTransmit, SIGNAL(triggered()), this, SLOT(OnTransmitDlg()));
	connect(ui->actionStayOnTop, SIGNAL(triggered()), this, SLOT(StayOnTop()));
	connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(OnAbout()));
	connect(ui->frameThresh, SIGNAL(sliderValChanged(int)), this, SLOT(OnAgcThresh(int)));
	connect(ui->frameDecay, SIGNAL(sliderValChanged(int)), this, SLOT(OnAgcDecay(int)));
	connect(ui->frameFreqCtrl, SIGNAL(NewFrequency(qint64)), this, SLOT(OnNewCenterFrequency(qint64)));

	connect(ui->framePlot, SIGNAL(NewWidth(int)), this, SLOT(OnNewWidth(int)));
	connect(ui->framePlot, SIGNAL(NewCenterFreq(qint64)), this, SLOT(OnNewPlotCenterFrequency(qint64)));
	connect(ui->framePlot, SIGNAL(NewLowCutFreq(int)),  this, SLOT(OnNewLowCutFreq(int)));
	connect(ui->framePlot, SIGNAL(NewHighCutFreq(int)),  this, SLOT(OnNewHighCutFreq(int)));

	//setup frequency control
	ui->frameFreqCtrl->Setup(10, 0U, 3000000000UL, 1, UNITS_MHZ );
	ui->frameFreqCtrl->SetBkColor(Qt::black);
	ui->frameFreqCtrl->SetDigitColor(Qt::cyan);
	ui->frameFreqCtrl->SetUnitsColor(Qt::lightGray);
	ui->frameFreqCtrl->SetHighlightColor(Qt::yellow);
	ui->frameFreqCtrl->SetFrequency(m_RxCenterFrequency);

	ui->framePlot->SetVideoMode(m_VideoCompressionIndex);

	ui->horizontalSliderVol->setValue(m_Volume);
	ui->spinBoxAtten->setValue(m_RfGain);
	if( m_RxCenterFrequency > DOWNCONVERTER_TRANSITION_FREQ )
		ui->spinBoxAtten->setSpecialValueText("Auto Gain");
	else
		ui->spinBoxAtten->setSpecialValueText("");

	//initiallize the combo boxes
	ui->comboBoxAudioCompression->clear();
	if(DEMOD_MODE_RAW != m_DemodMode )
	{
		ui->comboBoxAudioCompression->addItem("No Audio");
		ui->comboBoxAudioCompression->addItem("Raw 64K");
		ui->comboBoxAudioCompression->addItem("G711 64K");
		ui->comboBoxAudioCompression->addItem("G726 40K");
		ui->comboBoxAudioCompression->addItem("G726 32K");
		ui->comboBoxAudioCompression->addItem("G726 24K");
		ui->comboBoxAudioCompression->addItem("G726 16K");
		ui->comboBoxAudioCompression->setMaxCount(7);
		ui->comboBoxAudioCompression->setCurrentIndex(m_AudioCompressionIndex);
        ui->labelAudioRate->setText("Codec");
	}
	else
	{
		ui->comboBoxAudioCompression->addItem("16 ksps I/Q");
		ui->comboBoxAudioCompression->addItem("8 ksps I/Q");
		ui->comboBoxAudioCompression->addItem("4 ksps I/Q");
		ui->comboBoxAudioCompression->addItem("2 ksps I/Q");
		ui->comboBoxAudioCompression->addItem("1 ksps I/Q");
		ui->comboBoxAudioCompression->addItem("500 sps I/Q");
		ui->comboBoxAudioCompression->setMaxCount(6);
		ui->comboBoxAudioCompression->setCurrentIndex(m_RawRateIndex);
		ui->labelAudioRate->setText("Rate");
	}

	ui->comboBoxVideoCompression->addItem("No Video");
	ui->comboBoxVideoCompression->addItem("8 Bit Raw");
	ui->comboBoxVideoCompression->addItem("4 Bit Delta");
	ui->comboBoxVideoCompression->setCurrentIndex(m_VideoCompressionIndex);

	ui->comboBoxdBStep->addItem("10 dB", 10);
	ui->comboBoxdBStep->addItem("5 dB", 5);
	ui->comboBoxdBStep->addItem("3 dB", 3);
	ui->comboBoxdBStep->addItem("2 dB", 2);
	ui->comboBoxdBStep->addItem("1 dB", 1);
	ui->comboBoxdBStep->setCurrentIndex(m_dBStepIndex);
	m_dBStepSize = ui->comboBoxdBStep->itemData(m_dBStepIndex).toInt();

	//setup display range so view is close to same dB position
	m_dBMax = -m_dBMaxIndex*m_dBStepSize;
	if(10 == m_dBStepSize)
	{
		ui->verticalScrollBar_dBOffset->setMinimum(-5);
		ui->verticalScrollBar_dBOffset->setMaximum(7);
	}
	else if(5 == m_dBStepSize)
	{
		ui->verticalScrollBar_dBOffset->setMinimum(0);
		ui->verticalScrollBar_dBOffset->setMaximum(27);
	}
	else if(3 == m_dBStepSize)
	{
		ui->verticalScrollBar_dBOffset->setMinimum(0);
		ui->verticalScrollBar_dBOffset->setMaximum(47);
	}
	else if(2 == m_dBStepSize)
	{
		ui->verticalScrollBar_dBOffset->setMinimum(0);
		ui->verticalScrollBar_dBOffset->setMaximum(70);
	}
	else if(1 == m_dBStepSize)
	{
		ui->verticalScrollBar_dBOffset->setMinimum(0);
		ui->verticalScrollBar_dBOffset->setMaximum(140);
	}
	ui->verticalScrollBar_dBOffset->setValue(m_dBMaxIndex);

	ui->spinBoxAve->setValue(m_FftAve);
	ui->spinBoxRate->setValue(m_FftRate);

	ui->spinBoxSpan->setRange(1000,10000000);
	ui->spinBoxSpan->setValue(m_RxSpanFreq);

	m_SpanFreq = m_RxSpanFreq;
	ui->framePlot->SetPercent2DScreen(40);
	ui->framePlot->SetSpanFreq( m_SpanFreq );
	ui->framePlot->SetClickResolution(m_DemodSettings[m_DemodMode].ClickResolution);
	ui->framePlot->SetdBStepSize(ui->comboBoxdBStep->itemData(m_dBStepIndex).toInt());
	ui->framePlot->SetMaxdB(m_dBMax);
	ui->framePlot->SetCenterFreq( m_RxCenterFrequency );
    ui->framePlot->SetRunningState(false);
	ui->framePlot->SetHiLowCutFrequencies(m_DemodSettings[m_DemodMode].LowCut, m_DemodSettings[m_DemodMode].HiCut);
	ui->framePlot->SetDemodRanges(m_DemodSettings[m_DemodMode].LowCutMin,m_DemodSettings[m_DemodMode].LowCutMax,
								  m_DemodSettings[m_DemodMode].HiCutMin, m_DemodSettings[m_DemodMode].HiCutMax,
								  m_DemodSettings[m_DemodMode].Symetric);
	ui->framePlot->SetSquelchThreshold(m_DemodSettings[m_DemodMode].SquelchValue);
	ui->framePlot->UpdateOverlay();

	ui->doubleSpinBoxAlpha->setValue( 10.0*(1.0-m_SmoothAlpha) );

	ui->frameMeter->SetSquelchPos( m_DemodSettings[m_DemodMode].SquelchValue );
    ui->frameMeter->SetdBmLevel( -120.0, false );
	m_pSdrInterface->SetVolume(m_Volume);

    SetDemodSelector(m_DemodMode);

	ui->spinBoxOffset->setValue(m_DemodSettings[m_DemodMode].Offset);
	ui->horizontalSliderSquelch->setValue(m_DemodSettings[m_DemodMode].SquelchValue);
	ui->frameDecay->SetValue(m_DemodSettings[m_DemodMode].AgcDecay);
	ui->frameThresh->SetValue(m_DemodSettings[m_DemodMode].AgcThresh);

	ui->checkBoxAudioHP->setChecked( m_DemodSettings[m_DemodMode].AudioFilter);

	emit NewSdrStatus(SDR_OFF);	//make sure sdr is off
	m_pTimer->start(1000);		//start up status timer

	ui->actionStayOnTop->setChecked(m_StayOnTop);
	StayOnTop();
	ui->pushButtonPtt->setStyleSheet("background-color: rgb(180, 180, 180);");
	ui->pushButtonPtt->setEnabled(false);

	if(DEMOD_MODE_DIG == m_DemodMode)
		SetChatDialogState(true);
	if(DEMOD_MODE_RAW == m_DemodMode)
		SetRawIQWidgetState(true);

	m_InhibitUpdate = false;
}

MainWindow::~MainWindow()
{
	if(m_pSdrInterface)
		delete m_pSdrInterface;
	if(m_pMemDialog)
		delete m_pMemDialog;
	if(g_pChatDialog)
		delete g_pChatDialog;
	delete ui;

}

/////////////////////////////////////////////////////////////
// Called when program is closed to save persistant data
/////////////////////////////////////////////////////////////
void MainWindow::closeEvent(QCloseEvent *event)
{
	Q_UNUSED(event);
	if(	SDR_RECEIVING == m_pSdrInterface->m_SdrStatus)
	{
        OnButtonPtt(false);
        m_pSdrInterface->SetSdrRunState(false);	//stop SDR
		m_pSdrInterface->DisconnectFromServer(SDR_OFF);
	}
	m_pSdrInterface->StopAudioOut();
	m_pSdrInterface->StopAudioIn();
	//retrieve current settings from controls
	m_MemoryFilePath = m_pMemDialog->GetFilePath();
	writeSettings();
}

/////////////////////////////////////////////////////////////////////
// Called to set "Stay on Top" Main Window state
/////////////////////////////////////////////////////////////////////
void MainWindow::StayOnTop()
{
	m_StayOnTop = ui->actionStayOnTop->isChecked();
	Qt::WindowFlags flags = this->windowFlags() & ~(Qt::WindowStaysOnTopHint|Qt::WindowStaysOnBottomHint);
#ifdef Q_OS_WIN
	if (m_StayOnTop)
		this->setWindowFlags(flags | Qt::WindowStaysOnTopHint  | Qt::CustomizeWindowHint);
	else
		this->setWindowFlags( flags | Qt::WindowStaysOnBottomHint  | Qt::CustomizeWindowHint);
	this->show();
	this->raise();
#else
	if (m_StayOnTop)
		this->setWindowFlags(flags | Qt::WindowStaysOnTopHint  | Qt::CustomizeWindowHint);
	else
		this->setWindowFlags( (flags | Qt::CustomizeWindowHint) & ~Qt::WindowStaysOnTopHint);
	this->show();
#endif
}

/////////////////////////////////////////////////////////////////////
// Menu Bar action item handler.
//Exit menu
/////////////////////////////////////////////////////////////////////
void MainWindow::OnExit()
{
	qApp->exit(0);
}

/////////////////////////////////////////////////////////////////////
// Program persistant data save method
/////////////////////////////////////////////////////////////////////
void MainWindow::writeSettings()
{
	QSettings settings( QSettings::UserScope,"MoeTronix", m_ProgramExeName);
	settings.beginGroup("MainWindow");
	settings.setValue("state", saveState());
	settings.setValue("geometry", saveGeometry());
	settings.setValue("minstate",isMinimized());
	settings.setValue("StayOnTop",m_StayOnTop);

	if( m_pMemDialog->isVisible() )
		m_MemDialogRect = m_pMemDialog->geometry();
	settings.setValue(tr("MemDialogRect"),m_MemDialogRect);
	if( g_pChatDialog->isVisible() )
		m_ChatDialogRect = g_pChatDialog->geometry();
	settings.setValue(tr("ChatDialogRect"),m_ChatDialogRect);
	if( g_pRawIQWidget->isVisible() )
		m_RawIQWidgetRect = g_pRawIQWidget->geometry();
	settings.setValue(tr("RawIQWidgetRect"),m_RawIQWidgetRect);

	settings.endGroup();

	settings.beginGroup("Common");
	settings.setValue("IPAdr",m_IPAdr.toIPv4Address());
	settings.setValue("Port",m_Port);
	settings.setValue("sndin",m_SoundInIndex);
	settings.setValue("sndout",m_SoundOutIndex);
	settings.setValue("RxPassword", m_RxPassword);
	settings.setValue("TxPassword", m_TxPassword);
	settings.setValue("DomainName", m_DomainName);
	settings.setValue("ClientDesc",m_ClientDesc);
	settings.setValue("ListServer", m_ListServer);
	settings.setValue("ListServerActionPath", m_ListServerActionPath);
	settings.setValue("MemoryFilePath", m_MemoryFilePath);
	settings.setValue("CenterFrequency",m_RxCenterFrequency);
	settings.setValue("TxCenterFrequency",m_TxCenterFrequency);

	settings.setValue("DemodMode",m_DemodMode);
	settings.setValue("Volume",m_Volume);
	settings.setValue("RfGain",m_RfGain);
	settings.setValue("AudioCompressionIndex",m_AudioCompressionIndex);
	settings.setValue("RawRateIndex",m_RawRateIndex);
	settings.setValue("VideoCompressionIndex",m_VideoCompressionIndex);
	settings.setValue("RxSpanFreq",m_RxSpanFreq);
	settings.setValue("TxSpanFreq",m_TxSpanFreq);
	settings.setValue("dBStepIndex",m_dBStepIndex);
	settings.setValue("dBMaxIndex",m_dBMaxIndex);
	settings.setValue("FftAve",m_FftAve);
	settings.setValue("FftRate",m_FftRate);
	settings.setValue("CtcssFreq",m_CtcssFreq);
	settings.setValue("TestToneFreq",m_TestToneFreq);
	settings.setValue("SmoothAlpha",m_SmoothAlpha);
	settings.endGroup();

	settings.beginGroup("Equalizer");
	settings.setValue("Gls",m_EqualizerParams.Gls);
	settings.setValue("Fls",m_EqualizerParams.Fls);
	settings.setValue("Gpk",m_EqualizerParams.Gpk);
	settings.setValue("Fpk",m_EqualizerParams.Fpk);
	settings.setValue("Ghs",m_EqualizerParams.Ghs);
	settings.setValue("Fhs",m_EqualizerParams.Fhs);

	settings.setValue("Gls1",m_EqualizerParams1.Gls);
	settings.setValue("Fls1",m_EqualizerParams1.Fls);
	settings.setValue("Gpk1",m_EqualizerParams1.Gpk);
	settings.setValue("Fpk1",m_EqualizerParams1.Fpk);
	settings.setValue("Ghs1",m_EqualizerParams1.Ghs);
	settings.setValue("Fhs1",m_EqualizerParams1.Fhs);

	settings.setValue("Gls2",m_EqualizerParams2.Gls);
	settings.setValue("Fls2",m_EqualizerParams2.Fls);
	settings.setValue("Gpk2",m_EqualizerParams2.Gpk);
	settings.setValue("Fpk2",m_EqualizerParams2.Fpk);
	settings.setValue("Ghs2",m_EqualizerParams2.Ghs);
	settings.setValue("Fhs2",m_EqualizerParams2.Fhs);

	settings.endGroup();

	settings.beginWriteArray("Demod");
	//save demod settings
	for (int i = 0; i < NUM_DEMODS; i++)
	{
		settings.setArrayIndex(i);
		settings.setValue("Offset", m_DemodSettings[i].Offset);
		settings.setValue("SquelchValue", m_DemodSettings[i].SquelchValue);
		settings.setValue("AgcThresh", m_DemodSettings[i].AgcThresh);
		settings.setValue("AgcDecay", m_DemodSettings[i].AgcDecay);
		settings.setValue("HiCut", m_DemodSettings[i].HiCut);
		settings.setValue("LowCut", m_DemodSettings[i].LowCut);
		settings.setValue("AudioFilter", m_DemodSettings[i].AudioFilter);
	}
	settings.endArray();
}

/////////////////////////////////////////////////////////////////////
// Program persistant data recall method
/////////////////////////////////////////////////////////////////////
void MainWindow::readSettings()
{
	QSettings settings(QSettings::UserScope,"MoeTronix", m_ProgramExeName);
	settings.beginGroup("MainWindow");
	QByteArray state = settings.value("state", QByteArray()).toByteArray();
	QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
	restoreState(state);
	restoreGeometry(geometry);
    bool ismin = settings.value("minstate", false).toBool();
    m_StayOnTop = settings.value("StayOnTop", false).toBool();

	m_MemDialogRect = settings.value(tr("MemDialogRect"), QRect(10,10,500,200)).toRect();
	if( (m_MemDialogRect.x()<=0) ||  (m_MemDialogRect.y()<=0) )
		 m_MemDialogRect = QRect(10,10,500,200);
	m_ChatDialogRect = settings.value(tr("ChatDialogRect"), QRect(10,10,500,200)).toRect();
	if( (m_ChatDialogRect.x()<=0) ||  (m_ChatDialogRect.y()<=0) )
		 m_ChatDialogRect = QRect(10,10,500,200);
	m_RawIQWidgetRect = settings.value(tr("RawIQWidgetRect"), QRect(10,10,500,200)).toRect();
	if( (m_RawIQWidgetRect.x()<=0) ||  (m_RawIQWidgetRect.y()<=0) )
		 m_RawIQWidgetRect = QRect(10,10,500,200);

	settings.endGroup();

	settings.beginGroup("Common");
	m_IPAdr.setAddress(settings.value("IPAdr", 0xC0A80166).toInt() );
	m_Port = settings.value("Port", 50000).toUInt();
	m_SoundInIndex = settings.value("sndin", 0).toInt();
	m_SoundOutIndex = settings.value("sndout", 0).toInt();
	m_RxPassword = settings.value("RxPassword","").toString();
	m_TxPassword = settings.value("TxPassword","password").toString();
	m_DomainName = settings.value("DomainName","").toString();
	m_ClientDesc = settings.value("ClientDesc","").toString();
	m_ListServer = settings.value("ListServer","moetronix.com").toString();
	m_ListServerActionPath = settings.value("ListServerActionPath","/cgi-bin/getentries.py").toString();
	m_MemoryFilePath =  settings.value("MemoryFilePath","").toString();
	m_TxCenterFrequency = settings.value("TxCenterFrequency", 10000000).toLongLong();
	m_TxSpanFreq = settings.value("TxSpanFreq",15000).toInt();
	m_CtcssFreq = settings.value("CtcssFreq",0).toInt();
	m_TestToneFreq = settings.value("TestToneFreq",1500).toInt();
	m_SmoothAlpha =  settings.value("SmoothAlpha",1.0).toDouble();

	m_dBStepIndex = settings.value("dBStepIndex",1).toInt();
	m_dBMaxIndex = settings.value("dBMaxIndex",10).toInt();
	m_FftAve = settings.value("FftAve",1).toInt();
	m_FftRate = settings.value("FftRate",10).toInt();
	m_DemodMode = settings.value("DemodMode", DEMOD_MODE_AM).toInt();
	m_Volume = settings.value("Volume",100).toInt();
	m_RfGain = settings.value("RfGain",0).toInt();
	m_AudioCompressionIndex = settings.value("AudioCompressionIndex",4).toInt();
	m_RawRateIndex = settings.value("RawRateIndex",5).toInt();
	m_VideoCompressionIndex = settings.value("VideoCompressionIndex",2).toInt();
	m_RxSpanFreq = settings.value("RxSpanFreq",50000).toInt();
	m_RxCenterFrequency = settings.value("CenterFrequency", 10000000).toLongLong();
	settings.endGroup();

	settings.beginGroup("Equalizer");
	m_EqualizerParams.Gls = settings.value("Gls",0).toInt();
	m_EqualizerParams.Fls = settings.value("Fls",300).toInt();
	m_EqualizerParams.Gpk = settings.value("Gpk",0).toInt();
	m_EqualizerParams.Fpk = settings.value("Fpk",1000).toInt();
	m_EqualizerParams.Ghs = settings.value("Ghs",0).toInt();
	m_EqualizerParams.Fhs = settings.value("Fhs",2500).toInt();

	m_EqualizerParams1.Gls = settings.value("Gls1",0).toInt();
	m_EqualizerParams1.Fls = settings.value("Fls1",300).toInt();
	m_EqualizerParams1.Gpk = settings.value("Gpk1",0).toInt();
	m_EqualizerParams1.Fpk = settings.value("Fpk1",1000).toInt();
	m_EqualizerParams1.Ghs = settings.value("Ghs1",0).toInt();
	m_EqualizerParams1.Fhs = settings.value("Fhs1",2500).toInt();;

	m_EqualizerParams2.Gls = settings.value("Gls2",0).toInt();
	m_EqualizerParams2.Fls = settings.value("Fls2",300).toInt();
	m_EqualizerParams2.Gpk = settings.value("Gpk2",0).toInt();
	m_EqualizerParams2.Fpk = settings.value("Fpk2",1000).toInt();
	m_EqualizerParams2.Ghs = settings.value("Ghs2",0).toInt();
	m_EqualizerParams2.Fhs = settings.value("Fhs2",2500).toInt();;
	settings.endGroup();

	settings.beginReadArray("Demod");
	//get demod settings
	for (int i = 0; i < NUM_DEMODS; i++)
	{
		settings.setArrayIndex(i);
		m_DemodSettings[i].Offset = settings.value("Offset", m_DemodSettings[i].DefOffset).toInt();
		m_DemodSettings[i].SquelchValue = settings.value("SquelchValue", -160).toInt();
		m_DemodSettings[i].AgcThresh = settings.value("AgcThresh", m_DemodSettings[i].DefAgcThresh).toInt();
		m_DemodSettings[i].AgcDecay = settings.value("AgcDecay", m_DemodSettings[i].DefAgcDecay).toInt();
		m_DemodSettings[i].HiCut = settings.value("HiCut", m_DemodSettings[i].DefHiCut).toInt();
		m_DemodSettings[i].LowCut = settings.value("LowCut", m_DemodSettings[i].DefLowCut).toInt();
		m_DemodSettings[i].AudioFilter = settings.value("AudioFilter", m_DemodSettings[i].DefAudioFilter).toBool();
	}
	settings.endArray();
	if(ismin)
		showMinimized();
}

/////////////////////////////////////////////////////////////////////
// About Dialog Menu Bar action item handler.
/////////////////////////////////////////////////////////////////////
void MainWindow::OnAbout()
{
CAboutDlg dlg(this,PROGRAM_TITLE_VERSION);
	dlg.exec();
}

/////////////////////////////////////////////////////////////////////
// Status Timer event handler
//SDR_OFF,
//SDR_CONNECTING,
//SDR_CONNECTED,
//SDR_RECEIVING,
//SDR_TRANSMITTING,
//SDR_DISCONNECT_BUSY,
//SDR_DISCONNECT_PWERROR,
//SDR_DISCONNECT_TIMEOUT
/////////////////////////////////////////////////////////////////////
void MainWindow::OnTimer()
{
int count;
	switch(m_pSdrInterface->m_SdrStatus)
	{
		case SDR_RECEIVING:
		case SDR_TRANSMITTING:
			count = m_pSdrInterface->GetDataLatency();
			m_Str = "Connected to " + m_pSdrInterface->m_ServerName +
					" SN=" + m_pSdrInterface->m_SerialNumStr +
					" (" + QString::number(count) + "mSec)";
			m_pSdrInterface->SendKeepalive();
			break;
		case SDR_CONNECTING:
		case SDR_CONNECTED:
		case SDR_PWOK:
			m_Str = "Trying to Connect";
			m_pSdrInterface->SendKeepalive();
			break;
		case SDR_OFF:
			m_Str = "Not Connected";
			break;
		case SDR_DISCONNECT_BUSY:
			m_Str = "Server Was Busy";
			break;
		case SDR_DISCONNECT_PWERROR:
			m_Str = "Incorrect Rx Password";
			break;
		case SDR_DISCONNECT_TIMEOUT:
			m_Str = "Server Not Found";
			break;
		default:
			break;
	}
	statusBar()->showMessage(m_Str,0);
}

/////////////////////////////////////////////////////////////////////
//called when SDR/network status changes
/////////////////////////////////////////////////////////////////////
void MainWindow::NewSdrStatus(int stat)
{
QPalette pal(ui->pushButtonStart->palette());
eSdrStatus status = (eSdrStatus)stat;

	switch(status)
	{
		case SDR_OFF:
		case SDR_DISCONNECT_BUSY:
		case SDR_DISCONNECT_PWERROR:
		case SDR_DISCONNECT_TIMEOUT:
			//stop soundcards
			m_pSdrInterface->StopAudioOut();
			m_pSdrInterface->StopAudioIn();
			ui->pushButtonStart->setText("Start");
			pal.setColor( QPalette::Active, QPalette::ButtonText, Qt::black );
            ui->framePlot->SetRunningState(false);
			ui->pushButtonPtt->setStyleSheet("background-color: rgb(180, 180, 180);");
            ui->pushButtonPtt->setEnabled(false);
			if( ui->pushButtonPtt->isChecked() )
			   ui->pushButtonPtt->setChecked(false);
qDebug()<<"Sdr Not Connected";
			break;
		case SDR_CONNECTING:
			ui->pushButtonStart->setText("Connecting");
			pal.setColor( QPalette::Active, QPalette::ButtonText, Qt::lightGray);
qDebug()<<"Searching for Sdr";
			break;
		case SDR_CONNECTED:
			//first thing after connect is to try password
			m_pSdrInterface->TryPW(m_RxPassword, m_TxPassword);
qDebug()<<"Sdr Idle";
			break;
		case SDR_PWOK:
			m_pSdrInterface->GetInfo();
			m_pSdrInterface->SetAtten(m_RfGain);
			m_pSdrInterface->SetDemodMode(m_DemodMode);
			if(DEMOD_MODE_RAW == m_DemodMode)
				m_pSdrInterface->SetAudioCompressionMode(m_RawRateIndex + COMP_MODE_RAW_16000);	//offset since shares same msg as audio comp
			else
				m_pSdrInterface->SetAudioCompressionMode(m_AudioCompressionIndex);
			m_pSdrInterface->SetVideoCompressionMode(m_VideoCompressionIndex);
			m_pSdrInterface->SetAgc(0, m_DemodSettings[m_DemodMode].AgcThresh, m_DemodSettings[m_DemodMode].AgcDecay);
			m_pSdrInterface->SetSquelchThreshold(m_DemodSettings[m_DemodMode].SquelchValue);
			ui->framePlot->SetSquelchThreshold(m_DemodSettings[m_DemodMode].SquelchValue);
			ui->frameMeter->SetSquelchPos( m_DemodSettings[m_DemodMode].SquelchValue );
			m_pSdrInterface->SetDemodFilter(m_DemodSettings[m_DemodMode].LowCut,
											m_DemodSettings[m_DemodMode].HiCut,
											m_DemodSettings[m_DemodMode].Offset);

			if(m_DemodSettings[m_DemodMode].AudioFilter)
				m_pSdrInterface->SetAudioFilter(RX_AUDIOFILTER_CTCSS);
			else
				m_pSdrInterface->SetAudioFilter(RX_AUDIOFILTER_NONE);
			m_pSdrInterface->SendClientDesc(m_ClientDesc);
			m_pSdrInterface->SetSdrRunState(true);	//start SDR
			m_pSdrInterface->SetEqualizer(m_EqualizerParams);
			m_pSdrInterface->StartAudioOut(m_SoundOutIndex);	//start soundcard
qDebug()<<"Sdr PW ok";
			break;
		case SDR_RECEIVING:	//here at start of receiving or end of transmitting
qDebug()<<"Sdr Receiving";
            m_TxActive = false;
            m_InhibitUpdate = true;	//use to keep controls from updating on initialization
			m_pSdrInterface->SetTxTestSignalMode(TESTSIGNAL_MODE_OFF, 0);
			m_pSdrInterface->SetRxFrequency(m_RxCenterFrequency);
			m_pSdrInterface->SetTxFrequency(m_TxCenterFrequency);
			//setup spectrum display plot
//qDebug()<<m_pSdrInterface->m_pRxFrequencyRangeMin[0] << m_pSdrInterface->m_pRxFrequencyRangeMax[0];
			ui->frameFreqCtrl->Setup(10, m_pSdrInterface->m_pRxFrequencyRangeMin[0], m_pSdrInterface->m_pRxFrequencyRangeMax[0], 1, UNITS_MHZ );
			ui->frameFreqCtrl->SetFrequency(m_RxCenterFrequency);

			ui->spinBoxSpan->setRange(m_pSdrInterface->m_RxSpanMin,m_pSdrInterface->m_RxSpanMax);
			if(m_RxSpanFreq<m_pSdrInterface->m_RxSpanMin)
				m_RxSpanFreq = m_pSdrInterface->m_RxSpanMin;
			if(m_RxSpanFreq>m_pSdrInterface->m_RxSpanMax)
				m_RxSpanFreq = m_pSdrInterface->m_RxSpanMax;
			m_SpanFreq = m_RxSpanFreq;
			ui->spinBoxSpan->setValue(m_RxSpanFreq);
			if(m_RxSpanFreq < 10000)
				ui->spinBoxSpan->setSingleStep(1000);
			else if(m_RxSpanFreq < 100000)
				ui->spinBoxSpan->setSingleStep(10000);
			else if(m_RxSpanFreq < 1000000)
				ui->spinBoxSpan->setSingleStep(100000);
			else if(m_RxSpanFreq <= 10000000)
				ui->spinBoxSpan->setSingleStep(1000000);

			ui->pushButtonPtt->setStyleSheet("background-color: rgb(0, 255, 0)");
			ui->pushButtonStart->setText("Stop");
			pal.setColor( QPalette::Active, QPalette::ButtonText, Qt::darkGreen );
			m_pSdrInterface->SetCtcssFreq(m_CtcssFreq);
			m_pSdrInterface->SetupFft(m_PlotWidth, m_SpanFreq, m_dBMax,
						  m_dBMax - (m_dBStepSize*VERT_DIVS), m_FftAve, m_FftRate );

			ui->framePlot->SetSpanFreq( m_SpanFreq );
			ui->framePlot->UpdateOverlay();


            ui->framePlot->SetRunningState(true);
			m_pSdrInterface->SendKeepalive();
			if( (0 != m_AudioCompressionIndex) && (	DEMOD_MODE_RAW != m_DemodMode ) )
                ui->pushButtonPtt->setEnabled(true);
			if( ui->pushButtonPtt->isChecked() )
                ui->pushButtonPtt->setChecked(false);
			break;
		case SDR_TRANSMITTING:
            m_InhibitUpdate = true;	//use to keep controls from updating on initialization
			ui->pushButtonPtt->setStyleSheet("background-color: rgb(255, 0, 0)");
			m_pSdrInterface->StartAudioIn(m_SoundInIndex);
            m_TxActive = true;

			if(m_TxSpanFreq<m_pSdrInterface->m_TxSpanMin)
				m_TxSpanFreq = m_pSdrInterface->m_TxSpanMin;
			if(m_TxSpanFreq>m_pSdrInterface->m_TxSpanMax)
				m_TxSpanFreq = m_pSdrInterface->m_TxSpanMax;
			m_SpanFreq = m_TxSpanFreq;
			m_pSdrInterface->SetupFft(m_PlotWidth, m_SpanFreq, m_dBMax,
						  m_dBMax - (m_dBStepSize*VERT_DIVS), m_FftAve, m_FftRate );
			ui->spinBoxSpan->setRange(m_pSdrInterface->m_TxSpanMin, m_pSdrInterface->m_TxSpanMax);
			ui->spinBoxSpan->setValue(m_TxSpanFreq);
			ui->spinBoxSpan->setSingleStep(1000);
			ui->framePlot->SetSpanFreq( m_TxSpanFreq );
			ui->framePlot->UpdateOverlay();
qDebug()<<"Sdr Transmitting";
			break;
		default:
			break;
	}
	ui->pushButtonStart->setPalette(pal);
    ui->pushButtonStart->setAutoFillBackground(true);
    m_InhibitUpdate = false;	//use to keep controls from updating on initialization
}

/////////////////////////////////////////////////////////////////////
// Menu Bar action item handler.
//Network Setup Menu
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNetworkDlg()
{
CEditNetDlg dlg(this);
	dlg.m_IPAdr = m_IPAdr;
	dlg.m_Port = m_Port;
	dlg.m_DomainName = m_DomainName;
	dlg.SetPasswords(m_RxPassword, m_TxPassword);
	dlg.SetListServer(m_ListServer);
	dlg.SetListServerActionPath(m_ListServerActionPath);
	dlg.SetClientDesc(m_ClientDesc);
	dlg.InitDlg();
	if( dlg.exec() )
	{
		m_IPAdr = dlg.m_IPAdr;
		m_Port = dlg.m_Port;
		m_DomainName = dlg.m_DomainName;
		dlg.GetPasswords(m_RxPassword, m_TxPassword);
		dlg.GetListServer(m_ListServer);
		dlg.GetListServerActionPath(m_ListServerActionPath);
		dlg.GetClientDesc(m_ClientDesc);
	}
}

/////////////////////////////////////////////////////////////////////
// Menu Bar action item handler.
//Sound Card Setup Menu
/////////////////////////////////////////////////////////////////////
void MainWindow::OnSoundCardDlg()
{
CSoundDlg dlg(this);
	dlg.SetInputIndex(m_SoundInIndex);
	dlg.SetOutputIndex(m_SoundOutIndex);
	if(QDialog::Accepted == dlg.exec() )
	{
		m_SoundInIndex = dlg.GetInputIndex();
		m_SoundOutIndex = dlg.GetOutputIndex();
	}
}

/////////////////////////////////////////////////////////////////////
// Menu Bar action item handler.
//Transmit Setup Menu
/////////////////////////////////////////////////////////////////////
void MainWindow::OnTransmitDlg()
{
CTransmitDlg dlg(this,m_pSdrInterface );
	dlg.m_EqualizerParams = m_EqualizerParams;
	dlg.m_EqualizerParams1 = m_EqualizerParams1;
	dlg.m_EqualizerParams2 = m_EqualizerParams2;
	dlg.m_TestToneFreq = m_TestToneFreq;
	dlg.m_CtcssFreq = m_CtcssFreq;
	dlg.InitDlg();
	if(QDialog::Accepted == dlg.exec() )
	{
		m_EqualizerParams = dlg.m_EqualizerParams;
		m_EqualizerParams1 = dlg.m_EqualizerParams1;
		m_EqualizerParams2 = dlg.m_EqualizerParams2;
		m_TestToneFreq = dlg.m_TestToneFreq;
		m_CtcssFreq = dlg.m_CtcssFreq;
	}
//	m_pSdrInterface->SetTxTestSignalMode(TESTSIGNAL_MODE_OFF, 0);
}

/////////////////////////////////////////////////////////////////////
// Called when new video data is available to update the plot
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewVideoData()
{
	ui->framePlot->Draw(true);		//slot called when new data avail
}

/////////////////////////////////////////////////////////////////////
// Called by Start/Stop Button event
/////////////////////////////////////////////////////////////////////
void MainWindow::OnStart()
{
QPalette pal(ui->pushButtonStart->palette());
	switch(m_pSdrInterface->m_SdrStatus)
	{
		case SDR_OFF:
		case SDR_DISCONNECT_BUSY:
		case SDR_DISCONNECT_PWERROR:
		case SDR_DISCONNECT_TIMEOUT:
			statusBar()->showMessage("Trying to Connect",0);
			ui->pushButtonStart->setText("Connecting");
			pal.setColor( QPalette::Active, QPalette::ButtonText, Qt::lightGray);
			ui->pushButtonStart->setPalette(pal);
            ui->pushButtonStart->setAutoFillBackground(true);
			m_pSdrInterface->SetServerParameters(m_DomainName, m_IPAdr, m_Port);
			m_pSdrInterface->ConnectToServer();
			break;
		case SDR_RECEIVING:
		case SDR_TRANSMITTING:
			OnButtonPtt(false);
			m_pSdrInterface->SetSdrRunState(false);	//stop SDR
			m_pSdrInterface->DisconnectFromServer(SDR_OFF);
			break;
		default:
			break;
	}
}

/////////////////////////////////////////////////////////////
// Slot called when PTT button is pressed.
/////////////////////////////////////////////////////////////
void MainWindow::OnButtonPtt(bool state)
{
//	if(m_InhibitUpdate)
//		return;
	if( (SDR_RECEIVING != m_pSdrInterface->m_SdrStatus) &&
			(SDR_TRANSMITTING != m_pSdrInterface->m_SdrStatus) )
		return;
	if(state)
	{	//try to start Transmit mode
		if( ui->checkBoxTxTrack->isChecked() )
		{	 //if TX tracking
			m_TxCenterFrequency = m_RxCenterFrequency;
		}
		m_pSdrInterface->SetPTT(CI_TX_STATE_ON );
		ui->frameFreqCtrl->SetDigitColor(Qt::red);
		ui->frameFreqCtrl->SetUnitsColor(Qt::darkRed);
		ui->frameFreqCtrl->Setup(10, m_pSdrInterface->m_pTxFrequencyRangeMin[0], m_pSdrInterface->m_pTxFrequencyRangeMax[0], 1, UNITS_MHZ );
		ui->framePlot->SetCenterFreq( m_TxCenterFrequency );
		ui->frameFreqCtrl->SetFrequency(m_TxCenterFrequency);
		m_pSdrInterface->SetTxFrequency(m_TxCenterFrequency);
	}
	else
	{
		m_pSdrInterface->SetPTT(CI_TX_STATE_OFF);
		ui->pushButtonPtt->setStyleSheet("background-color: rgb(0, 255, 0)");
		m_pSdrInterface->StopAudioIn();
		ui->frameFreqCtrl->SetDigitColor(Qt::cyan);
		ui->frameFreqCtrl->SetUnitsColor(Qt::lightGray);
		ui->framePlot->SetCenterFreq( m_RxCenterFrequency );
		ui->frameFreqCtrl->Setup(10, m_pSdrInterface->m_pRxFrequencyRangeMin[0], m_pSdrInterface->m_pRxFrequencyRangeMax[0], 1, UNITS_MHZ );
		ui->frameFreqCtrl->SetFrequency(m_RxCenterFrequency);
		m_pSdrInterface->SetTxFrequency(m_RxCenterFrequency);
	}
}

/////////////////////////////////////////////////////////////
// Called when PTT button is pressed from digital chat dialog.
/////////////////////////////////////////////////////////////
void MainWindow::OnDigitalPtt(bool state)
{
	if( (SDR_RECEIVING != m_pSdrInterface->m_SdrStatus) &&
			(SDR_TRANSMITTING != m_pSdrInterface->m_SdrStatus) )
		return;
	if(state)
	{	//try to start Transmit mode
		if( ui->checkBoxTxTrack->isChecked() )
		{	 //if TX tracking
			m_TxCenterFrequency = m_RxCenterFrequency;
		}
		m_pSdrInterface->SetPTT(CI_TX_STATE_ON );
		ui->frameFreqCtrl->SetDigitColor(Qt::red);
		ui->frameFreqCtrl->SetUnitsColor(Qt::darkRed);
		ui->frameFreqCtrl->Setup(10, m_pSdrInterface->m_pTxFrequencyRangeMin[0], m_pSdrInterface->m_pTxFrequencyRangeMax[0], 1, UNITS_MHZ );
		ui->framePlot->SetCenterFreq( m_TxCenterFrequency );
		ui->frameFreqCtrl->SetFrequency(m_TxCenterFrequency);
		m_pSdrInterface->SetTxFrequency(m_TxCenterFrequency);
	}
	else
	{
		if(SDR_TRANSMITTING == m_pSdrInterface->m_SdrStatus)
			m_pSdrInterface->SetPTT(CI_TX_STATE_DELAYOFF);
		else
			m_pSdrInterface->SetPTT(CI_TX_STATE_OFF );
		ui->pushButtonPtt->setStyleSheet("background-color: rgb(0, 255, 0)");
		m_pSdrInterface->StopAudioIn();
		ui->frameFreqCtrl->SetDigitColor(Qt::cyan);
		ui->frameFreqCtrl->SetUnitsColor(Qt::lightGray);
		ui->framePlot->SetCenterFreq( m_RxCenterFrequency );
		ui->frameFreqCtrl->Setup(10, m_pSdrInterface->m_pRxFrequencyRangeMin[0], m_pSdrInterface->m_pRxFrequencyRangeMax[0], 1, UNITS_MHZ );
		ui->frameFreqCtrl->SetFrequency(m_RxCenterFrequency);
		m_pSdrInterface->SetTxFrequency(m_RxCenterFrequency);
	}
}


/////////////////////////////////////////////////////////////
// Slot called when keyboard key is pressed.
/////////////////////////////////////////////////////////////
void MainWindow::keyPressEvent ( QKeyEvent * event )
{	//uses CTRL key as PTT

	return;		//disable for now

	if(	(SDR_RECEIVING != m_pSdrInterface->m_SdrStatus ) &&
		(SDR_TRANSMITTING != m_pSdrInterface->m_SdrStatus )  )
		return;
	if( event->key() == Qt::Key_Control )
	{
		if(ui->pushButtonPtt->isEnabled())
		{
			if( ui->pushButtonPtt->isChecked() )
                ui->pushButtonPtt->setChecked(false);
			else
                ui->pushButtonPtt->setChecked(true);
		}
	}
}

/////////////////////////////////////////////////////////////
// Slot called when keyboard key is released.
/////////////////////////////////////////////////////////////
void MainWindow::keyReleaseEvent ( QKeyEvent * event )
{
Q_UNUSED(event);
	return;		//disable for now
	if(	(SDR_RECEIVING != m_pSdrInterface->m_SdrStatus ) &&
		(SDR_TRANSMITTING != m_pSdrInterface->m_SdrStatus )  )
		return;
//	if( event->key() == Qt::Key_Control )
//		ui->pushButtonPtt->setChecked(false);
}


/////////////////////////////////////////////////////////////////////
// Handle change event for Tx Frequency Track checkbox control
/////////////////////////////////////////////////////////////////////
void MainWindow::OnTxTrackChanged(int state)
{
	if(state)
	{

	}
}

/////////////////////////////////////////////////////////////////////
// Handle change event for center frequency control
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewCenterFrequency(qint64 freq)
{
	if(m_InhibitUpdate)
		return;
	if(ui->pushButtonPtt->isChecked())
	{	//if PTT is active
		if( !ui->checkBoxTxTrack->isChecked() )
		{	 //if not TX tracking
			m_TxCenterFrequency = freq;
			m_pSdrInterface->SetTxFrequency(m_TxCenterFrequency);
		}
		else
		{	//TX tracking is ON
			m_TxCenterFrequency = freq;
			m_RxCenterFrequency = freq;
			m_pSdrInterface->SetRxFrequency(m_RxCenterFrequency);
			m_pSdrInterface->SetTxFrequency(m_TxCenterFrequency);
		}
		ui->framePlot->SetCenterFreq( m_TxCenterFrequency );
	}
	else
	{	//if PTT is not active
		m_RxCenterFrequency = freq;
		m_pSdrInterface->SetRxFrequency(m_RxCenterFrequency);
	}

	if( ui->pushButtonPtt->isChecked() )
		ui->framePlot->SetCenterFreq( m_TxCenterFrequency );
	else
		ui->framePlot->SetCenterFreq( m_RxCenterFrequency );
	if( m_RxCenterFrequency > DOWNCONVERTER_TRANSITION_FREQ )
		ui->spinBoxAtten->setSpecialValueText("Auto Gain");
	else
		ui->spinBoxAtten->setSpecialValueText("");
	ui->framePlot->UpdateOverlay();
}

/////////////////////////////////////////////////////////////////////
// Called when plot center frequency control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewPlotCenterFrequency(qint64 freq)
{
	if(m_InhibitUpdate)
		return;
	m_RxCenterFrequency = freq;
	ui->frameFreqCtrl->SetFrequency(m_RxCenterFrequency);
//	m_pSdrInterface->SetFrequency(freq);
//	ui->framePlot->SetCenterFreq( freq );
//	ui->framePlot->SetDemodCenterFreq( m_RxCenterFrequency );
//	ui->framePlot->UpdateOverlay();
}

/////////////////////////////////////////////////////////////////////
// Called when Low cut frequency changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewLowCutFreq(int f)
{
	m_DemodSettings[m_DemodMode].LowCut = f;
	m_pSdrInterface->SetDemodFilter(m_DemodSettings[m_DemodMode].LowCut,
									m_DemodSettings[m_DemodMode].HiCut,
									m_DemodSettings[m_DemodMode].Offset);
}

/////////////////////////////////////////////////////////////////////
// Called when high cut frequency changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewHighCutFreq(int f)
{
	m_DemodSettings[m_DemodMode].HiCut = f;
	m_pSdrInterface->SetDemodFilter(m_DemodSettings[m_DemodMode].LowCut,
									m_DemodSettings[m_DemodMode].HiCut,
									m_DemodSettings[m_DemodMode].Offset);
}


/////////////////////////////////////////////////////////////////////
// Called when volume control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnVolumeSlider(int value)
{
	if(m_InhibitUpdate)
		return;
	m_Volume = value;
	m_pSdrInterface->SetVolume(m_Volume);
}

/////////////////////////////////////////////////////////////////////
// Called when audio compression control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnAudioCompressionChanged(int indx)
{
	if(m_InhibitUpdate || (indx <0) )
		return;
	if(	m_DemodMode != DEMOD_MODE_RAW )
	{
		m_AudioCompressionIndex = indx;
		m_pSdrInterface->SetAudioCompressionMode(m_AudioCompressionIndex);
		if(0==m_AudioCompressionIndex)
		{
			if( ui->pushButtonPtt->isChecked() )
				ui->pushButtonPtt->setChecked(false);
			ui->pushButtonPtt->setEnabled(false);
		}
		else
		{
			if(	SDR_RECEIVING == m_pSdrInterface->m_SdrStatus )
				ui->pushButtonPtt->setEnabled(true);
		}
	}
	else
	{
		m_RawRateIndex = indx;
		m_pSdrInterface->SetAudioCompressionMode(m_RawRateIndex + COMP_MODE_RAW_16000);	//offset since shares same msg as audio comp
	}
}

/////////////////////////////////////////////////////////////////////
// Called when video compression control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnVideoCompressionChanged(int indx)
{
	if(m_InhibitUpdate )
		return;
	m_VideoCompressionIndex = indx;
	ui->framePlot->SetVideoMode(m_VideoCompressionIndex);
	m_pSdrInterface->SetVideoCompressionMode(m_VideoCompressionIndex);
}

/////////////////////////////////////////////////////////////////////
// Called when S-Meter has updated data changes
/////////////////////////////////////////////////////////////////////
void MainWindow::NewSMeterValue(qint16 Val)
{
double dB = (double)Val/10.0;
bool ovr = false;
//	qDebug()<<"Smeter"<<Val;
	if(dB > 10.0)
        ovr = true;
	if(m_TxActive)
	{
		ui->frameMeter->SetTxLevel( dB );
	}
	else
	{
		ui->frameMeter->SetdBmLevel( dB, ovr );
	}
}

/////////////////////////////////////////////////////////////////////
// Called when chat dialog needs to change state
/////////////////////////////////////////////////////////////////////
void MainWindow::SetChatDialogState(int state)
{
	if(state)
	{	//make chat dialog visable if not already
		if(!g_pChatDialog->isVisible())
		{
			g_pChatDialog->setGeometry(m_ChatDialogRect);
			g_pChatDialog->show();
		}
		g_pChatDialog->activateWindow();
		g_pChatDialog->raise();
	}
	else
	{	//hide chat dialog
		if(g_pChatDialog->isVisible())
		{
			m_ChatDialogRect = g_pChatDialog->geometry();
			g_pChatDialog->hide();
		}
	}
}

/////////////////////////////////////////////////////////////////////
// Called when RawIQ dialog needs to change state
/////////////////////////////////////////////////////////////////////
void MainWindow::SetRawIQWidgetState(int state)
{
	if(state)
	{	//make chat dialog visable if not already
		if(!g_pRawIQWidget->isVisible())
		{
			g_pRawIQWidget->setGeometry(m_RawIQWidgetRect);
			g_pRawIQWidget->show();
		}
		g_pRawIQWidget->activateWindow();
		g_pRawIQWidget->raise();
	}
	else
	{	//hide chat dialog
		if(g_pRawIQWidget->isVisible())
		{
			m_RawIQWidgetRect = g_pRawIQWidget->geometry();
			g_pRawIQWidget->hide();
		}
	}
}

void MainWindow::OnDemodChanged(int index)
{
    if(m_InhibitUpdate)
        return;

    int lastmode = m_DemodMode;

    switch (index)
    {
    default:
    case 0:
        m_DemodMode = DEMOD_MODE_AM;
        break;
    case 1:
        m_DemodMode = DEMOD_MODE_SAM;
        break;
    case 2:
        m_DemodMode = DEMOD_MODE_WAM;
        break;
    case 3:
        m_DemodMode = DEMOD_MODE_FM;
        break;
    case 4:
        m_DemodMode = DEMOD_MODE_WFM;
        break;
    case 5:
        m_DemodMode = DEMOD_MODE_USB;
        break;
    case 6:
        m_DemodMode = DEMOD_MODE_LSB;
        break;
    case 7:
        m_DemodMode = DEMOD_MODE_DSB;
        break;
    case 8:
        m_DemodMode = DEMOD_MODE_CWL;
        break;
    case 9:
        m_DemodMode = DEMOD_MODE_CWU;
        break;
    case 10:
        m_DemodMode = DEMOD_MODE_DIG;
        break;
    case 11:
        m_DemodMode = DEMOD_MODE_RAW;
        break;
    }

    if(m_DemodMode != lastmode)
    {
        if(DEMOD_MODE_DIG == m_DemodMode)
            SetChatDialogState(true);
        else
            SetChatDialogState(false);
        if(DEMOD_MODE_RAW == m_DemodMode)
            SetRawIQWidgetState(true);
        else
            SetRawIQWidgetState(false);

        m_InhibitUpdate = true;	//use to keep controls from updating on initialization
        ui->comboBoxAudioCompression->clear();
        if(DEMOD_MODE_RAW != m_DemodMode )
        {
            ui->comboBoxAudioCompression->setMaxCount(7);
            ui->comboBoxAudioCompression->addItem("No Audio");
            ui->comboBoxAudioCompression->addItem("Raw 64K");
            ui->comboBoxAudioCompression->addItem("G711 64K");
            ui->comboBoxAudioCompression->addItem("G726 40K");
            ui->comboBoxAudioCompression->addItem("G726 32K");
            ui->comboBoxAudioCompression->addItem("G726 24K");
            ui->comboBoxAudioCompression->addItem("G726 16K");
            ui->comboBoxAudioCompression->setCurrentIndex(m_AudioCompressionIndex);
            m_pSdrInterface->SetAudioCompressionMode(m_AudioCompressionIndex);
            ui->labelAudioRate->setText("Audio");
            ui->pushButtonPtt->setEnabled(true);
        }
        else
        {
            ui->comboBoxAudioCompression->setMaxCount(6);
            ui->comboBoxAudioCompression->addItem("16 ksps I/Q");
            ui->comboBoxAudioCompression->addItem("8 ksps I/Q");
            ui->comboBoxAudioCompression->addItem("4 ksps I/Q");
            ui->comboBoxAudioCompression->addItem("2 ksps I/Q");
            ui->comboBoxAudioCompression->addItem("1 ksps I/Q");
            ui->comboBoxAudioCompression->addItem("500 sps I/Q");
            ui->comboBoxAudioCompression->setCurrentIndex(m_RawRateIndex);
            m_pSdrInterface->SetAudioCompressionMode(m_RawRateIndex + COMP_MODE_RAW_16000);	//offset since shares same msg as audio comp
            ui->labelAudioRate->setText("Rate");
            if( ui->pushButtonPtt->isChecked() )
                ui->pushButtonPtt->setChecked(false);
            ui->pushButtonPtt->setEnabled(false);
        }
        m_InhibitUpdate = false;	//use to keep controls from updating on initialization
    }

    ui->spinBoxOffset->setValue(m_DemodSettings[m_DemodMode].Offset);
    ui->horizontalSliderSquelch->setValue(m_DemodSettings[m_DemodMode].SquelchValue);
    ui->frameDecay->SetValue(m_DemodSettings[m_DemodMode].AgcDecay);
    ui->frameThresh->SetValue(m_DemodSettings[m_DemodMode].AgcThresh);

    ui->framePlot->SetDemodRanges(m_DemodSettings[m_DemodMode].LowCutMin,m_DemodSettings[m_DemodMode].LowCutMax,
                                  m_DemodSettings[m_DemodMode].HiCutMin, m_DemodSettings[m_DemodMode].HiCutMax,
                                  m_DemodSettings[m_DemodMode].Symetric);

    m_pSdrInterface->SetDemodMode(m_DemodMode);
    m_pSdrInterface->SetAgc(0, m_DemodSettings[m_DemodMode].AgcThresh, m_DemodSettings[m_DemodMode].AgcDecay);
    m_pSdrInterface->SetSquelchThreshold(m_DemodSettings[m_DemodMode].SquelchValue);
    m_pSdrInterface->SetDemodFilter(m_DemodSettings[m_DemodMode].LowCut,
                                    m_DemodSettings[m_DemodMode].HiCut,
                                    m_DemodSettings[m_DemodMode].Offset);
    ui->framePlot->SetSquelchThreshold(m_DemodSettings[m_DemodMode].SquelchValue);
    ui->frameMeter->SetSquelchPos( m_DemodSettings[m_DemodMode].SquelchValue );

    ui->framePlot->SetHiLowCutFrequencies(m_DemodSettings[m_DemodMode].LowCut, m_DemodSettings[m_DemodMode].HiCut);
    ui->framePlot->UpdateOverlay();

    ui->framePlot->SetClickResolution(m_DemodSettings[m_DemodMode].ClickResolution);
qDebug()<<m_DemodSettings[m_DemodMode].HiCut << m_DemodSettings[m_DemodMode].LowCut;

    if(m_DemodSettings[m_DemodMode].AudioFilter)
        m_pSdrInterface->SetAudioFilter(RX_AUDIOFILTER_CTCSS);
    else
        m_pSdrInterface->SetAudioFilter(RX_AUDIOFILTER_NONE);
    ui->checkBoxAudioHP->setChecked( m_DemodSettings[m_DemodMode].AudioFilter);
}

/////////////////////////////////////////////////////////////////////
// Called when AGC threshold control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnAgcThresh(int val)
{
	if(m_InhibitUpdate)
		return;
	m_DemodSettings[m_DemodMode].AgcThresh = val;
	m_pSdrInterface->SetAgc(0, m_DemodSettings[m_DemodMode].AgcThresh, m_DemodSettings[m_DemodMode].AgcDecay);
}

/////////////////////////////////////////////////////////////////////
// Called when  AGC decay control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnAgcDecay(int val)
{
	if(m_InhibitUpdate)
		return;
	m_DemodSettings[m_DemodMode].AgcDecay = val;
	m_pSdrInterface->SetAgc(0, m_DemodSettings[m_DemodMode].AgcThresh, m_DemodSettings[m_DemodMode].AgcDecay);
}

/////////////////////////////////////////////////////////////////////
// Called when Demod frequency offset control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnOffsetChanged(int Offset)
{
	if(m_InhibitUpdate)
		return;
	m_DemodSettings[m_DemodMode].Offset = Offset;
	m_pSdrInterface->SetDemodFilter(m_DemodSettings[m_DemodMode].LowCut,
									m_DemodSettings[m_DemodMode].HiCut,
									m_DemodSettings[m_DemodMode].Offset);
}

/////////////////////////////////////////////////////////////////////
// Called when HP audio filter control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnAudioFilterChanged(int state)
{
	if(m_InhibitUpdate)
		return;
	if(state)
	{
        m_DemodSettings[m_DemodMode].AudioFilter = true;
		m_pSdrInterface->SetAudioFilter(RX_AUDIOFILTER_CTCSS);
	}
	else
	{
        m_DemodSettings[m_DemodMode].AudioFilter = false;
		m_pSdrInterface->SetAudioFilter(RX_AUDIOFILTER_NONE);
	}
}

/////////////////////////////////////////////////////////////////////
// Called when FM Squelch control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnSquelchChanged(int SquelchValue)
{
	if(m_InhibitUpdate)
		return;
	m_DemodSettings[m_DemodMode].SquelchValue = SquelchValue;
	m_pSdrInterface->SetSquelchThreshold(m_DemodSettings[m_DemodMode].SquelchValue);
	ui->framePlot->SetSquelchThreshold(m_DemodSettings[m_DemodMode].SquelchValue);
	ui->frameMeter->SetSquelchPos( m_DemodSettings[m_DemodMode].SquelchValue );
	ui->framePlot->UpdateOverlay();
qDebug()<<"SquelchValue="<<m_DemodSettings[m_DemodMode].SquelchValue;

}

/////////////////////////////////////////////////////////////////////
// Called when RF Gain control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnRfGainChanged(int gain)
{
	if(m_InhibitUpdate)
		return;
	m_RfGain = gain;
	m_pSdrInterface->SetAtten(m_RfGain);
	if( m_RxCenterFrequency > DOWNCONVERTER_TRANSITION_FREQ )
		ui->spinBoxAtten->setSpecialValueText("Auto Gain");
	else
		ui->spinBoxAtten->setSpecialValueText("");
}

/////////////////////////////////////////////////////////////////////
// Called when Span frequency  control changes
/////////////////////////////////////////////////////////////////////
//service Spectrum Span frequency change
void MainWindow::OnSpanChanged(int span)
{
	if(m_InhibitUpdate)
		return;
	if(m_TxActive)
	{
		m_TxSpanFreq = span;
	}
	else
	{
		if( span <= m_RxSpanFreq )		//if scrolling down
		{
			if( m_RxSpanFreq == 1000000 )	//deal with case where scroll below 1 000 000
			{
				span = 900000;
				ui->spinBoxSpan->setValue(span);
			}
			else if( m_RxSpanFreq == 100000 )	//deal with case where scroll below 100 000
			{
				span = 90000;
				ui->spinBoxSpan->setValue(span);
			}
			else if( m_RxSpanFreq == 10000 )	//deal with case where scroll below 10000
			{
				span = 9000;
				ui->spinBoxSpan->setValue(span);
			}
		}
		if(span < 10000)
			ui->spinBoxSpan->setSingleStep(1000);
		else if(span < 100000)
			ui->spinBoxSpan->setSingleStep(10000);
		else if(span < 1000000)
			ui->spinBoxSpan->setSingleStep(100000);
		else if(span <= 10000000)
			ui->spinBoxSpan->setSingleStep(1000000);
		m_RxSpanFreq = span;
	}
	m_SpanFreq = span;
	ui->framePlot->SetSpanFreq( m_SpanFreq );
	ui->framePlot->UpdateOverlay();
	m_pSdrInterface->SetupFft(m_PlotWidth, m_SpanFreq, m_dBMax,
					  m_dBMax - (m_dBStepSize*VERT_DIVS), m_FftAve, m_FftRate );
}

/////////////////////////////////////////////////////////////////////
// Called when spectrum vertical dB step sixe control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnStepSizeChanged(int index)
{
	if(m_InhibitUpdate)
		return;
	m_dBStepIndex = index;

	int LastdBStepsize = m_dBStepSize;
	int LastMaxdB = m_dBMax;
	m_dBStepSize = ui->comboBoxdBStep->itemData(m_dBStepIndex).toInt();

	if(10 == m_dBStepSize)
	{
		ui->verticalScrollBar_dBOffset->setMinimum(-5);
		ui->verticalScrollBar_dBOffset->setMaximum(7);
	}
	else if(5 == m_dBStepSize)
	{
		ui->verticalScrollBar_dBOffset->setMinimum(0);
		ui->verticalScrollBar_dBOffset->setMaximum(27);
	}
	else if(3 == m_dBStepSize)
	{
		ui->verticalScrollBar_dBOffset->setMinimum(0);
		ui->verticalScrollBar_dBOffset->setMaximum(47);
	}
	else if(2 == m_dBStepSize)
	{
		ui->verticalScrollBar_dBOffset->setMinimum(0);
		ui->verticalScrollBar_dBOffset->setMaximum(70);
	}
	else if(1 == m_dBStepSize)
	{
		ui->verticalScrollBar_dBOffset->setMinimum(0);
		ui->verticalScrollBar_dBOffset->setMaximum(140);
	}
	//adjust m_dBMax to try and keep signal roughly centered at bottom of screen
	if(m_dBStepSize!=LastdBStepsize)
	{
		m_dBMax = LastMaxdB + 11*(m_dBStepSize-LastdBStepsize);
		m_dBMax = (m_dBMax/m_dBStepSize)*m_dBStepSize;
		m_dBMaxIndex = -m_dBMax / m_dBStepSize;
	}
	ui->verticalScrollBar_dBOffset->setValue(m_dBMaxIndex);

	ui->framePlot->SetdBStepSize(m_dBStepSize);
	ui->framePlot->SetMaxdB(m_dBMax);
	ui->framePlot->UpdateOverlay();
	m_pSdrInterface->SetupFft(m_PlotWidth, m_SpanFreq, m_dBMax,
					  m_dBMax - (m_dBStepSize*VERT_DIVS), m_FftAve , m_FftRate);
}

/////////////////////////////////////////////////////////////////////
// Called when Spectrum  averaging control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnAveSpin(int val)
{
	if(m_InhibitUpdate)
		return;
	m_FftAve = val;
	m_pSdrInterface->SetupFft(m_PlotWidth, m_SpanFreq, m_dBMax,
				  m_dBMax - (m_dBStepSize*VERT_DIVS), m_FftAve , m_FftRate);

}

/////////////////////////////////////////////////////////////////////
// Called when Spectrum  averaging control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnAlphaSpin(double val)
{
	m_SmoothAlpha = 1.0 - val/10.0;
	ui->framePlot->SetSmoothAlpha(m_SmoothAlpha);
}

/////////////////////////////////////////////////////////////////////
// Called when Spectrum update rate control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnRateSpin(int rate)
{
	if(m_InhibitUpdate)
		return;
	m_FftRate = rate;
	m_pSdrInterface->SetupFft(m_PlotWidth, m_SpanFreq, m_dBMax,
				  m_dBMax - (m_dBStepSize*VERT_DIVS), m_FftAve , m_FftRate);

}

/////////////////////////////////////////////////////////////////////
// Called when Spectrum dB Offset control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OndBOffsetChanged(int val)
{
	if(m_InhibitUpdate)
		return;
qDebug()<<val <<"MaxdB"<<m_dBMax << m_dBStepSize;
	m_dBMaxIndex = val;
	m_dBMax = -m_dBMaxIndex*m_dBStepSize;
	ui->framePlot->SetMaxdB(m_dBMax);
	ui->framePlot->UpdateOverlay();
	m_pSdrInterface->SetupFft(m_PlotWidth, m_SpanFreq, m_dBMax,
					  m_dBMax - (m_dBStepSize*VERT_DIVS), m_FftAve , m_FftRate);
}

/////////////////////////////////////////////////////////////////////
// Called when NULL NCO Button is pressed
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNullNco()
{
	m_pSdrInterface->PerformNcoNull();
}

/////////////////////////////////////////////////////////////////////
// Called when AutoScale Button is pressed
/////////////////////////////////////////////////////////////////////
void MainWindow::OnAutoScale()
{
	m_pSdrInterface->GetFftAvePwr();

}

/////////////////////////////////////////////////////////////////////
// Called when AutoScale average data is received so auto scale screen
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewFftAvePwr(qint16 Val)
{
	int newMax = Val/100 + (m_dBStepSize*(VERT_DIVS-2));	//put signal baseline 2 divisions above bottom.
	m_dBMaxIndex = (-newMax/m_dBStepSize);
	ui->verticalScrollBar_dBOffset->setValue(m_dBMaxIndex);
//qDebug()<<"FFT AvePwr="<<Val <<"m_dBMax="<<m_dBMax<<" dBMin="<<m_dBMax - (m_dBStepSize*VERT_DIVS);
//qDebug()<<"newMax="<<newMax<<" m_dBMaxIndex="<<m_dBMaxIndex;
}


/////////////////////////////////////////////////////////////////////
// Called when memory dialog checkbox is clicked
/////////////////////////////////////////////////////////////////////
void MainWindow::OnMemoryCheckChanged(int state)
{
	if(state)
	{	//make memory dialog visable if not already
		if(!m_pMemDialog->isVisible())
		{
			m_pMemDialog->setGeometry(m_MemDialogRect);
			m_pMemDialog->show();
		}
		m_pMemDialog->activateWindow();
		m_pMemDialog->raise();
	}
	else
	{	//hide memory dialog
		if(m_pMemDialog->isVisible())
		{
			m_MemDialogRect = m_pMemDialog->geometry();
			m_pMemDialog->hide();
		}
	}
}

/////////////////////////////////////////////////////////////////////
// Called when Spectrum Bandwidth control changes
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewWidth(int width)
{
//qDebug()<<"Width="<<width;
	m_PlotWidth = width;
	m_pSdrInterface->SetupFft(m_PlotWidth, m_SpanFreq, m_dBMax,
					  m_dBMax - (m_dBStepSize*VERT_DIVS), m_FftAve , m_FftRate);
}

/////////////////////////////////////////////////////////////////////
//	Called when MemoryDialog requests current settings
/////////////////////////////////////////////////////////////////////
void MainWindow::OnGetCurrentSetting(bool update)
{
tMem_Record Tmp;
	int mode = m_DemodMode;

	if(update)
	{
		m_pMemDialog->GetRecord(Tmp);	//get copy of current record
	}
	else
	{
		Tmp.MemName = "New Enty";
	}
	Tmp.RxCenterFrequency = m_RxCenterFrequency;
	Tmp.dBStepIndex = m_dBStepIndex;
	Tmp.dBMaxIndex = m_dBMaxIndex;
	Tmp.FftAve = m_FftAve;
	Tmp.FftRate = m_FftRate;
	Tmp.DemodMode = mode;
	Tmp.Volume = m_Volume;
	Tmp.RfGain = m_RfGain;
	Tmp.AudioCompressionIndex = m_AudioCompressionIndex;
	Tmp.VideoCompressionIndex = m_VideoCompressionIndex;
	Tmp.RxSpanFreq = m_RxSpanFreq;
	//demod mode specific items
	Tmp.Offset = m_DemodSettings[mode].Offset;
	Tmp.SquelchValue = m_DemodSettings[mode].SquelchValue;
	Tmp.AgcThresh = m_DemodSettings[mode].AgcThresh;
	Tmp.AgcDecay = m_DemodSettings[mode].AgcDecay;
	Tmp.HiCut = m_DemodSettings[mode].HiCut;
	Tmp.LowCut = m_DemodSettings[mode].LowCut;
	Tmp.Symetric = m_DemodSettings[mode].Symetric;
	Tmp.AudioFilter = m_DemodSettings[mode].AudioFilter;
	if(update)
		m_pMemDialog->UpdateRecord(Tmp);
	else
		m_pMemDialog->AddRecord(Tmp, true);
}

/////////////////////////////////////////////////////////////////////
//	Called when MemoryDialog requests change settings to memory values
/////////////////////////////////////////////////////////////////////
void MainWindow::OnSetCurrentSetting()
{
tMem_Record Tmp;
	m_pMemDialog->GetRecord(Tmp);
	ui->frameFreqCtrl->SetFrequency(Tmp.RxCenterFrequency);
	ui->comboBoxdBStep->setCurrentIndex(Tmp.dBStepIndex);
	ui->verticalScrollBar_dBOffset->setValue(Tmp.dBMaxIndex);
	ui->spinBoxAve->setValue(Tmp.FftAve);
	ui->spinBoxRate->setValue(Tmp.FftRate);
	ui->spinBoxAtten->setValue(Tmp.RfGain);
	if( Tmp.RxCenterFrequency > DOWNCONVERTER_TRANSITION_FREQ )
		ui->spinBoxAtten->setSpecialValueText("Auto Gain");
	else
		ui->spinBoxAtten->setSpecialValueText("");

	ui->comboBoxAudioCompression->setCurrentIndex(Tmp.AudioCompressionIndex);
	ui->comboBoxVideoCompression->setCurrentIndex(Tmp.VideoCompressionIndex);
	ui->horizontalSliderVol->setValue(Tmp.Volume);
	m_SpanFreq = Tmp.RxSpanFreq;
	m_RxSpanFreq = m_SpanFreq;
	ui->spinBoxSpan->setValue(m_RxSpanFreq);
	ui->framePlot->SetSpanFreq( m_SpanFreq );
	ui->framePlot->UpdateOverlay();

	//set demod mode specific items
	m_DemodMode = Tmp.DemodMode;
	m_DemodSettings[m_DemodMode].SquelchValue = Tmp.SquelchValue;
	m_DemodSettings[m_DemodMode].AgcThresh = Tmp.AgcThresh;
	m_DemodSettings[m_DemodMode].AgcDecay = Tmp.AgcDecay;
	m_DemodSettings[m_DemodMode].SquelchValue = Tmp.SquelchValue;
	m_DemodSettings[m_DemodMode].LowCut = Tmp.LowCut;
	m_DemodSettings[m_DemodMode].HiCut = Tmp.HiCut;
	m_DemodSettings[m_DemodMode].Symetric = Tmp.Symetric;
	m_DemodSettings[m_DemodMode].AudioFilter = Tmp.AudioFilter;
    SetDemodSelector(m_DemodMode);
//qDebug()<<Tmp.HiCut << Tmp.LowCut;
	setWindowTitle( m_ProgramExeName + PROGRAM_TITLE_VERSION + "  -  " + Tmp.MemName);
}

void MainWindow::SetDemodSelector(int DemodMode)
{
    int idx;

    m_DemodMode = DemodMode;

    switch(DemodMode)
    {
    default:
    case DEMOD_MODE_AM:
        idx = 0;
        break;
    case DEMOD_MODE_SAM:
        idx = 1;
        break;
    case DEMOD_MODE_WAM:
        idx = 2;
        break;
    case DEMOD_MODE_FM:
        idx = 3;
        break;
    case DEMOD_MODE_WFM:
        idx = 4;
        break;
    case DEMOD_MODE_USB:
        idx = 5;
        break;
    case DEMOD_MODE_LSB:
        idx = 6;
        break;
    case DEMOD_MODE_DSB:
        idx = 7;
        break;
    case DEMOD_MODE_CWL:
        idx = 8;
        break;
    case DEMOD_MODE_CWU:
        idx = 9;
        break;
    case DEMOD_MODE_DIG:
        idx = 10;
        break;
    case DEMOD_MODE_RAW:
        idx = 11;
        break;
    }

    ui->comboBoxDemod->setCurrentIndex(idx);
    OnDemodChanged(idx);
}

/////////////////////////////////////////////////////////////////////
// Setup Demod initial parameters/limits
/////////////////////////////////////////////////////////////////////
void MainWindow::InitDemodSettings()
{
	//set filter limits based on final sample rates etc.
	//These parameters are fixed and not saved in Settings
	m_DemodSettings[DEMOD_MODE_AM].txt = "AM";
    m_DemodSettings[DEMOD_MODE_AM].Symetric = true;
    m_DemodSettings[DEMOD_MODE_AM].DefHiCut = 5000;
	m_DemodSettings[DEMOD_MODE_AM].HiCutMin = 1000;
    m_DemodSettings[DEMOD_MODE_AM].HiCutMax = 10000;
    m_DemodSettings[DEMOD_MODE_AM].DefLowCut = -5000;
    m_DemodSettings[DEMOD_MODE_AM].LowCutMin = -10000;
	m_DemodSettings[DEMOD_MODE_AM].LowCutMax = -1000;
	m_DemodSettings[DEMOD_MODE_AM].ClickResolution = 1000;
    m_DemodSettings[DEMOD_MODE_AM].AudioFilter = false;
	m_DemodSettings[DEMOD_MODE_AM].DefAgcDecay = 500;
	m_DemodSettings[DEMOD_MODE_AM].DefOffset = 0;
	m_DemodSettings[DEMOD_MODE_AM].DefAgcThresh = -120;
	m_DemodSettings[DEMOD_MODE_AM].DefAudioFilter = 0;

	m_DemodSettings[DEMOD_MODE_SAM].txt = "SAM";
    m_DemodSettings[DEMOD_MODE_SAM].Symetric = false;
    m_DemodSettings[DEMOD_MODE_SAM].DefHiCut = 5000;
    m_DemodSettings[DEMOD_MODE_SAM].HiCutMin = 1000;
    m_DemodSettings[DEMOD_MODE_SAM].HiCutMax = 10000;
    m_DemodSettings[DEMOD_MODE_SAM].DefLowCut = -5000;
    m_DemodSettings[DEMOD_MODE_SAM].LowCutMin = -10000;
    m_DemodSettings[DEMOD_MODE_SAM].LowCutMax = -1000;
	m_DemodSettings[DEMOD_MODE_SAM].ClickResolution = 1000;
	m_DemodSettings[DEMOD_MODE_SAM].AudioFilter = false;
	m_DemodSettings[DEMOD_MODE_SAM].DefAgcDecay = 500;
	m_DemodSettings[DEMOD_MODE_SAM].DefOffset = 0;
	m_DemodSettings[DEMOD_MODE_SAM].DefAgcThresh = -120;
	m_DemodSettings[DEMOD_MODE_SAM].DefAudioFilter = 0;

	m_DemodSettings[DEMOD_MODE_WAM].Symetric = true;
	m_DemodSettings[DEMOD_MODE_WAM].DefHiCut = 90000;
	m_DemodSettings[DEMOD_MODE_WAM].HiCutMin = 90000;
	m_DemodSettings[DEMOD_MODE_WAM].HiCutMax = 90000;
	m_DemodSettings[DEMOD_MODE_WAM].DefLowCut = -90000;
	m_DemodSettings[DEMOD_MODE_WAM].LowCutMin = -90000;
	m_DemodSettings[DEMOD_MODE_WAM].LowCutMax = -90000;
	m_DemodSettings[DEMOD_MODE_WAM].ClickResolution = 100000;
	m_DemodSettings[DEMOD_MODE_WAM].AudioFilter = false;
	m_DemodSettings[DEMOD_MODE_WAM].DefAgcDecay = 500;
	m_DemodSettings[DEMOD_MODE_WAM].DefOffset = 0;
	m_DemodSettings[DEMOD_MODE_WAM].DefAgcThresh = -120;
	m_DemodSettings[DEMOD_MODE_WAM].DefAudioFilter = 0;

	m_DemodSettings[DEMOD_MODE_FM].txt = "FM";
    m_DemodSettings[DEMOD_MODE_FM].Symetric = true;
	m_DemodSettings[DEMOD_MODE_FM].DefHiCut = 5000;
    m_DemodSettings[DEMOD_MODE_FM].HiCutMin = 1000;
    m_DemodSettings[DEMOD_MODE_FM].HiCutMax = 15000;
	m_DemodSettings[DEMOD_MODE_FM].DefLowCut = -5000;
    m_DemodSettings[DEMOD_MODE_FM].LowCutMin = -15000;
    m_DemodSettings[DEMOD_MODE_FM].LowCutMax = -1000;
	m_DemodSettings[DEMOD_MODE_FM].ClickResolution = 5000;
	m_DemodSettings[DEMOD_MODE_FM].AudioFilter = false;
	m_DemodSettings[DEMOD_MODE_FM].DefAgcDecay = 500;
	m_DemodSettings[DEMOD_MODE_FM].DefOffset = 0;
	m_DemodSettings[DEMOD_MODE_FM].DefAgcThresh = -120;
	m_DemodSettings[DEMOD_MODE_FM].DefAudioFilter = 1;

	m_DemodSettings[DEMOD_MODE_WFM].txt = "WFM";
    m_DemodSettings[DEMOD_MODE_WFM].Symetric = true;
	m_DemodSettings[DEMOD_MODE_WFM].DefHiCut = 90000;
	m_DemodSettings[DEMOD_MODE_WFM].HiCutMin = 90000;
	m_DemodSettings[DEMOD_MODE_WFM].HiCutMax = 90000;
	m_DemodSettings[DEMOD_MODE_WFM].DefLowCut = -90000;
	m_DemodSettings[DEMOD_MODE_WFM].LowCutMin = -90000;
	m_DemodSettings[DEMOD_MODE_WFM].LowCutMax = -90000;
	m_DemodSettings[DEMOD_MODE_WFM].ClickResolution = 100000;
	m_DemodSettings[DEMOD_MODE_WFM].AudioFilter = false;
	m_DemodSettings[DEMOD_MODE_WFM].DefAgcDecay = 500;
	m_DemodSettings[DEMOD_MODE_WFM].DefOffset = 0;
	m_DemodSettings[DEMOD_MODE_WFM].DefAgcThresh = -120;
	m_DemodSettings[DEMOD_MODE_WFM].DefAudioFilter = 0;

	m_DemodSettings[DEMOD_MODE_USB].txt = "USB";
    m_DemodSettings[DEMOD_MODE_USB].Symetric = false;
	m_DemodSettings[DEMOD_MODE_USB].DefHiCut = 2800;
	m_DemodSettings[DEMOD_MODE_USB].HiCutMin = 1000;
    m_DemodSettings[DEMOD_MODE_USB].HiCutMax = 10000;
	m_DemodSettings[DEMOD_MODE_USB].DefLowCut = 0;
	m_DemodSettings[DEMOD_MODE_USB].LowCutMin = 0;
	m_DemodSettings[DEMOD_MODE_USB].LowCutMax = 500;
	m_DemodSettings[DEMOD_MODE_USB].ClickResolution = 100;
	m_DemodSettings[DEMOD_MODE_USB].AudioFilter = true;
    m_DemodSettings[DEMOD_MODE_USB].DefAgcDecay = 500;
	m_DemodSettings[DEMOD_MODE_USB].DefOffset = 0;
	m_DemodSettings[DEMOD_MODE_USB].DefAgcThresh = -120;
	m_DemodSettings[DEMOD_MODE_USB].DefAudioFilter = 0;

	m_DemodSettings[DEMOD_MODE_LSB].txt = "LSB";
    m_DemodSettings[DEMOD_MODE_LSB].Symetric = false;
	m_DemodSettings[DEMOD_MODE_LSB].DefHiCut = -100;
	m_DemodSettings[DEMOD_MODE_LSB].HiCutMin = -500;
	m_DemodSettings[DEMOD_MODE_LSB].HiCutMax = 0;
	m_DemodSettings[DEMOD_MODE_LSB].DefLowCut = -2800;
    m_DemodSettings[DEMOD_MODE_LSB].LowCutMin = -10000;
	m_DemodSettings[DEMOD_MODE_LSB].LowCutMax = -1000;
	m_DemodSettings[DEMOD_MODE_LSB].ClickResolution = 100;
	m_DemodSettings[DEMOD_MODE_LSB].AudioFilter = false;
    m_DemodSettings[DEMOD_MODE_LSB].DefAgcDecay = 500;
	m_DemodSettings[DEMOD_MODE_LSB].DefOffset = 0;
	m_DemodSettings[DEMOD_MODE_LSB].DefAgcThresh = -120;
	m_DemodSettings[DEMOD_MODE_LSB].DefAudioFilter = 0;

	m_DemodSettings[DEMOD_MODE_DSB].txt = "DSB";
    m_DemodSettings[DEMOD_MODE_DSB].Symetric = false;
	m_DemodSettings[DEMOD_MODE_DSB].DefHiCut = 3000;
	m_DemodSettings[DEMOD_MODE_DSB].HiCutMin = 0;
    m_DemodSettings[DEMOD_MODE_DSB].HiCutMax = 10000;
	m_DemodSettings[DEMOD_MODE_DSB].DefLowCut = -3000;
    m_DemodSettings[DEMOD_MODE_DSB].LowCutMin = -10000;
	m_DemodSettings[DEMOD_MODE_DSB].LowCutMax = 0;
	m_DemodSettings[DEMOD_MODE_DSB].ClickResolution = 100;
	m_DemodSettings[DEMOD_MODE_DSB].AudioFilter = false;
    m_DemodSettings[DEMOD_MODE_DSB].DefAgcDecay = 500;
	m_DemodSettings[DEMOD_MODE_DSB].DefOffset = 0;
	m_DemodSettings[DEMOD_MODE_DSB].DefAgcThresh = -120;
	m_DemodSettings[DEMOD_MODE_DSB].DefAudioFilter = 0;

	m_DemodSettings[DEMOD_MODE_CWU].txt = "CWU";
    m_DemodSettings[DEMOD_MODE_CWU].Symetric = true;
	m_DemodSettings[DEMOD_MODE_CWU].DefHiCut = 250;
	m_DemodSettings[DEMOD_MODE_CWU].HiCutMin = 100;
	m_DemodSettings[DEMOD_MODE_CWU].HiCutMax = 2000;
	m_DemodSettings[DEMOD_MODE_CWU].DefLowCut = -250;
	m_DemodSettings[DEMOD_MODE_CWU].LowCutMin = -2000;
	m_DemodSettings[DEMOD_MODE_CWU].LowCutMax = -100;
	m_DemodSettings[DEMOD_MODE_CWU].ClickResolution = 10;
	m_DemodSettings[DEMOD_MODE_CWU].AudioFilter = false;
	m_DemodSettings[DEMOD_MODE_CWU].DefAgcDecay = 100;
	m_DemodSettings[DEMOD_MODE_CWU].DefOffset = 700;
	m_DemodSettings[DEMOD_MODE_CWU].DefAgcThresh = -120;
	m_DemodSettings[DEMOD_MODE_CWU].DefAudioFilter = 0;

	m_DemodSettings[DEMOD_MODE_CWL].txt = "CWL";
    m_DemodSettings[DEMOD_MODE_CWL].Symetric = true;
	m_DemodSettings[DEMOD_MODE_CWL].DefHiCut = 250;
	m_DemodSettings[DEMOD_MODE_CWL].HiCutMin = 100;
	m_DemodSettings[DEMOD_MODE_CWL].HiCutMax = 2000;
	m_DemodSettings[DEMOD_MODE_CWL].DefLowCut = -250;
	m_DemodSettings[DEMOD_MODE_CWL].LowCutMin = -2000;
	m_DemodSettings[DEMOD_MODE_CWL].LowCutMax = -100;
	m_DemodSettings[DEMOD_MODE_CWL].ClickResolution = 10;
	m_DemodSettings[DEMOD_MODE_CWL].AudioFilter = false;
	m_DemodSettings[DEMOD_MODE_CWL].DefAgcDecay = 100;
	m_DemodSettings[DEMOD_MODE_CWL].DefOffset = -700;
	m_DemodSettings[DEMOD_MODE_CWL].DefAgcThresh = -120;
	m_DemodSettings[DEMOD_MODE_CWL].DefAudioFilter = 0;

	m_DemodSettings[DEMOD_MODE_DIG].txt = "DIG";
	m_DemodSettings[DEMOD_MODE_DIG].Symetric = true;
	m_DemodSettings[DEMOD_MODE_DIG].DefHiCut = 50;
	m_DemodSettings[DEMOD_MODE_DIG].HiCutMin = 50;
	m_DemodSettings[DEMOD_MODE_DIG].HiCutMax = 50;
	m_DemodSettings[DEMOD_MODE_DIG].DefLowCut = -50;
	m_DemodSettings[DEMOD_MODE_DIG].LowCutMin = -50;
	m_DemodSettings[DEMOD_MODE_DIG].LowCutMax = -50;
	m_DemodSettings[DEMOD_MODE_DIG].ClickResolution = 1;
	m_DemodSettings[DEMOD_MODE_DIG].AudioFilter = false;
	m_DemodSettings[DEMOD_MODE_DIG].DefAgcDecay = 100;
	m_DemodSettings[DEMOD_MODE_DIG].DefOffset = 0;
	m_DemodSettings[DEMOD_MODE_DIG].DefAgcThresh = -120;
	m_DemodSettings[DEMOD_MODE_DIG].DefAudioFilter = 0;

	m_DemodSettings[DEMOD_MODE_RAW].txt = "RAWIQ";
	m_DemodSettings[DEMOD_MODE_RAW].Symetric = true;
	m_DemodSettings[DEMOD_MODE_RAW].DefHiCut = 250;
	m_DemodSettings[DEMOD_MODE_RAW].HiCutMin = 50;
	m_DemodSettings[DEMOD_MODE_RAW].HiCutMax = 7500;
	m_DemodSettings[DEMOD_MODE_RAW].DefLowCut = -250;
	m_DemodSettings[DEMOD_MODE_RAW].LowCutMin = -7500;
	m_DemodSettings[DEMOD_MODE_RAW].LowCutMax = -50;
	m_DemodSettings[DEMOD_MODE_RAW].ClickResolution = 1;
	m_DemodSettings[DEMOD_MODE_RAW].AudioFilter = false;
	m_DemodSettings[DEMOD_MODE_RAW].DefAgcDecay = 100;
	m_DemodSettings[DEMOD_MODE_RAW].DefOffset = 0;
	m_DemodSettings[DEMOD_MODE_RAW].DefAgcThresh = -120;
	m_DemodSettings[DEMOD_MODE_RAW].DefAudioFilter = 0;

	ui->frameThresh->SetName("Thresh");
	ui->frameThresh->SetSuffix(" dB");
	ui->frameThresh->setRange(-120, -20);
	ui->frameThresh->setSingleStep(5);
	ui->frameThresh->setPageStep(5);
    ui->frameThresh->setTickInterval(10);

	ui->frameDecay->SetName("Decay");
	ui->frameDecay->SetSuffix(" mS");
	ui->frameDecay->setRange(20, 2000);
	ui->frameDecay->setSingleStep(10);
	ui->frameDecay->setPageStep(100);
    ui->frameDecay->setTickInterval(200);
}
