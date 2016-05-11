#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <functional>
#include <stdexcept>

class ThreadPool {
	using Task = std::function<void()>;

private:
	std::vector<std::thread> pool;
	std::queue<Task> tasks;
	std::mutex m_task;
	std::condition_variable condition;
	std::atomic<bool> stop;

public:
	ThreadPool(size_t size = 4): stop(false) {
		size = size < 1 ? 1 : size;
		for (size_t i = 0; i < size; i++) {
			pool.emplace_back(&ThreadPool::schedual, this); // bind()
		}
	}

	~ThreadPool() {
		stop.store(true);
		condition.notify_all();
		for (std::thread& thread : pool) {
			thread.join();
		}
	}

	template<class F, class... Args>
	auto commit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
		if(stop.load()) {
            throw std::runtime_error("commit on stopped ThreadPool");
        }

		using ResType =  decltype(f(args...));
		auto task = std::make_shared<std::packaged_task<ResType()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		std::future<ResType> res = task->get_future();
		
		{
			std::lock_guard<std::mutex> lock(m_task);
			tasks.emplace([task](){ (*task)(); });
		}

		condition.notify_all();
		return res;
	}

private:
	void schedual() {
		while(true) {
			Task task;

			{
				std::unique_lock<std::mutex> lock(m_task);
				condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
				if (stop && tasks.empty()) {
					return;
				}
				task = std::move(tasks.front());
				tasks.pop();
				// lock.unlock();
			}
			
			task();
		}
	}
};


int main(int argc, char const *argv[]) {
	/* code */
	ThreadPool pool(4);
	std::vector<std::future<int>> results;

	for (int i = 0; i < 8; i++) {
		results.emplace_back(
			pool.commit([i] {
				// std::cout << "hello " << i << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(1));
				// std::cout << "world " << i << std::endl;
				return i * i;
			})
		);
	}

	for (auto &result: results) {
		std::cout << result.get() << ' ';
	}
	std::cout << std::endl;

	return 0;
}