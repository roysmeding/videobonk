#pragma once

struct video_buffer {
	unsigned int index;
	void     *start;
	size_t   length;
	size_t   bytesused;
};

struct video_ctx {
	int fd;

	unsigned int n_buffers;
	struct video_buffer *buffers;

	unsigned int cur_read_buffer;
};

int video_init(const char *dev_name, struct video_ctx *ctx);
int video_start(struct video_ctx *ctx);
struct video_buffer *video_read(struct video_ctx *ctx);
int video_recycle_buffer(struct video_ctx *ctx, struct video_buffer *buffer);
int video_stop(struct video_ctx *ctx);
int video_deinit(struct video_ctx *ctx);
