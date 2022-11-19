// Copyright (c) 2021 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once
//#include "lua.hpp"

namespace ProtoVoxel::Core {
    class Script {
        private:
            //lua_State* state;
        public:
            Script();
            ~Script();
            void LoadScript(const char* file);
            void Call(const char* func);
    };
}