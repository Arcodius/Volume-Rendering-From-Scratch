#include "image.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

Image::Image(int width, int height, Pixel fill)
    : width_(width), height_(height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Image dimensions must be positive");
    }

    const auto widthSize = static_cast<std::size_t>(width);
    const auto heightSize = static_cast<std::size_t>(height);
    if (widthSize > std::numeric_limits<std::size_t>::max() / heightSize) {
        throw std::length_error("Image dimensions are too large");
    }

    pixels_.assign(widthSize * heightSize, fill);
}

int Image::width() const noexcept {
    return width_;
}

int Image::height() const noexcept {
    return height_;
}

bool Image::empty() const noexcept {
    return pixels_.empty();
}

const std::vector<Image::Pixel>& Image::pixels() const noexcept {
    return pixels_;
}

Image::Pixel& Image::operator()(int x, int y) {
    return pixels_.at(index(x, y));
}

const Image::Pixel& Image::operator()(int x, int y) const {
    return pixels_.at(index(x, y));
}

std::size_t Image::index(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        throw std::out_of_range("Image coordinates are out of range");
    }
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x);
}

bool Image::load(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }

    int loadedWidth = 0;
    int loadedHeight = 0;
    int sourceChannels = 0;
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> decoded(
        stbi_load(filename.c_str(), &loadedWidth, &loadedHeight, &sourceChannels, 1),
        stbi_image_free);
    if (!decoded || loadedWidth <= 0 || loadedHeight <= 0) {
        return false;
    }

    Image loaded(loadedWidth, loadedHeight);
    loaded.pixels_.assign(decoded.get(), decoded.get() + loaded.pixels_.size());
    *this = std::move(loaded);
    return true;
}

bool Image::savePNG(const std::string& filename) const {
    if (filename.empty() || empty()) {
        return false;
    }

    return stbi_write_png(filename.c_str(), width_, height_, 1, pixels_.data(), width_) != 0;
}

Image Image::fromVolumeSlice(const VolumeGrid3D& volume, int z) {
    const int resolution = volume.resolution;
    if (resolution <= 0 || z < 0 || z >= resolution) {
        throw std::out_of_range("Volume slice is out of range");
    }

    const auto resolutionSize = static_cast<std::size_t>(resolution);
    if (resolutionSize > std::numeric_limits<std::size_t>::max() / resolutionSize ||
        resolutionSize * resolutionSize > std::numeric_limits<std::size_t>::max() / resolutionSize ||
        volume.data.size() < resolutionSize * resolutionSize * resolutionSize) {
        throw std::invalid_argument("Volume data does not match its resolution");
    }

    Image image(resolution, resolution);
    for (int y = 0; y < resolution; ++y) {
        for (int x = 0; x < resolution; ++x) {
            const float density = volume.get(x, y, z);
            const float normalized = std::isfinite(density) ? std::clamp(density, 0.0f, 1.0f) : 0.0f;
            image(x, y) = static_cast<Pixel>(std::lround(normalized * 255.0f));
        }
    }
    return image;
}

bool Image::saveVolumeSlice(const VolumeGrid3D& volume, int z, const std::string& filename) {
    return fromVolumeSlice(volume, z).savePNG(filename);
}