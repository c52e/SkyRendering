#include "GLReloadableProgram.h"

#include <iostream>
#include <sstream>
#include <algorithm>

#include <imgui.h>

void GLReloadableProgram::Reload() {
	if (loader_) {
		try {
			program_ = loader_();
		}
		catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		}
	}
}

void GLReloadableProgram::ReloadAll() {
	for (auto p : GetObjects()) {
		p->Reload();
	}
}

GLReloadableComputeProgram::GLReloadableComputeProgram(std::string path, std::vector<glm::ivec3> localsizes
		, std::function<std::string(const std::string&)> post_process, std::string tag) {
	data_ = std::make_unique<Data>();
	data_->display_text = tag + ": " + path;
	data_->path = std::move(path);
	data_->post_process = std::move(post_process);
	data_->localsizes_str.reserve(localsizes.size());
	for (const auto& localsize : localsizes) {
		std::stringstream ss;
		ss << localsize.x << " * " << localsize.y << " * " << localsize.z;
		data_->localsizes_str.emplace_back(ss.str());
	}
	data_->programs.resize(localsizes.size());
	data_->localsizes = std::move(localsizes);
	Construct(0);
}

void GLReloadableComputeProgram::DrawGUI() {
	if (!data_) return;
	auto current_value = data_->localsizes_str[data_->index].c_str();
	ImGui::PushItemWidth(110);
	if (ImGui::BeginCombo(data_->display_text.c_str(), current_value)) {
		for (int i = 0; i < static_cast<int>(data_->localsizes_str.size()); ++i) {
			const auto& value = data_->localsizes_str[i];
			bool is_selected = (current_value == value.c_str());
			if (ImGui::Selectable(value.c_str(), is_selected))
				data_->index = i;
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();
	if (data_->programs[data_->index].id() == 0) {
		Construct(data_->index);
	}
}

void GLReloadableComputeProgram::DrawGUIAll() {
	static std::vector<GLReloadableComputeProgram*> objects;
	objects.clear();
	objects.reserve(GetObjects().size());
	for (auto p : GetObjects()) {
		objects.push_back(p);
	}
	std::sort(objects.begin(), objects.end(),
		[](const GLReloadableComputeProgram* lhs, const GLReloadableComputeProgram* rhs) {
			return lhs->data_->display_text < rhs->data_->display_text;
		});
	for (auto p : objects) {
		p->DrawGUI();
	}
}

void GLReloadableComputeProgram::Construct(int i) {
	data_->programs[i] = ([p = data_.get(), i]{
		const auto & localsize = p->localsizes[i];
		std::stringstream ss;
		ss << "#define LOCAL_SIZE_X " << std::to_string(localsize.x) << "\n";
		ss << "#define LOCAL_SIZE_Y " << std::to_string(localsize.y) << "\n";
		ss << "#define LOCAL_SIZE_Z " << std::to_string(localsize.z) << "\n";
		ss << ReadWithPreprocessor(p->path.c_str());
		return GLProgram(p->post_process(ss.str()).c_str());
	});
}

std::vector<glm::ivec3> GLReloadableComputeProgram::ToVec3(const std::vector<glm::ivec2>& in) {
	std::vector<glm::ivec3> out;
	out.reserve(in.size());
	for (const auto& v : in) {
		out.emplace_back(glm::ivec3(v, 1));
	}
	return out;
}
