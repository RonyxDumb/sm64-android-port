#include "android_feedback.h"
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
#else
void android_rumble_event(int frames, int strength, int continuous) {
    (void)frames; (void)strength; (void)continuous;
}
int android_menu_poll(int page, int selected, int status, int ready, const char *detail) {
    (void)page; (void)selected; (void)status; (void)ready; (void)detail; return 0;
}
#endif
