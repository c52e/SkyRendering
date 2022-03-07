#pragma once

#include <string>

void SetCurrentDirToExe();

std::string ReadFile(const char* path);

// 根据天顶角theta和方位角phi(弧度)计算方向向量，Y轴为上方向
void FromThetaPhiToDirection(float theta, float phi, float direction[3]);
