#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include <glm/glm.hpp>


class VolumeGrid3D {
public:
    int resolution;
    std::vector<float> data;

    explicit VolumeGrid3D(int resolution);

    void set(int x, int y, int z, float value);
    float get(int x, int y, int z) const;
    bool load(const std::string& filename);
    bool save(const std::string& filename) const;

private:
    std::size_t index(int x, int y, int z) const;
};

VolumeGrid3D generateCloudVolume(int resolution = 128);