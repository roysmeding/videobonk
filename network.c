#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "network.h"

const int PORT         = 6969;
const int BACKLOG_SIZE = 5;

int network_init(struct network_ctx *ctx) {
	// create socket
	ctx->host_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (ctx->host_fd == -1) {
		perror("socket");
		return -1;
	}

	memset((char *) &ctx->host_addr, 0, sizeof(struct sockaddr_in));

	ctx->host_addr.sin_family      = AF_INET;
	ctx->host_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ctx->host_addr.sin_port        = htons(PORT);

	if(bind(ctx->host_fd, (struct sockaddr *) (&ctx->host_addr), sizeof(struct sockaddr_in)) == -1) {
		perror("bind");
		return -1;
	}

	if(listen(ctx->host_fd, BACKLOG_SIZE) == -1) {
		perror("listen");
		return -1;
	}

	return 0;
}

int network_wait_for_client(struct network_ctx *ctx) {
	socklen_t client_addr_size = sizeof(struct sockaddr_in);
	ctx->client_fd = accept(ctx->host_fd, (struct sockaddr *) &ctx->client_addr, &client_addr_size);
	if(ctx->client_fd == -1) {
		perror("accept");
		return -1;
	}

	return 0;
}

int network_write(struct network_ctx *ctx, void* buf, size_t len) {
	if(write(ctx->client_fd, buf, len) == -1) {
		perror("write");
		return -1;
	}

	return 0;
}

int network_deinit(struct network_ctx *ctx) {
	if(close(ctx->client_fd) == -1) {
		perror("close");
		return -1;
	}

	if(close(ctx->host_fd) == -1) {
		perror("close");
		return -1;
	}

	return 0;
}
