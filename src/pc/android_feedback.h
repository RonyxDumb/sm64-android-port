#ifndef ANDROID_FEEDBACK_H
#define ANDROID_FEEDBACK_H
void android_rumble_event(int frames, int strength, int continuous);
int android_menu_poll(int page, int selected, int status, int ready, const char *detail);
#endif
