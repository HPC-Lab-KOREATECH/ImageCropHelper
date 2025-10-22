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

