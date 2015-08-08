//////////////////////////////////////////////////////////////////////
// datatypes.h: Common data type declarations
//
// History:
//	2013-10-02  Initial creation MSW
//////////////////////////////////////////////////////////////////////
#ifndef DATATYPES_H
#define DATATYPES_H

#include <QApplication>
#include <math.h>


//define single or double precision reals and complex types
typedef float tSReal;
typedef double tDReal;

typedef struct _sCplx
{
	tSReal re;
	tSReal im;
}tSComplex;

typedef struct _dCplx
{
	tDReal re;
	tDReal im;
}tDComplex;

#define TYPEREAL tSReal
#define TYPECPX	tSComplex

#define K_2PI (2.0 * 3.14159265358979323846)
#define K_PI (3.14159265358979323846)
#define K_PI4 (K_PI/4.0)
#define K_PI2 (K_PI/2.0)
#define K_3PI4 (3.0*K_PI4)


#endif // DATATYPES_H
