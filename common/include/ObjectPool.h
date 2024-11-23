#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

namespace flaw {
	template <typename T>
	class ObjectPool {
	public:
		ObjectPool(int size) {
			_pool.resize(size);
			_isUsed.resize(size, false);
			_freeIndices.reserve(size);
			for (int i = 0; i < size; ++i) {
				_freeIndices.push_back(i);
			}
		}

		T* Get() {
			if (_freeIndices.empty()) {
				return nullptr;
			}
			const int index = _freeIndices.back();
			_freeIndices.pop_back();
			_isUsed[index] = true;
			return &_pool[index];
		}

		void Release(T* obj) {
			const int index = obj - &_pool[0];
			_isUsed[index] = false;
			_freeIndices.push_back(index);
		}

		bool IsUsed(T* obj) {
			const int index = obj - &_pool[0];
			return _isUsed[index];
		}

		int GetSize() const {
			return _pool.size();
		}

		int GetFreeSize() const {
			return _freeIndices.size();
		}

		int GetUsedSize() const {
			return _pool.size() - _freeIndices.size();
		}

	private:
		std::vector<T> _pool;
		std::vector<bool> _isUsed;
		std::vector<int> _freeIndices;
	};
}

#endif
