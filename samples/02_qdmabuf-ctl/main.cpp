#include "ZzLog.h"
#include "ZzUtils.h"

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
		int err;

		switch(1) { case 1:
			LOGD("HERE");
			err = 0;
		}

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
