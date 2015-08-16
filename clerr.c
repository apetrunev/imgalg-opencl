#include <stdlib.h>

#include <CL/cl.h>

#include "clerr.h"
#include "xmalloc.h"

struct error_codes {
	cl_int code;
	char *desc;
};

struct error_codes errtable[] = {
	/* run-time errors */
	{ CL_SUCCESS, "success" },									/* 0 */
	{ CL_DEVICE_NOT_FOUND, "device not found" },							/* -1 */
	{ CL_DEVICE_NOT_AVAILABLE, "device not available" },						/* -2 */
	{ CL_COMPILER_NOT_AVAILABLE, "compiler not available" },					/* -3 */
	{ CL_MEM_OBJECT_ALLOCATION_FAILURE, "mem object allocation failure" },				/* -4 */
	{ CL_OUT_OF_RESOURCES, "out of resources" },							/* -5 */
	{ CL_OUT_OF_HOST_MEMORY, "out of host memmory" },						/* -6 */
	{ CL_PROFILING_INFO_NOT_AVAILABLE, "profilig info not available" },				/* -7 */
	{ CL_MEM_COPY_OVERLAP, "mem copy overlap" },							/* -8 */
	{ CL_IMAGE_FORMAT_MISMATCH, "image format mismatch" },						/* -9 */
	{ CL_IMAGE_FORMAT_NOT_SUPPORTED, "image format not supported" },				/* -10 */
	{ CL_BUILD_PROGRAM_FAILURE, "build program failure" },						/* -11 */
	{ CL_MAP_FAILURE, "map failure" },								/* -12 */
	{ CL_MISALIGNED_SUB_BUFFER_OFFSET, "misaligned sub buffer offset" },				/* -13 */
	{ CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST, "exec status error for events in wait list" },	/* -14 */
	{ CL_COMPILE_PROGRAM_FAILURE, "compile program failure" },					/* -15 */
	{ CL_LINKER_NOT_AVAILABLE, "linker not available" },						/* -16 */
	{ CL_LINK_PROGRAM_FAILURE, "link program failure" },						/* -17 */
	{ CL_DEVICE_PARTITION_FAILED, "device partition failed" },					/* -18 */
	{ CL_KERNEL_ARG_INFO_NOT_AVAILABLE, "kernel arg info not available" },				/* -19 */
	/* compile time errors */
	{ CL_INVALID_VALUE, "invalid value" },								/* -30 */
	{ CL_INVALID_DEVICE_TYPE, "invalid device type" },						/* -31 */
	{ CL_INVALID_PLATFORM, "invalid plarform" },							/* -32 */
	{ CL_INVALID_DEVICE, "invalid device" },							/* -33 */
	{ CL_INVALID_CONTEXT, "invalid context" },							/* -34 */
	{ CL_INVALID_QUEUE_PROPERTIES, "invalid queue properties" },					/* -35 */
	{ CL_INVALID_COMMAND_QUEUE, "invalid command queue" },						/* -36 */
	{ CL_INVALID_HOST_PTR, "invalid host pointer" },						/* -37 */	
	{ CL_INVALID_MEM_OBJECT, "invalid mem object" },						/* -38 */
	{ CL_INVALID_IMAGE_FORMAT_DESCRIPTOR, "invalid image format descriptor" },			/* -39 */
	{ CL_INVALID_IMAGE_SIZE, "invalid image size" },						/* -40 */
	{ CL_INVALID_SAMPLER, "invalid sampler" },							/* -41 */
	{ CL_INVALID_BINARY, "invalid binary" },							/* -42 */
	{ CL_INVALID_BUILD_OPTIONS, "invalid build options" },						/* -43 */
	{ CL_INVALID_PROGRAM, "invalid program" },							/* -44 */
	{ CL_INVALID_PROGRAM_EXECUTABLE, "invalid program executable" },				/* -45 */
	{ CL_INVALID_KERNEL_NAME, "invalid kernel name" },						/* -46 */
	{ CL_INVALID_KERNEL_DEFINITION, "invalid kernel definition" },					/* -47 */
	{ CL_INVALID_KERNEL, "invalid kernel" },							/* -48 */
	{ CL_INVALID_ARG_INDEX, "invalid arg index" },							/* -49 */
	{ CL_INVALID_ARG_VALUE, "invalid arg value" },							/* -50 */
	{ CL_INVALID_ARG_SIZE, "invalid arg size" },							/* -51 */
	{ CL_INVALID_KERNEL_ARGS, "invalid kernel args" },						/* -52 */
	{ CL_INVALID_WORK_DIMENSION, "invalid work dimention" },					/* -53 */
	{ CL_INVALID_WORK_GROUP_SIZE, "invalid work group size" },					/* -54 */
	{ CL_INVALID_WORK_ITEM_SIZE, "invalid work item size" },					/* -55 */
	{ CL_INVALID_GLOBAL_OFFSET, "invalid gobal offset" },						/* -56 */
	{ CL_INVALID_EVENT_WAIT_LIST, "invalid event wait list" },					/* -57 */
	{ CL_INVALID_EVENT, "invalid event" },								/* -58 */
	{ CL_INVALID_OPERATION, "invalid operation" },							/* -59 */
	{ CL_INVALID_GL_OBJECT, "invalid gl object" },							/* -60 */
	{ CL_INVALID_BUFFER_SIZE, "invalid buffer size" },						/* -61 */
	{ CL_INVALID_MIP_LEVEL, "invalid mip level" },							/* -62 */
	{ CL_INVALID_GLOBAL_WORK_SIZE, "invalid global work size" },					/* -63 */
	{ CL_INVALID_PROPERTY, "invalid property" },							/* -64 */
	{ CL_INVALID_IMAGE_DESCRIPTOR, "invalid image descriptor" },					/* -65 */
	{ CL_INVALID_COMPILER_OPTIONS, "invalid compiler options" },					/* -66 */
	{ CL_INVALID_LINKER_OPTIONS, "invalid linker options" },					/* -67 */
	{ CL_INVALID_DEVICE_PARTITION_COUNT, "invalid device partition count" },			/* -68 */
#define CL_TABLE_END (-2000)
	{ CL_TABLE_END, NULL }
};

char *cl_strerror(int code)
{
	int i;

	for (i = 0; errtable[i].code != CL_TABLE_END; i++) {
		if (errtable[i].code == code)
			return xstrdup(errtable[i].desc);
	}
	return NULL;
}
