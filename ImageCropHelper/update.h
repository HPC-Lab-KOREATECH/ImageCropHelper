#pragma once
#include "GlocalVariable.h"

//void updateMouse() {
//	ImGuiIO& io = ImGui::GetIO();
//	if (textureName == "-1")
//		return;
//	if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered())
//		return;
//	float mouseX = io.MousePos.x;
//	float mouseY = io.MousePos.y;
//
//	static bool isDown = false;
//	if (io.MouseDown[0] && !isDown) {
//
//		if (mouseX <= area.x || mouseX >= area.x + area.z)
//			return;
//		if (mouseY <= area.y || mouseY >= area.y + area.w)
//			return;
//		
//		isDown = true;
//		currBox.x = (mouseX - area.x) / area.z;
//		currBox.y = (mouseY - area.y) / area.w;
//		currBox.z = currBox.x;
//		currBox.w = currBox.y;
//	}
//
//	if (!io.MouseDown[0]) {
//		if (isDown) {
//			isDown = false;
//			if (textureName == "-1")
//				return;
//			currBox.z = (mouseX - area.x) / area.z;
//			currBox.w = (mouseY - area.y) / area.w;
//			currBox.z = std::clamp(currBox.z, 0.f, 1.f);
//			currBox.w = std::clamp(currBox.w, 0.f, 1.f);
//
//			float tmp = std::min(currBox.z, currBox.x);
//			currBox.z = std::max(currBox.z, currBox.x);
//			currBox.x = tmp;
//			tmp = std::min(currBox.w, currBox.y);
//			currBox.w = std::max(currBox.w, currBox.y);
//			currBox.y = tmp;
//
//			if (!isFree) 
//				if (currBox.z - currBox.x > currBox.w - currBox.y)
//					currBox.z = currBox.x + (currBox.w - currBox.y);
//				else
//					currBox.w = currBox.y + (currBox.z - currBox.x);
//		}
//	}
//
//}


static inline void NormalizeClamp(ImVec4& b) {
	float x0 = std::min(b.x, b.z), x1 = std::max(b.x, b.z);
	float y0 = std::min(b.y, b.w), y1 = std::max(b.y, b.w);
	b = ImVec4(
		std::clamp(x0, 0.f, 1.f), std::clamp(y0, 0.f, 1.f),
		std::clamp(x1, 0.f, 1.f), std::clamp(y1, 0.f, 1.f)
	);
}

static inline void ApplyAspectByMode(ImVec4& b, BoxMode mode, const ImVec4& area, const std::pair<int,int>& imgSize) {
	if (mode == BoxMode::Free) { NormalizeClamp(b); return; }

	float x0 = b.x, y0 = b.y, x1 = b.z, y1 = b.w;
	float dx = x1 - x0, dy = y1 - y0;
	if (dx == 0.f && dy == 0.f) return;

	float sgnx = (dx >= 0.f) ? 1.f : -1.f;
	float sgny = (dy >= 0.f) ? 1.f : -1.f;
	dx = std::fabs(dx);
	dy = std::fabs(dy);

	if (mode == BoxMode::KeepAspect) {
		// 이미지와 동일 비율(= 뷰포트와 동일 AR) → 정규화에서 dx == dy
		float s = std::max(dx, dy);
		dx = dy = s;
	}
	else if (mode == BoxMode::Square) {
		// 픽셀 정사각형: (dx*area.z) == (dy*area.w) → dx/dy = area.w/area.z
		float target = (imgSize.second > 0 && imgSize.first > 0) ? (float)imgSize.second / (float)imgSize.first : (area.w / area.z);
		if (dx > dy * target) dy = dx / target;
		else                  dx = dy * target;
	}
	else if (mode == BoxMode::CustomRatio) {
		float rw = (g_customRatioW > 0.0f) ? g_customRatioW : 1.0f;
		float rh = (g_customRatioH > 0.0f) ? g_customRatioH : 1.0f;
		float target = rw / rh;
		if (dx > dy * target) dy = dx / target;
		else                  dx = dy * target;
	}

	b = ImVec4(x0, y0, x0 + sgnx * dx, y0 + sgny * dy);
	NormalizeClamp(b);
}

void updateMouse() {
    ImGuiIO& io = ImGui::GetIO();
    if (g_squareBaseMode) return; // placing square base ROI disables drag selection
    if (textureName == "-1") return;
    if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered()) return;

	float mouseX = io.MousePos.x;
	float mouseY = io.MousePos.y;

	static bool isDown = false;

	// 1) 마우스 누른 순간: 시작점 고정
	if (io.MouseDown[0] && !isDown) {
		if (mouseX <= area.x || mouseX >= area.x + area.z) return;
		if (mouseY <= area.y || mouseY >= area.y + area.w) return;

        isDown = true;
        currBox.x = (mouseX - area.x) / area.z;
        currBox.y = (mouseY - area.y) / area.w;
        if (g_squareBaseROIEnabled) {
            currBox.x = std::clamp(currBox.x, g_squareBaseROI.x, g_squareBaseROI.z);
            currBox.y = std::clamp(currBox.y, g_squareBaseROI.y, g_squareBaseROI.w);
        }
        currBox.z = currBox.x;
        currBox.w = currBox.y;
    }

	// 2) 드래그 중: 끝점 갱신 + 모드 보정 (★ 실시간 보정)
    if (io.MouseDown[0] && isDown) {
        ImVec4 b = currBox;
        b.z = std::clamp((mouseX - area.x) / area.z, 0.f, 1.f);
        b.w = std::clamp((mouseY - area.y) / area.w, 0.f, 1.f);
        if (g_squareBaseROIEnabled) {
            b.z = std::clamp(b.z, g_squareBaseROI.x, g_squareBaseROI.z);
            b.w = std::clamp(b.w, g_squareBaseROI.y, g_squareBaseROI.w);
        }

        ApplyAspectByMode(b, g_boxMode, area, imageSize[textureName]);
        if (g_squareBaseROIEnabled) {
            b.x = std::clamp(b.x, g_squareBaseROI.x, g_squareBaseROI.z);
            b.z = std::clamp(b.z, g_squareBaseROI.x, g_squareBaseROI.z);
            b.y = std::clamp(b.y, g_squareBaseROI.y, g_squareBaseROI.w);
            b.w = std::clamp(b.w, g_squareBaseROI.y, g_squareBaseROI.w);
        }
        currBox = b;
    }

	// 3) 마우스 뗄 때: 최종 정리 + 모드 보정(한 번 더 안전하게)
	if (!io.MouseDown[0]) {
		if (isDown) {
			isDown = false;
			currBox.z = std::clamp((mouseX - area.x) / area.z, 0.f, 1.f);
			currBox.w = std::clamp((mouseY - area.y) / area.w, 0.f, 1.f);

			// 좌표 정렬
			float x0 = std::min(currBox.x, currBox.z);
			float x1 = std::max(currBox.x, currBox.z);
			float y0 = std::min(currBox.y, currBox.w);
			float y1 = std::max(currBox.y, currBox.w);
			currBox = ImVec4(x0, y0, x1, y1);

            ApplyAspectByMode(currBox, g_boxMode, area, imageSize[textureName]);
            if (g_squareBaseROIEnabled) {
                currBox.x = std::clamp(currBox.x, g_squareBaseROI.x, g_squareBaseROI.z);
                currBox.z = std::clamp(currBox.z, g_squareBaseROI.x, g_squareBaseROI.z);
                currBox.y = std::clamp(currBox.y, g_squareBaseROI.y, g_squareBaseROI.w);
                currBox.w = std::clamp(currBox.w, g_squareBaseROI.y, g_squareBaseROI.w);
            }
        }
    }
}

// Mouse-follow temporary square ROI placement; click to set ROI (no immediate crop)
void updateSquareBaseMode() {
    if (!g_squareBaseMode) { g_squarePreviewBox = ImVec4(0,0,0,0); return; }
    if (textureName == "-1") return;
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered()) return;

    auto sz = imageSize[textureName];
    float iw = (float)sz.first, ih = (float)sz.second;
    if (iw <= 0 || ih <= 0) return;
    float sw = 1.0f, sh = 1.0f;
    if (iw <= ih) { sw = 1.0f; sh = iw / ih; }
    else { sw = ih / iw; sh = 1.0f; }

    float mx = (io.MousePos.x - area.x) / area.z;
    float my = (io.MousePos.y - area.y) / area.w;
    float left = std::clamp(mx - sw * 0.5f, 0.0f, 1.0f - sw);
    float top  = std::clamp(my - sh * 0.5f, 0.0f, 1.0f - sh);
    g_squarePreviewBox = ImVec4(left, top, left + sw, top + sh);

    bool inside = (io.MousePos.x > area.x && io.MousePos.x < area.x + area.z &&
                   io.MousePos.y > area.y && io.MousePos.y < area.y + area.w);
    if (inside && ImGui::IsMouseClicked(0)) {
        g_squareBaseROIEnabled = true;
        g_squareBaseROI = g_squarePreviewBox;
        g_squareBaseMode = false;
        g_squarePreviewBox = ImVec4(0,0,0,0);
        // restore previous BoxMode when exiting placement
        if (g_roiPrevBoxModeSaved) { g_boxMode = g_roiPrevBoxMode; g_roiPrevBoxModeSaved = false; }
    }
}

// Mouse wheel zoom for capture mode (pixelated zoom, no interpolation changes here)
inline void updateZoom() {
    if (textureName == "-1") { g_zoom = 1.0f; return; }
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered()) return;
    if (io.MouseWheel != 0.0f) {
        g_zoom = std::clamp(g_zoom + io.MouseWheel * 0.15f, 0.2f, 8.0f);
    }
}

// Middle-mouse drag pans the zoomed image
inline void updatePan() {
    if (textureName == "-1") { g_pan = ImVec2(0,0); return; }
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered()) return;
    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        g_pan.x += io.MouseDelta.x;
        g_pan.y += io.MouseDelta.y;
    }
}


void updateKeyboard(std::string name) {
	ImGuiIO& io = ImGui::GetIO();
	if (!ImGui::IsWindowFocused())
		return;
	if (!io.KeyCtrl)
		return;

	if (io.KeysDown['W'])
		onWindow[name] = false;
	if (io.KeysDown['Z']) {
		onWindow.erase(name);
		glDeleteTextures(1, &imagesGL[name]);
		imagesGL.erase(name);
		imagesCV.erase(name);
		imagesCVOriginal.erase(name);
        g_rotationSteps.erase(name);
		imageSize.erase(name);
	}
	if (io.KeysDown['X'] && textureName == "-1") {
		onWindow[name] = false;
		textureName = name;
        g_zoom = 1.0f;
        g_pan = ImVec2(0,0);
        g_manualNudgeMode = false;
	}

}

void updateKeyboard() {
	ImGuiIO& io = ImGui::GetIO();
	if (GetForegroundWindow() != hwnd && ImGui::IsWindowFocused())
		return;
	if (!io.KeyCtrl)
		return;

	if (io.KeysDown['C'] && textureName != "-1") {
		onWindow[textureName] = true;
		textureName = "-1";
        g_zoom = 1.0f;
        g_pan = ImVec2(0,0);
        g_manualNudgeMode = false;
	}
	if (io.KeysDown['R']) 
		boxList.clear();

	if (io.KeysDown['S']) {
		if (boxList.size() != 0) {
			auto& top = boxList[boxList.size() - 1];
			if (top.first.x == currBox.x && top.first.y == currBox.y && top.first.z == currBox.z && top.first.w == currBox.w)
				return;
		}
		boxList.push_back({ currBox, color });
	 }
}

// Non-ctrl hotkeys for quick square base ROI control
inline void updateQuickKeys() {
    ImGuiIO& io = ImGui::GetIO();
    if (textureName == "-1") return; // only in capture mode
    if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered()) return;

    static bool prevQ = false;
    static bool prevE = false;
    bool q = io.KeysDown['Q'];
    bool e = io.KeysDown['E'];

    if (q && !prevQ) {
        // Toggle placement preview mode and sync Box Mode to Square while active
        if (!g_squareBaseMode) {
            if (!g_roiPrevBoxModeSaved) { g_roiPrevBoxMode = g_boxMode; g_roiPrevBoxModeSaved = true; }
            g_squareBaseMode = true;
            g_boxMode = BoxMode::Square;
        } else {
            g_squareBaseMode = false;
            g_squarePreviewBox = ImVec4(0,0,0,0);
            if (g_roiPrevBoxModeSaved) { g_boxMode = g_roiPrevBoxMode; g_roiPrevBoxModeSaved = false; }
        }
    }
    if (e && !prevE) {
        // Clear existing ROI quickly
        g_squareBaseROIEnabled = false;
        g_squareBaseROI = ImVec4(0,0,0,0);
    }

    prevQ = q; prevE = e;
}

// Toggle manual nudge (M) then move current box with arrow keys
inline void updateBoxNudge() {
    if (textureName == "-1") return;
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered()) return;

    static bool prevM = false;
    bool m = io.KeysDown['M'];
    if (m && !prevM) g_manualNudgeMode = !g_manualNudgeMode;
    prevM = m;
    if (!g_manualNudgeMode) return;

    float step = io.KeyShift ? 0.01f : 0.002f;
    auto nudge = [&](float dx, float dy) {
        currBox.x += dx; currBox.z += dx;
        currBox.y += dy; currBox.w += dy;
        NormalizeClamp(currBox);
        if (g_squareBaseROIEnabled) {
            currBox.x = std::clamp(currBox.x, g_squareBaseROI.x, g_squareBaseROI.z);
            currBox.z = std::clamp(currBox.z, g_squareBaseROI.x, g_squareBaseROI.z);
            currBox.y = std::clamp(currBox.y, g_squareBaseROI.y, g_squareBaseROI.w);
            currBox.w = std::clamp(currBox.w, g_squareBaseROI.y, g_squareBaseROI.w);
        }
    };

    if (io.KeysDown[VK_LEFT])  nudge(-step, 0.f);
    if (io.KeysDown[VK_RIGHT]) nudge(step, 0.f);
    if (io.KeysDown[VK_UP])    nudge(0.f, -step);
    if (io.KeysDown[VK_DOWN])  nudge(0.f, step);
}
