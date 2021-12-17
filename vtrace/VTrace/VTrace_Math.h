#ifndef _VTRACE_MATH_H_
#define _VTRACE_MATH_H_

// basic matrix and vector stuff I wrote for reyesir, it's kinda junky but hey, if it works
// all self-explanatory, with the exception being the lattice which represents an 8 point control lattice in 3-space

struct vMat4x4
{
	float i00, i10, i20, i30;
	float i01, i11, i21, i31;
	float i02, i12, i22, i32;
	float i03, i13, i23, i33;

	vMat4x4() {};

	vMat4x4(float v00, float v10, float v20, float v30,
			float v01, float v11, float v21, float v31,
			float v02, float v12, float v22, float v32,
			float v03, float v13, float v23, float v33)
	{
		i00 = v00; i10 = v10; i20 = v20; i30 = v30;
		i01 = v01; i11 = v11; i21 = v21; i31 = v31;
		i02 = v02; i12 = v12; i22 = v22; i32 = v32;
		i03 = v03; i13 = v13; i23 = v23; i33 = v33;
	}

	vMat4x4 operator+(const vMat4x4& rhs) const
	{
		return vMat4x4(i00 + rhs.i00, i10 + rhs.i10, i20 + rhs.i20, i30 + rhs.i30,
					   i01 + rhs.i01, i11 + rhs.i11, i21 + rhs.i21, i31 + rhs.i31,
					   i02 + rhs.i02, i12 + rhs.i12, i22 + rhs.i22, i32 + rhs.i32,
					   i03 + rhs.i03, i13 + rhs.i13, i23 + rhs.i23, i33 + rhs.i33);
	}

	vMat4x4 operator-(const vMat4x4& rhs) const
	{
		return vMat4x4(i00 - rhs.i00, i10 - rhs.i10, i20 - rhs.i20, i30 - rhs.i30,
					   i01 - rhs.i01, i11 - rhs.i11, i21 - rhs.i21, i31 - rhs.i31,
					   i02 - rhs.i02, i12 - rhs.i12, i22 - rhs.i22, i32 - rhs.i32,
					   i03 - rhs.i03, i13 - rhs.i13, i23 - rhs.i23, i33 - rhs.i33);
	}

	vMat4x4 operator*(const vMat4x4& rhs) const
	{
		return vMat4x4(i00 * rhs.i00, i10 * rhs.i10, i20 * rhs.i20, i30 * rhs.i30,
					   i01 * rhs.i01, i11 * rhs.i11, i21 * rhs.i21, i31 * rhs.i31,
					   i02 * rhs.i02, i12 * rhs.i12, i22 * rhs.i22, i32 * rhs.i32,
					   i03 * rhs.i03, i13 * rhs.i13, i23 * rhs.i23, i33 * rhs.i33);
	}

	vMat4x4 operator/(const vMat4x4& rhs) const
	{
		return vMat4x4(i00 / rhs.i00, i10 / rhs.i10, i20 / rhs.i20, i30 / rhs.i30,
					   i01 / rhs.i01, i11 / rhs.i11, i21 / rhs.i21, i31 / rhs.i31,
					   i02 / rhs.i02, i12 / rhs.i12, i22 / rhs.i22, i32 / rhs.i32,
					   i03 / rhs.i03, i13 / rhs.i13, i23 / rhs.i23, i33 / rhs.i33);
	}

	vMat4x4 operator+(const float& rhs) const
	{
		return vMat4x4(i00 + rhs, i10 + rhs, i20 + rhs, i30 + rhs,
					   i01 + rhs, i11 + rhs, i21 + rhs, i31 + rhs,
					   i02 + rhs, i12 + rhs, i22 + rhs, i32 + rhs,
					   i03 + rhs, i13 + rhs, i23 + rhs, i33 + rhs);
	}

	vMat4x4 operator-(const float& rhs) const
	{
		return vMat4x4(i00 - rhs, i10 - rhs, i20 - rhs, i30 - rhs,
					   i01 - rhs, i11 - rhs, i21 - rhs, i31 - rhs,
					   i02 - rhs, i12 - rhs, i22 - rhs, i32 - rhs,
					   i03 - rhs, i13 - rhs, i23 - rhs, i33 - rhs);
	}

	vMat4x4 operator*(const float& rhs) const
	{
		return vMat4x4(i00 * rhs, i10 * rhs, i20 * rhs, i30 * rhs,
					   i01 * rhs, i11 * rhs, i21 * rhs, i31 * rhs,
					   i02 * rhs, i12 * rhs, i22 * rhs, i32 * rhs,
					   i03 * rhs, i13 * rhs, i23 * rhs, i33 * rhs);
	}

	vMat4x4 operator/(const float& rhs) const
	{
		return vMat4x4(i00 / rhs, i10 / rhs, i20 / rhs, i30 / rhs,
					   i01 / rhs, i11 / rhs, i21 / rhs, i31 / rhs,
					   i02 / rhs, i12 / rhs, i22 / rhs, i32 / rhs,
					   i03 / rhs, i13 / rhs, i23 / rhs, i33 / rhs);
	}
};

struct vVec2
{
	float x, y;

	vVec2() {};

	vVec2(float vx, float vy)
	{
		x = vx;
		y = vy;
	}

	vVec2 operator+(const vVec2& rhs) const { return vVec2(x + rhs.x, y + rhs.y); }
	vVec2 operator-(const vVec2& rhs) const { return vVec2(x - rhs.x, y - rhs.y); }
	vVec2 operator*(const vVec2& rhs) const { return vVec2(x * rhs.x, y * rhs.y); }
	vVec2 operator/(const vVec2& rhs) const { return vVec2(x / rhs.x, y / rhs.y); }

	vVec2 operator+(const float& rhs) const { return vVec2(x + rhs, y + rhs); }
	vVec2 operator-(const float& rhs) const { return vVec2(x - rhs, y - rhs); }
	vVec2 operator*(const float& rhs) const { return vVec2(x * rhs, y * rhs); }
	vVec2 operator/(const float& rhs) const { float i = 1.0f / rhs; return vVec2( x * i, y * i); }
};

struct vVec3
{
	float x, y, z;

	vVec3() {};

	vVec3(vVec2 v, float vz)
	{
		x = v.x;
		y = v.y;
		z = vz;
	}

	vVec3(float vx, float vy, float vz)
	{
		x = vx;
		y = vy;
		z = vz;
	}

	vVec3 operator+(const vVec3& rhs) const { return vVec3(x + rhs.x, y + rhs.y, z + rhs.z); }
	vVec3 operator-(const vVec3& rhs) const { return vVec3(x - rhs.x, y - rhs.y, z - rhs.z); }
	vVec3 operator*(const vVec3& rhs) const { return vVec3(x * rhs.x, y * rhs.y, z * rhs.z); }
	vVec3 operator/(const vVec3& rhs) const { return vVec3(x / rhs.x, y / rhs.y, z / rhs.z); }

	vVec3 operator+(const float& rhs) const { return vVec3(x + rhs, y + rhs, z + rhs); }
	vVec3 operator-(const float& rhs) const { return vVec3(x - rhs, y - rhs, z - rhs); }
	vVec3 operator*(const float& rhs) const { return vVec3(x * rhs, y * rhs, z * rhs); }
	vVec3 operator/(const float& rhs) const { float i = 1.0f / rhs; return vVec3( x * i, y * i, z * i); }
};

struct vVec4
{
	float x, y, z, w;

	vVec4() {};

	vVec4(vVec3 v, float vw)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		w = vw;
	}

	vVec4(vVec2 v, float vz, float vw)
	{
		x = v.x;
		y = v.y;
		z = vz;
		w = vw;
	}

	vVec4(float vx, float vy, float vz, float vw)
	{
		x = vx;
		y = vy;
		z = vz;
		w = vw;
	}

	vVec4 operator+(const vVec4& rhs) const { return vVec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
	vVec4 operator-(const vVec4& rhs) const { return vVec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
	vVec4 operator*(const vVec4& rhs) const { return vVec4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }
	vVec4 operator/(const vVec4& rhs) const { return vVec4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w); }

	vVec4 operator+(const float& rhs) const { return vVec4(x + rhs, y + rhs, z + rhs, w + rhs); }
	vVec4 operator-(const float& rhs) const { return vVec4(x - rhs, y - rhs, z - rhs, w - rhs); }
	vVec4 operator*(const float& rhs) const { return vVec4(x * rhs, y * rhs, z * rhs, w * rhs); }
	vVec4 operator/(const float& rhs) const { float i = 1.0f / rhs; return vVec4( x * i, y * i, z * i, w * i); }
};

struct vLattice
{
	vVec4 c0, c1, c2, c3, c4, c5, c6, c7;	// these are in the order of front top left -> front top right -> front bottom left -> front bottom right -> etc

	vLattice() {}

	vLattice(vVec4 c0, vVec4 c1, vVec4 c2, vVec4 c3, vVec4 c4, vVec4 c5, vVec4 c6, vVec4 c7);

	vLattice operator+(const vLattice& rhs) const
	{
		return vLattice(c0 + rhs.c0, c1 + rhs.c1, c2 + rhs.c2, c3 + rhs.c3,
			            c4 + rhs.c4, c5 + rhs.c5, c6 + rhs.c6, c7 + rhs.c7);
	}

	vLattice operator-(const vLattice& rhs) const
	{
		return vLattice(c0 - rhs.c0, c1 - rhs.c1, c2 - rhs.c2, c3 - rhs.c3,
			            c4 - rhs.c4, c5 - rhs.c5, c6 - rhs.c6, c7 - rhs.c7);
	}

};

void  vVec3Cross(vVec3 *result, const vVec3 *a, const vVec3 *b);
float vVec2Dot(const vVec2 *a, const vVec2 *b);
float vVec3Dot(const vVec3 *a, const vVec3 *b);
float vVec4Dot(const vVec4 *a, const vVec4 *b);
float vVec2Length(const vVec2 *v);
float vVec3Length(const vVec3 *v);
float vVec4Length(const vVec4 *v);
float vVec2Normalize(vVec2 *v);
float vVec3Normalize(vVec3 *v);
float vVec4Normalize(vVec4 *v);

void  vMatrixIdentity(vMat4x4 *mat);
void  vMatrixZero(vMat4x4 *mat);
void  vMatrixTranslate(vMat4x4 *result, const vVec3 *v);
void  vMatrixScale(vMat4x4 *result, const vVec3 *v);
void  vMatrixRotateX(vMat4x4 *mat, float rads);
void  vMatrixRotateY(vMat4x4 *mat, float rads);
void  vMatrixRotateZ(vMat4x4 *mat, float rads);
void  vMatrixPerspectiveFovLH(vMat4x4 *mat, float fov, float aspectRatio, float zNear, float zFar);
void  vMatrixMultiply(vMat4x4 *result, const vMat4x4 *a, const vMat4x4 *b);
void  vMatrixLookAt(vMat4x4 *mat, const vVec3 *from, const vVec3 *to);
void  vMatrixPitchYawRoll(vMat4x4 *result, const vVec3 *rads);
void  vMatrixTranspose(vMat4x4 *result, const vMat4x4 *mat);
float vMatrixDeterminant(const vMat4x4 *mat);
void  vMatrixInverse( vMat4x4 *result, const vMat4x4 *mat);
void  vMatrixReflection(vMat4x4 *result, const vVec4 *plane);
void  vMatrixMultiplyVec3(vVec3 *result, const vMat4x4 *mat, const vVec3 *v);
void  vMatrixMultiplyVec3NoTranslate(vVec3 *result, const vMat4x4 *mat, const vVec3 *v);
void  vMatrixMultiplyVec4(vVec4 *result, const vMat4x4 *mat, const vVec4 *v);
void  vMatrixProjectVec3(vVec3 *result, const vMat4x4 *mat, const vVec3 *v);
void  vMatrixProjectVec4(vVec4 *result, const vMat4x4 *mat, const vVec4 *v);

void vLatticeIdentity(vLattice *result);
void vLatticeZero(vLattice *result);

#endif // #ifndef _VTRACE_MATH_H_