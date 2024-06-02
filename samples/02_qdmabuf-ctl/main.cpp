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
		int fd_dma_buf_dma_contig[4];
		int fd_dma_buf_dma_sg[4];
		int fd_dma_buf_vmalloc[4];
		int fd_dma_buf_sys_heap[4];

		switch(1) { case 1:
			fd_qdmabuf = open("/dev/qdmabuf", O_RDWR);
			if(fd_qdmabuf == -1) {
				err = errno;
				LOGE("%s(%d): open() failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}
			oFreeStack += [&]() {
				close(fd_qdmabuf);
			};

#if 0
			for(int i = 0;i < 4;i++) {
				qdmabuf_alloc_args args;
				args.len = 4 * 1024 * 1024;
				args.type = QDMABUF_TYPE_DMA_CONTIG;
				args.fd_flags = O_RDWR | O_CLOEXEC;
				args.dma_dir = QDMABUF_DMA_DIR_BIDIRECTIONAL;
				args.fd = 0;
				err = ioctl(fd_qdmabuf, QDMABUF_IOCTL_ALLOC, &args);
				if(err < 0) {
					LOGE("%s(%d): ioctl(QDMABUF_IOCTL_ALLOC) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				LOGD("args={.len=%d, .fd=%d}", args.len, args.fd);

				fd_dma_buf_dma_contig[i] = args.fd;
				oFreeStack += [&]() {
					close(fd_dma_buf_dma_contig[i]);
				};
			}

			if(err < 0)
				break;

			for(int i = 0;i < 4;i++) {
				qdmabuf_info_args args;
				args.fd = fd_dma_buf_dma_contig[i];
				err = ioctl(fd_qdmabuf, QDMABUF_IOCTL_INFO, &args);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(QDMABUF_IOCTL_INFO) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}
			}

			if(err < 0)
				break;
#endif

#if 0
			for(int i = 0;i < 4;i++) {
				qdmabuf_alloc_args args;
				args.len = 2 * 1024 * 1024;
				args.type = QDMABUF_TYPE_DMA_SG;
				args.fd_flags = O_RDWR | O_CLOEXEC;
				args.dma_dir = QDMABUF_DMA_DIR_BIDIRECTIONAL;
				args.fd = 0;
				err = ioctl(fd_qdmabuf, QDMABUF_IOCTL_ALLOC, &args);
				if(err < 0) {
					LOGE("%s(%d): ioctl(QDMABUF_IOCTL_ALLOC) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				LOGD("args={.len=%d, .fd=%d}", args.len, args.fd);

				fd_dma_buf_dma_sg[i] = args.fd;

				oFreeStack += [&, i]() {
					close(fd_dma_buf_dma_sg[i]);
				};
			}

			if(err < 0)
				break;

			for(int i = 0;i < 4;i++) {
				qdmabuf_info_args args;
				args.fd = fd_dma_buf_dma_sg[i];
				err = ioctl(fd_qdmabuf, QDMABUF_IOCTL_INFO, &args);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(QDMABUF_IOCTL_INFO) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}
			}

			if(err < 0)
				break;
#endif

#if 0
			for(int i = 0;i < 4;i++) {
				qdmabuf_alloc_args args;
				args.len = 16 * 1024 * 1024;
				args.type = QDMABUF_TYPE_VMALLOC;
				args.fd_flags = O_RDWR | O_CLOEXEC;
				args.dma_dir = QDMABUF_DMA_DIR_BIDIRECTIONAL;
				args.fd = 0;
				err = ioctl(fd_qdmabuf, QDMABUF_IOCTL_ALLOC, &args);
				if(err < 0) {
					LOGE("%s(%d): ioctl(QDMABUF_IOCTL_ALLOC) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				LOGD("args={.len=%d, .fd=%d}", args.len, args.fd);

				fd_dma_buf_vmalloc[i] = args.fd;
				oFreeStack += [&, i]() {
					close(fd_dma_buf_vmalloc[i]);
				};
			}

			if(err < 0)
				break;

			for(int i = 0;i < 4;i++) {
				qdmabuf_info_args args;
				args.fd = fd_dma_buf_vmalloc[i];
				err = ioctl(fd_qdmabuf, QDMABUF_IOCTL_INFO, &args);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(QDMABUF_IOCTL_INFO) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}
			}

			if(err < 0)
				break;
#endif

#if 0
			for(int i = 0;i < 4;i++) {
				qdmabuf_alloc_args args;
				args.len = 1 * 1024 * getpagesize();
				args.type = QDMABUF_TYPE_SYS_HEAP;
				args.fd_flags = O_RDWR | O_CLOEXEC;
				args.dma_dir = QDMABUF_DMA_DIR_BIDIRECTIONAL;
				args.fd = 0;
				err = ioctl(fd_qdmabuf, QDMABUF_IOCTL_ALLOC, &args);
				if(err < 0) {
					LOGE("%s(%d): ioctl(QDMABUF_IOCTL_ALLOC) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				LOGD("args={.len=%d, .fd=%d}", args.len, args.fd);

				fd_dma_buf_sys_heap[i] = args.fd;
				oFreeStack += [&]() {
					close(fd_dma_buf_sys_heap[i]);
				};
			}

			if(err < 0)
				break;

#if 0
			for(int i = 0;i < 4;i++) {
				qdmabuf_info_args args;
				args.fd = fd_dma_buf_sys_heap[i];
				err = ioctl(fd_qdmabuf, QDMABUF_IOCTL_INFO, &args);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(QDMABUF_IOCTL_INFO) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}
			}

			if(err < 0)
				break;
#endif
#endif

#if 0
			{
				qdmabuf_wq_args args;

				args.type = 0;
				args.value = (__u32)-1;
				LOGD("args={.type=%d, .value=%d}", (int)args.type, (int)args.value);

				err = ioctl(fd_qdmabuf, QDMABUF_IOCTL_WQ, &args);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(QDMABUF_IOCTL_WQ) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				LOGD("args={.type=%d, .value=%d}", (int)args.type, (int)args.value);
			}
#endif

#if 1
			{
				qdmabuf_wq_args args;

				args.type = 1;
				args.value = 123;
				LOGD("args={.type=%d, .value=%d}", (int)args.type, (int)args.value);

				err = ioctl(fd_qdmabuf, QDMABUF_IOCTL_WQ, &args);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(QDMABUF_IOCTL_WQ) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				LOGD("args={.type=%d, .value=%d}", (int)args.type, (int)args.value);
			}
#endif

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
