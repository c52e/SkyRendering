#pragma once

#include <string>

void SetCurrentDirToExe();

std::string ReadFile(const char* path);

// �����춥��theta�ͷ�λ��phi(����)���㷽��������Y��Ϊ�Ϸ���
void FromThetaPhiToDirection(float theta, float phi, float direction[3]);
