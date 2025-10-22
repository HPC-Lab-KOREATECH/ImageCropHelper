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

	b = ImVec4(x0, y0, x0 + sgnx * dx, y0 + sgny * dy);
	NormalizeClamp(b);
}

void updateMouse() {
	ImGuiIO& io = ImGui::GetIO();
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
		currBox.z = currBox.x;
		currBox.w = currBox.y;
	}

	// 2) 드래그 중: 끝점 갱신 + 모드 보정 (★ 실시간 보정)
	if (io.MouseDown[0] && isDown) {
		ImVec4 b = currBox;
		b.z = std::clamp((mouseX - area.x) / area.z, 0.f, 1.f);
		b.w = std::clamp((mouseY - area.y) / area.w, 0.f, 1.f);

		ApplyAspectByMode(b, g_boxMode, area, imageSize[textureName]);
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
		}
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
		imageSize.erase(name);
	}
	if (io.KeysDown['X'] && textureName == "-1") {
		onWindow[name] = false;
		textureName = name;
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
