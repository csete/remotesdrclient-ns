//////////////////////////////////////////////////////////////////////
// meter.cpp: implementation of the CMeter class.
//
//  This class creates and draws an S meter widget
//
// History:
//	2013-10-02  Initial creation MSW
//	2013-10-13  Added Squelch threshold marker
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
#include "gui/meter.h"
#include <stdlib.h>
#include <QDebug>

//////////////////////////////////////////////////////////////////////
// Local defines
//////////////////////////////////////////////////////////////////////
//ratio to total control width or height
#define CTRL_MARGIN 0.07		//left/right margin
#define CTRL_MAJOR_START 0.4	//top of major tic line
#define CTRL_MINOR_START 0.5	//top of minor tic line
#define CTRL_XAXIS_HEGHT 0.6	//vert position of horizontal axis
#define CTRL_NEEDLE_TOP 0.5		//vert position of top of needle triangle

#define MIN_DBM -121.0		//S1
#define MAX_DBM -13.0		//S9+60

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CMeter::CMeter(QWidget *parent) :
	QFrame(parent)
{
	m_Overload = false;
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //setFocusPolicy(Qt::StrongFocus);
    //setAttribute(Qt::WA_PaintOnScreen,false);
    //setAutoFillBackground(false);
    //setAttribute(Qt::WA_OpaquePaintEvent, false);
    //setAttribute(Qt::WA_NoSystemBackground, true);
    //setMouseTracking(true);

	m_Pixmap = QPixmap(0,0);
	m_OverlayPixmap = QPixmap(0,0);
	m_Size = QSize(0,0);
	m_Slevel = 0;
	m_dBm = -120;
	m_SquelchPos = 0;
}

CMeter::~CMeter()
{
}

//////////////////////////////////////////////////////////////////////
// Sizing interface
//////////////////////////////////////////////////////////////////////
QSize CMeter::minimumSizeHint() const
{
    return QSize(35, 10);
}

QSize CMeter::sizeHint() const
{
	return QSize(100, 30);
}


//////////////////////////////////////////////////////////////////////
// Called when screen size changes so must recalculate bitmaps
//////////////////////////////////////////////////////////////////////
void CMeter::resizeEvent(QResizeEvent* )
{
	if(!size().isValid())
		return;
    if( m_Size != size() )
	{	//if size changed, resize pixmaps to new screensize
		m_Size = size();
		m_OverlayPixmap = QPixmap(m_Size.width(), m_Size.height());
        m_OverlayPixmap.fill(Qt::white);
		m_Pixmap = QPixmap(m_Size.width(), m_Size.height());
        m_Pixmap.fill(Qt::white);
	}
	DrawOverlay();
	draw();
}

//////////////////////////////////////////////////////////////////////
// Slot called to update meter Receiver level position
//////////////////////////////////////////////////////////////////////
void CMeter::SetdBmLevel(double dbm, bool Overload)
{
    double m_dBm_d = m_dBm;

    if (dbm > m_dBm_d)
        m_dBm_d += 0.9 * (dbm - m_dBm_d);
    else
        m_dBm_d += 0.2 * (dbm - m_dBm_d);

    m_dBm = (int) m_dBm_d;
    m_Slevel = CalcPosFromdB(m_dBm_d);
    if (m_Overload != Overload)
	{
		m_Overload = Overload;
		DrawOverlay();
	}
	draw();
}


//////////////////////////////////////////////////////////////////////
// called to calculate meter position from a dB level
//////////////////////////////////////////////////////////////////////
int CMeter::CalcPosFromdB(double db)
{
qreal w = (qreal)m_Pixmap.width();
	w = w - 2.0*CTRL_MARGIN*w;	//width of meter scale in pixels
	if(db<MIN_DBM)
		db = MIN_DBM;
	if(db>MAX_DBM)
		db = MAX_DBM;
	if(db <= -73.0)
	{
		qreal div = w/14.0;
		return (int)( (db/6.0 + 121.0/6.0)*div + .5);
	}
	else
	{
		qreal div = w/14.0;
		return (int)( ( 8 + db/10.0 + 73.0/10.0 )*div + .5);
	}
}

//////////////////////////////////////////////////////////////////////
// called to set Squelch Threshold meter position from a dB level
//////////////////////////////////////////////////////////////////////
void CMeter::SetSquelchPos(double db)
{
	m_SquelchPos = CalcPosFromdB(db);
	DrawOverlay();
	draw();
}


//////////////////////////////////////////////////////////////////////
// Slot called to update transmit audio meter level position
//////////////////////////////////////////////////////////////////////
void CMeter::SetTxLevel(double db)
{
qreal w = (qreal)m_Pixmap.width();
	w = w - 2.0*CTRL_MARGIN*w;	//width of meter scale in pixels
	m_dBm = (int)db;
	if(db<-60)
		db = -60;
	if(db>0)
		db = 0;
	qreal div = w/14.0;
	m_Slevel = (int)(( db*14.0/60.0 + 14.0)*div + .5);
	if(db > -3.0)
	{
		if(!m_Overload)
		{
			m_Overload = true;
			DrawOverlay();
		}
	}
	else
	{
		if(m_Overload)
		{
			m_Overload = false;
			DrawOverlay();
		}
	}
	draw();
}

//////////////////////////////////////////////////////////////////////
// Called by Qt when screen needs to be redrawn
//////////////////////////////////////////////////////////////////////
void CMeter::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.drawPixmap(0,0,m_Pixmap);
	return;
}

//////////////////////////////////////////////////////////////////////
// Called to update meter data display on the screen
//////////////////////////////////////////////////////////////////////
void CMeter::draw()
{
int w;
int h;
	if(m_Pixmap.isNull())
		return;
	//get/draw the 2D spectrum
	w = m_Pixmap.width();
	h = m_Pixmap.height();
	//first copy into 2Dbitmap the overlay bitmap.
	m_Pixmap = m_OverlayPixmap.copy(0,0,w,h);
	QPainter painter(&m_Pixmap);

	//DrawCurrent position indicator
	qreal hline = (qreal)h*CTRL_XAXIS_HEGHT;
	qreal marg = (qreal)w*CTRL_MARGIN;
	qreal ht = (qreal)h*CTRL_NEEDLE_TOP;
	qreal x = marg + m_Slevel;
	QPoint pts[3];
	pts[0].setX(x); pts[0].setY(ht);
	pts[1].setX(x-6); pts[1].setY(hline+6);
	pts[2].setX(x+6); pts[2].setY(hline+6);


	if(m_Overload)
		painter.setBrush(QBrush(Qt::red));
	else
		painter.setBrush(QBrush(Qt::yellow));
	painter.setOpacity(1.0);
	painter.drawPolygon(pts,3);

	//create Font to use for scales
    QFont Font;
	int y = (h)/3;
	Font.setPixelSize(y);
	Font.setWeight(QFont::Normal);
	painter.setFont(Font);

    painter.setPen(QColor(0xFF1F1D1D));
	painter.setOpacity(1.0);
	m_Str.setNum(m_dBm);
	painter.drawText(marg, h-1, m_Str+" dBm" );
	update();
}

//////////////////////////////////////////////////////////////////////
// Called to draw an overlay bitmap containing graphics that
// does not need to be recreated every data update.
//////////////////////////////////////////////////////////////////////
void CMeter::DrawOverlay()
{
	if(m_OverlayPixmap.isNull())
		return;
	int w = m_OverlayPixmap.width();
	int h = m_OverlayPixmap.height();
	int x,y;
	QRect rect;
	QPainter painter(&m_OverlayPixmap);

    // fill background with gradient
#if 0
    QLinearGradient gradient(0, 0, 0 ,h);
	if(m_Overload)
	{
		gradient.setColorAt(1, Qt::white);
		gradient.setColorAt(0, Qt::red);
	}
	else
	{
		gradient.setColorAt(1, Qt::cyan);
		gradient.setColorAt(0, Qt::blue);
	}
    painter.setBrush(gradient);
#endif
    painter.setPen(Qt::white);      // needed to avoid black line on left and upper sides
    if (m_Overload)
        painter.setBrush(QBrush(Qt::darkRed, Qt::SolidPattern));
    else
        painter.setBrush(QBrush(Qt::white, Qt::SolidPattern));
	painter.drawRect(0, 0, w, h);

	//Draw scale lines
	qreal marg = (qreal)w*CTRL_MARGIN;
	qreal hline = (qreal)h*CTRL_XAXIS_HEGHT;
	qreal majstart = (qreal)h*CTRL_MAJOR_START;
	qreal minstart = (qreal)h*CTRL_MINOR_START;
	qreal hstop = (qreal)w-marg;
    painter.setPen(QPen(QColor(0xFF1F1D1D), 1, Qt::SolidLine));
    painter.drawLine(QLineF( marg, hline, hstop, hline));
	qreal xpos = marg;
	for(x=0; x<15; x++)
	{
		if(x&1)	//minor tics
			painter.drawLine( QLineF(xpos, minstart, xpos, hline) );
		else
			painter.drawLine( QLineF(xpos, majstart, xpos, hline) );
		xpos += (hstop-marg)/14.0;
	}


	//draw scale text
	//create Font to use for scales
    QFont Font;
//	QFontMetrics metrics(Font);
	y = h/4;
	Font.setPixelSize(y);
	Font.setWeight(QFont::Normal);
	painter.setFont(Font);
	int rwidth = (int)((hstop-marg)/7.0);
	m_Str = "+60";
	rect.setRect(marg/2, 0, rwidth, majstart);
	for(x=1; x<=9; x+=2)
	{
		m_Str.setNum(x);
		painter.drawText(rect, Qt::AlignHCenter|Qt::AlignVCenter, m_Str);
		rect.translate( rwidth,0);
	}
    painter.setPen(QPen(Qt::darkRed, 1, Qt::SolidLine));
	for(x=20; x<=60; x+=20)
	{
		m_Str = "+" + m_Str.setNum(x);
		painter.drawText(rect, Qt::AlignHCenter|Qt::AlignVCenter, m_Str);
		rect.translate( rwidth,0);
	}
	//Draw Squelch Threshold Position thingy
    painter.setPen(QPen(Qt::darkRed, 2, Qt::SolidLine));
	painter.drawEllipse( marg + m_SquelchPos-1, majstart, 2, 2 );
}

