#include "cuda_renderer.h"

#include <cuda_runtime.h>
#include <limits>
#include <string>
#include <stdexcept>

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

__global__ void testKernel(uint8_t* output, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    float value = width == 1 ? 255.f : float(x) / float(width - 1) * 255.f;

    *(output + y * width + x) = value;
}

__device__ size_t volumeIndex(int x, int y, int z, int resolution) {
    return (static_cast<size_t>(z) * resolution + y) * resolution + x;
}

__global__ void sliceKernel(const float* input, uint8_t* output, int width, int height, int resolution, int z) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    int vx = static_cast<int>(floorf(static_cast<float>(x) / width * resolution));
    int vy = static_cast<int>(floorf(static_cast<float>(y) / height * resolution));

    vx = min(max(vx, 0), resolution - 1);
    vy = min(max(vy, 0), resolution - 1);

    size_t id = volumeIndex(vx, vy, z, resolution);

    float val = input[id];
    const size_t outputIndex = static_cast<size_t>(y) * width + x;
    output[outputIndex] = static_cast<uint8_t>(lroundf(__saturatef(val) * 255.f));
}

__global__ void renderKernel(const float* input, uint8_t* output, int width, int height, int resolution) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    int vx = static_cast<int>(floorf(static_cast<float>(x) / width * resolution));
    int vy = static_cast<int>(floorf(static_cast<float>(y) / height * resolution));

    vx = min(max(vx, 0), resolution - 1);
    vy = min(max(vy, 0), resolution - 1);

    // input ray

    // intersect and ray march
    // size_t id = volumeIndex(vx, vy, z, resolution);
}

Image renderCudaTest(int width, int height) {
    Image image(width, height);
    uint8_t* deviceOutput = nullptr;
    const size_t byteCount = static_cast<size_t>(width) * height * sizeof(uint8_t);
    CHECK_CUDA(cudaMalloc(&deviceOutput, byteCount));

    const dim3 blockSize(16, 16); // 256 threads
    const dim3 gridSize((width + blockSize.x - 1) / blockSize.x, (height + blockSize.y - 1) / blockSize.y);

    testKernel<<<gridSize, blockSize>>>(deviceOutput, width, height);
    CHECK_CUDA(cudaGetLastError());

    CHECK_CUDA(cudaMemcpy(image.data(), deviceOutput, byteCount, cudaMemcpyDeviceToHost));
    CHECK_CUDA(cudaFree(deviceOutput));

    return image;
}

Image renderCudaSlice(int width, int height, const VolumeGrid3D& cloud) {
    Image image(width, height);

    if (cloud.resolution <= 0) {
        throw std::invalid_argument("Volume resolution must be positive");
    }
    const size_t side = static_cast<size_t>(cloud.resolution);
    if (side > std::numeric_limits<size_t>::max() / side ||
        side * side > std::numeric_limits<size_t>::max() / side) {
        throw std::length_error("Volume resolution is too large");
    }
    const size_t voxelCount = side * side * side;
    if (cloud.data.size() < voxelCount) {
        throw std::invalid_argument("Volume data does not match its resolution");
    }
    if (voxelCount > std::numeric_limits<size_t>::max() / sizeof(float)) {
        throw std::length_error("Volume data is too large");
    }

    const size_t volumeByteCount = voxelCount * sizeof(float);
    const size_t imageByteCount = image.pixels().size() * sizeof(Image::Pixel);
    float* deviceInput = nullptr;
    uint8_t* output = nullptr;

    CHECK_CUDA(cudaMalloc(&deviceInput, volumeByteCount));
    CHECK_CUDA(cudaMemcpy(deviceInput, cloud.data.data(), volumeByteCount, cudaMemcpyHostToDevice));
    CHECK_CUDA(cudaMalloc(&output, imageByteCount));

    const dim3 blockSize(16, 16);
    const dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                        (height + blockSize.y - 1) / blockSize.y);

    sliceKernel<<<gridSize, blockSize>>>(deviceInput, output, width, height, cloud.resolution,
                                            cloud.resolution / 2);
    CHECK_CUDA(cudaGetLastError());
    CHECK_CUDA(cudaMemcpy(image.data(), output, imageByteCount, cudaMemcpyDeviceToHost));
    CHECK_CUDA(cudaFree(output));
    output = nullptr;
    CHECK_CUDA(cudaFree(deviceInput));
    deviceInput = nullptr;

    return image;
}