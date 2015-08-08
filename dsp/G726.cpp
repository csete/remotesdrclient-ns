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

#include "G726.h"


//SUBTA function from %G726 Section 4.2.1 - Input PCM format conversion and difference signal computation
inline void SUBTA(int SL,int SE,int& D)
{
	D = SL-SE;
}


//LOG function from %G726 Section 4.2.2 - Adaptive quantizer
inline void LOG(int D,unsigned int& DL,int& DS)
{
	DS = D>>15;
	unsigned int DQM = (D<0) ? -D : D;
	DQM &= 0x7fff;

	unsigned int EXP = 0;
	unsigned int x = DQM;
	if(x>=(1<<8))
	{
		EXP |= 8;
		x >>= 8;
	}
	if(x>=(1<<4))
	{
		EXP |= 4;
		x >>= 4;
	}
	if(x>=(1<<2))
	{
		EXP |= 2;
		x >>= 2;
	}
	EXP |= x>>1;
	unsigned int MANT = ((DQM<<7)>>EXP)&0x7f;
	DL = (EXP<<7) + MANT;
}


//QUAN function from %G726 Section 4.2.2 - Adaptive quantizer
inline void QUAN(unsigned int RATE,int DLN,int DS,unsigned int& I)
{
	int x;
	if(RATE==2)
		x = (DLN>=261);
	else
	{
		static const int16_t quan3[4] = {8,218,331,0x7fff};
		static const int16_t quan4[8] = {3972-0x1000,80,178,246,300,349,400,0x7fff};
		static const int16_t quan5[16] = {3974-0x1000,4080-0x1000,68,139,198,250,298,339,378,413,445,475,502,528,553,0x7fff};
		static const int16_t* const quan[3] = {quan3,quan4,quan5};
		const int16_t* levels = quan[RATE-3];
		const int16_t* levels0 = levels;
		while(DLN>=*levels++) {}
		x = levels-levels0-1;
		if(!x)
			x = ~DS;
	}
	int mask = (1<<RATE)-1;
	I = (x^DS)&mask;
}


//SUBTB function from %G726 Section 4.2.2 - Adaptive quantizer
inline void SUBTB(unsigned int DL,unsigned int Y,int& DLN)
{
	DLN = DL-(Y>>2);
}


//ADDA function from %G726 Section 4.2.3 - Inverse adaptive quantizer
inline static void ADDA(int DQLN,unsigned int Y,int& DQL)
{
	DQL = DQLN+(Y>>2);
}


//ANTILOG function from %G726 Section 4.2.3 - Inverse adaptive quantizer
inline void ANTILOG(int DQL,int DQS,unsigned int& DQ)
{
	unsigned int DEX = (DQL >> 7) & 15;
	unsigned int DMN = DQL & 127;
	unsigned int DQT = (1 << 7) + DMN;

	unsigned int DQMAG;
	if(DQL>=0)
		DQMAG = (DQT << 7) >> (14 - DEX);
	else
		DQMAG = 0;
	DQ = DQS ? DQMAG+(1<<15) : DQMAG;
}


//RECONST function from %G726 Section 4.2.3 - Inverse adaptive quantizer
inline void RECONST(unsigned int RATE,unsigned int I,int& DQLN,int& DQS)
{
	// Tables 11-14
static const int16_t reconst2[2] = {116,365};
static const int16_t reconst3[4] = {2048-4096,135,273,373};
static const int16_t reconst4[8] = {2048-4096,4,135,213,273,323,373,425};
static const int16_t reconst5[16] = {2048-4096,4030-4096,28,104,169,224,274,318,358,395,429,459,488,514,539,566};
static const int16_t* const reconst[4] = {reconst2,reconst3,reconst4,reconst5};

	int x = I;
	int m = 1<<(RATE-1);
	if(x&m)
	{
		DQS = -1;
		x = ~x;
	}
	else
		DQS = 0;
	DQLN = reconst[RATE-2][x&(m-1)];

}


//FILTD function from %G726 Section 4.2.4 - Quantizer scale factor adaptation
inline void FILTD(int WI,unsigned int Y,unsigned int& YUT)
{
	int DIF = (WI<<5)-Y;
	int DIFSX = DIF>>5;
	YUT = (Y+DIFSX); // & 8191
}


//FILTE function from %G726 Section 4.2.4 - Quantizer scale factor adaptation
inline void FILTE(unsigned int YUP,unsigned int YL,unsigned int& YLP)
{
	int DIF = (YUP<<6)-YL;
	int DIFSX = DIF>>6;
	YLP = (YL+DIFSX); // & 524287
}


//FUNCTW function from %G726 Section 4.2.4 - Quantizer scale factor adaptation
inline void FUNCTW(unsigned int RATE,unsigned int I,int& WI)
{
	static const int16_t functw2[2] = {4074-4096,439};
	static const int16_t functw3[4] = {4092-4096,30,137,582};
	static const int16_t functw4[8] = {4084-4096,18,41,64,112,198,355,1122};
	static const int16_t functw5[16] = {14,14,24,39,40,41,58,100,141,179,219,280,358,440,529,696};
	static const int16_t* const functw[4] = {functw2,functw3,functw4,functw5};
	unsigned int signMask = 1<<(RATE-1);
	unsigned int n = (I&signMask) ? (2*signMask-1)-I : I;
	WI = functw[RATE-2][n];
}


//LIMB function from %G726 Section 4.2.4 - Quantizer scale factor adaptation
inline void LIMB(unsigned int YUT,unsigned int& YUP)
{
	unsigned int GEUL = (YUT+11264)&(1<<13);
	unsigned int GELL = (YUT+15840)&(1<<13);
	if(GELL)
		YUP = 544;
	else if (!GEUL)
		YUP = 5120;
	else
		YUP = YUT;
}


//MIX function from %G726 Section 4.2.4 - Quantizer scale factor adaptation
inline void MIX(unsigned int AL,unsigned int YU,unsigned int YL,unsigned int& Y)
{
int DIF = YU-(YL>>6);
int PROD = DIF*AL;
	if(DIF<0) PROD += (1<<6)-1; // Force round towards zero for following shift
	PROD >>= 6;
	Y = ((YL>>6)+PROD); // & 8191;
}


//FILTA function from %G726 Section 4.2.5 - Adaptation speed control
inline void FILTA(unsigned int FI,unsigned int DMS,unsigned int& DMSP)
{
int DIF = (FI<<9)-DMS;
int DIFSX = (DIF>>5);
	DMSP = (DIFSX+DMS); // & 4095;
}


//FILTB function from %G726 Section 4.2.5 - Adaptation speed control
inline void FILTB(unsigned int FI,unsigned int DML,unsigned int& DMLP)
{
int DIF = (FI<<11)-DML;
int DIFSX = (DIF>>7);
	DMLP = (DIFSX+DML); // & 16383;
}


//FILTC function from %G726 Section 4.2.5 - Adaptation speed control
inline void FILTC(unsigned int AX,unsigned int AP,unsigned int& APP)
{
int DIF = (AX<<9)-AP;
int DIFSX = (DIF>>4);
	APP = (DIFSX+AP); // & 1023;
}


//FUNCTF function from %G726 Section 4.2.5 - Adaptation speed control
inline void FUNCTF(unsigned int RATE,unsigned int I,unsigned int& FI)
{
static const int16_t functf2[2] = {0,7};
static const int16_t functf3[4] = {0,1,2,7};
static const int16_t functf4[8] = {0,0,0,1,1,1,3,7};
static const int16_t functf5[16] = {0,0,0,0,0,1,1,1,1,1,2,3,4,5,6,6};
static const int16_t* const functf[4] = {functf2,functf3,functf4,functf5};

	unsigned int x = I;
	int mask=(1<<(RATE-1));
	if(x&mask)
		x = ~x;
	x &= mask-1;
	FI = functf[RATE-2][x];
}


//LIMA function from %G726 Section 4.2.5 - Adaptation speed control
inline void LIMA(unsigned int AP,unsigned int& AL)
{
	AL = (AP>256) ? 64 : AP>>2;
}


//SUBTC function from %G726 Section 4.2.5 - Adaptation speed control
inline void SUBTC(unsigned int DMSP,unsigned int DMLP,unsigned int TDP,unsigned int Y,unsigned int& AX)
{
int DIF = (DMSP<<2)-DMLP;
unsigned int DIFM;
	if(DIF<0)
		DIFM = -DIF;
	else
		DIFM = DIF;
	unsigned int DTHR = DMLP >> 3;
	AX = (Y>=1536 && DIFM<DTHR) ? TDP : 1;
}


//TRIGA function from %G726 Section 4.2.5 - Adaptation speed control
inline void TRIGA(unsigned int TR,unsigned int APP,unsigned int& APR)
{
	APR = TR ? 256 : APP;
}

//ACCUM function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void ACCUM(int WAn[2],int WBn[6],int& SE,int& SEZ)
{
int16_t SEZI = (int16_t)(WBn[0]+WBn[1]+WBn[2]+WBn[3]+WBn[4]+WBn[5]);
int16_t SEI = (int16_t)(SEZI+WAn[0]+WAn[1]);
	SEZ = SEZI >> 1;
	SE = SEI >> 1;
}

//ACCUM function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void ADDB(unsigned int DQ,int SE,int& SR)
{
int DQI;
	if(DQ&(1<<15))
		DQI = (1<<15)-DQ;
	else
		DQI = DQ;
	SR = (int16_t)(DQI+SE);
}


//ADDC function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void ADDC(unsigned int DQ,int SEZ,int& PK0,unsigned int& SIGPK)
{
int DQI;
	if(DQ&(1<<15))
		DQI = (1<<15)-DQ;
	else
		DQI = DQ;
	int DQSEZ = (int16_t)(DQI+SEZ);
	PK0 = DQSEZ>>15;
	SIGPK = DQSEZ ? 0 : 1;
}


inline void MagToFloat(unsigned int mag,unsigned int& exp,unsigned int& mant)
{
unsigned int e = 0;
unsigned int m = mag<<1;
	if(m>=(1<<8))
	{
		e |= 8;
		m >>= 8;
	}
	if(m>=(1<<4))
	{
		e |= 4;
		m >>= 4;
	}
	if(m>=(1<<2))
	{
		e |= 2;
		m >>= 2;
	}
	e |= m>>1;
	exp = e;
	mant = mag ? (mag<<6)>>e : 1<<5;
}


//FLOATA function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void FLOATA(unsigned int DQ, unsigned int& DQ0)
{
unsigned int DQS = (DQ>>15);
unsigned int MAG = DQ&32767;
unsigned int EXP;
unsigned int MANT;
	MagToFloat(MAG,EXP,MANT);
	DQ0 = (DQS<<10) + (EXP<<6) + MANT;
}


//FLOATB function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void FLOATB(int SR, unsigned int& SR0)
{
unsigned int SRS = (SR>>15)&1;
unsigned int MAG = SRS ? (-SR)&32767 : SR;
unsigned int EXP;
unsigned int MANT;
	MagToFloat(MAG,EXP,MANT);
	SR0 = (SRS<<10) + (EXP<<6) + MANT;
}


//FMULT function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void FMULT(int An,unsigned int SRn,int& WAn)
{
	unsigned int AnS = (An>>15)&1;
	unsigned int AnMAG = AnS ? (-(An>>2))&8191 : An>>2;
	unsigned int AnEXP;
	unsigned int AnMANT;
	MagToFloat(AnMAG,AnEXP,AnMANT);

	unsigned int SRnS = SRn>>10;
	unsigned int SRnEXP = (SRn>>6) & 15;
	unsigned int SRnMANT = SRn&63;

	unsigned int WAnS = SRnS^AnS;
	unsigned int WAnEXP = SRnEXP+AnEXP;
	unsigned int WAnMANT = ((SRnMANT*AnMANT)+48)>>4;
	unsigned int WAnMAG;
	if(WAnEXP<=26)
		WAnMAG = (WAnMANT<<7) >> (26-WAnEXP);
	else
		WAnMAG = ((WAnMANT<<7) << (WAnEXP-26)) & 32767;
	WAn = WAnS ? -(int)WAnMAG : WAnMAG;
}


//LIMC function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void LIMC(int A2T,int& A2P)
{
	const int A2UL = 12288;
	const int A2LL = 53248-65536;
	if(A2T<=A2LL)
		A2P = A2LL;
	else if(A2T>=A2UL)
		A2P = A2UL;
	else
		A2P = A2T;
}

//LIMD function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void LIMD(int A1T,int A2P,int& A1P)
{
	const int OME = 15360;
	int A1UL = (int16_t)(OME-A2P);
	int A1LL = (int16_t)(A2P-OME);
	if(A1T<=A1LL)
		A1P = A1LL;
	else if(A1T>=A1UL)
		A1P = A1UL;
	else
		A1P = A1T;
}

//TRIGB function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void TRIGB(unsigned int TR,int AnP,int& AnR)
{
	AnR = TR ? 0 : AnP;
}

//UPA1 function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void UPA1(int PK0,int PK1,int A1,unsigned int SIGPK,int& A1T)
{
	int UGA1;
	if(SIGPK==0)
	{
		if(PK0^PK1)
			UGA1 = -192;
		else
			UGA1 = 192;
	}
	else
		UGA1 = 0;
	A1T = (int16_t)(A1+UGA1-(A1>>8));
}


//UPA2 function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void UPA2(int PK0,int PK1,int PK2,int A1,int A2,unsigned int SIGPK,int& A2T)
{
	unsigned int UGA2;
	if(SIGPK==0)
	{
		int UGA2A = (PK0^PK2) ? -16384 : 16384;

		int FA1;
		if(A1<-8191)	 FA1 = -8191<<2;
		else if(A1>8191) FA1 = 8191<<2;
		else			 FA1 = A1<<2;

		int FA = (PK0^PK1) ? FA1 : -FA1;

		UGA2 = (UGA2A+FA) >> 7;
	}
	else
		UGA2 = 0;
	A2T = (int16_t)(A2+UGA2-(A2>>7));
}


//UPB function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void UPB(unsigned int RATE,int Un,int Bn,unsigned int DQ,int& BnP)
{
int UGBn;
	if(DQ&32767)
		UGBn = Un ? -128 : 128;
	else
		UGBn = 0;
	int ULBn = (RATE==5) ? Bn>>9 : Bn>>8;
	BnP = (int16_t)(Bn+UGBn-ULBn);
}


//XOR function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void XOR(unsigned int DQn,unsigned int DQ,int& Un)
{
	Un = -(int)((DQn>>10)^(DQ>>15));
}


//TONE function from %G726 Section 4.2.7 - Tone and transition detector
inline void TONE(int A2P,unsigned int& TDP)
{
	TDP = A2P<(53760-65536);
}


//TRANS function from %G726 Section 4.2.7 - Tone and transition detector
inline void TRANS(unsigned int TD,unsigned int YL,unsigned int DQ,unsigned int& TR)
{
unsigned int DQMAG = DQ&32767;
unsigned int YLINT = YL>>15;
unsigned int YLFRAC = (YL>>10) & 31;
unsigned int THR1 = (32+YLFRAC)<<YLINT;
unsigned int THR2;
	if(YLINT>9)
		THR2 = 31<<10;
	else
		THR2 = THR1;
	unsigned int DQTHR = (THR2+(THR2>>1)) >> 1;
	TR = DQMAG>DQTHR && TD;
}

//LIMO function from %G726 Section A.3.5 - Output limiting (decoder only)
inline void LIMO(int SR,int& SO)
{
	if(SR>=(1<<13))
		SO = (1<<13)-1;
	else if(SR<-(1<<13))
		SO = -(1<<13);
	else
		SO = SR;
}


//g726_section4B Internal - Functional blocks from Section 4 of G726
//Some of these have been broken into two parts, the first of which can generate
//it's outputs using only the saved internal state from the previous itteration
//of the algorithm.
//FIGURE 5/G.726 from Section 4.2.2 - Adaptive quantizer
inline void G726::AdaptiveQuantizer(int D,unsigned int Y,unsigned int& I)
{
unsigned int DL;
int DS;
	LOG(D,DL,DS);
	int DLN;
	SUBTB(DL,Y,DLN);
	QUAN(m_RATE,DLN,DS,I);
}


//FIGURE 6/G.726 from Section 4.2.3 - Inverse adaptive quantizer
inline void G726::InverseAdaptiveQuantizer(unsigned int I,unsigned int Y,unsigned int& DQ)
{
	int DQLN;
	int DQS;
	RECONST(m_RATE,I,DQLN,DQS);
	int DQL;
	ADDA(DQLN,Y,DQL);
	ANTILOG(DQL,DQS,DQ);
}


//FIGURE 7/G.726 (Part 1) from Section 4.2.4 - Quantizer scale factor adaptation
inline void G726::QuantizerScaleFactorAdaptation1(unsigned int AL,unsigned int& Y)
{
	MIX(AL,m_YU,m_YL,Y);
}

//FIGURE 7/G.726 (Part 2) from Section 4.2.4 - Quantizer scale factor adaptation
inline void G726::QuantizerScaleFactorAdaptation2(unsigned int I,unsigned int Y)
{
	int WI;
	FUNCTW(m_RATE,I,WI);
	unsigned int YUT;
	FILTD(WI,Y,YUT);
	unsigned int YUP;
	LIMB(YUT,YUP);
	unsigned int YLP;
	FILTE(YUP,m_YL,YLP);
	m_YU = YUP; // Delay
	m_YL = YLP; // Delay
}


//FIGURE 8/G.726 (Part 1) from Section 4.2.5 - Adaptation speed control
inline void G726::AdaptationSpeedControl1(unsigned int& AL)
{
	LIMA(m_AP,AL);
}

//FIGURE 8/G.726 (Part 2) from Section 4.2.5 - Adaptation speed control
inline void G726::AdaptationSpeedControl2(unsigned int I,unsigned int Y,unsigned int TDP,unsigned int TR)
{
	unsigned int FI;
	FUNCTF(m_RATE,I,FI);

	FILTA(FI,m_DMS,m_DMS); // Result 'DMSP' straight to delay storage 'DMS'
	FILTB(FI,m_DML,m_DML); // Result 'DMSP' straight to delay storage 'DMS'

	unsigned int AX;
	SUBTC(m_DMS,m_DML,TDP,Y,AX); // DMSP and DMLP are read from delay storage 'DMS' and 'DML'

	unsigned int APP;
	FILTC(AX,m_AP,APP);

	TRIGA(TR,APP,m_AP); // Result 'APR' straight to delay storage 'AP'
}


//FIGURE 9/G.726 (Part1) from Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void G726::AdaptativePredictorAndReconstructedSignalCalculator1(int& SE,int& SEZ)
{
	int WBn[6];
	for(int i=0; i<6; i++)
		FMULT(m_Bn[i],m_DQn[i],WBn[i]);
	int WAn[2];
	FMULT(m_A1,m_SR1,WAn[0]);
	FMULT(m_A2,m_SR2,WAn[1]);
	ACCUM(WAn,WBn,SE,SEZ);
}

//FIGURE 9/G.726 (Part2) from Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
inline void G726::AdaptativePredictorAndReconstructedSignalCalculator2(unsigned int DQ,unsigned int TR,int SE,int SEZ,int& SR,int& A2P)
{
	int PK0;
	unsigned int SIGPK;
	ADDC(DQ,SEZ,PK0,SIGPK);

	ADDB(DQ,SE,SR);
	m_SR2 = m_SR1; // Delay
	FLOATB(SR,m_SR1);  // Result 'SR0' straight to delay storage 'SR1'

	unsigned int DQ0;
	FLOATA(DQ,DQ0);

	int i;
	for(i=0; i<6; i++)
	{
		int Un;
		XOR(m_DQn[i],DQ,Un);
		int BnP;
		UPB(m_RATE,Un,m_Bn[i],DQ,BnP);
		TRIGB(TR,BnP,m_Bn[i]); // Result 'BnR' straight to delay storage 'Bn'
	}

	int A2T;
	UPA2(PK0,m_PK1,m_PK2,m_A1,m_A2,SIGPK,A2T);
	LIMC(A2T,A2P);
	TRIGB(TR,A2P,m_A2); // Result 'A2R' straight to delay storage 'A2'

	int A1T;
	UPA1(PK0,m_PK1,m_A1,SIGPK,A1T);
	int A1P;
	LIMD(A1T,A2P,A1P);
	TRIGB(TR,A1P,m_A1); // Result 'A1R' straight to delay storage 'A1'

	m_PK2 = m_PK1; // Delay
	m_PK1 = PK0; // Delay

	for(i=5; i>0; i--)
		m_DQn[i] = m_DQn[i-1]; // Delay
	m_DQn[0] = DQ0; // Delay
}


//FIGURE 10/G.726 (Part 1) from Section 4.2.7 - Tone and transition detector
inline void G726::ToneAndTransitionDetector1(unsigned int DQ,unsigned int& TR)
{
	TRANS(m_TD,m_YL,DQ,TR);
}

//FIGURE 10/G.726 (Part 2) from Section 4.2.7 - Tone and transition detector
inline void G726::ToneAndTransitionDetector2(int A2P,unsigned int TR,unsigned int& TDP)
{
	TONE(A2P,TDP);
	TRIGB(TR,TDP,(int&)m_TD);  // Result 'TDR' straight to delay storage 'TD'
}

//FIGURE A.4/G.726 from Section A.3.3 - Difference signal computation
inline void G726::DifferenceSignalComputation(int SL,int SE,int& D)
{
	SUBTA(SL,SE,D);
}


//FIGURE A.5/G.726 from Section A.3.5 - Output limiting (decoder only)
inline void G726::OutputLimiting(int SR,int& SO)
{
	LIMO(SR,SO);
}


//The top level method which implements the complete algorithm for both
//encoding and decoding.
//param input Either the PCM input to the encoder or the ADPCM input to the decoder.
//param encode A flag which if true makes this method perform the encode function.
//			  If the flag is false then the decode function is performed.
//return Either the ADPCM output to the encoder or the PCM output to the decoder.
inline unsigned int G726::EncodeDecode(unsigned int input,bool encode)
{
	unsigned int AL;
	AdaptationSpeedControl1(AL);
	unsigned int Y;
	QuantizerScaleFactorAdaptation1(AL,Y);
	int SE;
	int SEZ;
	AdaptativePredictorAndReconstructedSignalCalculator1(SE,SEZ);

	unsigned int I;
	if(encode)
	{
		int D;
		int SL = (int16_t)input;
		SL >>= 2; // Convert input from 16bit to 14bit
		DifferenceSignalComputation(SL,SE,D);
		AdaptiveQuantizer(D,Y,I);
	}
	else
		I = input;

	unsigned int DQ;
	InverseAdaptiveQuantizer(I,Y,DQ);
	unsigned int TR;
	ToneAndTransitionDetector1(DQ,TR);
	int SR;
	int A2P;
	AdaptativePredictorAndReconstructedSignalCalculator2(DQ,TR,SE,SEZ,SR,A2P);
	unsigned int TDP;
	ToneAndTransitionDetector2(A2P,TR,TDP);
	AdaptationSpeedControl2(I,Y,TDP,TR);
	QuantizerScaleFactorAdaptation2(I,Y);

	if(encode)
		return I;

	int SO;
	OutputLimiting(SR,SO);
	return SO<<2; // Convert result from 14bit to 16 bit
}


//Public members of class G726
void G726::Reset()
{
	int i;
	for(i=0; i<6; i++)
	{
		m_Bn[i] = 0;
		m_DQn[i] = 32;
	}
	m_A1 = 0;
	m_A2 = 0;
	m_AP = 0;
	m_DML = 0;
	m_DMS = 0;
	m_PK1 = 0;
	m_PK2 = 0;
	m_SR1 = 32;
	m_SR2 = 32;
	m_TD = 0;
	m_YL = 34816;
	m_YU = 544;
}

void G726::SetRate(Rate rate)
{
	m_RATE = rate;
}

unsigned int G726::Encode(void* dst, int dstOffset, const void* src, int srcSize)
{
	// convert pointers into more useful types
	uint8_t* out = (uint8_t*)dst;
	union
	{
		const uint8_t* ptr8;
		const uint16_t* ptr16;
	}
	in;
	in.ptr8 = (const uint8_t*)src;

	// use given bit offset
	out += dstOffset>>3;
	unsigned int bitOffset = dstOffset&7;

	unsigned int bits = m_RATE;			// bits per adpcm sample
	unsigned int mask = (1<<bits)-1;	// bitmask for an adpcm sample

	// calculate number of bits to be written
	unsigned int outBits;
	outBits = bits*(srcSize>>1);
	srcSize &= ~1; // make sure srcSize represents a whole number of samples

	// calculate end of input buffer
	const uint8_t* end = in.ptr8+srcSize;

	while(in.ptr8<end)
	{
		// read a single PCM value from input
		unsigned int pcm;
		pcm = *in.ptr16++;

		// encode the pcm value as an adpcm value
		unsigned int adpcm = EncodeDecode(pcm,true);
		// shift it to the required output position
		adpcm <<= bitOffset;

		// write adpcm value to buffer...
		unsigned int b = *out;			// get byte from ouput
		b &= ~(mask<<bitOffset);	// clear bits which we want to write to
		b |= adpcm;					// or in adpcm value
		*out = (uint8_t)b;			// write value back to output

		// update bitOffset for next adpcm value
		bitOffset += bits;

		// loop if not moved on to next byte
		if(bitOffset<8)
			continue;

		// move pointers on to next byte
		++out;
		bitOffset &= 7;

		// write any left-over bits from the last adpcm value
		if(bitOffset)
			*out = (uint8_t)(adpcm>>8);
	}

	// return number bits written to dst
	return outBits;
}


unsigned int G726::Decode(void* dst, const void* src, int srcOffset, unsigned int srcSize)
{
	// convert pointers into more useful types
	union
	{
		uint8_t* ptr8;
		uint16_t* ptr16;
	}
	out;
	out.ptr8 = (uint8_t*)dst;
	const uint8_t* in = (const uint8_t*)src;

	// use given bit offset
	in += srcOffset>>3;
	unsigned int bitOffset = srcOffset&7;

	unsigned int bits = m_RATE;		// bits per adpcm sample

	while(srcSize>=bits)
	{
		// read adpcm value from input
		unsigned int adpcm = *in;
		if(bitOffset+bits>8)
			adpcm |= in[1]<<8;	// need bits from next byte as well

		// allign adpcm value to bit 0
		adpcm >>= bitOffset;

		// decode the adpcm value into a pcm value
		adpcm &= (1<<m_RATE)-1; // Mask off un-needed bits
		unsigned int pcm = EncodeDecode(adpcm, false);

		// write pcm value to output
		*out.ptr16++ = (int16_t)pcm;

		// update bit values for next adpcm value
		bitOffset += bits;
		srcSize -= bits;

		// move on to next byte of input if required
		if(bitOffset>=8)
		{
			bitOffset &= 7;
			++in;
		}
	}

	// return number of bytes written to dst
	return out.ptr8-(uint8_t*)dst;
}


