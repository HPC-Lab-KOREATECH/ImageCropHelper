#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <vector>
#include <cmath>
#include <filesystem>
#include <windef.h>
#include <wingdi.h>
#include "opencv.hpp"
#include "imgui.h"


#ifndef GetRValue	
#define GetRValue(rgb)	( (BYTE)(rgb) )
#endif
#ifndef GetGValue
#define GetGValue(rgb)	( (BYTE)(((WORD)(rgb))>>8) )
#endif
#ifndef GetBValue
#define GetBValue(rgb)	( (BYTE)((rgb)>>16) )
#endif

void saveImage(cv::Mat mat, std::wstring dir, cv::String file) {
    if (mat.empty()) {
        return; // Avoid cv::cvtColor assertion on empty images
    }
    char charDir[MAX_PATH];
    size_t size = MAX_PATH;
    wcstombs_s(&size, charDir, dir.c_str(), MAX_PATH);
    cv::String path = cv::String(charDir);

    // Matrices we draw on are RGB in-memory; convert back to BGR for imwrite
    if (mat.channels() == 3)
        cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
    cv::imwrite(path + file, mat);
}

void initSaveFolder(std::wstring dir) {
	char charDir[MAX_PATH];
	size_t size = MAX_PATH;
	wcstombs_s(&size, charDir, dir.c_str(), MAX_PATH);
	cv::String path = cv::String(charDir);

	if (!std::filesystem::exists(path))
		std::filesystem::create_directory(path);
}

void drawPixsel(cv::Vec3b& pixel, COLORREF color) {
	pixel[2] = GetBValue(color);
	pixel[1] = GetGValue(color);
	pixel[0] = GetRValue(color);
}

void drawRect(cv::Mat* mat, COLORREF color, int width, int height, int top, int bottom, int left, int right) {
	for (int y = 0; y <= height; y++) {
		cv::Vec3b& matPixelLeft = mat->at<cv::Vec3b>(top + y, left);
		cv::Vec3b& matPixelRight = mat->at<cv::Vec3b>(top + y, right);
		drawPixsel(matPixelLeft, color);
		drawPixsel(matPixelRight, color);
	}

	for (int x = 0; x <= width; x++) {
		cv::Vec3b& matPixelTop = mat->at<cv::Vec3b>(top, left + x);
		cv::Vec3b& matPixelBottom = mat->at<cv::Vec3b>(bottom, left + x);
		drawPixsel(matPixelTop, color);
		drawPixsel(matPixelBottom, color);
	}
}

void drawRect(cv::Mat* mat, ImVec4 box, ImVec4 colorVec, std::pair<int, int> imageSize, int thickness) {
	COLORREF color = RGB(colorVec.x * 255, colorVec.y * 255, colorVec.z * 255);
	int width = std::ceil(imageSize.first * (box.z - box.x));
	int height = std::ceil(imageSize.second * (box.w - box.y));
	int top = std::ceil(imageSize.second * box.y);
	int bottom = top + height;
	int left = std::ceil(imageSize.first * box.x);
	int right = left + width;

	for (int i = 0;i < thickness;i++)
		drawRect(mat, color, width - i * 2, height - i * 2, top + i, bottom - i, left + i, right - i);
}


cv::Mat cropMat(cv::Mat* mat, ImVec4 box, std::pair<int, int> imageSize) {
    int width = std::ceil(imageSize.first * (box.z - box.x));
    int height = std::ceil(imageSize.second * (box.w - box.y));
    int top = std::ceil(imageSize.second * box.y);
    int left = std::ceil(imageSize.first * box.x);
    cv::Rect roi(left, top, width, height);
    return (*mat)(roi).clone();
}

inline int scaledThickness(int baseThickness, int targetHeight, int refHeight) {
    if (baseThickness <= 0) return 0;
    if (refHeight <= 0) return baseThickness;
    int t = (int)std::round((double)baseThickness * (double)targetHeight / (double)refHeight);
    return std::max(1, t);
}

// Compose a new image by placing the original on the left and
// appending cropped regions on the right as 1/2-size tiles,
// stacked 2 per column (top/bottom), moving to the next column as needed.
// - Each tile size: (base_width/2, base_height/2)
// - Output size: (base_height, base_width + columns * (base_width/2))
inline cv::Mat composeRightGrid(cv::Mat* base,
    const std::vector<std::pair<ImVec4, ImVec4>>& entries, // {box, color}
    const std::pair<int, int>& imageSize,
    BoxMode mode,
    int baseThickness,
    int tileBaseThickness,
    int refHeight)
{
    const int base_w = imageSize.first;
    const int base_h = imageSize.second;
    if (base_w <= 0 || base_h <= 0 || entries.empty())
        return base->clone();

    const int tile_h = std::max(1, base_h / 2);
    const int n = static_cast<int>(entries.size());

    // Pre-compute each tile width (preserve aspect per mode)
    std::vector<int> tile_w(n, 0);
    for (int i = 0; i < n; ++i) {
        const ImVec4& box = entries[i].first;
        int cw = std::max(1, (int)std::ceil(imageSize.first * (box.z - box.x)));
        int ch = std::max(1, (int)std::ceil(imageSize.second * (box.w - box.y)));
        int tw = tile_h; // default square
        if (mode == BoxMode::Square) {
            tw = tile_h;
        } else {
            tw = (int)std::round((double)tile_h * (double)cw / (double)ch);
        }
        tile_w[i] = std::max(1, tw);
    }

    // Compute per-column widths (2 rows per column)
    std::vector<int> col_w;
    for (int i = 0; i < n; i += 2) {
        int w0 = tile_w[i];
        int w1 = (i + 1 < n) ? tile_w[i + 1] : 0;
        col_w.push_back(std::max(w0, w1));
    }

    int out_w = base_w;
    for (int w : col_w) out_w += w;
    const int out_h = base_h;

    cv::Mat out(out_h, out_w, base->type(), cv::Scalar(0, 0, 0));
    // Copy original image on the left
    base->copyTo(out(cv::Rect(0, 0, base_w, base_h)));
    // Draw the boxes on the left original area as well (scaled thickness)
    int leftTh = scaledThickness(baseThickness, base_h, refHeight);
    for (const auto& e : entries) {
        drawRect(&out, e.first, e.second, imageSize, std::max(1, leftTh));
    }

    // Paste crops on the right grid, two per column, left-aligned with padding
    int xoff = base_w;
    for (int i = 0, col = 0; i < n; ++col) {
        int cw = col_w[col];
        for (int row = 0; row < 2 && i < n; ++row, ++i) {
            const ImVec4& box = entries[i].first;
            const ImVec4& colv = entries[i].second;

            int tw = tile_w[i];
            int x = xoff; // left-aligned
            int y = row * tile_h;

            // Resize preserving aspect
            cv::Mat crop = cropMat(base, box, imageSize);
            if (crop.empty()) continue;
            cv::Mat resized;
            cv::resize(crop, resized, cv::Size(tw, tile_h), 0, 0, cv::INTER_AREA);
            resized.copyTo(out(cv::Rect(x, y, tw, tile_h)));

            // Draw border with scaled thickness
            COLORREF color = RGB((int)(colv.x * 255), (int)(colv.y * 255), (int)(colv.z * 255));
            int w = tw - 1, h = tile_h - 1;
            int tscaled = std::max(0, scaledThickness(tileBaseThickness, tile_h, refHeight));
            for (int t = 0; t < tscaled; ++t) {
                int rw = w - t * 2, rh = h - t * 2;
                if (rw <= 0 || rh <= 0) break;
                drawRect(&out, color, rw, rh, y + t, y + t + rh, x + t, x + t + rw);
            }
        }
        xoff += cw;
    }

    return out;
}
