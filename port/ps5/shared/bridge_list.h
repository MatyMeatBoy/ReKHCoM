/* Host functions the guest may call, in table order (see bridge.h). The guest
 * stub of entry N loads N into eax and jumps to the trampoline; the host
 * table holds the function of entry N. Append only: both sides include this. */
X(log)        /* void (const char* text) */
X(exit)       /* void (int code) */
X(ticks_us)   /* uint64_t (void) */
X(sleep_us)   /* void (uint32_t us) */
X(getenv)     /* int (const char* name, char* out, int cap): length, -1 unset */
X(fopen)      /* int (const char* path, int mode): handle >= 0, -1; mode 0 read, 1 write, 2 append, 3 read/write */
X(fread)      /* int (int h, void* buf, uint32_t n) */
X(fwrite)     /* int (int h, const void* buf, uint32_t n) */
X(fseek)      /* int (int h, int32_t off, int whence) */
X(ftell)      /* int (int h) */
X(fclose)     /* int (int h) */
X(fflush)     /* int (int h) */
X(stat)       /* int (const char* path): 0 absent, 1 file, 2 directory */
X(remove)     /* int (const char* path) */
X(rename)     /* int (const char* from, const char* to) */
X(mkdir)      /* int (const char* path) */
X(dir_open)   /* int (const char* path) */
X(dir_next)   /* int (int d, char* name, int cap): 1 an entry, 0 the end */
X(dir_close)  /* void (int d) */
X(gfx_size)   /* void (int* w, int* h) */
X(gfx_begin)  /* void (uint32_t clear_argb) */
X(gfx_end)    /* void (void) */
X(tex_create) /* int (const void* rgba, int w, int h, int smooth): id, -1 */
X(tex_update) /* void (int id, const void* rgba) */
X(tex_free)   /* void (int id) */
X(gfx_draw)   /* void (int id, const void* vertices, int count): HostVertex triangles, id -1 none */
X(gfx_clip)   /* void (int x, int y, int w, int h): w <= 0 off */
X(gfx_blend)  /* void (int on) */
X(audio_open) /* int (int rate) */
X(audio_push) /* void (const int16_t* frames, uint32_t count) */
X(audio_queued) /* uint32_t (void): frames waiting */
X(audio_clear) /* void (void) */
X(audio_ratio) /* void (double ratio) */
X(pad_poll)   /* int (HostPad* pads, int count): pads filled */
X(quit_requested) /* int (void) */
X(sin)        /* double (double) */
X(cos)        /* double (double) */
X(tan)        /* double (double) */
X(atan2)      /* double (double, double) */
X(pow)        /* double (double, double) */
X(exp)        /* double (double) */
X(log_e)      /* double (double) */
X(fmod)       /* double (double, double) */
X(sinf)       /* float (float) */
X(cosf)       /* float (float) */
X(powf)       /* float (float, float) */
X(set_title)  /* void (const char*): unused where there is no window */
X(notify)     /* void (const char* name): the app draws the box assets/<name>.rgba over the game for five seconds */
X(touch_poll) /* int (HostTouch* fingers, int max): fingers on the screen now (Android 64-bit), 0 elsewhere */
X(app_state)  /* int (void): 1 while the app is in the background (Android 64-bit), 0 elsewhere */
X(hd_music)   /* int (int op, const char* path, int gain): HD music (shared/hd_music_host.h): 0 open path, 1 play at gain/1000, 2 pause, 3 close */
X(hd_movie)   /* int (const char* movie, const char* subs1, const char* subs2, const char* font): plays an HD movie to the end (Android 64-bit, port/portable/hdmovie_android.c); 0 elsewhere */
X(udp_open)   /* int (int bind_port): 1 a non-blocking UDP socket for the network link (shared/udp_host.h), 0 none */
X(udp_resolve) /* int (const char* host, int port, uint8_t* addr6): 1 found; addr6 = IPv4 + port, network order */
X(udp_send)   /* void (const uint8_t* addr6, const void* data, int size) */
X(udp_recv)   /* int (void* data, int cap, uint8_t* addr6): one datagram's bytes, 0 none waiting */
