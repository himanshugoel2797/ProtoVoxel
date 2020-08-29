#include "chunkjobmanager.h"

namespace PVV = ProtoVoxel::Voxel;

PVV::ChunkJobManager::ChunkJobManager() : semaphore()
{
	chunkers = nullptr;
	current = nullptr;
	next = nullptr;

	finishedJobCount = 0;
	reloadCurrent = false;
}

PVV::ChunkJobManager::~ChunkJobManager() {}

void PVV::ChunkJobManager::Initialize()
{
	proc_cnt = std::thread::hardware_concurrency();
	if (proc_cnt == 0)
		proc_cnt = 8;

	current = new struct JobQueue();
	next = new struct JobQueue();

	chunkers = new struct ChunkThread[proc_cnt];
	for (int i = 0; i < proc_cnt; i++)
	{
		chunkers[i].thd = new std::thread(&PVV::ChunkJobManager::JobRunner, this, i);
	}
}

void PVV::ChunkJobManager::RequestRemesh(PVV::Chunk *chnk)
{

	ChunkJob job;
	job.target_chunk = chnk;
	job.type = ChunkJobType::CompileMesh;

	next->queue_guard.lock();
	next->jobs.push(job);
	next->queue_guard.unlock();
}

void PVV::ChunkJobManager::JobRunner(int tid)
{
	ChunkUpdater &updater = chunkers[tid].updater;
	std::queue<struct ChunkJob> current_jobs;
	struct JobQueue *cur_q = current;
	while (true)
	{
		semaphore.Decrement();
		if (reloadCurrent)
		{
			cur_q = current; //reload the pointer
			finishedJobCount++;
		}
		else
		{
			cur_q->queue_guard.lock();
			auto job = cur_q->jobs.top();
			current_jobs.push(job);
			cur_q->jobs.pop();
			while (cur_q->jobs.size() > 0 && cur_q->jobs.top().chunk_id == current_jobs.front().chunk_id)
			{
				semaphore.Decrement();

				job = cur_q->jobs.top();
				current_jobs.push(job);
				cur_q->jobs.pop();
			}
			cur_q->queue_guard.unlock();
		}

		//execute the requested job on the chunk
		for (int i = 0; i < current_jobs.size(); i++)
		{
			auto job = current_jobs.front();
			current_jobs.pop();

			updater.UnpackChunk(job.target_chunk);
			switch (job.type)
			{
			case ChunkJobType::CompileMesh:
			{
				//Allocate temporary buffer locally
				//Copy exact sized allocation into global mesh pool
				//Record mesh info into chunk
			}
			break;
			case ChunkJobType::SetBlock:

				break;
			}
			finishedJobCount++;
		}
	}
}

void PVV::ChunkJobManager::EndFrame()
{
	//Wait for all jobs to finish
	while (finishedJobCount != totalJobCount)
		;
	//Swap queues
	auto tmp = current;
	current = next;
	next = current;

	reloadCurrent = true;
	finishedJobCount = 0;
	totalJobCount = proc_cnt;
	semaphore.Increment(proc_cnt);

	//Wait for all threads to swap
	while (finishedJobCount != totalJobCount)
		;

	//Start running next set of tasks
	reloadCurrent = false;
	finishedJobCount = 0;
	totalJobCount = current->jobs.size();
	semaphore.Increment(totalJobCount);
}

uint64_t ProtoVoxel::Voxel::ChunkJobManager::ChunkJob::get_sorter() const
{
	return static_cast<uint64_t>(type) | ((uint64_t)chunk_id << 32);
}

bool ProtoVoxel::Voxel::operator<(const struct ProtoVoxel::Voxel::ChunkJobManager::ChunkJob &lhs, const struct ProtoVoxel::Voxel::ChunkJobManager::ChunkJob &rhs)
{
	return lhs.get_sorter() < rhs.get_sorter();
}

bool ProtoVoxel::Voxel::operator>(const struct ProtoVoxel::Voxel::ChunkJobManager::ChunkJob &lhs, const struct ProtoVoxel::Voxel::ChunkJobManager::ChunkJob &rhs)
{
	return lhs.get_sorter() > rhs.get_sorter();
}
