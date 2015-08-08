//////////////////////////////////////////////////////////////////////
// mainwindow.h:
//
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHostAddress>
#include <QTimer>
#include <QCheckBox>
#include "interface/sdrinterface.h"
#include "interface/sdrprotocol.h"
#include "gui/memdialog.h"
#include "gui/chatdialog.h"
#include "gui/rawiqwidget.h"

#define NUM_DEMODS (DEMOD_MODE_LAST+1)

typedef struct _sdmd
{
	int HiCut;
	int LowCut;
	int Offset;
	int SquelchValue;
	int AgcThresh;
	int AgcDecay;
	bool AudioFilter;
	int DefHiCut;		//fixed value
	int DefLowCut;		//fixed value
	int HiCutMin;		//fixed value
	int LowCutMin;		//fixed value
	int HiCutMax;		//fixed value
	int LowCutMax;		//fixed value
	int DefOffset;		//fixed value
	int DefAgcThresh;	//fixed value
	int DefAgcDecay;	//fixed value
	int DefAudioFilter;	//fixed value
	int ClickResolution;//fixed value
	bool Symetric;		//fixed value
	QString txt;	//not saved in settings
}tDemodInfo;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	tDemodInfo m_DemodSettings[NUM_DEMODS];	//not all fields are saved in Settings
	void OnDigitalPtt(bool state);

public slots:
	void NewSdrStatus(int status);
	void OnGetCurrentSetting(bool update);
	void OnSetCurrentSetting();
	void OnButtonPtt(bool);

private slots:
	void closeEvent(QCloseEvent *event);
	void keyPressEvent( QKeyEvent * event );
	void keyReleaseEvent( QKeyEvent * event );
	void StayOnTop();
	void OnExit();
	void OnAbout();
	void OnTimer();
	void OnNetworkDlg();
	void OnSoundCardDlg();
	void OnTransmitDlg();
	void OnNewCenterFrequency(qint64 freq);	//called when center frequency has changed
	void OnNewPlotCenterFrequency(qint64 freq);
	void ModeChanged();
	void OnStart();
	void OnNullNco();
	void OnAutoScale();
	void OnAudioFilterChanged(int state);
	void OnVolumeSlider(int value);
	void OnOffsetChanged(int Offset);
	void OnRfGainChanged(int gain);
	void OnSquelchChanged(int SquelchValue);
	void OnAgcThresh(int);
	void OnAgcDecay(int);
	void OnSpanChanged(int);
	void OnAlphaSpin(double val);
	void OnAveSpin(int);
	void OnRateSpin(int);
	void OnStepSizeChanged(int);
	void OndBOffsetChanged(int);
	void OnAudioCompressionChanged(int);
	void OnVideoCompressionChanged(int);
	void OnNewWidth(int);
	void OnNewLowCutFreq(int);
	void OnNewHighCutFreq(int);
	void OnTxTrackChanged(int);
	void OnMemoryCheckChanged(int);

	void NewSMeterValue(qint16 Val);
	void OnNewVideoData();
	void OnNewFftAvePwr(qint16 Val);

private:
	//Private Methods
	void readSettings();
	void writeSettings();
	void InitDemodSettings();
	void SetDemodModeCheckBoxes(int DemodMode);
	void SetChatDialogState(int state);
	void SetRawIQWidgetState(int state);

	//Persistant Variables saved with Settings
	bool m_StayOnTop;
	QHostAddress m_IPAdr;
	quint16 m_Port;
	QString m_RxPassword;
	QString m_TxPassword;
	QString m_DomainName;
	QString m_ListServer;
	QString m_ListServerActionPath;
	QString m_MemoryFilePath;
	QString m_ClientDesc;
	qint64 m_RxCenterFrequency;
	qint64 m_TxCenterFrequency;
	double m_SmoothAlpha;
	int m_DemodMode;
	int m_SoundInIndex;
	int m_SoundOutIndex;
	int m_RfGain;
	int m_AudioCompressionIndex;
	int m_RawRateIndex;
	int m_VideoCompressionIndex;
	int m_RxSpanFreq;
	int m_TxSpanFreq;
	int m_dBStepIndex;
	int m_dBMaxIndex;
	int m_dBMax;
	int m_dBStepSize;
	int m_FftAve;
	int m_FftRate;
	int m_CtcssFreq;
	qint32 m_Volume;
	int m_TestToneFreq;
	tEqualizer m_EqualizerParams;
	tEqualizer m_EqualizerParams1;
	tEqualizer m_EqualizerParams2;
	QRect m_MemDialogRect;
	QRect m_ChatDialogRect;
	QRect m_RawIQWidgetRect;

	//Non-persistant variables
	QString m_Str;
	QString m_ProgramExeName;
	bool m_InhibitUpdate;
	bool m_TxActive;
	int m_PlotWidth;
	int m_ViewdB;
	int m_SpanFreq;
	Ui::MainWindow *ui;
	CSdrInterface* m_pSdrInterface;
	CMemDialog* m_pMemDialog;
	QTimer *m_pTimer;

};

#endif // MAINWINDOW_H
