#ifndef CONTROLLER_SDL_H
#define CONTROLLER_SDL_H

#include <stdbool.h>
#include "controller_api.h"

extern struct ControllerAPI controller_sdl;

bool controller_sdl_is_connected(void);

#endif