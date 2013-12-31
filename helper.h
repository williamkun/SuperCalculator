#ifndef HELPER_H
#define HELPER_H

#include <stdint.h>
#include "ffbuffer.h"

void dump_data(void *data, size_t size);

uint16_t ntoh16(uint16_t net);

uint16_t hton16(uint16_t host);

uint32_t ntoh32(uint32_t net);

uint32_t hton32(uint32_t host);

uint64_t ntoh64(uint64_t net);

uint64_t hton64(uint64_t host);

int32_t arrto32(ffbuffer *in);

//NULL means error
char *read_string(int fd);

//zero means succeed
int read_wrapper(int fd, void *buffer, int num);

//zero means succeed
int write_wrapper(int fd, const void *buffer, int num);

//always succeed
void shutdown_wrapper(int fd);

//calculate
void calculate(ffbuffer *in, ffbuffer *out);

#endif
