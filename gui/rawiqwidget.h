#ifndef RAWIQWIDGET_H
#define RAWIQWIDGET_H

#include <QDialog>
#include <QTimer>

#define MAX_IQ_POINTS 1024


namespace Ui {
class CRawIQWidget;
}

class CRawIQWidget : public QDialog
{
	Q_OBJECT

public:
	explicit CRawIQWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
	~CRawIQWidget();
	void ProccessRawIQData(qint8* pBuf, int NumBytes);

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

protected:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent* event);

private slots:
	void OnDrawData();

signals:
	void DrawData();

private:
	void DrawOverlay();
	Ui::CRawIQWidget *ui;
	qint8 m_RawBuf[MAX_IQ_POINTS*2];
	QPixmap m_2DPixmap;
	QPixmap m_OverlayPixmap;
	QPointF m_PointBuf[MAX_IQ_POINTS];
	QSize m_Size;
	int m_NumPoints;

};

extern CRawIQWidget* g_pRawIQWidget;		//pointer to this class is global so everybody can access


#endif // RAWIQWIDGET_H
