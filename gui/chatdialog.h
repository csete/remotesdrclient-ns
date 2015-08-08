//////////////////////////////////////////////////////////////////////
// chatdialog.h: interface of the CChatDialog class.
//
//  This class creates a  dialog box for user entry and receiving of text data
//
// History:
//	2015-02-21  Initial creation MSW
//////////////////////////////////////////////////////////////////////
//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2010 Moe Wheatley. All rights reserved.
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
#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include "interface/sdrinterface.h"

namespace Ui {
class CChatDialog;
}

class CChatDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CChatDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
	~CChatDialog();
	void SetSdrInterface(CSdrInterface* ptr){m_pSdrInterface = ptr;}

signals:
	void SendRxChatData(quint8 ch);

private slots:
	void GotRxChatData(quint8 ch);
	void OnClearRx();
	void OnStartTx();
	void OnStopTx();
	void OnTxTextChanged();
	void OnPskModeChanged(int index);

private:
	QString m_Str;
	int m_TxLength;
	Ui::CChatDialog *ui;
	CSdrInterface* m_pSdrInterface;
	QWidget* m_pParent;
};

extern CChatDialog* g_pChatDialog;

#endif // CHATDIALOG_H
