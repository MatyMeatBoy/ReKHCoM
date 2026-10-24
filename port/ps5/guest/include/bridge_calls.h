/* Prototypes of the host functions (stubs.c) as the guest sees them. */
#ifndef G_BRIDGE_CALLS_H
#define G_BRIDGE_CALLS_H
#include <stdint.h>
#include "bridge.h"
void h_log(const char*);
void h_exit(int);
uint64_t h_ticks_us(void);
void h_sleep_us(uint32_t);
int h_getenv(const char*, char*, int);
int h_fopen(const char*, int);
int h_fread(int, void*, uint32_t);
int h_fwrite(int, const void*, uint32_t);
int h_fseek(int, int32_t, int);
int h_ftell(int);
int h_fclose(int);
int h_fflush(int);
int h_stat(const char*);
int h_remove(const char*);
int h_rename(const char*, const char*);
int h_mkdir(const char*);
int h_dir_open(const char*);
int h_dir_next(int, char*, int);
void h_dir_close(int);
void h_gfx_size(int*, int*);
void h_gfx_begin(uint32_t);
void h_gfx_end(void);
int h_tex_create(const void*, int, int, int);
void h_tex_update(int, const void*);
void h_tex_free(int);
void h_gfx_draw(int, const void*, int);
void h_gfx_clip(int, int, int, int);
void h_gfx_blend(int);
int h_audio_open(int);
void h_audio_push(const int16_t*, uint32_t);
uint32_t h_audio_queued(void);
void h_audio_clear(void);
void h_audio_ratio(double);
int h_pad_poll(HostPad*, int);
int h_quit_requested(void);
double h_sin(double);
double h_cos(double);
double h_tan(double);
double h_atan2(double, double);
double h_pow(double, double);
double h_exp(double);
double h_log_e(double);
double h_fmod(double, double);
float h_sinf(float);
float h_cosf(float);
float h_powf(float, float);
void h_set_title(const char*);
void h_notify(const char*);
int h_touch_poll(HostTouch* fingers, int max);
int h_app_state(void);
int h_hd_music(int op, const char* path, int gain);
int h_hd_movie(const char* movie, const char* subs1, const char* subs2, const char* font);
int h_udp_open(int bind_port);
int h_udp_resolve(const char* host, int port, uint8_t* addr6);
void h_udp_send(const uint8_t* addr6, const void* data, int size);
int h_udp_recv(void* data, int cap, uint8_t* addr6);
#endif
