/////////////////////////////////////////////////////////////////////
// ceditnetdlg.cpp: implementation of the CEditNetDlg class.
//
//	This class implements a dialog to setup the network parameters
// and discover all network sdr devices
//
// History:
//	2013-10-02  Initial creation MSW
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
//==========================================================================================
#include <QRegExpValidator>
#include <QRegExp>
#include <QKeyEvent>
#include "gui/editnetdlg.h"
#include "gui/sdrdiscoverdlg.h"
#include "ui_editnetdlg.h"
#include <QDebug>

CEditNetDlg::CEditNetDlg(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CEditNetDlg)
{
	m_DomainName = "";
	ui->setupUi(this);
	qApp->installEventFilter(this);
	m_pPortAddressValidator = NULL;
	m_pPortAddressValidator = new QIntValidator(0, 65535, this);
//	ui.lineEdit_TCPPort->setValidator(m_pPortAddressValidator);
}

CEditNetDlg::~CEditNetDlg()
{
	if(m_pPortAddressValidator)
		delete m_pPortAddressValidator;
}

//intercept return key and map it to the tab key so the dialog doesnt exit
bool CEditNetDlg::eventFilter(QObject* o, QEvent* e)
{
	if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease)
	{
		QKeyEvent* ke = (QKeyEvent*)e;
		if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
		{
			QKeyEvent rep(ke->type(), Qt::Key_Tab, Qt::NoModifier, QString::null, ke->isAutoRepeat(), ke->count());
			*ke = rep;
		}
	}
	return QDialog::eventFilter(o, e);
}

//Fill in initial data
void CEditNetDlg::InitDlg()
{
	ui->IPEditwidget_IP->SetIP(m_IPAdr.toIPv4Address());
	ui->lineEdit_TCPPort->setText(QString().number( m_Port ));
	ui->lineEditDomain->setText(m_DomainName);
}

////////////////////////////////////////////////////////////////////////////////
//	Sends out request to Internet server get list of active SDR's
////////////////////////////////////////////////////////////////////////////////
void CEditNetDlg::FindSdrs()
{
CSdrDiscoverDlg dlg(this);
	dlg.SetListServer(ui->lineEditListServerUrl->text());
	dlg.SetListServerActionPath(ui->lineEditListServerActionPath->text());
	dlg.InitDlg();
	if( dlg.exec() )
	{
		m_DomainName = dlg.m_DomainName;
		m_IPAdr = dlg.m_IPAdr;
		m_Port = dlg.m_Port;
		ui->IPEditwidget_IP->SetIP(m_IPAdr.toIPv4Address());
		ui->lineEdit_TCPPort->setText(QString().number( m_Port ));
		ui->lineEditDomain->setText(m_DomainName);
		if(!dlg.m_NeedPW)
			ui->lineEditRxPw->setText("");
	}
}

void CEditNetDlg::ListSdrs()
{
    CSdrDiscoverDlg     dlg(this);

    dlg.SetListServer("test.oz9aec.net");
    dlg.SetListServerActionPath("/servers.txt");
    dlg.InitDlg();
    if (dlg.exec())
    {
        m_DomainName = dlg.m_DomainName;
        m_IPAdr = dlg.m_IPAdr;
        m_Port = dlg.m_Port;
        ui->IPEditwidget_IP->SetIP(m_IPAdr.toIPv4Address());
        ui->lineEdit_TCPPort->setText(QString().number(m_Port));
        ui->lineEditDomain->setText(m_DomainName);
        if (!dlg.m_NeedPW)
            ui->lineEditRxPw->setText("");
    }
}

//Called when OK button pressed so get all the dialog data
void CEditNetDlg::accept()
{	//OK was pressed so get all data from edit controls
quint32 ip;
	ui->IPEditwidget_IP->GetIP(ip);
	m_IPAdr.setAddress(ip);
	m_Port = ui->lineEdit_TCPPort->text().toUInt();
	m_DomainName = ui->lineEditDomain->text();
	QDialog::accept();
}
