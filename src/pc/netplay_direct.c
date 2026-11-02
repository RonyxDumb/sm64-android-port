/* SDL owns the game thread's JNI environment. All Android work is posted to UI. */
#include "netplay_direct.h"
#if defined(__ANDROID__) && !defined(NETPLAY_SOCKET_TEST)
#include <SDL.h>
#include <SDL_system.h>
#include <jni.h>
#include <string.h>

static int call_int(const char *method, const char *signature, int arg) {
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass cls;
    jmethodID id;
    int value = -1;
    if (!env || !activity) return -1;
    cls = (*env)->GetObjectClass(env, activity);
    id = (*env)->GetMethodID(env, cls, method, signature);
    if (id) value = (*env)->CallIntMethod(env, activity, id, arg);
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); value = -1; }
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
    return value;
}
static const char *call_string(const char *method, char *buffer, size_t size) {
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass cls;
    jmethodID id;
    jstring result = NULL;
    buffer[0] = 0;
    if (!env || !activity) return buffer;
    cls = (*env)->GetObjectClass(env, activity);
    id = (*env)->GetMethodID(env, cls, method, "()Ljava/lang/String;");
    if (id) result = (jstring)(*env)->CallObjectMethod(env, activity, id);
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); result = NULL; }
    if (result) {
        const char *utf = (*env)->GetStringUTFChars(env, result, NULL);
        if (utf) { strncpy(buffer, utf, size - 1); buffer[size - 1] = 0;
            (*env)->ReleaseStringUTFChars(env, result, utf); }
        (*env)->DeleteLocalRef(env, result);
    }
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
    return buffer;
}
int net_direct_start(int host) { return call_int("startLocalDirect", "(I)I", host); }
void net_direct_stop(void) { (void)call_int("stopLocalDirect", "()I", 0); }
int net_direct_status(void) { return call_int("localDirectStatus", "()I", 0); }
const char *net_direct_host_address(void) {
    static char ip[64]; return call_string("localDirectAddress", ip, sizeof(ip));
}
const char *net_direct_games(void) {
    static char games[256]; return call_string("localDirectGames", games, sizeof(games));
}
const char *net_direct_detail(void) {
    static char detail[64]; return call_string("localDirectDetail", detail, sizeof(detail));
}
#else
int net_direct_start(int host) { (void)host; return -1; }
void net_direct_stop(void) {}
int net_direct_status(void) { return -1; }
const char *net_direct_host_address(void) { return ""; }
const char *net_direct_games(void) { return ""; }
const char *net_direct_detail(void) { return "WIFI DIRECT UNAVAILABLE"; }
#endif
