/**
 * video_player.h — Video player for FMV cutscenes.
 *
 * On Vita, video playback used SceAvPlayer with OpenGL texture rendering.
 * On Android, we use MediaPlayer (hardware-accelerated) via the
 * Bio4VideoActivity Java class.
 *
 * This native module provides a C API that the game can call to
 * start/stop video playback, bridging to the Java Activity.
 */

#ifndef ARMBIO4_VIDEO_PLAYER_H
#define ARMBIO4_VIDEO_PLAYER_H

/**
 * Start playing a video file.
 *
 * @param filepath  Path to the video file (typically in the assets directory)
 * @param volume   Volume level (0.0 - 1.0)
 * @return 0 on success, -1 on error
 */
int video_start(const char *filepath, float volume);

/**
 * Check if a video is currently playing.
 *
 * @return 1 if playing, 0 if not
 */
int video_is_playing(void);

/**
 * Draw the current video frame (if playing).
 * On Android, this is a no-op since MediaPlayer handles rendering.
 *
 * @return 1 if a frame was drawn, 0 if not
 */
int video_draw(void);

#endif /* ARMBIO4_VIDEO_PLAYER_H */
