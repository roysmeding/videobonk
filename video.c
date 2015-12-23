#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>

#include "video.h"

static int xioctl(int fh, int request, void *arg) {
	int r;

	do {
		r = v4l2_ioctl(fh, request, arg);
	} while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

	if (r == -1) {
		fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
	}
	return r;
}

#define VIDEO_FORMAT V4L2_PIX_FMT_MJPEG
#define VIDEO_WIDTH  1280
#define VIDEO_HEIGHT  960

int video_init(const char *dev_name, struct video_ctx *ctx) {
	struct v4l2_format fmt;

	// open device
	ctx->fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
	if(ctx->fd < 0)
		return -1;


	// set format
	memset(&fmt, 0, sizeof(struct v4l2_format));

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = VIDEO_WIDTH;
	fmt.fmt.pix.height      = VIDEO_HEIGHT;
	fmt.fmt.pix.pixelformat = VIDEO_FORMAT;
	fmt.fmt.pix.field       = V4L2_FIELD_ANY;

	xioctl(ctx->fd, VIDIOC_S_FMT, &fmt);

	if (fmt.fmt.pix.pixelformat != VIDEO_FORMAT) {
		fprintf(stderr, "Libv4l didn't accept requested pixel format. Can't proceed.\n");
		return -1;
	}

	if ((fmt.fmt.pix.width != VIDEO_WIDTH) || (fmt.fmt.pix.height != VIDEO_HEIGHT))
		fprintf(stderr, "Warning: driver is sending image at %dx%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);


	// request buffers
	struct v4l2_requestbuffers req;

	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	req.count = 2;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	xioctl(ctx->fd, VIDIOC_REQBUFS, &req);

	ctx->n_buffers = req.count;
	ctx->buffers   = calloc(ctx->n_buffers, sizeof(struct video_buffer));
	for(unsigned int cur_buffer = 0; cur_buffer < ctx->n_buffers; ++cur_buffer) {
		// query buffer from V4L
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(struct v4l2_buffer));

		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = cur_buffer;

		xioctl(ctx->fd, VIDIOC_QUERYBUF, &buf);

		// mmap and assign to buffer array
		ctx->buffers[cur_buffer].index    = buf.index;
		ctx->buffers[cur_buffer].length = buf.length;
		ctx->buffers[cur_buffer].start  = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, ctx->fd, buf.m.offset);

		if (MAP_FAILED == ctx->buffers[cur_buffer].start) {
			perror("mmap");
			return -1;
		}
	}

	return 0;
}

int video_start(struct video_ctx *ctx) {
	// enqueue buffers
	for(unsigned int i = 0; i < ctx->n_buffers; ++i) {
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof(struct v4l2_buffer));
		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = i;

		xioctl(ctx->fd, VIDIOC_QBUF, &buf);
	}

	// start streaming!
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	xioctl(ctx->fd, VIDIOC_STREAMON, &type);

	return 0;
}

struct video_buffer *video_read(struct video_ctx *ctx) {
	struct timeval tv;
	struct v4l2_buffer buf;
	int r;

	// select() on the device fd to know when to read
	do {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(ctx->fd, &fds);

		/* Timeout. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		r = select(ctx->fd + 1, &fds, NULL, NULL, &tv);
	} while ((r == -1 && (errno = EINTR)));

	if (r == -1) {
		perror("select");
		return NULL;
	}

	memset(&buf, 0, sizeof(struct v4l2_buffer));
	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	xioctl(ctx->fd, VIDIOC_DQBUF, &buf);

	ctx->buffers[buf.index].bytesused = buf.bytesused;

	return &ctx->buffers[buf.index];

}

int video_recycle_buffer(struct video_ctx *ctx, struct video_buffer *buffer) {
	struct v4l2_buffer buf;

	memset(&buf, 0, sizeof(struct v4l2_buffer));
	buf.index  = buffer->index;
	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	return xioctl(ctx->fd, VIDIOC_QBUF, &buf);
}

int video_stop(struct video_ctx *ctx) {
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	return xioctl(ctx->fd, VIDIOC_STREAMOFF, &type);
}

int video_deinit(struct video_ctx *ctx) {
	for(unsigned int i = 0; i < ctx->n_buffers; i++) {
		v4l2_munmap(ctx->buffers[i].start, ctx->buffers[i].length);
	}

	free(ctx->buffers);
	v4l2_close(ctx->fd);

	return 0;
}
