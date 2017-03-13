//////////////////////////////////////////////////////////////////////
// sdrinterface.h: interface for the CSdrInterface class.
//
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////
#ifndef SDRINTERFACE_H
#define SDRINTERFACE_H

#include <QObject>
#include <QString>
#include <QElapsedTimer>
#include "netio.h"
#include "soundout.h"
#include "soundin.h"
#include "dsp/G711.h"
#include "dsp/G726.h"
#include "dsp/fir.h"

#ifdef ENABLE_CODEC2
#include "freedv.h"
#endif

#define MAX_DATAPKT_LENGTH 3000
#define MAX_VIDEO_LENGTH 3000

#define FM_DEVIATION 5000

#define RATE_64 0
#define RATE_40 1
#define RATE_32 2
#define RATE_24 3
#define RATE_16 4


typedef  union
{
	struct
	{
		quint16 header;			// ASCP header length/type
		qint16 smeter;			// S meter in dB * 10
		quint8 comptype;		// Compression type
		quint8 Data[MAX_DATAPKT_LENGTH];
	} __attribute__ ((__packed__)) hdr;
	char Buf[MAX_DATAPKT_LENGTH+5];
}trxaudiodatapkt;

typedef  union
{
	struct
	{
		quint16 header;			// ASCP header length/type
		quint8 comptype;		// Compression type
		quint8 Data[MAX_DATAPKT_LENGTH];
	} __attribute__ ((__packed__)) hdr;
	char Buf[MAX_DATAPKT_LENGTH+3];
}ttxaudiodatapkt;

typedef  union
{
	struct
	{
		quint16 header;		// ASCP header length/type
		quint8 comptype;	// Compression type
		uint8_t Data[MAX_DATAPKT_LENGTH];
	} __attribute__ ((__packed__)) hdr;
	char Buf[MAX_DATAPKT_LENGTH+3];
}tvideodatapkt;

typedef struct
{
	int Fls;	//low shelf frequency
	int Gls;	//low shelf gain in dB
	int Fpk;	//Peak frequency
	int Gpk;	//peak gain in dB
	int Fhs;	//high shelf frequency
	int Ghs;	//High shelf gain in dB
}tEqualizer;

class CSdrInterface : public CNetio
{
	Q_OBJECT
public:
	CSdrInterface(QObject *parent = 0);
	virtual ~CSdrInterface();
	void StartAudioOut(int DevIndex);
	void StopAudioOut();
	void StartAudioIn(int DevIndex);
	void StopAudioIn();
	void SetSdrRunState(bool on);
	void GetInfo();
	void GetFftAvePwr();
	void SetVolume(qint32 vol);
	void SetRxFrequency(qint64 freq);
	void SetTxFrequency(qint64 freq);
	void SetDemodMode(quint32 mode);
	void SetDigitalMode(quint32 mode);
	void SetPTT(int TxState);
	void SetAtten(qint32 atten);
	void SetAgc(int Slope, int Thresh, int delay);
	void SetDemodFilter(int LowCut, int HighCut, int Offset);
	void SetSquelchThreshold(int Val);
	void SetAudioFilter(int val);
	void SetAudioCompressionMode(int Mode);
	void SetEqualizer(tEqualizer Params);
	void SetTxTestSignalMode(quint8 Mode, int Freq);
	void SetVideoCompressionMode(int Mode);
	void SetCtcssFreq(int Freq);
	void SendTxDigitalData(int Len, char* pBuf);
	void PerformNcoNull();
	void TryPW(QString RxPw, QString TxPw);
	void SendClientDesc(QString ClientDesc);
	void SetupFft(int xpoints, int Span, int MindB, int MaxdB, int Ave, int Rate );
	void GetFftData(quint32* Buf, int Length){for(int i=0; i<Length;i++) Buf[i] = m_VideoData[i];}
	void SendKeepalive();
	void ParseAscpMsg(CAscpRxMsg *pMsg);
	qint32 GetDataLatency() {return (qint32)m_CurrentLatency;}

    void SetAntenna(int antenna);

	int m_RxSpanMin;
	int m_RxSpanMax;
	int m_TxSpanMin;
	int m_TxSpanMax;
	int m_NumRxFreqRanges;
	int m_NumTxFreqRanges;
	qint64 m_pRxFrequencyRangeMin[4];
	qint64 m_pRxFrequencyRangeMax[4];
	qint64 m_pTxFrequencyRangeMin[4];
	qint64 m_pTxFrequencyRangeMax[4];
	QString m_SdrName;
	QString m_ServerName;
	QString m_SerialNumStr;
	
signals:
	void NewSMeterValue(qint16 Val);
	void NewFftAvePwr(qint16 Val);
	void NewVideoData();

public slots:
    void SetFreedvMode(const QString &mode_str)
    {
#ifdef ENABLE_CODEC2
        fdv->set_mode(mode_str);
#else
        (void) mode_str;
#endif
    }

private slots:
	void OnNewSoundDataRdySlot();
	
private:
	void DecodeAudioPacket( quint8* pInBuf, int Length);
	void DecodeVideoPacket(quint8 comptype, quint8* pInBuf, int Length);
	void SetupAudioDecompression();
	void SetupVideoDecompression(int Mode);

	quint8 m_VideoData[MAX_VIDEO_LENGTH];
	qint16 m_SoundOutBuf[MAX_DATAPKT_LENGTH];
	qint16 m_SoundInBuf[MAX_DATAPKT_LENGTH];
	qint32 m_CurrentLatency;
	bool m_TxUnlocked;
	bool m_TxActive;
	int m_MsgPos;
	int m_SoundcardOutIndex;
	int m_SoundcardInIndex;
	int m_AudioCompressionMode;
	int m_VideoCompressionMode;
	int m_CurLowCut;
	int m_CurHighCut;
	int m_CurOffset;
	int m_KeepAliveCount;
	int m_KeepAliveTimer;
	int m_MaxTxSamplesInPkt;
	int m_CurrentDemodMode;
	QString m_Str;
	G726 m_RxG726;
	G726 m_TxG726;
	G711 m_G711;
	CFir m_Fir;
	CSoundOut* m_pSoundOut;
	CSoundIn* m_pSoundIn;
	QObject* m_pParent;
	QElapsedTimer m_LatencyTimer;
	QMutex m_Mutex;		//for keeping threads from stomping on each other

#ifdef ENABLE_CODEC2
    // FreeDV stuff
    CFreedv   *fdv;
    qint16 m_SoundDvBuf[MAX_DATAPKT_LENGTH];
#endif
};

#endif // SDRINTERFACE_H
