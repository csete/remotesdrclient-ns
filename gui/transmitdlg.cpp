/////////////////////////////////////////////////////////////////////
// transmitdlg.cpp: implementation of the CTransmitDlg class.
//
//	This class implements a dialog to setup some transmit parameters
//
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////

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
#include "transmitdlg.h"
#include <QDebug>
#include "ui_transmitdlg.h"
#include "interface/sdrprotocol.h"

CTransmitDlg::CTransmitDlg(QWidget *parent, CSdrInterface* pSdrInterface) :
	QDialog(parent), ui(new Ui::CTransmitDlg), m_pSdrInterface(pSdrInterface)

{
	ui->setupUi(this);
	m_EqualizerParams.Gls = 0;
	m_EqualizerParams.Gpk = 0;
	m_EqualizerParams.Ghs = 0;
	m_EqualizerParams.Fls = 500;
	m_EqualizerParams.Fpk = 1000;
	m_EqualizerParams.Fhs = 2500;
	m_EqualizerParams1 = m_EqualizerParams;
	m_EqualizerParams2 = m_EqualizerParams;
	m_TestToneFreq = 1000;
	m_TestMode = TESTSIGNAL_MODE_OFF;
	connect(ui->LowShelfGainSlider, SIGNAL(valueChanged(int)), this, SLOT(OnLowShelfGainSlider(int)));
	connect(ui->PkGainSlider, SIGNAL(valueChanged(int)), this, SLOT(OnPkGainSlider(int)));
	connect(ui->HiShelfGainSlider, SIGNAL(valueChanged(int)), this, SLOT(OnHiShelfGainSlider(int)));
	connect(ui->LowShelfGainSlider, SIGNAL(valueChanged(int)), this, SLOT(OnLowShelfGainSlider(int)));
	connect(ui->LowShelfFreqSlider, SIGNAL(valueChanged(int)), this, SLOT(OnLowShelfFreqSlider(int)));
	connect(ui->PkFreqSlider, SIGNAL(valueChanged(int)), this, SLOT(OnPkFreqSlider(int)));
	connect(ui->HiShelfFreqSlider, SIGNAL(valueChanged(int)), this, SLOT(OnHiShelfFreqSlider(int)));
	connect(ui->ToneFreqSlider, SIGNAL(valueChanged(int)), this, SLOT(OnToneFreqSlider(int)));
	connect(ui->FlatButton, SIGNAL(pressed()), this, SLOT(OnFlatButton()));
	connect(ui->Recall1Button, SIGNAL(pressed()), this, SLOT(OnRecall1Button()));
	connect(ui->Save1Button, SIGNAL(pressed()), this, SLOT(OnSave1Button()));
	connect(ui->Recall2Button, SIGNAL(pressed()), this, SLOT(OnRecall2Button()));
	connect(ui->Save2Button, SIGNAL(pressed()), this, SLOT(OnSave2Button()));

	connect(ui->CtcssSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnCtssChanged(double)));

}

CTransmitDlg::~CTransmitDlg()
{
	delete ui;
}


//////////////////////////////////////////////////////////////////////////////
//Fill in initial data
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::InitDlg()
{
	SetEqControls();
	ui->ToneEdit->setText( QString().number( m_TestToneFreq) );
	ui->TestOffButton->setChecked(true);
	m_TestMode = TESTSIGNAL_MODE_OFF;
	ui->CtcssSpinBox->setValue( (double)m_CtcssFreq/10.0 );
}

//////////////////////////////////////////////////////////////////////////////
//Called to set parameters to tall the controls
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::SetEqControls(void)
{
	ui->LowShelfGainSlider->setValue(m_EqualizerParams.Gls);
	ui->PkGainSlider->setValue(m_EqualizerParams.Gpk);
	ui->HiShelfGainSlider->setValue(m_EqualizerParams.Ghs);
	ui->LowShelfFreqSlider->setValue(m_EqualizerParams.Fls/100);
	ui->PkFreqSlider->setValue(m_EqualizerParams.Fpk/100);
	ui->HiShelfFreqSlider->setValue(m_EqualizerParams.Fhs/100);

	ui->LowGainEdit->setText( QString().number( m_EqualizerParams.Gls ) );
	ui->LowFreqEdit->setText( QString().number( m_EqualizerParams.Fls ) );
	ui->PkGainEdit->setText( QString().number( m_EqualizerParams.Gpk ) );
	ui->PkFreqEdit->setText( QString().number( m_EqualizerParams.Fpk ) );
	ui->HiGainEdit->setText( QString().number( m_EqualizerParams.Ghs ) );
	ui->HiFreqEdit->setText( QString().number( m_EqualizerParams.Fhs ) );
}

//////////////////////////////////////////////////////////////////////////////
//Called to when low shelf gain slider control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnLowShelfGainSlider(int value)
{
	m_EqualizerParams.Gls = value;
	ui->LowGainEdit->setText( QString().number( m_EqualizerParams.Gls ) );
	m_pSdrInterface->SetEqualizer(m_EqualizerParams);
}

//////////////////////////////////////////////////////////////////////////////
//Called to when low shelf freq slider control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnLowShelfFreqSlider(int value)
{
	m_EqualizerParams.Fls = value*100;
	ui->LowFreqEdit->setText( QString().number( m_EqualizerParams.Fls ) );
	m_pSdrInterface->SetEqualizer(m_EqualizerParams);
}

//////////////////////////////////////////////////////////////////////////////
//Called to when peak gain slider control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnPkGainSlider(int value)
{
	m_EqualizerParams.Gpk = value;
	ui->PkGainEdit->setText( QString().number( m_EqualizerParams.Gpk ) );
	m_pSdrInterface->SetEqualizer(m_EqualizerParams);
}

//////////////////////////////////////////////////////////////////////////////
//Called to when peak freq slider control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnPkFreqSlider(int value)
{
	m_EqualizerParams.Fpk = value*100;
	ui->PkFreqEdit->setText( QString().number( m_EqualizerParams.Fpk ) );
	m_pSdrInterface->SetEqualizer(m_EqualizerParams);
}

//////////////////////////////////////////////////////////////////////////////
//Called to when high shelf gain slider control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnHiShelfGainSlider(int value)
{
	m_EqualizerParams.Ghs = value;
	ui->HiGainEdit->setText( QString().number( m_EqualizerParams.Ghs ) );
	m_pSdrInterface->SetEqualizer(m_EqualizerParams);
}

//////////////////////////////////////////////////////////////////////////////
//Called to when high shelf freq slider control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnHiShelfFreqSlider(int value)
{
	m_EqualizerParams.Fhs = value*100;
	ui->HiFreqEdit->setText( QString().number( m_EqualizerParams.Fhs ) );
	m_pSdrInterface->SetEqualizer(m_EqualizerParams);
}

//////////////////////////////////////////////////////////////////////////////
//Called to when tone freq slider control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnToneFreqSlider(int value)
{
	m_TestToneFreq = value;
	ui->ToneEdit->setText( QString().number( m_TestToneFreq) );
	if( TESTSIGNAL_MODE_TONE == m_TestMode)
		m_pSdrInterface->SetTxTestSignalMode(m_TestMode, m_TestToneFreq);
}

//////////////////////////////////////////////////////////////////////////////
//Called to when test off button control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnTestOffButton(bool state)
{
	if(state)
	{
		m_TestMode = TESTSIGNAL_MODE_OFF;
		m_pSdrInterface->SetTxTestSignalMode(m_TestMode, m_TestToneFreq);
	}
}

//////////////////////////////////////////////////////////////////////////////
//Called to when test noise button control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnTestNoiseButton(bool state)
{
	if(state)
	{
		m_TestMode = TESTSIGNAL_MODE_NOISE;
		m_pSdrInterface->SetTxTestSignalMode(m_TestMode, m_TestToneFreq);
	}
}

//////////////////////////////////////////////////////////////////////////////
//Called to when test tone button control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnTestToneButton(bool state)
{
	if(state)
	{
		m_TestMode = TESTSIGNAL_MODE_TONE;
		m_pSdrInterface->SetTxTestSignalMode(m_TestMode, m_TestToneFreq);
	}
}

//////////////////////////////////////////////////////////////////////////////
//Called to when Set Flat response button control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnFlatButton(void)
{
	m_EqualizerParams.Gls = 0;
	m_EqualizerParams.Fls = 500;
	m_EqualizerParams.Gpk = 0;
	m_EqualizerParams.Fpk = 1500;
	m_EqualizerParams.Ghs = 0;
	m_EqualizerParams.Fhs = 2000;
	m_pSdrInterface->SetEqualizer(m_EqualizerParams);
	SetEqControls();
}


//////////////////////////////////////////////////////////////////////////////
//Called to when recall user setting 1 button control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnRecall1Button(void)
{
	m_EqualizerParams = m_EqualizerParams1;
	m_pSdrInterface->SetEqualizer(m_EqualizerParams);
	SetEqControls();
}

//////////////////////////////////////////////////////////////////////////////
//Called to when save user setting 1 button control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnSave1Button(void)
{
	m_EqualizerParams1 = m_EqualizerParams;
}

//////////////////////////////////////////////////////////////////////////////
//Called to when recall user setting 2 button control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnRecall2Button(void)
{
	m_EqualizerParams = m_EqualizerParams2;
	m_pSdrInterface->SetEqualizer(m_EqualizerParams);
	SetEqControls();

}

//////////////////////////////////////////////////////////////////////////////
//Called to when save user setting 2 button control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnSave2Button(void)
{
	m_EqualizerParams2 = m_EqualizerParams;
}


//////////////////////////////////////////////////////////////////////////////
//Called to when FM CTSS tone frequency edit control changes
//////////////////////////////////////////////////////////////////////////////
void CTransmitDlg::OnCtssChanged(double value)
{
	m_CtcssFreq = (int)(value*10.0);
	m_pSdrInterface->SetCtcssFreq(m_CtcssFreq);
}
