/////////////////////////////////////////////////////////////////////
// soundin.cpp: implementation of the CSoundIn class.
//
//	This class implements a class to get data from a soundcard.
//
// History:
//	2013-10-02  Initial creation MSW
//	2013-10-12  Fixed bug when no soundcard
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
#include "soundin.h"
#include <QDebug>
#include <math.h>

/*---------------------------------------------------------------------------*/
/*--------------------> L O C A L   D E F I N E S <--------------------------*/
/*---------------------------------------------------------------------------*/
#define SOUNDCARD_INRATE 48000	//output soundcard sample rate

//#define TEST_TONE 1			//un comment for test tone
#define TEST_FREQ 400
#define TEST_AMP 32760
#define K_2PI 6.283185307

/////////////////////////////////////////////////////////////////////
//   constructor/destructor
/////////////////////////////////////////////////////////////////////
CSoundIn::CSoundIn()
{
	m_pIODevice = NULL;
	m_pAudioInput = NULL;
	m_InAudioFormat.setSampleRate(SOUNDCARD_INRATE);
	m_SampCnt = 0;
	m_Dec3State = DEC3TAP_LENGTH-1;
	for(int i=0; i<DEC2TAP_LENGTH ;i++)
		m_Dec2FirBuf[i] = 0.0;
	for(int i=0; i<DEC3TAP_LENGTH ;i++)
		m_Dec3FirBuf[i] = 0.0;
//	qDebug()<<"CSoundIn constructor";

	m_ToneInc = K_2PI*(double)TEST_FREQ/SOUNDCARD_INRATE;
	m_ToneTime = 0.0;
}

CSoundIn::~CSoundIn()
{
//	qDebug()<<"CSoundIn destructor";
}

//////////////////////////////////////////////////////////////////////////
//Called when worker thread starts to initialize things
//////////////////////////////////////////////////////////////////////////
void CSoundIn::ThreadInit()	//overrided funciton is called by new thread when started
{
	connect(this,SIGNAL( StartSig(int)), this, SLOT(StartSlot(int)) );
	connect(this,SIGNAL( StopSig()), this, SLOT(StopSlot()) );
//qDebug()<<"Soundin Thread Init "<<this->thread()->currentThread();
}

/////////////////////////////////////////////////////////////////////
// Called by this worker thread to cleanup after itself
/////////////////////////////////////////////////////////////////////
void CSoundIn::ThreadExit()
{
	StopSlot();
}


/////////////////////////////////////////////////////////////////////
// Starts up soundcard output thread using soundcard at list OutDevIndx
/////////////////////////////////////////////////////////////////////
void CSoundIn::StartSlot(int InDevIndx)
{
QAudioDeviceInfo  DeviceInfo;
	//Get required soundcard from list
	m_InDevices = DeviceInfo.availableDevices(QAudio::AudioInput);
	if(m_InDevices.isEmpty())
	{
		qDebug()<<"Soundcard not found";
		return;
	}
	if(InDevIndx >= m_InDevices.count())
		InDevIndx = m_InDevices.count()-1;
	m_InDeviceInfo = m_InDevices.at(InDevIndx);

	//Setup fixed format for sound ouput
	m_InAudioFormat.setCodec("audio/pcm");
	m_InAudioFormat.setSampleRate(SOUNDCARD_INRATE);
	m_InAudioFormat.setSampleSize(16);
	m_InAudioFormat.setSampleType(QAudioFormat::SignedInt);
	m_InAudioFormat.setByteOrder(QAudioFormat::LittleEndian);
	m_InAudioFormat.setChannelCount(1);
	m_pThread->setPriority(QThread::HighestPriority);

	m_pAudioInput = new QAudioInput(m_InDeviceInfo, m_InAudioFormat, this);
	if(!m_pAudioInput)
	{
		qDebug()<<"Soundcard input error";
		return;
	}
	if(QAudio::NoError == m_pAudioInput->error() )
	{
		//initialize the data queue variables
		Reset();
		m_pAudioInput->setBufferSize(SOUND_READBUFSIZE);
		connect(m_pAudioInput,SIGNAL(notify()), this, SLOT(NewDataReady()));
		m_pIODevice = m_pAudioInput->start(); //start Qt AudioInput
		m_pAudioInput->setNotifyInterval(10);
qDebug()<<"notify Interval "<<m_pAudioInput->notifyInterval();
qDebug()<<"bufferSize "<<m_pAudioInput->bufferSize();
qDebug()<<"rate "<<m_pAudioInput->format().sampleRate();
	}
	else
	{
		qDebug()<<"Soundcard input error";
	}
}

/////////////////////////////////////////////////////////////////////
// Closes down sound card input thread
/////////////////////////////////////////////////////////////////////
void CSoundIn::StopSlot()
{
	if(m_pAudioInput)
	{
		if( ( QAudio::ActiveState==m_pAudioInput->state() )
					|| (QAudio::IdleState==m_pAudioInput->state()) )
			m_pAudioInput->stop();
		delete m_pAudioInput;
		m_pAudioInput = NULL;
	}
}

/////////////////////////////////////////////////////////////////////
// Closes down sound card input thread
/////////////////////////////////////////////////////////////////////
void CSoundIn::Reset()
{
	for(int i=0; i<INQSIZE ;i++)	//zero buffer for data input
		m_InQueue[i] = 0;
	m_InQHead = 0;
	m_InQTail = 0;
	m_InQAvailable = 0;
}

qint16 CSoundIn::GetInputLevel()
{
//	qDebug()<<m_MaxInputLevel;
	m_Mutex.lock();
	//range is -90 to 0 dB
	double x = 200.0 * log10( 3.16e-5 + (double)m_MaxInputLevel/32767.0);
	m_MaxInputLevel = 0;
	m_Mutex.unlock();
	return (qint16)x;
}


/////////////////////////////////////////////////////////////////////
//Slot called by thread every "notify()" interval
//to read any new data from soundcard
/////////////////////////////////////////////////////////////////////
void CSoundIn::NewDataReady()
{
	if(!m_pAudioInput)
		return;
	if( (QAudio::IdleState == m_pAudioInput->state() ) ||
		(QAudio::ActiveState == m_pAudioInput->state() ) )
	{	//Process sound data while soundcard is active and no errors
		int len =  m_pAudioInput->bytesReady();	//in bytes
//qDebug()<<len;
		if( len>0 )
		{
			if(len > SOUND_READBUFSIZE)
				len = SOUND_READBUFSIZE;
			len &= ~(0x01);	//keep on 2 byte chunks
			len = m_pIODevice->read((char*)m_pData, len);
			PutInQueue(len/2, (qint16*)m_pData );
		}
	}
	else
	{	//bail out if error occurs
		emit StopSig();
		qDebug()<<"SoundIn Error";
	}
}

////////////////////////////////////////////////////////////////
//Called by CSoundIn worker thread to put new samples into queue
// This routine is called from a worker thread so must be careful.
////////////////////////////////////////////////////////////////
void CSoundIn::PutInQueue(int numsamples, qint16* pData )
{
int i;
#if TEST_TONE
	for(i=0; i<numsamples; i++)
	{
		pData[i] = (double)TEST_AMP*sin(m_ToneTime);
		m_ToneTime += m_ToneInc;
		if(m_ToneTime > K_2PI)
			m_ToneTime -= K_2PI;
	}
#endif

	int n = DecBy2(numsamples, pData, m_TmpDecBuf);
	n = DecBy3(n, m_TmpDecBuf, pData);

	m_Mutex.lock();
	if( numsamples < (INQSIZE-m_InQAvailable) )
	{	//put in queue if enought room left else ignor
		for( i=0; i<n; i++)
		{
			qint16 val = pData[i];
			if(abs(val) > m_MaxInputLevel)
				m_MaxInputLevel = abs(val);
			m_InQueue[m_InQHead++] = val;
			if(m_InQHead >= INQSIZE)
				m_InQHead = 0;
			m_InQAvailable++;
		}
		//just a safety net in case m_InQAvailable gets out of sync
		if(m_InQAvailable >= INQSIZE)
		{
			m_InQHead = 0;
			m_InQTail = 0;
			m_InQAvailable = 0;
		}
	}
	else
	{
		qDebug()<<"SoundIn overflow";
	}
	emit NewSoundDataRdy();
	m_Mutex.unlock();
}

////////////////////////////////////////////////////////////////
//Called by application to get soundcard input samples
//returns samples still available in queue
////////////////////////////////////////////////////////////////
int CSoundIn::GetInQueue(int numsamples, qint16* pData )
{
int i;
	if( 0==numsamples )
		return m_InQAvailable;
	m_Mutex.lock();

	if(numsamples <= m_InQAvailable)
	{
		for( i=0; i<numsamples; i++)
		{
			pData[i] = m_InQueue[m_InQTail++];
			if(m_InQTail >= INQSIZE)
				m_InQTail = 0;
			m_InQAvailable--;
		}
		//just a safety net in case m_InQAvailable gets out of sync
		if(m_InQAvailable<0)
		{
			m_InQHead = 0;
			m_InQTail = 0;
			m_InQAvailable = 0;
		}
	}
	else
	{	//if not enough in queue just fill with zeros
		for( i=0; i<numsamples; i++)
			pData[i] = 0;
		qDebug()<<"SoundIn underflow";
	}
	m_Mutex.unlock();
	return m_InQAvailable;
}

//////////////////////////////////////////////////////////////////////
// Half band filter and decimate by 2 function.
// Two restrictions on this routine:
// InLength must be larger or equal to the Number of Halfband Taps
// InLength must be an even number
//////////////////////////////////////////////////////////////////////
int CSoundIn::DecBy2(int InLength, qint16* pInData, double* pOutData)
{
int i;
int j;
int numoutsamples = 0;
#if 0
	//safety net to make sure InLength is even and large enough to process
	if( (InLength&1) || (InLength<DEC2TAP_LENGTH) )
	{
		qDebug()<<"Dec2 error";
		return InLength/2;
	}
#endif
	//copy input samples into buffer starting at position m_FirLength-1
	for(i=0,j = DEC2TAP_LENGTH - 1; i<InLength; i++)
		m_Dec2FirBuf[j++] = (double)pInData[i];
	//perform decimation FIR filter on even samples
	for(i=0; i<InLength; i+=2)
	{
		double acc;
		acc = ( m_Dec2FirBuf[i] * DEC2_H[0] );
		for(j=2; j<DEC2TAP_LENGTH; j+=2)	//only use even coefficients since odd are zero(except center point)
			acc += ( m_Dec2FirBuf[i+j] * DEC2_H[j] );
		//now multiply the center coefficient
		acc += ( m_Dec2FirBuf[i+(DEC2TAP_LENGTH-1)/2] * DEC2_H[(DEC2TAP_LENGTH-1)/2] );
		pOutData[numoutsamples++] = acc;	//put in  output buffer

	}
	//need to copy last m_FirLength - 1 input samples in buffer to beginning of buffer
	// for FIR wrap around management
	for(i=0,j = InLength-DEC2TAP_LENGTH+1; i<DEC2TAP_LENGTH - 1; i++)
		m_Dec2FirBuf[i] = (double)pInData[j++];
	return numoutsamples;
}

//////////////////////////////////////////////////////////////////////
// Decimate by 3 function.
//////////////////////////////////////////////////////////////////////
int CSoundIn::DecBy3(int InLength, double* pInData, qint16* pOutData)
{
int i,j,n;
const double* Kptr;
double acc;
double* Firptr;
	n = 0;
	for( i = 0; i<InLength; i++ )	// put new samples into Queue
	{
//decimate by 3 filter
		m_Dec3FirBuf[m_Dec3State] = pInData[i];	//place in circular Queue
		if(--m_SampCnt <=0 )	//calc first decimation filter every 2 samples
		{
			m_SampCnt = 3;
			acc = 0.0;
			Firptr = m_Dec3FirBuf;
			Kptr = DEC3_H + DEC3TAP_LENGTH - m_Dec3State;
			for(j=0; j<	DEC3TAP_LENGTH; j++)	//do the MAC's
				acc += ( (*(Firptr++)) * (*Kptr++) );
			pOutData[n++] = (qint16)acc;
		}
		if( --m_Dec3State < 0)	//deal with FIR pointer wraparound
			m_Dec3State = DEC3TAP_LENGTH-1;
	}
	return n;
}
