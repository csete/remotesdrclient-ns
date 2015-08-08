/////////////////////////////////////////////////////////////////////
// soundout.cpp: implementation of the CSoundOut class.
// Only mono 16 bit mode supported
//
//
// History:
//	2013-10-02  Initial creation MSW
//	2013-10-12  Fixed bug when no soundcard
/////////////////////////////////////////////////////////////////////
//==============================================================================
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

/*---------------------------------------------------------------------------*/
/*------------------------> I N C L U D E S <--------------------------------*/
/*---------------------------------------------------------------------------*/
#include "soundout.h"
#include <math.h>
#include <QDebug>

/*---------------------------------------------------------------------------*/
/*--------------------> L O C A L   D E F I N E S <--------------------------*/
/*---------------------------------------------------------------------------*/
#define SOUNDCARD_OUTRATE 48000	//output soundcard sample rate
//#define SOUNDCARD_OUTRATE 44100
//#define SOUNDCARD_OUTRATE 8000

#define FILTERQLEVEL_ALPHA 0.02
#define P_GAIN 2.0e-6		//Proportional gain

/////////////////////////////////////////////////////////////////////
//   constructor/destructor
/////////////////////////////////////////////////////////////////////
CSoundOut::CSoundOut()
{
	m_pAudioOutput = NULL;
	m_pIODevice = NULL;
	m_OutRatio = 1.0;
	m_OutAudioFormat.setSampleRate(SOUNDCARD_OUTRATE);
	m_Gain = 1.0;
	m_Startup = true;
	m_PpmError = 0;
}

CSoundOut::~CSoundOut()
{
	CleanupThread();	//tell thread to cleanup after itself by calling ThreadExit()
//	qDebug()<<"CSoundOut destructor";
}

//////////////////////////////////////////////////////////////////////////
//Called when worker thread starts to initialize things
//////////////////////////////////////////////////////////////////////////
void CSoundOut::ThreadInit()	//overrided funciton is called by new thread when started
{
	connect(this,SIGNAL( StartSig(int)), this, SLOT(StartSlot(int)) );
	connect(this,SIGNAL( StopSig()), this, SLOT(StopSlot()) );
//qDebug()<<"Soundout Thread Init "<<this->thread()->currentThread();
}

/////////////////////////////////////////////////////////////////////
// Called by this worker thread to cleanup after itself
/////////////////////////////////////////////////////////////////////
void CSoundOut::ThreadExit()
{
	StopSlot();
}

/////////////////////////////////////////////////////////////////////
// Starts up soundcard output thread using soundcard at list OutDevIndx
/////////////////////////////////////////////////////////////////////
void CSoundOut::StartSlot(int OutDevIndx)
{
QAudioDeviceInfo  DeviceInfo;
	m_PpmError = 0;
	m_OutQHead = 0;
	m_OutQTail = 0;
	m_OutQLevel = 0;
	m_AveOutQLevel = 0;
	m_Startup = true;
	//init X6 interpolator
	m_FirState2 = INTP2_QUE_SIZE-1;
	m_FirState3 = INTP3_QUE_SIZE-1;
	m_Indx = 0;
	for(int i=0; i<INTP2_QUE_SIZE; i++)
		m_pQue2[i] = 0.0;
	for( int i=0; i<INTP3_QUE_SIZE; i++)
		m_pQue3[i] = 0.0;

	m_pThread->setPriority(QThread::HighestPriority);
	//Get required soundcard from list
	m_OutDevices = DeviceInfo.availableDevices(QAudio::AudioOutput);
	if(m_OutDevices.isEmpty())
	{
		qDebug()<<"Soundcard not found";
		return;
	}
	if(OutDevIndx >= m_OutDevices.count())
		OutDevIndx = m_OutDevices.count()-1;
	m_OutDeviceInfo = m_OutDevices.at(OutDevIndx);

	//Setup fixed format for sound ouput
	m_OutAudioFormat.setCodec("audio/pcm");
	m_OutAudioFormat.setSampleRate(SOUNDCARD_OUTRATE);
	m_OutAudioFormat.setSampleSize(16);
	m_OutAudioFormat.setSampleType(QAudioFormat::SignedInt);
	m_OutAudioFormat.setByteOrder(QAudioFormat::LittleEndian);
	m_OutAudioFormat.setChannelCount(1);

	m_pAudioOutput = new QAudioOutput(m_OutDeviceInfo, m_OutAudioFormat, this);
	if(!m_pAudioOutput)
	{
		qDebug()<<"Soundcard output error";
		return;
	}
	if(QAudio::NoError == m_pAudioOutput->error() )
	{
		//initialize the data queue variables
		//operate in mode where notify() slot is called periodically to
		// see how much data can be sent to soundcard output
		//connect notify signal to get more soundcard output data
		m_pAudioOutput->setBufferSize(SOUND_WRITEBUFSIZE);
		connect(m_pAudioOutput,SIGNAL(notify()), this, SLOT(GetNewData()));
		m_pIODevice = m_pAudioOutput->start(); //start Qt AudioOutput
		m_pAudioOutput->setNotifyInterval(20);
		GetNewData();	//fill buffer initially
		//qDebug()<<"Soundcard output opened";
	}
	else
	{
		qDebug()<<"Soundcard output error";
	}
}

//////////////////////////////////////////////////////////////////////////
// Called in thread to stop the output sound card
//////////////////////////////////////////////////////////////////////////
void CSoundOut::StopSlot()
{
	if(m_pAudioOutput)
	{
		if( ( QAudio::ActiveState==m_pAudioOutput->state() )
				|| (QAudio::IdleState==m_pAudioOutput->state()) )
			m_pAudioOutput->stop();
		if(m_pAudioOutput)
		{
			delete m_pAudioOutput;
			m_pAudioOutput = NULL;
		}
	}
//qDebug()<<"Soundcard output stopped";
}

/////////////////////////////////////////////////////////////////////
//Called to clear and reset data queue.
/////////////////////////////////////////////////////////////////////
void CSoundOut::Reset()
{
	m_Mutex.lock();
	m_PpmError = 0;
	m_OutQHead = 0;
	m_OutQTail = 0;
	m_OutQLevel = 0;
	m_AveOutQLevel = 0;
	m_Startup = true;
	m_Mutex.unlock();
}

/////////////////////////////////////////////////////////////////////
//Slot called by thread every "notify()" interval
//to write any new data into soundcard
/////////////////////////////////////////////////////////////////////
void CSoundOut::GetNewData()
{
	if(!m_pAudioOutput)
		return;
	if( (QAudio::IdleState == m_pAudioOutput->state() ) ||
		(QAudio::ActiveState == m_pAudioOutput->state() ) )
	{	//Process sound data while soundcard is active and no errors
		int len =  m_pAudioOutput->bytesFree();	//in bytes
//qDebug()<<len;
		if( len>0 )
		{
			//limit size to m_periodSize
			if(len > SOUND_WRITEBUFSIZE)
				len = SOUND_WRITEBUFSIZE;
			len &= ~(0x01);	//keep on 2 byte chunks
			GetOutQueue( len/2, (qint16*)m_pData );
			m_pIODevice->write((char*)m_pData, len);
		}
	}
	else
	{	//bail out if error occurs
		emit StopSig();
		qDebug()<<"SoundOut Error";
	}
}


/////////////////////////////////////////////////////////////////////
// Sets/changes volume control gain  0 <= vol <= 99
//range scales to attenuation(gain) of -50dB to 0dB
/////////////////////////////////////////////////////////////////////
void CSoundOut::SetVolume(qint32 vol)
{
	m_Mutex.lock();
	if(0==vol)	//if zero make infinite attenuation
		m_Gain = 0.0;
	else if(vol<=99)
		m_Gain = pow(10.0, ((double)vol-99.0)/39.2 );
	m_Mutex.unlock();
//qDebug()<<"Volume "<<vol << m_Gain;
}

////////////////////////////////////////////////////////////////
//Called by application to put 16 bit soundcard output samples
//into soundcard queue
// Input is at 8Ksps rate (1/6 the 48Ksps soundcard rate)
////////////////////////////////////////////////////////////////
void CSoundOut::PutOutQueue(int numsamples, qint16* pData )
{
int i;
	if( (0==numsamples) || !pData)
		return;
	//interoplate up to 48Ksps
	int n = InterpolateX6(pData, m_InterpolatedOutput, numsamples);
	m_Mutex.lock();
	for( i=0; i<n; i++)
	{
		m_OutQueueMono[m_OutQHead++] = (qint16)(m_Gain * (double)m_InterpolatedOutput[i]);
		if(m_OutQHead >= OUTQSIZE)
			m_OutQHead = 0;
		m_OutQLevel++;
		if(m_OutQHead==m_OutQTail)	//if full
		{	//remove 1/2 a queue's worth of data
			m_OutQLevel = OUTQSIZE/2;
			m_AveOutQLevel = m_OutQLevel;
			m_OutQTail += (OUTQSIZE/2);
			if(m_OutQTail >= OUTQSIZE)
				m_OutQTail = m_OutQTail - OUTQSIZE;
			i = numsamples;		//force break out of for loop
			qDebug()<<"Snd Out Overflow";
		}
	}
	//calculate average Queue fill level
	m_AveOutQLevel = (1.0-FILTERQLEVEL_ALPHA)*m_AveOutQLevel + FILTERQLEVEL_ALPHA*(double)m_OutQLevel;
	m_Mutex.unlock();
}

////////////////////////////////////////////////////////////////
//Called by CSoundOut worker thread to get new samples from queue
// This routine is called from a worker thread so must be careful.
//   MONO version
////////////////////////////////////////////////////////////////
void CSoundOut::GetOutQueue(int numsamples, qint16* pData )
{
int i;
	m_Mutex.lock();
	if(m_Startup)
	{	//if no data in queue yet just stuff in silence until something is put in queue
		for( i=0; i<numsamples; i++)
			pData[i] = 0;
		if(m_OutQLevel>OUTQSIZE/2)
		{
			m_Startup = false;
			m_RateUpdateCount = -5*SOUNDCARD_OUTRATE;	//delay first error update to let settle
			m_PpmError = 0;
			m_AveOutQLevel = m_OutQLevel;
		}
		m_Mutex.unlock();
		return;
	}

	for( i=0; i<numsamples; i++)
	{
		if(m_OutQHead != m_OutQTail)
		{
			pData[i] = m_OutQueueMono[m_OutQTail++];
			if(m_OutQTail >= OUTQSIZE)
				m_OutQTail = 0;
			if(m_OutQLevel>0)
				m_OutQLevel--;
		}
		else	//queue went empty
		{
#if 1
			for(i=0; i<OUTQSIZE; i++)
				m_OutQueueMono[i] = 0;
#endif
			//backup 1/2 queue ptr and use previous data in queue
			m_OutQTail -= (OUTQSIZE/2);
			if(m_OutQTail < 0)
				m_OutQTail = m_OutQTail + OUTQSIZE;
			pData[i] = m_OutQueueMono[m_OutQTail++];
			if(m_OutQTail >= OUTQSIZE)
				m_OutQTail = 0;
			m_OutQLevel = (OUTQSIZE/2)-1;
			m_AveOutQLevel = m_OutQLevel;
			qDebug()<<"Snd Out Underflow";
		}
	}

	//calculate average Queue fill level
	m_AveOutQLevel = (1.0-FILTERQLEVEL_ALPHA)*m_AveOutQLevel + FILTERQLEVEL_ALPHA*m_OutQLevel;

	// See if time to update rate error calculation routine
	m_RateUpdateCount += numsamples;
	if(m_RateUpdateCount >= SOUNDCARD_OUTRATE)	//every second
	{
		CalcError();
		m_RateUpdateCount = 0;
	}
	m_Mutex.unlock();
}

////////////////////////////////////////////////////////////////
// Called from the Get routine to update the
// error correction process
////////////////////////////////////////////////////////////////
void CSoundOut::CalcError()
{
double error;
	error = (double)(m_AveOutQLevel - OUTQSIZE/2 );	//neg==level is too low  pos == level is to high
	error = error * P_GAIN;
	m_PpmError = (int)( error*1e6 );
//qDebug()<<"SoundOut "<<m_PpmError << m_AveOutQLevel;
}

////////////////////////////////////////////////////////////////
// Called Interpolate x6 from 8Ksps to 48 Ksps sample rate
// 'n' samples in the pIn buffer are output to the pOut buffer.
// Returns the number of output samples.
//  Note: make sure the pOut Buffer is greater than 6 times the input
// buffer size.
////////////////////////////////////////////////////////////////
int CSoundOut::InterpolateX6(qint16* pIn, qint16* pOut, int n)
{
int j;
float acc;
const float* Kptr;
float* Firptr;
int ip = 0;
int numout = 0;
	while(ip < n )
	{
		if( m_Indx%(INTP2_VALUE*INTP3_VALUE) == 0 )	//every 6 outsamples
		{
			m_pQue2[m_FirState2/INTP2_VALUE] = (float)pIn[ip++]; //get new input sample
		}
		if( m_Indx%(INTP3_VALUE) == 0 )
		{
			acc = 0.0;
			Firptr = m_pQue2;
			Kptr = X2IntrpFIRCoef+INTP2_FIR_SIZE-m_FirState2;
			for(j=0; j<INTP2_QUE_SIZE; j++)
			{
				acc += ( (*(Firptr++))*(*Kptr) );
				Kptr += INTP2_VALUE;
			}
			if( --m_FirState2 < 0)
				m_FirState2 = INTP2_FIR_SIZE-1;

			m_pQue3[m_FirState3/INTP3_VALUE] = acc;
		}
		acc = 0.0;
		Firptr = m_pQue3;
		Kptr = X3IntrpFIRCoef+INTP3_FIR_SIZE-m_FirState3;
		for(j=0; j<INTP3_QUE_SIZE; j++)
		{
			acc += ( (*(Firptr++))*(*Kptr) );
			Kptr += INTP3_VALUE;
		}
		if( --m_FirState3 < 0)
			m_FirState3 = INTP3_FIR_SIZE-1;
		pOut[numout++] = (qint16)acc;
		if( ++m_Indx >= (INTP2_VALUE*INTP3_VALUE) )
			m_Indx = 0;
	}
	return numout;
}
