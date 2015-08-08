/*
This program is distributed under the terms of the 'MIT license'. The text
of this licence follows...

Copyright (c) 2004 J.D.Medhurst (a.k.a. Tixy)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef G726_H
#define G726_H

#include "stdint.h"


//A class which implements ITU-T (formerly CCITT) Recomendation G726
//	   "40, 32, 24, 16 kbit/s Adaptive Differential Pulse Code Modulation (ADPCM)"
//G726 replaces recomendations G721 and G723.
//
//Note, this implemetation reproduces bugs found in the G191 reference implementation
//of %G726. These bugs can be controlled by the #IMPLEMENT_G191_BUGS macro.

class G726
{
public:
	//	Enumeration used to specify the ADPCM bit-rate.
	enum Rate{
		Rate16kBits=2,	/* 16k bits per second (2 bits per ADPCM sample) */
		Rate24kBits=3,	/* 24k bits per second (3 bits per ADPCM sample) */
		Rate32kBits=4,	/* 32k bits per second (4 bits per ADPCM sample) */
		Rate40kBits=5	/* 40k bits per second (5 bits per ADPCM sample) */
		};

	G726(){	Reset(); }

	//Clears the internal state variables to their 'reset' values.
	//Call this function before starting to decode/encode a new audio stream.
	void Reset();

	//Set the ADPCM bit-rate used for encoder output and decoder input.
	//param rate The ADPCM bit-rate.
	void SetRate(Rate rate);

	//Encode a buffer of uniform PCM values into ADPCM values.
	//Each ADPCM value only occupies the minimum number of bits required and successive
	//values occupy adjacent bit positions. E.g. Four 3 bit ADPCM values (A,B,C,D) are
	//stored in two successive bytes like this: 1st byte: ccbbbaaa 2nd byte: ----dddc.
	//Note that any unused bits in the last byte are set to zero.
	//param dst		 Pointer to location to store ADPCM values.
	//param dstOffset Offset from dst, in number-of-bits, at which the decoded values
	//				 will be stored. I.e. the least significant bit of the first ADPCM
	//				 value will be stored in byte
	//				 dst[dstOffset>>3]
	//				 at bit position
	//				 dstOffset&7
	//				 Where the bit 0 is the least significant bit in a byte
	//				 and bit 7 is the most significant bit.
	//param src		 Pointer to the buffer of PCM values to be converted.
	//param srcSize	 The size in bytes of the buffer at src.
	//				 Must be a multiple of the size of a single PCM sample.
	//return 		 The number of bits were stored in the dst buffer.
	unsigned int Encode(void* dst, int dstOffset, const void* src, int srcSize);

	//Decode a buffer of ADPCM values into uniform PCM values.
	//
	//Each ADPCM value only occupies the minimum number of bits required and successive
	//values occupy adjacent bit positions. E.g. Four 3 bit ADPCM values (A,B,C,D) are
	//stored in two successive bytes like this: 1st byte: ccbbbaaa 2nd byte: ----dddc.
	//
	//param dst		 Pointer to location to store PCM values.
	//param src		 Pointer to the buffer of ADPCM values to be converted.
	//param srcOffset Offset from src, in number-of-bits, from which the ADPCM values
	//				 will be read. I.e. the least significant bit of the first ADPCM
	//				 value will be read from byte
	//				 src[srcOffset>>3]
	//				 at bit position
	//				 srcOffset&7
	//				 Where the bit 0 is the least significant bit in a byte
	//				 and bit 7 is the most significant bit.
	//param srcSize	 The number of bits to be read from the buffer at src.
	//				 Must be a multiple of the size of a single ADPCM value.
	//return 		 The number of bytes which were stored in the dst buffer.
	unsigned int Decode(void* dst, const void* src, int srcOffset, unsigned int srcSize);

private:
	void AdaptiveQuantizer(int D,unsigned int Y,unsigned int& I);
	void InverseAdaptiveQuantizer(unsigned int I,unsigned int Y,unsigned int& DQ);
	void QuantizerScaleFactorAdaptation1(unsigned int AL,unsigned int& Y);
	void QuantizerScaleFactorAdaptation2(unsigned int I,unsigned int Y);
	void AdaptationSpeedControl1(unsigned int& AL);
	void AdaptationSpeedControl2(unsigned int I,unsigned int y,unsigned int TDP,unsigned int TR);
	void AdaptativePredictorAndReconstructedSignalCalculator1(int& SE,int& SEZ);
	void AdaptativePredictorAndReconstructedSignalCalculator2(unsigned int DQ,unsigned int TR,int SE,int SEZ,int& SR,int& A2P);
	void ToneAndTransitionDetector1(unsigned int DQ,unsigned int& TR);
	void ToneAndTransitionDetector2(int A2P,unsigned int TR,unsigned int& TDP);
	void DifferenceSignalComputation(int SL,int SE,int& D);
	void OutputLimiting(int SR,int& S0);
	unsigned int EncodeDecode(unsigned int input,bool encode);

private:
	Rate m_RATE;
	// Persistant states for DELAY elements...
	int m_A1;
	int m_A2;
	unsigned int m_AP;
	int m_Bn[6];
	unsigned int m_DML;
	unsigned int m_DMS;
	unsigned int m_DQn[6];
	int m_PK1;
	int m_PK2;
	unsigned int m_SR1;
	unsigned int m_SR2;
	unsigned int m_TD;
	unsigned int m_YL;
	unsigned int m_YU;
};

#endif

