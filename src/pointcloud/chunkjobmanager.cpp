#include "chunkjobmanager.h"
#include "core/frustumcull.h"

namespace PPC = ProtoVoxel::PointCloud;

PPC::ChunkJobManager::ChunkJobManager() : semaphore()
{
	chunkers = nullptr;
	current = nullptr;
	next = nullptr;

	totalJobCount = 0;
	finishedJobCount = 0;
	reloadCurrent = false;
}

PPC::ChunkJobManager::~ChunkJobManager() {}

void PPC::ChunkJobManager::Initialize(MeshMalloc* mesh_mem_ptr, DrawCmdList* cmd_list_ptr)
{
	mesh_mem = mesh_mem_ptr;
	cmd_list = cmd_list_ptr;
	proc_cnt = std::thread::hardware_concurrency() - 1;
	if (proc_cnt <= 0)
		proc_cnt = 8;

	current = new struct JobQueue();
	next = new struct JobQueue();

	chunkers = new struct ChunkThread[proc_cnt];
	for (int i = 0; i < proc_cnt; i++)
	{
		chunkers[i].thd = new std::thread(&PPC::ChunkJobManager::JobRunner, this, i);
	}
}

void PPC::ChunkJobManager::RequestRemesh(PPC::Chunk* chnk)
{
	ChunkJob job;
	job.target_chunk = chnk;
	job.chunk_id = chnk->id;
	job.type = ChunkJobType::CompileSurface;

	next->queue_guard.lock();
	next->unique_chunks.insert(chnk->id);
	next->jobs.push(job);
	next->queue_guard.unlock();
}

void PPC::ChunkJobManager::RequestBuild(PPC::Chunk* chnk)
{
	ChunkJob job;
	job.target_chunk = chnk;
	job.chunk_id = chnk->id;
	job.type = ChunkJobType::BuildChunk;

	next->queue_guard.lock();
	next->unique_chunks.insert(chnk->id);
	next->jobs.push(job);
	next->queue_guard.unlock();
}

void PPC::ChunkJobManager::RequestCull(PPC::Chunk* chnk, glm::mat4& ivp)
{
	ChunkJob job;
	job.target_chunk = chnk;
	job.chunk_id = chnk->id;
	job.type = ChunkJobType::CullChunk;
	job.ivp = ivp;

	next->queue_guard.lock();
	next->unique_chunks.insert(chnk->id);
	next->jobs.push(job);
	next->queue_guard.unlock();
}

void PPC::ChunkJobManager::JobRunner(int tid)
{
	ChunkUpdater& updater = chunkers[tid].updater;
	std::queue<struct ChunkJob> current_jobs;
	struct JobQueue* cur_q = current;
	while (true)
	{
		semaphore.Decrement();
		if (reloadCurrent)
		{
			cur_q = current; //reload the pointer
			finishedJobCount++;
			swap_tracker.Decrement();	//only allow a thread to execute this once
		}
		else
		{
			cur_q->queue_guard.lock();
			auto job = cur_q->jobs.top();
			current_jobs.push(job);
			cur_q->jobs.pop();
			while (cur_q->jobs.size() > 0 && cur_q->jobs.top().chunk_id == current_jobs.front().chunk_id)
			{
				job = cur_q->jobs.top();
				current_jobs.push(job);
				cur_q->jobs.pop();
			}
			cur_q->queue_guard.unlock();
		}

		//execute the requested job on the chunk
		bool first_run = true;
		while (current_jobs.size() > 0)
		{
			auto job = current_jobs.front();
			current_jobs.pop();

			if (first_run) {
				updater.UnpackChunk(job.target_chunk);
				first_run = false;
			}
			switch (job.type)
			{
			case ChunkJobType::BuildChunk:
			{
				/*siv::PerlinNoise noise(0);
				auto posvec = job.target_chunk->GetPosition();
				for (int x = -1; x < 31; x++)
					for (int z = -1; z < 31; z++)
						for (int y = -1; y < 63; y++)
						{
							auto d = noise.noise3D_0_1((posvec.x + x) * 0.005, (posvec.y + y) * 0.005, (posvec.z + z) * 0.005);

							//float off = 10;
							//if (fabsf((x - off) * (x - off) + (y - off) * (y - off) + (z - off) * (z - off)) < 10)
							//	d = 1;

							if (d > 0.5)
								//auto d = noise.noise3D_0_1((posvec.x + x) * 0.005, 0 * 0.005, (posvec.z + z) * 0.005);
								//for (int y = posvec.y; y < d * 240 && y < posvec.y + 64; y++)
								updater.SetBlock(x + 1, y + 1, z + 1, 1);
							//else
							//	updater.SetBlock(x + 1, y + 1, z + 1, 0);
						}*/
			}
			break;
			case ChunkJobType::CompileSurface:
			{
				uint32_t loopback_cntr = 0;

				//Allocate buffer
				auto count = updater.GetCompiledLength();
				auto mem_blk = mesh_mem->Alloc(count, loopback_cntr);
				updater.Compile(mem_blk, job.target_chunk->min_bound, job.target_chunk->max_bound);
				mesh_mem->Flush(mem_blk, count);

				//Record mesh info into chunk
				job.target_chunk->status = ChunkStatus::None;
				job.target_chunk->loopback_cnt = loopback_cntr;
				job.target_chunk->mesh_area_ptr = mesh_mem->GetOffset(mem_blk);
				job.target_chunk->mesh_area_len = count;
			}
			break;
			case ChunkJobType::CullChunk:
			{
				//TODO: Check if chunk has been overwritten in buffer and remesh if so
				//Frustum f(job.ivp);
				//if (f.IsBoxVisible(glm::vec3(job.target_chunk->GetPosition()), glm::vec3(job.target_chunk->GetPosition()) + glm::vec3(30, 62, 30)))
					cmd_list->RecordDraw(job.target_chunk->mesh_area_len, 0, job.target_chunk->mesh_area_ptr, 0, 1, job.target_chunk->position, job.target_chunk->min_bound, job.target_chunk->max_bound);
			}
			break;
			}
			finishedJobCount++;
			finished_job_tracker.Increment(1);
		}
	}
}

void PPC::ChunkJobManager::FinishCurrentTasks() {
	//Wait for all jobs to finish
	for (int i = 0; i < totalJobCount; i++)
		finished_job_tracker.Decrement();
	//while (finishedJobCount != totalJobCount)
	//	;
}

void PPC::ChunkJobManager::EndFrame()
{
	//Swap queues
	auto tmp = current;
	current = next;
	next = tmp;

	reloadCurrent = true;
	finishedJobCount = 0;
	totalJobCount = proc_cnt;
	semaphore.Increment(proc_cnt);

	//Wait for all threads to swap
	while (finishedJobCount != totalJobCount)
		;
	swap_tracker.Increment(proc_cnt);	//Allow all threads to proceed

	//Start running next set of tasks
	reloadCurrent = false;
	finishedJobCount = 0;
	totalJobCount = current->jobs.size();
	semaphore.Increment(current->unique_chunks.size());
	current->unique_chunks.clear();
}

uint64_t ProtoVoxel::PointCloud::ChunkJobManager::ChunkJob::get_sorter() const
{
	return static_cast<uint64_t>(type) | ((uint64_t)chunk_id << 32);
}

bool ProtoVoxel::PointCloud::operator<(const struct ProtoVoxel::PointCloud::ChunkJobManager::ChunkJob& lhs, const struct ProtoVoxel::PointCloud::ChunkJobManager::ChunkJob& rhs)
{
	return lhs.get_sorter() < rhs.get_sorter();
}

bool ProtoVoxel::PointCloud::operator>(const struct ProtoVoxel::PointCloud::ChunkJobManager::ChunkJob& lhs, const struct ProtoVoxel::PointCloud::ChunkJobManager::ChunkJob& rhs)
{
	return lhs.get_sorter() > rhs.get_sorter();
}
