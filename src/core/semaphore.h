#pragma once
#include <stdint.h>
#include <iostream>
#if _WIN32
#include <Windows.h>
#else
#include <semaphore>
#endif

namespace ProtoVoxel::Core {
	class Semaphore {
	private:
#if _WIN32
		HANDLE sema;
#endif
	public:
		inline Semaphore(uint32_t initvalue = 0, uint32_t maxvalue = 1024) {
#if _WIN32
			sema = CreateSemaphore(nullptr, initvalue, maxvalue, nullptr);
#endif
		}
		inline ~Semaphore() {
#if _WIN32
			CloseHandle(sema);
#endif
		}

		inline void Increment(uint32_t v) {
#if _WIN32
			ReleaseSemaphore(sema, v, nullptr);
#endif
		}

		inline void Decrement() {
#if _WIN32
			WaitForSingleObject(sema, INFINITE);
#endif
		}

	};
}