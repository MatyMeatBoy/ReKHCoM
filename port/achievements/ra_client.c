#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <wincrypt.h>
#else
#include <stddef.h>
/* the Android launcher (LAUNCHER_ANDROID): the app's private folder keeps the sign-in, and no request reaches the
   server (the Android game has no online achievements: the official set is earned offline there) */
static void SecureZeroMemory(void* p, size_t n) {
    volatile unsigned char* b = (volatile unsigned char*)p;
    while (n--)
        *b++ = 0;
}
#define _strdup strdup
#endif
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

#include "ra_client.h"
#include "../third_party/rcheevos/src/rc_client_internal.h"
#include "rc_consoles.h"
#include "rc_hash.h"
#include "rc_api_user.h"
#include "rc_api_runtime.h"

/* Linux handhelds and desktops talk to the server themselves (ra_https_linux.inc); Android stays offline. */
#if !defined(_WIN32) && defined(PORT_LINUX) && !defined(__ANDROID__)
#define RA_LINUX_ONLINE 1
#endif

typedef struct KhRaJob {
    struct KhRaJob* next;
    char *url, *post, *content_type, *body;
    size_t body_size;
    int status;
    SDL_AtomicInt done;
    SDL_Thread* thread;
    rc_client_server_callback_t callback;
    void* callback_data;
} KhRaJob;

struct KhRa {
    rc_client_t* client;
    KhRaRead read;
    void* userdata;
    KhRaTransport transport;
    KhRaJob* jobs;
    char directory[1024], hash[33], status[512], notice[512];
    uint32_t notice_id;
    char username[128], token[128];
    int enabled, busy, error, remember, closing, invalidated;
    uint32_t *account_unlocks, account_unlock_count;
    unsigned generation;
    int account_progress_available;
    const uint32_t* official; /* KhRaStartOfficial: the only achievements evaluated */
    uint32_t official_count;
    /* sign-in from the card (RA_ACCOUNT_FILE) and retries while there is no network */
    char password[256];
    int account_file, retry;
    uint64_t next_try;
};

static void SetStatus(KhRa* ra, const char* text, int error) {
    snprintf(ra->status, sizeof(ra->status), "%s", text ? text : "Unknown error");
    ra->error = error;
#ifdef RA_LINUX_ONLINE
    {
        /* what the player can read on the card (no secrets: the account name at most) */
        char path[1200];
        FILE* file;
        snprintf(path, sizeof(path), "%s/retroachievements_estado.txt", ra->directory);
        if ((file = fopen(path, "w"))) {
            time_t now = time(NULL);
            fprintf(file, "%s%s%s\n%s\n", ra->username[0] ? "Cuenta: " : "", ra->username,
                    ra->username[0] ? "\n" : "", ra->status);
            fprintf(file, "Hora de la consola: %s", ctime(&now));
            fclose(file);
        }
    }
#endif
}

#ifdef _WIN32
static wchar_t* Wide(const char* text) {
    int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0);
    wchar_t* result;
    if (size <= 0) return NULL;
    result = (wchar_t*)calloc((size_t)size, sizeof(wchar_t));
    if (result) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, result, size);
    return result;
}

static FILE* DataFile(KhRa* ra, const char* name, const wchar_t* mode) {
    char path[1200];
    wchar_t* wide;
    FILE* file;
    snprintf(path, sizeof(path), "%s/%s", ra->directory, name);
    wide = Wide(path);
    if (!wide) return NULL;
    file = _wfopen(wide, mode);
    free(wide);
    return file;
}
#else
static FILE* DataFile(KhRa* ra, const char* name, const char* mode) {
    char path[1200];
    snprintf(path, sizeof(path), "%s/%s", ra->directory, name);
    return fopen(path, mode);
}
#endif
#ifdef _WIN32
#define RA_MODE(m) L##m
#else
#define RA_MODE(m) m
#endif

/* The session of this computer's user (25/10): the last account signed in with any ReKHCoM on this PC, for a year, so a
 * portable copy elsewhere (another disk) signs in by itself. A copy with its own account keeps it (it wins) and that
 * becomes the last one. Per user of the system: %APPDATA%\ReKHCoM (DPAPI, this Windows user only) or
 * ~/.config/ReKHCoM (0600). Cleared on a sign-out of the same account. */
#define RA_CACHE_DAYS 365
typedef struct { char user[128], token[128]; long long saved; } RaCached;

static int CachePath(char* out, size_t size) {
#ifdef __ANDROID__
    (void)out; (void)size;
    return 0; /* the phone's app keeps one data folder: nothing to share */
#endif
#ifdef _WIN32
    const char* base = getenv("APPDATA");
    if (!base || !base[0]) return 0;
    snprintf(out, size, "%s\\ReKHCoM", base);
    _mkdir(out);
    snprintf(out, size, "%s\\ReKHCoM\\retroachievements_last.auth", base);
#else
    const char* xdg = getenv("XDG_CONFIG_HOME");
    const char* home = getenv("HOME");
    char dir[900];
    if (xdg && xdg[0]) snprintf(dir, sizeof(dir), "%s/ReKHCoM", xdg);
    else if (home && home[0]) snprintf(dir, sizeof(dir), "%s/.config/ReKHCoM", home);
    else return 0;
    mkdir(dir, 0700);
    snprintf(out, size, "%s/retroachievements_last.auth", dir);
#endif
    return 1;
}

static FILE* CacheFile(const char* mode) {
    char path[1024];
    if (!CachePath(path, sizeof(path))) return NULL;
#ifdef _WIN32
    {
        wchar_t* wide = Wide(path), wmode[8];
        FILE* f;
        if (!wide) return NULL;
        MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, 8);
        f = _wfopen(wide, wmode);
        free(wide);
        return f;
    }
#else
    {
        FILE* f = fopen(path, mode);
        if (f && mode[0] == 'w') chmod(path, 0600);
        return f;
    }
#endif
}

static void CacheWrite(const char* user, const char* token) {
    RaCached c;
    FILE* f;
    memset(&c, 0, sizeof(c));
    snprintf(c.user, sizeof(c.user), "%s", user);
    snprintf(c.token, sizeof(c.token), "%s", token);
    c.saved = (long long)time(NULL);
#ifdef _WIN32
    {
        DATA_BLOB in, out = {0};
        in.pbData = (BYTE*)&c; in.cbData = sizeof(c);
        if (CryptProtectData(&in, L"ReKHCoM RetroAchievements (this PC)", NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &out)) {
            if ((f = CacheFile("wb")) != NULL) { fwrite(out.pbData, 1, out.cbData, f); fclose(f); }
            SecureZeroMemory(out.pbData, out.cbData);
            LocalFree(out.pbData);
        }
    }
#else
    if ((f = CacheFile("wb")) != NULL) { fwrite(&c, 1, sizeof(c), f); fclose(f); }
#endif
    SecureZeroMemory(&c, sizeof(c));
}

/* 1: the last account of this PC's user, still within its year */
static int CacheRead(RaCached* c) {
    FILE* f = CacheFile("rb");
    int ok = 0;
    memset(c, 0, sizeof(*c));
    if (!f) return 0;
#ifdef _WIN32
    {
        BYTE bytes[4096];
        size_t size = fread(bytes, 1, sizeof(bytes), f);
        DATA_BLOB in, out = {0};
        in.pbData = bytes; in.cbData = (DWORD)size;
        if (size && size < sizeof(bytes) && CryptUnprotectData(&in, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &out)) {
            if (out.cbData == sizeof(*c)) { memcpy(c, out.pbData, sizeof(*c)); ok = 1; }
            SecureZeroMemory(out.pbData, out.cbData);
            LocalFree(out.pbData);
        }
        SecureZeroMemory(bytes, sizeof(bytes));
    }
#else
    ok = fread(c, 1, sizeof(*c), f) == sizeof(*c);
#endif
    fclose(f);
    ok = ok && c->user[0] && c->token[0] && c->user[127] == 0 && c->token[127] == 0 &&
         (long long)time(NULL) - c->saved < (long long)RA_CACHE_DAYS * 86400;
    if (!ok) SecureZeroMemory(c, sizeof(*c));
    return ok;
}

static void CacheForget(const char* user) {
    RaCached c;
    if (CacheRead(&c) && strcmp(c.user, user) == 0) {
        FILE* f = CacheFile("wb");
        if (f) fclose(f);
    }
    SecureZeroMemory(&c, sizeof(c));
}

static void ForgetToken(KhRa* ra) {
    FILE* file = DataFile(ra, "retroachievements.auth", RA_MODE("wb"));
    if (file) fclose(file);
    if (ra->username[0]) CacheForget(ra->username); /* signed out: this PC forgets it too */
    SecureZeroMemory(ra->token, sizeof(ra->token));
    SecureZeroMemory(ra->username, sizeof(ra->username));
}

static int SaveTokenFile(KhRa* ra);
static int SaveToken(KhRa* ra) {
    if (ra->username[0] && ra->token[0]) CacheWrite(ra->username, ra->token);
    return SaveTokenFile(ra);
}

static int SaveTokenFile(KhRa* ra) {
    struct { char user[128], token[128]; } credentials;
#ifndef _WIN32
    FILE* plain;
    int saved = 0;
    memset(&credentials, 0, sizeof(credentials));
    snprintf(credentials.user, sizeof(credentials.user), "%s", ra->username);
    snprintf(credentials.token, sizeof(credentials.token), "%s", ra->token);
    plain = DataFile(ra, "retroachievements.auth", "wb");
    if (plain) { saved = fwrite(&credentials, 1, sizeof(credentials), plain) == sizeof(credentials); fclose(plain); }
    SecureZeroMemory(&credentials, sizeof(credentials));
    return saved;
#else
    DATA_BLOB in, out = {0};
    FILE* file;
    int ok = 0;
    memset(&credentials, 0, sizeof(credentials));
    snprintf(credentials.user, sizeof(credentials.user), "%s", ra->username);
    snprintf(credentials.token, sizeof(credentials.token), "%s", ra->token);
    in.pbData = (BYTE*)&credentials;
    in.cbData = sizeof(credentials);
    if (CryptProtectData(&in, L"ReKHCoM RetroAchievements", NULL, NULL, NULL,
                         CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        file = DataFile(ra, "retroachievements.auth", RA_MODE("wb"));
        if (file) { ok = fwrite(out.pbData, 1, out.cbData, file) == out.cbData; fclose(file); }
        SecureZeroMemory(out.pbData, out.cbData);
        LocalFree(out.pbData);
    }
    SecureZeroMemory(&credentials, sizeof(credentials));
    return ok;
#endif
}

static void ReadToken(KhRa* ra) {
#ifndef _WIN32
    unsigned char plain[256];
    FILE* in = DataFile(ra, "retroachievements.auth", "rb");
    if (!in) return;
    if (fread(plain, 1, sizeof(plain), in) == sizeof(plain) && plain[127] == 0 && plain[255] == 0) {
        memcpy(ra->username, plain, 128);
        memcpy(ra->token, plain + 128, 128);
        ra->remember = 1;
    }
    fclose(in);
    SecureZeroMemory(plain, sizeof(plain));
#else
    BYTE bytes[4096];
    DATA_BLOB in, out = {0};
    FILE* file = DataFile(ra, "retroachievements.auth", RA_MODE("rb"));
    size_t size;
    if (!file) return;
    size = fread(bytes, 1, sizeof(bytes), file);
    fclose(file);
    in.pbData = bytes;
    in.cbData = (DWORD)size;
    if (size && size < sizeof(bytes) && CryptUnprotectData(&in, NULL, NULL, NULL, NULL,
            CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        if (out.cbData == 256 && out.pbData[127] == 0 && out.pbData[255] == 0) {
            memcpy(ra->username, out.pbData, 128);
            memcpy(ra->token, out.pbData + 128, 128);
            ra->remember = 1;
        }
        SecureZeroMemory(out.pbData, out.cbData);
        LocalFree(out.pbData);
    }
    SecureZeroMemory(bytes, sizeof(bytes));
#endif
}

#ifdef RA_LINUX_ONLINE
#include "ra_https_linux.inc"
#endif

/* Workers only fill their response. rcheevos callbacks are dispatched by Pump
 * on the owning thread, so UI access and frame evaluation cannot race. */
static int HttpWorker(void* userdata) {
    KhRaJob* job = (KhRaJob*)userdata;
#ifndef _WIN32
#ifdef RA_LINUX_ONLINE
    RaHttpsLinux(job);
#else
    job->status = RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR; /* no online requests here (see the top) */
#endif
    SDL_SetAtomicInt(&job->done, 1);
    return 0;
#else
    wchar_t *url = Wide(job->url), *content_type = NULL;
    HINTERNET session = NULL, connect = NULL, request = NULL;
    URL_COMPONENTS parts;
    wchar_t host[256], path[2048];
    DWORD status_size = sizeof(DWORD), status = 0, redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    job->status = RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR;
    memset(&parts, 0, sizeof(parts));
    parts.dwStructSize = sizeof(parts);
    parts.lpszHostName = host; parts.dwHostNameLength = 255;
    parts.lpszUrlPath = path; parts.dwUrlPathLength = 2047;
    if (!url || !WinHttpCrackUrl(url, 0, 0, &parts) || parts.nScheme != INTERNET_SCHEME_HTTPS)
        goto done;
    host[parts.dwHostNameLength] = 0;
    path[parts.dwUrlPathLength] = 0;
    if (_wcsicmp(host, L"retroachievements.org") != 0 &&
        _wcsicmp(host, L"media.retroachievements.org") != 0 &&
        _wcsicmp(host, L"static.retroachievements.org") != 0) goto done;
    session = WinHttpOpen(L"ReKHCoM/1.9.3 rcheevos/12.5", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                          WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) goto done;
    WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);
    connect = WinHttpConnect(session, host, parts.nPort, 0);
    if (!connect) goto done;
    request = WinHttpOpenRequest(connect, job->post ? L"POST" : L"GET", path, NULL,
                  WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request) goto done;
    WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect));
    if (job->content_type) {
        char header[256];
        snprintf(header, sizeof(header), "Content-Type: %s\r\n", job->content_type);
        content_type = Wide(header);
    }
    if (!WinHttpSendRequest(request, content_type ? content_type : WINHTTP_NO_ADDITIONAL_HEADERS,
            content_type ? (DWORD)-1 : 0, job->post, job->post ? (DWORD)strlen(job->post) : 0,
            job->post ? (DWORD)strlen(job->post) : 0, 0) || !WinHttpReceiveResponse(request, NULL)) goto done;
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX)) goto done;
    for (;;) {
        DWORD available = 0, read = 0;
        char* grown;
        if (!WinHttpQueryDataAvailable(request, &available)) goto done;
        if (!available) break;
        if (available > (8u << 20) || job->body_size + available > (8u << 20)) goto done;
        grown = (char*)realloc(job->body, job->body_size + available + 1);
        if (!grown) goto done;
        job->body = grown;
        if (!WinHttpReadData(request, job->body + job->body_size, available, &read)) goto done;
        if (!read) break;
        job->body_size += read;
        job->body[job->body_size] = 0;
    }
    job->status = (int)status;
done:
    if (request) WinHttpCloseHandle(request);
    if (connect) WinHttpCloseHandle(connect);
    if (session) WinHttpCloseHandle(session);
    free(content_type); free(url);
    SDL_SetAtomicInt(&job->done, 1);
    return 0;
#endif
}

static void ServerCall(const rc_api_request_t* request, rc_client_server_callback_t callback,
                       void* callback_data, rc_client_t* client) {
    KhRa* ra = (KhRa*)rc_client_get_userdata(client);
    KhRaJob *job, **tail;
    if (ra->transport && !ra->closing) { ra->transport(request, callback, callback_data); return; }
    job = ra->closing ? NULL : (KhRaJob*)calloc(1, sizeof(*job));
    if (!job) {
        rc_api_server_response_t response = {"", 0, RC_API_SERVER_RESPONSE_CLIENT_ERROR};
        callback(&response, callback_data);
        return;
    }
    job->url = _strdup(request->url);
    job->post = request->post_data ? _strdup(request->post_data) : NULL;
    job->content_type = request->content_type ? _strdup(request->content_type) : NULL;
    job->callback = callback; job->callback_data = callback_data;
    if (!job->url || (request->post_data && !job->post)) {
        job->status = RC_API_SERVER_RESPONSE_CLIENT_ERROR;
        SDL_SetAtomicInt(&job->done, 1);
    }
    tail = &ra->jobs;
    while (*tail) tail = &(*tail)->next;
    *tail = job;
}

static uint32_t ReadMemory(uint32_t address, uint8_t* out, uint32_t size, rc_client_t* client) {
    KhRa* ra = (KhRa*)rc_client_get_userdata(client);
    return ra->read && !ra->invalidated ? ra->read(address, out, size, ra->userdata) : 0;
}

typedef struct KhRaImageRequest { KhRaImageCallback callback; void* userdata; } KhRaImageRequest;
static void ImageResponse(const rc_api_server_response_t* response, void* userdata) {
    KhRaImageRequest* request = (KhRaImageRequest*)userdata;
    request->callback((const uint8_t*)response->body, response->body_length,
                      response->http_status_code, request->userdata);
    free(request);
}
int KhRaFetchImage(KhRa* ra, const char* url, KhRaImageCallback callback, void* userdata) {
    rc_api_request_t api;
    KhRaImageRequest* request;
    if (!ra || !ra->enabled || ra->closing || !url || !callback) return 0;
    request = (KhRaImageRequest*)calloc(1, sizeof(*request));
    if (!request) return 0;
    memset(&api, 0, sizeof(api)); api.url = url;
    request->callback = callback; request->userdata = userdata;
    ServerCall(&api, ImageResponse, request, ra->client);
    return 1;
}



/* Unlocks made while signed in but without a connection (a phone in the underground, 25/10): rc_client retries them

 * only while the game runs. Each one is also kept in retroachievements_pending.txt ("id unix-time" per line, the

 * account's name first) until the account shows it; the next start with a connection sends the rest with their

 * seconds since the unlock, as RetroAchievements takes a client's queued offline unlocks. Unlocks earned with no

 * account signed in never go here (RetroAchievements' rules). */

#define RA_PENDING "retroachievements_pending.txt"
#define RA_ACCOUNT_FILE "retroachievements_cuenta.txt"

#define RA_PENDING_MAX 128

typedef struct { uint32_t id; long long when; } RaPendingEntry;



static int PendingRead(KhRa* ra, RaPendingEntry* out, char* user, size_t userSize) {

    FILE* f = DataFile(ra, RA_PENDING, RA_MODE("r"));

    char line[160];

    int n = 0;

    user[0] = 0;

    if (!f) return 0;

    if (fgets(line, sizeof(line), f)) {

        line[strcspn(line, "\r\n")] = 0;

        snprintf(user, userSize, "%s", line);

    }

    while (n < RA_PENDING_MAX && fgets(line, sizeof(line), f)) {

        unsigned id; long long when;

        if (sscanf(line, "%u %lld", &id, &when) == 2 && id) { out[n].id = id; out[n].when = when; n++; }

    }

    fclose(f);

    return n;

}



static void PendingWrite(KhRa* ra, const RaPendingEntry* e, int n, const char* user) {

    FILE* f = DataFile(ra, RA_PENDING, RA_MODE("w"));

    int i;

    if (!f) return;

    if (n > 0) {

        fprintf(f, "%s\n", user);

        for (i = 0; i < n; i++) fprintf(f, "%u %lld\n", e[i].id, e[i].when);

    }

    fclose(f);

}



static void PendingAdd(KhRa* ra, uint32_t id) {

    RaPendingEntry e[RA_PENDING_MAX];

    char user[128];

    int n = PendingRead(ra, e, user, sizeof(user)), i;

    const rc_client_user_t* me = rc_client_get_user_info(ra->client);

    if (!me || !me->username) return;

    if (n && strcmp(user, me->username) != 0) n = 0; /* another account's queue: not this one's to send */

    for (i = 0; i < n; i++) if (e[i].id == id) return;

    if (n == RA_PENDING_MAX) return;

    e[n].id = id; e[n].when = (long long)time(NULL); n++;

    PendingWrite(ra, e, n, me->username);

}



static void PendingDrop(KhRa* ra, uint32_t id) {

    RaPendingEntry e[RA_PENDING_MAX];

    char user[128];

    int n = PendingRead(ra, e, user, sizeof(user)), i, k = 0;

    for (i = 0; i < n; i++) if (e[i].id != id) e[k++] = e[i];

    if (k != n) PendingWrite(ra, e, k, user);

}



typedef struct PendingRequest { KhRa* ra; uint32_t id; } PendingRequest;

static void PendingAwarded(const rc_api_server_response_t* response, void* userdata) {

    PendingRequest* request = (PendingRequest*)userdata;

    rc_api_award_achievement_response_t award;

    memset(&award, 0, sizeof(award));

    /* sent (or already on the account): off the queue; a network error leaves it for the next start */

    if (rc_api_process_award_achievement_server_response(&award, response) == RC_OK &&

        (award.response.succeeded || award.awarded_achievement_id == request->id))

        PendingDrop(request->ra, request->id);

    rc_api_destroy_award_achievement_response(&award);

    free(request);

}



/* with the account's unlocks just read: drop what it has, send the rest */

static void PendingFlush(KhRa* ra) {

    RaPendingEntry e[RA_PENDING_MAX];

    char user[128];

    int n = PendingRead(ra, e, user, sizeof(user)), i, k = 0;

    const rc_client_user_t* me = rc_client_get_user_info(ra->client);

    long long now = (long long)time(NULL);

    if (!n || !me || !me->username || strcmp(user, me->username) != 0 || !ra->hash[0]) return;

    for (i = 0; i < n; i++) {

        uint32_t j, have = 0;

        for (j = 0; j < ra->account_unlock_count; ++j) have |= ra->account_unlocks[j] == e[i].id;

        if (have) continue;

        e[k++] = e[i];

    }

    PendingWrite(ra, e, k, user);

    for (i = 0; i < k; i++) {

        rc_api_award_achievement_request_t params;

        rc_api_request_t api;

        PendingRequest* request = (PendingRequest*)calloc(1, sizeof(*request));

        long long ago = now - e[i].when;

        if (!request) break;

        memset(&params, 0, sizeof(params));

        params.username = me->username; params.api_token = me->token;

        params.achievement_id = e[i].id; params.hardcore = 0; params.game_hash = ra->hash;

        params.seconds_since_unlock = ago < 0 ? 0 : ago > 0x7FFFFFFF ? 0x7FFFFFFF : (uint32_t)ago;

        request->ra = ra; request->id = e[i].id;

        if (rc_api_init_award_achievement_request(&api, &params) == RC_OK)

            ServerCall(&api, PendingAwarded, request, ra->client);

        else free(request);

        rc_api_destroy_request(&api);

    }

}



static void Event(const rc_client_event_t* event, rc_client_t* client) {
    KhRa* ra = (KhRa*)rc_client_get_userdata(client);
    if (event->type == RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED && event->achievement) {
        ra->notice_id = event->achievement->id;
        if (!rc_client_get_spectator_mode_enabled(client) && rc_client_get_user_info(client))
            PendingAdd(ra, event->achievement->id); /* until the account shows it (a phone without signal) */
        snprintf(ra->notice, sizeof(ra->notice), "%s\n%s", event->achievement->title,
                 event->achievement->description ? event->achievement->description : "");
    } else if (event->type == RC_CLIENT_EVENT_SERVER_ERROR && event->server_error)
        SetStatus(ra, event->server_error->error_message, 1);
}

typedef struct ProgressRequest { KhRa* ra; unsigned generation; } ProgressRequest;

static void AccountProgress(const rc_api_server_response_t* response, void* userdata) {
    ProgressRequest* request = (ProgressRequest*)userdata;
    KhRa* ra = request->ra;
    rc_api_fetch_user_unlocks_response_t progress;
    memset(&progress, 0, sizeof(progress));
    if (request->generation == ra->generation) {
        int result = rc_api_process_fetch_user_unlocks_server_response(&progress, response);
        if (result == RC_OK && progress.response.succeeded) {
            free(ra->account_unlocks); ra->account_unlocks = NULL;
            ra->account_unlock_count = progress.num_achievement_ids;
            if (progress.num_achievement_ids) {
                ra->account_unlocks = (uint32_t*)malloc(progress.num_achievement_ids * sizeof(uint32_t));
                if (ra->account_unlocks) memcpy(ra->account_unlocks, progress.achievement_ids,
                                               progress.num_achievement_ids * sizeof(uint32_t));
                else ra->account_unlock_count = 0;
            }
            ra->account_progress_available = !progress.num_achievement_ids || ra->account_unlocks != NULL;
            if (ra->account_progress_available) PendingFlush(ra);
        } else SetStatus(ra, "Catalog loaded; account progress could not be retrieved.", 1);
    }
    rc_api_destroy_fetch_user_unlocks_response(&progress);
    free(request);
}

/* Achievements whose memory reads are not verified against the cartridge yet
 * are disabled as rcheevos disables an unsupported one: they stay in the
 * list, never trigger and never submit. */
static void RestrictOfficial(KhRa* ra) {
    rc_client_game_info_t* game = ra->client->game;
    rc_client_subset_info_t* subset;
    if (!ra->official || !game) return;
    for (subset = game->subsets; subset; subset = subset->next) {
        rc_client_achievement_info_t* a = subset->achievements;
        rc_client_achievement_info_t* end = a + subset->public_.num_achievements;
        for (; a < end; ++a) {
            uint32_t i, ok = 0;
            for (i = 0; i < ra->official_count; ++i) ok |= ra->official[i] == a->public_.id;
            if (ok || a->public_.state == RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED) continue;
            a->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED;
            a->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;
            if (a->trigger) a->trigger->state = RC_TRIGGER_STATE_DISABLED;
        }
    }
}

static void Loaded(int result, const char* error, rc_client_t* client, void* userdata) {
    KhRa* ra = (KhRa*)userdata;
    ra->busy = 0;
    if (result == RC_OK) RestrictOfficial(ra);
    SetStatus(ra, result == RC_OK ? "Catalog loaded. Validation mode: online awards disabled." : error,
              result != RC_OK);
    if (result == RC_OK && !ra->closing) {
        const rc_client_user_t* user = rc_client_get_user_info(client);
        const rc_client_game_t* game = rc_client_get_game_info(client);
        rc_api_request_t api_request;
        rc_api_fetch_user_unlocks_request_t params;
        ProgressRequest* request = (ProgressRequest*)calloc(1, sizeof(*request));
        if (!user || !game || !request) { free(request); return; }
        memset(&params, 0, sizeof(params));
        params.username = user->username; params.api_token = user->token;
        params.game_id = game->id; params.hardcore = 0;
        request->ra = ra; request->generation = ra->generation;
        if (rc_api_init_fetch_user_unlocks_request(&api_request, &params) == RC_OK)
            ServerCall(&api_request, AccountProgress, request, client);
        else free(request);
        rc_api_destroy_request(&api_request);
    }
}

static void LoadGame(KhRa* ra) {
    if (!ra->enabled || !rc_client_get_user_info(ra->client) || !ra->hash[0] || ra->invalidated) return;
    if (strcmp(ra->hash, "d369f791d86a969778d2c3a5278d544c") != 0) {
        SetStatus(ra, "This ROM is not supported by the official set. Local achievements remain available.", 0);
        return;
    }
    ra->busy = 1;
    SetStatus(ra, "Loading official catalog...", 0);
    rc_client_begin_load_game(ra->client, ra->hash, Loaded, ra);
}

static void LoggedIn(int result, const char* error, rc_client_t* client, void* userdata) {
    KhRa* ra = (KhRa*)userdata;
    ra->busy = 0;
    if (result != RC_OK) {
        /* no network yet (a handheld whose Wi-Fi comes later): try again in a minute; a refused account does not */
        ra->retry = result != RC_INVALID_CREDENTIALS && result != RC_EXPIRED_TOKEN && result != RC_ACCESS_DENIED;
        ra->next_try = SDL_GetTicks() + 60000;
        if (!ra->retry && ra->password[0]) { SecureZeroMemory(ra->password, sizeof(ra->password)); ra->account_file = 0; }
        #ifdef RA_LINUX_ONLINE
        if (ra->retry && sRaSslTried && !sRaSsl.ready) {
            SetStatus(ra, "No se pudo cargar OpenSSL (libssl.so.3): sin conexion segura.", 1);
            return;
        }
#endif
        SetStatus(ra, ra->retry ? "Sin conexion con RetroAchievements: se reintenta cada minuto." : error, 1);
        return;
    }
    ra->retry = 0;
    SecureZeroMemory(ra->password, sizeof(ra->password));
#ifdef RA_LINUX_ONLINE
    if (ra->account_file) {
        /* signed in: the password leaves the card; the token (retroachievements.auth) keeps the session */
        FILE* file = DataFile(ra, RA_ACCOUNT_FILE, "w");
        if (file) {
            fprintf(file, "usuario=%s\n# Sesion iniciada: la contrasena se borro de este archivo.\n",
                    rc_client_get_user_info(client)->username);
            fclose(file);
        }
        ra->account_file = 0;
    }
#endif
    {
        const rc_client_user_t* user = rc_client_get_user_info(client);
        snprintf(ra->username, sizeof(ra->username), "%s", user->username);
        snprintf(ra->token, sizeof(ra->token), "%s", user->token);
        if (ra->remember && !SaveToken(ra))
            SetStatus(ra, "Connected. Could not save the encrypted token.", 1);
        else SetStatus(ra, "Connected to RetroAchievements.", 0);
    }
    LoadGame(ra);
}

#ifdef RA_LINUX_ONLINE
/* A handheld has no launcher to type the account in: the player writes it once in this file on the card
 *     usuario=NAME
 *     contrasena=PASSWORD
 * The game signs in, keeps the session token (retroachievements.auth) and rewrites the file without the password. */
static void AccountFile(KhRa* ra) {
    char line[512], user[128] = "", pass[256] = "";
    FILE* file = DataFile(ra, RA_ACCOUNT_FILE, "r");
    if (!file) return;
    while (fgets(line, sizeof(line), file)) {
        char* value = strchr(line, '=');
        size_t n;
        if (!value || line[0] == '#') continue;
        *value++ = 0;
        n = strcspn(value, "\r\n");
        value[n] = 0;
        if (!strcmp(line, "usuario") || !strcmp(line, "user") || !strcmp(line, "username"))
            snprintf(user, sizeof(user), "%s", value);
        else if (!strcmp(line, "contrasena") || !strcmp(line, "contraseña") || !strcmp(line, "password"))
            snprintf(pass, sizeof(pass), "%s", value);
    }
    fclose(file);
    SecureZeroMemory(line, sizeof(line));
    if (user[0] && pass[0]) {
        FILE* ini = DataFile(ra, "retroachievements.ini", "w");
        if (ini) { fprintf(ini, "enabled=1\n"); fclose(ini); }
        ra->enabled = 1;
        ra->remember = 1;
        ra->account_file = 1;
        snprintf(ra->username, sizeof(ra->username), "%s", user);
        snprintf(ra->password, sizeof(ra->password), "%s", pass);
        ra->token[0] = 0;
    }
    SecureZeroMemory(pass, sizeof(pass));
}
#endif

KhRa* KhRaCreate(const char* directory, KhRaRead read, void* userdata, KhRaTransport transport) {
    KhRa* ra = (KhRa*)calloc(1, sizeof(*ra));
    FILE* file;
    if (!ra) return NULL;
    snprintf(ra->directory, sizeof(ra->directory), "%s", directory);
    ra->read = read; ra->userdata = userdata; ra->transport = transport;
    ra->client = rc_client_create(ReadMemory, ServerCall);
    if (!ra->client) { free(ra); return NULL; }
    rc_client_set_userdata(ra->client, ra);
    rc_client_set_hardcore_enabled(ra->client, 0);
    rc_client_set_spectator_mode_enabled(ra->client, 1);
    /* All network completions run in Pump on the owning thread. Let loading
     * finish there even for a catalog-only client that never calls do_frame. */
    rc_client_set_allow_background_memory_reads(ra->client, 1);
    rc_client_set_event_handler(ra->client, Event);
    SetStatus(ra, "Not connected", 0);
    file = DataFile(ra, "retroachievements.ini", RA_MODE("r"));
    if (file) { int value = 0; if (fscanf(file, "enabled=%d", &value) == 1) ra->enabled = value == 1; fclose(file); }
    ReadToken(ra);
    if (!ra->token[0]) {
        RaCached c;
        if (CacheRead(&c)) {
            FILE* own = DataFile(ra, "retroachievements.ini", RA_MODE("r"));
            snprintf(ra->username, sizeof(ra->username), "%s", c.user);
            snprintf(ra->token, sizeof(ra->token), "%s", c.token);
            ra->remember = 1;
            if (own) fclose(own); /* the copy said on or off: it keeps that */
            else ra->enabled = 1; /* a new copy: signed in like the rest of this PC */
            SecureZeroMemory(&c, sizeof(c));
        }
    } else if (ra->remember) {
        CacheWrite(ra->username, ra->token); /* its own account wins and becomes this PC's last one */
    }
#ifdef RA_LINUX_ONLINE
    AccountFile(ra);
#endif
    if (ra->enabled && ra->password[0]) {
        ra->busy = 1;
        SetStatus(ra, "Iniciando sesion...", 0);
        rc_client_begin_login_with_password(ra->client, ra->username, ra->password, LoggedIn, ra);
    } else if (ra->enabled && ra->token[0]) {
        ra->busy = 1;
        SetStatus(ra, "Signing in...", 0);
        rc_client_begin_login_with_token(ra->client, ra->username, ra->token, LoggedIn, ra);
    }
    return ra;
}

void KhRaPump(KhRa* ra, int evaluate) {
    KhRaJob **link, *ready = NULL;
    int active = 0;
    if (!ra) return;
    link = &ra->jobs;
    while (*link) {
        KhRaJob* job = *link;
        if (SDL_GetAtomicInt(&job->done)) { *link = job->next; job->next = ready; ready = job; }
        else { if (job->thread) ++active; link = &job->next; }
    }
    while (ready) {
        KhRaJob* job = ready;
        rc_api_server_response_t response;
        ready = ready->next;
        if (job->thread) SDL_WaitThread(job->thread, NULL);
        response.body = job->body ? job->body : "";
        response.body_length = job->body_size; response.http_status_code = job->status;
        job->callback(&response, job->callback_data);
        if (job->post) { SecureZeroMemory(job->post, strlen(job->post)); free(job->post); }
        if (job->body) { SecureZeroMemory(job->body, job->body_size); free(job->body); }
        free(job->content_type); free(job->url); free(job);
    }
    for (link = &ra->jobs; *link && active < 4; link = &(*link)->next) {
        KhRaJob* job = *link;
        if (!job->thread && !SDL_GetAtomicInt(&job->done)) {
            job->thread = SDL_CreateThread(HttpWorker, "RA HTTPS", job);
            if (job->thread) ++active;
            else { job->status = RC_API_SERVER_RESPONSE_CLIENT_ERROR; SDL_SetAtomicInt(&job->done, 1); }
        }
    }
    if (ra->retry && ra->enabled && !ra->busy && SDL_GetTicks() >= ra->next_try) {
        ra->retry = 0;
        ra->busy = 1;
        if (ra->password[0])
            rc_client_begin_login_with_password(ra->client, ra->username, ra->password, LoggedIn, ra);
        else if (ra->token[0])
            rc_client_begin_login_with_token(ra->client, ra->username, ra->token, LoggedIn, ra);
        else
            ra->busy = 0;
    }
    if (evaluate && ra->enabled && !ra->invalidated) rc_client_do_frame(ra->client);
    else rc_client_idle(ra->client);
}

void KhRaDestroy(KhRa* ra) {
    if (ra) SecureZeroMemory(ra->password, sizeof(ra->password));
    if (!ra) return;
    ra->closing = 1;
    while (ra->jobs) {
        KhRaJob* job;
        for (job = ra->jobs; job; job = job->next) {
            if (job->thread) { SDL_WaitThread(job->thread, NULL); job->thread = NULL; }
            else { job->status = RC_API_SERVER_RESPONSE_CLIENT_ERROR; SDL_SetAtomicInt(&job->done, 1); }
        }
        KhRaPump(ra, 0);
    }
    rc_client_destroy(ra->client);
    free(ra->account_unlocks);
    SecureZeroMemory(ra, sizeof(*ra)); free(ra);
}

void KhRaSetEnabled(KhRa* ra, int enabled) {
    FILE* file;
    if (!ra || ra->busy) return;
    ra->enabled = enabled != 0;
    ++ra->generation; ra->account_progress_available = 0;
    file = DataFile(ra, "retroachievements.ini", RA_MODE("w"));
    if (file) { fprintf(file, "enabled=%d\n", ra->enabled); fclose(file); }
    if (!ra->enabled) { rc_client_logout(ra->client); SetStatus(ra, "Not connected", 0); }
    else if (ra->token[0]) {
        ra->busy = 1; SetStatus(ra, "Signing in...", 0);
        rc_client_begin_login_with_token(ra->client, ra->username, ra->token, LoggedIn, ra);
    }
}

void KhRaLogin(KhRa* ra, const char* username, const char* password, int remember) {
    if (!ra || !ra->enabled || ra->busy || !username[0] || !password[0]) return;
    if (strlen(username) >= sizeof(ra->username)) { SetStatus(ra, "Username too long", 1); return; }
    ra->remember = remember != 0;
    if (!ra->remember) ForgetToken(ra);
    ra->busy = 1; SetStatus(ra, "Signing in...", 0);
    rc_client_begin_login_with_password(ra->client, username, password, LoggedIn, ra);
}

void KhRaLogout(KhRa* ra) {
    if (!ra || ra->busy) return;
    ++ra->generation; ra->account_progress_available = 0;
    rc_client_logout(ra->client); ForgetToken(ra); SetStatus(ra, "Not connected", 0);
}

void KhRaSetRom(KhRa* ra, const uint8_t* rom, size_t size) {
    rc_hash_iterator_t iterator;
    if (!ra) return;
    ++ra->generation; ra->account_progress_available = 0;
    rc_client_unload_game(ra->client);
    ra->hash[0] = 0;
    if (!rom || !size) { SetStatus(ra, "Install the selected ROM to identify its catalog.", 0); return; }
    rc_hash_initialize_iterator(&iterator, NULL, rom, size);
    if (!rc_hash_generate(ra->hash, RC_CONSOLE_GAMEBOY_ADVANCE, &iterator))
        SetStatus(ra, "Could not identify the ROM", 1);
    rc_hash_destroy_iterator(&iterator);
    LoadGame(ra);
}

void KhRaInvalidate(KhRa* ra, const char* reason) {
    if (!ra || ra->invalidated) return;
    ra->invalidated = 1;
    rc_client_unload_game(ra->client);
    SetStatus(ra, reason, 1);
}

rc_client_t* KhRaClient(KhRa* ra) { return ra ? ra->client : NULL; }
const char* KhRaStatus(const KhRa* ra) { return ra ? ra->status : "Client unavailable"; }
const char* KhRaHash(const KhRa* ra) { return ra ? ra->hash : ""; }
int KhRaEnabled(const KhRa* ra) { return ra && ra->enabled; }
int KhRaBusy(const KhRa* ra) { return ra && ra->busy; }
int KhRaError(const KhRa* ra) { return !ra || ra->error; }
int KhRaAccountProgressAvailable(const KhRa* ra) { return ra && ra->account_progress_available; }
int KhRaAccountUnlocked(const KhRa* ra, uint32_t id) {
    uint32_t i;
    if (!ra || !ra->account_progress_available) return 0;
    for (i = 0; i < ra->account_unlock_count; ++i) if (ra->account_unlocks[i] == id) return 1;
    return 0;
}
int KhRaValidationMode(void) { return 1; }

void KhRaStartOfficial(KhRa* ra, const uint32_t* ids, uint32_t count) {
    if (!ra) return;
    ra->official = ids; ra->official_count = count;
    rc_client_set_spectator_mode_enabled(ra->client, 0); /* softcore unlocks are submitted */
    if (ra->enabled && ra->token[0] && !rc_client_get_user_info(ra->client) && !ra->busy) {
        ra->busy = 1; SetStatus(ra, "Signing in...", 0);
        rc_client_begin_login_with_token(ra->client, ra->username, ra->token, LoggedIn, ra);
    }
}
uint32_t KhRaNoticeId(const KhRa* ra) { return ra ? ra->notice_id : 0; }
const char* KhRaTakeNotice(KhRa* ra) {
    static char notice[512];
    if (!ra || !ra->notice[0]) return NULL;
    memcpy(notice, ra->notice, sizeof(notice)); ra->notice[0] = 0;
    return notice;
}
