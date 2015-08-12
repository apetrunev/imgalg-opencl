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

#include "img.h"
#include "img_utils.h"
#include "xmalloc.h"

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

int main(int argc, char **argv)
{
	GdkPixbuf *pbuf, *newbuf;
	GError *error = NULL;
	struct img_ctx *rgb, *gray, *blur;
	char *fname, *imgname, *outname, *ext, *src, *log;
	int w, h, len, opt;
	struct stat sb;
	FILE *file;
	size_t fsize, log_size;

	fname = NULL;
	outname = NULL;
	imgname = NULL;

	while ((opt = getopt(argc, argv, "i:")) != -1) {
		switch (opt) {
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

	if (imgname == NULL) {
		fprintf(stderr, "error: no image name is specified\n");
		exit(EXIT_FAILURE);
	}	
	
	if (outname == NULL) {
		outname = xstrdup("out.png");
		ext = get_extention(outname);
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
	len = w*h;

	gray = img_ctx_new(w, h, TYPE_GRAY, C_NONE);
	img_grayscale(rgb, gray);
	img_gaussian_blur(gray, gray);	
	
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

	return EXIT_SUCCESS;
}
