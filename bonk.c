#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "video.h"
#include "network.h"

// signal handling
int done;

void handle_sigint(int sig __attribute__((unused))) {
	done = 1;
}

static const char DEV_NAME[] = "/dev/video0";

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
	struct video_ctx   vid_ctx;
	struct network_ctx net_ctx;


	fprintf(stderr, "Initializing video...\n");
	if(video_init(DEV_NAME, &vid_ctx) != 0) {
		fprintf(stderr, "Error while initializing video\n");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "Initializing network...\n");
	if(network_init(&net_ctx) != 0) {
		fprintf(stderr, "Error while initializing network\n");
		exit(EXIT_FAILURE);
	}

	done = 0;

	if (signal(SIGINT, handle_sigint) == SIG_ERR) {
		fprintf(stderr, "Could not register SIGINT handler\n");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "Waiting for client...\n");
	while(!done) {
		network_wait_for_client(&net_ctx);

		fprintf(stderr, "Got client, starting stream...\n");
		if(video_start(&vid_ctx) != 0) {
			fprintf(stderr, "Error starting video\n");
			exit(EXIT_FAILURE);
		}

		fprintf(stderr, "Streaming...\n");
		while(!done) {
			struct video_buffer *cur_buf = video_read(&vid_ctx);
			if(cur_buf == NULL) {
				fprintf(stderr, "Error reading video\n");
				exit(EXIT_FAILURE);
			}

			if(network_write(&net_ctx, cur_buf->start, cur_buf->bytesused) != 0) {
				fprintf(stderr, "Error writing to network connection\n");
				break;
			}
			video_recycle_buffer(&vid_ctx, cur_buf);
		}

		fprintf(stderr, "Stopping stream...\n");
		video_stop(&vid_ctx);
	}

	fprintf(stderr, "Cleaning up...\n");

	video_deinit(&vid_ctx);

	network_deinit(&net_ctx);

	return 0;
}
