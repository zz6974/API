#ifndef CCAUTOLOCK_H
#define CCAUTOLOCK_H
#include <condition_variable>

typedef std::unique_lock<std::mutex> CCUniqueLock;
typedef std::mutex CCMutex;
typedef std::condition_variable CCCondition_Var;

namespace CTL {
	class AutoLock {
	public:
		template<typename F>
		explicit AutoLock(CCMutex& m,F && f) : m_mutex(m) {
			m_locked = true;
			m_mutex.lock();
			f();
			m_mutex.unlock();
		}
		explicit AutoLock(CCMutex& m) : m_mutex(m){
			m_mutex.lock();
		}
		~AutoLock(){
			if(!m_locked) {
				m_mutex.unlock();
			}
		}
		void UnLock() const {
			m_mutex.unlock();
		}
		void Lock() const {
			m_mutex.lock();
		}
	private:
		CCMutex& m_mutex;
		bool m_locked = false;
	};
}



#endif
