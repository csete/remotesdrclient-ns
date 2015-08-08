//////////////////////////////////////////////////////////////////////
// fir.h: interface for the CFir class.
//
//  This class implements a FIR  filter using a double flat coefficient
//array to eliminate testing for buffer wrap around.
//
//Also a decimate by 3 half band filter class CDecimateBy2 is implemented
//
// History:
//	2013-10-02  Initial creation MSW
//////////////////////////////////////////////////////////////////////
#ifndef FIR_H
#define FIR_H

#include "dsp/datatypes.h"

#define MAX_NUMCOEF 125
#include <QMutex>

////////////
//class for FIR Filters
////////////
class CFir
{
public:
    CFir();

	int CreateLPFilter(int NumTaps, TYPEREAL Scale, TYPEREAL Astop,
			   TYPEREAL Fpass, TYPEREAL Fstop, TYPEREAL Foffset, TYPEREAL Fsamprate);
	void ProcessFilter(int InLength, TYPEREAL* InBuf, TYPEREAL* OutBuf);//double version
	void ProcessFilter(int InLength, qint16* InBuf, qint16* OutBuf);	//short integer version

private:
	TYPEREAL Izero(TYPEREAL x);

	int m_NumTaps;
	int m_State;
	TYPEREAL m_SampleRate;
	TYPEREAL m_Coef[MAX_NUMCOEF*2];
	TYPEREAL m_rZBuf[MAX_NUMCOEF];
	QMutex m_Mutex;		//for keeping threads from stomping on each other
};


#endif // FIR_H
