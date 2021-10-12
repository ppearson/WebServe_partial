/*
 WebServe
 Copyright 2018 Peter Pearson.

 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ---------
*/

#ifndef THREADED_TASK_WORKER_H
#define THREADED_TASK_WORKER_H

#include <thread>
#include <atomic>
#include <condition_variable>

#include <vector>
#include <queue>

class ThreadedTaskWorker
{
public:
	class Task
	{
	public:
		Task()
		{
		}

		virtual ~Task()
		{
		}

		virtual void doTask() = 0;
	};

	ThreadedTaskWorker(unsigned int numThreads);
	virtual ~ThreadedTaskWorker()
	{

	}

	void addTask(Task* pTask);

	void process();


protected:
	void workerThreadFunction();

protected:
	std::vector<std::thread>		m_aWorkerThreads;

	std::mutex						m_lock;
	unsigned int					m_numThreads;

	std::queue<Task*>				m_aTasks;

	unsigned int					m_numTasksToDo;
	unsigned int					m_numTasksCompleted;
};

#endif // THREADED_TASK_WORKER_H
