#include "ZzUtils.h"
#include "ZzLog.h"

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

ZZ_INIT_LOG("ZzUtils");

#include "ZzClock.h"

using namespace __zz_clock__;

namespace ZzUtils {
	FreeStack::FreeStack() {
	}

	FreeStack::~FreeStack() {
		if(! empty()) {
			LOGE("%s(%d): unexpected value, size()=%u", __FUNCTION__, __LINE__, size());
		}
	}

	void FreeStack::Flush() {
		while(! empty()) {
			top()();
			pop();
		}
	}

	void TestLoop(std::function<int ()> idle, int64_t dur_num, int64_t dur_den) {
		int err;

		int fd_stdin = 0; // stdin
		int64_t now = _clk();
		int64_t beg = now;
		int64_t nTick = now * dur_den / dur_num - 1;

		LOGW("Wait for test...");
		while(true) {
			fd_set readfds;
			FD_ZERO(&readfds);

			int fd_max = -1;
			if(fd_stdin > fd_max) fd_max = fd_stdin;
			FD_SET(fd_stdin, &readfds);

			struct timeval tval;

			int64_t nInterval = (nTick + 1) * dur_num / dur_den - now;
			if(nInterval < 4000) {
				tval.tv_sec  = 0;
				tval.tv_usec = 4000LL;
			} else {
				tval.tv_sec  = nInterval / dur_num;
				tval.tv_usec = nInterval % dur_num;
			}

			err = select(fd_max + 1, &readfds, NULL, NULL, &tval);
			if (err < 0) {
				LOGE("%s(%d): select() failed! err = %d", __FUNCTION__, __LINE__, err);
				break;
			}

			now = _clk();
			nTick++;

			if (FD_ISSET(fd_stdin, &readfds)) {
				int ch = getchar();

				if(ch == 'q')
					break;
			}

			if(err == 0) {
				err = idle();
				if(err) {
					break;
				}
			}
		}
		int64_t fini = _clk();
		LOGW("Test done. %.4fs", (fini - beg) / 1000000.0);
	}
}