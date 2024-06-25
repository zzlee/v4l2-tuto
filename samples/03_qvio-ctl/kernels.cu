#include <stdio.h>

__global__ void kernel_Copy_8u_C1R(uchar1* pSrc, int srcStep, uchar1* pDst, int dstStep, int nWidth, int nHeight) {
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if(x < nWidth && y < nHeight) {
		pDst[y * dstStep + x] = pSrc[y * srcStep + x];
	}
}
