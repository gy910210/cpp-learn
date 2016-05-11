#include <iostream>
#include <memory>
#include <vector>
#include <functional>

template <class T>
class SimpleObjectPool {
public:
	// using DeleterType = std::function<void(T*)>;

	void add(std::shared_ptr<T> t) {
		pool_.push_back(std::move(t));
	}

	std::shared_ptr<T> get() {
		if (pool_.empty()) {
			throw std::logic_error("no more object");
		}

		std::shared_ptr<T> ptr = pool_.back();
		auto p = std::shared_ptr<T>(new T(std::move(*ptr.get())), [this](T* t) {
			std::cout << "deleter" << std::endl;
			this->pool_.push_back(std::shared_ptr<T>(t));
		});

		pool_.pop_back();
		return p;
	}

	bool empty() const {
		return pool_.empty();
	}

	size_t size() const {
		return pool_.size();
	}

private:
	std::vector<std::shared_ptr<T>> pool_;
};

class A {
public:
	A(){}
};

int main(int argc, char const *argv[]) {
	/* code */
	SimpleObjectPool<A> p;
	p.add(std::shared_ptr<A>(new A()));
	p.add(std::shared_ptr<A>(new A()));

	std::shared_ptr<A> aa;
	std::shared_ptr<A> bb;

	{
		aa = p.get();
		p.get();
	}

	{
		// bb = p.get();
		p.get();
	}

	std::cout << p.size() << std::endl;
	return 0;
}