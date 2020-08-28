#pragma once
#include <stdint.h>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include "core/semaphore.h"
#include "chunkupdater.h"

namespace ProtoVoxel::Voxel
{
	class ChunkJobManager
	{
	public:
		enum class ChunkJobType
		{
			SetBlock,
			CompileMesh,
			CullChunk,
		};

		struct ChunkJob
		{
			Chunk* target_chunk;
			ChunkJobType type;
			uint32_t chunk_id;

			uint64_t get_sorter() const;

			friend bool operator <(const struct ChunkJob& lhs, const struct ChunkJob& rhs);
			friend bool operator >(const struct ChunkJob& lhs, const struct ChunkJob& rhs);
		};
		
		struct JobQueue {
			std::priority_queue<struct ChunkJob, std::vector<struct ChunkJob>, std::less<struct ChunkJob>> jobs;
			std::mutex queue_guard;

			JobQueue() : jobs(), queue_guard() {}

		};

	private:

		struct JobQueue* volatile current;
		struct JobQueue* volatile next;
		int32_t totalJobCount = 0;
		int32_t proc_cnt;
		std::atomic_int32_t finishedJobCount = 0;
		std::atomic_bool reloadCurrent = false;
		ProtoVoxel::Core::Semaphore semaphore;

		void JobRunner(int tid);
		//Sort next jobs to group them by chunk and job type before submission

		struct ChunkThread
		{
			std::thread* thd;
			ChunkUpdater updater;

			ChunkThread() : updater() {
				thd = nullptr;
			}
		};
		struct ChunkThread* chunkers;

	public:
		ChunkJobManager();
		~ChunkJobManager();

		void Initialize();
		void RequestRemesh(ProtoVoxel::Voxel::Chunk* chnk);
		void EndFrame();
	};
} // namespace ProtoVoxel::Voxel