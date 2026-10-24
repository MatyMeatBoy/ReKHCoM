/* Story events → Re:CoM HD scenes (port/progress/HD_MOD.md): the "reasonably located" rows of issue #25's map by
   DL-6-case (strong or partial dialogue match), checked against include/evt/event_ids.h names, then checked and completed by dialogue (match_hd_scenes.py:
   69 of the 72 confirmed, one corrected, eight added). */
#include "evt/event_ids.h"
static const struct { unsigned short eventId; const char* movie; } kHdScenes[] = {
    { EVENT_000_1F_ENTRANCE_PART1, "zz_event_011" },
    { EVENT_010_1F_GOAL_1, "zz_event_012" },
    { EVENT_011_1F_GOAL_2, "zz_event_013" },
    { EVENT_012_2F_ENTRANCE, "zz_event_021" },
    { EVENT_013_2F_GOAL, "zz_event_022" },
    { EVENT_015_3F_ENTRANCE, "zz_event_031" },
    { EVENT_016_3F_GOAL, "zz_event_032" },
    { EVENT_017_3F_DEMO, "zz_event_033" },
    { EVENT_018_4F_ENTRANCE, "zz_event_041" },
    { EVENT_019_4F_GOAL, "zz_event_042" },
    { EVENT_021_5F_ENTRANCE, "zz_event_051" },
    { EVENT_022_5F_GOAL, "zz_event_052" },
    { EVENT_023_5F_DEMO, "zz_event_053" },
    { EVENT_024_6F_ENTRANCE, "zz_event_061" },
    { EVENT_026_6F_GOAL_2, "zz_event_062" },
    { EVENT_027_6F_GOAL_3, "zz_event_063" },
    { EVENT_028_6F_DEMO, "zz_event_064" },
    { EVENT_029_7F_ENTRANCE, "zz_event_071" },
    { EVENT_030_7F_GOAL_1, "zz_event_072" },
    { EVENT_032_8F_ENTRANCE, "zz_event_081" },
    { EVENT_033_8F_GOAL_1, "zz_event_082" },
    { EVENT_034_8F_GOAL_2, "zz_event_083" },
    { EVENT_036_9F_ENTRANCE, "zz_event_091" },
    { EVENT_038_9F_DEMO, "zz_event_093" },
    { EVENT_039_10F_ENTRANCE, "zz_event_101" },
    { EVENT_040_10F_GOAL_1, "zz_event_102" },
    { EVENT_041_10F_GOAL_2, "zz_event_103" },
    { EVENT_045_11F_TWILIGHT_TOWN_E1, "zz_event_112" },
    { EVENT_047_11F_DEMO_1, "zz_event_114" },
    { EVENT_048_11F_GOAL_1, "zz_event_115" },
    { EVENT_050_11F_GOAL_3, "zz_event_117" },
    { EVENT_051_11F_DEMO_2, "zz_event_118" },
    { EVENT_056_12F_DESTINY_ISLAND_E2_2, "zz_event_123" },
    { EVENT_058_12F_GOAL, "zz_event_124" },
    { EVENT_059_12F_GOAL_2, "zz_event_125" },
    { EVENT_060_12F_GOAL_3, "zz_event_126" },
    { EVENT_063_13F_CASTLE_OBLIVION_E1, "zz_event_133" },
    { EVENT_064_13F_CASTLE_OBLIVION_E1_2, "zz_event_134" },
    { EVENT_065_13F_CASTLE_OBLIVION_E1_3, "zz_event_135" },
    { EVENT_066_13F_CASTLE_OBLIVION_LAST1, "zz_event_136" },
    { EVENT_072_13F_CASLTE_OBLIVION_LAST7, "zz_event_140" },
    { EVENT_073_13F_CAPSULE_ROOM_ENDING, "zz_event_151" },
    { EVENT_149_RIKU_B12F_OPNING, "zz_event_501" },
    { EVENT_155_RIKU_B12F_GOAL, "zz_event_502" },
    { EVENT_156_RIKU_B12F_GOAL_2, "zz_event_503" },
    { EVENT_157_RIKU_B12F_DEMO, "zz_event_504" },
    { EVENT_158_RIKU_B11F_ENTRANCE, "zz_event_511" },
    { EVENT_159_RIKU_B11F_DEMO, "zz_event_512" },
    { EVENT_160_RIKU_B10F_GOAL, "zz_event_521" },
    { EVENT_162_RIKU_B9F_DEMO, "zz_event_531" },
    { EVENT_163_RIKU_B8F_GOAL, "zz_event_541" },
    { EVENT_164_RIKU_B8F_GOAL_2, "zz_event_542" },
    { EVENT_166_RIKU_B7F_ENTRANCE, "zz_event_551" },
    { EVENT_167_RIKU_B7F_DEMO, "zz_event_552" },
    { EVENT_168_RIKU_B6F_DEMO, "zz_event_561" },
    { EVENT_169_RIKU_B5F_DEMO, "zz_event_571" },
    { EVENT_171_RIKU_B4F_GOAL, "zz_event_582" },
    { EVENT_173_RIKU_B4F_DARK, "zz_event_584" },
    { EVENT_174_RIKU_B4F_GOAL_3, "zz_event_585" },
    { EVENT_175_RIKU_B4F_DEMO, "zz_event_586" },
    { EVENT_176_RIKU_B3F_ENTRANCE, "zz_event_591" },
    { EVENT_179_RIKU_B3F_E2, "zz_event_592" },
    { EVENT_182_RIKU_B3F_LIGHT, "zz_event_594" }, /* by dialogue (match_hd_scenes.py), was EVENT_181 */
    { EVENT_184_RIKU_B3F_DEMO, "zz_event_596" },
    { EVENT_185_RIKU_B2F_ENTRANCE, "zz_event_601" },
    { EVENT_186_RIKU_B2F_E0, "zz_event_602" },
    { EVENT_187_RIKU_B2F_E1, "zz_event_603" },
    { EVENT_189_RIKU_B2F_E2, "zz_event_605" },
    { EVENT_190_RIKU_B2F_GOAL, "zz_event_606" },
    { EVENT_191_RIKU_B1F_ENTRANCE, "zz_event_611" },
    { EVENT_193_RIKU_B1F_LAST1, "zz_event_613" },
    { EVENT_195_RIKU_ENDING, "zz_event_621" },
    /* matched by dialogue, 22/10 (port/tools/match_hd_scenes.py: the subtitles against the event text) */
    { EVENT_031_7F_GOAL_2, "zz_event_073" },
    { EVENT_061_13F_ENTRANCE, "zz_event_131" },
    { EVENT_067_13F_CASTLE_OBLIVION_LAST2, "zz_event_137" },
    { EVENT_165_RIKU_B8F_DEMO, "zz_event_543" },
    { EVENT_170_RIKU_B4F_ENTRANCE, "zz_event_581" },
    { EVENT_183_RIKU_B3F_E2_3, "zz_event_595" },
    { EVENT_188_RIKU_B2F_E1_2, "zz_event_604" },
    { EVENT_194_RIKU_B1F_LAST2, "zz_event_615" },
    /* 22/10, by dialogue in the five cartridge languages, the scenes' order and wordless pairs (HD_MOD.md); left out:
       zz_event_001 (the prologue, inside the opening the ROP movie replaces), 614 and 631 (Re:CoM only) */
    { EVENT_014_2F_DEMO, "zz_event_023" }, /* wordless in both, between 013 and 015 */
    { EVENT_020_4F_DEMO, "zz_event_043" }, /* wordless in both, between 019 and 021 */
    { EVENT_035_8F_DEMO, "zz_event_084" }, /* Larxene taunts Naminé */
    { EVENT_037_9F_GOAL, "zz_event_092" }, /* dialogue 0.60 */
    { EVENT_042_10F_DEMO, "zz_event_104" }, /* Vexen's rash act, Axel */
    { EVENT_043_11F_ENTRANCE, "zz_event_111" }, /* the card made of memories from the other side of the heart */
    { EVENT_046_11F_TWILIGHT_TOWN_E1_2, "zz_event_113" }, /* dialogue, between 045 and 047 */
    { EVENT_049_11F_GOAL_2, "zz_event_116" }, /* dialogue 0.59, between 048 and 050 */
    { EVENT_052_12F_ENTRANCE, "zz_event_121" }, /* "Sora, you shouldn't talk to Donald and Goofy like-" / "Keep it to yourself!" */
    { EVENT_055_12F_DESTINY_ISLAND_E2, "zz_event_122" }, /* the one-line island collapse */
    { EVENT_062_12F_GOAL, "zz_event_132" }, /* wordless in both, between 061 and 063 */
    { EVENT_070_13F_CASTLE_OBLIVION_LAST5, "zz_event_138" }, /* Marluxia: the world of nothingness */
    { EVENT_071_13F_CASTLE_OBLIVION_LAST6, "zz_event_139" }, /* wordless in both, after 070 */
    { EVENT_161_RIKU_B10F_GOAL_2, "zz_event_522" }, /* the darkness coursing through Riku, the provocation */
    { EVENT_172_RIKU_B4F_GOAL_2, "zz_event_583" }, /* Lexaeus beaten, between 171 and 173 */
    { EVENT_181_RIKU_B3F_E2_2, "zz_event_593" }, /* "Sora?!", before 594/182 and 595/183 */
    { EVENT_192_RIKU_B1F_E0, "zz_event_612" }, /* Ansem: at the heart of darkness */
};
