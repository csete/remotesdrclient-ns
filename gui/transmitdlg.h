//////////////////////////////////////////////////////////////////////
// transmitdlg.h: interface for the CTransmitDlg class.
//
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////
#ifndef TRANSMITDLG_H
#define TRANSMITDLG_H

#include <QDialog>
#include "interface/sdrinterface.h"


namespace Ui {
class CTransmitDlg;
}

class CTransmitDlg : public QDialog
{
	Q_OBJECT
	
public:
	CTransmitDlg(QWidget *parent = 0, CSdrInterface* pSdrInterface = 0);
	~CTransmitDlg();
	void InitDlg();
	tEqualizer m_EqualizerParams;
	tEqualizer m_EqualizerParams1;
	tEqualizer m_EqualizerParams2;
	int m_TestToneFreq;
	int m_CtcssFreq;

private slots:
	void OnLowShelfGainSlider(int value);
	void OnLowShelfFreqSlider(int value);
	void OnPkGainSlider(int value);
	void OnPkFreqSlider(int value);
	void OnHiShelfGainSlider(int value);
	void OnHiShelfFreqSlider(int value);
	void OnToneFreqSlider(int value);
	void OnTestOffButton(bool);
	void OnTestNoiseButton(bool);
	void OnTestToneButton(bool);
	void OnFlatButton(void);
	void OnRecall1Button(void);
	void OnSave1Button(void);
	void OnRecall2Button(void);
	void OnSave2Button(void);
	void OnCtssChanged(double value);

private:
	void SetEqControls(void);

	int m_TestMode;
	Ui::CTransmitDlg *ui;
	CSdrInterface* m_pSdrInterface;
};

#endif // TRANSMITDLG_H
