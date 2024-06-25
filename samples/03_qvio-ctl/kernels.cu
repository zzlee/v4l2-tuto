#include <stdio.h>

__global__ void kernel_YCbCr422_8u_C2P3R(uchar1* pSrc, int srcStep,
	uchar1* pDst[3], int dstStep[3], int nWidth, int nHeight) {
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if(x < nWidth && y < nHeight) {
		int nSrcIdx = y * srcStep + x * 4;
		int nDst0Idx = y * dstStep[0] + x * 2;
		int nDst1Idx = y * dstStep[1] + x;
		int nDst2Idx = y * dstStep[2] + x;

		pDst[0][nDst0Idx] = pSrc[nSrcIdx];
		pDst[0][nDst0Idx + 1] = pSrc[nSrcIdx + 2];
		pDst[1][nDst1Idx] = pSrc[nSrcIdx + 1];
		pDst[2][nDst2Idx] = pSrc[nSrcIdx + 3];
	}
}

extern cudaError_t zppiYCbCr422_8u_C2P3R(
	uchar1* pSrc, int srcStep, uchar1* pDst[3], int dstStep[3], int nWidth, int nHeight) {
	static int BLOCK_W = 16;
	static int BLOCK_H = 16;

	nWidth /= 2;

	dim3 grid((nWidth + BLOCK_W-1) / BLOCK_W, (nHeight + BLOCK_H-1) / BLOCK_H, 1);
	dim3 block(BLOCK_W, BLOCK_H, 1);

	kernel_YCbCr422_8u_C2P3R<<<grid, block>>>(
		pSrc, srcStep,
		pDst, dstStep,
		nWidth, nHeight);

	return cudaDeviceSynchronize();
}
