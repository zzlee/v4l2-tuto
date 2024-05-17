#ifndef __QDMABUF_VERSION_H__
#define __QDMABUF_VERSION_H__

#define DRV_MOD_MAJOR		2020
#define DRV_MOD_MINOR		2
#define DRV_MOD_PATCHLEVEL	2

#define DRV_MODULE_VERSION      \
	__stringify(DRV_MOD_MAJOR) "." \
	__stringify(DRV_MOD_MINOR) "." \
	__stringify(DRV_MOD_PATCHLEVEL)

#define DRV_MOD_VERSION_NUMBER  \
	((DRV_MOD_MAJOR)*1000 + (DRV_MOD_MINOR)*100 + DRV_MOD_PATCHLEVEL)

#endif // __QDMABUF_VERSION_H__
