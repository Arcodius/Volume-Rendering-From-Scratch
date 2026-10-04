#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "volume.h"

class Image {
public:
	using Pixel = std::uint8_t;

	Image() = default;
	Image(int width, int height, Pixel fill = 0);

	int width() const noexcept;
	int height() const noexcept;
	bool empty() const noexcept;
	const std::vector<Pixel>& pixels() const noexcept;

	Pixel* data() noexcept;
	const Pixel* data() const noexcept;

	Pixel& operator()(int x, int y);
	const Pixel& operator()(int x, int y) const;

	bool load(const std::string& filename);
	bool savePNG(const std::string& filename) const;

	static Image fromVolumeSlice(const VolumeGrid3D& volume, int z);
	static bool saveVolumeSlice(const VolumeGrid3D& volume, int z, const std::string& filename);

private:
	std::size_t index(int x, int y) const;

	int width_ = 0;
	int height_ = 0;
	std::vector<Pixel> pixels_;
};