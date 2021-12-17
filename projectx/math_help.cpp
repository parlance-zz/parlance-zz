#include "math_help.h"

float fMod(float num, float quotient)
{
	float tmp = num / quotient;
	tmp -= (float)floor(tmp);

	return tmp * quotient;
}

int intPow(int base, int exp)
{
	int result = 1;

	for (int i = 0; i < exp; i++)
	{
		result *= base;
	}

	return result;
}

void  quaternionFromMatrix3x3(float *matrix, D3DXQUATERNION *out)
{
    float m4x4[16] = {0};

    m4x4[0]  = matrix[0];   m4x4[1]  = matrix[1];   m4x4[2]  = matrix[2];
    m4x4[4]  = matrix[3];   m4x4[5]  = matrix[4];   m4x4[6]  = matrix[5];
    m4x4[8]  = matrix[6];   m4x4[9]  = matrix[7];   m4x4[10] = matrix[8];

    m4x4[15] = 1;

    matrix = &m4x4[0];

	D3DXQuaternionRotationMatrix(out, (D3DXMATRIX*)matrix);

	float tmpX = out->x;

	out->x = -out->y;
	out->y = out->z;
	out->z = tmpX;
	out->w = -out->w;
}