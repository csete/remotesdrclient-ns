//////////////////////////////////////////////////////////////////////
// NetInterface.cpp: implementation of the CNetio and CTcp.
//
//  This base class implements the low level TCP interface.
// A worker thread is launched to handle all the TCP signals
//
// History:
//	2013-10-02  Initial creation MSW
//	2013-12-17  added logic to abort connecting state when reconnecting
/////////////////////////////////////////////////////////////////////
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
//=============================================================================
/*---------------------------------------------------------------------------*/
/*------------------------> I N C L U D E S <--------------------------------*/
/*---------------------------------------------------------------------------*/
#include <QtNetwork>
#include <QDebug>
#include "netio.h"

#define TCP_CONNECT_TIMELIMIT 100	//time to wait for connect in 100mSec steps

/////////////////////////////////////////////////////////////////////
// Constructor/Destructor
/////////////////////////////////////////////////////////////////////
CTcp::CTcp(QObject *parent) : m_pParent(parent)
{
	m_pTcpClient = NULL;
//	qDebug()<<"CTcp constructor";
}

CTcp::~CTcp()
{
	CleanupThread();	//signals thread to call "ThreadExit() to clean up resources
//	qDebug()<<"CTcp destructor";
}

/////////////////////////////////////////////////////////////////////
// Called by this worker thread to initialize itself
/////////////////////////////////////////////////////////////////////
void CTcp::ThreadInit()	//override called by new thread when started
{
//qDebug()<<"CTcp Thread Init "<<this->thread()->currentThread();
	m_TxPosition = 0;
	m_pTcpClient = new QTcpSocket;	//must be deleted in this same thread
									//by calling CleanupThread() in destructor
	connect(m_pTcpClient, SIGNAL(readyRead()), this, SLOT(ReadTcpDataSlot()));
	connect(m_pTcpClient, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
			this, SLOT(TcpStateChangedSlot(QAbstractSocket::SocketState)));

	connect(m_pParent, SIGNAL(ConnectToServerSig() ), this, SLOT( ConnectToServerSlot() ) );
	connect(m_pParent, SIGNAL(DisconnectFromServerSig() ), this, SLOT( DisconnectFromServerSlot() ) );
	connect(m_pParent, SIGNAL(SendSig() ), this, SLOT( SendSlot() ) );
	connect(this, SIGNAL(NewSdrStatusSig(int)  ), m_pParent, SLOT( NewSdrStatusSlot(int) ) );
	emit NewSdrStatusSig(SDR_OFF);
}

/////////////////////////////////////////////////////////////////////
// Called by this worker thread to cleanup after itself
/////////////////////////////////////////////////////////////////////
void CTcp::ThreadExit()
{
	DisconnectFromServerSlot();
	disconnect();
	//must delete this resource in thread context that created it
	if(m_pTcpClient)
		delete m_pTcpClient;
}

/////////////////////////////////////////////////////////////////////
// Called to connect to TCP server
/////////////////////////////////////////////////////////////////////
void CTcp::ConnectToServerSlot()
{
	emit NewSdrStatusSig(SDR_CONNECTING);
	if( (QAbstractSocket::ConnectingState == m_pTcpClient->state() ) ||
		(QAbstractSocket::HostLookupState == m_pTcpClient->state() ) )
		m_pTcpClient->abort();
	if( QAbstractSocket::ConnectedState == m_pTcpClient->state())
		m_pTcpClient->close();

qDebug()<<"Connecting to Server"<<m_pTcpClient->state() << m_DomainName << m_ServerIPAdr <<m_ServerPort;
	if(m_DomainName != "")
		m_pTcpClient->connectToHost(m_DomainName, m_ServerPort);
	else
		m_pTcpClient->connectToHost(m_ServerIPAdr, m_ServerPort);
}

/////////////////////////////////////////////////////////////////////
// Called to disconnect from TCP server
/////////////////////////////////////////////////////////////////////
void CTcp::DisconnectFromServerSlot()
{
	if( (QAbstractSocket::ConnectedState == m_pTcpClient->state() ) ||
		(QAbstractSocket::HostLookupState == m_pTcpClient->state() ) ||
		(QAbstractSocket::ConnectingState == m_pTcpClient->state() ) )
	{
		m_pTcpClient->abort();
		m_pTcpClient->close();
		qDebug()<<"Disconnect from serverSlot";
	}
}

/////////////////////////////////////////////////////////////////////
//  Slot called when TCP data is available
/////////////////////////////////////////////////////////////////////
void CTcp::ReadTcpDataSlot()
{
quint8 pBuf[50000];
qint64 n;
	if( QAbstractSocket::ConnectedState == m_pTcpClient->state())
	{
		do
		{
			n = m_pTcpClient->bytesAvailable();
			if( (n<50000) && n>0 )
			{
				m_pTcpClient->read((char*)pBuf, 50000);
				((CNetio*)m_pParent)->AssembleAscpMsg(pBuf, n);
			}
		}while(n>0);
	}
}

/////////////////////////////////////////////////////////////////////
// called from external thread context to send buffer to TCP server
/////////////////////////////////////////////////////////////////////
void CTcp::PutTxBuf(char* pBuf, int length)
{
	m_Mutex.lock();
	for(int i=0; i<length; i++)
	{
		m_TxBuf[m_TxPosition++] = pBuf[i];
		if(m_TxPosition >= TXQ_SIZE)
			m_TxPosition = 0;	//should never get here
	}
	m_Mutex.unlock();
}

/////////////////////////////////////////////////////////////////////
// called by tcp worker thread to send buffer to TCP server
/////////////////////////////////////////////////////////////////////
void CTcp::SendSlot()
{
	if(0==m_TxPosition)
		return;	//nothing to send
	if( QAbstractSocket::ConnectedState == m_pTcpClient->state())
	{
		m_Mutex.lock();
		qint64 sent = m_pTcpClient->write(m_TxBuf, m_TxPosition );
		m_TxPosition = m_TxPosition - sent;
		m_Mutex.unlock();
		if(m_TxPosition<0)
		{
			qDebug()<<"Tx Error";
			m_TxPosition = 0;
		}
	}
}

/////////////////////////////////////////////////////////////////////
// Slot Called when Client TCP state changes
/////////////////////////////////////////////////////////////////////
void CTcp::TcpStateChangedSlot(QAbstractSocket::SocketState State)
{
qDebug()<<State;
	switch(State)
	{
		case QAbstractSocket::ConnectingState:
		case QAbstractSocket::HostLookupState:
			break;
		case QAbstractSocket::ConnectedState:
			emit NewSdrStatusSig(SDR_CONNECTED);
qDebug()<<"Connected to Server";
			break;
		case QAbstractSocket::ClosingState:
			break;
		case QAbstractSocket::UnconnectedState:
			if(  SDR_CONNECTING ==((CNetio*)m_pParent)->m_SdrStatus)
			{
				emit NewSdrStatusSig(SDR_DISCONNECT_TIMEOUT);
qDebug()<<"Disconnected from Server";
			}
			break;
		default:
			break;
	}
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
/*************  Start of CNetio Implementation  *******************/
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

/////////////////////////////////////////////////////////////////////
// Constructor/Destructor
/////////////////////////////////////////////////////////////////////
CNetio::CNetio()
{
	m_SdrStatus = SDR_OFF;
	m_pTcpIo = new CTcp(this);
	qDebug()<<"CNetio constructor";
	m_TcpConnectTimer = 0;
	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(OnTimer()));
	m_pTimer->start(100);		//start up status timer
}

CNetio::~CNetio()
{
qDebug()<<"CNetio destructor";
	if(m_pTcpIo)
		delete m_pTcpIo;
}


/////////////////////////////////////////////////////////////////////
// Called to try and connect to server
/////////////////////////////////////////////////////////////////////
void CNetio::ConnectToServer()
{
	m_MsgState = MSGSTATE_HDR1;
	m_RxMsgIndex = 0;
	m_RxMsgLength = 0;
	m_TcpConnectTimer = TCP_CONNECT_TIMELIMIT;	//start connect timer
	emit ConnectToServerSig();
}

/////////////////////////////////////////////////////////////////////
// Called to disconnect to server
/////////////////////////////////////////////////////////////////////
void CNetio::DisconnectFromServer(eSdrStatus reason )
{
	m_SdrStatus = reason;
	NewSdrStatusSlot(reason);
	emit DisconnectFromServerSig();
}

/////////////////////////////////////////////////////////////////////
// Status 100mSec Timer event handler manages TCP timeouts
/////////////////////////////////////////////////////////////////////
void CNetio::OnTimer()
{
	if( (SDR_OFF != m_SdrStatus ) &&
		(SDR_DISCONNECT_BUSY != m_SdrStatus ) &&
		(SDR_DISCONNECT_PWERROR != m_SdrStatus ) &&
		(SDR_DISCONNECT_TIMEOUT != m_SdrStatus ) )
	{	//if trying or connected to server
		if(0 == m_TcpConnectTimer)
		{	//if no response from server during connect
			DisconnectFromServer(SDR_DISCONNECT_TIMEOUT);
qDebug()<<"TCP_Connect_Timeout";
		}
		else
		{
			m_TcpConnectTimer--;
		}
	}
}

/////////////////////////////////////////////////////////////////////
// Called to send an ASCP formatted message to the server
/////////////////////////////////////////////////////////////////////
void CNetio::SendAscpMsg(CAscpTxMsg* pMsg)
{
	if(m_pTcpIo)
	{
		m_pTcpIo->PutTxBuf( (char*)pMsg->Buf8, pMsg->GetLength() );
		emit SendSig();
	}
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                  -------------------------------                          */
/*                 | A s s e m b l e A s c p M s g |                         */
/*                  -------------------------------                          */
/*  Helper function to assemble TCP data stream into ASCP formatted messages */
/* Called from worker thread context so be careful.                          */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
void CNetio::AssembleAscpMsg(quint8* Buf, int Len)
{
	for(int i=0; i<Len; i++)
	{	//process everything in Buf
		switch(m_MsgState)	//Simple state machine to get generic ASCP msg
		{
			case MSGSTATE_HDR1:		//get first header byte of ASCP msg
				m_RxAscpMsg.Buf8[0] = Buf[i];
				m_MsgState = MSGSTATE_HDR2;	//go to get second header byte state
				break;
			case MSGSTATE_HDR2:	//here for second byte of header
				m_RxAscpMsg.Buf8[1] = Buf[i];
				m_RxMsgLength = m_RxAscpMsg.Buf16[0] & 0x1FFF;
				m_RxMsgIndex = 2;
				if(2 == m_RxMsgLength)	//if msg has no parameters then we are done
				{
					m_MsgState = MSGSTATE_HDR1;	//go back to first state
					ParseAscpMsg( &m_RxAscpMsg );
				}
				else	//there are data bytes to fetch
				{
					if( 0 == m_RxMsgLength)
						m_RxMsgLength = 8192+2;	//handle special case of 8192 byte data message
					m_MsgState = MSGSTATE_DATA;	//go to data byte reading state
				}
				break;
			case MSGSTATE_DATA:	//try to read the rest of the message
				m_RxAscpMsg.Buf8[m_RxMsgIndex++] = Buf[i];
				if( m_RxMsgIndex >= m_RxMsgLength )
				{
					m_MsgState = MSGSTATE_HDR1;	//go back to first stage
					m_RxMsgIndex = 0;
					ParseAscpMsg( &m_RxAscpMsg );	//got complete msg so call virtual parser.
					m_TcpConnectTimer = TCP_CONNECT_TIMELIMIT;	//reset connect timer
				}
				break;
		} //end switch statement
	}
}

