#include "ZzLog.h"
#include "ZzUtils.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/media.h>
#include <linux/version.h>
#include <sys/eventfd.h>

#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <deque>
#include <atomic>
#include <memory>

#include "qvio.h"

ZZ_INIT_LOG("03_qvio-ctl")

namespace __03_qvio_ctl__ {

	struct App {
		int argc;
		char **argv;

		ZzUtils::FreeStack oFreeStack;
		int nVidFd;
		int nVidCtrlFd;
		int nMemory;
		int nBufType;

		struct mmap_buffer {
			int nIndex;
			intptr_t pVirAddr[4];
			int nLength[4];
		};
		std::vector<mmap_buffer> oMbuffers;

		struct exp_buffer {
			int nFd[4];
		};
		std::vector<exp_buffer> oDmaBufs;

		App(int argc, char **argv);
		~App();

		int Run();

		void OpenVidRx();

		// user job handling
		void VidUserJobHandling();

		v4l2_format oCurrentFormat;
		int nDeferredTaskEvent;
		std::mutex oDeferredTasksMutex;
		std::deque<std::function<void ()> > oDeferredTasks; // defer tasks for done-ioctl

		void VidDeferred_Main();
		void AddDeferredTask(std::function<void ()> fn);
		void VidUserJob_S_FMT(const qvio_user_job& user_job, qvio_user_job_done& user_job_done);
		void VidUserJob_QUEUE_SETUP(const qvio_user_job& user_job, qvio_user_job_done& user_job_done);
		void VidUserJob_BUF_INIT(const qvio_user_job& user_job, qvio_user_job_done& user_job_done);
		void VidUserJob_BUF_CLEANUP(const qvio_user_job& user_job, qvio_user_job_done& user_job_done);
		void VidUserJob_START_STREAMING(const qvio_user_job& user_job, qvio_user_job_done& user_job_done);
		void VidUserJob_STOP_STREAMING(const qvio_user_job& user_job, qvio_user_job_done& user_job_done);
		void VidUserJob_BUF_DONE(const qvio_user_job& user_job, qvio_user_job_done& user_job_done);
		void VidUserJob_ERROR(const qvio_user_job& user_job, qvio_user_job_done& user_job_done);

		// video source
		int nVidSrcFd;
		std::atomic<bool> bVidSrcDone;
		std::shared_ptr<std::thread> pVidSrcThread;
		std::vector<mmap_buffer> oSrcMbuffers;
		std::mutex oSrcBufferQMutex;
		std::deque<v4l2_buffer> oSrcBufferQ; // defer tasks for done-ioctl
		void VidSrc_Main();
	};

	App::App(int argc, char **argv) : argc(argc), argv(argv) {
		// LOGD("%s(%d):", __FUNCTION__, __LINE__);
	}

	App::~App() {
		// LOGD("%s(%d):", __FUNCTION__, __LINE__);
	}

	int App::Run() {
		int err;

		srand((unsigned)time(NULL));

		switch(1) { case 1:
			nVidFd = -1;
			nVidCtrlFd = -1;
			nVidSrcFd = -1;

			nMemory = V4L2_MEMORY_MMAP;
			nBufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			ZzUtils::TestLoop([&](int ch) -> int {
				int err = 0;

				// LOGD("ch=%d", ch);

				switch(ch) {
				case 'r':
				case 'R':
					OpenVidRx();
					break;

				case 'd':
				case 'D':
					VidUserJobHandling();
					break;
				}

				return err;
			}, 1000000LL, 1LL);

			err = 0;
		}

		oFreeStack.Flush();

		return err;
	}

	void App::OpenVidRx() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			if(nVidFd != -1) {
				LOGE("%s(%d): unexpected value, nVidFd=%d", __FUNCTION__, __LINE__, nVidFd);
				break;
			}

			nVidFd = open("/dev/video2", O_RDWR | O_NONBLOCK);
			if(nVidFd == -1) {
				err = errno;
				LOGE("%s(%d): open() failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}
			oFreeStack += [&]() {
				int err;

				err = close(nVidFd);
				if(err) {
					err = errno;
					LOGE("%s(%d): close() failed, err=%d", __FUNCTION__, __LINE__, err);
				}
				nVidFd = -1;
			};

			LOGD("nVidFd=%d", nVidFd);
			nBufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			nVidCtrlFd = open("/dev/qvio0", O_RDWR);
			if(nVidCtrlFd == -1) {
				err = errno;
				LOGE("%s(%d): open() failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}
			oFreeStack += [&]() {
				int err;

				err = close(nVidCtrlFd);
				if(err) {
					err = errno;
					LOGE("%s(%d): close() failed, err=%d", __FUNCTION__, __LINE__, err);
				}
				nVidCtrlFd = -1;
			};

			LOGD("nVidCtrlFd=%d", nVidCtrlFd);
		}
	}

	void App::VidUserJobHandling() {
		int err;

		LOGD("%s(%d):+++", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			err = eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
			if(err < 0) {
				err = errno;
				LOGE("%s(%d): eventfd() failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}
			nDeferredTaskEvent = err;

			std::thread oDeferredThread(std::bind(&App::VidDeferred_Main, this));

			int fd_stdin = 0; // stdin

			while(true) {
				fd_set readfds;
				FD_ZERO(&readfds);

				int fd_max = -1;
				if(fd_stdin > fd_max) fd_max = fd_stdin;
				if(nVidCtrlFd > fd_max) fd_max = nVidCtrlFd;
				FD_SET(fd_stdin, &readfds);
				FD_SET(nVidCtrlFd, &readfds);

				err = select(fd_max + 1, &readfds, NULL, NULL, NULL);
				if (err < 0) {
					LOGE("%s(%d): select() failed! err = %d", __FUNCTION__, __LINE__, err);
					break;
				}

				if (FD_ISSET(fd_stdin, &readfds)) {
					int ch = getchar();

					if(ch == 'q')
						break;
				}

				if (FD_ISSET(nVidCtrlFd, &readfds)) {
					qvio_user_job user_job;
					qvio_user_job_done user_job_done;

					err = ioctl(nVidCtrlFd, QVID_IOC_USER_JOB_GET, &user_job);
					if(err) {
						err = errno;
						LOGE("%s(%d): ioctl(QVID_IOC_USER_JOB_GET) failed, err=%d", __FUNCTION__, __LINE__, err);
						break;
					}

					switch(user_job.id) {
					case QVIO_USER_JOB_ID_S_FMT:
						VidUserJob_S_FMT(user_job, user_job_done);
						break;

					case QVIO_USER_JOB_ID_QUEUE_SETUP:
						VidUserJob_QUEUE_SETUP(user_job, user_job_done);
						break;

					case QVIO_USER_JOB_ID_BUF_INIT:
						VidUserJob_BUF_INIT(user_job, user_job_done);
						break;

					case QVIO_USER_JOB_ID_BUF_CLEANUP:
						VidUserJob_BUF_CLEANUP(user_job, user_job_done);
						break;

					case QVIO_USER_JOB_ID_START_STREAMING:
						VidUserJob_START_STREAMING(user_job, user_job_done);
						break;

					case QVIO_USER_JOB_ID_STOP_STREAMING:
						VidUserJob_STOP_STREAMING(user_job, user_job_done);
						break;

					case QVIO_USER_JOB_ID_BUF_DONE:
						VidUserJob_BUF_DONE(user_job, user_job_done);
						break;

					default:
						VidUserJob_ERROR(user_job, user_job_done);
						break;
					}

					user_job_done.id = user_job.id;
					user_job_done.sequence = user_job.sequence;

#if 0 // DEBUG
					LOGD("user_job_done(%d, %d)", (int)user_job_done.id, user_job_done.sequence);
#endif

#if 1
					err = ioctl(nVidCtrlFd, QVID_IOC_USER_JOB_DONE, &user_job_done);
					if(err) {
						err = errno;
						LOGE("%s(%d): ioctl(QVID_IOC_USER_JOB_DONE) failed, err=%d", __FUNCTION__, __LINE__, err);
						break;
					}
#endif
				}
			}

			// notify oDeferredThread to stop
			int64_t inc = 1;
			err = write(nDeferredTaskEvent, &inc, sizeof(inc));
			if(err == -1) {
				err = errno;
				LOGE("%s(%d): write() failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}

			if(err != sizeof(inc)) {
				err = errno;
				LOGE("%s(%d): write() failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}

			oDeferredThread.join();
		}

		if(nDeferredTaskEvent != -1) {
			close(nDeferredTaskEvent);
			nDeferredTaskEvent = -1;
		}

		LOGD("%s(%d):---", __FUNCTION__, __LINE__);
	}

	void App::VidDeferred_Main() {
		int err;

		LOGD("+++%s", __FUNCTION__);

		while(true) {
			fd_set readfds;
			FD_ZERO(&readfds);

			int fd_max = -1;
			if(nDeferredTaskEvent > fd_max) fd_max = nDeferredTaskEvent;
			FD_SET(nDeferredTaskEvent, &readfds);

			err = select(fd_max + 1, &readfds, NULL, NULL, NULL);
			if (err < 0) {
				LOGE("%s(%d): select() failed! err = %d", __FUNCTION__, __LINE__, err);
				break;
			}

			if (FD_ISSET(nDeferredTaskEvent, &readfds)) {
				int64_t inc;
				err = read(nDeferredTaskEvent, &inc, sizeof(inc));
				if(err == -1) {
					err = errno;
					LOGE("%s(%d): read() failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				if(err != sizeof(inc)) {
					err = errno;
					LOGE("%s(%d): read() failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				std::unique_lock<std::mutex> lck (oDeferredTasksMutex, std::defer_lock);
				std::function<void ()> fn;

				lck.lock();
				switch(1) { case 1:
					if(oDeferredTasks.empty()) {
						LOGW("%s(%d): unexpected, oDeferredTasks.empty(), stopping...", __FUNCTION__, __LINE__);
						break;
					}

					fn = oDeferredTasks.front();
					oDeferredTasks.pop_front();
				}
				lck.unlock();

				if(! fn)
					break;

				fn();
			}
		}

		LOGD("---%s", __FUNCTION__);
	}

	void App::AddDeferredTask(std::function<void ()> fn) {
		int err;
		std::unique_lock<std::mutex> lck (oDeferredTasksMutex, std::defer_lock);

		lck.lock();

		switch(1) { case 1:
			oDeferredTasks.push_back(fn);

			// wake oDeferredTasks up
			int64_t inc = 1;
			err = write(nDeferredTaskEvent, &inc, sizeof(inc));
			if(err == -1) {
				err = errno;
				LOGE("%s(%d): write() failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}

			if(err != sizeof(inc)) {
				err = errno;
				LOGE("%s(%d): write() failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}
		}

		lck.unlock();
	}

	void App::VidUserJob_S_FMT(const qvio_user_job& user_job, qvio_user_job_done& user_job_done) {
		LOGD("%s(%d): type=%d %dx%d %d", __FUNCTION__, (int)user_job.sequence,
			(int)user_job.u.s_fmt.format.type,
			(int)user_job.u.s_fmt.format.fmt.pix.width,
			(int)user_job.u.s_fmt.format.fmt.pix.height,
			(int)user_job.u.s_fmt.format.fmt.pix.bytesperline);

		oCurrentFormat = user_job.u.s_fmt.format;
	}

	void App::VidUserJob_QUEUE_SETUP(const qvio_user_job& user_job, qvio_user_job_done& user_job_done) {
		int err;

		LOGD("%s(%d): num_buffers=%d", __FUNCTION__, (int)user_job.sequence, (int)user_job.u.queue_setup.num_buffers);

		int nNumBuffers = (int)user_job.u.queue_setup.num_buffers;
		switch(1) { case 1:
			oMbuffers.resize(nNumBuffers);

			for(int i = 0;i < nNumBuffers;i++) {
				mmap_buffer& buffer = oMbuffers[i];

				memset(&buffer, 0, sizeof(mmap_buffer));
			}
		}
	}

	void App::VidUserJob_BUF_INIT(const qvio_user_job& user_job, qvio_user_job_done& user_job_done) {
		int err;

		LOGD("%s(%d): index=%d", __FUNCTION__, (int)user_job.sequence, user_job.u.buf_init.index);

#if 1
		int nIndex = (int)user_job.u.buf_init.index;
		if(nIndex >= 0 && nIndex < oMbuffers.size()) {
			AddDeferredTask([&, nIndex]() {
				LOGD("VIDIOC_QUERYBUF(%d)...", nIndex);

				v4l2_buffer buffer;
				memset(&buffer, 0, sizeof(v4l2_buffer));
				buffer.index = nIndex;
				buffer.type = nBufType;
				buffer.memory = nMemory;
				err = ioctl(nVidFd, VIDIOC_QUERYBUF, &buffer);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(VIDIOC_QUERYBUF) failed, err=%d", __FUNCTION__, __LINE__, err);
					return;
				}

				LOGD("BUFFER[%d]: length=%d offset=%d",
					buffer.index, buffer.length, buffer.m.offset);

				mmap_buffer& mbuffer = oMbuffers[nIndex];
				memset(&mbuffer, 0, sizeof(mmap_buffer));

				mbuffer.nIndex = buffer.index;

				size_t nLength = (size_t)buffer.length;
				void* pVirAddr = mmap(NULL, nLength, PROT_READ | PROT_WRITE, MAP_SHARED,
					nVidFd, (off_t)buffer.m.offset);
				if(pVirAddr == MAP_FAILED) {
					LOGE("%s(%d): mmap() failed, err=%d", __FUNCTION__, __LINE__, err);
					return;
				}

				mbuffer.pVirAddr[0] = (intptr_t)pVirAddr;
				mbuffer.nLength[0] = buffer.length;

				LOGD("BUFFER[%d]: mmap [%p %d]",
					buffer.index, mbuffer.pVirAddr[0], mbuffer.nLength[0]);
			});
		}
#endif
	}

	void App::VidUserJob_BUF_CLEANUP(const qvio_user_job& user_job, qvio_user_job_done& user_job_done) {
		int err;

		LOGD("%s(%d): index=%d", __FUNCTION__, (int)user_job.sequence, user_job.u.buf_cleanup.index);

#if 1
		int nIndex = (int)user_job.u.buf_init.index;
		if(nIndex >= 0 && nIndex < oMbuffers.size()) {
			AddDeferredTask([&, nIndex]() {
				LOGD("munmap BUFFER(%d)...", nIndex);

				mmap_buffer& mbuffer = oMbuffers[nIndex];

				for(int p = 0;p < 4;p++) {
					if(mbuffer.pVirAddr[p] && mbuffer.nLength[p]) {
						LOGD("BUFFER[%d]: munmap p=%d [%p %d]",
							nIndex, p, mbuffer.pVirAddr[p], mbuffer.nLength[p]);

						err = munmap((void*)mbuffer.pVirAddr[p], mbuffer.nLength[p]);
						if(err) {
							err = errno;
							LOGE("%s(%d): munmap() failed, err=%d", __FUNCTION__, __LINE__, err);
						}
					}
				}

				memset(&mbuffer, 0, sizeof(mmap_buffer));
			});
		}
#endif
	}

	void App::VidUserJob_START_STREAMING(const qvio_user_job& user_job, qvio_user_job_done& user_job_done) {
		LOGD("%s(%d): ", __FUNCTION__, (int)user_job.sequence);

		bVidSrcDone.store(false);
		pVidSrcThread.reset(new std::thread(std::bind(&App::VidSrc_Main, this)));
	}

	void App::VidUserJob_STOP_STREAMING(const qvio_user_job& user_job, qvio_user_job_done& user_job_done) {
		LOGD("%s(%d): ", __FUNCTION__, (int)user_job.sequence);

		bVidSrcDone.store(true);
		if(pVidSrcThread) {
			pVidSrcThread->join();
			pVidSrcThread.reset();
		}
	}

	void App::VidUserJob_BUF_DONE(const qvio_user_job& user_job, qvio_user_job_done& user_job_done) {
		LOGD("%s(%d): index=%d", __FUNCTION__, (int)user_job.sequence, user_job.u.buf_done.index);

		int nIndex = (int)user_job.u.buf_done.index;
		if(nIndex >= 0 && nIndex < oMbuffers.size()) switch(1) { case 1:
			mmap_buffer& mbuffer = oMbuffers[nIndex];

			LOGD("BUFFER[%d]: [%p %d]",
				nIndex, mbuffer.pVirAddr[0], mbuffer.nLength[0]);

			std::unique_lock<std::mutex> lck (oSrcBufferQMutex, std::defer_lock);
			v4l2_buffer buffer;

			lck.lock();
			if(oSrcBufferQ.empty()) {
				buffer.index = -1;
			} else {
			 	buffer = oSrcBufferQ.front();
			 	oSrcBufferQ.pop_front();
			}
			lck.unlock();

			LOGD("SRC-BUFFER[%d]:...", buffer.index);
		}
	}

	void App::VidUserJob_ERROR(const qvio_user_job& user_job, qvio_user_job_done& user_job_done) {
		LOGD("%s(%d): id=%d", __FUNCTION__, (int)user_job.sequence, (int)user_job.id);
	}

	void App::VidSrc_Main() {
		int err;
		int nWidth = 1920;
		int nHeight = 1080;
		ZzUtils::FreeStack _FreeStack;

		LOGD("+++%s", __FUNCTION__);

		switch(1) { case 1:
			nVidSrcFd = open("/dev/video0", O_RDWR | O_NONBLOCK);
			if(nVidSrcFd == -1) {
				err = errno;
				LOGE("%s(%d): open() failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}
			_FreeStack += [&]() {
				int err;

				err = close(nVidSrcFd);
				if(err) {
					err = errno;
					LOGE("%s(%d): close() failed, err=%d", __FUNCTION__, __LINE__, err);
				}
			};

			LOGD("nVidSrcFd=%d", nVidSrcFd);

			v4l2_format format;
			memset(&format, 0, sizeof(v4l2_format));
			format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			format.fmt.pix.width = nWidth;
			format.fmt.pix.height = nHeight;
			format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
			format.fmt.pix.field = V4L2_FIELD_NONE;
			format.fmt.pix.bytesperline = ZZ_ALIGN(nWidth, 0x1000) * 2;
			format.fmt.pix.sizeimage = format.fmt.pix.bytesperline * nHeight;

			err = ioctl(nVidSrcFd, VIDIOC_S_FMT, &format);
			if(err) {
				err = errno;
				LOGE("%s(%d): ioctl(VIDIOC_S_FMT) failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}

			LOGD("-param: %d %d",
				(int)format.fmt.pix.sizeimage,
				(int)format.fmt.pix.bytesperline);

			struct v4l2_requestbuffers requestBuffers;
			memset(&requestBuffers, 0, sizeof(struct v4l2_requestbuffers));
			requestBuffers.count = 4;
			requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			requestBuffers.memory = V4L2_MEMORY_MMAP;
			err = ioctl(nVidSrcFd, VIDIOC_REQBUFS, &requestBuffers);
			if(err) {
				err = errno;
				LOGE("%s(%d): ioctl(VIDIOC_REQBUFS) failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}

			oSrcMbuffers.resize(requestBuffers.count);
			_FreeStack += [&]() {
				oSrcMbuffers.clear();
			};

			LOGD("SRC-BUFFER: requestBuffers.count=%d", (int)requestBuffers.count);

			for(int i = 0;i < requestBuffers.count;i++) {
				int nIndex = i;

				v4l2_buffer buffer;
				memset(&buffer, 0, sizeof(v4l2_buffer));

				buffer.index = nIndex;
				buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buffer.memory = V4L2_MEMORY_MMAP;
				err = ioctl(nVidSrcFd, VIDIOC_QUERYBUF, &buffer);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(VIDIOC_QUERYBUF) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				LOGD("SRC-BUFFER[%d]: length=%d offset=%d",
					buffer.index, buffer.length, buffer.m.offset);

				mmap_buffer& mbuffer = oSrcMbuffers[nIndex];
				memset(&mbuffer, 0, sizeof(mmap_buffer));

				mbuffer.nIndex = buffer.index;

				size_t nLength = (size_t)buffer.length;
				void* pVirAddr = mmap(NULL, nLength, PROT_READ | PROT_WRITE, MAP_SHARED,
					nVidSrcFd, (off_t)buffer.m.offset);
				if(pVirAddr == MAP_FAILED) {
					LOGE("%s(%d): mmap() failed, err=%d", __FUNCTION__, __LINE__, err);
					return;
				}
				_FreeStack += [&, nIndex]() {
					mmap_buffer& mbuffer = oSrcMbuffers[nIndex];

					for(int p = 0;p < 4;p++) {
						if(mbuffer.pVirAddr[p] && mbuffer.nLength[p]) {
							LOGD("BUFFER[%d]: munmap p=%d [%p %d]",
								nIndex, p, mbuffer.pVirAddr[p], mbuffer.nLength[p]);

							err = munmap((void*)mbuffer.pVirAddr[p], mbuffer.nLength[p]);
							if(err) {
								err = errno;
								LOGE("%s(%d): munmap() failed, err=%d", __FUNCTION__, __LINE__, err);
							}
						}
					}

					memset(&mbuffer, 0, sizeof(mmap_buffer));
				};

				memset(&mbuffer, 0, sizeof(mmap_buffer));

				mbuffer.pVirAddr[0] = (intptr_t)pVirAddr;
				mbuffer.nLength[0] = buffer.length;

				LOGD("SRC-BUFFER[%d]: mmap [%p %d]",
					buffer.index, mbuffer.pVirAddr[0], mbuffer.nLength[0]);

				buffer.flags = 0;
				buffer.index = nIndex;
				buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buffer.memory = V4L2_MEMORY_MMAP;
				err = ioctl(nVidSrcFd, VIDIOC_QBUF, &buffer);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(VIDIOC_QBUF) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}
			}

			enum v4l2_buf_type buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			err = ioctl(nVidSrcFd, VIDIOC_STREAMON, &buf_type);
			if(err) {
				LOGE("%s(%d): ioctl(VIDIOC_STREAMON) failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}
			_FreeStack += [&]() {
				int err;

				enum v4l2_buf_type buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				err = ioctl(nVidSrcFd, VIDIOC_STREAMOFF, &buf_type);
				if(err) {
					LOGE("%s(%d): ioctl(VIDIOC_STREAMOFF) failed, err=%d", __FUNCTION__, __LINE__, err);
				}
			};

			while(! bVidSrcDone.load()) {
				fd_set readfds;
				FD_ZERO(&readfds);

				int fd_max = -1;
				if(nVidSrcFd > fd_max) fd_max = nVidSrcFd;
				FD_SET(nVidSrcFd, &readfds);

				err = select(fd_max + 1, &readfds, NULL, NULL, NULL);
				if (err < 0) {
					LOGE("%s(%d): select() failed! err = %d", __FUNCTION__, __LINE__, err);
					break;
				}

				if (FD_ISSET(nVidSrcFd, &readfds)) {
					v4l2_buffer buffer;
					memset(&buffer, 0, sizeof(v4l2_buffer));
					buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					buffer.memory = V4L2_MEMORY_MMAP;
					err = ioctl(nVidSrcFd, VIDIOC_DQBUF, &buffer);
					if(err) {
						err = errno;
						LOGE("%s(%d): ioctl(VIDIOC_DQBUF) failed, err=%d", __FUNCTION__, __LINE__, err);
						break;
					}

					std::unique_lock<std::mutex> lck (oSrcBufferQMutex, std::defer_lock);

					lck.lock();
					oSrcBufferQ.push_back(buffer);
					lck.unlock();

					err = ioctl(nVidCtrlFd, QVID_IOC_BUF_DONE);
					if(err) {
						err = errno;
						LOGE("%s(%d): ioctl(QVID_IOC_BUF_DONE) failed, err=%d", __FUNCTION__, __LINE__, err);
						break;
					}

					buffer.flags = 0;
					err = ioctl(nVidSrcFd, VIDIOC_QBUF, &buffer);
					if(err) {
						err = errno;
						LOGE("%s(%d): ioctl(VIDIOC_QBUF) failed, err=%d", __FUNCTION__, __LINE__, err);
						break;
					}
				}
			}
		}

		_FreeStack.Flush();

		LOGD("---%s", __FUNCTION__);
	}
}

using namespace __03_qvio_ctl__;

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
