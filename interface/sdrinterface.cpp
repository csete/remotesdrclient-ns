//////////////////////////////////////////////////////////////////////
// SdrInterface.cpp: implementation of the CSdrInterface class.
//
//  This class implements the TCP message processing.
//	Also compresses, formats, and sends the audio to the UDP port
//
// History:
//	2013-10-02  Initial creation MSW
//	2013-10-12  Extended frequency ranges added squelched audio data message
//	2014-03-15  Added compression mode byte to video and audio packets
//	2014-08-03  Fixed demod audio filter bug for SAM mode
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
#include "sdrinterface.h"
#include "interface/sdrprotocol.h"
#include "gui/chatdialog.h"
#include "gui/rawiqwidget.h"

//lookup table maps 4 bit value spectrum delta(0 to 15)
// to an 8 bit signed sort of anti log value
const int ANTILOGTBL[16] = {
	-1, 7, 15, 26, 42, 63, 91, 127, -128, -92, -80, -43, -28, -16, -6, -1,  //0000 to 000F
};

//Look-up table to get number of samples of un-compressed data to send on transmit pkt.
//Index into table is the compression mode
const int MAX_TXPACKET_BYTES[7] = {
	0,		//#define COMP_MODE_NOAUDIO 0
	512,	//#define COMP_MODE_RAW 1
	512,	//#define COMP_MODE_G711 2
	816,	//#define COMP_MODE_G726_40 3
	1024,	//#define COMP_MODE_G726_32 4
	1360,	//#define COMP_MODE_G726_24 5
	2048	//#define COMP_MODE_G726_16 6
};


#define TRYCONNECT_TIMELIMIT 5	//number of seconds to wait for a connect attempt
#define KEEPALIVE_TIME 2		//rate to send keepalive msg
#define KEEPALIVE_TRYLIMIT 3	//retry limit

#define MIN_RX_SPAN 1000
#define MAX_RX_SPAN 10000000
#define MIN_TX_SPAN 1000
#define MAX_TX_SPAN 40000

/////////////////////////////////////////////////////////////////////
// Constructor/Destructor
/////////////////////////////////////////////////////////////////////
CSdrInterface::CSdrInterface(QObject *parent) : m_pParent(parent)
{
	m_SdrStatus = SDR_OFF;
	m_MsgPos = 0;
	m_SoundcardOutIndex = 0;
	m_SoundcardInIndex = 0;
	m_KeepAliveTimer = 0;
	m_CurrentLatency = 0;
	m_RxSpanMin = MIN_TX_SPAN;
	m_RxSpanMax = MAX_RX_SPAN;
	m_TxSpanMin = 1000;
	m_TxSpanMax = 20000;
	m_CurrentDemodMode = DEMOD_MODE_AM;
	m_TxUnlocked = false;
	m_TxActive = false;
	m_pSoundOut = new CSoundOut;
	m_pSoundIn = new CSoundIn;
	for(int i=0; i<4; i++)
	{
		m_pRxFrequencyRangeMin[i] = 0;
		m_pRxFrequencyRangeMax[i] = 30000000;
	}
	m_AudioCompressionMode = COMP_MODE_NOAUDIO;
	SetupAudioDecompression();
	SetupVideoDecompression(COMP_MODE_NOVIDEO);

	connect(m_pSoundIn,SIGNAL( NewSoundDataRdy() ), this, SLOT( OnNewSoundDataRdySlot() ) );

qDebug()<<"CSdrInterface constructor";
}

CSdrInterface::~CSdrInterface()
{
	if(m_pSoundOut)
		delete m_pSoundOut;
	if(m_pSoundIn)
		delete m_pSoundIn;
}

////////////////////////////////////////////////////////////////////////
// Start/Stop Sound card output
////////////////////////////////////////////////////////////////////////
void CSdrInterface::StartAudioOut(int DevIndex)
{
	m_SoundcardOutIndex = DevIndex;
	if(!m_pSoundOut->IsRunning() )
		m_pSoundOut->Start(DevIndex);
}
void CSdrInterface::StopAudioOut()
{
	m_pSoundOut->Stop();
}

////////////////////////////////////////////////////////////////////////
// Start/Stop Sound card input
////////////////////////////////////////////////////////////////////////
void CSdrInterface::StartAudioIn(int DevIndex)
{
	if(MOD_MODE_DIG != m_CurrentDemodMode)
	{
		m_pSoundIn->Start(DevIndex);
		m_SoundcardInIndex = DevIndex;
	}
}
void CSdrInterface::StopAudioIn()
{
	m_pSoundIn->Stop();
}

////////////////////////////////////////////////////////////////////////
// Send messages to SDR to obtain a bunch of intial information
// like name serial number etc.
////////////////////////////////////////////////////////////////////////
void CSdrInterface::GetInfo()
{
CAscpTxMsg TxAscpMsg;
	m_SdrName = "";
	m_ServerName = "";
	m_SerialNumStr = "";

	TxAscpMsg.InitTxMsg(TYPE_HOST_REQ_CITEM);
	TxAscpMsg.AddCItem(CI_GENERAL_INTERFACE_NAME);
	SendAscpMsg(&TxAscpMsg);

	TxAscpMsg.InitTxMsg(TYPE_HOST_REQ_CITEM);
	TxAscpMsg.AddCItem(CI_GENERAL_INTERFACE_SERIALNUM);
	SendAscpMsg(&TxAscpMsg);

	TxAscpMsg.InitTxMsg(TYPE_HOST_REQ_CITEM);
	TxAscpMsg.AddCItem(CI_GENERAL_CUSTOM_NAME);
	SendAscpMsg(&TxAscpMsg);

	TxAscpMsg.InitTxMsg(TYPE_HOST_REQ_CITEM);
	TxAscpMsg.AddCItem(CI_SPECTRUM_RANGE);
	SendAscpMsg(&TxAscpMsg);

	TxAscpMsg.InitTxMsg(TYPE_HOST_REQ_CITEM_RANGE);
	TxAscpMsg.AddCItem(CI_RX_FREQUENCY);
	TxAscpMsg.AddParm8(0);
	SendAscpMsg(&TxAscpMsg);

	TxAscpMsg.InitTxMsg(TYPE_HOST_REQ_CITEM_RANGE);
	TxAscpMsg.AddCItem(CI_TX_FREQUENCY);
	TxAscpMsg.AddParm8(0);
	SendAscpMsg(&TxAscpMsg);

	m_CurLowCut = 0;
	m_CurHighCut = 0;
	m_CurOffset = 0;
	m_RxSpanMin = 10000;
	m_RxSpanMax = 50000;
}

////////////////////////////////////////////////////////////////////////
// Send password unlock msg's
////////////////////////////////////////////////////////////////////////
void CSdrInterface::TryPW(QString RxPw, QString TxPw)
{
CAscpTxMsg TxAscpMsg;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_PW_UNLOCK);
	for(int i=0; i<RxPw.length(); i++)
		TxAscpMsg.AddParm8( (quint8)RxPw[i].toLatin1() );
	TxAscpMsg.AddParm8(0);	//null terminate string
	SendAscpMsg(&TxAscpMsg);

	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_TX_PW_UNLOCK);
	for(int i=0; i<TxPw.length(); i++)
		TxAscpMsg.AddParm8( (quint8)TxPw[i].toLatin1() );
	TxAscpMsg.AddParm8(0);	//null terminate string
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr receiver On/Off msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetSdrRunState(bool on)
{
CAscpTxMsg TxAscpMsg;
	m_CurrentLatency = 0;

	m_KeepAliveTimer = 0;
	m_KeepAliveCount = 0;

	m_TxUnlocked = false;

	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_STATE);
	TxAscpMsg.AddParm8(RX_STATE_DATAREAL);
	if(on)
	{
		TxAscpMsg.AddParm8(RX_STATE_ON);
	}
	else
	{
		TxAscpMsg.AddParm8(RX_STATE_IDLE);
	}
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm8(0);
	SendAscpMsg(&TxAscpMsg);
	qDebug()<<"Run";
}

////////////////////////////////////////////////////////////////////////
// Send sdr transmiter On/Off msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetPTT(int TxState)
{
CAscpTxMsg TxAscpMsg;
qDebug()<<"SetPTT "<<TxState;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_TX_STATE);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm8((quint8)TxState);
	if(CI_TX_STATE_ON == TxState)
		m_TxActive = true;
	else
		m_TxActive = false;
	SendAscpMsg(&TxAscpMsg);
	m_pSoundIn->Reset();
	m_pSoundOut->Reset();
	m_TxG726.Reset();
}

////////////////////////////////////////////////////////////////////////
// Send sdr keepalive msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SendKeepalive()
{
CAscpTxMsg TxAscpMsg;
	if(--m_KeepAliveTimer <= 0)
	{
		m_KeepAliveTimer = KEEPALIVE_TIME;

		m_LatencyTimer.start();
		TxAscpMsg.InitTxMsg(TYPE_HOST_REQ_CITEM);
		TxAscpMsg.AddCItem(CI_GENERAL_STATUS_CODE);
		SendAscpMsg(&TxAscpMsg);

		m_KeepAliveCount++;
		if(m_KeepAliveCount > KEEPALIVE_TRYLIMIT)
		{	//if keepalive timed out due to no response
			DisconnectFromServer(SDR_DISCONNECT_TIMEOUT);
		}
//qDebug()<<"Send "<<m_KeepAliveCount;
	}
}

////////////////////////////////////////////////////////////////////////
// Send ClientDesc string to the conencted server
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SendClientDesc(QString ClientDesc)
{
CAscpTxMsg TxAscpMsg;
	if(ClientDesc.length() > 80)
		return;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_GENERAL_LASTCLIENT_INFO);
	for(int i=0; i<ClientDesc.length(); i++)
		TxAscpMsg.AddParm8( (quint8)ClientDesc[i].toLatin1() );
	TxAscpMsg.AddParm8(0);	//null terminate string
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr volume msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetVolume(qint32 vol)
{
CAscpTxMsg TxAscpMsg;
	m_pSoundOut->SetVolume(vol);
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_AF_GAIN);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm8((quint8)vol);
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr rx frequency msgs
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetRxFrequency(qint64 freq)
{
CAscpTxMsg TxAscpMsg;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_FREQUENCY);
	TxAscpMsg.AddParm8( 0 ) ;
	TxAscpMsg.AddParm32( (quint32)(freq&0xFFFFFFFF) );
	TxAscpMsg.AddParm8( (quint8)(freq>>32) ) ;
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr tx frequency msgs
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetTxFrequency(qint64 freq)
{
CAscpTxMsg TxAscpMsg;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_TX_FREQUENCY);
	TxAscpMsg.AddParm8( 0 ) ;
	TxAscpMsg.AddParm32( (quint32)(freq&0xFFFFFFFF) );
	TxAscpMsg.AddParm8( (quint8)(freq>>32) ) ;
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr demodulation/modulation mode msgs
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetDemodMode(quint32 mode)
{
CAscpTxMsg TxAscpMsg;
	m_CurrentDemodMode = mode;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_DEMOD_MODE);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm8((quint8)mode);
	SendAscpMsg(&TxAscpMsg);
//send same mode for transmitter
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_TX_MOD_MODE);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm8((quint8)mode);
	SendAscpMsg(&TxAscpMsg);
	if(DEMOD_MODE_DIG == mode)
		SetDigitalMode(DIGITAL_MODE_BPSK31);
}

////////////////////////////////////////////////////////////////////////
// Send sdr digital mode msgs
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetDigitalMode(quint32 mode)
{
CAscpTxMsg TxAscpMsg;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_DIGITAL_MODE);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm8((quint8)mode);
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr digital mode tx data msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SendTxDigitalData(int Len, char* pBuf)
{
CAscpTxMsg TxAscpMsg;
	TxAscpMsg.InitTxMsg(TYPE_TARG_DATA_ITEM3);
	TxAscpMsg.AddParm8(DIGDATA_TYPE_TXCHAR);
	for(int i=0; i<Len; i++)
		TxAscpMsg.AddParm8(pBuf[i]);
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr rx attenuator msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetAtten(qint32 atten)
{
CAscpTxMsg TxAscpMsg;
qint8 bg;
	//limit values to 0, -10, -20, and -30
	if( atten > -5 )
		atten = 0;
	else if( atten > -15 )
		atten = -10;
	else if( atten > -25 )
		atten = -20;
	else
		atten = -30;
	bg = (qint8)atten;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_RF_GAIN);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm8(bg);
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr rx AGC parameters msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetAgc(int Slope, int Thresh, int delay)
{
CAscpTxMsg TxAscpMsg;
qint8 sb;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_AGC);
	TxAscpMsg.AddParm8(0);
	sb = (qint8)Thresh;
	TxAscpMsg.AddParm8((quint8)sb);
	sb = (qint8)Slope;
	TxAscpMsg.AddParm8((quint8)sb);
	TxAscpMsg.AddParm16((quint16)delay);
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr rx/tx audio compression msgs
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetAudioCompressionMode(int Mode)
{
CAscpTxMsg TxAscpMsg;
//	SetupAudioDecompression(Mode);	//now compressionmode is in data packet

	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_AUDIO_COMPRESSION);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm8((quint8)Mode);
	SendAscpMsg(&TxAscpMsg);

	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_TX_AUDIO_COMPRESSION);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm8((quint8)Mode);
	SendAscpMsg(&TxAscpMsg);

}

////////////////////////////////////////////////////////////////////////
// Send sdr video compression mode msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetVideoCompressionMode(int Mode)
{
	CAscpTxMsg TxAscpMsg;
//	SetupVideoDecompression(Mode);	//now compressionmode is in data packet
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_VIDEO_COMPRESSION);
	TxAscpMsg.AddParm8(CVIDEO_MODE_RXTX);
	TxAscpMsg.AddParm8((quint8)Mode);
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr rx squelch Threshold msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetSquelchThreshold(int val)
{
CAscpTxMsg TxAscpMsg;
	if(val < CI_RX_SQUELCH_THRESH_MIN)
		val = CI_RX_SQUELCH_THRESH_MIN;
	if(val > CI_RX_SQUELCH_THRESH_MAX)
		val = CI_RX_SQUELCH_THRESH_MAX;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_SQUELCH_THRESH);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm16((qint16)val);
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr rx HP audio filter msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetAudioFilter(int val)
{
CAscpTxMsg TxAscpMsg;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_AUDIO_FILTER);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm8((quint8)val);
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr rx demodulator filter parameters msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetDemodFilter(int LowCut, int HighCut, int Offset)
{
CAscpTxMsg TxAscpMsg;
	if( (m_CurLowCut == LowCut) &&
		(m_CurHighCut == HighCut ) &&
		(m_CurOffset == Offset ) )
		return;		//do nothing if no changes
	m_CurLowCut = LowCut;
	m_CurHighCut = HighCut;
	m_CurOffset = Offset;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_DEMOD_FILTER);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm16((quint16)( (qint16)LowCut) );
	TxAscpMsg.AddParm16((quint16)( (qint16)HighCut) );
	TxAscpMsg.AddParm16((quint16)Offset);
	SendAscpMsg(&TxAscpMsg);
	// Create audio lowpass/bandpass post filter to limit audio BW to the
	//same as the receiver filtering.  This is important to limit the
	//high frequency noise created by the audio de-compression of narrow bandwidths
	//such as when CW filters are used.
	TYPEREAL fc;
	//figure out audio post filter limts based on receiver demod filter settings
	if( (LowCut==HighCut) && ( (LowCut*HighCut) < 0) )
	{	//is a symetric filter so use |H-L|/2
		fc = abs(HighCut-LowCut)/2.0;
	}
	else
	{	//is sideband filter so use max(H,L)
		if( abs(HighCut) > abs(LowCut) )
			fc = abs(HighCut);
		else
			fc = abs(LowCut);
	}
	if(fc > 3900.0)
		fc = 3900.0;
	if(fc < 100.0)
		fc = 700.0;
	m_Fir.CreateLPFilter(0, 1.0, 50.0, fc, fc+200.0, fabs(Offset),  8000.0);
}

////////////////////////////////////////////////////////////////////////
// Send sdr Spectrum display parameters msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetupFft(int Xpoints, int Span, int MaxdB, int MindB, int Ave, int Rate)
{
CAscpTxMsg TxAscpMsg;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_SPECTRUM_SETTINGS);
	TxAscpMsg.AddParm32( (quint32)Span );
	TxAscpMsg.AddParm16( (quint16)Xpoints );
	TxAscpMsg.AddParm16( (qint16)MaxdB );
	TxAscpMsg.AddParm16( (qint16)MindB );
	TxAscpMsg.AddParm8( (quint8)Ave );
	TxAscpMsg.AddParm8( (quint8)Rate );
	SendAscpMsg(&TxAscpMsg);
//qDebug()<<Xpoints << Span << MindB << MaxdB << Ave << Rate;
}

////////////////////////////////////////////////////////////////////////
// Send sdr Tx Audio equalizer settings msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetEqualizer(tEqualizer Params)
{
CAscpTxMsg TxAscpMsg;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_TX_EQUALIZER_SETTINGS);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm8((qint8)Params.Gls);
	TxAscpMsg.AddParm16( (qint16)Params.Fls );
	TxAscpMsg.AddParm8((qint8)Params.Gpk);
	TxAscpMsg.AddParm16( (qint16)Params.Fpk );
	TxAscpMsg.AddParm8((qint8)Params.Ghs);
	TxAscpMsg.AddParm16( (qint16)Params.Fhs );
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr Tx test mode settings msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetTxTestSignalMode(quint8 Mode, int Freq)
{
CAscpTxMsg TxAscpMsg;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_TX_TESTSIGNAL_MODE);
	TxAscpMsg.AddParm8(Mode);
	TxAscpMsg.AddParm16((quint16)Freq);
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send sdr Tx FM CTCSS frequency msg
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetCtcssFreq(int Freq)
{
CAscpTxMsg TxAscpMsg;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_TX_FM_PARAMS);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm16(FM_DEVIATION);
	TxAscpMsg.AddParm16((quint16)Freq);
	SendAscpMsg(&TxAscpMsg);
}

////////////////////////////////////////////////////////////////////////
// Send Get FFT Ave Pwr message command
////////////////////////////////////////////////////////////////////////
void CSdrInterface::GetFftAvePwr()
{
CAscpTxMsg TxAscpMsg;
	TxAscpMsg.InitTxMsg(TYPE_HOST_REQ_CITEM);
	TxAscpMsg.AddCItem(CI_SPECTRUM_AVEPWR);
	SendAscpMsg(&TxAscpMsg);
qDebug()<<"Send REQ FFT Ave Pwr";
}

////////////////////////////////////////////////////////////////////////
// Send NCO Null command
////////////////////////////////////////////////////////////////////////
void CSdrInterface::PerformNcoNull()
{
CAscpTxMsg TxAscpMsg;
	TxAscpMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	TxAscpMsg.AddCItem(CI_RX_PERFORM_CAL);
	TxAscpMsg.AddParm8(0);
	TxAscpMsg.AddParm8(RX_PERFORM_CAL_NCONULL);
	SendAscpMsg(&TxAscpMsg);
qDebug()<<"Send CAl";
}

////////////////////////////////////////////////////////////////////////
//  Called with new TCP message to parse from client thread context
////////////////////////////////////////////////////////////////////////
void CSdrInterface::ParseAscpMsg(CAscpRxMsg *pMsg)
{
CAscpTxMsg TxAscpMsg;
trxaudiodatapkt* aptr;
tvideodatapkt* vptr;
int i;
int length = pMsg->GetLength();
quint16 Ci;
//qDebug()<<"Got Msg";
	pMsg->InitRxMsg();	//initialize receive msg object for read back
	if(pMsg->GetLength() > MAX_DATAPKT_LENGTH)
	{	//errant message so close
		DisconnectFromServer(SDR_DISCONNECT_TIMEOUT);
		return;
	}
	if( pMsg->GetType() == TYPE_TARG_RESP_CITEM )
	{	// Is a message from Server in response to a request
		m_KeepAliveCount = 0;
		switch(Ci=pMsg->GetCItem())
		{
			case CI_GENERAL_INTERFACE_NAME:
				m_SdrName = (const char*)(&pMsg->Buf8[4]);
				qDebug()<<m_SdrName;
				break;
			case CI_GENERAL_INTERFACE_SERIALNUM:
				m_SerialNumStr = (const char*)(&pMsg->Buf8[4]);
				qDebug()<<m_SerialNumStr;
				break;
			case CI_GENERAL_CUSTOM_NAME:
				m_ServerName = (const char*)(&pMsg->Buf8[4]);
				qDebug()<<m_ServerName;
				break;
			case CI_RX_STATE:
				qDebug()<<m_CurrentLatency;
				pMsg->GetParm8();
				if(RX_STATE_ON == pMsg->GetParm8() )
				{
					m_SdrStatus = SDR_RECEIVING;
					emit NewSdrStatus(m_SdrStatus);
				}
				break;
			case CI_GENERAL_INTERFACE_VERSION:
				break;
			case CI_GENERAL_HARDFIRM_VERSION:
				break;
			case CI_GENERAL_STATUS_CODE:
				m_CurrentLatency = (qint32)m_LatencyTimer.elapsed();
				break;
			case CI_RX_FREQUENCY:
				pMsg->GetParm8();
qDebug()<<"f = "<<pMsg->GetParm32();
				break;
			case CI_GENERAL_PRODUCT_ID:
				break;
			case CI_RX_PW_UNLOCK:
				m_Str = (const char*)(&pMsg->Buf8[4]);
				if( 0xFF == pMsg->Buf8[4] )
				{
					m_SdrStatus = SDR_DISCONNECT_PWERROR;
				}
				else
				{
					m_SdrStatus = SDR_PWOK;
					qDebug()<<"rx Pw = "<<m_Str;
				}
				emit NewSdrStatus(m_SdrStatus);
				break;
			case CI_TX_STATE:
				pMsg->GetParm8();
				if( CI_TX_STATE_ON == pMsg->GetParm8() )
				{
					m_SdrStatus = SDR_TRANSMITTING;
					emit NewSdrStatus(m_SdrStatus);
				}
				else
				{
					m_TxActive = false;
					if(SDR_TRANSMITTING == m_SdrStatus )
					{
						m_SdrStatus = SDR_RECEIVING;
						emit NewSdrStatus(m_SdrStatus);
					}
				}
qDebug()<<"TX_STATE "<<m_SdrStatus;
				break;
			case CI_TX_PW_UNLOCK:
				m_Str = (const char*)(&pMsg->Buf8[4]);
				if( "" != m_Str)	//if PW returned NULL then was not a match
					m_TxUnlocked = true;
				else
					m_TxUnlocked = false;
				qDebug()<<"tx Pw = "<<m_Str;
				break;
			case CI_SPECTRUM_RANGE:
				m_RxSpanMin =	pMsg->GetParm32();
				m_RxSpanMax =	pMsg->GetParm32();
				//Clamp ranges
				if( (m_RxSpanMin < MIN_RX_SPAN) || (m_RxSpanMin >= MAX_RX_SPAN))
					m_RxSpanMin = MIN_RX_SPAN;
				if( (m_RxSpanMax < MIN_RX_SPAN) || (m_RxSpanMax > MAX_RX_SPAN))
					m_RxSpanMax = MAX_RX_SPAN;
				m_TxSpanMin = pMsg->GetParm32();
				m_TxSpanMax = pMsg->GetParm32();
				if( (m_TxSpanMin < MIN_TX_SPAN) || (m_TxSpanMin >= MAX_TX_SPAN))
					m_TxSpanMin = MIN_TX_SPAN;
				if( (m_TxSpanMax < MIN_TX_SPAN) || (m_TxSpanMax > MAX_TX_SPAN))
					m_TxSpanMax = MAX_TX_SPAN;
qDebug()<<"rx span limits "<<m_RxSpanMin << m_RxSpanMax;
qDebug()<<"tx span limits "<<m_TxSpanMin << m_TxSpanMax;
				break;
			case CI_SPECTRUM_AVEPWR:
				emit NewFftAvePwr(pMsg->GetParm16());
				break;
			case CI_RX_AUDIO_COMPRESSION:
				pMsg->GetParm8();	//ignor chan num
				//if NO Audio is selected then shut off sound card output
				if(	pMsg->GetParm8() == COMP_MODE_NOAUDIO)
				{
					m_AudioCompressionMode = COMP_MODE_NOAUDIO;
					if( m_pSoundOut->IsRunning() )
						StopAudioOut();
				}
				break;
			default:
//qDebug()<<"Got Unknown "<<Ci;
				break;
		}
	}
	else if( pMsg->GetType() == TYPE_TARG_RESP_CITEM_RANGE )
	{	//Is a response to a range request message
		if(CI_RX_FREQUENCY == pMsg->GetCItem() )
		{
			pMsg->GetParm8();	//ignor channel
			m_NumRxFreqRanges = pMsg->GetParm8();
qDebug("Num Rx Ranges=%u\n\r", m_NumRxFreqRanges );
			for(i=0; i<m_NumRxFreqRanges; i++)
			{
				m_pRxFrequencyRangeMin[i] =  pMsg->GetParm32();//get bottom 4 bytes of min frequency
				qint64 tmp = (qint64)pMsg->GetParm8();	//get upper msbyte
				m_pRxFrequencyRangeMin[i] = m_pRxFrequencyRangeMin[i] + (tmp<<32);
				m_pRxFrequencyRangeMax[i] =  pMsg->GetParm32();//get bottom 4 bytes of max frequency and add upper byte
				tmp = (qint64)pMsg->GetParm8();	//get upper msbyte
				m_pRxFrequencyRangeMax[i] = m_pRxFrequencyRangeMax[i] + (tmp<<32);
qDebug()<<"Rx Range Fmin = "<<m_pRxFrequencyRangeMin[i];
qDebug()<<"Rx Range Fmax = "<<m_pRxFrequencyRangeMax[i];
			}
		}
		else if(CI_TX_FREQUENCY == pMsg->GetCItem() )
		{
			pMsg->GetParm8();	//ignor channel
			m_NumTxFreqRanges = pMsg->GetParm8();
qDebug("Num Tx Ranges=%u\n\r", m_NumTxFreqRanges );
			for(i=0; i<m_NumTxFreqRanges; i++)
			{
				m_pTxFrequencyRangeMin[i] =  pMsg->GetParm32();//get bottom 4 bytes of min frequency
				qint64 tmp = (qint64)pMsg->GetParm8();	//get upper msbyte
				m_pTxFrequencyRangeMin[i] = m_pTxFrequencyRangeMin[i] + (tmp<<32);
				m_pTxFrequencyRangeMax[i] =  pMsg->GetParm32();//get bottom 4 bytes of max frequency and add upper byte
				tmp = (qint64)pMsg->GetParm8();	//get upper msbyte
				m_pTxFrequencyRangeMax[i] = m_pTxFrequencyRangeMax[i] + (tmp<<32);
qDebug()<<"Tx Range Fmin = "<<m_pTxFrequencyRangeMin[i];
qDebug()<<"Tx Range Fmax = "<<m_pTxFrequencyRangeMax[i];
			}
		}

	}
	else if( pMsg->GetType() == TYPE_TARG_UNSOLICITED_CITEM )
	{	//got unsolicited msg from server
		if( CI_GENERAL_STATUS_CODE == pMsg->GetCItem() )
		{
		}
	}
	// decode all data item messages
	else if( pMsg->GetType() == TYPE_TARG_DATA_ITEM0 )
	{	// rx audio data msg from sdr
		aptr = (trxaudiodatapkt*)pMsg->Buf8;
		if( aptr->hdr.comptype != m_AudioCompressionMode)
		{	//if compression mode changed
			m_AudioCompressionMode = aptr->hdr.comptype;
			if( m_AudioCompressionMode < COMP_MODE_RAW_16000)
			{
				SetupAudioDecompression();
			}
			else
			{
				if( m_pSoundOut->IsRunning())
					StopAudioOut();
			}
		}
		if( m_AudioCompressionMode < COMP_MODE_RAW_16000)
		{
			m_Mutex.lock();
			DecodeAudioPacket(aptr->hdr.Data, length-5);
			m_Mutex.unlock();
		}
		else
		{
			m_Mutex.lock();
			g_pRawIQWidget->ProccessRawIQData((qint8*)aptr->hdr.Data,  (int)((aptr->hdr.header&0x7FFF) - 5));
			m_Mutex.unlock();
		}
		if(!m_TxActive)
			emit NewSMeterValue(aptr->hdr.smeter);
	}
	else if( pMsg->GetType() == TYPE_TARG_DATA_ITEM1 )
	{	//video data msg from sdr
		vptr = (tvideodatapkt*)pMsg->Buf8;
		m_Mutex.lock();
		DecodeVideoPacket(vptr->hdr.comptype, vptr->hdr.Data, length-3);
		m_Mutex.unlock();
		emit NewVideoData();
	}
	else if( pMsg->GetType() == TYPE_TARG_DATA_ITEM2 )
	{
	}
	else if( pMsg->GetType() == TYPE_TARG_DATA_ITEM3 )
	{	//rx data from server
		m_Mutex.lock();
		if( DIGDATA_TYPE_RXCHAR == pMsg->Buf8[2] )
			emit g_pChatDialog->SendRxChatData(pMsg->Buf8[3]);
		if( DIGDATA_TYPE_TXECHO == pMsg->Buf8[2] )
			emit g_pChatDialog->SendRxChatData(pMsg->Buf8[3]);
		m_Mutex.unlock();
	}
	else if(pMsg->GetType() == TYPE_DATA_ITEM_ACK)
	{
	}
}

////////////////////////////////////////////////////////////////////////
// call to set the video compression mode
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetupVideoDecompression(int Mode)
{
	m_VideoCompressionMode = Mode;
//	qDebug()<<"Vid COmp = "<<Mode;
}

////////////////////////////////////////////////////////////////////////
// call to set the audio compression mode
////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetupAudioDecompression()
{
	m_RxG726.Reset();
	m_TxG726.Reset();
	if(COMP_MODE_NOAUDIO == m_AudioCompressionMode )
	{
		if( m_pSoundOut->IsRunning())
			StopAudioOut();
	}
	else
	{
		if( !m_pSoundOut->IsRunning())
			m_pSoundOut->Start(m_SoundcardOutIndex);
	}
	if(COMP_MODE_G726_40 == m_AudioCompressionMode)
	{
		m_RxG726.SetRate(G726::Rate40kBits);	//5 bits/sample
		m_TxG726.SetRate(G726::Rate40kBits);	//5 bits/sample
	}
	else if(COMP_MODE_G726_32 == m_AudioCompressionMode)
	{
		m_RxG726.SetRate(G726::Rate32kBits);	//4 bits/sample
		m_TxG726.SetRate(G726::Rate32kBits);	//4 bits/sample
	}
	else if(COMP_MODE_G726_24 == m_AudioCompressionMode)
	{
		m_RxG726.SetRate(G726::Rate24kBits);	//3 bits/sample
		m_TxG726.SetRate(G726::Rate24kBits);	//3 bits/sample
	}
	else if(COMP_MODE_G726_16 == m_AudioCompressionMode)
	{
		m_RxG726.SetRate(G726::Rate16kBits);	//2 bits/sample
		m_TxG726.SetRate(G726::Rate16kBits);	//2 bits/sample
	}
	m_MaxTxSamplesInPkt = MAX_TXPACKET_BYTES[m_AudioCompressionMode];
}

////////////////////////////////////////////////////////////////////////
// call to decode the audio data packet
//if length is == 2, then it is a squelched audio packet so the 2 data
// bytes specify how many Zero samples to send to the sound card.
////////////////////////////////////////////////////////////////////////
void CSdrInterface::DecodeAudioPacket( quint8* pInBuf, int Length)
{
int n = 0;
//qDebug()<<Length;
	if( 2 == Length )
	{	//is a squelch packet so fill in pInBuf[1,0] samples with zero
		n = ( (quint32)pInBuf[1] << 8 )+ (quint32)pInBuf[0];
		for(int i=0; i<n; i++)
			m_SoundOutBuf[i] = 0;
	}
	else if(COMP_MODE_RAW ==m_AudioCompressionMode)
	{
		for(int i=0; i<Length; i++)
			m_SoundOutBuf[i] = (qint16)pInBuf[i] << 8;
		n = Length;
	}
	else if(COMP_MODE_G711 == m_AudioCompressionMode)
	{
		n = m_G711.ULawDecode(m_SoundOutBuf, pInBuf, Length) / 2;
	}
	else
	{
		n = m_RxG726.Decode(m_SoundOutBuf, pInBuf, 0, Length*8 ) / 2;
	}
	if( m_pSoundOut->IsRunning())
	{
		if(m_TxActive)
		{
			for(int i=0; i<n; i++)
				m_SoundOutBuf[i] = 0;
		}
		else
		{	//process post audio filter to get rid of noise created by
			//audio de-compression on narrow BW reeciver settings
			m_Fir.ProcessFilter(n, m_SoundOutBuf, m_SoundOutBuf);
		}
		m_pSoundOut->PutOutQueue(n, m_SoundOutBuf);
	}
}

/////////////////////////////////////////////////////////////////
//	Slot Called by Sound Card Input when new Tx audio data is available
/////////////////////////////////////////////////////////////////
void  CSdrInterface::OnNewSoundDataRdySlot()
{
	ttxaudiodatapkt TxDataPacket;
	if(!m_TxActive || (COMP_MODE_NOAUDIO == m_AudioCompressionMode) )
		return;

	TxDataPacket.hdr.comptype = m_AudioCompressionMode;
	int availsamples = m_pSoundIn->GetInQueueSampsAvailable();
	while(m_MaxTxSamplesInPkt <= availsamples)
	{
		availsamples = m_pSoundIn->GetInQueue(m_MaxTxSamplesInPkt, m_SoundInBuf);
		int n = m_MaxTxSamplesInPkt;
		if(COMP_MODE_RAW == m_AudioCompressionMode)
		{
			for(int i=0; i<m_MaxTxSamplesInPkt; i++)
				TxDataPacket.hdr.Data[i] = (m_SoundInBuf[i]>>8)& 0xFF;
		}
		else if(COMP_MODE_G711 == m_AudioCompressionMode)
		{
			n = m_G711.ULawEncode(TxDataPacket.hdr.Data, m_SoundInBuf, m_MaxTxSamplesInPkt*2 );
		}
		else
		{
			n = m_TxG726.Encode(TxDataPacket.hdr.Data, 0, m_SoundInBuf, m_MaxTxSamplesInPkt*2)/8;
		}
		TxDataPacket.hdr.header = (n+3) | (TYPE_TARG_DATA_ITEM0<<8);
		SendAscpMsg((CAscpTxMsg*)&TxDataPacket);
		emit NewSMeterValue(m_pSoundIn->GetInputLevel());
qDebug()<<availsamples << n <<m_MaxTxSamplesInPkt;
	}
}

void CSdrInterface::DecodeVideoPacket(quint8 comptype, quint8* pInBuf, int Length)
{
int j=0;
int preval = 0;
int curval;
quint8 tmp;

//qDebug()<<"L="<<Length;
	if(comptype != m_VideoCompressionMode)
		SetupVideoDecompression(comptype);
	if(COMP_MODE_8BIT == m_VideoCompressionMode)
	{
		for(int i=0; i<Length; i++)
			m_VideoData[i] = pInBuf[i];
		for(int i=Length; i<MAX_VIDEO_LENGTH; i++)
			m_VideoData[i] = 0;
	}
	else if(COMP_MODE_4BIT == m_VideoCompressionMode )
	{
		for(int i=0; i<Length; i++)
		{
			tmp = pInBuf[i];
			curval = ANTILOGTBL[tmp&0x0F] + preval;
			if(curval > 255) curval = 255;
			if(curval < 0) curval = 0;
			preval = curval;
			m_VideoData[j++] = (quint8)(curval&0xFF);

			curval = ANTILOGTBL[tmp >> 4] + preval;
			if(curval > 255) curval = 255;
			if(curval < 0) curval = 0;
			preval = curval;
			m_VideoData[j++] = (quint8)(curval&0xFF);
		}
	}
}



