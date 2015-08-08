//////////////////////////////////////////////////////////////////////
// fir.cpp: implementation of the CFir class.
//
//  This class implements a FIR  filter using a double flat coefficient
//array to eliminate testing for buffer wrap around.
//
//Filter coefficients are created by this class for a
// a lowpass filter from frequency and attenuation specifications
// using a Kaiser-Bessel windowed sinc algorithm
//
// History:
//	2013-01-23  Initial creation MSW
//////////////////////////////////////////////////////////////////////

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
#include "fir.h"
#include <QFile>
#include <QDir>
#include <QDebug>


//////////////////////////////////////////////////////////////////////
// Local Defines
//////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
//	Construct CFir object
/////////////////////////////////////////////////////////////////////////////////
CFir::CFir()
{
	m_NumTaps = 1;
	m_State = 0;
}


/////////////////////////////////////////////////////////////////////////////////
//	Process InLength InBuf[] samples and place in OutBuf[]
//  Note the Coefficient array is twice the length and has a duplicated set
// in order to eliminate testing for buffer wrap in the inner loop
//  ex: if 3 tap FIR with coefficients{21,-43,15} is made into a array of 6 entries
//   {21, -43, 15, 21, -43, 15 }
//REAL version
/////////////////////////////////////////////////////////////////////////////////
void CFir::ProcessFilter(int InLength, TYPEREAL* InBuf, TYPEREAL* OutBuf)
{
TYPEREAL acc;
TYPEREAL* Zptr;
const TYPEREAL* Hptr;
	m_Mutex.lock();
	for(int i=0; i<InLength; i++)
	{
		m_rZBuf[m_State] = InBuf[i];
		Hptr = &m_Coef[m_NumTaps - m_State];
		Zptr = m_rZBuf;
		acc = 0.0;
		for(int j=0; j<m_NumTaps; j++)
			acc += (*Hptr++ * *Zptr++);	//do the MACs
		if(--m_State < 0)
			m_State += m_NumTaps;
		OutBuf[i] = acc;
	}
	m_Mutex.unlock();
}

/////////////////////////////////////////////////////////////////////////////////
//	Process InLength InBuf[] samples and place in OutBuf[]
//  Note the Coefficient array is twice the length and has a duplicated set
// in order to eliminate testing for buffer wrap in the inner loop
//  ex: if 3 tap FIR with coefficients{21,-43,15} is made into a array of 6 entries
//   {21, -43, 15, 21, -43, 15 }
//INTEGER version
/////////////////////////////////////////////////////////////////////////////////
void CFir::ProcessFilter(int InLength, qint16* InBuf, qint16* OutBuf)
{
TYPEREAL acc;
TYPEREAL* Zptr;
const TYPEREAL* Hptr;
	m_Mutex.lock();
	for(int i=0; i<InLength; i++)
	{
		m_rZBuf[m_State] = (TYPEREAL)InBuf[i];
		Hptr = &m_Coef[m_NumTaps - m_State];
		Zptr = m_rZBuf;
		acc = 0.0;
		for(int j=0; j<m_NumTaps; j++)
			acc += (*Hptr++ * *Zptr++);	//do the MACs
		if(--m_State < 0)
			m_State += m_NumTaps;
		OutBuf[i] = (qint16)acc;
	}
	m_Mutex.unlock();
}

////////////////////////////////////////////////////////////////////
// Create a FIR Low Pass filter with scaled amplitude 'Scale'
// using a Kaiser-Bessel windowed sinc algorithm.
//
// NumTaps if non-zero, forces filter design to be 'NumTaps' number of taps
//otherwise the algorithm determines the number of taps.
// Scale is linear amplitude scale factor.
// Astop = Stopband Atenuation in dB (ie 40dB is 40dB stopband attenuation)
// Fpass = Lowpass passband frequency in Hz
// Fstop = Lowpass stopband frequency in Hz
// Fsamprate = Sample Rate in Hz
//
//           -------------
//                        |
//                         |
//                          |
//                           |
//    Astop                   ---------------
//                    Fpass   Fstop
// Example call:
//  Let algorithm determine number of taps,
//	Gain scale is 1.0(unity gain)
//	Stopband attenuation is 50dB
//	Lowpass cutoff frequency is 1000Hz
//	Lowpass stopband frequency is 1000Hz
//	Frequency Shift frequency is 0Hz
//	Sample Rate is 8000sps
//	m_Fir.CreateLPFilter(0, 1.0, 50.0, 1000, 1200.0, 0,  8000.0);
//
////////////////////////////////////////////////////////////////////
int CFir::CreateLPFilter(int NumTaps, TYPEREAL Scale, TYPEREAL Astop,
			 TYPEREAL Fpass, TYPEREAL Fstop, TYPEREAL Foffset, TYPEREAL Fsamprate)
{
int n;
TYPEREAL Beta;
	m_Mutex.lock();
	m_SampleRate = Fsamprate;
	//create normalized frequency parameters
	TYPEREAL normFpass = Fpass/Fsamprate;
	TYPEREAL normFstop = Fstop/Fsamprate;
	TYPEREAL normFcut = (normFstop + normFpass)/2.0;	//low pass filter 6dB cutoff
	TYPEREAL normFoffset = K_2PI*(Foffset/Fsamprate);

	//calculate Kaiser-Bessel window shape factor, Beta, from stopband attenuation
	if(Astop < 20.96)
		Beta = 0;
	else if(Astop >= 50.0)
		Beta = .1102 * (Astop - 8.71);
	else
		Beta = .5842 * pow( (Astop-20.96), 0.4) + .07886 * (Astop - 20.96);

	//Now Estimate number of filter taps required based on filter specs
	m_NumTaps = (Astop - 8.0) / (2.285*K_2PI*(normFstop - normFpass) ) + 1;

	//clamp range of filter taps
	if(m_NumTaps > MAX_NUMCOEF )
		m_NumTaps = MAX_NUMCOEF;
	if(m_NumTaps < 3)
		m_NumTaps = 3;

	if(NumTaps)	//if need to force to to a number of taps
		m_NumTaps = NumTaps;

//qDebug()<<"LP taps="<<m_NumTaps << Fpass << Foffset;

	TYPEREAL fCenter = .5*(TYPEREAL)(m_NumTaps-1);
	TYPEREAL izb = Izero(Beta);		//precalculate denominator since is same for all points
	for( n=0; n < m_NumTaps; n++)
	{
		TYPEREAL x = (TYPEREAL)n - fCenter;
		TYPEREAL c;
		// create ideal Sinc() LP filter with normFcut
		if( (TYPEREAL)n == fCenter )	//deal with odd size filter singularity where sin(0)/0==1
			c = 2.0 * normFcut;
		else
			c = (TYPEREAL)sin(K_2PI*x*normFcut)/(K_PI*x);
		//calculate Kaiser window and multiply to get coefficient
		TYPEREAL z = ((TYPEREAL)n - ((TYPEREAL)m_NumTaps-1.0)/2.0 ) / (((TYPEREAL)m_NumTaps-1.0)/2.0);
		m_Coef[n] = Scale * c * Izero( Beta * sqrt(1 - (z*z) ) )  / izb;
		m_Coef[n] = m_Coef[n] * cos(normFoffset*x);
	}

	//make a 2x length array for FIR flat calculation efficiency
	for (n = 0; n < m_NumTaps; n++)
		m_Coef[n+m_NumTaps] = m_Coef[n];

	//Initialize the FIR buffers and state
	for(int i=0; i<m_NumTaps; i++)
	{
		m_rZBuf[i] = 0.0;
	}
	m_State = 0;

	m_Mutex.unlock();

#if 0		//debug hack to write m_Coef to a file for analysis
	QDir::setCurrent("d:/");
	QFile File;
	File.setFileName("LPcoef.txt");
	if(File.open(QIODevice::WriteOnly))
	{
		qDebug()<<"file Opened OK";
		char Buf[256];
		for(n=0; n<m_NumTaps; n++)
		{
			sprintf( Buf, "%g\r\n", m_Coef[n]);
			File.write(Buf);
		}
	}
	else
		qDebug()<<"file Failed to Open";

	qDebug()<<"LP taps="<<m_NumTaps;
#endif

	return m_NumTaps;

}

///////////////////////////////////////////////////////////////////////////
// private helper function to Compute Modified Bessel function I0(x)
//     using a series approximation.
// I0(x) = 1.0 + { sum from k=1 to infinity ---->  [(x/2)^k / k!]^2 }
///////////////////////////////////////////////////////////////////////////
TYPEREAL CFir::Izero(TYPEREAL x)
{
TYPEREAL x2 = x/2.0;
TYPEREAL sum = 1.0;
TYPEREAL ds = 1.0;
TYPEREAL di = 1.0;
TYPEREAL errorlimit = 1e-9;
TYPEREAL tmp;
	do
	{
		tmp = x2/di;
		tmp *= tmp;
		ds *= tmp;
		sum += ds;
		di += 1.0;
	}while(ds >= errorlimit*sum);
//qDebug()<<"x="<<x<<" I0="<<sum;
	return(sum);
}

