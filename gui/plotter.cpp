//////////////////////////////////////////////////////////////////////
// plotter.cpp: implementation of the CPlotter class.
//
//  This class creates and draws a combination spectral view using
// a 2D and waterfall display and manages mouse events within the plot area
//
// History:
//	2013-10-02  Initial creation MSW
//	2013-10-13  Added Squelch Threshold Indicator
//	2014-06-28  Fixed bug in pixel draw truncation(thanks to find and fix by Alex OZ9AEC)
//	2014-07-07  Fixed bug in demod hi low cut frequency tooltip display during scroll wheel
//	2014-07-09  Fixed bug in demod hi low cut frequency tooltip display when right clicked
//	2014-07-18  Added pixel smoothing function for compressed video mode
//	2014-10-08  Added exponential pixel smoothing function for all 2D video data
//////////////////////////////////////////////////////////////////////

//==========================================================================================
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
//==========================================================================================
#include "gui/plotter.h"
#include <stdlib.h>
#include <QDebug>
#include <QToolTip>
#include "interface/sdrprotocol.h"

//////////////////////////////////////////////////////////////////////
// Local defines
//////////////////////////////////////////////////////////////////////
#define CUR_CUT_DELTA 10		//cursor capture delta in pixels
#define OVERLOAD_DISPLAY_LIMIT 10

//color lookup table for waterfall
// index is 0 to 255  where 255 is the minimum dB level, o is max dB level
// grouped as R,G,B values
const quint8 PALTBL[256*3] =
{
252,238,251,
252,230,251,
252,218,251,
251,208,254,
249,201,253,
250,195,253,
255,182,253,
250,172,248,
250,162,249,
250,155,249,
253,146,252,
255,133,252,
255,124,252,
251,115,249,
252,109,251,
255,97,255,
249,87,251,
251,79,253,
251,69,252,
251,62,252,
255,54,254,
252,44,252,
250,37,251,
252,25,252,
255,21,255,
254,15,255,
252,7,252,
252,2,250,
253,0,248,
254,1,242,
253,1,234,
251,2,228,
252,0,221,
254,0,220,
253,0,215,
253,1,208,
253,1,200,
254,0,192,
254,1,188,
254,0,182,
253,1,176,
251,0,167,
251,1,163,
250,0,160,
251,1,153,
250,1,145,
251,1,137,
252,1,134,
251,1,127,
250,1,121,
252,1,117,
253,1,114,
252,0,111,
252,1,106,
252,1,98,
251,1,88,
252,1,80,
252,1,78,
252,0,73,
253,0,67,
253,1,62,
253,0,57,
253,0,54,
252,1,44,
252,1,34,
253,1,26,
252,0,21,
250,0,14,
251,0,5,
250,2,2,
251,8,2,
252,14,2,
252,26,2,
251,37,0,
252,48,0,
254,57,2,
255,63,1,
252,72,0,
252,79,0,
251,85,0,
250,93,0,
249,96,2,
251,104,1,
252,110,2,
255,113,3,
255,113,1,
253,118,0,
254,124,2,
252,127,1,
253,132,1,
255,134,3,
251,137,2,
252,142,1,
250,145,1,
254,150,3,
251,152,0,
253,154,0,
253,159,1,
253,164,0,
251,169,0,
251,169,0,
251,174,0,
252,179,0,
252,181,1,
248,183,0,
252,189,0,
251,192,0,
252,198,1,
251,203,0,
252,205,1,
251,209,0,
248,211,0,
251,216,0,
252,217,1,
254,220,1,
254,224,2,
254,229,2,
253,235,1,
251,238,0,
252,239,0,
253,244,1,
251,247,0,
250,251,0,
247,252,2,
242,251,2,
232,251,1,
223,252,2,
214,250,2,
210,250,3,
205,251,3,
197,249,0,
187,252,0,
182,253,1,
177,252,1,
168,251,0,
159,251,0,
150,252,2,
146,252,2,
140,250,1,
132,251,1,
121,252,0,
113,252,1,
104,253,3,
72,251,0,
39,253,0,
13,252,0,
2,251,1,
1,249,3,
0,243,1,
1,238,0,
1,235,0,
3,233,3,
1,231,1,
1,226,2,
2,221,3,
0,217,4,
1,215,3,
0,211,8,
1,206,25,
0,203,36,
1,200,49,
1,197,53,
0,192,53,
0,189,61,
1,185,73,
0,180,83,
1,178,88,
2,173,95,
1,171,98,
1,166,106,
1,163,116,
0,158,118,
2,156,130,
2,152,140,
1,148,142,
1,144,148,
3,139,161,
2,138,164,
1,133,171,
1,128,179,
2,127,183,
2,123,192,
2,118,201,
0,114,202,
1,109,210,
2,107,214,
2,106,217,
1,102,220,
0,97,222,
0,94,228,
0,92,229,
0,89,233,
0,86,235,
0,83,237,
2,82,239,
1,81,240,
1,80,243,
1,78,246,
2,77,248,
0,74,247,
0,71,247,
2,73,251,
0,69,243,
0,68,239,
0,68,239,
3,67,237,
4,67,234,
1,63,226,
2,61,223,
2,60,219,
0,58,217,
0,57,209,
0,56,205,
0,56,205,
0,53,201,
1,52,197,
2,51,192,
2,49,187,
2,48,185,
0,46,181,
0,45,174,
0,43,171,
1,42,166,
0,42,163,
0,41,159,
3,40,155,
3,39,151,
3,37,145,
2,36,143,
0,33,138,
0,32,135,
1,30,132,
0,29,131,
0,28,127,
1,25,121,
1,24,117,
0,23,113,
0,21,110,
0,21,104,
1,20,99,
1,18,96,
0,16,91,
0,16,91,
1,15,86,
0,13,83,
0,10,76,
0,7,75,
1,6,70,
2,7,65,
3,6,59,
3,5,56,
3,5,56,
3,3,55,
2,1,41,
0,1,21,
0,0,8
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CPlotter::CPlotter(QWidget *parent) :
	QFrame(parent)
{
	m_pSdrInterface = NULL;
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //setFocusPolicy(Qt::StrongFocus);
    //setAttribute(Qt::WA_PaintOnScreen,false);
    //setAutoFillBackground(false);
    //setAttribute(Qt::WA_OpaquePaintEvent, false);
    //setAttribute(Qt::WA_NoSystemBackground, true);
    setMouseTracking(true);

    setPalette(COLPAL_BLUE);

	m_CenterFreq = 680000;
	m_DemodHiCutFreq = 5000;
	m_DemodLowCutFreq = -5000;

	m_FLowCmin = -10000;
	m_FLowCmax = -100;
	m_FHiCmin = 100;
	m_FHiCmax = 10000;
	m_symetric = true;

	m_ClickResolution = 100;
	m_FilterClickResolution = 100;
	m_CursorCaptureDelta = CUR_CUT_DELTA;

	m_Span = 50000;
	m_MaxdB = 0;
	m_MindB = -130;
	m_dBStepSize = 10;
	m_FreqUnits = 1000;
	m_SquelchThresholddB = -160;
	m_CursorCaptured = NONE;
	m_Running = false;
	m_ADOverloadOneShotCounter = 0;
	m_ADOverLoad = false;
	m_2DPixmap = QPixmap(0,0);
	m_OverlayPixmap = QPixmap(10,10);
	m_WaterfallPixmap = QPixmap(10,10);
	m_Size = QSize(10,10);
	m_GrabPosition = 0;
	m_Percent2DScreen = 50;	//percent of screen used for 2D display
	m_VideoCompressionMode = 0;
	m_SmoothAlpha = 0.7;
}

CPlotter::~CPlotter()
{
}

void CPlotter::setPalette(int pal)
{
    int     i;
    int     j;

    switch (pal)
    {
    default:
    case COLPAL_DEFAULT:
        for (i = 0, j = 0; i < 256; i++, j+=3)
            m_ColorTbl[255-i].setRgb(PALTBL[j], PALTBL[j+1], PALTBL[j+2]);
        break;

    case COLPAL_GRAY:
        for (i = 0; i < 256; i++)
            m_ColorTbl[i].setRgb(i, i, i);
        break;

    case COLPAL_BLUE:

        for (i = 0; i < 256-63; i++)
            m_ColorTbl[i].setRgb(1+i, 1+i, 63+i);
        for (i = 256-63; i< 256; i++)
            m_ColorTbl[i].setRgb(i, i, 255);
        break;
    }
}

//////////////////////////////////////////////////////////////////////
// Sizing interface
//////////////////////////////////////////////////////////////////////
QSize CPlotter::minimumSizeHint() const
{
	return QSize(200, 200);
}

QSize CPlotter::sizeHint() const
{
	return QSize(200, 200);
}


//////////////////////////////////////////////////////////////////////
// Called when mouse moves and does different things depending
//on the state of the mouse buttons for dragging the demod bar or
// filter edges.
//////////////////////////////////////////////////////////////////////
void CPlotter::mouseMoveEvent(QMouseEvent* event)
{
	QPoint pt = event->pos();
	if( m_OverlayPixmap.rect().contains(pt) )
	{	//is in Overlay bitmap region
		if( event->buttons()==Qt::NoButton)
		{	//if no mouse button monitor grab regions and change cursor icon
			if( IsPointCloseTo( pt.x(),m_DemodHiCutFreqX, m_CursorCaptureDelta) )
			{	//in move demod hicut region
				if(RIGHT!=m_CursorCaptured)
					setCursor(QCursor(Qt::SizeFDiagCursor));
				m_CursorCaptured = RIGHT;
				QToolTip::showText(event->globalPos(),
								   QString::number(m_DemodHiCutFreq),
								   this, rect() );
			}
			else if( IsPointCloseTo( pt.x(),m_DemodLowCutFreqX, m_CursorCaptureDelta) )
			{	//in move demod lowcut region
				if(LEFT!=m_CursorCaptured)
					setCursor(QCursor(Qt::SizeBDiagCursor));
				m_CursorCaptured = LEFT;
				QToolTip::showText(event->globalPos(),
								   QString::number(m_DemodLowCutFreq),
								   this, rect() );
			}
			else
			{	//if not near any grab boundaries
				if(NONE!=m_CursorCaptured)
				{
					setCursor(QCursor(Qt::ArrowCursor));
					m_CursorCaptured = NONE;
				}
				QToolTip::showText(event->globalPos(),
								   QString::number( FreqfromX( pt.x() ) ),
								   this, rect() );
			}
			m_GrabPosition = 0;
		}
	}
	else
	{	//not in Overlay region
		if( event->buttons()==Qt::NoButton)
		{
			if(NONE!=m_CursorCaptured)
				setCursor(QCursor(Qt::ArrowCursor));
			m_CursorCaptured = NONE;
			m_GrabPosition = 0;
			QToolTip::showText(event->globalPos(),
							   QString::number( FreqfromX( pt.x() ) ),
							   this, rect() );
		}
	}

	//process mouse moves while in cursor capture modes
	if(LEFT==m_CursorCaptured)
	{	//moving in demod lowcut region
		if(event->buttons()&Qt::RightButton)
		{	//moving in demod lowcut region with right button held
				if(m_GrabPosition!=0)
			{
				m_DemodLowCutFreq = FreqfromX(pt.x()-m_GrabPosition ) - m_CenterFreq;
				m_DemodLowCutFreq = RoundFreq(m_DemodLowCutFreq, m_FilterClickResolution);
				DrawOverlay();
				if(m_symetric)
				{
					m_DemodHiCutFreq = -m_DemodLowCutFreq;
					emit NewHighCutFreq(m_DemodHiCutFreq);
				}
				emit NewLowCutFreq(m_DemodLowCutFreq);
			}
			else
			{	//save initial grab postion from m_DemodLowCutFreqX
				m_GrabPosition = pt.x()-m_DemodLowCutFreqX;
			}
			QToolTip::showText(event->globalPos(),
							   QString::number(m_DemodLowCutFreq),
							   this, rect() );
		}
		else if(event->buttons() & ~Qt::NoButton)
		{
			setCursor(QCursor(Qt::ArrowCursor));
			m_CursorCaptured = NONE;
		}
	}
	else if(RIGHT==m_CursorCaptured)
	{	//moving in demod highcut region
		if(event->buttons()&Qt::RightButton)
		{	//moving in demod highcut region with right button held
			if(m_GrabPosition!=0)
			{
				m_DemodHiCutFreq = FreqfromX( pt.x()-m_GrabPosition ) - m_CenterFreq;
				m_DemodHiCutFreq = RoundFreq(m_DemodHiCutFreq, m_FilterClickResolution);
				DrawOverlay();
				if(m_symetric)
				{
					m_DemodLowCutFreq = -m_DemodHiCutFreq;
					emit NewLowCutFreq(m_DemodLowCutFreq);
				}
				emit NewHighCutFreq(m_DemodHiCutFreq);
			}
			else
			{	//save initial grab postion from m_DemodHiCutFreqX
				m_GrabPosition = pt.x()-m_DemodHiCutFreqX;
			}
			QToolTip::showText(event->globalPos(),
							   QString::number(m_DemodHiCutFreq),
							   this, rect() );
		}
		else if(event->buttons() & ~Qt::NoButton)
		{
			setCursor(QCursor(Qt::ArrowCursor));
			m_CursorCaptured = NONE;
		}
	}
	else if(DEMODDRAG==m_CursorCaptured)
	{	//Dragging grabbed demod freqeuncy bar
		if(event->buttons()&Qt::LeftButton)
		{
			m_GrabFrequency = FreqfromX( pt.x() );
			m_GrabFrequency = RoundFreq( m_GrabFrequency, m_ClickResolution);
			QToolTip::showText(event->globalPos(),
							   QString::number(m_GrabFrequency),
							   this, rect() );
			DrawOverlay();
		}
		else if(event->buttons() & ~Qt::NoButton)
		{
			m_CursorCaptured = NONE;
			m_CenterFreq = m_GrabFrequency;
			emit NewCenterFreq(m_CenterFreq);
			setCursor(QCursor(Qt::ArrowCursor));
		}
	}
	else	//if cursor not captured
	{
		m_GrabPosition = 0;
	}
	if( !this->rect().contains(pt) )
	{
		if(NONE != m_CursorCaptured)
			setCursor(QCursor(Qt::ArrowCursor));
		m_CursorCaptured = NONE;
	}
}

//////////////////////////////////////////////////////////////////////
// Called when a mouse button is pressed
//////////////////////////////////////////////////////////////////////
void CPlotter::mousePressEvent(QMouseEvent * event)
{
QPoint pt = event->pos();
	if(event->buttons()==Qt::LeftButton)
	{
		if( DEMODDRAG!=m_CursorCaptured)
		{	//if cursor not captured set start Center frequency drag capture
			setCursor(QCursor(Qt::OpenHandCursor));
			m_CursorCaptured = DEMODDRAG;
			m_GrabPosition =  pt.x();
			m_GrabFrequency = FreqfromX( pt.x() );
			m_GrabFrequency = RoundFreq( m_GrabFrequency, m_ClickResolution);
			QToolTip::showText(event->globalPos(),
							   QString::number( m_GrabFrequency ),
							   this, rect() );
			DrawOverlay();
//qDebug()<< m_GrabFrequency;
		}
	}
	else if(event->buttons()==Qt::RightButton)
	{
		if( NONE==m_CursorCaptured)
		{	//if cursor not captured set center freq
			m_CenterFreq = RoundFreq(FreqfromX(pt.x()),m_ClickResolution );
			emit NewCenterFreq(m_CenterFreq);
		}else if(RIGHT==m_CursorCaptured)
		{
			QToolTip::showText(event->globalPos(),
							   QString::number(m_DemodHiCutFreq),
							   this, rect() );
		}else if(LEFT==m_CursorCaptured)
		{
			QToolTip::showText(event->globalPos(),
							   QString::number(m_DemodLowCutFreq),
							   this, rect() );
		}
	}
	else if(event->buttons()==Qt::MiddleButton)
	{
		m_CenterFreq = RoundFreq(FreqfromX(pt.x()),m_ClickResolution );
		emit NewCenterFreq(m_CenterFreq);
	}
}

//////////////////////////////////////////////////////////////////////
// Called when a mouse button is released
//////////////////////////////////////////////////////////////////////
void CPlotter::mouseReleaseEvent(QMouseEvent * event)
{
QPoint pt = event->pos();
	if( DEMODDRAG == m_CursorCaptured )
	{
		setCursor(QCursor(Qt::ArrowCursor));
		m_CursorCaptured = NONE;
		m_GrabFrequency = RoundFreq( m_GrabFrequency, m_ClickResolution);
		m_CenterFreq = m_GrabFrequency;
		emit NewCenterFreq(m_CenterFreq);
	}else if( !m_OverlayPixmap.rect().contains(pt)  )
	{	//not in Overlay region
		if(NONE!=m_CursorCaptured)
			setCursor(QCursor(Qt::ArrowCursor));
		m_CursorCaptured = NONE;
		m_GrabPosition = 0;
		DrawOverlay();
	}
}

//////////////////////////////////////////////////////////////////////
// Called when a mouse wheel is turned
//////////////////////////////////////////////////////////////////////
void CPlotter::wheelEvent( QWheelEvent * event )
{
Q_UNUSED(event);
int numDegrees = event->delta() / 8;
int numSteps = numDegrees / 15;
	if(event->buttons()==Qt::RightButton)
	{	//right button held while wheel is spun
		if(RIGHT==m_CursorCaptured)
		{	//change demod high cut
			m_DemodHiCutFreq  += (numSteps*m_FilterClickResolution);
			m_DemodHiCutFreq = RoundFreq(m_DemodHiCutFreq, m_FilterClickResolution);
			DrawOverlay();
			if(m_symetric)
			{
				m_DemodLowCutFreq = -m_DemodHiCutFreq;
				emit NewLowCutFreq(m_DemodLowCutFreq);
			}
			emit NewHighCutFreq(m_DemodHiCutFreq);
			QToolTip::showText(event->globalPos(),
							   QString::number(m_DemodHiCutFreq),
							   this, rect() );
		}
		else if(LEFT==m_CursorCaptured)
		{	//change demod low cut
			m_DemodLowCutFreq  += (numSteps*m_FilterClickResolution);
			m_DemodLowCutFreq = RoundFreq(m_DemodLowCutFreq, m_FilterClickResolution);
			DrawOverlay();
			if(m_symetric)
			{
				m_DemodHiCutFreq = -m_DemodLowCutFreq;
				emit NewHighCutFreq(m_DemodHiCutFreq);
			}
			emit NewLowCutFreq(m_DemodLowCutFreq);
			QToolTip::showText(event->globalPos(),
							   QString::number(m_DemodLowCutFreq),
							   this, rect() );
		}
	}
	else
	{	//inc/dec demod frequency if right button NOT pressed
		m_CenterFreq += (numSteps*m_ClickResolution);
		m_CenterFreq = RoundFreq(m_CenterFreq, m_ClickResolution );
		emit NewCenterFreq(m_CenterFreq);
	}
}

//////////////////////////////////////////////////////////////////////
// Called when screen size changes so must recalculate bitmaps
//////////////////////////////////////////////////////////////////////
void CPlotter::resizeEvent(QResizeEvent* )
{
	if(!size().isValid())
		return;
	if( m_Size != size() )
	{	//if changed, resize pixmaps to new screensize
		m_Size = size();
		m_OverlayPixmap = QPixmap(m_Size.width(), m_Percent2DScreen*m_Size.height()/100);
        m_OverlayPixmap.fill(Qt::white);
		m_2DPixmap = QPixmap(m_Size.width(), m_Percent2DScreen*m_Size.height()/100);
        m_2DPixmap.fill(Qt::white);
		m_WaterfallPixmap = QPixmap(m_Size.width(), (100-m_Percent2DScreen)*m_Size.height()/100);
		emit NewWidth(m_Size.width());
	}
    m_WaterfallPixmap.fill(Qt::black);
    DrawOverlay();
}

//////////////////////////////////////////////////////////////////////
// Called by QT when screen needs to be redrawn
//////////////////////////////////////////////////////////////////////
void CPlotter::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.drawPixmap(0,0,m_2DPixmap);
	painter.drawPixmap(0, m_Percent2DScreen*m_Size.height()/100,m_WaterfallPixmap);
	return;
}


//////////////////////////////////////////////////////////////////////
// Called to update the screen with data from video queue
//////////////////////////////////////////////////////////////////////
void CPlotter::Draw(bool newdata)
{
int i;
int w;
int h;
quint32 fftbuf[MAX_SCREENSIZE];
QPoint LineBuf[MAX_SCREENSIZE];

	for(i=0; i<MAX_SCREENSIZE; i++)
		fftbuf[i] = 0;

//StartPerformance();
	//get/draw the waterfall
	w = m_WaterfallPixmap.width();
	h = m_WaterfallPixmap.height();

	//move current data down one line(must do before attaching a QPainter object)
	if( newdata )
		m_WaterfallPixmap.scroll(0,1,0,0, w, h);

	QPainter painter1(&m_WaterfallPixmap);
	//get scaled FFT data
	if(	m_pSdrInterface )
		m_pSdrInterface->GetFftData(fftbuf, w);

	//draw new line of fft data at top of waterfall bitmap
	for(i=0; i<w; i++)
	{
		painter1.setPen(m_ColorTbl[ fftbuf[i] ]);
		painter1.drawPoint(i,0);
	}

	//get/draw the 2D spectrum
	w = m_2DPixmap.width();
	h = m_2DPixmap.height();
	//first copy into 2Dbitmap the overlay bitmap.
	m_2DPixmap = m_OverlayPixmap.copy(0,0,w,h);

	QPainter painter2(&m_2DPixmap);

// workaround for "fixed" line drawing since Qt 5
// see http://stackoverflow.com/questions/16990326
#if QT_VERSION >= 0x050000
	painter2.translate(0.5, 0.5);
#endif

	//scale 0 to 255 fft data to maximum heigth of 2d and invert
	for(i=0; i<w; i++)
	{
		fftbuf[i] = h - (h * (fftbuf[i]*100) + 12750 ) /(25500);
	}

//	if(COMP_MODE_4BIT == m_VideoCompressionMode )
		SmoothData(fftbuf, w);

	//draw the 2D spectrum
	bool fftoverload = false;
	if(m_ADOverLoad || fftoverload)
	{
        painter2.setPen(Qt::red);
		if(m_ADOverloadOneShotCounter++ > OVERLOAD_DISPLAY_LIMIT)
		{
			m_ADOverloadOneShotCounter = 0;
			m_ADOverLoad = false;
		}
	}
	else
        //painter2.setPen(Qt::blue);
        painter2.setPen(QColor(48, 48, 255, 255));
	for(i=0; i<w; i++)
	{
		LineBuf[i].setX(i);
		LineBuf[i].setY(fftbuf[i]);
	}
	painter2.drawPolyline(LineBuf,w);

	//trigger a new paintEvent
	update();
//StopPerformance(1);

}

#if 1
//////////////////////////////////////////////////////////////////////
// Called to Smooth the 2D data by exponential smoothing over 2 pixels
// with an alpha of m_SmoothAlpha
//////////////////////////////////////////////////////////////////////
void CPlotter::SmoothData(quint32* pBuf, qint32 Length)
{
double Lbuf[MAX_SCREENSIZE];
double Rbuf[MAX_SCREENSIZE];
	for (int i = 0; i < Length; i++)
	{
		Lbuf[i] = (float)pBuf[i];
		Rbuf[i] = (float)pBuf[i];
	}
	//Fwd scan
	for (int i = 1; i < Length; ++i)
		Lbuf[i] = m_SmoothAlpha*Lbuf[i] + (1.0-m_SmoothAlpha)*Lbuf[i-1];
	//Rev scan
	for (int i = Length - 2; i >= 0; --i)
		Rbuf[i] = m_SmoothAlpha*Rbuf[i] + (1.0-m_SmoothAlpha)*Rbuf[i+1];
	for (int i = 0; i < Length; i++)
		pBuf[i] = (quint32)( (Lbuf[i]+Rbuf[i])/2.0 );
}
#else
//////////////////////////////////////////////////////////////////////
// Called to Smooth the 2D data by averaging over 3 pixels
//////////////////////////////////////////////////////////////////////
void CPlotter::SmoothData(quint32* pBuf, qint32 Length)
{
	for(int i=1; i<Length-1; i++)
	{
		quint32 x0 = pBuf[i-1];
		quint32 x1 = pBuf[i-0];
		quint32 x2 = pBuf[i+1];
		pBuf[i] = (x0 + x1 + x2 + 2)/3;
	}
}
#endif
//////////////////////////////////////////////////////////////////////
// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every fft data update.
//////////////////////////////////////////////////////////////////////
void CPlotter::DrawOverlay()
{
	if(m_OverlayPixmap.isNull())
		return;
int w = m_OverlayPixmap.width();
int h = m_OverlayPixmap.height();
int x,y;
int SquelchThresh;
float pixperdiv;
QRect rect;
	QPainter painter(&m_OverlayPixmap);
	painter.initFrom(this);

    painter.setPen(Qt::white);      // needed to avoid black line on left and upper sides
    painter.setBrush(QBrush(Qt::white, Qt::SolidPattern));
	painter.drawRect(0, 0, w, h);

	//Draw demod filter box
	ClampDemodParameters();
    if (DEMODDRAG != m_CursorCaptured)
	{
		m_CenterFreqX = XfromFreq(m_CenterFreq);
		m_DemodLowCutFreqX = XfromFreq(m_CenterFreq + m_DemodLowCutFreq);
		m_DemodHiCutFreqX = XfromFreq(m_CenterFreq + m_DemodHiCutFreq);
	}
	else
	{
		m_CenterFreqX = XfromFreq(m_GrabFrequency);
		m_DemodLowCutFreqX = XfromFreq(m_GrabFrequency + m_DemodLowCutFreq);
		m_DemodHiCutFreqX = XfromFreq(m_GrabFrequency + m_DemodHiCutFreq);
	}

	//calculate position of Squelch Threshold position
    m_MindB = m_MaxdB - VERT_DIVS * m_dBStepSize;
    SquelchThresh = h * (m_SquelchThresholddB - m_MaxdB) / (m_MindB-m_MaxdB);
    if (SquelchThresh < 0)
		SquelchThresh = 0;
    if (SquelchThresh > h)
		SquelchThresh = h;
    painter.setPen(QPen(Qt::darkRed, 1,Qt::SolidLine));
	painter.drawLine(m_DemodLowCutFreqX, SquelchThresh, m_DemodHiCutFreqX, SquelchThresh);

	int dw = m_DemodHiCutFreqX - m_DemodLowCutFreqX;
	painter.setBrush(Qt::SolidPattern);
    painter.setOpacity(0.3);
    painter.fillRect(m_DemodLowCutFreqX, 0,dw, h, Qt::lightGray);

    painter.setPen(QPen(Qt::darkGray, 1, Qt::DotLine));
	painter.drawLine(m_DemodLowCutFreqX, 0, m_DemodLowCutFreqX, h);
	painter.drawLine(m_DemodHiCutFreqX, 0, m_DemodHiCutFreqX, h);

    painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
	painter.drawLine(m_CenterFreqX, 0, m_CenterFreqX, h);
	painter.setOpacity(1);

	//create Font to use for scales
    QFont Font;
    Font.setPointSize(10);
	QFontMetrics metrics(Font);
	y = h/VERT_DIVS;
	if(y<metrics.height())
		Font.setPixelSize(y);
	Font.setWeight(QFont::Normal);
	painter.setFont(Font);

    // draw vertical grids
	pixperdiv = (float)w / (float)HORZ_DIVS;
	y = h - h/VERT_DIVS;
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    for (int i = 1; i < HORZ_DIVS; i++)
	{
        x = (int) (0.5 + (float) i * pixperdiv );
		painter.drawLine(x, 0, x , y);
		painter.drawLine(x, h-5, x , h);
	}
//qDebug()<<"w = "<<w <<"center = "<<pixperdiv;
    // draw frequency values
	MakeFrequencyStrs();
    painter.setPen(Qt::black);
	y = h - (h/VERT_DIVS);
    for (int i=0; i <= HORZ_DIVS; i++)
	{
		if(0==i)
		{	//left justify the leftmost text
			x = (int)( (float)i*pixperdiv);
			rect.setRect(x ,y, (int)pixperdiv, h/VERT_DIVS);
			painter.drawText(rect, Qt::AlignLeft|Qt::AlignVCenter, m_HDivText[i]);
		}
		else if(HORZ_DIVS == i)
		{	//right justify the rightmost text
			x = (int)( (float)i*pixperdiv - pixperdiv);
			rect.setRect(x ,y, (int)pixperdiv, h/VERT_DIVS);
			painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, m_HDivText[i]);
		}
		else
		{	//center justify the rest of the text
			x = (int)( (float)i*pixperdiv - pixperdiv/2);
			rect.setRect(x ,y, (int)pixperdiv, h/VERT_DIVS);
			painter.drawText(rect, Qt::AlignHCenter|Qt::AlignVCenter, m_HDivText[i]);
		}
	}


	//draw horizontal grids
	pixperdiv = (float)h / (float)VERT_DIVS;
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
	for( int i=1; i<VERT_DIVS; i++)
	{
		y = (int)( (float)i*pixperdiv );
		painter.drawLine(0, y, w, y);
	}

    // draw amplitude values
    painter.setPen(Qt::black);
    //Font.setWeight(QFont::Light);
	painter.setFont(Font);
	int dB = m_MaxdB;
	for( int i=0; i<VERT_DIVS-1; i++)
	{
		y = (int)( (float)i*pixperdiv );
		painter.drawStaticText(5, y-1, QString::number(dB)+" dB");
		dB -= m_dBStepSize;
	}

	if(!m_Running  )
	{	//if not running so is no data updates to draw to screen
		//copy into 2Dbitmap the overlay bitmap.
		m_2DPixmap = m_OverlayPixmap.copy(0,0,w,h);
		//trigger a new paintEvent
		update();
	}
	else if( DEMODDRAG == m_CursorCaptured )
	{
		m_2DPixmap = m_OverlayPixmap.copy(0,0,w,h);
		Draw(false);
	}
}

//////////////////////////////////////////////////////////////////////
// Helper function Called to create all the frequency division text
//strings based on start frequency, span frequency, frequency units.
//Places in QString array m_HDivText
//Keeps all strings the same fractional length
//////////////////////////////////////////////////////////////////////
void CPlotter::MakeFrequencyStrs()
{
qint64 FreqPerDiv = m_Span/HORZ_DIVS;
qint64 StartFreq = m_CenterFreq - m_Span/2;
float freq;
int i,j;
int numfractdigits = (int)log10((double)m_FreqUnits);
	if(1 == m_FreqUnits)
	{	//if units is Hz then just output integer freq
		for(int i=0; i<=HORZ_DIVS; i++)
		{
			freq = (float)StartFreq/(float)m_FreqUnits;
			m_HDivText[i].setNum((int)freq);
			StartFreq += FreqPerDiv;
		}
		return;
	}
	//here if is fractional frequency values
	//so create max sized text based on frequency units
	for(int i=0; i<=HORZ_DIVS; i++)
	{
		freq = (float)StartFreq/(float)m_FreqUnits;
		m_HDivText[i].setNum(freq,'f', numfractdigits);
		StartFreq += FreqPerDiv;
	}
	//now find the division text with the longest non-zero digit
	//to the right of the decimal point.
	int max = 0;
	for(i=0; i<=HORZ_DIVS; i++)
	{
		int dp = m_HDivText[i].indexOf('.');
		int l = m_HDivText[i].length()-1;
		for(j=l; j>dp; j--)
		{
			if(m_HDivText[i][j] != '0')
				break;
		}
		if( (j-dp) > max)
			max = j-dp;
	}
	//truncate all strings to maximum fractional length
	StartFreq = m_CenterFreq - m_Span/2;
	for( i=0; i<=HORZ_DIVS; i++)
	{
		freq = (float)StartFreq/(float)m_FreqUnits;
		m_HDivText[i].setNum(freq,'f', max);
		StartFreq += FreqPerDiv;
	}
}

//////////////////////////////////////////////////////////////////////
// Helper functions to convert to/from screen coordinates to frequency
//////////////////////////////////////////////////////////////////////
int CPlotter::XfromFreq(qint64 freq)
{
float w = m_OverlayPixmap.width();
float StartFreq = (float)m_CenterFreq - (float)m_Span/2.;
int x = (int) w * ((float)freq - StartFreq)/(float)m_Span;
	if(x<0 )
		return 0;
	if(x>(int)w)
		return m_OverlayPixmap.width();
	return x;
}

qint64 CPlotter::FreqfromX(int x)
{
float w = m_OverlayPixmap.width();
float StartFreq = (float)m_CenterFreq - (float)m_Span/2.;
qint64 f = (int)(StartFreq + (float)m_Span * (float)x/(float)w );
	return f;
}

qint64 CPlotter::DeltaFreqfromX(int x)
{
float w = m_OverlayPixmap.width();
qint64 df = (int)( (float)m_Span * (float)x/(float)w  - (float)m_Span/2.0);
	return df;
}

//////////////////////////////////////////////////////////////////////
// Helper function to round frequency to click resolution value
//////////////////////////////////////////////////////////////////////
qint64 CPlotter::RoundFreq(qint64 freq, int resolution)
{
qint64 delta = resolution;
qint64 delta_2 = delta/2;
	if(freq>=0)
		return ( freq - (freq+delta_2)%delta + delta_2);
	else
		return ( freq - (freq+delta_2)%delta - delta_2);
}

//////////////////////////////////////////////////////////////////////
// Helper function clamps demod freqeuency limits of
// m_CenterFreq
//////////////////////////////////////////////////////////////////////
void CPlotter::ClampDemodParameters()
{

	if(m_DemodLowCutFreq < m_FLowCmin)
		m_DemodLowCutFreq = m_FLowCmin;
	if(m_DemodLowCutFreq > m_FLowCmax)
		m_DemodLowCutFreq = m_FLowCmax;

	if(m_DemodHiCutFreq < m_FHiCmin)
		m_DemodHiCutFreq = m_FHiCmin;
	if(m_DemodHiCutFreq > m_FHiCmax)
		m_DemodHiCutFreq = m_FHiCmax;

}

//////////////////////////////////////////////////////////////////////
// Helper function sets demod filter parameters
//////////////////////////////////////////////////////////////////////
void CPlotter::SetDemodRanges(int FLowCmin, int FLowCmax, int FHiCmin, int FHiCmax, bool symetric)
{
	m_FLowCmin=FLowCmin;
	m_FLowCmax=FLowCmax;
	m_FHiCmin=FHiCmin;
	m_FHiCmax=FHiCmax;
	m_symetric=symetric;
	ClampDemodParameters();
}
