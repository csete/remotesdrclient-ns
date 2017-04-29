/////////////////////////////////////////////////////////////////////////////////////
// freqctrl.cpp  Widget to enter/display frequency data
// History:
//	2013-10-23  Initial creation MSW
//	2013-10-11  fixed comma problem MSW
//////////////////////////////////////////////////////////////////////

//= =========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
// Copyright 2013 Moe Wheatley. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//	  conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//	  of conditions and the following disclaimer in the documentation and/or other materials
//	  provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The views and conclusions contained in the software and documentation are those of the
// authors and should not be interpreted as representing official policies, either expressed
// or implied, of Moe Wheatley.
//= =========================================================================================

/*---------------------------------------------------------------------------*/
/*------------------------> I N C L U D E S <--------------------------------*/
/*---------------------------------------------------------------------------*/
#include "freqctrl.h"
#include <stdlib.h>
#include <QDebug>

/*---------------------------------------------------------------------------*/
/*--------------------> L O C A L   D E F I N E S <--------------------------*/
/*---------------------------------------------------------------------------*/
// Manual adjustment of Font size as percent of control height
#define DIGIT_SIZE_PERCENT 90
#define UNITS_SIZE_PERCENT 40

// adjustment for separation between digits
#define SEPRATIO_N 100         // separation rectangle size ratio numerator times 100
#define SEPRATIO_D 3           // separation rectangle size ratio denominator

/////////////////////////////////////////////////////////////////////
// Constructor/Destructor
/////////////////////////////////////////////////////////////////////
CFreqCtrl::CFreqCtrl(QWidget *parent) :
    QFrame(parent)
{
    // setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    m_BkColor = Qt::black;
    m_DigitColor = Qt::white;
    m_HighlightColor = Qt::gray;
    m_UnitsColor = Qt::gray;
    m_freq = 15000000;
    Setup(8, 1, 150000000U, 1, UNITS_MHZ);
    m_Oldfreq = 0;
    m_LastLeadZeroPos = 0;
    m_ActiveEditDigit = -1;
    m_LRMouseFreqSel = false;
    m_UnitsFont = QFont("Arial", 8, QFont::Normal);
    m_DigitFont = QFont("Arial", 8, QFont::Normal);
}

CFreqCtrl::~CFreqCtrl()
{
}

/////////////////////////////////////////////////////////////////////
//  Size hint stuff
/////////////////////////////////////////////////////////////////////
QSize CFreqCtrl::minimumSizeHint() const
{
    return QSize(200, 30);
}

QSize CFreqCtrl::sizeHint() const
{
    return QSize(200, 30);
}

/////////////////////////////////////////////////////////////////////
//  Various helper functions
/////////////////////////////////////////////////////////////////////
bool CFreqCtrl::InRect(QRect &rect, QPoint &point)
{
    if ((point.x() < rect.right()) && (point.x() > rect.x()) &&
        (point.y() < rect.bottom()) && (point.y() > rect.y()))
        return true;
    else
        return false;
}

//////////////////////////////////////////////////////////////////////////////
//  Setup various parameters for the control
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::Setup(int NumDigits, qint64 Minf, qint64 Maxf, int MinStep,
                      FUNITS UnitsType)
{
    int       i;
    qint64    pwr = 1;
    m_LastEditDigit = 0;
    m_Oldfreq = -1;
    m_NumDigits = NumDigits;
    if (m_NumDigits > MAX_DIGITS)
        m_NumDigits = MAX_DIGITS;
    if (m_NumDigits < MIN_DIGITS)
        m_NumDigits = MIN_DIGITS;
    m_UnitString = "";
    m_MinStep = MinStep;
    if (m_MinStep == 0)
        m_MinStep = 1;
    m_MinFreq = Minf;
    m_MaxFreq = Maxf;
    if (m_freq < m_MinFreq)
        m_freq = m_MinFreq;
    if (m_freq > m_MaxFreq)
        m_freq = m_MaxFreq;

    for (i = 0; i < m_NumDigits; i++)
    {
        m_DigitInfo[i].weight = pwr;
        m_DigitInfo[i].incval = pwr;
        m_DigitInfo[i].modified = true;
        m_DigitInfo[i].editmode = false;
        m_DigitInfo[i].val = 0;
        pwr *= 10;
    }
    if (m_MaxFreq > pwr)
        m_MaxFreq = pwr - 1;
    m_MaxFreq = m_MaxFreq - m_MaxFreq % m_MinStep;
    if (m_MinFreq > pwr)
        m_MinFreq = 1;
    m_MinFreq = m_MinFreq - m_MinFreq % m_MinStep;
    m_DigStart = 0;
    SetUnits(UnitsType);
    for (i = m_NumDigits - 1; i >= 0; i--)
    {
        if (m_DigitInfo[i].weight <= m_MinStep)
        {
            if (m_DigStart == 0)
            {
                m_DigitInfo[i].incval = m_MinStep;
                m_DigStart = i;
            }
            else
            {
                if ((m_MinStep % m_DigitInfo[i + 1].weight) != 0)
                    m_DigStart = i;
                m_DigitInfo[i].incval = 0;
            }
        }
    }
    m_NumSeps = (m_NumDigits - 1) / 3 - m_DigStart / 3;
}

//////////////////////////////////////////////////////////////////////////////
//  Sets the frequency and individual digit values
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::SetFrequency(qint64 freq)
{
    int       i;
    qint64    acc = 0;
    qint64    rem;
    int       val;
    if (freq == m_Oldfreq)
        return;
    if (freq < m_MinFreq)
        freq = m_MinFreq;
    if (freq > m_MaxFreq)
        freq = m_MaxFreq;
    m_freq = freq - freq % m_MinStep;
    rem = m_freq;
    m_LeadZeroPos = m_NumDigits;
    for (i = m_NumDigits - 1; i >= m_DigStart; i--)
    {
        val = (int)(rem / m_DigitInfo[i].weight);
        if (m_DigitInfo[i].val != val)
        {
            m_DigitInfo[i].val = val;
            m_DigitInfo[i].modified = true;
        }
        rem = rem - val * m_DigitInfo[i].weight;
        acc += val;
        if ((acc == 0) && (i > m_DecPos))
            m_LeadZeroPos = i;
    }
// qDebug()<<"FreqControl = " <<m_freq;
    // signal the new frequency to world
    emit    NewFrequency(m_freq);
    m_Oldfreq = m_freq;
    UpdateCtrl(m_LastLeadZeroPos != m_LeadZeroPos);
    m_LastLeadZeroPos = m_LeadZeroPos;
}

//////////////////////////////////////////////////////////////////////////////
//  Sets the Digit and comma and decimal pt color
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::SetDigitColor(QColor cr)
{
    m_UpdateAll = true;
    m_DigitColor = cr;
    for (int i = m_DigStart; i < m_NumDigits; i++)
        m_DigitInfo[i].modified = true;
    UpdateCtrl(true);
}

//////////////////////////////////////////////////////////////////////////////
//  Sets the Digit units
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::SetUnits(FUNITS units)
{
    switch (units)
    {
    case UNITS_HZ:
        m_DecPos = 0;
        m_UnitString = "Hz ";
        break;
    case UNITS_KHZ:
        m_DecPos = 3;
        m_UnitString = "KHz";
        break;
    case UNITS_MHZ:
        m_DecPos = 6;
        m_UnitString = "MHz";
        break;
    case UNITS_GHZ:
        m_DecPos = 9;
        m_UnitString = "GHz";
        break;
    case UNITS_SEC:
        m_DecPos = 6;
        m_UnitString = "Sec";
        break;
    case UNITS_MSEC:
        m_DecPos = 3;
        m_UnitString = "mS ";
        break;
    case UNITS_USEC:
        m_DecPos = 0;
        m_UnitString = "uS ";
        break;
    case UNITS_NSEC:
        m_DecPos = 0;
        m_UnitString = "nS ";
        break;
    }
    m_UpdateAll = true;
    UpdateCtrl(true);
}
//////////////////////////////////////////////////////////////////////////////
//  Sets the Background color
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::SetBkColor(QColor cr)
{
    m_UpdateAll = true;
    m_BkColor = cr;
    for (int i = m_DigStart; i < m_NumDigits; i++)
        m_DigitInfo[i].modified = true;
    UpdateCtrl(true);
}

//////////////////////////////////////////////////////////////////////////////
//  Sets the Units text color
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::SetUnitsColor(QColor cr)
{
    m_UpdateAll = true;
    m_UnitsColor = cr;
    UpdateCtrl(true);
}

//////////////////////////////////////////////////////////////////////////////
//  Sets the Mouse edit selection text background color
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::SetHighlightColor(QColor cr)
{
    m_UpdateAll = true;
    m_HighlightColor = cr;
    UpdateCtrl(true);
}


//////////////////////////////////////////////////////////////////////////////
//  call to force control to redraw
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::UpdateCtrl(bool all)
{
    if (all)
    {
        m_UpdateAll = true;
        for (int i = m_DigStart; i < m_NumDigits; i++)
            m_DigitInfo[i].modified = true;
    }
    update();
}


/////////////////////////////////////////////////////////////////////
//  Various Event overrides
/////////////////////////////////////////////////////////////////////
void CFreqCtrl::resizeEvent(QResizeEvent *)
{
// qDebug() <<rect.width() << rect.height();
    m_Pixmap = QPixmap(size()); // resize pixmap to current control size
    m_Pixmap.fill(m_BkColor);
    m_UpdateAll = true;
}

void CFreqCtrl::leaveEvent(QEvent *)
{                              // called when mouse cursor leaves this control so deactivate any highlights
    if (m_ActiveEditDigit >= 0)
    {
        if (m_DigitInfo[m_ActiveEditDigit].editmode)
        {
            m_DigitInfo[m_ActiveEditDigit].editmode = false;
            m_DigitInfo[m_ActiveEditDigit].modified = true;
            m_ActiveEditDigit = -1;
            UpdateCtrl(false);
        }
    }
}

/////////////////////////////////////////////////////////////////////
//  main draw event for this control
/////////////////////////////////////////////////////////////////////
void CFreqCtrl::paintEvent(QPaintEvent *)
{
    QPainter    painter(&m_Pixmap);
    if (m_UpdateAll)           // if need to redraw everything
    {
        DrawBkGround(painter);
        m_UpdateAll = false;
    }
    // draw any modified digits to the m_MemDC
    DrawDigits(painter);
    // now draw pixmap onto screen
    QPainter    scrnpainter(this);
    scrnpainter.drawPixmap(0, 0, m_Pixmap); // blt to the screen(flickers like a candle, why?)
}

/////////////////////////////////////////////////////////////////////
//  Mouse Event overrides
/////////////////////////////////////////////////////////////////////
void CFreqCtrl::mouseMoveEvent(QMouseEvent *event)
{
    QPoint    pt = event->pos();
    // find which digit is to be edited
    if (isActiveWindow())
    {
        if (!hasFocus())
            setFocus(Qt::MouseFocusReason);
        for (int i = m_DigStart; i < m_NumDigits; i++)
        {
            if (InRect(m_DigitInfo[i].dQRect, pt))
            {
                if (!m_DigitInfo[i].editmode)
                {
                    m_DigitInfo[i].editmode = true;
                    m_ActiveEditDigit = i;
                }
            }
            else
            {                  // un-highlight the previous digit if moved off it
                if (m_DigitInfo[i].editmode)
                {
                    m_DigitInfo[i].editmode = false;
                    m_DigitInfo[i].modified = true;
                }
            }
        }
        UpdateCtrl(false);
    }
}

//////////////////////////////////////////////////////////////////////////////
//  Service mouse button clicks to inc or dec the selected frequency
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::mousePressEvent(QMouseEvent *event)
{
    QPoint    pt = event->pos();
    if (event->button() == Qt::LeftButton)
    {
        for (int i = m_DigStart; i < m_NumDigits; i++)
        {
            if (InRect(m_DigitInfo[i].dQRect, pt)) // if in i'th digit
            {
                if (m_LRMouseFreqSel)
                {
                    DecFreq();
                }
                else
                {
                    if (pt.y() < m_DigitInfo[i].dQRect.bottom() / 2) // top half?
                        IncFreq();
                    else
                        DecFreq();                                   // bottom half
                }
            }
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        for (int i = m_DigStart; i < m_NumDigits; i++)
        {
            if (InRect(m_DigitInfo[i].dQRect, pt)) // if in i'th digit
            {
                if (m_LRMouseFreqSel)
                {
                    IncFreq();
                }
                else
                {
                    if (pt.y() < m_DigitInfo[i].dQRect.bottom() / 2) // top half?
                        IncDigit();
                    else
                        DecDigit();                                  // botom half
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////
//  Mouse Wheel Event overrides
/////////////////////////////////////////////////////////////////////
void CFreqCtrl::wheelEvent(QWheelEvent *event)
{
    QPoint    pt = event->pos();
    int       numDegrees = event->delta() / 8;
    int       numSteps = numDegrees / 15;

    for (int i = m_DigStart; i < m_NumDigits; i++)
    {
        if (InRect(m_DigitInfo[i].dQRect, pt)) // if in i'th digit
        {
            if (numSteps > 0)
                IncFreq();
            else if (numSteps < 0)
                DecFreq();
        }
    }
}


/////////////////////////////////////////////////////////////////////
//  Keyboard Event overrides
/////////////////////////////////////////////////////////////////////
void CFreqCtrl::keyPressEvent(QKeyEvent *event)
{                              // call base class if dont over ride key
    bool      fSkipMsg = false;
    qint64    tmp;
// qDebug() <<event->key();
    switch (event->key())
    {
    case Qt::Key_0:
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9:
        if (m_ActiveEditDigit >= 0)
        {
            if (m_DigitInfo[m_ActiveEditDigit].editmode)
            {
                tmp = (m_freq / m_DigitInfo[m_ActiveEditDigit].weight) % 10;
                m_freq -= tmp * m_DigitInfo[m_ActiveEditDigit].weight;
                m_freq = m_freq + (event->key() - '0') *
                         m_DigitInfo[m_ActiveEditDigit].weight;
                SetFrequency(m_freq);
            }
        }
        MoveCursorRight();
        fSkipMsg = true;
        break;
    case Qt::Key_Left:
        if (m_ActiveEditDigit != -1)
        {
            MoveCursorLeft();
            fSkipMsg = true;
        }
        break;
    case Qt::Key_Up:
        if (m_ActiveEditDigit != -1)
        {
            IncFreq();
            fSkipMsg = true;
        }
        break;
    case Qt::Key_Down:
        if (m_ActiveEditDigit != -1)
        {
            DecFreq();
            fSkipMsg = true;
        }
        break;
    case Qt::Key_Right:
        if (m_ActiveEditDigit != -1)
        {
            MoveCursorRight();
            fSkipMsg = true;
        }
        break;
    case Qt::Key_Home:
        CursorHome();
        fSkipMsg = true;
        break;
    case Qt::Key_End:
        CursorEnd();
        fSkipMsg = true;
        break;
    default:
        break;
    }
    if (!fSkipMsg)
        QFrame::keyPressEvent(event);
}


//////////////////////////////////////////////////////////////////////////////
//  Calculates all the rectangles for the digits, separators, and units text
//    and creates the fonts for them.
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::DrawBkGround(QPainter &Painter)
{
    QRect    rect(0, 0, width(), height());
// qDebug() <<rect;
    int      cellwidth = 100 * rect.width() /
                         (100 * (m_NumDigits + 2) + (m_NumSeps * SEPRATIO_N) /
                          SEPRATIO_D);
    int    sepwidth = (SEPRATIO_N * cellwidth) / (100 * SEPRATIO_D);
// qDebug() <<cellwidth <<sepwidth;

    m_UnitsRect.setRect(rect.right() - 2 * cellwidth,
                        rect.top(),
                        2 * cellwidth,
                        rect.height());
// draw units text
    m_UnitsFont.setPixelSize((UNITS_SIZE_PERCENT * rect.height()) / 100);
    m_UnitsFont.setFamily("Arial");
    Painter.setFont(m_UnitsFont);
    Painter.setPen(m_UnitsColor);
    Painter.drawText(m_UnitsRect, Qt::AlignHCenter | Qt::AlignVCenter,
                     m_UnitString);


    m_DigitFont.setPixelSize((DIGIT_SIZE_PERCENT * rect.height()) / 100);
    m_DigitFont.setFamily("Arial");
    Painter.setFont(m_DigitFont);
    Painter.setPen(m_DigitColor);

    int    digpos = rect.right() - 2 * cellwidth - 1; // starting digit x position
    for (int i = m_DigStart; i < m_NumDigits; i++)
    {
        if ((i > m_DigStart) && ((i % 3) == 0))
        {
            m_SepRect[i].setCoords(digpos - sepwidth,
                                   rect.top(),
                                   digpos,
                                   rect.bottom());
            digpos -= sepwidth;
            Painter.fillRect(m_SepRect[i], m_BkColor);
            if (i == m_DecPos)
                Painter.drawText(m_SepRect[i],
                                 Qt::AlignHCenter | Qt::AlignVCenter, ".");
            else if (i > m_DecPos)
            {
                if (m_LeadZeroPos > i)
                    Painter.drawText(m_SepRect[i],
                                     Qt::AlignHCenter | Qt::AlignVCenter, ",");
                else
                    Painter.drawText(m_SepRect[i],
                                     Qt::AlignHCenter | Qt::AlignVCenter, " ");
            }
        }
        else
        {
            m_SepRect[i].setCoords(0, 0, 0, 0);
        }
        m_DigitInfo[i].dQRect.setCoords(digpos - cellwidth,
                                        rect.top(),
                                        digpos,
                                        rect.bottom());
        digpos -= cellwidth;
    }
}

//////////////////////////////////////////////////////////////////////////////
//  Draws just the Digits that have been modified
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::DrawDigits(QPainter &Painter)
{
    Painter.setFont(m_DigitFont);
    m_FirstEditableDigit = m_DigStart;
// qDebug() <<m_DigStart <<m_LeadZeroPos;
    for (int i = m_DigStart; i < m_NumDigits; i++)
    {
        if (m_DigitInfo[i].incval == 0)
            m_FirstEditableDigit++;
        if (m_DigitInfo[i].modified || m_DigitInfo[i].editmode)
        {
            if (m_DigitInfo[i].editmode && m_DigitInfo[i].incval != 0)
                Painter.fillRect(m_DigitInfo[i].dQRect, m_HighlightColor);
            else
                Painter.fillRect(m_DigitInfo[i].dQRect, m_BkColor);
            if (i >= m_LeadZeroPos)
                Painter.setPen(m_BkColor);
            else
                Painter.setPen(m_DigitColor);
            Painter.drawText(m_DigitInfo[i].dQRect,
                             Qt::AlignHCenter | Qt::AlignVCenter,
                             QString().number(m_DigitInfo[i].val));
            m_DigitInfo[i].modified = false;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//  Increment just the digit active in edit mode
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::IncDigit()
{
    int       tmp;
    qint64    tmpl;
    if (m_ActiveEditDigit >= 0)
    {
        if (m_DigitInfo[m_ActiveEditDigit].editmode)
        {
            if (m_DigitInfo[m_ActiveEditDigit].weight ==
                m_DigitInfo[m_ActiveEditDigit].incval)
            {
                // get the current digit value
                tmp =
                    (int)((m_freq / m_DigitInfo[m_ActiveEditDigit].weight) %
                          10);
                // set the current digit value to zero
                m_freq -= tmp * m_DigitInfo[m_ActiveEditDigit].weight;
                tmp++;
                if (tmp > 9)
                    tmp = 0;
                m_freq = m_freq + (qint64)tmp *
                         m_DigitInfo[m_ActiveEditDigit].weight;
            }
            else
            {
                tmp =
                    (int)((m_freq / m_DigitInfo[m_ActiveEditDigit + 1].weight) %
                          10);
                tmpl = m_freq + m_DigitInfo[m_ActiveEditDigit].incval;
                if (tmp !=
                    (int)((tmpl / m_DigitInfo[m_ActiveEditDigit + 1].weight) %
                          10))
                {
                    tmpl -= m_DigitInfo[m_ActiveEditDigit + 1].weight;
                }
                m_freq = tmpl;
            }
            SetFrequency(m_freq);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//  Increment the frequency by this digit active in edit mode
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::IncFreq()
{
    if (m_ActiveEditDigit >= 0)
    {
        if (m_DigitInfo[m_ActiveEditDigit].editmode)
        {
            m_freq += m_DigitInfo[m_ActiveEditDigit].incval;
            m_freq = m_freq - m_freq % m_DigitInfo[m_ActiveEditDigit].weight;
            SetFrequency(m_freq);
            m_LastEditDigit = m_ActiveEditDigit;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//  Decrement the digit active in edit mode
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::DecDigit()
{
    int       tmp;
    qint64    tmpl;
    if (m_ActiveEditDigit >= 0)
    {
        if (m_DigitInfo[m_ActiveEditDigit].editmode)
        {
            if (m_DigitInfo[m_ActiveEditDigit].weight ==
                m_DigitInfo[m_ActiveEditDigit].incval)
            {
                // get the current digit value
                tmp =
                    (int)((m_freq / m_DigitInfo[m_ActiveEditDigit].weight) %
                          10);
                // set the current digit value to zero
                m_freq -= tmp * m_DigitInfo[m_ActiveEditDigit].weight;
                tmp--;
                if (tmp < 0)
                    tmp = 9;
                m_freq = m_freq + (qint64)tmp *
                         m_DigitInfo[m_ActiveEditDigit].weight;
            }
            else
            {
                tmp =
                    (int)((m_freq / m_DigitInfo[m_ActiveEditDigit + 1].weight) %
                          10);
                tmpl = m_freq - m_DigitInfo[m_ActiveEditDigit].incval;
                if (tmp !=
                    (int)((tmpl / m_DigitInfo[m_ActiveEditDigit + 1].weight) %
                          10))
                {
                    tmpl += m_DigitInfo[m_ActiveEditDigit + 1].weight;
                }
                m_freq = tmpl;
            }
            SetFrequency(m_freq);
        }
    }
}
//////////////////////////////////////////////////////////////////////////////
//  Decrement the frequency by this digit active in edit mode
//////////////////////////////////////////////////////////////////////////////
void CFreqCtrl::DecFreq()
{
    if (m_ActiveEditDigit >= 0)
    {
        if (m_DigitInfo[m_ActiveEditDigit].editmode)
        {
            m_freq -= m_DigitInfo[m_ActiveEditDigit].incval;
            m_freq = m_freq - m_freq % m_DigitInfo[m_ActiveEditDigit].weight;
            SetFrequency(m_freq);
            m_LastEditDigit = m_ActiveEditDigit;
        }
    }
}

/////////////////////////////////////////////////////////////////////
//  Cursor move routines for arrow key editing
/////////////////////////////////////////////////////////////////////
void CFreqCtrl::MoveCursorLeft()
{
    QPoint    pt;
    if ((m_ActiveEditDigit >= 0) && (m_ActiveEditDigit < m_NumDigits - 1))
    {
        cursor().setPos(mapToGlobal(m_DigitInfo[++m_ActiveEditDigit].dQRect.
                                    center()));
    }
}

void CFreqCtrl::MoveCursorRight()
{
    QPoint    pt;
    if (m_ActiveEditDigit > m_FirstEditableDigit)
    {
        cursor().setPos(mapToGlobal(m_DigitInfo[--m_ActiveEditDigit].dQRect.
                                    center()));
    }
}

void CFreqCtrl::CursorHome()
{
    QPoint    pt;
    if (m_ActiveEditDigit >= 0)
    {
        cursor().setPos(mapToGlobal(
                            m_DigitInfo[m_NumDigits - 1].dQRect.center()));
    }
}

void CFreqCtrl::CursorEnd()
{
    QPoint    pt;
    if (m_ActiveEditDigit > 0)
    {
        cursor().setPos(mapToGlobal(m_DigitInfo[m_FirstEditableDigit].dQRect.
                                    center()));
    }
}
