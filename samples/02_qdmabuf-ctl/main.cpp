#include "ZzLog.h"
#include "ZzUtils.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "qdmabuf.h"

ZZ_INIT_LOG("02_qdmabuf-ctl")

namespace __02_qdmabuf_ctl__ {

	struct App {
		int argc;
		char **argv;

		App(int argc, char **argv);
		~App();

		int Run();
	};

	App::App(int argc, char **argv) : argc(argc), argv(argv) {
		LOGD("%s(%d):", __FUNCTION__, __LINE__);
	}

	App::~App() {
		LOGD("%s(%d):", __FUNCTION__, __LINE__);
	}

	int App::Run() {
		int err = 0;
		ZzUtils::FreeStack oFreeStack;

		int fd_qdmabuf;
		int fd_dma_heap;
		int fd_dma_buf;

		switch(1) { case 1:
			fd_qdmabuf = open("/dev/qdmabuf0", O_RDWR);
			if(fd_qdmabuf == -1) {
				err = errno;
				LOGE("%s(%d): open() failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}
			oFreeStack += [&]() {
				close(fd_qdmabuf);
			};

			{
				qdmabuf_alloc_args args;
				args.len = 4 * 1024 * 1024;
				args.type = QDMABUF_ALLOC_TYPE_CONTIG;
				args.fd_flags = O_RDWR | O_CLOEXEC;
				args.fd = 0;
				err = ioctl(fd_qdmabuf, QDMABUF_IOCTL_ALLOC, &args);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(QDMABUF_IOCTL_ALLOC) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				LOGD("args={.len=%d, .fd=%d}", args.len, args.fd);

				fd_dma_buf = args.fd;
			}
			oFreeStack += [&]() {
				close(fd_dma_buf);
			};

			{
				qdmabuf_info_args args;
				args.fd = fd_dma_buf;
				err = ioctl(fd_qdmabuf, QDMABUF_IOCTL_INFO, &args);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(QDMABUF_IOCTL_INFO) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}
			}

			ZzUtils::TestLoop([&]() -> int {
				return 0;
			}, 1000000LL, 60LL);
		}

		oFreeStack.Flush();

		return err;
	}
}

using namespace __02_qdmabuf_ctl__;

int main(int argc, char *argv[]) {
	LOGD("entering...");

	int err;
	{
		App app(argc, argv);
		err = app.Run();

		LOGD("leaving...");
	}

	return err;
}
