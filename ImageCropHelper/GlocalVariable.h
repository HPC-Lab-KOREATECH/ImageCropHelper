#pragma once
#include <Windows.h>
#include <map>
#include <string>
#include <stack>
#include <algorithm>
#include "imgui.h"
#include "opencv.hpp"

struct WGL_WindowData { HDC hDC; };

static HGLRC            g_hRC;
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;
static std::map<std::string, unsigned int> imagesGL;
static std::map<std::string, cv::Mat> imagesCV;
// Keep originals to rebuild flipped variants on toggle
static std::map<std::string, cv::Mat> imagesCVOriginal;
static std::map<std::string, std::pair<int, int>> imageSize;
static std::map<std::string, bool> onWindow;
static std::vector<std::pair<ImVec4, ImVec4>> boxList;

static WNDCLASSEXW wc;
static HWND hwnd;
static int boxSize = 1;

//static bool isFree = true;
//static bool isSquare = false; 
enum class BoxMode { Free, KeepAspect, Square };
BoxMode g_boxMode = BoxMode::Free;

// Square base-cropping mode (fit to shorter side, follow mouse, click to crop base)
static bool g_squareBaseMode = false;
static ImVec4 g_squarePreviewBox = ImVec4(0, 0, 0, 0); // normalized [0..1] in current view
// Persisted temporary square ROI (applied only on Save)
static bool g_squareBaseROIEnabled = false;
static ImVec4 g_squareBaseROI = ImVec4(0, 0, 0, 0);
// Remember previous BoxMode while ROI placement is active (to auto-switch to Square and restore)
static bool g_roiPrevBoxModeSaved = false;
static BoxMode g_roiPrevBoxMode = BoxMode::Free;

static std::string textureName = "-1";
static ImVec4 color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
static ImVec4 area;
static ImVec4 currBox = {0,0,0,0};
static float scale = 1.f;
static ImVec2 selectWindowSize;
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
static bool done = false;
// Save options
static bool g_saveComposeRight = false; // Save full image with right-side 1/2 grid of crops
static int g_tileBorderSize = 2;        // Border thickness for tiles/crops in pixels
static bool g_saveCropPlain = true;      // Save cropped images without border
static bool g_saveCropBordered = false;  // Also save cropped images with border
static int g_refHeight = 1000;          // Reference height for thickness scaling

// Global flip state (applies to all images in memory)
static bool g_flipHorizontal = false;
static bool g_flipVertical = false;

