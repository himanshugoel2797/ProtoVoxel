// Copyright (c) 2021 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once
#include <string>
#include <vector>
#include <map>

namespace ProtoVoxel::Misc {
	struct Mesh {
	private:
	public:
		std::map<std::string, std::string> str_props;
		std::map<std::string, float> num_props;
	};

	class Model {
    private:
	public:
		std::vector<struct ProtoVoxel::Misc::Mesh> meshes;
        Model() = default;
		static int LoadModel(const char *file, Model &m);
	};
}