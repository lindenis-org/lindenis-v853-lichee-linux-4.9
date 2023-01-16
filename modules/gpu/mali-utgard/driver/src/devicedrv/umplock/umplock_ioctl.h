/*
 * Copyright (C) 2010-2017 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 * Class Path Exception
 * Linking this library statically or dynamically with other modules is making a combined work based on this library. 
 * Thus, the terms and conditions of the GNU General Public License cover the whole combination.
 * As a special exception, the copyright holders of this library give you permission to link this library with independent modules 
 * to produce an executable, regardless of the license terms of these independent modules, and to copy and distribute the resulting 
 * executable under terms of your choice, provided that you also meet, for each linked independent module, the terms and conditions 
 * of the license of that module. An independent module is a module which is not derived from or based on this library. If you modify 
 * this library, you may extend this exception to your version of the library, but you are not obligated to do so. 
 * If you do not wish to do so, delete this exception statement from your version.
 */

#ifndef __UMPLOCK_IOCTL_H__
#define __UMPLOCK_IOCTL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>
#include <linux/ioctl.h>

#ifndef __user
#define __user
#endif


/**
 * @file umplock_ioctl.h
 * This file describes the interface needed to use the Linux device driver.
 * The interface is used by the userpace Mali DDK.
 */

typedef enum {
	_LOCK_ACCESS_RENDERABLE = 1,
	_LOCK_ACCESS_TEXTURE,
	_LOCK_ACCESS_CPU_WRITE,
	_LOCK_ACCESS_CPU_READ,
} _lock_access_usage;

typedef struct _lock_item_s {
	unsigned int secure_id;
	_lock_access_usage usage;
} _lock_item_s;


#define LOCK_IOCTL_GROUP 0x91

#define _LOCK_IOCTL_CREATE_CMD  0   /* create kernel lock item        */
#define _LOCK_IOCTL_PROCESS_CMD 1   /* process kernel lock item       */
#define _LOCK_IOCTL_RELEASE_CMD 2   /* release kernel lock item       */
#define _LOCK_IOCTL_ZAP_CMD     3   /* clean up all kernel lock items */
#define _LOCK_IOCTL_DUMP_CMD    4   /* dump all the items */

#define LOCK_IOCTL_MAX_CMDS     5

#define LOCK_IOCTL_CREATE  _IOW( LOCK_IOCTL_GROUP, _LOCK_IOCTL_CREATE_CMD,  _lock_item_s )
#define LOCK_IOCTL_PROCESS _IOW( LOCK_IOCTL_GROUP, _LOCK_IOCTL_PROCESS_CMD, _lock_item_s )
#define LOCK_IOCTL_RELEASE _IOW( LOCK_IOCTL_GROUP, _LOCK_IOCTL_RELEASE_CMD, _lock_item_s )
#define LOCK_IOCTL_ZAP     _IO ( LOCK_IOCTL_GROUP, _LOCK_IOCTL_ZAP_CMD )
#define LOCK_IOCTL_DUMP    _IO ( LOCK_IOCTL_GROUP, _LOCK_IOCTL_DUMP_CMD )

#ifdef __cplusplus
}
#endif

#endif /* __UMPLOCK_IOCTL_H__ */

