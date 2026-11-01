/* Luigi mesh from RonyxDumb/showfloor-nds, actors/player (see SOURCE.md). */
ALIGNED8 static const u8 l_mark_txt[] = {
#include "actors/player/l_mark_txt.rgba16.inc.c"
};
ALIGNED8 static const u8 l_momi_txt[] = {
#include "actors/player/l_momi_txt.rgba16.inc.c"
};
ALIGNED8 static const u8 l_hige_txt[] = {
#include "actors/player/l_hige_txt.rgba16.inc.c"
};
ALIGNED8 static const u8 meye1_txt[] = {
#include "actors/player/meye1_txt.rgba16.inc.c"
};
ALIGNED8 static const u8 meye2_txt[] = {
#include "actors/player/meye2_txt.rgba16.inc.c"
};
ALIGNED8 static const u8 meye3_txt[] = {
#include "actors/player/meye3_txt.rgba16.inc.c"
};
ALIGNED8 static const u8 meye4_txt[] = {
#include "actors/player/meye4_txt.rgba16.inc.c"
};
ALIGNED8 static const u8 meye5_txt[] = {
#include "actors/player/meye5_txt.rgba16.inc.c"
};
ALIGNED8 static const u8 m_button_txt[] = {
#include "actors/player/m_button_txt.rgba16.inc.c"
};
#define ShapeColor_luigi(R,G,B) gdSPDefLights1((R)/4,(G)/4,(B)/4,R,G,B,40,40,40)
#include "luigi/luigi_near_poly.sou"
#include "luigi/luigi_near_body.sou"
#include "luigi/luigi_near_head.sou"
#undef ShapeColor_luigi
