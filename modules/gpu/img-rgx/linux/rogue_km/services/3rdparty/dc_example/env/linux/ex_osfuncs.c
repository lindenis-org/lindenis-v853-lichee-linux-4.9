/*************************************************************************/ /*!
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#if defined(LINUX)

#include <asm/page.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
/* For MODULE_LICENSE */
#include "pvrmodule.h"

#include "dc_osfuncs.h"
#include "dc_example.h"

#ifndef DC_EXAMPLE_WIDTH
#define DC_EXAMPLE_WIDTH 640
#endif

#ifndef DC_EXAMPLE_HEIGHT
#define DC_EXAMPLE_HEIGHT 480
#endif

#ifndef DC_EXAMPLE_BIT_DEPTH
#define DC_EXAMPLE_BIT_DEPTH 32
#endif

#ifndef DC_EXAMPLE_FORMAT
#define DC_EXAMPLE_FORMAT 0
#endif

#ifndef DC_EXAMPLE_MEMORY_LAYOUT
#define DC_EXAMPLE_MEMORY_LAYOUT 0
#endif

#ifndef DC_EXAMPLE_FBC_FORMAT
#define DC_EXAMPLE_FBC_FORMAT 0
#endif

#ifndef DC_EXAMPLE_REFRESH_RATE
#define DC_EXAMPLE_REFRESH_RATE 60
#endif

#ifndef DC_EXAMPLE_DPI
#define DC_EXAMPLE_DPI 160
#endif


DC_EXAMPLE_MODULE_PARAMETERS sModuleParams =
{
	.ui32Width = DC_EXAMPLE_WIDTH,
	.ui32Height = DC_EXAMPLE_HEIGHT,
	.ui32Depth = DC_EXAMPLE_BIT_DEPTH,
	.ui32Format = DC_EXAMPLE_FORMAT,
	.ui32MemLayout = DC_EXAMPLE_MEMORY_LAYOUT,
	.ui32FBCFormat = DC_EXAMPLE_FBC_FORMAT,
	.ui32RefreshRate = DC_EXAMPLE_REFRESH_RATE,
	.ui32XDpi = DC_EXAMPLE_DPI,
	.ui32YDpi = DC_EXAMPLE_DPI
};

module_param_named(width, 	sModuleParams.ui32Width, uint, S_IRUGO);
module_param_named(height, 	sModuleParams.ui32Height, uint, S_IRUGO);
module_param_named(depth, 	sModuleParams.ui32Depth, uint, S_IRUGO);
module_param_named(format, 	sModuleParams.ui32Format, uint, S_IRUGO);
module_param_named(memlayout, 	sModuleParams.ui32MemLayout, uint, S_IRUGO);
module_param_named(fbcformat, 	sModuleParams.ui32FBCFormat, uint, S_IRUGO);
module_param_named(refreshrate, sModuleParams.ui32RefreshRate, uint, S_IRUGO);
module_param_named(xdpi,	sModuleParams.ui32XDpi, uint, S_IRUGO);
module_param_named(ydpi,	sModuleParams.ui32YDpi, uint, S_IRUGO);

const DC_EXAMPLE_MODULE_PARAMETERS *DCExampleGetModuleParameters(void)
{
	return &sModuleParams;
}

IMG_CPU_VIRTADDR DCExampleVirtualAllocUncached(size_t uiSize)
{
	unsigned int gfp_flags = GFP_USER | __GFP_NOWARN | __GFP_NOMEMALLOC;

#if defined(CONFIG_X86_64)
    gfp_flags |= __GFP_DMA32;
#else
#if !defined(CONFIG_X86_PAE)
    gfp_flags |= __GFP_HIGHMEM;
#endif
#endif

	return __vmalloc(uiSize, gfp_flags, pgprot_noncached(PAGE_KERNEL));
}

IMG_BOOL DCExampleVirtualFree(void *lpAddress, size_t uiSize)
{
	PVR_UNREFERENCED_PARAMETER(uiSize);
	vfree(lpAddress);

	/* vfree does not return a value, so all we can do is hard code IMG_TRUE */
	return IMG_TRUE;
}

#if defined(CONFIG_L4)
/* In L4, the kernel PFN space is a virtual space and not a physical space */
#define	VMALLOC_TO_PAGE_PHYS(vAddr) l4x_virt_to_phys((void *)((phys_addr_t)page_to_pfn(vmalloc_to_page(vAddr)) << PAGE_SHIFT));
#else
#define	VMALLOC_TO_PAGE_PHYS(vAddr) page_to_phys(vmalloc_to_page(vAddr))
#endif

PVRSRV_ERROR DCExampleLinAddrToDevPAddrs(IMG_CPU_VIRTADDR pvLinAddr,
					 IMG_DEV_PHYADDR *pasDevPAddr,
					 size_t uiSize)
{
	unsigned long ulPages = DC_OS_BYTES_TO_PAGES(uiSize);
	int i;

	for (i = 0; i < ulPages; i++)
	{
		pasDevPAddr[i].uiAddr = VMALLOC_TO_PAGE_PHYS(pvLinAddr);
		pvLinAddr += PAGE_SIZE;
	}

	return PVRSRV_OK;
}

static int __init dc_example_init(void)
{
	if (DCExampleInit() != PVRSRV_OK)
	{
		return -ENODEV;
	}

	return 0;
}

/*****************************************************************************
 Function Name: DC_NOHW_Cleanup
 Description  : Remove the driver from the kernel.

                __exit places the function in a special memory section that
                the kernel frees once the function has been run.  Refer also
                to module_exit() macro call below.
*****************************************************************************/
static void __exit dc_example_deinit(void)
{
	DCExampleDeinit();
}

module_init(dc_example_init);
module_exit(dc_example_deinit);

#endif /* defined(LINUX) */

