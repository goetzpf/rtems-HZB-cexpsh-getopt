/* $Id$ */
#ifndef ECDR_814_OS_DEP_H
#define ECDR_814_OS_DEP_H
/* handle operating system dependencies for the ecdr814 driver
 * Author: Till Straumann, <strauman@slac.stanford.edu>, 8/2001
 */

#if defined(__vxworks)
#include <vxWorks.h>
#elif defined(__rtems)
#include <rtems.h>
#elif defined(__linux)
#else
#error "no OS type defined"
#endif

/* mapping of local to bus addresses */

#if defined(__vxworks)
#   include <vme.h>
#   define osdep_vme2local(vmeaddr, plocaladdr) \
	sysBusToLocalAdrs(VME_AM_EXT_SUP_DATA, (void*)(vmeaddr), (void**)plocaladdr)
#   define osdep_local2vme(localaddr, pvmeaddr) \
	sysLocalToBusAdrs(VME_AM_EXT_SUP_DATA, (void*)(localaddr), (void**)pvmeaddr)
#elif defined(__rtems)
#   include <bsp.h>
#   include <bsp/VME.h>
#   define osdep_vme2local(vmeaddr, plocaladdr) \
	BSP_vme2local_adrs(VME_AM_EXT_SUP_DATA, \
		(unsigned long)(vmeaddr), \
		(unsigned long*)(plocaladdr))
#   define osdep_local2vme(localaddr, pvmeaddr) \
	BSP_local2vme_adrs(VME_AM_EXT_SUP_DATA, \
		(unsigned long)(localaddr), \
		(unsigned long*)(pvmeaddr))
#elif defined(__linux)
#   define osdep_vme2local(vmeaddr, plocaladdr) \
	((*(plocaladdr)=(vmeaddr)),0)
#   define osdep_local2vme(localaddr, pvmeaddr) \
	((*(pvmeaddr)=(localaddr)),0)
#endif

/* probing of memory addresses */

#if defined(__vxworks)

#   define osdep_memProbe(addr, writeNotRead, nBytes, pVal) \
	vxMemProbe((void*)(addr),(writeNotRead),(nBytes),(void*)(pVal))

#elif defined(__rtems)

#   include <bsp/bspExt.h>

#   define osdep_memProbe(addr, writeNotRead, nBytes, pVal) \
	bspExtMemProbe((void*)(addr),(writeNotRead),(nBytes),(void*)(pVal))

#elif defined(__linux)

/* since __linux is only used for testing, no real implementation
 * (catching segfaults) is provided...
 */
#   define osdep_memProbe(addr, writeNotRead, nBytes, pVal) \
	((writeNotRead) ? (memcpy((void*)(addr),(void*)(pVal),(nBytes)), 0) : \
                          (memcpy((void*)(pVal),(void*)(addr),(nBytes)), 0))
#endif

/* interrupt handlers */

#if defined(__vxworks)
#include <intLib.h>
#include <iv.h>
#	define osdep_IRQ_HANDLER_PROTO(name, arg) \
		void name(void *arg)

#	define osdep_intConnect(vector, handler, arg) \
		intConnect(	(VOIDFUNCPTR*)INUM_TO_IVEC(vector),\
				(VOIDFUNCPTR)handler,\
				(int)arg)
#	define osdep_intEnable(level) \
		intEnable(level)
#elif defined(__rtems)
#	define osdep_IRQ_HANDLER_PROTO(name, arg) \
		void name(void *arg, unsigned long vector)

#	define osdep_intConnect(vector, handler, arg) \
		BSP_installVME_isr( vector, handler, arg )

#	define osdep_intEnable(level) \
		BSP_enableVME_int_lvl(level)

#else
#	define osdep_IRQ_HANDLER_PROTO(name, arg) \
		void name(void *arg)

#	define osdep_intConnect(vector, handler, arg)
#	define osdep_intEnable(level)
#endif

#endif
