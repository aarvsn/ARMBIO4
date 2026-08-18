/**
 * bio4_bridge.h — Native bridge header for ARMBIO4.
 *
 * Declares the interface between the Java Bio4NativeBridge class
 * and the native game code loaded from libbio4af.so and
 * libmc_eruption_for_android_jni.so.
 */

#ifndef ARMBIO4_BRIDGE_H
#define ARMBIO4_BRIDGE_H

#include <jni.h>
#include <stdbool.h>
#include <stdint.h>

/* Screen dimensions — matches original game's Android resolution target */
#define SCREEN_W 960
#define SCREEN_H 544

/* Game data paths */
#define SO_LIB_DIR "data/Bio4/"
#define SO_ASSETS_DIR "data/Bio4/assets/"
#define SO_CACHE_DIR "data/Bio4/cache/"

/* Loaded module handles */
typedef struct {
  void *handle; /* dlopen handle */
  char *name;   /* library name */
} bio4_module_t;

/* Function pointer types for the game's native methods */
typedef jint (*fn_onCreate)(JNIEnv *, jobject, jobject);
typedef void (*fn_onRestart)(JNIEnv *, jobject);
typedef void (*fn_onStart)(JNIEnv *, jobject);
typedef void (*fn_onResume)(JNIEnv *, jobject);
typedef jboolean (*fn_onSurfaceCreated)(JNIEnv *, jobject, jobject, jobject,
                                        jint, jint);
typedef jboolean (*fn_onSurfaceChanged)(JNIEnv *, jobject, jint, jint);
typedef jint (*fn_onDrawFrame)(JNIEnv *, jobject, jobject);
typedef void (*fn_onPause)(JNIEnv *, jobject);
typedef void (*fn_onStop)(JNIEnv *, jobject);
typedef void (*fn_onDestroy)(JNIEnv *, jobject);
typedef jboolean (*fn_onKeyDown)(JNIEnv *, jobject, jint, jobject);
typedef jboolean (*fn_onKeyUp)(JNIEnv *, jobject, jint, jobject);
typedef void (*fn_onTouchBegan)(JNIEnv *, jobject, jobject, jfloat, jfloat,
                                jfloat, jfloat);
typedef void (*fn_onTouchMoved)(JNIEnv *, jobject, jobject, jfloat, jfloat,
                                jfloat, jfloat);
typedef void (*fn_onTouchEnded)(JNIEnv *, jobject, jobject, jfloat, jfloat,
                                jfloat, jfloat);
typedef void (*fn_onTouchCancelled)(JNIEnv *, jobject, jobject, jfloat, jfloat,
                                    jfloat, jfloat);
typedef void (*fn_setStereoHard)(JNIEnv *, jobject, jobject, jboolean);
typedef void (*fn_onShake)(JNIEnv *, jobject);
typedef jint (*fn_jni_onload)(JavaVM *, void *);

/* Global bridge state */
typedef struct {
  /* Loaded modules */
  bio4_module_t mce_mod;
  bio4_module_t bio4_mod;

  /* JNI references */
  JavaVM *java_vm;
  JNIEnv *jni_env;
  jclass activity_class;
  jobject activity_object;

  /* Game native method pointers (from libbio4af.so) */
  fn_onCreate on_create;
  fn_onRestart on_restart;
  fn_onStart on_start;
  fn_onResume on_resume;
  fn_onSurfaceCreated on_surface_created;
  fn_onSurfaceChanged on_surface_changed;
  fn_onDrawFrame on_draw_frame;
  fn_onPause on_pause;
  fn_onStop on_stop;
  fn_onDestroy on_destroy;
  fn_onKeyDown on_key_down;
  fn_onKeyUp on_key_up;
  fn_onTouchBegan on_touch_began;
  fn_onTouchMoved on_touch_moved;
  fn_onTouchEnded on_touch_ended;
  fn_onTouchCancelled on_touch_cancelled;
  fn_setStereoHard set_stereo_hard;
  fn_onShake on_shake;

  /* Game state */
  bool is_app_alive;
  bool is_stereo_flg;
  long n_prev_time;

  /* Data directory path */
  char data_dir[1024];
} bio4_bridge_t;

/* Global bridge instance */
extern bio4_bridge_t g_bridge;

/* Bridge lifecycle */
int bio4_bridge_init(const char *data_dir);
void bio4_bridge_run();
void bio4_bridge_term();

/* Module loading */
bio4_module_t bio4_load_library(const char *data_dir, const char *soname);
int bio4_call_jni_onload(bio4_module_t *mod);

/* Patches */
int bio4_patch_mce(bio4_module_t *mod);
int bio4_patch_bio4(bio4_module_t *mod);

/* Logging */
#define LOG_TAG "Bio4Native"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#endif /* ARMBIO4_BRIDGE_H */
