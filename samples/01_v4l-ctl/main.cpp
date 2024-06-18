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
#include <vector>

#include "qvio.h"

ZZ_INIT_LOG("01_v4l-ctl")

namespace __01_v4l_ctl__ {

	struct App {
		int argc;
		char **argv;

		ZzUtils::FreeStack oFreeStack;
		int nVidFd;
		int nVidCtrlFd;
		int nMemory;
		int nBufType;
		int nPixelFormat;
		int nWidth;
		int nHeight;
		int nHorStride;
		int nVerStride;
		int nBuffers;

		struct mmap_buffer {
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
		void OpenVidTx();
		void VidSFmt();
		void VidReqBufs();
		void VidQueryBufs();
		void VidMmap();
		void MmapWriteTest();
		void MmapReadTest();
		void VidExpBufs();
		void VidStreamOn();
		void VidStreamOff();
		void VidQbufs();
		void VidWaitForBuffer();
		void VidBufDone();
		void VidWaitForUserJob();
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
			nMemory = V4L2_MEMORY_MMAP;
			nBufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
			nPixelFormat = V4L2_PIX_FMT_NV12;
			nWidth = 4096;
			nHeight = 2160;
			nHorStride = ZZ_ALIGN(nWidth, 0x1000);
			nVerStride = ZZ_ALIGN(nHeight, 0x1000);
			nBuffers = 4;

			LOGD("param: %d %d %d %d %d", nMemory, nBufType, nWidth, nHeight,
				nHorStride, nVerStride, nBuffers);

			ZzUtils::TestLoop([&](int ch) -> int {
				int err = 0;

				// LOGD("ch=%d", ch);

				switch(ch) {
				case 'r':
				case 'R':
					OpenVidRx();
					break;

				case 't':
				case 'T':
					OpenVidTx();
					break;

				case '1':
					VidSFmt();
					break;

				case '2':
					VidReqBufs();
					break;

				case '3':
					VidQueryBufs();
					break;

				case '4':
					VidMmap();
					break;

				case '5':
					VidExpBufs();
					break;

				case '6':
					MmapWriteTest();
					break;

				case '7':
					MmapReadTest();
					break;

				case '8':
					VidStreamOn();
					break;

				case '9':
					VidStreamOff();
					break;

				case '0':
					VidQbufs();
					break;

				case 'a':
				case 'A':
					VidWaitForBuffer();
					break;

				case 'b':
				case 'B':
					VidBufDone();
					break;

				case 'c':
				case 'C':
					VidWaitForUserJob();
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
			nBufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

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

	void App::OpenVidTx() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			if(nVidFd != -1) {
				LOGE("%s(%d): unexpected value, nVidFd=%d", __FUNCTION__, __LINE__, nVidFd);
				break;
			}

			nVidFd = open("/dev/video3", O_RDWR | O_NONBLOCK);
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
			nBufType = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

			nVidCtrlFd = open("/dev/qvio1", O_RDWR);
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

	void App::VidSFmt() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			v4l2_format format;
			memset(&format, 0, sizeof(v4l2_format));
			format.type = nBufType;
			format.fmt.pix_mp.width = nWidth;
			format.fmt.pix_mp.height = nHeight;
			format.fmt.pix_mp.pixelformat = nPixelFormat;
			format.fmt.pix_mp.field = V4L2_FIELD_NONE;
			format.fmt.pix_mp.num_planes = 2;
			format.fmt.pix_mp.plane_fmt[0].sizeimage = nHorStride * nHeight;
			format.fmt.pix_mp.plane_fmt[0].bytesperline = nHorStride;
			format.fmt.pix_mp.plane_fmt[1].sizeimage = nHorStride * nHeight / 2;
			format.fmt.pix_mp.plane_fmt[1].bytesperline = nHorStride;

			LOGD("+param: %d %d %d %d %d", (int)format.fmt.pix_mp.num_planes,
				(int)format.fmt.pix_mp.plane_fmt[0].sizeimage,
				(int)format.fmt.pix_mp.plane_fmt[0].bytesperline,
				(int)format.fmt.pix_mp.plane_fmt[1].sizeimage,
				(int)format.fmt.pix_mp.plane_fmt[1].bytesperline);

			err = ioctl(nVidFd, VIDIOC_S_FMT, &format);
			if(err) {
				err = errno;
				LOGE("%s(%d): ioctl(VIDIOC_S_FMT) failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}

			LOGD("-param: %d %d %d %d %d", (int)format.fmt.pix_mp.num_planes,
				(int)format.fmt.pix_mp.plane_fmt[0].sizeimage,
				(int)format.fmt.pix_mp.plane_fmt[0].bytesperline,
				(int)format.fmt.pix_mp.plane_fmt[1].sizeimage,
				(int)format.fmt.pix_mp.plane_fmt[1].bytesperline);
		}
	}

	void App::VidReqBufs() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			struct v4l2_requestbuffers requestBuffers;
			memset(&requestBuffers, 0, sizeof(struct v4l2_requestbuffers));
			requestBuffers.count = nBuffers;
			requestBuffers.type = nBufType;
			requestBuffers.memory = nMemory;
			err = ioctl(nVidFd, VIDIOC_REQBUFS, &requestBuffers);
			if(err) {
				err = errno;
				LOGE("%s(%d): ioctl(VIDIOC_REQBUFS) failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}

			LOGD("param: %d", requestBuffers.count);
		}
	}

	void App::VidQueryBufs() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			for(int i = 0;i < nBuffers;i++) {
				v4l2_buffer buffer;
				v4l2_plane planes[2];
				memset(&buffer, 0, sizeof(v4l2_buffer));
				buffer.index = i;
				buffer.type = nBufType;
				buffer.memory = nMemory;
				buffer.m.planes = planes;
				buffer.length = 2;
				err = ioctl(nVidFd, VIDIOC_QUERYBUF, &buffer);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(VIDIOC_QUERYBUF) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				LOGD("BUFFER[%d]: length=%d [%d %d] [%d %d]", buffer.index, buffer.length,
					(int)buffer.m.planes[0].length, (int)buffer.m.planes[1].length,
					(int)buffer.m.planes[0].m.mem_offset, (int)buffer.m.planes[1].m.mem_offset);
			}
		}
	}

	void App::VidMmap() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			if(! oMbuffers.empty()) {
				LOGE("%s(%d): unexpected, oMbuffers.size()=%d", __FUNCTION__, __LINE__, (int)oMbuffers.size());
				break;
			}

			for(int i = 0;i < nBuffers;i++) {
				v4l2_buffer buffer;
				v4l2_plane planes[2];
				memset(&buffer, 0, sizeof(v4l2_buffer));
				buffer.index = i;
				buffer.type = nBufType;
				buffer.memory = nMemory;
				buffer.m.planes = planes;
				buffer.length = 2;
				err = ioctl(nVidFd, VIDIOC_QUERYBUF, &buffer);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(VIDIOC_QUERYBUF) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				mmap_buffer mbuffer;
				memset(&mbuffer, 0, sizeof(mmap_buffer));
				for(int p = 0;p < 2;p++) {
					size_t nLength = (size_t)buffer.m.planes[p].length;
					void* pVirAddr = mmap(NULL, nLength, PROT_READ | PROT_WRITE, MAP_SHARED,
						nVidFd, (off_t)buffer.m.planes[p].m.mem_offset);
					if(pVirAddr == MAP_FAILED) {
						LOGE("%s(%d): mmap() failed, err=%d", __FUNCTION__, __LINE__, err);
						break;
					}
					oFreeStack += [pVirAddr, nLength]() {
						int err;

						err = munmap(pVirAddr, nLength);
						if(err) {
							err = errno;
							LOGE("%s(%d): munmap() failed, err=%d", __FUNCTION__, __LINE__, err);
						}
					};

					mbuffer.pVirAddr[p] = (intptr_t)pVirAddr;
					mbuffer.nLength[p] = buffer.m.planes[p].length;
				}

				oMbuffers.push_back(mbuffer);

				LOGD("BUFFER[%d]: length=%d [%d %d] [%d %d] [%p %p]", buffer.index, buffer.length,
					(int)buffer.m.planes[0].length, (int)buffer.m.planes[1].length,
					(int)buffer.m.planes[0].m.mem_offset, (int)buffer.m.planes[1].m.mem_offset,
					mbuffer.pVirAddr[0], mbuffer.pVirAddr[1]);
			}
		}
	}

	void App::MmapWriteTest() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			if(oMbuffers.empty()) {
				LOGE("%s(%d): unexpected, oMbuffers.size()=%d", __FUNCTION__, __LINE__, (int)oMbuffers.size());
				break;
			}

			mmap_buffer mbuffer = oMbuffers[0];
			int nVal0 = rand() % 256;
			int nVal1 = rand() % 256;

			LOGD("param: [%d %d] [%p %p] [%d %d]",
				nVal0, nVal1, mbuffer.pVirAddr[0], mbuffer.pVirAddr[1],
				mbuffer.nLength[0], mbuffer.nLength[1]);

			memset((void*)mbuffer.pVirAddr[0], nVal0, mbuffer.nLength[0]);
			memset((void*)mbuffer.pVirAddr[1], nVal1, mbuffer.nLength[1]);
		}
	}

	void App::MmapReadTest() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			if(oMbuffers.empty()) {
				LOGE("%s(%d): unexpected, oMbuffers.size()=%d", __FUNCTION__, __LINE__, (int)oMbuffers.size());
				break;
			}

			mmap_buffer mbuffer = oMbuffers[0];

			uint8_t* pVal0 = (uint8_t*)mbuffer.pVirAddr[0];
			uint8_t* pVal1 = (uint8_t*)mbuffer.pVirAddr[1];
			int nVal0 = (int)pVal0[mbuffer.nLength[0] / 2];
			int nVal1 = (int)pVal1[mbuffer.nLength[1] / 2];

			LOGD("param: [%d %d] [%p %p] [%d %d]",
				nVal0, nVal1, mbuffer.pVirAddr[0], mbuffer.pVirAddr[1],
				mbuffer.nLength[0], mbuffer.nLength[1]);
		}
	}

	void App::VidExpBufs() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			if(! oDmaBufs.empty()) {
				LOGE("%s(%d): unexpected, oDmaBufs.size()=%d", __FUNCTION__, __LINE__, (int)oDmaBufs.size());
				break;
			}

			for(int i = 0;i < nBuffers;i++) {
				v4l2_buffer buffer;
				v4l2_plane planes[2];
				memset(&buffer, 0, sizeof(v4l2_buffer));
				buffer.index = i;
				buffer.type = nBufType;
				buffer.memory = nMemory;
				buffer.m.planes = planes;
				buffer.length = 2;
				err = ioctl(nVidFd, VIDIOC_QUERYBUF, &buffer);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(VIDIOC_QUERYBUF) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				exp_buffer ebuffer;
				memset(&ebuffer, 0, sizeof(exp_buffer));
				for(int p = 0;p < 2;p++) {
					struct v4l2_exportbuffer expbuf;

					memset(&expbuf, 0, sizeof(expbuf));
					expbuf.type = nBufType;
					expbuf.index = i;
					expbuf.plane = p;
					err = ioctl(nVidFd, VIDIOC_EXPBUF, &expbuf);
					if(err) {
						err = errno;
						LOGE("%s(%d): ioctl(VIDIOC_QUERYBUF) failed, err=%d", __FUNCTION__, __LINE__, err);
						break;
					}
					oFreeStack += [expbuf]() {
						int err;

						err = close(expbuf.fd);
						if(err) {
							err = errno;
							LOGE("%s(%d): close() failed, err=%d", __FUNCTION__, __LINE__, err);
						}
					};

					ebuffer.nFd[p] = expbuf.fd;
				}

				oDmaBufs.push_back(ebuffer);

				LOGD("BUFFER[%d]: length=%d [%d %d] [%d %d]", buffer.index, buffer.length,
					(int)buffer.m.planes[0].length, (int)buffer.m.planes[1].length,
					ebuffer.nFd[0], ebuffer.nFd[1]);
			}
		}
	}

	void App::VidStreamOn() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			enum v4l2_buf_type buf_type = (v4l2_buf_type)nBufType;
			err = ioctl(nVidFd, VIDIOC_STREAMON, &buf_type);
			if(err) {
				LOGE("%s(%d): ioctl(VIDIOC_STREAMON) failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}
		}
	}

	void App::VidStreamOff() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			enum v4l2_buf_type buf_type = (v4l2_buf_type)nBufType;
			err = ioctl(nVidFd, VIDIOC_STREAMOFF, &buf_type);
			if(err) {
				LOGE("%s(%d): ioctl(VIDIOC_STREAMOFF) failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}
		}
	}

	void App::VidQbufs() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			for(int i = 0;i < nBuffers;i++) {
				v4l2_buffer buffer;
				v4l2_plane planes[2];
				memset(&buffer, 0, sizeof(v4l2_buffer));
				buffer.index = i;
				buffer.type = nBufType;
				buffer.memory = nMemory;
				buffer.m.planes = planes;
				buffer.length = 2;
				err = ioctl(nVidFd, VIDIOC_QUERYBUF, &buffer);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(VIDIOC_QUERYBUF) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				err = ioctl(nVidFd, VIDIOC_QBUF, &buffer);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(VIDIOC_QBUF) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}
			}
		}
	}

	void App::VidWaitForBuffer() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		LOGW("Wait for test...");
		while(true) {
			fd_set readfds;
			FD_ZERO(&readfds);

			int fd_max = -1;
			if(nVidFd > fd_max) fd_max = nVidFd;
			FD_SET(nVidFd, &readfds);

			err = select(fd_max + 1, &readfds, NULL, NULL, NULL);
			if (err < 0) {
				LOGE("%s(%d): select() failed! err = %d", __FUNCTION__, __LINE__, err);
				break;
			}

			if (FD_ISSET(nVidFd, &readfds)) {
				v4l2_buffer buffer;
				v4l2_plane planes[2];

				memset(&buffer, 0, sizeof(v4l2_buffer));
				buffer.type = nBufType;
				buffer.memory = nMemory;
				buffer.m.planes = planes;
				buffer.length = 2;
				err = ioctl(nVidFd, VIDIOC_DQBUF, &buffer);
				if(err) {
					LOGE("%s(%d): ioctl(VIDIOC_DQBUF) failed! err = %d", __FUNCTION__, __LINE__, err);
					break;
				}

				LOGD("VIDIOC_DQBUF, buffer.index=%d", buffer.index);

				buffer.flags = 0;
				err = ioctl(nVidFd, VIDIOC_QBUF, &buffer);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(VIDIOC_QBUF) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				break;
			}
		}
	}

	void App::VidBufDone() {
		int err;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			err = ioctl(nVidCtrlFd, QVID_IOC_BUF_DONE);
			if(err) {
				err = errno;
				LOGE("%s(%d): ioctl(QVID_IOC_BUF_DONE) failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}
		}
	}

	void App::VidWaitForUserJob() {
		int err;
		qvio_user_job_args user_job_args;
		qvio_user_job_done_args user_job_done_args;
		qvio_user_job* pCurrentUserJob;

		LOGD("%s(%d):...", __FUNCTION__, __LINE__);

		switch(1) { case 1:
			err = ioctl(nVidCtrlFd, QVID_IOC_USER_JOB, &user_job_args);
			if(err) {
				err = errno;
				LOGE("%s(%d): ioctl(QVID_IOC_USER_JOB) failed, err=%d", __FUNCTION__, __LINE__, err);
				break;
			}

			pCurrentUserJob = &user_job_args.user_job;

			while(true) {
				switch(pCurrentUserJob->id) {
				case QVIO_USER_JOB_ID_S_FMT:
					LOGD("QVIO_USER_JOB_ID_S_FMT(%d): type=%d %dx%d", (int)pCurrentUserJob->sequence,
						(int)pCurrentUserJob->u.s_fmt.format.type,
						(int)pCurrentUserJob->u.s_fmt.format.fmt.pix_mp.width,
						(int)pCurrentUserJob->u.s_fmt.format.fmt.pix_mp.height,
						(int)pCurrentUserJob->u.s_fmt.format.fmt.pix_mp.num_planes);
					break;

				case QVIO_USER_JOB_ID_QUEUE_SETUP:
					LOGD("QVIO_USER_JOB_ID_QUEUE_SETUP(%d): %d", (int)pCurrentUserJob->sequence,
						(int)pCurrentUserJob->u.queue_setup.num_buffers);
					break;

				case QVIO_USER_JOB_ID_BUF_INIT:
					LOGD("QVIO_USER_JOB_ID_BUF_INIT(%d): %d", (int)pCurrentUserJob->sequence,
						pCurrentUserJob->u.buf_init.index);
					break;

				case QVIO_USER_JOB_ID_BUF_CLEANUP:
					LOGD("QVIO_USER_JOB_ID_BUF_CLEANUP(%d): %d", (int)pCurrentUserJob->sequence,
						pCurrentUserJob->u.buf_cleanup.index);
					break;

				case QVIO_USER_JOB_ID_START_STREAMING:
					LOGD("QVIO_USER_JOB_ID_START_STREAMING(%d)", (int)pCurrentUserJob->sequence);
					break;

				case QVIO_USER_JOB_ID_STOP_STREAMING:
					LOGD("QVIO_USER_JOB_ID_STOP_STREAMING(%d)", (int)pCurrentUserJob->sequence);
					break;

				case QVIO_USER_JOB_ID_BUF_DONE:
					LOGD("QVIO_USER_JOB_ID_BUF_DONE(%d): %d", (int)pCurrentUserJob->sequence,
						pCurrentUserJob->u.buf_done.index);
					break;

				default:
					LOGW("pCurrentUserJob: %d, %d", (int)pCurrentUserJob->id, (int)pCurrentUserJob->sequence);
					break;
				}

				user_job_done_args.user_job_done.id = pCurrentUserJob->id;
				user_job_done_args.user_job_done.sequence = pCurrentUserJob->sequence;

				err = ioctl(nVidCtrlFd, QVID_IOC_USER_JOB_DONE, &user_job_done_args);
				if(err) {
					err = errno;
					LOGE("%s(%d): ioctl(QVID_IOC_USER_JOB_DONE) failed, err=%d", __FUNCTION__, __LINE__, err);
					break;
				}

				pCurrentUserJob = &user_job_done_args.next_user_job;
			}
		}
	}
}

using namespace __01_v4l_ctl__;

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
