#include "Utils.h"
#include <iostream>
#include <fstream>
#include <Windows.h>
#include <filesystem>

// ��������Ŀ¼Ϊexe����Ŀ¼�����VS����ʱĬ��Ŀ¼��ProjectĿ¼������
void SetCurrentDirToExe() {
	static TCHAR buffer[MAX_PATH];
	memset(buffer, 0, sizeof(buffer));
	GetModuleFileName(0, buffer, MAX_PATH);
	auto path = std::filesystem::path(buffer).parent_path();
	std::filesystem::current_path(path);
}

std::string ReadFile(const char* path) {
	std::ifstream fin(path);
	if (!fin)
		throw std::runtime_error(std::string("Read file failed: ") + path);
	return std::string(std::istreambuf_iterator<char>{fin}, {});
}

void FromThetaPhiToDirection(float theta, float phi, float direction[3]) {
	float cos_theta = cos(theta);
	float sin_theta = sin(theta);
	float cos_phi = cos(phi);
	float sin_phi = sin(phi);
	direction[0] = cos_phi * sin_theta;
	direction[1] = cos_theta;
	direction[2] = sin_phi * sin_theta;
}
