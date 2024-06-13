#include "ZzLog.h"
#include "ZzUtils.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <linux/media.h>
#include <linux/version.h>
#include <vector>

ZZ_INIT_LOG("01_v4l-ctl")

namespace __01_v4l_ctl__ {

	struct App {
		int argc;
		char **argv;

		ZzUtils::FreeStack oFreeStack;
		int nVideoFd;
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

		App(int argc, char **argv);
		~App();

		int Run();
	};

	App::App(int argc, char **argv) : argc(argc), argv(argv) {
		// LOGD("%s(%d):", __FUNCTION__, __LINE__);
	}

	App::~App() {
		// LOGD("%s(%d):", __FUNCTION__, __LINE__);
	}

	int App::Run() {
		int err;

		switch(1) { case 1:
			nVideoFd = v4l2_open("/dev/video0", O_RDWR | O_NONBLOCK);
			if(nVideoFd == -1) {
				err = errno;
				LOGE("%s(%d): v4l2_open() failed, __FUNCTION__, __LINE__, err");
				break;
			}
			oFreeStack += [&]() {
				v4l2_close(nVideoFd);
				nVideoFd = -1;
			};

			LOGD("nVideoFd=%d", nVideoFd);

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
				case '1': {
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

					LOGD("+param: %d %d %d %d %d %d", (int)format.fmt.pix_mp.num_planes,
						(int)format.fmt.pix_mp.plane_fmt[0].sizeimage,
						(int)format.fmt.pix_mp.plane_fmt[0].bytesperline,
						(int)format.fmt.pix_mp.plane_fmt[0].sizeimage,
						(int)format.fmt.pix_mp.plane_fmt[0].bytesperline);

					err = ioctl(nVideoFd, VIDIOC_S_FMT, &format);
					if(err) {
						err = errno;
						LOGE("%s(%d): ioctl(VIDIOC_S_FMT) failed, err=%d", __FUNCTION__, __LINE__, err);
						break;
					}

					LOGD("-param: %d %d %d %d %d %d", (int)format.fmt.pix_mp.num_planes,
						(int)format.fmt.pix_mp.plane_fmt[0].sizeimage,
						(int)format.fmt.pix_mp.plane_fmt[0].bytesperline,
						(int)format.fmt.pix_mp.plane_fmt[0].sizeimage,
						(int)format.fmt.pix_mp.plane_fmt[0].bytesperline);
				}
					break;

				case '2': {
					struct v4l2_requestbuffers requestBuffers;
					memset(&requestBuffers, 0, sizeof(struct v4l2_requestbuffers));
					requestBuffers.count = nBuffers;
					requestBuffers.type = nBufType;
					requestBuffers.memory = nMemory;
					err = ioctl(nVideoFd, VIDIOC_REQBUFS, &requestBuffers);
					if(err) {
						err = errno;
						LOGE("%s(%d): ioctl(VIDIOC_REQBUFS) failed, err=%d", __FUNCTION__, __LINE__, err);
						break;
					}

					LOGD("param: %d", requestBuffers.count);
				}
					break;

				case '3': {
					for(int i = 0;i < nBuffers;i++) {
						v4l2_buffer buffer;
						v4l2_plane planes[2];
						memset(&buffer, 0, sizeof(v4l2_buffer));
						buffer.index = i;
						buffer.type = nBufType;
						buffer.memory = nMemory;
						buffer.m.planes = planes;
						buffer.length = 2;
						err = ioctl(nVideoFd, VIDIOC_QUERYBUF, &buffer);
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
					break;

				case '4': {
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
						err = ioctl(nVideoFd, VIDIOC_QUERYBUF, &buffer);
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
								nVideoFd, (off_t)buffer.m.planes[p].m.mem_offset);
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
					break;
				}

				return err;
			}, 1000000LL, 1LL);

			err = 0;
		}

		oFreeStack.Flush();

		return err;
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
