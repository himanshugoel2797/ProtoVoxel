#pragma once
#include <iostream>
#include <stdint.h>
#if _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif

namespace ProtoVoxel::Core
{
	class Semaphore
	{
	private:
#if _WIN32
		HANDLE sema;
#else
		sem_t sema;
#endif
	public:
		inline Semaphore(uint32_t initvalue = 0, uint32_t maxvalue = 1024)
		{
#if _WIN32
			sema = CreateSemaphore(nullptr, initvalue, maxvalue, nullptr);
#else
			sem_init(&sema, 0, initvalue);
#endif
		}
		inline ~Semaphore()
		{
#if _WIN32
			CloseHandle(sema);
#else
			sem_destroy(&sema);
#endif
		}

		inline void Increment(uint32_t v)
		{
#if _WIN32
			ReleaseSemaphore(sema, v, nullptr);
#else
			for (uint32_t i = 0; i < v; i++)
				sem_post(&sema);
#endif
		}

		inline void Decrement()
		{
#if _WIN32
			WaitForSingleObject(sema, INFINITE);
#else
			sem_wait(&sema);
#endif
		}
	};
} // namespace ProtoVoxel::Core