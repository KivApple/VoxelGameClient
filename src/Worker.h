#pragma once

#include <atomic>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <easylogging++.h>

template<typename Job> class Worker: public std::thread {
	std::string m_name;
	std::deque<Job> m_queue;
	std::mutex m_queueMutex;
	std::condition_variable m_queueCondVar;
	bool m_running = true;
	bool m_processRemaining = false;
	Job *m_currentJob = nullptr;
	std::condition_variable m_currentJobCondVar;
	
	void run() {
		LOG(INFO) << "Started " << m_name << " worker";
		std::unique_lock<std::mutex> lock(m_queueMutex);
		while (m_running || (m_processRemaining && !m_queue.empty())) {
			if (m_queue.empty()) {
				m_queueCondVar.wait(lock);
				continue;
			}
			auto job = std::move(m_queue.front());
			m_queue.pop_front();
			m_currentJob = &job;
			lock.unlock();
			job();
			lock.lock();
			m_currentJob = nullptr;
			m_currentJobCondVar.notify_all();
		}
		LOG(INFO) << "Stopped " << m_name << " worker";
	}
	
public:
	explicit Worker(std::string name): m_name(std::move(name)) {
		*static_cast<std::thread*>(this) = std::thread(&Worker::run, this);
	}
	
	~Worker() {
		shutdown();
	}
	
	template<typename ...Args> void post(Args&&... args) {
		std::unique_lock<std::mutex> lock(m_queueMutex);
		m_queue.emplace_back(std::forward<Args>(args)...);
		m_queueCondVar.notify_one();
	}
	
	void cancel(const Job &job, bool waitRunning) {
		std::unique_lock<std::mutex> lock(m_queueMutex);
		auto it = m_queue.begin();
		while (it != m_queue.end()) {
			if (*it == job) {
				it = m_queue.erase(it);
			} else {
				++it;
			}
		}
		if (waitRunning) {
			while (m_currentJob != nullptr && *m_currentJob == job) {
				m_currentJobCondVar.wait(lock);
			}
		}
	}
	
	void shutdown(bool processRemaining = false) {
		std::unique_lock<std::mutex> lock(m_queueMutex);
		if (!m_running) return;
		m_running = false;
		m_processRemaining = processRemaining;
		m_queueCondVar.notify_one();
		lock.unlock();
		join();
	}
	
};
