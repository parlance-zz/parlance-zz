#ifndef _H_MATH_HELP
#define _H_MATH_HELP

#include <math.h>
#include <xtl.h>

#define Sin(angle)				(float)sin(D3DXToRadian(angle))
#define Cos(angle)				(float)cos(D3DXToRadian(angle))
#define ATan2(deltaX, deltaY)	D3DXToDegree((float)atan2(deltaX, deltaY))
#define fAbs(num)				(float)fabs(num)
#define Rand()					(float)rand() / 32767.0f
#define Pow(num, degree)		(float)pow(num, degree)
#define Sqrt(num)				(float)sqrt(num);

float fMod(float num, float quotient);
int   intPow(int base, int exp);
void  quaternionFromMatrix3x3(float *matrix, D3DXQUATERNION *out);

#endif