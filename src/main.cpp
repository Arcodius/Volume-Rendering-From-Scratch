#include "render.h"
#include "volume.h"

#include <iostream>

int main() {
    constexpr int volumeResolution = 128;
    constexpr int imageWidth = 512;
    constexpr int imageHeight = 512;
    std::cout << "正在生成 " << volumeResolution << "x" << volumeResolution << "x"
              << volumeResolution << " 的体积云数据..." << std::endl;

    VolumeGrid3D cloud = generateCloudVolume(volumeResolution);

    // 统计非空体素
    int activeVoxels = 0;
    for (float val : cloud.data) {
        if (val > 0.01f) activeVoxels++;
    }

    std::cout << "生成完成！" << std::endl;
    std::cout << "总体素数: " << cloud.data.size() << " (内存占用: "
              << (cloud.data.size() * sizeof(float)) / (1024 * 1024) << " MB)" << std::endl;
    std::cout << "有效云密度体素数: " << activeVoxels << std::endl;

    const Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f),
                        glm::vec3(0.0f, 1.0f, 0.0f), 45.0f);
    Image image = renderVolume(cloud, camera, imageWidth, imageHeight);
    if (!image.savePNG("cloud.png")) {
        std::cerr << "无法保存渲染结果 cloud.png" << std::endl;
        return 1;
    }

    std::cout << "渲染完成: cloud.png (" << imageWidth << "x" << imageHeight << ")" << std::endl;

    return 0;
}