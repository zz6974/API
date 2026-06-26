#include "IO/CCThreadPool.h"


CTL::ThreadPool::ThreadPool() {
	// CC::Println("ThreadPool Init");
}

CTL::ThreadPool::~ThreadPool() {
	Stop();
}

void CTL::ThreadPool::InitStart(const size_t corePoolSize, const size_t maximumPoolSize, const int keepAliveTime) {
	m_isStop = false;
	if (!m_thread.empty()) {
		return;
	}
	m_corePoolSize = corePoolSize;
	m_maximumPoolSize = maximumPoolSize;
	m_keepAliveTime = keepAliveTime;
	m_C_threadPool = 0;
	E_PoolSize = 0;
	T_PoolSize = 0;
	T_CorePoolSize = 0;
	for (size_t i = 0; i < m_corePoolSize; i++) {
		const auto thread = new std::thread(&ThreadPool::Worker, this, static_cast<int>(i));
		m_thread.emplace_back(thread);
	}
}

void CTL::ThreadPool::Stop() {
	m_isStop = true;
	m_condition.notify_all();
	if (!m_thread.empty()) {
		for (auto& thread : m_thread) {
			if (thread->joinable()) {
				thread->join(); // 等待所有线程完成
				if (thread) {
					delete thread;
					thread = nullptr;
				}
			}
		}
		m_thread.clear();
	}
	// CC::Println("ThreadPool Close");
}

int CTL::ThreadPool::GetActiveThread() const {
	return T_PoolSize;
}

int CTL::ThreadPool::GetMaxPoolSize() const {
	return m_maximumPoolSize;
}

int CTL::ThreadPool::GetCorePoolSize() const {
	return m_C_threadPool;
}

int CTL::ThreadPool::GetTimeout() const {
	return m_keepAliveTime;
}

int CTL::ThreadPool::GetActiveExtraThread() const {
	return E_PoolSize;
}

void CTL::ThreadPool::Worker(int index) {
	++m_C_threadPool;
	while (!m_isStop) {
		Function<void()> task = nullptr;
		{
			CCUniqueLock lock(m_mutex);
			// 使用超时等待避免无限阻塞
			m_condition.wait_for(lock, std::chrono::milliseconds(m_keepAliveTime),
							   [this] { return !m_taskQueue.empty() || m_isStop; });

			if (!m_taskQueue.empty()) {
				task = std::move(m_taskQueue.front());
				m_taskQueue.pop();
			} else if (m_isStop) {
				break;
			}
		}

		if (task) {
			try {
				++T_PoolSize;
				task();
				--T_PoolSize;
			}
			catch (CCException& e) {
				String errorMsg = "ThreadPool::Worker Error in function: ";
				errorMsg += e.what();
				CC_PRINT_ERROR(errorMsg);
			}
		}
		// 定期检查是否需要退出以避免僵尸线程
		else if (m_isStop) {
			break;
		}
	}
	--m_C_threadPool;
}

void CTL::ThreadPool::CC_PRINT_ERROR(const CTL::String& str) {
	CC::TimePrintln(str);
}

void CTL::ThreadPool::ExtraThread(Function<void()> Task) {
	Thread::addTask([&](){
		++E_PoolSize;
		try {
			while (!m_isStop) {
				if (Task) {
					++T_PoolSize;
					Task();
					--T_PoolSize;
				}
				Thread::Sleep(m_keepAliveTime);
				{
					CCUniqueLock lock(m_mutex);
					if (m_isStop) {
						break;
					}
					if (m_taskQueue.empty() || T_CorePoolSize < m_corePoolSize) {
						break;
					}
					m_condition.wait(lock, [this] {return !m_taskQueue.empty() || m_isStop; });
					Task = std::move(m_taskQueue.front());
					m_taskQueue.pop();
				}
				if (!Task) {
					break;
				}
			}
		}
		catch (CCException& e) {
			CC_PRINT_ERROR("ThreadPool::Worker() Error: " + CTL::String(e.what()));
		}
		--E_PoolSize;
	});
}
