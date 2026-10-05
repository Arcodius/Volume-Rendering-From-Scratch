#pragma once

#include "image.h"

Image renderCudaTest(int width, int height);
Image renderCudaSlice(int width, int height, const VolumeGrid3D& cloud);