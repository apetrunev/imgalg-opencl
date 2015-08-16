#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <CL/cl.h>

#include "img.h"
#include "clerr.h"
#include "xmalloc.h"
#include "cl_blur.h"

static cl_platform_id platform;
static cl_device_id device;
static cl_context context;
static cl_command_queue queue;
static cl_program program;

int get_ctx(GdkPixbuf *pbuf, img_type_t type, struct img_ctx **imctx)
{
	struct img_ctx *ctx;
	int nchan, w, h, i, j, rowstride, skip;
	unsigned char *pix, *p, *q, *r, *g, *b;
	
	assert(pbuf != NULL);
	assert(imctx != NULL);
	
	nchan = gdk_pixbuf_get_n_channels(pbuf);
	w = gdk_pixbuf_get_width(pbuf);
	h = gdk_pixbuf_get_height(pbuf);
	rowstride = gdk_pixbuf_get_rowstride(pbuf);
	pix = gdk_pixbuf_get_pixels(pbuf);

	skip = rowstride - w*nchan;

	switch (type) {
	case TYPE_GRAY:
		ctx = img_ctx_new(w, h, TYPE_GRAY, C_NONE);
		
		p = pix;
        	q = ctx->pix;

		for (i = 0; i < h; i++) {
                	for (j = 0; j < w; j++) {
				/* the same gray value in the next two pixels */
				q[0] = p[0];
				/* skip it */
                       		p += nchan;
				q++;
                	}
                	/* skip the padding bytes */
                	p += skip;
        	}	
		break;
	case TYPE_RGB:
		ctx = img_ctx_new(w, h, TYPE_RGB, C_NONE);
		
		p = pix;
		r = ctx->r;
		g = ctx->g;
		b = ctx->b;

		for (i = 0; i < h; i++) {
			for (j = 0; j < w; j++) {
				r[0] = p[0];
				g[0] = p[1];
				b[0] = p[2];
				p += nchan;
				r++;
				g++;
				b++;
			}
			p += skip;
		}
		break;
	default:
		fprintf(stderr, "error: not implemented\n");
		abort();
	}

	*imctx = ctx;

	return RET_OK;		
}

int load_ctx(struct img_ctx *ctx, GdkPixbuf *pbuf)
{
	unsigned int nchan, w, h, i, j, rowstride, skip;
	unsigned char *pix, *p, *gray;

	assert(ctx != NULL);
	assert(pbuf != NULL);

	w = ctx->w;
	h = ctx->h;

	nchan = gdk_pixbuf_get_n_channels(pbuf);
	rowstride = gdk_pixbuf_get_rowstride(pbuf);
	pix = gdk_pixbuf_get_pixels(pbuf);
	
	skip = rowstride - w*nchan;
	
	p = pix;
	gray = ctx->pix;

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {			
			p[0] = gray[0];
			p[1] = gray[0];
			p[2] = gray[0];
			p += nchan;
			gray++;
		}
		
		p += skip;
	}

	return RET_OK;
}

char *get_extention(char *fname)
{
	char *p;

	assert(fname != NULL);
	
	p = strchr(fname, '.') + 1;
	return xstrdup(p);
}

cl_mem create_buffer(cl_context context, 
		     cl_mem_flags flags, 
		     size_t size, 
		     void *host_ptr)
{
	cl_mem buf;
	cl_int err;

	buf = clCreateBuffer(context, flags, size, host_ptr, &err);

	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clCreateBuffer() %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}

	return buf;
}

void build_program(cl_program program, 
		   cl_uint num_devices, 
		   const cl_device_id *device, 
		   const char *options, 
		   void (*pfn_notify)(cl_program, void *user_data),
		   void *user_data)
{
	cl_int err;
	size_t log_size;
	char *log;

	/* building program */
	err = clBuildProgram(program, num_devices, device, options, pfn_notify, user_data);
	if (err != CL_SUCCESS) {
		/* determine size of compiler log */
		clGetProgramBuildInfo(program, *device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		log = malloc(log_size + 1);
		log[log_size] = '\0';
		/* store compiler output to buffer */
		clGetProgramBuildInfo(program, *device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
		fprintf(stderr, "error: clBuildProgram() %d %s\n", err, cl_strerror(err));
		fprintf(stderr, "%s\n", log);
		exit(EXIT_FAILURE);
	}
}

cl_kernel create_kernel(cl_program  program, const char *kernel_name)
{
	cl_kernel kernel;
	cl_int err;

	kernel = clCreateKernel(program, kernel_name, &err);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clCreateKerne() %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}
	
	return kernel;
}

void xcl_img_grayscale(struct img_ctx *rgb, struct img_ctx *gray)
{
	cl_kernel cl_img_grayscale;
	cl_mem r, g, b, out;
	cl_int err;
	size_t global_work_size;
	size_t local_work_size;
	int len;

	assert(rgb != NULL);
	assert(gray != NULL);

	len = rgb->w*rgb->h;
	err = 0;

	cl_img_grayscale = create_kernel(program, "cl_img_grayscale");

	r = create_buffer(context, CL_MEM_READ_WRITE, len, NULL);
	g = create_buffer(context, CL_MEM_READ_WRITE, len, NULL);
	b = create_buffer(context, CL_MEM_READ_WRITE, len, NULL);
	out = create_buffer(context, CL_MEM_WRITE_ONLY, len, NULL);

	err |= clSetKernelArg(cl_img_grayscale, 0, sizeof(cl_mem), &r);
	err |= clSetKernelArg(cl_img_grayscale, 1, sizeof(cl_mem), &g);
	err |= clSetKernelArg(cl_img_grayscale, 2, sizeof(cl_mem), &b);
	err |= clSetKernelArg(cl_img_grayscale, 3, sizeof(cl_mem), &out);
	err |= clSetKernelArg(cl_img_grayscale, 4, sizeof(cl_int), (void *)&len);

	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clSetKernelArg() %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}

	err |= clEnqueueWriteBuffer(queue, r, CL_FALSE, 0, len, rgb->r, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, g, CL_FALSE, 0, len, rgb->g, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, b, CL_FALSE, 0, len, rgb->b, 0, NULL, NULL);

	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clEnqueueWriteBuffer() %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}	

	global_work_size = len;
	local_work_size = 64;

	err = clEnqueueNDRangeKernel(queue, cl_img_grayscale, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clEnqueueNDRangeKernel() %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}
	
	err = clEnqueueReadBuffer(queue, out, CL_TRUE, 0, len, gray->pix, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clEnqueueReadBuffer() %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}
	
	clReleaseMemObject(r);
   	clReleaseMemObject(g);
   	clReleaseMemObject(b);
	clReleaseMemObject(out);
	clReleaseKernel(cl_img_grayscale);
}

void xcl_img_gaussian_blur(struct img_ctx *gray, struct img_ctx *blur)
{
	cl_mem gray_buf, blur_buf, gauss_buf;
	cl_kernel cl_img_gaussian_blur;
	cl_int err;
	size_t global_wblur[2];
	size_t local_wblur[2];
	int len;

	assert(gray != NULL);
	assert(blur != NULL);

	len = gray->w*gray->h;
	err = 0;

	cl_img_gaussian_blur = create_kernel(program, "cl_img_gaussian_blur");

	gray_buf = create_buffer(context,  CL_MEM_READ_ONLY, len, NULL);
	gauss_buf = create_buffer(context, CL_MEM_READ_ONLY, gauss_dim*gauss_dim*sizeof(cl_int), NULL);
	/* output buffer */
	blur_buf = create_buffer(context, CL_MEM_WRITE_ONLY, len, NULL);

	err |= clSetKernelArg(cl_img_gaussian_blur, 0, sizeof(cl_mem), &gray_buf);
	err |= clSetKernelArg(cl_img_gaussian_blur, 1, sizeof(cl_mem), &blur_buf);
	err |= clSetKernelArg(cl_img_gaussian_blur, 2, sizeof(cl_mem), &gauss_buf);
	err |= clSetKernelArg(cl_img_gaussian_blur, 3, sizeof(cl_int), &gauss_dim);
	err |= clSetKernelArg(cl_img_gaussian_blur, 4, sizeof(cl_int), &gauss_sum);
	err |= clSetKernelArg(cl_img_gaussian_blur, 5, sizeof(cl_int), &gray->w);
	err |= clSetKernelArg(cl_img_gaussian_blur, 6, sizeof(cl_int), &gray->h);
	
	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clSetKernelArg() %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}

	err |= clEnqueueWriteBuffer(queue, gray_buf, CL_FALSE, 0, len, gray->pix, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, gauss_buf, CL_FALSE, 0, gauss_dim*gauss_dim*sizeof(cl_int), gauss, 0, NULL, NULL);

	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clEnqueueWriteBuffer() %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}	

	global_wblur[0] = gray->h;
	global_wblur[1] = gray->w;
	local_wblur[0] = local_wblur[1] = 32;

	err = clEnqueueNDRangeKernel(queue, cl_img_gaussian_blur, 2, NULL, global_wblur, local_wblur, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clEnqueueNDRangeKernel() blur %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}

	err = clEnqueueReadBuffer(queue, blur_buf, CL_TRUE, 0, len, blur->pix, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clEnqueueReadBuffer() blur %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}

	clReleaseMemObject(gray_buf);
	clReleaseMemObject(blur_buf);
	clReleaseMemObject(gauss_buf);
	clReleaseKernel(cl_img_gaussian_blur);
}

int main(int argc, char **argv)
{
	GdkPixbuf *pbuf, *newbuf;
	GError *error = NULL;
	struct img_ctx *rgb, *gray;
	char *fname, *imgname, *outname, *ext, *src;
	int w, h, opt;
	struct stat sb;
	FILE *file;
	size_t fsize;
	cl_int err;

	fname = NULL;
	outname = NULL;
	imgname = NULL;

	while ((opt = getopt(argc, argv, "f:i:")) != -1) {
		switch (opt) {
		case 'f':
			fname = xstrdup(optarg);
			break;
		case 'i':
			imgname = xstrdup(optarg);
			break;
		case 'o':
			outname = xstrdup(optarg);
			break;
		default:
			fprintf(stderr, "error: unknown option `%i'\n", opt);
			exit(EXIT_FAILURE);
		}
	}

	argc -= optind;
	argv += optind;

	if (fname == NULL) {
		fprintf(stderr, "error: no file name is specified\n");
		exit(EXIT_FAILURE);
	}

	if (imgname == NULL) {
		fprintf(stderr, "error: no image name is specified\n");
		exit(EXIT_FAILURE);
	}	
	
	if (outname == NULL) {
		outname = xstrdup("out.png");
		ext = get_extention(outname);
	}

	/* chek file existance */
	if (stat(fname, &sb) == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	file = fopen(fname, "r");
	
	if (file == NULL) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	/* get file size */
	fsize = sb.st_size;
	/* mmap source file to memory */
	src = mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fileno(file), 0);
	
	fclose(file);
	
	if (src == (void *)(-1)) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* read image to buffer */
	gtk_init(&argc, &argv);

	newbuf = pbuf = NULL;

	pbuf = gdk_pixbuf_new_from_file(imgname, &error);
	if (pbuf == NULL) {
		fprintf(stderr, "error: unable to load image\n");
		exit(EXIT_FAILURE);
	}

	get_ctx(pbuf, TYPE_RGB, &rgb);

	w = rgb->w;
	h = rgb->h;

	gray = img_ctx_new(w, h, TYPE_GRAY, C_NONE);

	/* 
	 * OPENCL INITIALIZATION
	 */
	err = clGetPlatformIDs(1, &platform, NULL);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clGetPlatformIDs() errcode %d %s\n", err, cl_strerror(err));
		exit(EXIT_SUCCESS);
	}
	
	/* get available device */
	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clGetPlatformIDs() %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}

	/* create context */	
	context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clCreateContext() %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}

	/* create program source */
	program = clCreateProgramWithSource(context, 1, (const char **)&src, &fsize, &err);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clCreateProgramWithSource() %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}

	/* create comand queue */	
	queue = clCreateCommandQueue(context, device, 0, &err);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "error: clCreateCommandQueue() %d %s\n", err, cl_strerror(err));
		exit(EXIT_FAILURE);
	}
	
	build_program(program, 1, &device, NULL, NULL, NULL);
	
	/* run kernels */
	xcl_img_grayscale(rgb, gray);
	xcl_img_gaussian_blur(gray, gray);
	
	/* END OF OPENCL SECTION
	 */
	newbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, w, h);
	if (newbuf == NULL) {
		fprintf(stderr, "error: unable to create pixbuf\n");
		exit(EXIT_FAILURE);
	}

	load_ctx(gray, newbuf);

	if (!gdk_pixbuf_save(newbuf, outname, ext, &error, NULL)) {
		fprintf(stderr,"error: failed to save image: %s\n", error->message);
		g_error_free(error);
		exit(EXIT_FAILURE);
	}
	
	img_destroy_ctx(rgb);
	img_destroy_ctx(gray);

	g_object_unref(G_OBJECT(pbuf));
	g_object_unref(G_OBJECT(newbuf));

   	clReleaseCommandQueue(queue);
   	clReleaseProgram(program);
   	clReleaseContext(context);
	
	return EXIT_SUCCESS;
}
