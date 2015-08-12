__kernel void cl_img_grayscale(__global const uchar *r, __global const uchar *g, __global const uchar *b, __global uchar *gray,
			       uint len) 
{	
	uint i;

	i = get_global_id(0);
	
	if (i >= len)
		return;

	gray[i] = (uint)(0.229*r[i] + 0.587*g[i] + 0.114*b[i]);	
}

__kernel void cl_img_gaussian_blur(__global const uchar *gray, __global uchar *out, __global const uchar *gbox, uint n, uint sum, uint w, uint h)
{
	int i, j, offset;
	uint x, y, summ;

	x = get_global_id(0);
	y = get_global_id(1);

	offset = n/2;

	/* ignore border pixels 
	 */
	if (y - offset < 0 || y + offset > h || x - offset < 0 || x + offset > w) {
		out[y*w + x] = gray[y*w + x];
		return;
	}
	
	summ = 0;

	for (j = -offset; j <= offset; j++) {
		for (i = -offset; i <= offset; i++) {
			summ = summ + gray[(y + j)*w + x + i]*gbox[(j + offset)*n + i + offset];
		}
	}

	out[y*w + x] = summ/sum;
}

