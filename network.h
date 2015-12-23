#pragma once

#include <netinet/in.h>

struct network_ctx {
	int host_fd ,client_fd;
	struct sockaddr_in host_addr, client_addr;
} ;

int network_init(struct network_ctx *ctx);
int network_wait_for_client(struct network_ctx *ctx);
int network_write(struct network_ctx *ctx, void* buf, size_t len);
int network_deinit(struct network_ctx *ctx);
