#ifndef __ZZ_UTILS_H__
#define __ZZ_UTILS_H__

#include <cstdint>
#include <stack>
#include <functional>

#define container_of(ptr, type, member) ({ \
	void* __mptr = (void*)(ptr); \
	((type *)((char *)__mptr - offsetof(type, member))); \
})

#define ZZ_CONCAT_I(N, S) N ## S
#define ZZ_CONCAT(N, S) ZZ_CONCAT_I(N, S)
#define ZZ_GUARD_NAME ZZ_CONCAT(__GUARD, __LINE__)

namespace ZzUtils {
	struct FreeStack : protected std::stack<std::function<void ()> > {
		typedef FreeStack self_t;
		typedef std::stack<std::function<void ()> > parent_t;

		FreeStack();
		~FreeStack();

		template<class FUNC>
		self_t& operator +=(const FUNC& func) {
			push(func);

			return *this;
		}

		void Flush();
	};

	void TestLoop(std::function<int ()> idle, int64_t dur_num = 1000000LL, int64_t dur_den = 60LL);
}

#endif // __ZZ_UTILS_H__
