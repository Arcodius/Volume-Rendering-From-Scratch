#include "render.h"
#include "volume.h"
#include "cuda_renderer.h"

#include <exception>
#include <iostream>

int main() {
    constexpr int volumeResolution = 128;
    constexpr int imageWidth = 512;
    constexpr int imageHeight = 512;
    
    std::cout << "Generating " << volumeResolution << "x" << volumeResolution << "x"
              << volumeResolution << " volumetric cloud data..." << std::endl;

    VolumeGrid3D cloud = generateCloudVolume(volumeResolution);

    int activeVoxels = 0;
    for (float val : cloud.data) {
        if (val > 0.01f) activeVoxels++;
    }

    std::cout << "Voxel: " << cloud.data.size() << " (Memory: "
              << (cloud.data.size() * sizeof(float)) / (1024 * 1024) << " MB)" << std::endl;
    std::cout << "Valid voxels: " << activeVoxels << std::endl;

    // Slicing
    Image img;
    try {
        img = renderCudaSlice(imageWidth, imageHeight, cloud);
    } catch (const std::exception& error) {
        std::cerr << "CUDA test render failed: " << error.what() << std::endl;
        return 1;
    }
    if (!img.savePNG("cuda_test.png")) {
        std::cerr << "Unable to save render result: cuda_test.png" << std::endl;
        return 1;
    }

    // const Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f),
    //                     glm::vec3(0.0f, 1.0f, 0.0f), 45.0f);
    // Image image = renderVolume(cloud, camera, imageWidth, imageHeight);
    // if (!image.savePNG("cloud.png")) {
    //     std::cerr << "Unable to save render result: cloud.png" << std::endl;
    //     return 1;
    // }

    // std::cout << "Render complete: cloud.png (" << imageWidth << "x" << imageHeight << ")" << std::endl;

    return 0;
}