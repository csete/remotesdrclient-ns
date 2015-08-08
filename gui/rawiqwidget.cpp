#include "rawiqwidget.h"
#include "ui_rawiqwidget.h"
#include <QPainter>
#include <QDebug>

CRawIQWidget* g_pRawIQWidget = NULL;		//pointer to this class is global so everybody can access


CRawIQWidget::CRawIQWidget(QWidget *parent, Qt::WindowFlags f) :
	QDialog(parent,f),
	ui(new Ui::CRawIQWidget)
{
	ui->setupUi(this);
	setWindowTitle("Raw I/Q Scatter Plot");
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_PaintOnScreen,false);
	setAutoFillBackground(false);
	setAttribute(Qt::WA_OpaquePaintEvent, false);
	setAttribute(Qt::WA_NoSystemBackground, true);

	connect(this, SIGNAL(DrawData( ) ), this,  SLOT( OnDrawData() ) );

	m_2DPixmap = QPixmap(0,0);
	m_OverlayPixmap = QPixmap(10,10);
	m_Size = QSize(10,10);
	for(int i=0; i<MAX_IQ_POINTS; i++)
		m_PointBuf[i] = QPointF(0.0,0.0);
	m_NumPoints = 1;
}

CRawIQWidget::~CRawIQWidget()
{
	delete ui;
}

//////////////////////////////////////////////////////////////////////
// Sizing interface
//////////////////////////////////////////////////////////////////////
QSize CRawIQWidget::minimumSizeHint() const
{
	return QSize(200, 200);
}

QSize CRawIQWidget::sizeHint() const
{
	return QSize(200, 200);
}

//////////////////////////////////////////////////////////////////////
// Called when screen size changes so must recalculate bitmaps
//////////////////////////////////////////////////////////////////////
void CRawIQWidget::resizeEvent(QResizeEvent* )
{
	if(!size().isValid())
		return;
	if( m_Size != size() )
	{	//if changed, resize pixmaps to new screensize
		m_Size = size();
		m_OverlayPixmap = QPixmap(m_Size.width(), m_Size.height());
		m_OverlayPixmap.fill(Qt::black);
		m_2DPixmap = QPixmap(m_Size.width(), m_Size.height());
		m_2DPixmap.fill(Qt::black);
	}
	DrawOverlay();
}

//////////////////////////////////////////////////////////////////////
// Called by QT when screen needs to be redrawn
//////////////////////////////////////////////////////////////////////
void CRawIQWidget::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.drawPixmap(0,0,m_2DPixmap);
	return;
}

//////////////////////////////////////////////////////////////////////
// Slot Called when new data block arrives
//////////////////////////////////////////////////////////////////////
void CRawIQWidget::ProccessRawIQData(qint8* pBuf, int NumBytes)
{
	if(NumBytes > (MAX_IQ_POINTS*2) )
		return;
	for(int i=0; i<NumBytes; i++)
		m_RawBuf[i] = pBuf[i];	//just copy bytes since this is called from worker thread
	m_NumPoints = NumBytes/2;
	emit DrawData();
}

//////////////////////////////////////////////////////////////////////
// Called to update the screen with data from m_PointBuf
//////////////////////////////////////////////////////////////////////
void CRawIQWidget::OnDrawData()
{
int w;
int h;
	//draw the 2D plot
	w = m_2DPixmap.width();
	h = m_2DPixmap.height();
	//first copy into 2Dbitmap the overlay bitmap.
	m_2DPixmap = m_OverlayPixmap.copy(0,0,w,h);

	int j=0;
	for(int i=0; i<m_NumPoints; i++)
	{
		qreal x = (qreal)( (int)m_RawBuf[j++] + 128 ) + (qreal)rand()/RAND_MAX;
		x = ((qreal)w*x)/255.0;
		qreal y = (qreal)( (int)m_RawBuf[j++] + 128 ) + (qreal)rand()/RAND_MAX;
		y = ((qreal)h*y)/256;
		m_PointBuf[i].setX( x );
		m_PointBuf[i].setY( y );
	}

	QPainter painter(&m_2DPixmap);

// workaround for "fixed" line drawing since Qt 5
// see http://stackoverflow.com/questions/16990326
#if QT_VERSION >= 0x050000
	painter.translate(0.5, 0.5);
#endif

	painter.setPen( Qt::yellow );
#if 0
	for(int i=0; i<m_NumPoints; i++)
		painter.drawPoint( m_PointBuf[i]);
#else
	painter.drawPolyline(m_PointBuf,m_NumPoints);
#endif
	//trigger a new paintEvent
	update();
}

//////////////////////////////////////////////////////////////////////
// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every fft data update.
//////////////////////////////////////////////////////////////////////
void CRawIQWidget::DrawOverlay()
{
	if(m_OverlayPixmap.isNull())
		return;
int w = m_OverlayPixmap.width();
int h = m_OverlayPixmap.height();
	QPainter painter(&m_OverlayPixmap);
	painter.initFrom(this);

#if 1
	m_OverlayPixmap.fill(Qt::black);
#else
	//fill background with gradient
	QLinearGradient gradient(0, 0, 0 ,h);
	gradient.setColorAt(1, Qt::black);
	gradient.setColorAt(0, Qt::gray);
//	gradient.setColorAt(0, Qt::darkBlue);
	painter.setBrush(gradient);
	painter.drawRect(0, 0, w, h);
#endif

	int x = w/2;
	int y = h/2;
	painter.setPen(QPen(Qt::white, 1,Qt::DotLine));
	painter.drawLine(x, 0, x , h);
	painter.drawLine(0, y, w , y);
}

