// todo: - finish flush
//       - write bucket generation
//       - write bucket compaction
//       - change out hierarchical buffers and use generate screenspace normals and size using the assumption that normals face the viewer, stored packed and swizzled
//       - change out unswizzle to unpack while it unswizzles for easier processing in shaders
//       - test and optimize
//       - add optimized rendering kernel for shadowmap / depth only with scattered tiny directional lights for global illumination, possibly add direct cubemap rendering

#include "vtrace.h"

texture<unsigned int, 1, cudaReadModeElementType> dRenderBucketsTex;

texture<float4,  3,  cudaReadModeElementType> dSurfaceLatticesTex;
texture<float4,  3,  cudaReadModeElementType> dSurfaceLattices0Tex;

__constant__ DeviceSceneState dSceneState;

__device__ float3 axelUnpack(unsigned int axel)
{
	return make_float3(((axel & 0x000003FF)      ) * (1.0f / 1023.0f),
		               ((axel & 0x001FFC00) >> 10) * (1.0f / 2047.0f),
					   ((axel & 0x7FE00000) >> 21) * (1.0f / 1023.0f));
}

__device__ float4 normalUnpack(unsigned int normal)
{
	return make_float4(((normal & 0x000000FF)       ) * (1.0f / 127.5f) - 1.0f,
		               ((normal & 0x0000FF00) >>   8) * (1.0f / 127.5f) - 1.0f,
					   ((normal & 0x00FF0000) >>  16) * (1.0f / 127.5f) - 1.0f,
					   ((normal & 0xFF000000) >>  24) * (1.0f / 127.5f) - 1.0f);
}

__device__ float3 cross(float3 du, float3 dv)
{
	return make_float3(((du.y * dv.z) - (du.z * dv.y)),
		               ((du.z * dv.x) - (du.x * dv.z)),
					   ((du.x * dv.y) - (du.y * dv.x)));
}

__device__ float3 normalize(float3 n)
{
	float il = rsqrtf(n.x * n.x + n.y * n.y + n.z * n.z);

	return make_float3(n.x * il, n.y * il, n.z * il);
}

__global__ void bucketScanKernel(unsigned int numCompactionIndices)
{
   __shared__  unsigned int temp[1024];

    const unsigned short thid   = threadIdx.x;
	const unsigned short thid2  = thid << 1;
	const unsigned short thid21 = thid2 + 1;
	const unsigned short thid22 = thid21 + 1;

	const unsigned int gid  = (blockIdx.x << 10) + thid2;
	const unsigned int gid1 = gid + 1;

    unsigned short offset = 0;

	if (gid < numCompactionIndices)
	{
		temp[thid2]  = dSceneState.compactionIndicesIn[gid];
		temp[thid21] = dSceneState.compactionIndicesIn[gid1];
	}
	else
	{
		temp[thid2] = 0;
		temp[thid21] = 0;
	}

#pragma unroll
    for (unsigned short d = 512; d > 0; d >>= 1)
    {
        __syncthreads();

        if (thid < d)
        {
            unsigned int ai = (thid21 << offset) - 1;
            unsigned int bi = (thid22 << offset) - 1;

            temp[bi] += temp[ai];
        }

        offset++;
    }

    if (thid == 0)
    {
        temp[1023] = 0;
    }   

#pragma unroll
    for (unsigned short d = 1; d < 1024; d <<= 1)
    {
        offset --;

        __syncthreads();

        if (thid < d)
        {
            unsigned int ai = (thid21 << offset) - 1;
            unsigned int bi = (thid22 << offset) - 1;

            unsigned int t = temp[ai];

            temp[ai]  = temp[bi];
            temp[bi] += t;
        }
    }

    __syncthreads();

    dSceneState.compactionIndicesOut[thid2]  = temp[gid];
    dSceneState.compactionIndicesOut[thid21] = temp[gid1];
}

__global__ void bucketGenerationKernel(unsigned int layerNum, unsigned int axelOffset, unsigned int compactionIndicesOffset)
{
	// todo:
}

__global__ void bucketCompactionKernel(unsigned int numCompactionIndices)
{
	// todo:
}

__global__ void bucketRenderingKernel(unsigned int numBuckets) 
{
	if (blockIdx.x >= numBuckets) return;

	const unsigned int bucketInfo   = tex1Dfetch(dRenderBucketsTex, blockIdx.x);

	const unsigned int surfaceIndex = bucketInfo & 0x0000FFFF;
	const unsigned int axelIndex    = ((bucketInfo & 0xFFFF0000) >> 8) + threadIdx.x;

	const unsigned int _axel     = dSceneState.renderSurfaces[surfaceIndex].surfaceData[axelIndex];

	float3 axelLocalPos = axelUnpack(_axel);

	axelLocalPos.z += surfaceIndex;

	float4 axelPos  = tex3D(dSurfaceLatticesTex,  axelLocalPos.x, axelLocalPos.y, axelLocalPos.z);

	float inverseAxelW  = 1.0f / axelPos.w;

	__shared__ float frameX[256];
	__shared__ float frameY[256];

	unsigned int axelDepth = axelPos.z * inverseAxelW * float(0x00FFFFFFFF);

	frameX[threadIdx.x] = (axelPos.x * inverseAxelW + 0.5f) * dSceneState.renderTarget[0].width;
	frameY[threadIdx.x] = (axelPos.y * inverseAxelW + 0.5f) * dSceneState.renderTarget[0].height;

	unsigned short nextAxelIndex = threadIdx.x + (_axel >> 31);	// discontinuity protection is stored in the high bit

	__syncthreads();

	float size = hypotf(frameX[nextAxelIndex] - frameX[threadIdx.x], frameY[nextAxelIndex] - frameY[threadIdx.x]);

	unsigned char layerNum = min(VTRACE_NUM_RECONSTRUCTION_LAYERS, __float2uint_rd(__log2f(size)));

	frameX[threadIdx.x] = __int_as_float(__float_as_int(frameX[threadIdx.x]) - (layerNum << 23));	// like hell I can't shift floats
	frameY[threadIdx.x] = __int_as_float(__float_as_int(frameY[threadIdx.x]) - (layerNum << 23));

	size = __int_as_float(__float_as_int(size) - (layerNum << 23));

	unsigned int _size = min(unsigned int(size * VTRACE_AXEL_SIZE_SCALE), 255) << 24;

	ushort2 pixelCoords = make_ushort2(__float2int_rd(frameX[threadIdx.x]), __float2int_rd(frameY[threadIdx.x]));

	pixelCoords.x = min(pixelCoords.x, dSceneState.renderTarget[layerNum].width - 1);
	pixelCoords.y = min(pixelCoords.y, dSceneState.renderTarget[layerNum].height - 1);
	
	unsigned int frameBufferFragmentPos  = __umul24(pixelCoords.y & 0xFFFC, dSceneState.renderTarget[layerNum].width) +
			                                       (pixelCoords.x & 0x03) + ((pixelCoords.y & 0x03) << 2) + ((pixelCoords.x & 0xFFFC) << 2);

	if (axelDepth < (dSceneState.renderTarget[layerNum].s_zBuffer[frameBufferFragmentPos] & 0x00FFFFFFFF))
	{
		dSceneState.renderTarget[layerNum].s_zBuffer[frameBufferFragmentPos] = (axelDepth | _size);

		const unsigned int _material = dSceneState.renderSurfaces[surfaceIndex].skinData[axelIndex] | dSceneState.renderSurfaces[surfaceIndex].material;

		float4 axelPos0 = tex3D(dSurfaceLattices0Tex, axelLocalPos.x, axelLocalPos.y, axelLocalPos.z);

		uchar2 subPixelPos = make_uchar2((frameX[threadIdx.x] - pixelCoords.x) * 255.0f, (frameY[threadIdx.x] - pixelCoords.y) * 255.0f);

		float inverseAxelW0 = 1.0f / axelPos0.w;

		uchar2 axelVelocity = make_uchar2(fminf(fmaxf((axelPos0.x * inverseAxelW0 + 0.5f) * dSceneState.renderTarget[0].width  - frameX[threadIdx.x], -127.5f), 127.5f) + 127.5f,
		    					          fminf(fmaxf((axelPos0.y * inverseAxelW0 + 0.5f) * dSceneState.renderTarget[0].height - frameY[threadIdx.x], -127.5f), 127.5f) + 127.5f);

		dSceneState.renderTarget[layerNum].s_vBuffer[frameBufferFragmentPos] = (subPixelPos.x << 24) + (subPixelPos.y << 16) + (axelVelocity.x << 8) + (axelVelocity.y);

		dSceneState.renderTarget[layerNum].s_fBuffer[frameBufferFragmentPos] = _material;
	}
}

__global__ void clearSurfaceKernel(unsigned int layerNum, unsigned int bufferSize)
{
	dSceneState.renderTarget[layerNum].s_zBuffer[min(__umul24(blockIdx.x, gridDim.x) + threadIdx.x, bufferSize)] = 0x00FFFFFF;
}

__global__ void unswizzleSurfaceKernel(unsigned int layerNum)
{
	__shared__ unsigned int zBuffer[128];
	__shared__ unsigned int fBuffer[128];
	__shared__ unsigned int vBuffer[128];

	const unsigned int threadId     = (min(threadIdx.z, (dSceneState.renderTarget[layerNum].width >> 4) - 1) << 4)
		                             + min(threadIdx.x,  dSceneState.renderTarget[layerNum].width & 0x03) + (threadIdx.y << 2);

	const unsigned int blockId      = __umul24(blockIdx.y, gridDim.x) + blockIdx.x;

	const unsigned int sourceOffset = threadId + (blockId << 7);

	zBuffer[threadId] = dSceneState.renderTarget[layerNum].s_zBuffer[sourceOffset];
	fBuffer[threadId] = dSceneState.renderTarget[layerNum].s_fBuffer[sourceOffset];
	vBuffer[threadId] = dSceneState.renderTarget[layerNum].s_vBuffer[sourceOffset];

	const unsigned int sharedOffset  = (threadIdx.y << 4) + ((threadIdx.z & 0xFE) << 1) + ((threadIdx.z & 0x01) << 6) + threadIdx.x;

	const unsigned int destOffset    = min((blockIdx.x << 5) + (threadIdx.y << 2) + ((threadIdx.z & 0x01) << 4) + threadIdx.x, dSceneState.renderTarget[layerNum].width - 1)
		                             + __umul24(min((threadIdx.z >> 1) + (blockIdx.y << 2), dSceneState.renderTarget[layerNum].height - 1), dSceneState.renderTarget[layerNum].width);

	__syncthreads();

#ifndef __DEVICE_EMULATION__

	dSceneState.renderTarget[layerNum].c_zBuffer[destOffset] = zBuffer[sharedOffset];
	dSceneState.renderTarget[layerNum].c_fBuffer[destOffset] = fBuffer[sharedOffset];
	dSceneState.renderTarget[layerNum].c_vBuffer[destOffset] = vBuffer[sharedOffset];

#endif
}

void vEngine::CoreFlushBatch()
{
	// todo:

	//unsigned int compactionIndicesOffset = 0;

	for (int i = 0; i < 5; i++)
	{

	}


	/*
	cudaStreamSynchronize(deviceStream);

	void *_sceneState = (void*)sceneState;
	cudaMemcpyToSymbolAsync(dSceneState, _sceneState, sizeof(DeviceSceneState), 0, cudaMemcpyHostToDevice, deviceStream);

	int numBlocks = (numLayerVoxels * 4) / 256;

	dim3 dimBlock(256, 1, 1);
	dim3 dimGrid(32, numBlocks >> 5, 1);

	voxelSurfaceKernel<<<dimGrid, dimBlock, 0, deviceStream>>>();
	*/

	numRenderSurfaces = 0;

	for (int i = 0; i < 5; i++)
	{
		numLayerSurfaces[i] = 0;
	}

	numRenderBuckets = 0;
	numCompactionIndices = 0;
}

void vEngine::RenderSurface(Surface *surface)
{
	vVec4 *latticePoints = (vVec4*)&surfaceLattice;

	float minX = (latticePoints[0].x * (1.0f / latticePoints[0].w) + 0.5f) * primaryRenderTarget.coreSurfaceLevels[0].width;
	float minY = (latticePoints[0].y * (1.0f / latticePoints[0].w) + 0.5f) * primaryRenderTarget.coreSurfaceLevels[0].height;
	float minZ = latticePoints[0].z *  (1.0f / latticePoints[0].w);

	float maxX = minX;
	float maxY = minY;
	float maxZ = minZ;

	for (int p = 1; p < 8; p++)
	{
		float cx = (latticePoints[p].x * (1.0f / latticePoints[0].w) + 0.5f) * primaryRenderTarget.coreSurfaceLevels[0].width;
		float cy = (latticePoints[p].y * (1.0f / latticePoints[0].w) + 0.5f) * primaryRenderTarget.coreSurfaceLevels[0].height;
		float cz = latticePoints[p].z *  (1.0f / latticePoints[0].w);

		if (cx < minX) minX = cx; if (cx > maxX) maxX = cx;
		if (cy < minY) minY = cy; if (cy > maxY) maxY = cy;
		if (cz < minZ) minZ = cz; if (cz > maxZ) maxZ = cz;
	}

	if ((maxX < 0.0f) || (minX > primaryRenderTarget.coreSurfaceLevels[0].width) ||
		(maxY < 0.0f) || (minY > primaryRenderTarget.coreSurfaceLevels[0].height) ||
		(maxZ < 0.0f) || (minZ > 1.0f))
	{
		return;
	}
	
	float sizeX = maxX - minX;
	float sizeY = maxY - minY;

	float size = sizeX > sizeY ? sizeX : sizeY;

	int layerNum = int(log2f(size)) - 4;		// todo: this probably needs a fair bit of tweaking

	layerNum = layerNum < 0 ? 0 : layerNum;
	layerNum = layerNum + 5 > surface->mapLevels ? surface->mapLevels - 5 : layerNum;
	
	unsigned int numSurfaceCompactionIndices = 16 << (layerNum << 1);
	unsigned int numSurfaceBuckets = numSurfaceCompactionIndices << 4;	

	if ((numRenderSurfaces == VTRACE_MAX_BATCH_SURFACES) ||
		(numLayerSurfaces[layerNum] == VTRACE_MAX_BATCH_SURFACES_PER_LAYER) ||
		((numRenderBuckets + numSurfaceBuckets) >= VTRACE_MAX_BATCH_BUCKETS) ||
		((numCompactionIndices + numSurfaceCompactionIndices) >= VTRACE_MAX_COMPACTION_INDICES))
	{
		CoreFlushBatch();
	}

	sceneState->renderSurfaces[numRenderSurfaces].surfaceData = (unsigned int *)surface->surfaceData;
	sceneState->renderSurfaces[numRenderSurfaces].normalsData = (unsigned int *)surface->normalsData;

	sceneState->renderSurfaces[numRenderSurfaces].skinData = (unsigned short *)surfaceSkin->skinData;
	sceneState->renderSurfaces[numRenderSurfaces].material = surfaceMaterial;

	sceneState->layerSurfaceIndices[layerNum][numLayerSurfaces[layerNum]] = numRenderSurfaces;

	surfaceLattices[numRenderSurfaces]  = surfaceLattice;
	surfaceLattices0[numRenderSurfaces] = surfaceLattice0;

	numRenderSurfaces++;
	numLayerSurfaces[layerNum]++;

	numRenderBuckets     += numSurfaceBuckets;
	numCompactionIndices += numSurfaceCompactionIndices;
}

void vEngine::SetModelMatrix(vMat4x4 *matrix, vMat4x4 *matrix0)
{
	vMat4x4 worldProjectionMatrix;
	vMatrixMultiply(&worldProjectionMatrix, &viewProjectionMatrix, matrix);

	vMatrixMultiplyVec4(&surfaceLattice.c0, &worldProjectionMatrix, &identityLattice.c0);
	vMatrixMultiplyVec4(&surfaceLattice.c1, &worldProjectionMatrix, &identityLattice.c1);
	vMatrixMultiplyVec4(&surfaceLattice.c2, &worldProjectionMatrix, &identityLattice.c2);
	vMatrixMultiplyVec4(&surfaceLattice.c3, &worldProjectionMatrix, &identityLattice.c3);
	vMatrixMultiplyVec4(&surfaceLattice.c4, &worldProjectionMatrix, &identityLattice.c4);
	vMatrixMultiplyVec4(&surfaceLattice.c5, &worldProjectionMatrix, &identityLattice.c5);
	vMatrixMultiplyVec4(&surfaceLattice.c6, &worldProjectionMatrix, &identityLattice.c6);
	vMatrixMultiplyVec4(&surfaceLattice.c7, &worldProjectionMatrix, &identityLattice.c7);

	vMat4x4 worldProjectionMatrix0;

	if (matrix0)
	{
		vMatrixMultiply(&worldProjectionMatrix0, &viewProjectionMatrix0, matrix0);
	}
	else
	{
		vMatrixMultiply(&worldProjectionMatrix0, &viewProjectionMatrix0, matrix);
	}

	vMatrixMultiplyVec4(&surfaceLattice0.c0, &worldProjectionMatrix0, &identityLattice.c0);
	vMatrixMultiplyVec4(&surfaceLattice0.c1, &worldProjectionMatrix0, &identityLattice.c1);
	vMatrixMultiplyVec4(&surfaceLattice0.c2, &worldProjectionMatrix0, &identityLattice.c2);
	vMatrixMultiplyVec4(&surfaceLattice0.c3, &worldProjectionMatrix0, &identityLattice.c3);
	vMatrixMultiplyVec4(&surfaceLattice0.c4, &worldProjectionMatrix0, &identityLattice.c4);
	vMatrixMultiplyVec4(&surfaceLattice0.c5, &worldProjectionMatrix0, &identityLattice.c5);
	vMatrixMultiplyVec4(&surfaceLattice0.c6, &worldProjectionMatrix0, &identityLattice.c6);
	vMatrixMultiplyVec4(&surfaceLattice0.c7, &worldProjectionMatrix0, &identityLattice.c7);

	modelMatrix  = *matrix;		// todo: sterling needs to know that if lattices are used we simply won't have a model matrix and we shouldn't depend on having one
	modelMatrix0 = *matrix0;
}

void vEngine::SetLattice(vLattice *lattice, vLattice *lattice0)
{
	vMatrixMultiplyVec4(&surfaceLattice.c0, &viewProjectionMatrix, &lattice->c0);
	vMatrixMultiplyVec4(&surfaceLattice.c1, &viewProjectionMatrix, &lattice->c1);
	vMatrixMultiplyVec4(&surfaceLattice.c2, &viewProjectionMatrix, &lattice->c2);
	vMatrixMultiplyVec4(&surfaceLattice.c3, &viewProjectionMatrix, &lattice->c3);
	vMatrixMultiplyVec4(&surfaceLattice.c4, &viewProjectionMatrix, &lattice->c4);
	vMatrixMultiplyVec4(&surfaceLattice.c5, &viewProjectionMatrix, &lattice->c5);
	vMatrixMultiplyVec4(&surfaceLattice.c6, &viewProjectionMatrix, &lattice->c6);
	vMatrixMultiplyVec4(&surfaceLattice.c7, &viewProjectionMatrix, &lattice->c7);

	if (!lattice0) lattice0 = lattice;

	vMatrixMultiplyVec4(&surfaceLattice0.c0, &viewProjectionMatrix0, &lattice0->c0);
	vMatrixMultiplyVec4(&surfaceLattice0.c1, &viewProjectionMatrix0, &lattice0->c1);
	vMatrixMultiplyVec4(&surfaceLattice0.c2, &viewProjectionMatrix0, &lattice0->c2);
	vMatrixMultiplyVec4(&surfaceLattice0.c3, &viewProjectionMatrix0, &lattice0->c3);
	vMatrixMultiplyVec4(&surfaceLattice0.c4, &viewProjectionMatrix0, &lattice0->c4);
	vMatrixMultiplyVec4(&surfaceLattice0.c5, &viewProjectionMatrix0, &lattice0->c5);
	vMatrixMultiplyVec4(&surfaceLattice0.c6, &viewProjectionMatrix0, &lattice0->c6);
	vMatrixMultiplyVec4(&surfaceLattice0.c7, &viewProjectionMatrix0, &lattice0->c7);
}

void vEngine::SetViewMatrix(vMat4x4 *matrix, vMat4x4 *matrix0)
{
	viewMatrix = (*matrix);

	if (matrix0)
	{
		viewMatrix0 = (*matrix0);
	}
	else
	{
		viewMatrix0 = (*matrix);
	}

	vMatrixMultiply(&viewProjectionMatrix,  &projectionMatrix,  &viewMatrix);
	vMatrixMultiply(&viewProjectionMatrix0, &projectionMatrix0, &viewMatrix0);
}

void vEngine::SetProjectionMatrix(vMat4x4 *matrix, vMat4x4 *matrix0)
{
	projectionMatrix = (*matrix);

	if (matrix0)
	{
		projectionMatrix0 = (*matrix0);
	}
	else
	{
		projectionMatrix0 = (*matrix);
	}

	vMatrixMultiply(&viewProjectionMatrix,  &projectionMatrix,  &viewMatrix);
	vMatrixMultiply(&viewProjectionMatrix0, &projectionMatrix0, &viewMatrix0);
}

void vEngine::SetMaterial(float emissive, float specular, float specPow)
{
	surfaceMaterial = (int(emissive * 32.0f) << 27) | (int(specular * 64.0f) << 21) | (int(specPow * 32.0f) << 16);
}

void vEngine::SetSkin(Skin *skin)
{
	surfaceSkin = skin;
}

int vEngine::CoreBeginScene(RenderTarget *target)
{
	memcpy((void*)sceneState->renderTarget, (void*)target->coreSurfaceLevels, sizeof(CoreRenderSurface) * VTRACE_NUM_RECONSTRUCTION_LAYERS);

	void *_renderTarget = (void*)sceneState->renderTarget;
	cudaMemcpyToSymbolAsync(dSceneState.renderTarget, _renderTarget, sizeof(CoreRenderSurface) * VTRACE_NUM_RECONSTRUCTION_LAYERS, 0, cudaMemcpyHostToDevice, deviceStream);

	for (int i = 0; i < VTRACE_NUM_RECONSTRUCTION_LAYERS; i++)
	{
		dim3 dimBlock(sceneState->renderTarget[i].width >> 7, 1, 1);
		dim3 dimGrid(128, 1, 1);

		clearSurfaceKernel<<<dimGrid, dimBlock, 0, deviceStream>>>(i, (sceneState->renderTarget[i].width * sceneState->renderTarget[i].height) - 1);
	}

	numRenderSurfaces = 0;

	for (int i = 0; i < 5; i++)
	{
		numLayerSurfaces[i] = 0;
	}

	numRenderBuckets = 0;
	numCompactionIndices = 0;

	return 0;
}

int vEngine::CoreEndScene()
{
	if (numRenderBuckets > 0)
	{
		CoreFlushBatch();
	}

	dim3 dimBlock(4, 4, 8);

	for (int i = 0; i < VTRACE_NUM_RECONSTRUCTION_LAYERS; i++)
	{
		dim3 dimGrid(sceneState->renderTarget[i].width >> 5, sceneState->renderTarget[i].height >> 2, 1);

		unswizzleSurfaceKernel<<<dimGrid, dimBlock, 0, deviceStream>>>(i);
	}

	cudaThreadSynchronize();

	return 0;
}

int vEngine::CoreInit()
{
	vMatrixIdentity(&viewMatrix);       vMatrixIdentity(&viewMatrix0);
	vMatrixIdentity(&projectionMatrix); vMatrixIdentity(&projectionMatrix0);

	vLatticeIdentity(&surfaceLattice);
	vLatticeIdentity(&surfaceLattice0);

	vLatticeIdentity(&identityLattice);

	cudaMallocHost((void**)&sceneState, sizeof(DeviceSceneState));

	cudaStreamCreate(&deviceStream);

	cudaMallocHost((void**)&surfaceLattices,  sizeof(vLattice) * VTRACE_MAX_BATCH_SURFACES);
	cudaMallocHost((void**)&surfaceLattices0, sizeof(vLattice) * VTRACE_MAX_BATCH_SURFACES);

    dSurfaceLatticesTex.addressMode[0] = cudaAddressModeClamp;
    dSurfaceLatticesTex.addressMode[1] = cudaAddressModeClamp;
    dSurfaceLatticesTex.filterMode = cudaFilterModeLinear;
    dSurfaceLatticesTex.normalized = false;

    dSurfaceLattices0Tex.addressMode[0] = cudaAddressModeClamp;
    dSurfaceLattices0Tex.addressMode[1] = cudaAddressModeClamp;
    dSurfaceLattices0Tex.filterMode = cudaFilterModeLinear;
    dSurfaceLattices0Tex.normalized = false;

	cudaChannelFormatDesc latticePointDesc = cudaCreateChannelDesc<float4>();

	cudaMalloc3DArray(&surfaceLatticesArray,  &latticePointDesc, make_cudaExtent(2, 2, 2 * VTRACE_MAX_BATCH_SURFACES));
	cudaMalloc3DArray(&surfaceLattices0Array, &latticePointDesc, make_cudaExtent(2, 2, 2 * VTRACE_MAX_BATCH_SURFACES));

	cudaBindTextureToArray(&dSurfaceLatticesTex,  surfaceLatticesArray,  &latticePointDesc);
	cudaBindTextureToArray(&dSurfaceLattices0Tex, surfaceLattices0Array, &latticePointDesc);

    dRenderBucketsTex.addressMode[0] = cudaAddressModeClamp;
    dRenderBucketsTex.addressMode[1] = cudaAddressModeClamp;
    dRenderBucketsTex.filterMode = cudaFilterModePoint;
    dRenderBucketsTex.normalized = false;

	cudaMalloc((void**)&sceneState->renderBucketsIn,  VTRACE_MAX_BATCH_BUCKETS * sizeof(unsigned int));
	cudaMalloc((void**)&sceneState->renderBucketsOut, VTRACE_MAX_BATCH_BUCKETS * sizeof(unsigned int));

	cudaChannelFormatDesc renderBucketDesc = cudaCreateChannelDesc(32, 0, 0, 0, cudaChannelFormatKindUnsigned);
	cudaBindTexture(NULL, &dRenderBucketsTex, sceneState->renderBucketsOut, &renderBucketDesc, VTRACE_MAX_BATCH_BUCKETS * sizeof(unsigned int));

	cudaMalloc((void**)&sceneState->compactionIndicesIn,  VTRACE_MAX_COMPACTION_INDICES * sizeof(unsigned int));
	cudaMalloc((void**)&sceneState->compactionIndicesOut, VTRACE_MAX_COMPACTION_INDICES * sizeof(unsigned int));

	void *_sceneState = (void*)sceneState;
	cudaMemcpyToSymbolAsync(dSceneState, _sceneState, sizeof(DeviceSceneState), 0, cudaMemcpyHostToDevice, deviceStream);

	numRenderSurfaces = 0;

	for (int i = 0; i < 5; i++)
	{
		numLayerSurfaces[i] = 0;
	}

	numRenderBuckets = 0;
	numCompactionIndices = 0;

	return 0;
}

int vEngine::CoreShutdown()
{
	cudaThreadSynchronize();

	cudaFreeHost(sceneState);

	cudaFreeHost(surfaceLattices);
	cudaFreeHost(surfaceLattices0);

	cudaFreeArray(surfaceLatticesArray);
	cudaFreeArray(surfaceLattices0Array);

	cudaFree(sceneState->renderBucketsIn);
	cudaFree(sceneState->renderBucketsOut);

	cudaFree(sceneState->compactionIndicesIn);
	cudaFree(sceneState->compactionIndicesOut);
	
	cudaStreamDestroy(deviceStream);

	return 0;
}

int vEngine::CoreCreateRenderSurface(CoreRenderSurface *renderSurface)
{
	cudaMalloc((void**)&renderSurface->s_fBuffer,  renderSurface->width * renderSurface->height * sizeof(unsigned int));
	cudaMalloc((void**)&renderSurface->s_vBuffer,  renderSurface->width * renderSurface->height * sizeof(unsigned int));
	cudaMalloc((void**)&renderSurface->s_zBuffer,  renderSurface->width * renderSurface->height * sizeof(unsigned int));

	return 0;
}

int vEngine::CoreFreeRenderSurface(CoreRenderSurface *renderSurface)
{
	cudaFree(renderSurface->s_fBuffer);
	cudaFree(renderSurface->s_vBuffer);
	cudaFree(renderSurface->s_zBuffer);	

	return 0;
}

int vEngine::LoadSurface(Surface *surface, const char *fileName)
{
	memset((void*)surface, 0, sizeof(Surface));

	FILE *surfaceFile = fopen(fileName, "rb");

	if (!surfaceFile)
	{
		printf("Unable to open surface file : %s\n", fileName);

		return -1;
	}

	SurfaceHeader header;

	fread(&header, sizeof(SurfaceHeader), 1, surfaceFile);

	if (header.magic != 0x03460566)
	{
		printf("Error loading surface file '%s', not a valid surface file.\n", fileName);

		return -1;
	}
	
	int numLayerElements = 4;
	int numTotalElements = 0;

	for (int l = 0; l < header.mapLevels; l++)
	{
		numLayerElements *= 4;
		numTotalElements += numLayerElements;
	}

	surface->mapLevels  = header.mapLevels;
	surface->sizeOfData = numTotalElements;

	char *surfaceData = new char[numTotalElements * sizeof(unsigned int)];	//todo: support loading new normal layers and do asynchronous loading
	fread(surfaceData, 1, numTotalElements * sizeof(unsigned int), surfaceFile);

	cudaMalloc((void**)&surface->surfaceData, numTotalElements * sizeof(unsigned int));
	cudaMemcpy((void*)surface->surfaceData, (void*)surfaceData, numTotalElements * sizeof(unsigned int), cudaMemcpyHostToDevice);

	delete [] surfaceData;

	surface->bucketLevels = header.mapLevels > 6 ? 6 : header.mapLevels;

	numLayerElements = 4;
	numTotalElements = 0;

	for (int l = 0; l < surface->bucketLevels; l++)
	{
		numLayerElements *= 4;
		numTotalElements += numLayerElements;
	}

	char *normalsData = new char[numTotalElements * sizeof(unsigned int)];	//todo: support loading new normal layers and do asynchronous loading
	fread(normalsData, 1, numTotalElements * sizeof(unsigned int), surfaceFile);

	cudaMalloc((void**)&surface->normalsData, numTotalElements * sizeof(unsigned int));
	cudaMemcpy((void*)surface->normalsData, (void*)normalsData, numTotalElements * sizeof(unsigned int), cudaMemcpyHostToDevice);

	delete [] normalsData;

	return 0;
}

void vEngine::FreeSurface(Surface *surface)
{
	cudaFree((void*)surface->surfaceData);
	cudaFree((void*)surface->normalsData);
}

int vEngine::LoadSkin(Skin *skin, const char *fileName)
{
	memset((void*)skin, 0, sizeof(Skin));

	FILE *skinFile = fopen(fileName, "rb");

	if (!skinFile)
	{
		printf("Unable to open skin file : %s\n", fileName);

		return -1;
	}

	SurfaceHeader header;

	fread(&header, sizeof(SurfaceHeader), 1, skinFile);

	if (header.magic != 0x03460566)
	{
		printf("Error loading skin file '%s', not a valid skin file.\n", fileName);

		return -1;
	}
	
	int numLayerElements = 4;
	int numTotalElements = 0;

	for (int l = 0; l < header.mapLevels; l++)
	{
		numLayerElements *= 4;
		numTotalElements += numLayerElements;
	}

	skin->mapLevels  = header.mapLevels;
	skin->sizeOfData = numTotalElements;

	unsigned short *skinData = new unsigned short[numTotalElements];	//todo: support 32bit skin formats that include material info and do asynchronous loading

	fread(skinData, 1, numTotalElements * sizeof(unsigned short), skinFile);

	cudaMalloc((void**)&skin->skinData, numTotalElements * sizeof(unsigned short));
	cudaMemcpy((void*)skin->skinData, (void*)skinData, numTotalElements * sizeof(unsigned short), cudaMemcpyHostToDevice);

	delete [] skinData;

	return 0;
}

void vEngine::FreeSkin(Skin *skin)
{
	cudaFree((void*)skin->skinData);
}