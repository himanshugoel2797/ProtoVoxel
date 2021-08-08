// Copyright (c) 2021 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"
#include "model.h"


static tinygltf::TinyGLTF loader;

int ProtoVoxel::Misc::Model::LoadModel(const char *file, ProtoVoxel::Misc::Model &m)
{
    tinygltf::Model mdl;
	bool sts = loader.LoadBinaryFromFile(&mdl, nullptr, nullptr, file);
    if (!sts) return -1;
    
    for (auto node : mdl.nodes)
    {
        struct Mesh mesh;
        if (node.mesh != -1) 
        {
            auto mesh_l = mdl.meshes[node.mesh];

            //Is a mesh, store the data
            //extract mesh data and arrange for rendering, compute AABB too
            //need texcoords, vertices and indices
            //TODO: extract and store materials (pbr textures + derivative map)
            //get mesh rendering working
            //setup lua for object simulation
            //rudimentary physics engine that simulates forces and gravity
            //raycasting object selection
            //basic imgui ui for adding parts
            //clipmap sphere rendering
        }
        
        //Store properties
        for (auto prop_key : node.extras.Keys()) {
            auto prop = node.extras.Get(prop_key);
            if (prop.IsString())
                mesh.str_props[prop_key] = prop.Get<std::string>();

            if (prop.IsNumber())
                mesh.num_props[prop_key] = (float)prop.GetNumberAsDouble();
        }
        m.meshes.push_back(mesh);
    }

    return 0;
}