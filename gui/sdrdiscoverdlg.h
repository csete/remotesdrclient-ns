//////////////////////////////////////////////////////////////////////
// sdrdiscoverdlg.h: interface for the CSdrDiscoverDlg class.
//
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////
#ifndef SDRDISCOVERDLG_H
#define SDRDISCOVERDLG_H
#include "ui_sdrdiscoverdlg.h"

#include <QDialog>
#include <QTcpSocket>
#include <QTimer>
#include <QListWidget>
#include <QTableWidget>
#include <QKeyEvent>
#include<QHostAddress>

#define MAX_SDRS 200

typedef struct  _DISCOVER_PARAMS
{
	QString DomainName;
	QHostAddress IPAdr;
	quint16 Port;
	bool NeedPW;
	bool InUse;
}tDiscover_Params;


namespace Ui {
    class CSdrDiscoverDlg;
}

class CSdrDiscoverDlg : public QDialog
{
    Q_OBJECT

public:
	explicit CSdrDiscoverDlg(QWidget *parent);
    ~CSdrDiscoverDlg();

	void InitDlg();
	void SetListServer(QString Url){m_ListServerUrl = Url;}
	void SetListServerActionPath(QString path){m_ListServerActionPath = path;}

	QHostAddress m_IPAdr;
	unsigned short m_Port;
	QString m_DomainName;
	bool m_NeedPW;

public slots:
	void accept();

private slots:
	void OnFind();
	void OnTableItemDoubleClick( QTableWidgetItem * item );
	void SendDiscoverRequest();
	void TcpCloseTimeout();
	void ReadTcpClientData();
	void TcpClientStateChanged(QAbstractSocket::SocketState State);
	void OnTcpClientError(QAbstractSocket::SocketError err);
	void OnTcpClientDisconnected();
	void OnTcpClientConnected();

private:
    Ui::CSdrDiscoverDlg *ui;
	void PutEntryInTable(QStringList entry);
	void SendReqToListServer();
	QTcpSocket* m_pTcpClient;
	QStringList m_TableHeader;
	QString m_ListServerUrl;
	QString m_ListServerActionPath;
	QString m_Str;
	tDiscover_Params m_ParamList[MAX_SDRS];
};

#endif // SDRDISCOVERDLG_H
