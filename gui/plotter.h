//////////////////////////////////////////////////////////////////////
// plotter.h: interface for the CPlotter class.
//
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////
#ifndef PLOTTER_H
#define PLOTTER_H

#include <QtGui>
#include <QFrame>
#include <QImage>
#include "interface/sdrinterface.h"

#define VERT_DIVS 10    // specify grid screen divisions
#define HORZ_DIVS 10

#define MAX_TXT 128
#define MAX_SCREENSIZE 3000


#define COLPAL_DEFAULT  0
#define COLPAL_GRAY     1
#define COLPAL_BLUE     2


class CPlotter : public QFrame
{
    Q_OBJECT
public:
	explicit CPlotter(QWidget *parent = 0);
	~CPlotter();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	void SetSdrInterface(CSdrInterface* ptr){m_pSdrInterface = ptr;}
	void SetRunningState(bool running){m_Running = running;}
	void SetClickResolution(int clickres){m_ClickResolution=clickres;}
	void SetFilterClickResolution(int clickres){m_FilterClickResolution=clickres;}
	void ProcNewData();				//call to draw new fft data onto screen plot
	void SetCenterFreq(quint64 f){m_CenterFreq=f;}
	void SetHiLowCutFrequencies(int LowCut, int HiCut){m_DemodLowCutFreq = LowCut; m_DemodHiCutFreq = HiCut;}
	void SetDemodRanges(int FLowCmin, int FLowCmax, int FHiCmin, int FHiCmax, bool symetric);
	void SetSpanFreq(quint32 s){m_Span=(qint32)s;}
	void SetMaxdB(int max){m_MaxdB=max;}
	void SetADOverload(bool ADOverLoad){m_ADOverLoad = ADOverLoad;m_ADOverloadOneShotCounter=0;}
	void SetdBStepSize(int stepsz){m_dBStepSize=stepsz;}
	void SetSquelchThreshold(int dB){m_SquelchThresholddB = dB;}
	void SetVideoMode(int Mode){ m_VideoCompressionMode = Mode;}
	void SetSmoothAlpha(double alpha){ m_SmoothAlpha = alpha;}
	void UpdateOverlay(){DrawOverlay();}
	void Draw(bool newdata);		//call to draw new fft data onto screen plot

signals:
	void NewCenterFreq(qint64 f);
	void NewLowCutFreq(int f);
	void NewHighCutFreq(int f);
	void NewWidth(int w);

public slots:
    void setPalette(int pal);
    void SetPercent2DScreen(int percent) {
        m_Percent2DScreen = percent;
        m_Size = QSize(0,0);
        resizeEvent(NULL);
    }

protected:
		//re-implemented widget event handlers
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent* event);
	void mouseMoveEvent(QMouseEvent * event);
	void mousePressEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void wheelEvent( QWheelEvent * event );

private:
	enum eCapturetype {
	   NONE,
	   LEFT,
	   DEMODDRAG,
	   RIGHT
	};
	void DrawOverlay();
	void MakeFrequencyStrs();
	void SmoothData(quint32* pBuf, qint32 Length);
	int XfromFreq(qint64 freq);
	qint64 FreqfromX(int x);
	qint64 DeltaFreqfromX(int x);
	qint64 RoundFreq(qint64 freq, int resolution);
	bool IsPointCloseTo(int x, int xr, int delta){return ((x > (xr-delta) ) && ( x<(xr+delta)) );}
	void ClampDemodParameters();

	bool m_Running;
	bool m_ADOverLoad;
	bool m_symetric;

	int m_DemodHiCutFreq;
	int m_DemodLowCutFreq;
	int m_CenterFreqX;		//screen coordinate x position
	int m_DemodHiCutFreqX;	//screen coordinate x position
	int m_DemodLowCutFreqX;	//screen coordinate x position
	int m_CursorCaptureDelta;
	int m_GrabPosition;
	int m_Percent2DScreen;
	int m_ADOverloadOneShotCounter;
	int m_SquelchThresholddB;
	int m_VideoCompressionMode;
	int m_FLowCmin;
	int m_FLowCmax;
	int m_FHiCmin;
	int m_FHiCmax;
	int m_ClickResolution;
	int m_FilterClickResolution;
	double m_SmoothAlpha;

	quint32 m_LastSampleRate;

	qint32 m_Span;
	qint32 m_MaxdB;
	qint32 m_MindB;
	qint32 m_dBStepSize;
	qint32 m_FreqUnits;

	qint64 m_CenterFreq;
	qint64 m_GrabFrequency;

	eCapturetype m_CursorCaptured;
	QPixmap m_2DPixmap;
	QPixmap m_OverlayPixmap;
	QPixmap m_WaterfallPixmap;
	QColor m_ColorTbl[256];
	QSize m_Size;
	QString m_Str;
	QString m_HDivText[HORZ_DIVS+1];
	CSdrInterface* m_pSdrInterface;
};

#endif // PLOTTER_H
