#pragma once
#pragma warning(disable : 26812 26495)

// Standard Library
#include <vector>
#include <array>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <set>
#include <algorithm>
#include <array>
#include <fstream>

// Project Data Structures
#include "DataStructures.h"

// GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 

// STB IMAGE
#include <stb_image.h>

// DEAR IMGUI
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"
#include "imgui.h"