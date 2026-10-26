#ifndef PORT_SCENE_GALLERY_H
#define PORT_SCENE_GALLERY_H
#define PORT_SCENE_MOVIE_BASE 512
#define PORT_SCENE_MOVIE(id) (PORT_SCENE_MOVIE_BASE + (id))
#define PORT_SCENE_IS_MOVIE(id) ((id) > PORT_SCENE_MOVIE_BASE && (id) <= PORT_SCENE_MOVIE_BASE + 5)
const char* PortHdGalleryMovieName(int sceneId);
int PortHdGalleryMovieOffset(int sceneId);
int PortHdGalleryPlayScene(int sceneId, int hybrid);
void PortSceneGalleryAuditBegin(void);
void PortSceneGalleryAuditEnd(void);
void PortSceneSeen(int sceneId);
int PortSceneIsSeen(int sceneId);
int PortSceneGalleryActive(void);
unsigned PortSceneGalleryAge(void);
int PortScenePosterRendering(void);
int PortScenePosterFinished(void);
int PortScenePosterConfigure(int argc, char** argv);
#endif
