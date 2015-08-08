//////////////////////////////////////////////////////////////////////
// soundin.h: interface for the CSoundIn class.
//
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////

#ifndef SOUNDIN_H
#define SOUNDIN_H

#include <QList>
#include <QIODevice>
#include "threadwrapper.h"
#include <QAudioInput>

#define INQSIZE 4000	//max samples in Queue

#define SOUND_READBUFSIZE (INQSIZE*2)

#define DEC2TAP_LENGTH 23
const double DEC2_H[DEC2TAP_LENGTH] =
{
	-0.00014987651418332164, 0.0,
	0.0014748633283609852, 0.0,
	-0.0074416944990005314, 0.0,
	0.026163522731980929, 0.0,
	-0.077593699116544707, 0.0,
	0.30754683719791986, 0.5,
	0.30754683719791986, 0.0,
	-0.077593699116544707, 0.0,
	0.026163522731980929, 0.0,
	-0.0074416944990005314, 0.0,
	0.0014748633283609852, 0.0,
	-0.00014987651418332164
};

//3.5kHz PBand, .5dB ripple,  70dB StopBand at 4.5kHz at 24KHz samplerate
#define DEC3TAP_LENGTH 63
const double DEC3_H[DEC3TAP_LENGTH*2] =
{
	-0.000122089672, 0.000756870819, 0.002723170921, 0.005253908351, 0.006498627630,
	 0.004642639462,-0.000127922266,-0.004816317633,-0.005459270973,-0.000749873754,
	 0.005791180122, 0.007947782646, 0.002401758772,-0.007062842708,-0.011594604118,
	 -0.005173923318, 0.008331932993, 0.016607547268, 0.009560094412,-0.009513394530,
	-0.023769125601,-0.016708360339, 0.010511004278, 0.035111757024, 0.029654555288,
	-0.011273660216,-0.057805247862,-0.060635313789, 0.011753658267, 0.142509501597,
	 0.269092683398, 0.321418881518, 0.269092683398, 0.142509501597, 0.011753658267,
	 -0.060635313789,-0.057805247862,-0.011273660216, 0.029654555288, 0.035111757024,
	 0.010511004278,-0.016708360339,-0.023769125601,-0.009513394530, 0.009560094412,
	  0.016607547268, 0.008331932993,-0.005173923318,-0.011594604118,-0.007062842708,
	 0.002401758772, 0.007947782646, 0.005791180122,-0.000749873754,-0.005459270973,
	 -0.004816317633,-0.000127922266, 0.004642639462, 0.006498627630, 0.005253908351,
	 0.002723170921, 0.000756870819,-0.000122089672,
	//
	-0.000122089672, 0.000756870819, 0.002723170921, 0.005253908351, 0.006498627630,
	 0.004642639462,-0.000127922266,-0.004816317633,-0.005459270973,-0.000749873754,
	 0.005791180122, 0.007947782646, 0.002401758772,-0.007062842708,-0.011594604118,
	 -0.005173923318, 0.008331932993, 0.016607547268, 0.009560094412,-0.009513394530,
	-0.023769125601,-0.016708360339, 0.010511004278, 0.035111757024, 0.029654555288,
	-0.011273660216,-0.057805247862,-0.060635313789, 0.011753658267, 0.142509501597,
	 0.269092683398, 0.321418881518, 0.269092683398, 0.142509501597, 0.011753658267,
	 -0.060635313789,-0.057805247862,-0.011273660216, 0.029654555288, 0.035111757024,
	 0.010511004278,-0.016708360339,-0.023769125601,-0.009513394530, 0.009560094412,
	  0.016607547268, 0.008331932993,-0.005173923318,-0.011594604118,-0.007062842708,
	 0.002401758772, 0.007947782646, 0.005791180122,-0.000749873754,-0.005459270973,
	 -0.004816317633,-0.000127922266, 0.004642639462, 0.006498627630, 0.005253908351,
	 0.002723170921, 0.000756870819,-0.000122089672
};

class CSoundIn : public CThreadWrapper
{
	Q_OBJECT
public:
	explicit CSoundIn();
	virtual ~CSoundIn();

	//Exposed functions
	void Start(int InDevIndx){emit StartSig(InDevIndx);}//starts soundcard input
	void Stop(){emit StopSig();}	//stops soundcard input
	void Reset();
	int GetInQueueSampsAvailable(){return m_InQAvailable;}
	int GetInQueue(int numsamples, qint16* pData );
	bool IsRunning(){ if(m_pAudioInput) return (QAudio::ActiveState == m_pAudioInput->state()); else return false;}
	qint16 GetInputLevel();


signals:
	void StartSig(int InDevIndx);	//starts soundcard input
	void StopSig();	//stops soundcard input
	void NewSoundDataRdy();	//signals new soundcard data is available

private slots:
	void StartSlot(int InDevIndx);	//starts soundcard input
	void StopSlot();	//stops soundcard input
	void NewDataReady();
	void ThreadInit();	//override function is called by new thread when started
	void ThreadExit();	//override function is called by thread before exiting

protected:

private:
	void PutInQueue(int numsamples, qint16* pData );
	int DecBy2(int InLength, qint16* pInData, double* pOutData);
	int DecBy3(int InLength, double* pInData, qint16* pOutData);

	QList<QAudioDeviceInfo> m_InDevices;
	QAudioDeviceInfo  m_InDeviceInfo;
	QAudioFormat m_InAudioFormat;
	QAudioInput* m_pAudioInput;
	QIODevice* m_pIODevice;	// ptr to internal soundin IODevice

	qint16 m_InQueue[INQSIZE];
	char m_pData[SOUND_READBUFSIZE];
	int m_InQHead;
	int m_InQTail;
	int m_InQAvailable;
	int m_SampCnt;
	int m_Dec3State;
	double m_Dec2FirBuf[SOUND_READBUFSIZE+DEC2TAP_LENGTH];
	double m_Dec3FirBuf[SOUND_READBUFSIZE];
	double m_TmpDecBuf[SOUND_READBUFSIZE];
	double m_ToneInc;
	double m_ToneTime;
	qint16 m_MaxInputLevel;
};
#endif // SOUNDIN_H
