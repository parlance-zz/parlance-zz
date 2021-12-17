#ifndef _VTRACE_KERNELS_H_
#define _VTRACE_KERNELS_H_

#define VTRACE_MAX_BATCH_SURFACES_PER_LAYER		128
#define VTRACE_MAX_BATCH_SURFACES				5 * VTRACE_MAX_BATCH_SURFACES_PER_LAYER

#define VTRACE_MAX_BATCH_BUCKETS				0x400000
#define VTRACE_MAX_COMPACTION_INDICES			VTRACE_MAX_BATCH_BUCKETS / 16

#define VTRACE_AXEL_SIZE_SCALE					16.0f

struct __align__(16) CoreRenderSurface
{
	unsigned int width;
	unsigned int height;

	unsigned int *s_fBuffer;	// swizzled buffers used directly by core
	unsigned int *s_vBuffer;
	unsigned int *s_zBuffer;

	unsigned int *c_fBuffer;	// unswizzled buffers that are mapped from layer textures by composite, the unswizzle from core copies into these
	unsigned int *c_vBuffer;
	unsigned int *c_zBuffer;
};

struct __align__(16) DeviceSurfaceState
{
	unsigned int   *surfaceData;
	unsigned short *skinData;

	unsigned int    material; // only the high bits are used

	unsigned int   *normalsData;
};

struct __align__(16) DeviceSceneState
{
	CoreRenderSurface renderTarget[VTRACE_NUM_RECONSTRUCTION_LAYERS];

	DeviceSurfaceState renderSurfaces[VTRACE_MAX_BATCH_SURFACES];

	unsigned int layerSurfaceIndices[5][VTRACE_MAX_BATCH_SURFACES_PER_LAYER];

	unsigned int *renderBucketsIn;
	unsigned int *compactionIndicesIn;

	unsigned int *renderBucketsOut;
	unsigned int *compactionIndicesOut;
};

__global__ void bucketGenerationKernel(unsigned int layerNum, unsigned int axelOffset, unsigned int compactionIndicesOffset);
__global__ void bucketScanKernel(unsigned int numCompactionIndices);
__global__ void bucketCompactionKernel(unsigned int numCompactionIndices);
__global__ void bucketRenderingKernel(unsigned int numBuckets);

__global__ void clearSurfaceKernel(unsigned int layerNum, unsigned int bufferSize);
__global__ void unswizzleSurfaceKernel(unsigned int layerNum);

#endif