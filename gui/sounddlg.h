//////////////////////////////////////////////////////////////////////
// sounddlg.h: interface for the CSoundDlg class.
//
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////
#ifndef SOUNDDLG_H
#define SOUNDDLG_H

#include <QDialog>
#include "ui_sounddlg.h"


class CSoundDlg : public QDialog
{
	Q_OBJECT
public:
	CSoundDlg(QWidget *parent=0);
	~CSoundDlg();
	void SetInputIndex(int x){if(x>m_MaxInIndex) x=m_MaxInIndex; ui.comboBoxSndIn->setCurrentIndex(x); }
	void SetOutputIndex(int x){if(x>m_MaxOutIndex) x=m_MaxOutIndex; ui.comboBoxSndOut->setCurrentIndex(x); }
	int GetInputIndex(){return ui.comboBoxSndIn->currentIndex(); }
	int GetOutputIndex(){return ui.comboBoxSndOut->currentIndex(); }

public slots:

signals:

private slots:

private:
	Ui::DialogSndCard ui;
	int m_MaxInIndex;
	int m_MaxOutIndex;
};

#endif // SOUNDDLG_H
