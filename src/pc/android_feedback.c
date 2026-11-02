#include "android_feedback.h"
#include "netplay_menu.h"
#ifdef __ANDROID__
#include <SDL.h>
#include <SDL_system.h>
#include <jni.h>

void android_rumble_event(int frames, int strength, int continuous) {
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    if (!env || !activity) return;
    jclass cls = (*env)->GetObjectClass(env, activity);
    jmethodID id = (*env)->GetMethodID(env, cls, "rumbleEvent", "(III)V");
    if (id) (*env)->CallVoidMethod(env, activity, id, frames, strength, continuous);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
}

int android_menu_poll(int page, int selected, int status, int ready, const char *detail) {
    lan_menu_set_state(page, selected, status, ready, detail);
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    int result = 0;
    if (!env || !activity) return 0;
    jclass cls = (*env)->GetObjectClass(env, activity);
    jmethodID id = (*env)->GetMethodID(env, cls, "updateLocalMenu", "(IIIILjava/lang/String;)I");
    jstring text = (*env)->NewStringUTF(env, detail ? detail : "");
    if (id && text) result = (*env)->CallIntMethod(env, activity, id, page, selected, status, ready, text);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    if (text) (*env)->DeleteLocalRef(env, text);
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
    return result;
}
static int setting_call(const char *method, int value, int setter) {
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    int result = 0;
    if (!env || !activity) return 0;
    jclass cls = (*env)->GetObjectClass(env, activity);
    jmethodID id = (*env)->GetMethodID(env, cls, method, setter ? "(I)V" : "()I");
    if (id) {
        if (setter) (*env)->CallVoidMethod(env, activity, id, value);
        else result = (*env)->CallIntMethod(env, activity, id);
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
    return result;
}
int android_rumble_enabled(void) { return setting_call("rumbleEnabled", 0, 0); }
void android_rumble_set_enabled(int value) { setting_call("setRumbleEnabled", value, 1); }
void android_direct_choose(int index) { setting_call("selectLocalDirectGame", index, 1); }

#else
int android_rumble_enabled(void) { return 1; }
void android_rumble_set_enabled(int value) { (void)value; }
void android_direct_choose(int index) { (void)index; }
void android_rumble_event(int frames, int strength, int continuous) {
    (void)frames; (void)strength; (void)continuous;
}
int android_menu_poll(int page, int selected, int status, int ready, const char *detail) {
    (void)page; (void)selected; (void)status; (void)ready; (void)detail; return 0;
}
#endif
