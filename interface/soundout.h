//////////////////////////////////////////////////////////////////////
// soundout.h: interface for the CSoundOut class.
//
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////
#ifndef SOUNDOUT_H
#define SOUNDOUT_H

#include <QList>
#include <QIODevice>
#include "threadwrapper.h"
#include <QAudioOutput>

#define OUTQSIZE 24000	//max samples in Queue
#define SOUND_WRITEBUFSIZE (OUTQSIZE/2)

#define INTP2_FIR_SIZE 38
#define INTP2_VALUE 2
#define INTP2_QUE_SIZE (INTP2_FIR_SIZE/INTP2_VALUE)

const float X2IntrpFIRCoef[INTP2_FIR_SIZE*2] = {
	0.004595199062f,	 0.003263078569f,	-0.012307513857f,	-0.023605770514f,	-0.004776493194f,
	 0.020455597889f,	 0.003416955875f,	-0.032096849071f,	-0.010936520267f,	 0.043997619201f,
	 0.021109718259f,	-0.061591694157f,	-0.038850736238f,	 0.088428267617f,	 0.071893731289f,
	-0.140300185169f,	-0.155068799286f,	 0.316698335862f,	 0.879226589556f,	 0.879226589556f,
	 0.316698335862f,	-0.155068799286f,	-0.140300185169f,	 0.071893731289f,	 0.088428267617f,
	-0.038850736238f,	-0.061591694157f,	 0.021109718259f,	 0.043997619201f,	-0.010936520267f,
	-0.032096849071f,	 0.003416955875f,	 0.020455597889f,	-0.004776493194f,	-0.023605770514f,
	-0.012307513857f,	 0.003263078569f,	 0.004595199062f,
//
	0.004595199062f,	 0.003263078569f,	-0.012307513857f,	-0.023605770514f,	-0.004776493194f,
	 0.020455597889f,	 0.003416955875f,	-0.032096849071f,	-0.010936520267f,	 0.043997619201f,
	 0.021109718259f,	-0.061591694157f,	-0.038850736238f,	 0.088428267617f,	 0.071893731289f,
	-0.140300185169f,	-0.155068799286f,	 0.316698335862f,	 0.879226589556f,	 0.879226589556f,
	 0.316698335862f,	-0.155068799286f,	-0.140300185169f,	 0.071893731289f,	 0.088428267617f,
	-0.038850736238f,	-0.061591694157f,	 0.021109718259f,	 0.043997619201f,	-0.010936520267f,
	-0.032096849071f,	 0.003416955875f,	 0.020455597889f,	-0.004776493194f,	-0.023605770514f,
	-0.012307513857f,	 0.003263078569f,	 0.004595199062f
};

#define INTP3_FIR_SIZE 24
#define INTP3_VALUE 3
#define INTP3_QUE_SIZE (INTP3_FIR_SIZE/INTP3_VALUE)
const float X3IntrpFIRCoef[INTP3_FIR_SIZE*2] = {
	0.011573982735f,	0.014334218226f,	0.006363947118f,   -0.021096865220f,   -0.062826223097f,
   -0.096581775049f,   -0.088551387570f,   -0.009298026286f,	0.146903362727f,	0.350809254265f,
	0.544080700435f,	0.661916011265f,	0.661916011265f,	0.544080700435f,	0.350809254265f,
	0.146903362727f,   -0.009298026286f,   -0.088551387570f,   -0.096581775049f,   -0.062826223097f,
   -0.021096865220f,	0.006363947118f,	0.014334218226f,	0.011573982735f,
//
	0.011573982735f,	0.014334218226f,	0.006363947118f,   -0.021096865220f,   -0.062826223097f,
   -0.096581775049f,   -0.088551387570f,   -0.009298026286f,	0.146903362727f,	0.350809254265f,
	0.544080700435f,	0.661916011265f,	0.661916011265f,	0.544080700435f,	0.350809254265f,
	0.146903362727f,   -0.009298026286f,   -0.088551387570f,   -0.096581775049f,   -0.062826223097f,
   -0.021096865220f,	0.006363947118f,	0.014334218226f,	0.011573982735f,
};


class CSoundOut : public CThreadWrapper
{
	Q_OBJECT
public:
	explicit CSoundOut();
	virtual ~CSoundOut();

	//Exposed functions
	void Start(int OutDevIndx){	emit StartSig(OutDevIndx);	}//starts soundcard output
	void Stop(){emit StopSig();}	//stops soundcard output
	void Reset();
	void PutOutQueue(int numsamples, qint16* pData );

	void SetVolume(qint32 vol);
	int GetRateError(){return (int)m_PpmError;}
	bool IsRunning(){ if(m_pAudioOutput) return (QAudio::ActiveState == m_pAudioOutput->state()); else return false;}

signals:
	void StartSig(int OutDevIndx);	//starts soundcard output
	void StopSig();	//stops soundcard output

private slots:
	void StartSlot(int OutDevIndx);	//starts soundcard output
	void StopSlot();	//stops soundcard output
	void GetNewData();
	void ThreadInit();	//override function is called by new thread when started
	void ThreadExit();	//override function is called by thread before exiting

protected:

private:
	void GetOutQueue(int numsamples, qint16* pData );
	int InterpolateX6(qint16* pIn, qint16* pOut, int n);

	void CalcError();

	QList<QAudioDeviceInfo> m_OutDevices;
	QAudioDeviceInfo  m_OutDeviceInfo;
	QAudioFormat m_OutAudioFormat;
	QAudioOutput* m_pAudioOutput;
	QIODevice* m_pIODevice;	// ptr to internal soundout IODevice

	bool m_Startup;
	char m_pData[SOUND_WRITEBUFSIZE];
	qint16 m_OutQueueMono[OUTQSIZE];
	qint16 m_InterpolatedOutput[OUTQSIZE*6];
	int m_OutQHead;
	int m_OutQTail;
	int m_RateUpdateCount;
	int m_OutQLevel;
	int m_PpmError;
	int m_periodSize;
	double m_Gain;
	double m_OutRatio;
	double m_AveOutQLevel;
	int m_Indx;
	int m_FirState2;
	int m_FirState3;
	float m_pQue2[INTP2_QUE_SIZE];
	float m_pQue3[INTP3_QUE_SIZE];
};
#endif // SOUNDOUT_H
