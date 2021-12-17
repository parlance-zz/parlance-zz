#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "vtrace_math.h"

// basic matrix and vector stuff I wrote for reyesir, it's kinda junky but hey, if it works
// all self-explanatory

void vVec3Cross(vVec3 *result, const vVec3 *a, const vVec3 *b)
{
	vVec3 tmp(a->y * b->z - a->z * b->y,
			  a->z * b->x - a->x * b->z,
			  a->x * b->y - a->y * b->x);

	(*result) = tmp;
}

float vVec2Dot(const vVec2 *a, const vVec2 *b)
{
	return a->x * b->x + a->y * b->y;
}

float vVec3Dot(const vVec3 *a, const vVec3 *b)
{
	return a->x * b->x + a->y * b->y + a->z * b->z;
}

float vVec4Dot(const vVec4 *a, const vVec4 *b)
{
	return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

float vVec2Length(const vVec2 *v)
{
	return sqrtf(v->x * v->x + v->y * v->y);
}

float vVec3Length(const vVec3 *v)
{
	return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

float vVec4Length(const vVec4 *v)
{
	return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z + v->w * v->w);
}

float vVec2Normalize(vVec2 *v)
{
	float iLength = 1.0f / vVec2Length(v);
	(*v) = (*v) * iLength;

	return iLength;
}

float vVec3Normalize(vVec3 *v)
{
	float iLength = 1.0f / vVec3Length(v);
	(*v) = (*v) * iLength;

	return iLength;
}

float vVec4Normalize(vVec4 *v)
{
	float iLength = 1.0f / vVec4Length(v);
	(*v) = (*v) * iLength;

	return iLength;
}

void vMatrixIdentity(vMat4x4 *mat)
{
	mat->i00 = 1.0f; mat->i10 = 0.0f; mat->i20 = 0.0f; mat->i30 = 0.0f;
	mat->i01 = 0.0f; mat->i11 = 1.0f; mat->i21 = 0.0f; mat->i31 = 0.0f;
	mat->i02 = 0.0f; mat->i12 = 0.0f; mat->i22 = 1.0f; mat->i32 = 0.0f;
	mat->i03 = 0.0f; mat->i13 = 0.0f; mat->i23 = 0.0f; mat->i33 = 1.0f;
}

void vMatrixZero(vMat4x4 *mat)
{
	memset(&mat->i00, 0, sizeof(vMat4x4));
}

void vMatrixTranslate(vMat4x4 *result, const vVec3 *v)
{
	result->i00 = 1.0f; result->i10 = 0.0f; result->i20 = 0.0f; result->i30 = v->x;
	result->i01 = 0.0f; result->i11 = 1.0f; result->i21 = 0.0f; result->i31 = v->y;
	result->i02 = 0.0f; result->i12 = 0.0f; result->i22 = 1.0f; result->i32 = v->z;
	result->i03 = 0.0f; result->i13 = 0.0f; result->i23 = 0.0f; result->i33 = 1.0f;
}

void vMatrixScale(vMat4x4 *result, const vVec3 *v)
{
	result->i00 = v->x; result->i10 = 0.0f; result->i20 = 0.0f; result->i30 = 0.0f;
	result->i01 = 0.0f; result->i11 = v->y; result->i21 = 0.0f; result->i31 = 0.0f;
	result->i02 = 0.0f; result->i12 = 0.0f; result->i22 = v->z; result->i32 = 0.0f;
	result->i03 = 0.0f; result->i13 = 0.0f; result->i23 = 0.0f; result->i33 = 1.0f;
}

void vMatrixRotateX(vMat4x4 *mat, float rads)
{
	float sinx = sinf(rads), cosx = cosf(rads);

	mat->i00 = 1.0f; mat->i10 = 0.0f; mat->i20 = 0.0f; mat->i30 = 0.0f;
	mat->i01 = 0.0f; mat->i11 = cosx; mat->i21 = sinx; mat->i31 = 0.0f;
	mat->i02 = 0.0f; mat->i12 =-sinx; mat->i22 = cosx; mat->i32 = 0.0f;
	mat->i03 = 0.0f; mat->i13 = 0.0f; mat->i23 = 0.0f; mat->i33 = 1.0f;	
}

void vMatrixRotateY(vMat4x4 *mat, float rads)
{
	float siny = sinf(rads), cosy = cosf(rads);

	mat->i00 = cosy; mat->i10 = 0.0f; mat->i20 =-siny; mat->i30 = 0.0f;
	mat->i01 = 0.0f; mat->i11 = 1.0f; mat->i21 = 0.0f; mat->i31 = 0.0f;
	mat->i02 = siny; mat->i12 = 0.0f; mat->i22 = cosy; mat->i32 = 0.0f;
	mat->i03 = 0.0f; mat->i13 = 0.0f; mat->i23 = 0.0f; mat->i33 = 1.0f;	
}

void vMatrixRotateZ(vMat4x4 *mat, float rads)
{
	float sinz = sinf(rads), cosz = cosf(rads);

	mat->i00 = cosz; mat->i10 =-sinz; mat->i20 = 0.0f; mat->i30 = 0.0f;
	mat->i01 = sinz; mat->i11 = cosz; mat->i21 = 0.0f; mat->i31 = 0.0f;
	mat->i02 = 0.0f; mat->i12 = 0.0f; mat->i22 = 1.0f; mat->i32 = 0.0f;
	mat->i03 = 0.0f; mat->i13 = 0.0f; mat->i23 = 0.0f; mat->i33 = 1.0f;	
}

void vMatrixPerspectiveFovLH(vMat4x4 *mat, float fov, float aspectRatio, float zNear, float zFar)
{
	float h = 1.0f / tanf(fov * 0.5f);
	float w = h / aspectRatio;
	
	float a = zFar / (zFar - zNear);
	float b = -zNear * zFar / (zFar - zNear);

	mat->i00 =    w; mat->i10 = 0.0f; mat->i20 = 0.0f; mat->i30 = 0.0f;
	mat->i01 = 0.0f; mat->i11 =    h; mat->i21 = 0.0f; mat->i31 = 0.0f;
	mat->i02 = 0.0f; mat->i12 = 0.0f; mat->i22 =    a; mat->i32 = b;
	mat->i03 = 0.0f; mat->i13 = 0.0f; mat->i23 = 1.0f; mat->i33 = 0.0f;
}

void vMatrixMultiply(vMat4x4 *result, const vMat4x4 *a, const vMat4x4 *b)
{
	vMat4x4 tmp(a->i00 * b->i00 + a->i10 * b->i01 + a->i20 * b->i02 + a->i30 * b->i03,
		        a->i00 * b->i10 + a->i10 * b->i11 + a->i20 * b->i12 + a->i30 * b->i13,
				a->i00 * b->i20 + a->i10 * b->i21 + a->i20 * b->i22 + a->i30 * b->i23,
				a->i00 * b->i30 + a->i10 * b->i31 + a->i20 * b->i32 + a->i30 * b->i33,

				a->i01 * b->i00 + a->i11 * b->i01 + a->i21 * b->i02 + a->i31 * b->i03,
		        a->i01 * b->i10 + a->i11 * b->i11 + a->i21 * b->i12 + a->i31 * b->i13,
				a->i01 * b->i20 + a->i11 * b->i21 + a->i21 * b->i22 + a->i31 * b->i23,
				a->i01 * b->i30 + a->i11 * b->i31 + a->i21 * b->i32 + a->i31 * b->i33,

				a->i02 * b->i00 + a->i12 * b->i01 + a->i22 * b->i02 + a->i32 * b->i03,
		        a->i02 * b->i10 + a->i12 * b->i11 + a->i22 * b->i12 + a->i32 * b->i13,
				a->i02 * b->i20 + a->i12 * b->i21 + a->i22 * b->i22 + a->i32 * b->i23,
				a->i02 * b->i30 + a->i12 * b->i31 + a->i22 * b->i32 + a->i32 * b->i33,

				a->i03 * b->i00 + a->i13 * b->i01 + a->i23 * b->i02 + a->i33 * b->i03,
		        a->i03 * b->i10 + a->i13 * b->i11 + a->i23 * b->i12 + a->i33 * b->i13,
				a->i03 * b->i20 + a->i13 * b->i21 + a->i23 * b->i22 + a->i33 * b->i23,
				a->i03 * b->i30 + a->i13 * b->i31 + a->i23 * b->i32 + a->i33 * b->i33);


	(*result) = tmp;
}

void vMatrixLookAt(vMat4x4 *mat, const vVec3 *from, const vVec3 *to)
{
	vVec3 z = (*to) - (*from);
	vVec3Normalize(&z);

	vVec3 y(0.0f, 1.0f, 0.0f);

	vVec3 x;
	vVec3Cross(&x, &y, &z);
	vVec3Normalize(&x);

	vVec3Cross(&y, &z, &x);

	mat->i00 =  x.x; mat->i10 =  x.y; mat->i20 =  x.z; mat->i30 = 0.0f;
	mat->i01 =  y.x; mat->i11 =  y.y; mat->i21 =  y.z; mat->i31 = 0.0f;
	mat->i02 =  z.x; mat->i12 =  z.y; mat->i22 =  z.z; mat->i32 = 0.0f;
	mat->i03 =  0.0f;mat->i13 =  0.0f;mat->i23 = 0.0f; mat->i33 = 1.0f;

	vMat4x4 tmp;
	vVec3 trans(-from->x, -from->y, -from->z);

	vMatrixTranslate(&tmp, &trans);
	vMatrixMultiply(mat, mat, &tmp);
}

void vMatrixPitchYawRoll(vMat4x4 *result, const vVec3 *rads)
{
	vMat4x4 x, y, z;

	vMatrixRotateX(&x, rads->x);
	vMatrixRotateY(&y, rads->y);
	vMatrixRotateZ(&z, rads->z);

	vMatrixMultiply(result, &x, &y);
	vMatrixMultiply(result, result, &z);
}	

void vMatrixTranspose(vMat4x4 *result, const vMat4x4 *mat)
{
	(*result) = vMat4x4(mat->i00, mat->i10, mat->i20, mat->i30,
				    	mat->i01, mat->i11, mat->i21, mat->i31,
					    mat->i02, mat->i12, mat->i22, mat->i32,
					    mat->i03, mat->i13, mat->i23, mat->i33);
}

float vMatrixDeterminant(const vMat4x4 *mat)
{
	return  mat->i03 * mat->i12 * mat->i21 * mat->i30-mat->i02 * mat->i13 * mat->i21 * mat->i30-mat->i03 * mat->i11 * mat->i22 * mat->i30+mat->i01 * mat->i13 * mat->i22 * mat->i30 +
			mat->i02 * mat->i11 * mat->i23 * mat->i30-mat->i01 * mat->i12 * mat->i23 * mat->i30-mat->i03 * mat->i12 * mat->i20 * mat->i31+mat->i02 * mat->i13 * mat->i20 * mat->i31 +
			mat->i03 * mat->i10 * mat->i22 * mat->i31-mat->i00 * mat->i13 * mat->i22 * mat->i31-mat->i02 * mat->i10 * mat->i23 * mat->i31+mat->i00 * mat->i12 * mat->i23 * mat->i31 +
			mat->i03 * mat->i11 * mat->i20 * mat->i32-mat->i01 * mat->i13 * mat->i20 * mat->i32-mat->i03 * mat->i10 * mat->i21 * mat->i32+mat->i00 * mat->i13 * mat->i21 * mat->i32 +
			mat->i01 * mat->i10 * mat->i23 * mat->i32-mat->i00 * mat->i11 * mat->i23 * mat->i32-mat->i02 * mat->i11 * mat->i20 * mat->i33+mat->i01 * mat->i12 * mat->i20 * mat->i33 +
			mat->i02 * mat->i10 * mat->i21 * mat->i33-mat->i00 * mat->i12 * mat->i21 * mat->i33-mat->i01 * mat->i10 * mat->i22 * mat->i33+mat->i00 * mat->i11 * mat->i22 * mat->i33;
}

void vMatrixInverse(vMat4x4 *result, const vMat4x4 *mat)
{
	float scale = (1.0f / vMatrixDeterminant(mat));

	(*result) = vMat4x4( (mat->i12 * mat->i23 * mat->i31 -  mat->i13 * mat->i22 * mat->i31 +  mat->i13 * mat->i21 * mat->i32 -  mat->i11 * mat->i23 * mat->i32 -  mat->i12 * mat->i21 * mat->i33 +  mat->i11 * mat->i22 * mat->i33) * scale,
					     (mat->i03 * mat->i22 * mat->i31 -  mat->i02 * mat->i23 * mat->i31 -  mat->i03 * mat->i21 * mat->i32 +  mat->i01 * mat->i23 * mat->i32 +  mat->i02 * mat->i21 * mat->i33 -  mat->i01 * mat->i22 * mat->i33) * scale,
		                 (mat->i02 * mat->i13 * mat->i31 -  mat->i03 * mat->i12 * mat->i31 +  mat->i03 * mat->i11 * mat->i32 -  mat->i01 * mat->i13 * mat->i32 -  mat->i02 * mat->i11 * mat->i33 +  mat->i01 * mat->i12 * mat->i33) * scale,
		                 (mat->i03 * mat->i12 * mat->i21 -  mat->i02 * mat->i13 * mat->i21 -  mat->i03 * mat->i11 * mat->i22 +  mat->i01 * mat->i13 * mat->i22 +  mat->i02 * mat->i11 * mat->i23 -  mat->i01 * mat->i12 * mat->i23) * scale,
		                 (mat->i13 * mat->i22 * mat->i30 -  mat->i12 * mat->i23 * mat->i30 -  mat->i13 * mat->i20 * mat->i32 +  mat->i10 * mat->i23 * mat->i32 +  mat->i12 * mat->i20 * mat->i33 -  mat->i10 * mat->i22 * mat->i33) * scale,
		                 (mat->i02 * mat->i23 * mat->i30 -  mat->i03 * mat->i22 * mat->i30 +  mat->i03 * mat->i20 * mat->i32 -  mat->i00 * mat->i23 * mat->i32 -  mat->i02 * mat->i20 * mat->i33 +  mat->i00 * mat->i22 * mat->i33) * scale,
		                 (mat->i03 * mat->i12 * mat->i30 -  mat->i02 * mat->i13 * mat->i30 -  mat->i03 * mat->i10 * mat->i32 +  mat->i00 * mat->i13 * mat->i32 +  mat->i02 * mat->i10 * mat->i33 -  mat->i00 * mat->i12 * mat->i33) * scale,
		                 (mat->i02 * mat->i13 * mat->i20 -  mat->i03 * mat->i12 * mat->i20 +  mat->i03 * mat->i10 * mat->i22 -  mat->i00 * mat->i13 * mat->i22 -  mat->i02 * mat->i10 * mat->i23 +  mat->i00 * mat->i12 * mat->i23) * scale,
		                 (mat->i11 * mat->i23 * mat->i30 -  mat->i13 * mat->i21 * mat->i30 +  mat->i13 * mat->i20 * mat->i31 -  mat->i10 * mat->i23 * mat->i31 -  mat->i11 * mat->i20 * mat->i33 +  mat->i10 * mat->i21 * mat->i33) * scale,
		                 (mat->i03 * mat->i21 * mat->i30 -  mat->i01 * mat->i23 * mat->i30 -  mat->i03 * mat->i20 * mat->i31 +  mat->i00 * mat->i23 * mat->i31 +  mat->i01 * mat->i20 * mat->i33 -  mat->i00 * mat->i21 * mat->i33) * scale,
		                 (mat->i01 * mat->i13 * mat->i30 -  mat->i03 * mat->i11 * mat->i30 +  mat->i03 * mat->i10 * mat->i31 -  mat->i00 * mat->i13 * mat->i31 -  mat->i01 * mat->i10 * mat->i33 +  mat->i00 * mat->i11 * mat->i33) * scale,
		                 (mat->i03 * mat->i11 * mat->i20 -  mat->i01 * mat->i13 * mat->i20 -  mat->i03 * mat->i10 * mat->i21 +  mat->i00 * mat->i13 * mat->i21 +  mat->i01 * mat->i10 * mat->i23 -  mat->i00 * mat->i11 * mat->i23) * scale,
		                 (mat->i12 * mat->i21 * mat->i30 -  mat->i11 * mat->i22 * mat->i30 -  mat->i12 * mat->i20 * mat->i31 +  mat->i10 * mat->i22 * mat->i31 +  mat->i11 * mat->i20 * mat->i32 -  mat->i10 * mat->i21 * mat->i32) * scale,
		                 (mat->i01 * mat->i22 * mat->i30 -  mat->i02 * mat->i21 * mat->i30 +  mat->i02 * mat->i20 * mat->i31 -  mat->i00 * mat->i22 * mat->i31 -  mat->i01 * mat->i20 * mat->i32 +  mat->i00 * mat->i21 * mat->i32) * scale,
		                 (mat->i02 * mat->i11 * mat->i30 -  mat->i01 * mat->i12 * mat->i30 -  mat->i02 * mat->i10 * mat->i31 +  mat->i00 * mat->i12 * mat->i31 +  mat->i01 * mat->i10 * mat->i32 -  mat->i00 * mat->i11 * mat->i32) * scale,
		                 (mat->i01 * mat->i12 * mat->i20 -  mat->i02 * mat->i11 * mat->i20 +  mat->i02 * mat->i10 * mat->i21 -  mat->i00 * mat->i12 * mat->i21 -  mat->i01 * mat->i10 * mat->i22 +  mat->i00 * mat->i11 * mat->i22) * scale);
}

void vMatrixReflection(vMat4x4 *result, const vVec4 *plane)
{	
	(*result) = vMat4x4(-2.0f *plane->x * plane->x + 1.0f,	-2.0f * plane->y * plane->x,        -2.0f * plane->z * plane->x,        -2.0f * plane->w * plane->x,
				        -2.0f *plane->x * plane->y,		    -2.0f * plane->y * plane->y + 1.0f, -2.0f * plane->z * plane->y,        -2.0f * plane->w * plane->y,
				        -2.0f *plane->x * plane->z,		    -2.0f * plane->y * plane->z,        -2.0f * plane->z * plane->z + 1.0f, -2.0f * plane->w * plane->z,
				         0.0f,				                 0.0f,			                     0.0f,                               1.0f);
}



void vMatrixMultiplyVec3(vVec3 *result, const vMat4x4 *mat, const vVec3 *v)
{
	vVec3 tmp(mat->i00 * v->x + mat->i10 * v->y + mat->i20 * v->z + mat->i30,
			  mat->i01 * v->x + mat->i11 * v->y + mat->i21 * v->z + mat->i31,
			  mat->i02 * v->x + mat->i12 * v->y + mat->i22 * v->z + mat->i32);

	(*result) = tmp;
}

void vMatrixMultiplyVec3NoTranslate(vVec3 *result, const vMat4x4 *mat, const vVec3 *v)
{
	vVec3 tmp(mat->i00 * v->x + mat->i10 * v->y + mat->i20 * v->z,
			  mat->i01 * v->x + mat->i11 * v->y + mat->i21 * v->z,
			  mat->i02 * v->x + mat->i12 * v->y + mat->i22 * v->z);

	(*result) = tmp;
}

void vMatrixMultiplyVec4(vVec4 *result, const vMat4x4 *mat, const vVec4 *v)
{
	vVec4 tmp(mat->i00 * v->x + mat->i10 * v->y + mat->i20 * v->z + mat->i30 * v->w,
			  mat->i01 * v->x + mat->i11 * v->y + mat->i21 * v->z + mat->i31 * v->w,
			  mat->i02 * v->x + mat->i12 * v->y + mat->i22 * v->z + mat->i32 * v->w,
			  mat->i03 * v->x + mat->i13 * v->y + mat->i23 * v->z + mat->i33 * v->w);

	(*result) = tmp;
}

void vMatrixProjectVec3(vVec3 *result, const vMat4x4 *mat, const vVec3 *v)
{
	vVec4 tmp(mat->i00 * v->x + mat->i10 * v->y + mat->i20 * v->z + mat->i30,
			  mat->i01 * v->x + mat->i11 * v->y + mat->i21 * v->z + mat->i31,
			  mat->i02 * v->x + mat->i12 * v->y + mat->i22 * v->z + mat->i32,
			  mat->i03 * v->x + mat->i13 * v->y + mat->i23 * v->z + mat->i33);
	
	float invW = 1.0f / tmp.w;

	result->x = tmp.x * invW;
	result->y = tmp.y * invW;
	result->z = tmp.z * invW;
}

void vMatrixProjectVec4(vVec4 *result, const vMat4x4 *mat, const vVec4 *v)
{
	vVec4 tmp(mat->i00 * v->x + mat->i10 * v->y + mat->i20 * v->z + v->w * mat->i30,
			  mat->i01 * v->x + mat->i11 * v->y + mat->i21 * v->z + v->w * mat->i31,
			  mat->i02 * v->x + mat->i12 * v->y + mat->i22 * v->z + v->w * mat->i32,
			  mat->i03 * v->x + mat->i13 * v->y + mat->i23 * v->z + v->w * mat->i33);
	
	float invW = 1.0f / tmp.w;

	result->x = tmp.x * invW;
	result->y = tmp.y * invW;
	result->z = tmp.z * invW;
	result->w = 1.0f;
}

void vLatticeIdentity(vLattice *result)
{
	result->c0 = vVec4(-1.0f, +1.0f, -1.0f, 1.0f);
	result->c1 = vVec4(+1.0f, +1.0f, -1.0f, 1.0f);
	result->c2 = vVec4(-1.0f, -1.0f, -1.0f, 1.0f);
	result->c3 = vVec4(+1.0f, -1.0f, -1.0f, 1.0f);

	result->c4 = vVec4(-1.0f, +1.0f, +1.0f, 1.0f);
	result->c5 = vVec4(+1.0f, +1.0f, +1.0f, 1.0f);
	result->c6 = vVec4(-1.0f, -1.0f, +1.0f, 1.0f);
	result->c7 = vVec4(+1.0f, -1.0f, +1.0f, 1.0f);
}

void vLatticeZero(vLattice *result)
{
	result->c0 = vVec4(0.0f, 0.0f, 0.0f, 0.0f);
	result->c1 = vVec4(0.0f, 0.0f, 0.0f, 0.0f);
	result->c2 = vVec4(0.0f, 0.0f, 0.0f, 0.0f);
	result->c3 = vVec4(0.0f, 0.0f, 0.0f, 0.0f);

	result->c4 = vVec4(0.0f, 0.0f, 0.0f, 0.0f);
	result->c5 = vVec4(0.0f, 0.0f, 0.0f, 0.0f);
	result->c6 = vVec4(0.0f, 0.0f, 0.0f, 0.0f);
	result->c7 = vVec4(0.0f, 0.0f, 0.0f, 0.0f);
}