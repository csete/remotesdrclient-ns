//////////////////////////////////////////////////////////////////////
// editnetdlg.h: interface for the CEditNetDlg class.
//
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////
#ifndef EDITNETDLG_H
#define EDITNETDLG_H

#include <QDialog>
#include "gui/mainwindow.h"
#include "ui_editnetdlg.h"


namespace Ui {
class CEditNetDlg;
}

class CEditNetDlg : public QDialog
{
    Q_OBJECT
public:
	explicit CEditNetDlg(QWidget *parent = 0);
	~CEditNetDlg();

	void InitDlg();
	bool eventFilter(QObject* o, QEvent* e);

	void SetClientDesc(QString Txt)
	{
		ui->lineEditClientDesc->setText(Txt);
	}
	void GetClientDesc(QString &Txt)
	{
		Txt = ui->lineEditClientDesc->text();
	}

	void SetListServer(QString Url)
	{
		ui->lineEditListServerUrl->setText(Url);
	}
	void GetListServer(QString &Url)
	{
		Url = ui->lineEditListServerUrl->text();
	}

	void SetListServerActionPath(QString path)
	{
		ui->lineEditListServerActionPath->setText(path);
	}
	void GetListServerActionPath(QString &path)
	{
		path = ui->lineEditListServerActionPath->text();
	}

	void SetPasswords(QString rxpw, QString txpw)
	{
		ui->lineEditRxPw->setText(rxpw);
		ui->lineEditTxPw->setText(txpw);
	}
	void GetPasswords(QString &rxpw, QString &txpw)
	{
		rxpw = ui->lineEditRxPw->text();
		txpw = ui->lineEditTxPw->text();
	}

	QHostAddress m_IPAdr;
	QString m_DomainName;
	quint32 m_Port;

signals:

public slots:
	void accept();
    void FindSdrs();

private:
	Ui::CEditNetDlg *ui;
	QIntValidator* m_pPortAddressValidator;
};

#endif // EDITNETDLG_H
