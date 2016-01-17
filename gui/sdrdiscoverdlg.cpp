/////////////////////////////////////////////////////////////////////
// sdrdiscoverdlg.cpp: implementation of the CSdrDiscoverDlg class.
//
//	This class implements a discover widget to find RFSPACE SDR's
//connected to a network using an http data format.
//
// History:
//	2013-10-02  Initial creation MSW
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
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
#include "gui/sdrdiscoverdlg.h"
#include <QStringList>
#include <QDebug>
#include <QLabel>

/*---------------------------------------------------------------------------*/
/*--------------------> L O C A L   D E F I N E S <--------------------------*/
/*---------------------------------------------------------------------------*/
#define NUM_ENTRIES_OLD 9
#define NUM_ENTRIES 12


//////////////////////////////////////////////////////////////////////////////
//Constructor/Destructor
//////////////////////////////////////////////////////////////////////////////
CSdrDiscoverDlg::CSdrDiscoverDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CSdrDiscoverDlg)
{
    ui->setupUi(this);

	ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->tableWidget->verticalHeader()->setVisible(false);
	ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
//	ui->tableWidget->verticalHeader()->setStretchLastSection(true);
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->tableWidget->resizeRowsToContents();
	ui->tableWidget->setAlternatingRowColors(true);

	QFont fnt = ui->tableWidget->horizontalHeader()->font();
	fnt.setBold(true);
	ui->tableWidget->horizontalHeader()->setFont(fnt);

	ui->tableWidget->horizontalHeader()->setHighlightSections( false);

	m_ListServerUrl = "";
	m_pTcpClient = new QTcpSocket;
	//connect TCP client signals.
	connect(m_pTcpClient, SIGNAL(readyRead()), this, SLOT(ReadTcpClientData()));
	connect(m_pTcpClient, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
			this, SLOT(TcpClientStateChanged(QAbstractSocket::SocketState)));
	connect(m_pTcpClient, SIGNAL(error(QAbstractSocket::SocketError)),
			this, SLOT(OnTcpClientError(QAbstractSocket::SocketError)));
	connect(m_pTcpClient, SIGNAL(disconnected() ),this, SLOT(OnTcpClientDisconnected()));
	connect(m_pTcpClient, SIGNAL(connected() ),this, SLOT(OnTcpClientConnected()));
}

CSdrDiscoverDlg::~CSdrDiscoverDlg()
{
	if(m_pTcpClient)
		delete m_pTcpClient;
    delete ui;
}

//////////////////////////////////////////////////////////////////////////////
//Fill in initial data
//////////////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::InitDlg()
{
	OnFind();
}

//////////////////////////////////////////////////////////////////////////////
//Called when Find button pressed
//////////////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::OnFind()
{
	ui->tableWidget->clear();
	ui->tableWidget->setRowCount(0);
	ui->tableWidget->setColumnCount(9);
	m_TableHeader<<"Status"<<"SN"<<"Address"<<"Lat,Lon"<<"Security"<<"Description"<<"Descrption URL"<<"Options"<<"Last Client";
	ui->tableWidget->setHorizontalHeaderLabels(m_TableHeader);
	SendDiscoverRequest();
}

/////////////////////////////////////////////////////////////////////
// Slot Called when List Client TCP state changes
/////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::TcpClientStateChanged(QAbstractSocket::SocketState State)
{
qDebug()<<"TCP State "<<State;
	if(QAbstractSocket::ConnectedState == State)
	{	//send any data now that socket is open
		SendReqToListServer();
	}
}

/////////////////////////////////////////////////////////////////////
// Slot Called when List Client TCP connencts
/////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::OnTcpClientConnected()
{
	qDebug()<<"TCP Connected";
}

/////////////////////////////////////////////////////////////////////
// Slot Called when List Client TCP disconencts
/////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::OnTcpClientDisconnected()
{
	qDebug()<<"TCP Disconnected";
}

/////////////////////////////////////////////////////////////////////
// Slot Called when List Client TCP error occurs
/////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::OnTcpClientError(QAbstractSocket::SocketError err)
{
	qDebug()<<"TCP ErrorState "<<err;
}

//////////////////////////////////////////////////////////////////////////////
// Called by timeout in case socket still open after TCP send
//////////////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::TcpCloseTimeout()
{
qDebug()<<"Timeout Close State "<<m_pTcpClient->state();
	if( QAbstractSocket::ConnectedState == m_pTcpClient->state())
	{
		m_pTcpClient->abort();
		m_pTcpClient->close();
		qDebug()<<"Timed TCP Close2";
	}
}

//////////////////////////////////////////////////////////////////////////////
//Called when TCP data is received
//////////////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::ReadTcpClientData()
{
    char pBuf[10000];
    bool UseCommaSep = true;

    if (QAbstractSocket::ConnectedState == m_pTcpClient->state())
	{
		qint64 ret;
		int line=0;
        //bool respok = false;
        //bool datalines = false;
		do
		{	//read http response from server one line at  time
			UseCommaSep = true;
			ret = m_pTcpClient->readLine(pBuf, 256);

            qDebug()<< ret << " : " << pBuf;

            if (ret >0)
			{
				pBuf[ret] = 0;	//zero terminate line
				m_Str = pBuf;
                for (int i = 0; i < m_Str.length(); i++)
				{
					if( '\v' == pBuf[i])
					{
						UseCommaSep = false;
						break;
					}
				}
				line++;
				//see if ever get fttp good response line

                // if( m_Str.contains("200 OK") )
                //	respok = true;
                //if(respok)
                //{
                    //if(datalines)
					{	//here if a data field line
//qDebug()<<UseCommaSep;

                        if (UseCommaSep)
						{
							QStringList entry = m_Str.split(",");
							PutEntryInTable(entry);
						}
						else
						{
                            QStringList entry = m_Str.split("\v") ;//change field delimitor to VT
							PutEntryInTable(entry);
						}
					}
					//if found data start string key "&&&"
                    //if( m_Str.contains("&&&") )
                    //	datalines = true;
                //}
			}
        } while (ret > 0);
	}
}

//////////////////////////////////////////////////////////////////////////////
// Formats new 'entry' and places in next position in QListWidget
//  Entry[] array has 12 string fields:
//sn ♂ desc ♂ stat ♂ ipaddress string ♂ portnumber string ♂ domain string ♂ latitude string ♂ longitude string
//♂ security string♂ user description url string ♂ options string ♂ last client string
//////////////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::PutEntryInTable(QStringList entry)
{
	if( (entry.size() != NUM_ENTRIES) && (entry.size() != NUM_ENTRIES_OLD) )
	{
		qDebug()<<"Entry Size Err= "<<entry.size();
		  return;
	}
	int index = ui->tableWidget->rowCount();	//get index to next free position in list
	if(  (index<0) || (index>=MAX_SDRS) )
	{
		qDebug()<<"Entry Index = "<<index;
		return;
	}
	ui->tableWidget->setRowCount(index+1);

	QString SN = entry[0];
	QString Desc = entry[1];
	QString Status = entry[2];
	QHostAddress ipadr( entry[3] );
	quint16 Port = entry[4].toUShort();
	QString Domain = entry[5];
	float Lat = entry[6].toFloat();
	float Lon = entry[7].toFloat();
	QString pw = entry[8];

	QString durl = "";
	QString opt = "";
	QString lclient = "";
	if(entry.size() == NUM_ENTRIES)
	{
		durl = entry[9];
		opt = entry[10];
		lclient = entry[11];
	}

	//save key parameters in lm_ParamList for easier retrieval
	m_ParamList[index].DomainName = Domain;
	m_ParamList[index].IPAdr = ipadr;
	m_ParamList[index].Port = Port;

	QTableWidgetItem * protoitem = new QTableWidgetItem();
	protoitem->setTextAlignment(Qt::AlignCenter);

	//	m_TableHeader<<"Status"<<"SN"<<"Address"<<"LatLon"<<"Security"<<"Desc";
	QTableWidgetItem * newitem0 = protoitem->clone();
	if(Status == "busy")
	{
		newitem0->setText("In Use");
		m_ParamList[index].InUse = true;
	}
	else
	{
		newitem0->setText("Idle");
		m_ParamList[index].InUse = false;
	}
	ui->tableWidget->setItem(index, 0, newitem0);

	QTableWidgetItem * newitem1 = protoitem->clone();
	newitem1->setText(SN);
	ui->tableWidget->setItem(index, 1, newitem1);
	m_Str = "";
	if(Domain!="")
		m_Str = m_Str + ":" + QString::number(Port);
	else
		m_Str = ipadr.toString() + ":" + QString::number(Port);

	QTableWidgetItem * newitem2 = protoitem->clone();
	newitem2->setText(m_Str);
	ui->tableWidget->setItem(index, 2, newitem2);

	m_Str = QString::number(Lat) + "," + QString::number(Lon);
	QTableWidgetItem * newitem3 = protoitem->clone();
	newitem3->setText(m_Str);
	ui->tableWidget->setItem(index, 3, newitem3);

	QTableWidgetItem * newitem4 = protoitem->clone();
	if( pw.contains("private") )
	{
		m_ParamList[index].NeedPW = true;
		newitem4->setText("Password Required");
	}
	else
	{
		newitem4->setText("Open");
		m_ParamList[index].NeedPW = false;
	}
	ui->tableWidget->setItem(index, 4, newitem4);

	QTableWidgetItem * newitem5 = protoitem->clone();
	newitem5->setText(Desc);
	ui->tableWidget->setItem(index, 5, newitem5);

	QLabel *qlab = new QLabel;
	m_Str = "<a href="+durl+" >"+durl+"</a>";
	qDebug()<<m_Str;
	qlab->setText("<a href=http://"+durl+" >"+durl+" </a>");
	qlab->setTextFormat(Qt::RichText);
	qlab->setIndent(4);
	qlab->setTextInteractionFlags(Qt::TextBrowserInteraction);
	qlab->setOpenExternalLinks(true);
	ui->tableWidget->setCellWidget(index, 6, qlab);

//	QTableWidgetItem * newitem6 = protoitem->clone();
//	newitem6->setText(durl);
//	ui->tableWidget->setItem(index, 6, newitem6);

	QTableWidgetItem * newitem7 = protoitem->clone();
	newitem7->setText(opt);
	ui->tableWidget->setItem(index, 7, newitem7);

	QTableWidgetItem * newitem8 = protoitem->clone();
	newitem8->setText(lclient);
	ui->tableWidget->setItem(index, 8, newitem8);
}

//////////////////////////////////////////////////////////////////////////////
//Call to start discover request by trying to connect to list server
//////////////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::SendDiscoverRequest()
{
	if(m_ListServerUrl != "")	//only if Url not blank
	{
		qDebug()<<"Open ListServer "<<m_ListServerUrl;
		m_pTcpClient->connectToHost(m_ListServerUrl, 80);
	}
}

//////////////////////////////////////////////////////////////////////////////
//called when TCP connection is opened so can send http GET header
//////////////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::SendReqToListServer()
{
	if( QAbstractSocket::ConnectedState == m_pTcpClient->state())
	{
		m_Str = "";
		QTextStream(&m_Str)<<"GET "<<m_ListServerActionPath<<" HTTP/1.1\r\n";
		QTextStream(&m_Str)<<"Host: "<<m_ListServerUrl<<"\r\n";
		QTextStream(&m_Str)<<"Content-Type: application/x-www-form-urlencoded\r\n";
		QTextStream(&m_Str)<<"Connection: close\r\n";
		QTextStream(&m_Str)<<"Content-Length: 0\r\n";
		QTextStream(&m_Str)<<"\r\n";
		QByteArray ba = m_Str.toUtf8();
		char *pBuf = ba.data();;
		//fire one-shot timer in case server doesn't close socket after transaction
		QTimer::singleShot(5000, this, SLOT(TcpCloseTimeout()));
		m_pTcpClient->write(pBuf, (qint64)m_Str.size() );
//qDebug()<<pBuf;
	}
}


void CSdrDiscoverDlg::OnTableItemDoubleClick( QTableWidgetItem * item )
{
	Q_UNUSED(item);
	accept();
}

//////////////////////////////////////////////////////////////////////////////
//Called when ok is pressed to accept selected sdr parameters
//////////////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::accept()
{	//OK was pressed so get all data from edit controls
	int index = ui->tableWidget->currentRow();
	if( (index>=0) && !m_ParamList[index].InUse  )
	{
		m_IPAdr = m_ParamList[index].IPAdr;
		m_Port = m_ParamList[index].Port;
		m_DomainName = m_ParamList[index].DomainName;
		m_NeedPW =  m_ParamList[index].NeedPW;
	}
	QDialog::accept();		//call base class to exit
}
