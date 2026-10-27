/* QA build wrapper only. Production source stays byte-for-byte unchanged. */
#include "../src/port_ui.c"
void GalleryQaSelection(int values[8]) {
    values[0] = sSceneChoice;
    values[1] = sSceneScroll;
    values[2] = sSceneHd;
    values[3] = sSceneCharacter;
    values[4] = SceneAt(sSceneChoice, NULL);
    values[5] = sSceneHarnessTick;
    values[6] = sScenePause;
    values[7] = sSceneStep;
}
