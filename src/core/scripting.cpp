// Copyright (c) 2021 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "scripting.h"
//#include "lua.hpp"

namespace PVC = ProtoVoxel::Core;

PVC::Script::Script(){
    //state = luaL_newstate(); 
    //luaL_openlibs(state);
    //load any standard libraries
}

PVC::Script::~Script() {
    //lua_close(state);
}

void PVC::Script::LoadScript(const char* file) {
    //luaL_loadfile(state, file);
}

void PVC::Script::Call(const char* func) {
    //lua_getglobal(state, func);
    //lua_call(state, 0, 0);
}