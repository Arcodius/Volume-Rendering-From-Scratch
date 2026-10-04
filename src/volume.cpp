#include "volume.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

constexpr char volumeFileMagic[] = "VRGRID01";

std::size_t checkedVoxelCount(int resolution) {
    if (resolution <= 0) {
        throw std::invalid_argument("Volume resolution must be positive");
    }

    const auto side = static_cast<std::size_t>(resolution);
    if (side > std::numeric_limits<std::size_t>::max() / side) {
        throw std::length_error("Volume resolution is too large");
    }
    const auto sliceSize = side * side;
    if (sliceSize > std::numeric_limits<std::size_t>::max() / side) {
        throw std::length_error("Volume resolution is too large");
    }
    return sliceSize * side;
}

} // namespace

VolumeGrid3D::VolumeGrid3D(int resolution)
    : resolution(resolution), data(checkedVoxelCount(resolution), 0.0f) {}

std::size_t VolumeGrid3D::index(int x, int y, int z) const {
    if (x < 0 || x >= resolution || y < 0 || y >= resolution || z < 0 || z >= resolution) {
        throw std::out_of_range("Volume coordinates are out of range");
    }
    const auto side = static_cast<std::size_t>(resolution);
    return static_cast<std::size_t>(z) * side * side + static_cast<std::size_t>(y) * side +
           static_cast<std::size_t>(x);
}

void VolumeGrid3D::set(int x, int y, int z, float value) {
    data.at(index(x, y, z)) = value;
}

float VolumeGrid3D::get(int x, int y, int z) const {
    return data.at(index(x, y, z));
}

bool VolumeGrid3D::load(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }

    std::ifstream input(filename, std::ios::binary);
    if (!input) {
        return false;
    }

    char magic[sizeof(volumeFileMagic) - 1]{};
    std::uint32_t fileResolution = 0;
    input.read(magic, sizeof(magic));
    input.read(reinterpret_cast<char*>(&fileResolution), sizeof(fileResolution));
    if (!input || !std::equal(std::begin(magic), std::end(magic), volumeFileMagic) ||
        fileResolution == 0 || fileResolution > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    VolumeGrid3D loaded(static_cast<int>(fileResolution));
    const auto byteCount = loaded.data.size() * sizeof(float);
    if (byteCount > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
        return false;
    }
    input.read(reinterpret_cast<char*>(loaded.data.data()), static_cast<std::streamsize>(byteCount));
    if (!input || input.peek() != std::char_traits<char>::eof()) {
        return false;
    }

    *this = std::move(loaded);
    return true;
}

bool VolumeGrid3D::save(const std::string& filename) const {
    if (filename.empty() || resolution <= 0) {
        return false;
    }

    std::size_t expectedSize = 0;
    try {
        expectedSize = checkedVoxelCount(resolution);
    } catch (const std::exception&) {
        return false;
    }
    if (data.size() != expectedSize ||
        data.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()) / sizeof(float)) {
        return false;
    }
    const auto byteCount = data.size() * sizeof(float);

    std::ofstream output(filename, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }

    const auto fileResolution = static_cast<std::uint32_t>(resolution);
    output.write(volumeFileMagic, sizeof(volumeFileMagic) - 1);
    output.write(reinterpret_cast<const char*>(&fileResolution), sizeof(fileResolution));
    output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(byteCount));
    return static_cast<bool>(output);
}

// 3D Perlin Noise
namespace {

glm::vec3 hash33(glm::vec3 p) {
    p = glm::vec3(glm::dot(p, glm::vec3(127.1f, 311.7f, 74.7f)),
                  glm::dot(p, glm::vec3(269.5f, 183.3f, 246.1f)),
                  glm::dot(p, glm::vec3(113.5f, 271.9f, 124.6f)));
    return glm::fract(glm::sin(p) * 43758.5453123f);
}

float perlinNoise(const glm::vec3& p) {
    glm::vec3 i = glm::floor(p);
    glm::vec3 f = glm::fract(p);
    glm::vec3 u = f * f * (3.0f - 2.0f * f);

    return glm::mix(
        glm::mix(glm::mix(glm::dot(hash33(i + glm::vec3(0,0,0)), f - glm::vec3(0,0,0)),
                          glm::dot(hash33(i + glm::vec3(1,0,0)), f - glm::vec3(1,0,0)), u.x),
                 glm::mix(glm::dot(hash33(i + glm::vec3(0,1,0)), f - glm::vec3(0,1,0)),
                          glm::dot(hash33(i + glm::vec3(1,1,0)), f - glm::vec3(1,1,0)), u.x), u.y),
        glm::mix(glm::mix(glm::dot(hash33(i + glm::vec3(0,0,1)), f - glm::vec3(0,0,1)),
                          glm::dot(hash33(i + glm::vec3(1,0,1)), f - glm::vec3(1,0,1)), u.x),
                 glm::mix(glm::dot(hash33(i + glm::vec3(0,1,1)), f - glm::vec3(0,1,1)),
                          glm::dot(hash33(i + glm::vec3(1,1,1)), f - glm::vec3(1,1,1)), u.x), u.y), u.z);
}

// 3D Worley (Cellular) Noise - 返回到最近特征点的距离
float worleyNoise(const glm::vec3& p) {
    glm::vec3 i = glm::floor(p);
    glm::vec3 f = glm::fract(p);

    float minDist = 1.0f;
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                glm::vec3 neighbor = glm::vec3(x, y, z);
                glm::vec3 point = hash33(i + neighbor);
                glm::vec3 diff = neighbor + point - f;
                float dist = glm::length(diff);
                minDist = std::min(minDist, dist);
            }
        }
    }
    return minDist;
}

// -------------------------------------------------------------------
// 2. FBM (分形布朗运动) 叠加
// -------------------------------------------------------------------

float perlinFBM(glm::vec3 p, int octaves) {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;

    for (int i = 0; i < octaves; ++i) {
        value += amplitude * (perlinNoise(p * frequency) * 0.5f + 0.5f);
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    return value;
}

float worleyFBM(glm::vec3 p, int octaves) {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;

    for (int i = 0; i < octaves; ++i) {
        value += amplitude * worleyNoise(p * frequency);
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    return value;
}

} // namespace

// -------------------------------------------------------------------
// 3. 3D 网格生成器
// -------------------------------------------------------------------



VolumeGrid3D generateCloudVolume(int resolution) {
    VolumeGrid3D grid(resolution);

    #pragma omp parallel for collapse(3) // 若支持 OpenMP 可开启并行加速
    for (int z = 0; z < resolution; ++z) {
        for (int y = 0; y < resolution; ++y) {
            for (int x = 0; x < resolution; ++x) {
                // 将网格坐标归一化到 [-1, 1]
                glm::vec3 pos = (glm::vec3(x, y, z) / float(resolution)) * 2.0f - 1.0f;

                // 1. 球形边界约束 (SDF/Falloff)
                float dist = glm::length(pos);
                float sphereMask = glm::clamp(1.0f - dist / 1.4f, 0.0f, 1.0f);
                sphereMask = sphereMask * sphereMask * (3.0f - 2.0f * sphereMask); // Smoothstep

                if (sphereMask <= 0.0f) {
                    grid.set(x, y, z, 0.0f);
                    continue;
                }

                // 2. 基础形状：Perlin FBM
                glm::vec3 samplePos = pos * 3.0f; // 控制整体噪声频率
                float baseNoise = perlinFBM(samplePos, 4);

                // 3. 细部侵蚀：Worley FBM (反转 Worley 得到蜂窝团块感)
                float detailWorley = 1.0f - worleyFBM(samplePos * 2.0f, 3);

                // 4. Perlin-Worley 融合 (用 Worley 侵蚀 Perlin 的边缘)
                float cloudShape = baseNoise * sphereMask;
                float finalDensity = cloudShape - (1.0f - detailWorley) * 0.25f;

                // 5. 密度重塑 (阈值截断与对比度)
                float threshold = 0.15f;
                finalDensity = glm::clamp((finalDensity - threshold) / (1.0f - threshold), 0.0f, 1.0f);
                
                // 增加密实感 (Gamma 调节)
                finalDensity = std::pow(finalDensity, 1.5f);

                grid.set(x, y, z, finalDensity);
            }
        }
    }

    return grid;
}
