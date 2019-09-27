#ifndef __MYCHARDEVDRV_H
#define __MYCHARDEVDRV_H

#include <linux/ioctl.h>

#define VDG_UPPER	0x01
#define VDG_LOWER	0x02

#define VDG_MAGIC	'x'

#define VDG_CLEAR			_IO(VDG_MAGIC, 1)
#define VDG_CHANGE_CASE		_IOW(VDG_MAGIC, 2, int)

#endif

