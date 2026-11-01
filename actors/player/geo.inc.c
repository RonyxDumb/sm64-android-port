extern const Gfx RCP_luigi_head1[];
extern const Gfx RCP_luigi_head2[];
extern const Gfx RCP_luigi_head3[];
extern const Gfx RCP_luigi_head4[];
extern const Gfx RCP_luigi_head5[];
extern const Gfx RCP_luigi_near0[];
extern const Gfx RCP_luigi_near1[];
extern const Gfx RCP_luigi_near10[];
extern const Gfx RCP_luigi_near11[];
extern const Gfx RCP_luigi_near14[];
extern const Gfx RCP_luigi_near2[];
extern const Gfx RCP_luigi_near3[];
extern const Gfx RCP_luigi_near4[];
extern const Gfx RCP_luigi_near5[];
extern const Gfx RCP_luigi_near6[];
extern const Gfx RCP_luigi_near7[];
extern const Gfx RCP_luigi_near8[];
extern const Gfx RCP_luigi_near9[];
extern const Gfx RCP_luigi_near_body[];
static const GeoLayout luigi_body_geo[] = {
		GEO_ANIMATED_PART(LAYER_OPAQUE, 0, 261-189, 0, NULL),									/* chn14_2_1	*/
		GEO_OPEN_NODE(),
			GEO_ANIMATED_PART(LAYER_OPAQUE, 0, 0, 0, RCP_luigi_near14),							/* m_waist1_3_2	*/
			GEO_OPEN_NODE(),
				GEO_ANIMATED_PART(LAYER_OPAQUE, 33, 0, 0, RCP_luigi_near_body),					/* m_body1_2	*/
				GEO_OPEN_NODE(),

					GEO_ANIMATED_PART(LAYER_OPAQUE, 91, 0, 0, NULL),							/* m_head2		*/
					GEO_OPEN_NODE(),
						GEO_SWITCH_CASE(0, geo_switch_anim_state),					/* for the eye animation	*/
						GEO_OPEN_NODE(),
							GEO_DISPLAY_LIST(LAYER_OPAQUE, RCP_luigi_head1),
							GEO_DISPLAY_LIST(LAYER_OPAQUE, RCP_luigi_head2),
							GEO_DISPLAY_LIST(LAYER_OPAQUE, RCP_luigi_head3),
							GEO_DISPLAY_LIST(LAYER_OPAQUE, RCP_luigi_head4),
							GEO_DISPLAY_LIST(LAYER_OPAQUE, RCP_luigi_head5),
						GEO_CLOSE_NODE(),
					GEO_CLOSE_NODE(),

					GEO_ANIMATED_PART(LAYER_OPAQUE, 63, -10, 65, NULL),							/* chn6			*/
					GEO_OPEN_NODE(),
						GEO_ANIMATED_PART(LAYER_OPAQUE, 0, 0, 0, RCP_luigi_near2),				/* m_larmA1_2	*/
						GEO_OPEN_NODE(),
							GEO_ANIMATED_PART(LAYER_OPAQUE, 63, 0, 0, RCP_luigi_near1),			/* m_larmB1_2	*/
							GEO_OPEN_NODE(),
								GEO_ANIMATED_PART(LAYER_OPAQUE, 71, 0, 0, RCP_luigi_near0),		/* m_lhamd1_2	*/
							GEO_CLOSE_NODE(),
						GEO_CLOSE_NODE(),
					GEO_CLOSE_NODE(),

					GEO_ANIMATED_PART(LAYER_OPAQUE, 60, -10, -63, NULL),						/* chn10		*/
					GEO_OPEN_NODE(),
						GEO_ANIMATED_PART(LAYER_OPAQUE, 0, 0, 0, RCP_luigi_near5),				/* m_rarmA1_2	*/
						GEO_OPEN_NODE(),
							GEO_ANIMATED_PART(LAYER_OPAQUE, 62, 0, 0, RCP_luigi_near4),			/* m_rarmB1_2	*/
							GEO_OPEN_NODE(),
								GEO_ANIMATED_PART(LAYER_OPAQUE, 66, 0, 0, RCP_luigi_near3),		/* m_rhand1_2	*/
							GEO_CLOSE_NODE(),
						GEO_CLOSE_NODE(),
					GEO_CLOSE_NODE(),

				GEO_CLOSE_NODE(),

				GEO_ANIMATED_PART(LAYER_OPAQUE, -34, -9, 39, NULL),								/* chn15		*/
				GEO_OPEN_NODE(),
					GEO_ANIMATED_PART(LAYER_OPAQUE, 0, 0, 0, RCP_luigi_near11),					/* m_llegA1_2	*/
					GEO_OPEN_NODE(),
						GEO_ANIMATED_PART(LAYER_OPAQUE, 98, 0, 0, RCP_luigi_near10),			/* m_llegB1_2	*/
						GEO_OPEN_NODE(),
							GEO_ANIMATED_PART(LAYER_OPAQUE, 81, 0, 0, RCP_luigi_near9),			/* m_lfoot1_2	*/
						GEO_CLOSE_NODE(),
					GEO_CLOSE_NODE(),
				GEO_CLOSE_NODE(),

				GEO_ANIMATED_PART(LAYER_OPAQUE, -34, -9, -40, NULL),							/* chn17		*/
				GEO_OPEN_NODE(),
					GEO_ANIMATED_PART(LAYER_OPAQUE, 0, 0, 0, RCP_luigi_near8),					/* m_rlegA1_2	*/
					GEO_OPEN_NODE(),
						GEO_ANIMATED_PART(LAYER_OPAQUE, 98, 0, 0, RCP_luigi_near7),				/* m_rlegB1_2	*/
						GEO_OPEN_NODE(),
							GEO_ANIMATED_PART(LAYER_OPAQUE, 81, 0, 0, RCP_luigi_near6),			/* m_rfoot1_2	*/
						GEO_CLOSE_NODE(),
					GEO_CLOSE_NODE(),
				GEO_CLOSE_NODE(),

			GEO_CLOSE_NODE(),
		GEO_CLOSE_NODE(),
	GEO_CLOSE_NODE(),
	GEO_RETURN(),
};
const GeoLayout luigi_geo[] = {
    GEO_SHADOW(SHADOW_CIRCLE_PLAYER, 180, 100),
    GEO_OPEN_NODE(),
        GEO_SCALE(0, 16384),
        GEO_OPEN_NODE(),
            GEO_BRANCH(1, luigi_body_geo),
        GEO_CLOSE_NODE(),
    GEO_CLOSE_NODE(),
    GEO_END(),
};
