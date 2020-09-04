#pragma once
#include "glm/glm.hpp"
#include "chunkupdater.h"
#include "draw_cmdlist.h"
#include "core/semaphore.h"
#include <atomic>
#include <mutex>
#include <unordered_set>
#include <queue>
#include <stdint.h>
#include <thread>
#include "mesh_malloc.h"

namespace ProtoVoxel::Voxel
{
	class ChunkJobManager
	{
	public:
		enum class ChunkJobType
		{
			BuildChunk,
			SetBlock,
			CompileMesh,
			CullChunk,
		};

		struct ChunkJob
		{
			Chunk *target_chunk;
			ChunkJobType type;
			uint32_t chunk_id;
			glm::mat4 ivp;

			uint64_t get_sorter() const;

			friend bool operator<(const struct ChunkJob &lhs, const struct ChunkJob &rhs);
			friend bool operator>(const struct ChunkJob &lhs, const struct ChunkJob &rhs);
		};

		struct JobQueue
		{
			std::priority_queue<struct ChunkJob, std::vector<struct ChunkJob>, std::greater<struct ChunkJob>> jobs;
			std::mutex queue_guard;
			std::unordered_set<uint32_t> unique_chunks;

			JobQueue() : jobs(), queue_guard() {}
		};

	private:
		struct JobQueue *volatile current;
		struct JobQueue *volatile next;
		int32_t totalJobCount = 0;
		int32_t proc_cnt;
		std::atomic_int32_t finishedJobCount;
		std::atomic_bool reloadCurrent;
		ProtoVoxel::Core::Semaphore finished_job_tracker;
		ProtoVoxel::Core::Semaphore semaphore;
		ProtoVoxel::Core::Semaphore swap_tracker;
		ProtoVoxel::Voxel::MeshMalloc *mesh_mem;
		ProtoVoxel::Voxel::DrawCmdList* cmd_list;

		void JobRunner(int tid);
		//Sort next jobs to group them by chunk and job type before submission

		struct ChunkThread
		{
			std::thread *thd;
			ChunkUpdater updater;

			ChunkThread() : updater()
			{
				thd = nullptr;
			}
		};
		struct ChunkThread *chunkers;

	public:
		ChunkJobManager();
		~ChunkJobManager();

		void Initialize(MeshMalloc* mesh_mem_ptr, DrawCmdList* cmd_list_ptr);
		void RequestRemesh(ProtoVoxel::Voxel::Chunk *chnk);
		void RequestBuild(ProtoVoxel::Voxel::Chunk* chnk);
		void RequestCull(ProtoVoxel::Voxel::Chunk* chnk, glm::mat4 &ivp);
		void FinishCurrentTasks();
		void EndFrame();
	};
} // namespace ProtoVoxel::Voxel