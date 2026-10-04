#include "cuda_renderer.h"

#include <cuda_runtime.h>
#include <string>
#include <stdexcept>

__global__ void testKernel(std::uint8_t* output, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    float value = width == 1 ? 255.f : float(x) / float(width - 1) * 255.f;

    *(output + y * width + x) = value;
}

#define CHECK_CUDA(call) checkCuda(call, #call, __FILE__, __LINE__)
void checkCuda(cudaError_t result, const char* operation, const char* file, int line) {
    if (result != cudaSuccess) {
        std::string errorMsg = "CUDA Error at " + std::string(file) + ":" + std::to_string(line) + "\n"
                             + "  Operation: " + operation + "\n"
                             + "  Error Code: " + std::to_string(result) + "\n"
                             + "  Error String: " + cudaGetErrorString(result);
        throw std::runtime_error(errorMsg);
    }
}

Image renderCudaTest(int width, int height) {
    Image image(width, height);
    std::uint8_t* deviceOutput = nullptr;
    const std::size_t byteCount = static_cast<std::size_t>(width) * height * sizeof(std::uint8_t);
    CHECK_CUDA(cudaMalloc(&deviceOutput, byteCount));

    const dim3 blockSize(16, 16); // 256 threads
    const dim3 gridSize((width + blockSize.x - 1) / blockSize.x, (height + blockSize.y - 1) / blockSize.y);

    testKernel<<<gridSize, blockSize>>>(deviceOutput, width, height);
    CHECK_CUDA(cudaGetLastError());

    CHECK_CUDA(cudaMemcpy(image.data(), deviceOutput, byteCount, cudaMemcpyDeviceToHost));
    CHECK_CUDA(cudaFree(deviceOutput));
    
    return image;
}