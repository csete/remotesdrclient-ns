//////////////////////////////////////////////////////////////////////
// netio.h: interface for the CNetio class.
//
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////

#ifndef NETIO_H
#define NETIO_H

#include "threadwrapper.h"
#include "ascpmsg.h"
#include <QTcpServer>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QMutex>


enum eSdrStatus
{
	SDR_OFF,				//TCP not connected
	SDR_CONNECTING,
	SDR_CONNECTED,
	SDR_PWOK,
	SDR_RECEIVING,
	SDR_TRANSMITTING,
	SDR_DISCONNECT_BUSY,
	SDR_DISCONNECT_PWERROR,
	SDR_DISCONNECT_TIMEOUT
};

#define TXQ_SIZE 16384

class CTcp : public CThreadWrapper
{
	Q_OBJECT
public:
	CTcp(QObject *parent = 0);
	~CTcp();
	void PutTxBuf(char* pBuf, int length);

	QHostAddress m_ServerIPAdr;
	QString m_DomainName;
	quint16 m_ServerPort;
	QHostAddress m_ClientIPAdr;
	quint16 m_ClientPort;
	void SendAscpMsg(CAscpTxMsg* pMsg);

signals:
	void NewSdrStatusSig(int status);

private slots:
	void ThreadInit();	//override function is called by new thread when started
	void ThreadExit();	//override function is called by thread before exiting

public slots:
	void ReadTcpDataSlot();
	void TcpStateChangedSlot(QAbstractSocket::SocketState State);
	void ConnectToServerSlot();
	void DisconnectFromServerSlot();
	void SendSlot();

private:
	char m_TxBuf[TXQ_SIZE];
	qint64 m_TxPosition;
	QObject* m_pParent;
	QTcpSocket* m_pTcpClient;
};

///////////////////////////////////////////////////////////////////////////
// CNetIO class which is base class of CSdrInterface class
///////////////////////////////////////////////////////////////////////////
class CNetio : public QObject
{
	Q_OBJECT
public:
	explicit CNetio();
	~CNetio();
	void SetServerParameters(QString DomainName, QHostAddress IPAdr, quint16 Port)
	{
		m_pTcpIo->m_DomainName = DomainName;
		m_pTcpIo->m_ServerIPAdr = IPAdr;
		m_pTcpIo->m_ServerPort = Port;
	}
	void ConnectToServer();
	void DisconnectFromServer(eSdrStatus reason);
	void AssembleAscpMsg(quint8* Buf, int Len);

	virtual void ParseAscpMsg( CAscpRxMsg* pMsg){Q_UNUSED(pMsg)}
	QHostAddress GetServerAddress() { return m_pTcpIo->m_ServerIPAdr;}
	void SendAscpMsg(CAscpTxMsg* pMsg);
	eSdrStatus m_SdrStatus;

signals:
	void ConnectToServerSig();
	void DisconnectFromServerSig();
	void NewSdrStatus(int status);
	void SendSig();

public slots:
	void NewSdrStatusSlot(int status){m_SdrStatus=(eSdrStatus)status; emit NewSdrStatus(status);}

private slots:
	void OnTimer();

private:
	CAscpRxMsg m_RxAscpMsg;
	int m_RxMsgLength;
	int m_RxMsgIndex;
	int m_MsgState;
	int m_MsgTimer;
	int m_TcpConnectTimer;
	QMutex m_Mutex;		//for keeping threads from stomping on each other
	QTimer *m_pTimer;
	CTcp* m_pTcpIo;
};

#endif // NETIO_H
