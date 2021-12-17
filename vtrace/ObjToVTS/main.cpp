#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <algorithm>
#include <vector>
#include <fstream>
#include <string>

#include <sdl.h>

using namespace std;

#define DEFAULT_AXEL_DETAIL_LEVEL	8			// default detail level is 8 (4 * (4^(8)) axels), min is 1, max is 10

#define OBJ_LOAD_MAX_LINE_LENGTH	0x1000		// max number of chars per line in obj file
#define OBJ_LOAD_MAX_TOKENS			0x100		// max number of tokens per line in obj file

#define OBJ_LOAD_VECTOR_INITIAL_CAPACITY 0x100000	// initial reservation size for obj element vectors (for performance)

// surface file header

struct VTSHeader
{
	unsigned int magic;			// always 0x03460566
	unsigned char formatVersion;// high nibble = major version, low nibble = minor version
	unsigned char surfaceFlags; // specifies the surface type and a few other flags
	unsigned char numLayers;	// number of layers in the data
};

// intermediate data structures used in processing

struct ObjVertex
{
	float x,y,z;		// position
	float nx, ny, nz;	// normal
};

struct ObjTVertex
{
	float u, v;		// texture coordinates
};

struct ObjTriangle
{
	int v0,v1,v2;		// physical vertex references
	int vt0,vt1,vt2;	// uv vertex references
};

struct Vertex				// used in the recursive splice stage only
{
	float pos[3];
	float norm[3];
	float uv[2];
};

struct Polygon				// likewise
{
	Vertex verts[16];
	int numVerts;
};

struct Axel
{
	unsigned short x,y,z;	// 16 bit position
	unsigned int color;     // 8 bits each, high 8 bits unused
	short    nx,ny,nz;		// 16 bit signed normal
	unsigned long long hval;// hilbert value
};

bool operator < (const Axel& left, const Axel& right) // for sorting, compares hilbert values
{
	if  (left.hval < right.hval) return true;
	return false;
}

// hilbert magic functions, I didn't write these, but I did clean them up and modify them to special case them for 3 dimensions 16 bits each

typedef unsigned long long bitmask_t;
typedef unsigned long halfmask_t;

#define hilbert_ones(T, k) ((((T)2) << (k - 1)) - 1)
#define hilbert_rdbit(w, k) (((w) >> (k)) & 1)
#define hilbert_rotateRight(arg, nRots, nDims) ((((arg) >> (nRots)) | ((arg) << ((nDims) - (nRots)))) & hilbert_ones(bitmask_t, nDims))

#define hilbert_adjustRotation(rotation, nDims, bits)                 \
do {                                                                  \
	bits &= -bits & nd1Ones;                                          \
	while (bits)                                                      \
		bits >>= 1, ++rotation;                                       \
	if (++rotation >= nDims)                                          \
		rotation -= nDims;                                            \
} while (0)

bitmask_t hilbert_bitTranspose(bitmask_t inCoords)
{
	unsigned const nDims = 3;
	unsigned const nBits = 16;

	bitmask_t coords = 0;
	unsigned d;

	for (d = 0; d < nDims; ++d)
    {
		unsigned b;

		bitmask_t in = inCoords & hilbert_ones(bitmask_t, nBits);
		bitmask_t out = 0;

		inCoords >>= nBits;

		for (b = nBits; b--;)
		{
			out <<= nDims;
			out |= hilbert_rdbit(in, b);
		}

		coords |= out << d;
	}

	return coords;
}

bitmask_t hilbert_c2i(unsigned short *coord)	// coord should be unsigned short[3] such that minx = 0, maxx = 65535 for each dimension
{
	unsigned const nDims = 3;
	unsigned const nBits = 16;
	unsigned const nDimsBits = 3 * 16;

	bitmask_t index;
	unsigned d;

	bitmask_t coords = 0;

	for (d = nDims; d--;)
	{
		coords <<= nBits;
		coords |= coord[d];
	}

	halfmask_t const ndOnes = hilbert_ones(halfmask_t,nDims);
	halfmask_t const nd1Ones = ndOnes >> 1;

	unsigned b = nDimsBits;
	unsigned rotation = 0;

	halfmask_t flipBit = 0;
	bitmask_t const nthbits = hilbert_ones(bitmask_t, nDimsBits) / ndOnes;

	coords = hilbert_bitTranspose(coords);
	coords ^= coords >> nDims;

	index = 0;

	do
	{
		halfmask_t bits = (coords >> (b -= nDims)) & ndOnes;
		bits = hilbert_rotateRight(flipBit ^ bits, rotation, nDims);

		index <<= nDims;
		index |= bits;

		flipBit = (halfmask_t)1 << rotation;
		hilbert_adjustRotation(rotation, nDims, bits);

	} while (b);

	index ^= nthbits >> 1;

	for (d = 1; d < nDimsBits; d *= 2)
	{
		index ^= index >> d;
	}

	return index;
}

// some useful functions to keep the code a little cleaner

void vec3Add(float *result, float *a, float *b)
{
	result[0] = a[0] + b[0];
    result[1] = a[1] + b[1];
	result[2] = a[2] + b[2];
}

void vec2Add(float *result, float *a, float *b)
{
	result[0] = a[0] + b[0];
    result[1] = a[1] + b[1];
}

void vec3Sub(float *result, float *a, float *b) // a - b
{
	result[0] = a[0] - b[0];
    result[1] = a[1] - b[1];
	result[2] = a[2] - b[2];
}

void double3Sub(double *result, double *a, double *b) // a - b
{
	result[0] = a[0] - b[0];
    result[1] = a[1] - b[1];
	result[2] = a[2] - b[2];
}

void vec3Lerp(float *result, float *a, float *b, float t)	// such that result = a when t = 0
{
	float it = 1.0f - t;

	result[0] = a[0] * it + b[0] * t;
    result[1] = a[1] * it + b[1] * t;
	result[2] = a[2] * it + b[2] * t;
}

void vec2Lerp(float *result, float *a, float *b, float t)	// such that result = a when t = 0
{
	float it = 1.0f - t;

	result[0] = a[0] * it + b[0] * t;
    result[1] = a[1] * it + b[1] * t;
}

void vertexLerp(Vertex *result, Vertex *a, Vertex *b, float t)	// such that result = a when t = 0
{
	float it = 1.0f - t;

	result->pos[0] = a->pos[0] * it + b->pos[0] * t;
	result->pos[1] = a->pos[1] * it + b->pos[1] * t;
	result->pos[2] = a->pos[2] * it + b->pos[2] * t;

	result->norm[0] = a->norm[0] * it + b->norm[0] * t;
	result->norm[1] = a->norm[1] * it + b->norm[1] * t;
	result->norm[2] = a->norm[2] * it + b->norm[2] * t;

	result->uv[0] = a->uv[0] * it + b->uv[0] * t;
	result->uv[1] = a->uv[1] * it + b->uv[1] * t;
}

void vec3Cross(float *result, float *a, float *b)
{
	float tmp[3];

	tmp[0] = (a[1] * b[2]) - (a[2] * b[1]);
    tmp[1] = (a[2] * b[0]) - (a[0] * b[2]);
	tmp[2] = (a[0] * b[1]) - (a[1] * b[0]);

	result[0] = tmp[0];
	result[1] = tmp[1];
	result[2] = tmp[2];
}

void double3Cross(double *result, double *a, double *b)
{
	double tmp[3];

	tmp[0] = (a[1] * b[2]) - (a[2] * b[1]);
    tmp[1] = (a[2] * b[0]) - (a[0] * b[2]);
	tmp[2] = (a[0] * b[1]) - (a[1] * b[0]);

	result[0] = tmp[0];
	result[1] = tmp[1];
	result[2] = tmp[2];
}

float vec3Dot(float *a, float *b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float vec3Length(float *v)
{
	return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

float double3Length(double *v)
{
	return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

float vec3Normalize(float *v)
{
	float length = vec3Length(v);
	float iLength = 1.0f / length;

	v[0] *= iLength;
	v[1] *= iLength;
	v[2] *= iLength;

	return length;
}

float double3Normalize(double *v)
{
	double length = double3Length(v);

	v[0] /= length;
	v[1] /= length;
	v[2] /= length;

	return length;
}

int isPowerOf2(int x) // if x is a power of 2 returns log base 2 of x, else returns -1
{
	int p = -1;

	for (int b = 0; b < 31; b++)
	{
		if ((x & (1 << b)))
		{
			if (p < 0) p = b;
			else return -1;
		}
	}

	return p;
}

// quick and dirty file size routine

unsigned int fileSize(const char *fileName)
{
	ifstream fileStream(fileName);

	if (fileStream.fail())
	{
		return 0;
	}

	unsigned int begin_pos = fileStream.tellg();
	fileStream.seekg(0, std::ios_base::end);

	unsigned int size = unsigned int(fileStream.tellg())- begin_pos;

	fileStream.close();

	return size;
}

// used in the geometry refinement stage, splits a convex polygon about an axis aligned splitting plane

void splicePolygon(int axis, float *origin, Polygon *in, Polygon *outb, Polygon *outf)
{
	outf->numVerts = 0;
	outb->numVerts = 0;

	if (in->numVerts < 3) return;

	for (int e = 0; e < in->numVerts; e++)
	{
		int v0i = e;
		Vertex &v0 = in->verts[v0i];
		
		int v1i = e + 1;
		if ((v1i) == in->numVerts) v1i = 0;
		Vertex &v1 = in->verts[v1i];

		float v0d = v0.pos[axis] - origin[axis];
		float v1d = v1.pos[axis] - origin[axis];

		if (v0d * v1d >= 0.0f)
		{
			if (v0d + v1d < 0.0f)
			{
				outb->verts[outb->numVerts++] = v0;
			}
			else
			{
				outf->verts[outf->numVerts++] = v0;
			}
		}
		else
		{
			float t = fabs(v0d / (v1.pos[axis] - v0.pos[axis]));

			Vertex split;
			vertexLerp(&split, &v0, &v1, t);

			if (v0d < 0.0f)
			{
				outb->verts[outb->numVerts++] = v0;
				outb->verts[outb->numVerts++] = split;

				outf->verts[outf->numVerts++] = split;
			}
			else
			{
				outf->verts[outf->numVerts++] = v0;
				outf->verts[outf->numVerts++] = split;

				outb->verts[outb->numVerts++] = split;
			}
		}
	}
}

// used in the geometry refinement stage, turns an input triangle into axels with 1 cubic unit of volume in a theoretical 1024x1024x1024 grid

void axelizePolygon(int l, float *origin, Polygon *poly, vector<Axel> *axels, unsigned *diffuseMap, int diffuseMapWidth, int diffuseMapHeight)
{
	if (l == 10)
	{
		Vertex vert;
		memset(&vert, 0, sizeof(Vertex));

		for (int v = 0; v < poly->numVerts; v++)
		{
			vec3Add(vert.pos,  vert.pos,  poly->verts[v].pos);
			vec3Add(vert.norm, vert.norm, poly->verts[v].norm);
			vec2Add(vert.uv,   vert.uv,   poly->verts[v].uv);
		}

		double invNumVerts = 1.0 / poly->numVerts;

		vert.pos[0]  *= invNumVerts; vert.pos[1]  *= invNumVerts;  vert.pos[2] *= invNumVerts;
		vert.norm[0] *= invNumVerts; vert.norm[1] *= invNumVerts; vert.norm[2] *= invNumVerts;
		vert.uv[0]   *= invNumVerts; vert.uv[1]   *= invNumVerts;

		Axel axel;

		axel.x = int(double(origin[0] + 1.0) * 511.5);
		axel.y = int(double(origin[1] + 1.0) * 511.5);
		axel.z = int(double(origin[2] + 1.0) * 511.5);

		axel.hval = hilbert_c2i(&axel.x);

		float nLength = fabsf(vert.norm[0]);

		if (fabsf(vert.norm[1]) > nLength) nLength = fabsf(vert.norm[1]);
		if (fabsf(vert.norm[2]) > nLength) nLength = fabsf(vert.norm[2]);

		axel.nx = short(vert.norm[0] / double(nLength) * 32767.0);
		axel.ny = short(vert.norm[1] / double(nLength) * 32767.0);
		axel.nz = short(vert.norm[2] / double(nLength) * 32767.0);

		if (diffuseMap)
		{
			float pu = (vert.uv[0] - floorf(vert.uv[0])) * diffuseMapWidth;
			float pv = (vert.uv[1] - floorf(vert.uv[1])) * diffuseMapHeight;

			int lx = int(floorf(pu));
			int ty = int(floorf(pv));
			
			float spu1 = pu - floorf(pu);
			float spv1 = pv - floorf(pv);
			float spu0 = (1.0f - spu1);
			float spv0 = (1.0f - spv1);
			
			int rx = (lx + 1) % diffuseMapWidth;
			int by = (ty + 1) % diffuseMapHeight;
			
			unsigned char tlr = (diffuseMap[ty * diffuseMapHeight + lx] & 0x000000FF) >> 0;
			unsigned char tlg = (diffuseMap[ty * diffuseMapHeight + lx] & 0x0000FF00) >> 8;
			unsigned char tlb = (diffuseMap[ty * diffuseMapHeight + lx] & 0x00FF0000) >> 16;

			unsigned char trr = (diffuseMap[ty * diffuseMapHeight + rx] & 0x000000FF) >> 0;
			unsigned char trg = (diffuseMap[ty * diffuseMapHeight + rx] & 0x0000FF00) >> 8;
			unsigned char trb = (diffuseMap[ty * diffuseMapHeight + rx] & 0x00FF0000) >> 16;

			unsigned char blr = (diffuseMap[by * diffuseMapHeight + lx] & 0x000000FF) >> 0;
			unsigned char blg = (diffuseMap[by * diffuseMapHeight + lx] & 0x0000FF00) >> 8;
			unsigned char blb = (diffuseMap[by * diffuseMapHeight + lx] & 0x00FF0000) >> 16;

			unsigned char brr = (diffuseMap[by * diffuseMapHeight + rx] & 0x000000FF) >> 0;
			unsigned char brg = (diffuseMap[by * diffuseMapHeight + rx] & 0x0000FF00) >> 8;
			unsigned char brb = (diffuseMap[by * diffuseMapHeight + rx] & 0x00FF0000) >> 16;

			float tr = tlr * spu0 + trr * spu1;
			float tg = tlg * spu0 + trg * spu1;
			float tb = tlb * spu0 + trb * spu1;

			float br = blr * spu0 + brr * spu1;
			float bg = blg * spu0 + brg * spu1;
			float bb = blb * spu0 + brb * spu1;

			float r = tr * spv0 + br * spv1;
			float g = tg * spv0 + bg * spv1;
			float b = tb * spv0 + bb * spv1;

			axel.color = (int(r) | (int(g) << 8) | (int(b) << 16));
		}

#pragma omp critical
		{
			axels->push_back(axel);
		}

		return;
	}

	float offset = 1.0f / float(1 << (l + 1));

	Polygon t, b;
	Polygon tl, tr, bl, br;
	Polygon tlf, tlb, trf, trb, blf, blb, brf, brb;

	splicePolygon(1, origin, poly, &b, &t);

	splicePolygon(0, origin, &t, &tl, &tr);
	splicePolygon(0, origin, &b, &bl, &br);

	splicePolygon(2, origin, &tl, &tlb, &tlf);
	splicePolygon(2, origin, &tr, &trb, &trf);
	splicePolygon(2, origin, &bl, &blb, &blf);
	splicePolygon(2, origin, &br, &brb, &brf);

	if (tlb.numVerts > 0)
	{
		float norigin[3] = {origin[0] - offset, origin[1] + offset, origin[2] - offset};
		axelizePolygon(l + 1, norigin, &tlb, axels, diffuseMap, diffuseMapWidth, diffuseMapHeight);
	}

	if (tlf.numVerts > 0)
	{
		float norigin[3] = {origin[0] - offset, origin[1] + offset, origin[2] + offset};
		axelizePolygon(l + 1, norigin, &tlf, axels, diffuseMap, diffuseMapWidth, diffuseMapHeight);
	}

	if (trb.numVerts > 0)
	{
		float norigin[3] = {origin[0] + offset, origin[1] + offset, origin[2] - offset};
		axelizePolygon(l + 1, norigin, &trb, axels, diffuseMap, diffuseMapWidth, diffuseMapHeight);
	}

	if (trf.numVerts > 0)
	{
		float norigin[3] = {origin[0] + offset, origin[1] + offset, origin[2] + offset};
		axelizePolygon(l + 1, norigin, &trf, axels, diffuseMap, diffuseMapWidth, diffuseMapHeight);
	}

	if (blb.numVerts > 0)
	{
		float norigin[3] = {origin[0] - offset, origin[1] - offset, origin[2] - offset};
		axelizePolygon(l + 1, norigin, &blb, axels, diffuseMap, diffuseMapWidth, diffuseMapHeight);
	}

	if (blf.numVerts > 0)
	{
		float norigin[3] = {origin[0] - offset, origin[1] - offset, origin[2] + offset};
		axelizePolygon(l + 1, norigin, &blf, axels, diffuseMap, diffuseMapWidth, diffuseMapHeight);
	}

	if (brb.numVerts > 0)
	{
		float norigin[3] = {origin[0] + offset, origin[1] - offset, origin[2] - offset};
		axelizePolygon(l + 1, norigin, &brb, axels, diffuseMap, diffuseMapWidth, diffuseMapHeight);
	}

	if (brf.numVerts > 0)
	{
		float norigin[3] = {origin[0] + offset, origin[1] - offset, origin[2] + offset};
		axelizePolygon(l + 1, norigin, &brf, axels, diffuseMap, diffuseMapWidth, diffuseMapHeight);
	}
}

// convert the bitch

int main(int argc, char* argv[]) 
{
	// check for a valid number of parameters

	if (argc < 2)
	{
		printf("\nUsage: objtovts.exe inputfile.obj [-o|-ox outputfile]\n                    [-d diffusemap.bmp] [-v detaillevel]\n\n");
		printf("    inputfile             : Species input mesh file (with file extension)\n");
		printf("-o  outputfile            : Specifies output filename (without file extension)\n");
		printf("-ox outputfile            : As above, but does not output vts file\n");
		printf("-d  diffusemap            : Use the specified texture map to bake the axel\n");
		printf("                            skin. Multiple skins can be baked using one mesh.\n");
		printf("-v  detaillevel           : Explicitly specifies the axel detail level (1-10)\n\n");
		printf("Note: Wavefront .OBJ format is the only mesh format currently supported\n");
		printf("      24-bit .BMP format is the only texture format currently supported\n\n");

		return 0;
	}
	
	// initialize parameters to defaults

	string inputFileName  = argv[1];

	bool outputSurfaceFile = true;

	string outputSurfaceFileName = string(argv[1], strlen(argv[1]) - 4) + string(".vts");
	string outputSkinFileName    = string(argv[1], strlen(argv[1]) - 4) + string(".skn");
	string outputBoneFileName    = string(argv[1], strlen(argv[1]) - 4) + string(".sbn");
	string outputFramesFileName  = string(argv[1], strlen(argv[1]) - 4) + string(".anm");
	
	string diffuseMapFileName = string("");
	int	   diffuseMapWidth    = 0;
	int    diffuseMapHeight   = 0;
	unsigned int *diffuseMap = NULL;

	int   axelDetailLevel = DEFAULT_AXEL_DETAIL_LEVEL;
	Axel **axels          = NULL;

	// get parameters as explicitly defined

	vector<string> args;

	for (int c = 2; c < argc; c++)
	{
		args.push_back(argv[c]);
	}

	for (unsigned int c = 0; c < args.size(); c++)
	{
		string arg = args[c];

		if (arg == string("-o") || arg == string("-ox"))
		{
			if (arg == string("-ox"))
			{
				outputSurfaceFile = false;
			}

			if (++c < args.size())
			{
				outputSurfaceFileName = args[c] + string(".vts");
				outputSkinFileName    = args[c] + string(".skn");
				outputBoneFileName    = args[c] + string(".sbn");
				outputFramesFileName  = args[c] + string(".anm");
			}
			else
			{
				printf("Error - Missing output filename after %s\n", arg.c_str());

				return 1;
			}
		}
		else if (arg == string("-d"))
		{
			if (++c < args.size())
			{
				diffuseMapFileName = args[c];
			}
			else
			{
				printf("Error - Missing diffuse map file name after -d\n");

				return 1;
			}
		}
		else if (arg == string("-v"))
		{
			if (++c < args.size())
			{
				axelDetailLevel = atoi(args[c].c_str());
			}
			else
			{
				printf("Error - Missing axel detail level after -v\n");

				return 1;
			}
		}
		else
		{
			printf("Error - Unrecognized option - '%s'\n", arg.c_str());

			return 1;
		}
	}

	// init SDL to load images

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0)
	{
		printf("Unable to init SDL: %s\n", SDL_GetError());

		return 1;
    }

	// perform validation on explicit parameters and open file handles

	FILE *input = fopen(inputFileName.c_str(), "rb"); // todo: check format based on file extension, for now we're only doing obj though

	if (!input)
	{
		printf("Error - Unable to open input file '%s'\n", inputFileName.c_str());

		return 1;
	}

	// load diffuse map

	if (diffuseMapFileName != string(""))
	{
		printf("Loading diffuse map '%s'...\n", diffuseMapFileName.c_str());

		SDL_Surface *diffuseSurface = SDL_LoadBMP(diffuseMapFileName.c_str()); // todo: check file extension and load other kinds of files

		if (!diffuseSurface)
		{
			printf("Error - Unable to open diffuse map file '%s'\n", diffuseMapFileName.c_str());

			return 1;
		}

		if (diffuseSurface->format->BytesPerPixel != 3)
		{
			printf("Error - Diffuse map does not have 24bit pixel format '%s'\n", diffuseMapFileName.c_str());

			return 1;
		}

		diffuseMapWidth  = diffuseSurface->w;
		diffuseMapHeight = diffuseSurface->h;

		int diffuseMapSize = diffuseMapWidth * diffuseMapHeight;

		diffuseMap = new unsigned int[diffuseMapSize];

		SDL_LockSurface(diffuseSurface);

		unsigned char *pixels = (unsigned char *)diffuseSurface->pixels;

		for (int y = 0; y < diffuseMapHeight; y++)
		{
			for (int x = 0; x < diffuseMapWidth; x++)
			{
				unsigned char r = pixels[(y * diffuseMapWidth + x) * 3 + 0];
				unsigned char g = pixels[(y * diffuseMapWidth + x) * 3 + 1];
				unsigned char b = pixels[(y * diffuseMapWidth + x) * 3 + 2];

				diffuseMap[y * diffuseMapWidth + x] = (r | (g << 8) | (b << 16));
			}
		}

		SDL_UnlockSurface(diffuseSurface);

		SDL_FreeSurface(diffuseSurface);

		printf("Complete - Base diffuse map size = %ix%i\n", diffuseMapWidth, diffuseMapHeight);
	}

	if ((axelDetailLevel < 1) || (axelDetailLevel > 10))
	{
		printf("Error - Invalid axel surface detail level '%i', valid values are 1 to 8\n", axelDetailLevel);

		return 1;
	}

	// read obj file
	
	vector<ObjVertex>  objVertices;  objVertices.reserve(OBJ_LOAD_VECTOR_INITIAL_CAPACITY);
	vector<ObjTVertex> objTVertices; objTVertices.reserve(OBJ_LOAD_VECTOR_INITIAL_CAPACITY);
	vector<ObjTriangle> objTris;     objTris.reserve(OBJ_LOAD_VECTOR_INITIAL_CAPACITY);

	printf("Loading obj file '%s'...\n", inputFileName.c_str());

	unsigned int inputFileSize = fileSize(inputFileName.c_str());

	char *fileBuffer = new char[inputFileSize];
	fread(fileBuffer, 1, inputFileSize, input);

	printf("Parsing obj file '%s'...\n", inputFileName.c_str());

	int lineCount = 1;

	char lineBuffer[OBJ_LOAD_MAX_LINE_LENGTH];
	int  startPos = 0, endPos = 0;

	int tokens[OBJ_LOAD_MAX_TOKENS];
	int numTokens = 0;

	for (unsigned int c = 0; c < inputFileSize; c++)
	{
		char ch = fileBuffer[c];

		switch (ch)
		{
		case 0x0A: // new line
			
			if (startPos != endPos)
			{
				lineBuffer[endPos] = '\0';
	
				tokens[numTokens++] = startPos;
			}

			if (numTokens > 0)
			{
				if (lineBuffer[0] == 'v')
				{
					if (lineBuffer[1] == '\0')
					{
						if (numTokens == 4)
						{
							ObjVertex vertex;

							vertex.x = (float)atof(&lineBuffer[tokens[1]]);
							vertex.y = (float)atof(&lineBuffer[tokens[2]]);
							vertex.z = (float)atof(&lineBuffer[tokens[3]]);

							vertex.nx = 0.0f;
							vertex.ny = 0.0f;
							vertex.nz = 0.0f;

							objVertices.push_back(vertex);
						}
						else
						{
							printf("Error - Invalid number of vertex arguments on line %i of input file '%s'\n", lineCount, inputFileName.c_str());
		
							return 1;
						}
					}
					else if (lineBuffer[1] == 't')
					{
						if (lineBuffer[2] == '\0')
						{
							if (numTokens >= 3)
							{
								ObjTVertex tvertex;

								tvertex.u = (float)atof(&lineBuffer[tokens[1]]);
								tvertex.v = (float)atof(&lineBuffer[tokens[2]]);

								objTVertices.push_back(tvertex);
							}
							else
							{
								printf("Error - Invalid number of uv arguments on line %i of input file '%s'\n", lineCount, inputFileName.c_str());
			
								return 1;
							}
						}
					}
				}
				else if (lineBuffer[0] == 'f')
				{
					if (lineBuffer[1] == '\0')
					{
						if (numTokens == 9)
						{
							ObjTriangle tri;

							tri.v0  = atoi(&lineBuffer[tokens[1]]) - 1;
							tri.vt0 = atoi(&lineBuffer[tokens[2]]) - 1;

							tri.v1  = atoi(&lineBuffer[tokens[3]]) - 1;
							tri.vt1 = atoi(&lineBuffer[tokens[4]]) - 1;

							tri.v2  = atoi(&lineBuffer[tokens[5]]) - 1;
							tri.vt2 = atoi(&lineBuffer[tokens[6]]) - 1;

							objTris.push_back(tri);

							tri.v0  = atoi(&lineBuffer[tokens[5]]) - 1;
							tri.vt0 = atoi(&lineBuffer[tokens[6]]) - 1;

							tri.v1  = atoi(&lineBuffer[tokens[7]]) - 1;
							tri.vt1 = atoi(&lineBuffer[tokens[8]]) - 1;

							tri.v2  = atoi(&lineBuffer[tokens[1]]) - 1;
							tri.vt2 = atoi(&lineBuffer[tokens[2]]) - 1;

							objTris.push_back(tri);

						}
						else if (numTokens == 7)
						{	
							ObjTriangle tri;

							tri.v0  = atoi(&lineBuffer[tokens[1]]) - 1;
							tri.vt0 = atoi(&lineBuffer[tokens[2]]) - 1;

							tri.v1  = atoi(&lineBuffer[tokens[3]]) - 1;
							tri.vt1 = atoi(&lineBuffer[tokens[4]]) - 1;

							tri.v2  = atoi(&lineBuffer[tokens[5]]) - 1;
							tri.vt2 = atoi(&lineBuffer[tokens[6]]) - 1;

							objTris.push_back(tri);
						}
						else
						{
							printf("Error - Invalid number of face arguments on line %i of input file '%s'\n", lineCount, inputFileName.c_str());

							return 1;
						}
					}
				}
			}

			startPos  = 0;
			endPos    = 0;
			numTokens = 0;

			lineCount++;

			break;

		case 0x09: // tab
		case 0x2F: // right slash (/)
		case 0x20: // space

			if (startPos != endPos)
			{
				lineBuffer[endPos++] = '\0';
	
				tokens[numTokens++] = startPos;
	
				startPos = ++endPos;
			}

			break;

		default:
			
			if (ch > 0x20)
			{
				lineBuffer[endPos++] = ch;
			}
		}
	}

	delete [] fileBuffer;

	fclose(input);

	printf("Complete - Mesh statistics - Vertices:       %i\n", objVertices.size());
	printf("                             UV Coordinates: %i\n", objTVertices.size());
	printf("                             Triangles:      %i\n", objTris.size());

	// this is stupid but it's the only way to shrink a vector down to it's real size to conserve memory

	vector<ObjVertex>(objVertices).swap(objVertices);
	vector<ObjTriangle>(objTris).swap(objTris);
	vector<ObjTVertex>(objTVertices).swap(objTVertices);

	// calculate smooth mesh normals

	printf("Calculating normals...\n");

#pragma omp parallel for
	for (int q = 0; q < objTris.size(); q++)
	{
		double du[3];
		double dv[3];

		double dn[3];

		double v0[3] = {objVertices[objTris[q].v0].x, objVertices[objTris[q].v0].y, objVertices[objTris[q].v0].z};
		double v1[3] = {objVertices[objTris[q].v1].x, objVertices[objTris[q].v1].y, objVertices[objTris[q].v1].z};
		double v2[3] = {objVertices[objTris[q].v2].x, objVertices[objTris[q].v2].y, objVertices[objTris[q].v2].z};

		double3Sub(du, v2, v1);
		double3Sub(dv, v1, v0);

		double3Cross(dn, du, dv);
		double3Normalize(dn);
		
		float n[3] = {dn[0], dn[1], dn[2]};

#pragma omp critical
		{
			vec3Add(&objVertices[objTris[q].v0].nx, &objVertices[objTris[q].v0].nx, n);
			vec3Add(&objVertices[objTris[q].v1].nx, &objVertices[objTris[q].v1].nx, n);
			vec3Add(&objVertices[objTris[q].v2].nx, &objVertices[objTris[q].v2].nx, n);
		}
	}

#pragma omp parallel for
	for (int v = 0; v < objVertices.size(); v++)
	{
		vec3Normalize(&objVertices[v].nx);
	}

	// normalize mesh coordinates and center them such that -1...+1 for all 3 dimensions

	printf("Normalizing spatial range...\n");

	float pmin[3] = {objVertices[0].x, objVertices[0].y, objVertices[0].z};
	float pmax[3] = {pmin[0], pmin[1], pmin[2]};

	for (int v = 1; v < objVertices.size(); v++)
	{
		if (objVertices[v].x < pmin[0]) pmin[0] = objVertices[v].x;
		if (objVertices[v].y < pmin[1]) pmin[1] = objVertices[v].y;
		if (objVertices[v].z < pmin[2]) pmin[2] = objVertices[v].z;

		if (objVertices[v].x > pmax[0]) pmax[0] = objVertices[v].x;
		if (objVertices[v].y > pmax[1]) pmax[1] = objVertices[v].y;
		if (objVertices[v].z > pmax[2]) pmax[2] = objVertices[v].z;
	}
	
	float pcenter[3];
	vec3Add(pcenter, pmin, pmax);
	pcenter[0] *= 0.5f; pcenter[1] *= 0.5f; pcenter[2] *= 0.5f;

	float psize[3];
	vec3Sub(psize, pmax, pcenter);

	float maxSize = psize[0];
	if (psize[1] > maxSize) maxSize = psize[1];
	if (psize[2] > maxSize) maxSize = psize[2];

	float meshScale = 1.0f / maxSize;

#pragma omp parallel for
	for (int v = 0; v < objVertices.size(); v++)
	{
		vec3Sub(&objVertices[v].x, &objVertices[v].x, pcenter);

		objVertices[v].x *= meshScale;
		objVertices[v].y *= meshScale;
		objVertices[v].z *= meshScale;
	}

	printf("Refining geometry for axelization...\n");

	vector<Axel> baxels; baxels.reserve(0xA00000);

#pragma omp parallel for
	for (int q = 0; q < objTris.size(); q++)
	{
		int baxelsSize = baxels.size();

		Polygon p; p.numVerts = 3;

		p.verts[0].pos[0]  = objVertices[objTris[q].v0].x;
		p.verts[0].pos[1]  = objVertices[objTris[q].v0].y;
		p.verts[0].pos[2]  = objVertices[objTris[q].v0].z;
		p.verts[0].norm[0] = objVertices[objTris[q].v0].nx;
		p.verts[0].norm[1] = objVertices[objTris[q].v0].ny;
		p.verts[0].norm[2] = objVertices[objTris[q].v0].nz;
		p.verts[0].uv[0]   = objTVertices[objTris[q].vt0].u;
		p.verts[0].uv[1]   = objTVertices[objTris[q].vt0].v;

		p.verts[1].pos[0]  = objVertices[objTris[q].v1].x;
		p.verts[1].pos[1]  = objVertices[objTris[q].v1].y;
		p.verts[1].pos[2]  = objVertices[objTris[q].v1].z;
		p.verts[1].norm[0] = objVertices[objTris[q].v1].nx;
		p.verts[1].norm[1] = objVertices[objTris[q].v1].ny;
		p.verts[1].norm[2] = objVertices[objTris[q].v1].nz;
		p.verts[1].uv[0]   = objTVertices[objTris[q].vt1].u;
		p.verts[1].uv[1]   = objTVertices[objTris[q].vt1].v;

		p.verts[2].pos[0]  = objVertices[objTris[q].v2].x;
		p.verts[2].pos[1]  = objVertices[objTris[q].v2].y;
		p.verts[2].pos[2]  = objVertices[objTris[q].v2].z;
		p.verts[2].norm[0] = objVertices[objTris[q].v2].nx;
		p.verts[2].norm[1] = objVertices[objTris[q].v2].ny;
		p.verts[2].norm[2] = objVertices[objTris[q].v2].nz;
		p.verts[2].uv[0]   = objTVertices[objTris[q].vt2].u;
		p.verts[2].uv[1]   = objTVertices[objTris[q].vt2].v;

		float origin[3] = {0.0f, 0.0f, 0.0f};

		axelizePolygon(0, origin, &p, &baxels, diffuseMap, diffuseMapWidth, diffuseMapHeight);
	}

	if (diffuseMap) delete [] diffuseMap;

	// stupid looking but this is sadly the only way to REALLY get a vector to fuck off and release its memory

	vector<ObjTriangle>().swap(objTris);
	vector<ObjVertex>().swap(objVertices);
	vector<ObjTVertex>().swap(objTVertices);

	vector<Axel>(baxels).swap(baxels); // free unused reserve

	printf("Optimizing spatial locality...\n");

	sort(baxels.begin(), baxels.end());

	// and then average the redundant overlapping axels

	vector<Axel> paxels;

	unsigned long long ohval = baxels[0].hval;

	unsigned int ox = baxels[0].x;
	unsigned int oy = baxels[0].y;
	unsigned int oz = baxels[0].z;

	long long int onx = baxels[0].nx;
	long long int ony = baxels[0].ny;
	long long int onz = baxels[0].nz;

    unsigned int or = (baxels[0].color & 0x000000FF) >> 0;
	unsigned int og = (baxels[0].color & 0x0000FF00) >> 8;
	unsigned int ob = (baxels[0].color & 0x00FF0000) >> 16;

	unsigned int numOverlapped = 0;

	for (int v = 1; v < baxels.size(); v++)
	{
		if ((baxels[v].hval != ohval) || (v == baxels.size() - 1))
		{
			Axel axel;

			axel.x = ox;
			axel.y = oy;
			axel.z = oz;
			
			axel.nx = int(double(onx) / double(numOverlapped + 1.0));
			axel.ny = int(double(ony) / double(numOverlapped + 1.0));
			axel.nz = int(double(onz) / double(numOverlapped + 1.0));

			or = double(or) / double(numOverlapped + 1.0);
			og = double(og) / double(numOverlapped + 1.0);
			ob = double(ob) / double(numOverlapped + 1.0);

			axel.color = (int(or) | (int(og) << 8) | (int(ob) << 16));

			paxels.push_back(axel);

			numOverlapped = 0;

			ohval = baxels[v].hval;

			ox = baxels[v].x;
			oy = baxels[v].y;
			oz = baxels[v].z;

			onx = baxels[v].nx;
			ony = baxels[v].ny;
			onz = baxels[v].nz;

			or = ((baxels[v].color & 0x000000FF) >> 0);
			og = ((baxels[v].color & 0x0000FF00) >> 8);
			ob = ((baxels[v].color & 0x00FF0000) >> 16);
		}
		else
		{
			onx += baxels[v].nx;
			ony += baxels[v].ny;
			onz += baxels[v].nz;

			or += ((baxels[v].color & 0x000000FF) >> 0);
			og += ((baxels[v].color & 0x0000FF00) >> 8);
			ob += ((baxels[v].color & 0x00FF0000) >> 16);

			numOverlapped++;
		}
	}

	vector<Axel>().swap(baxels); // and force the stupid vector to fuck off and die and release it's memory while it's dying
	vector<Axel>(paxels).swap(paxels);

	int detailUtilization = int((4.0 * pow(4.0, double(axelDetailLevel))) / double(paxels.size()) * 100.0);

	printf("Complete - Generated %i axel fragments\n", paxels.size());
	printf("           Approx detail utilization : %i%%\n", detailUtilization);
	
	if (detailUtilization > 100)
	{
		printf("Warning - There is not enough surface detail for the targeted axel detail level and some axels will be redundant. Lower the target detail level and try again.\n");
	}

	printf("Generating optimal axel LODs...\n");

	axels = new Axel*[axelDetailLevel];

	int numLayerAxels = 4;

	for (int l = 0; l < axelDetailLevel; l++)
	{
		numLayerAxels *= 4;
		axels[l] = new Axel[numLayerAxels];
		
		double fragmentsPerAxel = double(paxels.size()) / double(numLayerAxels);
		double fi = 0.0;
		
		bool discontinuity = false;

		for (int v = 0; v < numLayerAxels; v++)
		{
			unsigned int sf = unsigned int(fi);
			unsigned int ef = unsigned int(fi + fragmentsPerAxel);

			if (ef > (paxels.size() - 1)) ef = paxels.size() - 1;
			
			bool disconnected = false;

			if (discontinuity || (((v - 1) % 256) == 0))
			{
				disconnected = true;
			}

			discontinuity = false;

			long long int x = 0;
			long long int y = 0;
			long long int z = 0;

			long long int nx = 0;
			long long int ny = 0;
			long long int nz = 0;

			long long int r = 0;
			long long int g = 0;
			long long int b = 0;

			int cv = sf;	

			while (cv <= ef)
			{
				x += paxels[cv].x; y += paxels[cv].y; z += paxels[cv].z;

				nx += paxels[cv].nx; ny += paxels[cv].ny; nz += paxels[cv].nz;
				
				r += ((paxels[cv].color & 0x000000FF) >> 0);
				g += ((paxels[cv].color & 0x0000FF00) >> 8);
				b += ((paxels[cv].color & 0x00FF0000) >> 16);

				if (cv < ef)
				{
					int dx = paxels[cv].x - paxels[cv + 1].x;
					int dy = paxels[cv].y - paxels[cv + 1].y;
					int dz = paxels[cv].z - paxels[cv + 1].z;

					unsigned int sdist = dx * dx + dy * dy + dz * dz;

					if (sdist > 64) // todo: tweak hard-coded discontinuity detection distance
					{
						discontinuity = true;

						break;
					}
				}

				cv++;
			}

			x = int(double(x) / double(cv - sf));       if ( x < 0) x = 0; if (x > 1024 - 1) x = 1024 - 1;
			y = int(2.0 * double(y) / double(cv - sf)); if ( y < 0) y = 0; if (y > 2048 - 1) y = 2048 - 1;
			z = int(double(z) / double(cv - sf));       if ( z < 0) z = 0; if (z > 1024 - 1) z = 1024 - 1;

			nx = int(double(nx) / double(cv - sf));
			ny = int(double(ny) / double(cv - sf));
			nz = int(double(nz) / double(cv - sf));

			r = int(double(r) / double(cv - sf)); if ( r < 0) r = 0; if (r > 256 - 1) r = 256 - 1;
			g = int(double(g) / double(cv - sf)); if ( g < 0) g = 0; if (g > 256 - 1) g = 256 - 1;
			b = int(double(b) / double(cv - sf)); if ( b < 0) b = 0; if (b > 256 - 1) b = 256 - 1;
			
			axels[l][v].x = unsigned short(x); axels[l][v].y = unsigned short(y); axels[l][v].z = unsigned short(z);
			axels[l][v].nx = short(nx); axels[l][v].ny = short(ny); axels[l][v].nz = short(nz);

			axels[l][v].color = (int(r) | (int(g) << 8) | (int(b) << 16));

			axels[l][v].hval = !disconnected;

			fi += fragmentsPerAxel;
		}
	}

	vector<Axel>().swap(paxels);

	// write output files

	printf("Complete - Writing output surface files...\n");

	FILE *output;

	if (outputSurfaceFile)
	{
		output = fopen(outputSurfaceFileName.c_str(), "wb");

		if (!output)
		{
			printf("Error - Unable to open output file '%s'\n", outputSurfaceFileName.c_str());
		}
		else
		{
			VTSHeader header;

			header.magic = 0x03460566;
			header.formatVersion  = 0x10;
			header.surfaceFlags = 0x00;	// specifies axel surface
			header.numLayers    = (unsigned char)axelDetailLevel;

			fwrite(&header, 1, sizeof(header), output);

			numLayerAxels = 4;

			for (int l = 0; l < axelDetailLevel; l++)
			{
				numLayerAxels *= 4;

				for (int v = 0; v < numLayerAxels; v++)
				{
					int x = int(axels[l][v].x);
					int y = int(axels[l][v].y);
					int z = int(axels[l][v].z);
					int d = int(axels[l][v].hval);

					unsigned int pos = (x | (y << 10) | (z << 21) | (d << 31));

					fwrite(&pos, 1, 4, output);
				}
			}

			int numBucketLayers = (axelDetailLevel > 6) ? 6 : axelDetailLevel;
			
			numLayerAxels = 4;

			for (int l = 0; l < numBucketLayers; l++)
			{
				numLayerAxels *= 4;

				for (int v = 0; v < numLayerAxels; v++)
				{
					int x = unsigned int(double(axels[l][v].nx + 32767.0) / 65534.0 * 255.0);
					int y = unsigned int(double(axels[l][v].ny + 32767.0) / 65534.0 * 255.0);
					int z = unsigned int(double(axels[l][v].nz + 32767.0) / 65534.0 * 255.0);
					int r = unsigned int((1.0 - double(l) / double(numBucketLayers)) * 255.0);  // todo: tweak calculation of r for conservative accuracy

					unsigned int normal = (x | (y << 8) | (z << 16) | (r << 24));

					fwrite(&normal, 1, 4, output);
				}
			}

			printf("Wrote '%s' (%i kb)\n", outputSurfaceFileName.c_str(), int(ftell(output)) / 1024);

			fclose(output);
		}
	}

	// todo: if bones / animation data are present write those files as well

	if (diffuseMap != NULL)
	{
		output = fopen(outputSkinFileName.c_str(), "wb");

		if (!output)
		{
			printf("Error - Unable to open output file '%s'\n", outputSurfaceFileName.c_str());
		}
		else
		{
			VTSHeader header;

			header.magic = 0x03460566;
			header.formatVersion  = 0x10;
			header.surfaceFlags = 0x01;	// specifies skin surface
			header.numLayers    = (unsigned char)axelDetailLevel;

			fwrite(&header, 1, sizeof(header), output);

			numLayerAxels = 4;

			for (int l = 0; l < axelDetailLevel; l++)
			{
				numLayerAxels *= 4;

				for (int v = 0; v < numLayerAxels; v++)
				{
					// convert to 16 bit color r5g6b5

					unsigned int r;
					unsigned int g;
					unsigned int b;

					r = unsigned int((double((axels[l][v].color & 0x000000FF) >> 0)  / 255.0 * 31.0));
					g = unsigned int((double((axels[l][v].color & 0x0000FF00) >> 8)  / 255.0 * 63.0));
					b = unsigned int((double((axels[l][v].color & 0x00FF0000) >> 16) / 255.0 * 31.0));

					unsigned short color = (r | (g << 5) | ( b << 11));

					fwrite(&color, 1, 2, output);
				}
			}

			printf("Complete - Sucessfully wrote '%s' (%i kb)\n", outputSkinFileName.c_str(), int(ceil(double(ftell(output)) / 1024.0)));

			fclose(output);
		}
	}

	// release the last of the memory

	for (int l = 0; l < axelDetailLevel; l++)
	{
		delete [] axels[l];
	}

	delete [] axels;

	SDL_Quit();

	return EXIT_SUCCESS;
}