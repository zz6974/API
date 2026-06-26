#include "IO/CCThread.h"
#include "CCSystem.h"

using namespace CTL;

std::mutex CTL::Thread::thread_mutex_t;
Queue<std::function<void()>> CTL::Thread::CC_thread_Queue_t;
bool CTL::Thread::SafeExit = true;

void Thread_Integration::Start() {
	this->Flag = true;
	Thread_m = std::thread(&Thread_Integration::running, this);
}

void Thread_Integration::Stop() {
	this->Flag = false;
	while (this->Flag_m) {
		CTL::Thread::SleepMS(10);
	}
}

void Thread_Integration::Wait() {
	this->Flag = true;
	Thread_m = std::thread(&Thread_Integration::running, this);
	Thread_m.join();
}

bool Thread_Integration::Sign() const {
	return Flag;
}

void Thread_Integration::running() {
	this->Flag = true;
	this->Flag_m = true;
	this->Run();
	this->Flag = false;
	this->Flag_m = false;
}

Thread::Thread() {
	this->threadID = AssignID();
}

Thread::Thread(Thread &&other) noexcept{
	threadID = other.threadID;
	this->Thread_m = std::move(other.Thread_m);
	this->Flag = other.Flag;
	other.threadID = -1; // 标记原对象已失效
}

Thread::~Thread(){
	if (!Thread_m) {
		return;
	}
	if (Thread_m->joinable() && SafeExit){   // SafeExit 默认为 true
		Thread_m->join();                     // ← 当前线程 join 自己！
	}
	// unique_ptr会自动清理资源，无需手动delete
}

void Thread::Stop(){
	Flag = false;
	if (Thread_m && Thread_m->joinable()) {
		Thread_m->join();
	}
}

bool Thread::Sign() const
{
	return Flag;
}

long Thread::Sleep(const unsigned int us) {
	auto start = std::chrono::system_clock::now();
	if(us != 0) {
		std::this_thread::sleep_for(std::chrono::microseconds(us));
	}
	auto end = std::chrono::system_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	return duration.count();
}

long Thread::SleepMS(const unsigned int MS) {
#ifdef _WIN32
	const auto start = std::chrono::system_clock::now();
	::Sleep(MS);
	const auto end = std::chrono::system_clock::now();
	const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	return duration.count();
#else
	auto start = std::chrono::system_clock::now();
	if(MS != 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(MS));
	}
	auto end = std::chrono::system_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	return duration.count();
#endif
}

std::thread* Thread::GetThread() const {
	return Thread_m.get();
}

std::thread::id Thread::GetThreadId() const {
	if (Thread_m) {
		return Thread_m->get_id();
	}
	return {};
}

void Thread::SetSafeExit(const bool Flag) {
	SafeExit = Flag;
}

int Thread::getHardWare() {
	return std::thread::hardware_concurrency();
}

void Thread::YieLd() {
	std::this_thread::yield();
}

size_t Thread::getNowMS(){
	using namespace std::chrono;
	const auto now = high_resolution_clock::now();
	return duration_cast<milliseconds>(now.time_since_epoch()).count();
}

void Thread::setInfo(const std::string &info) {
	this->Info = info;
}

int Thread::AssignID() {
	std::unique_lock<std::mutex> lock(thread_mutex_t);
	static int currentID = 0;
	return currentID++;
}

void Thread::run() {
	Flag = true;
	try {
		const auto task_t = CC_thread_Queue_t.Poll();
		if (task_t) {
			const auto task = task_t.get();
			task->operator()();
		}
	}
	catch (CCException& e) {
		System::Println("Thread Run ThreadID: {}\nError{}\nThreadInfo: {}",
			this->threadID,e.what(),Info.c_str());
	}
	Flag = false;
}
