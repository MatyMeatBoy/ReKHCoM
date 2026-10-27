/* The Android app's launcher (LAUNCHER_ANDROID): what Windows gives the PC launcher comes from the app's activity
 * (port/android/app/src/main/java/com/rekhcom/android/LauncherActivity.java) through JNI:
 *   - the system's file pickers (Storage Access Framework): the activity copies what is picked into the app's files
 *     folder (a ROM to roms/<code>.gba after its SHA-1 check, a save into its bank, the PC's hd folder to hd/), or
 *     writes a save out where the player says;
 *   - starting the game: GameActivity, in its own process (:game);
 *   - its status line (a copy running, the last result), read every frame: when it changes, the launcher looks at
 *     the files folder again.
 * The fonts, box art and logo ship in the APK's assets (launcher/) and are copied to files/assets on start. */
#include <jni.h>

extern "C" JNIEXPORT jboolean JNICALL Java_com_rekhcom_android_LauncherActivity_commitHdMedia(
        JNIEnv* env, jclass, jstring root, jint mode, jstring pending) {
    const char* data = env->GetStringUTFChars(root, nullptr);
    const char* file = env->GetStringUTFChars(pending, nullptr);
    char active[1600]; bool ok = false;
    if (data && file && PortHdMediaLabel(mode)) {
        fs::path expected = fs::u8path(data) / "hd/media" / PortHdMediaLabel(mode);
        if (fs::u8path(file).parent_path() == expected && PortHdMediaPendingValid(data, mode, file)) {
            SDL_snprintf(active, sizeof(active), "%s/hd/media/%s/active.txt", data, PortHdMediaLabel(mode));
            ok = PortHdMediaReplaceFile(file, active) != 0;
        }
    }
    if (file) env->ReleaseStringUTFChars(pending, file);
    if (data) env->ReleaseStringUTFChars(root, data);
    return ok ? JNI_TRUE : JNI_FALSE;
}

enum { ANDROID_PICK_ROM = 10, ANDROID_PICK_SAVE = 20, ANDROID_PICK_EXPORT = 30, ANDROID_PICK_HD = 40 };

/* calls a method of the activity; false when the activity is not there */
template <class F> static bool AndroidActivityCall(F call) {
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = env ? (jobject)SDL_GetAndroidActivity() : nullptr;
    if (!activity)
        return false;
    jclass cls = env->GetObjectClass(activity);
    call(env, activity, cls);
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
    return true;
}

/* what: ANDROID_PICK_*; region/bank: the save's (0 EU, 1 US, 2 JP; banks 1-5) */
static void AndroidPick(int what, int region, int bank) {
    AndroidActivityCall([&](JNIEnv* env, jobject activity, jclass cls) {
        jmethodID m = env->GetMethodID(cls, "nativePick", "(III)V");
        if (m)
            env->CallVoidMethod(activity, m, (jint)what, (jint)region, (jint)bank);
    });
}

/* the game for that version (eu/us/jp) with that dump; netlink.cfg and kh_com.ini are already written */
static bool AndroidLaunch(const char* region, const std::string& rom) {
    bool ok = false;
    AndroidActivityCall([&](JNIEnv* env, jobject activity, jclass cls) {
        jmethodID m = env->GetMethodID(cls, "nativeLaunch", "(Ljava/lang/String;Ljava/lang/String;)V");
        if (!m)
            return;
        jstring r = env->NewStringUTF(region), path = env->NewStringUTF(rom.c_str());
        env->CallVoidMethod(activity, m, r, path);
        env->DeleteLocalRef(r);
        env->DeleteLocalRef(path);
        ok = !env->ExceptionCheck();
    });
    return ok;
}

static void AndroidOpenUrl(const std::string& url) {
    SDL_OpenURL(url.c_str());
}

/* The activity's status: busy (a copy running), a serial that grows with every result, the line to show. */
struct AndroidStatus {
    bool busy = false;
    int serial = 0;
    bool error = false;
    std::string text;
};
static AndroidStatus AndroidStatusNow() {
    AndroidStatus st;
    AndroidActivityCall([&](JNIEnv* env, jobject activity, jclass cls) {
        jmethodID m = env->GetMethodID(cls, "nativeStatus", "()Ljava/lang/String;");
        jstring s = m ? (jstring)env->CallObjectMethod(activity, m) : nullptr;
        if (!s)
            return;
        const char* c = env->GetStringUTFChars(s, nullptr);
        /* "<busy 0|1>|<serial>|<error 0|1>|<text>" */
        int busy = 0, serial = 0, error = 0, used = 0;
        if (c && std::sscanf(c, "%d|%d|%d|%n", &busy, &serial, &error, &used) >= 3) {
            st.busy = busy != 0;
            st.serial = serial;
            st.error = error != 0;
            st.text = c + used;
        }
        if (c)
            env->ReleaseStringUTFChars(s, c);
        env->DeleteLocalRef(s);
    });
    return st;
}

/* the app's files folder: the same one the game reads (GameActivity's --data-dir) */
static fs::path AndroidFilesDir() {
    const char* p = SDL_GetAndroidInternalStoragePath();
    return p ? fs::u8path(p) : fs::path(".");
}

/* the launcher's pictures and fonts from the APK (assets/launcher/...) into files/assets/, when missing or changed */
static void AndroidExtractAssets(const fs::path& root) {
    static const char* const files[] = { "fonts/KHMenu.otf", "boxart.jpg", "logo.png", "achievements/badges.png" };
    for (const char* name : files) {
        std::string src = std::string("launcher/") + name;
        size_t size = 0;
        void* data = SDL_LoadFile(src.c_str(), &size);
        if (!data)
            continue;
        fs::path dst = root / "assets" / fs::u8path(name);
        std::error_code ec;
        if (!fs::exists(dst, ec) || fs::file_size(dst, ec) != size) {
            fs::create_directories(dst.parent_path(), ec);
            std::ofstream(dst, std::ios::binary).write((const char*)data, (std::streamsize)size);
        }
        SDL_free(data);
    }
}
