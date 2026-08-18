/**
 * bio4_bridge.c — Native bridge implementation for ARMBIO4.
 *
 * This is the core of the Android-native bridge. It:
 *   1. Uses dlopen() to load the original game's .so libraries from the data
 * directory
 *   2. Uses dlsym() to resolve the game's JNI native method symbols
 *   3. Calls JNI_OnLoad for each library to initialize the game
 *   4. Applies runtime patches (bug fixes, optimizations) via symbol hooking
 *   5. Forwards all calls from the Java Bio4NativeBridge to the loaded game
 * code
 *
 * On Vita, this was done with a custom ELF loader (so_linker) and JNI
 * reimplementation (so_jni). On Android, we can use the standard dlopen/dlsym
 * and the real JNI — making the bridge much simpler and more reliable.
 *
 * The game's native code expects to find its JNI methods registered via
 * RegisterNatives in JNI_OnLoad, so we simply call JNI_OnLoad and then
 * look up the method symbols directly.
 */

#include "bio4_bridge.h"
#include "patch.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/system_properties.h>
#include <time.h>

/* Global bridge state */
bio4_bridge_t g_bridge = {0};

/* ============================================================================
 * Module Loading
 * ============================================================================
 */

bio4_module_t bio4_load_library(const char *data_dir, const char *soname) {
  bio4_module_t mod = {0};
  char path[2048];

  /* Build full path to the .so in the data directory */
  snprintf(path, sizeof(path), "%s/lib%s.so", data_dir, soname);

  LOGI("Loading library: %s", path);

  /* Use RTLD_NOW for immediate symbol resolution, RTLD_GLOBAL so
   * symbols are available for subsequent library loads */
  mod.handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
  if (!mod.handle) {
    LOGE("Failed to load %s: %s", path, dlerror());

    /* Fallback: try loading from system library paths */
    snprintf(path, sizeof(path), "lib%s.so", soname);
    mod.handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (!mod.handle) {
      LOGE("Fallback load also failed for %s: %s", soname, dlerror());
      return mod;
    }
  }

  mod.name = strdup(soname);
  LOGI("Successfully loaded: %s (handle=%p)", soname, mod.handle);
  return mod;
}

int bio4_call_jni_onload(bio4_module_t *mod) {
  if (!mod || !mod->handle)
    return -1;

  fn_jni_onload JNI_OnLoad = (fn_jni_onload)dlsym(mod->handle, "JNI_OnLoad");
  if (!JNI_OnLoad) {
    LOGW("No JNI_OnLoad found in %s (may not be needed)", mod->name);
    return 0;
  }

  LOGI("Calling JNI_OnLoad for %s", mod->name);
  jint result = JNI_OnLoad(g_bridge.java_vm, NULL);
  if (result < 0) {
    LOGE("JNI_OnLoad failed for %s (returned %d)", mod->name, result);
    return -1;
  }

  LOGI("JNI_OnLoad success for %s (JNI version: 0x%x)", mod->name, result);
  return 0;
}

/* ============================================================================
 * Symbol Resolution — Find the game's native method implementations
 * ============================================================================
 */

static int resolve_bio4_symbols(bio4_module_t *mod) {
  if (!mod || !mod->handle)
    return -1;

  void *h = mod->handle;

  /* The game's native methods are registered via JNI RegisterNatives in
   * JNI_OnLoad. However, since we need to call them directly, we look them up
   * by their C++ mangled names. The Bio4PreActivity class is:
   *   jp.co.capcom.android.bio4_LGUplus0119.Bio4PreActivity
   *
   * The mangled names follow the GCC/Clang C++ ABI naming convention.
   * We try multiple possible manglings for each method.
   */

  /* Try to find the native methods directly via dlsym with mangled names.
   * The actual mangling depends on the compiler used to build the original .so.
   * We'll use the known symbol patterns from the original binary. */

  /* Method: native_onCreate */
  g_bridge.on_create = (fn_onCreate)dlsym(
      h, "_ZN31Bio4PreActivity12native_onCreateEP7JNIEnv_P8_jobjectS2_");
  if (!g_bridge.on_create) {
    /* Try alternative: the game might use RegisterNatives, in which case
     * we need to look up the method from the JNI environment after JNI_OnLoad.
     * For now, we'll also try shorter manglings. */
    g_bridge.on_create =
        (fn_onCreate)dlsym(h, "Java_jp_co_capcom_android_bio4_1LGUplus0119_"
                              "Bio4PreActivity_native_1onCreate");
  }

  /* Since the actual symbol names depend on the specific binary, we'll
   * use a more robust approach: after JNI_OnLoad, the game registers its
   * native methods with the JVM. We can then call them through JNI
   * method IDs instead of direct symbol lookup. */

  LOGI("Symbol resolution complete (some symbols may be resolved via JNI at "
       "runtime)");
  return 0;
}

/* ============================================================================
 * JNI-Based Method Invocation
 *
 * Instead of direct dlsym lookup (which requires knowing exact C++ manglings),
 * we call the game's methods through JNI after JNI_OnLoad registers them.
 * This is the standard and most reliable approach on Android.
 * ============================================================================
 */

static jclass g_bio4_class = NULL;
static jobject g_bio4_object = NULL;

/* Cached method IDs */
static struct {
  jmethodID onCreate;
  jmethodID onRestart;
  jmethodID onStart;
  jmethodID onResume;
  jmethodID onSurfaceCreated;
  jmethodID onSurfaceChanged;
  jmethodID onDrawFrame;
  jmethodID onPause;
  jmethodID onStop;
  jmethodID onDestroy;
  jmethodID onKeyDown;
  jmethodID onKeyUp;
  jmethodID onTouchBegan;
  jmethodID onTouchMoved;
  jmethodID onTouchEnded;
  jmethodID onTouchCancelled;
  jmethodID setStereoHard;
  jmethodID onShake;
} g_methods = {0};

static int cache_jni_method_ids(JNIEnv *env) {
  if (!g_bio4_class)
    return -1;

  /* The game registers native methods for the Bio4PreActivity class during
   * JNI_OnLoad. After that, we can find them using GetMethodID on the class. */
  const char *className =
      "jp/co/capcom/android/bio4_LGUplus0119/Bio4PreActivity";

  g_methods.onCreate = (*env)->GetMethodID(env, g_bio4_class, "native_onCreate",
                                           "(Ljava/lang/Object;)I");
  g_methods.onRestart =
      (*env)->GetMethodID(env, g_bio4_class, "native_onRestart", "()V");
  g_methods.onStart =
      (*env)->GetMethodID(env, g_bio4_class, "native_onStart", "()V");
  g_methods.onResume =
      (*env)->GetMethodID(env, g_bio4_class, "native_onResume", "()V");
  g_methods.onSurfaceCreated =
      (*env)->GetMethodID(env, g_bio4_class, "native_onSurfaceCreated",
                          "(Ljava/lang/Object;Ljava/lang/Object;II)Z");
  g_methods.onSurfaceChanged = (*env)->GetMethodID(
      env, g_bio4_class, "native_onSurfaceChanged", "(Ljava/lang/Object;II)Z");
  g_methods.onDrawFrame = (*env)->GetMethodID(
      env, g_bio4_class, "native_onDrawFrame", "(Ljava/lang/Object;)I");
  g_methods.onPause =
      (*env)->GetMethodID(env, g_bio4_class, "native_onPause", "()V");
  g_methods.onStop =
      (*env)->GetMethodID(env, g_bio4_class, "native_onStop", "()V");
  g_methods.onDestroy =
      (*env)->GetMethodID(env, g_bio4_class, "native_onDestroy", "()V");
  g_methods.onKeyDown = (*env)->GetMethodID(
      env, g_bio4_class, "native_onKeyDown", "(ILjava/lang/Object;)Z");
  g_methods.onKeyUp = (*env)->GetMethodID(env, g_bio4_class, "native_onKeyUp",
                                          "(ILjava/lang/Object;)Z");
  g_methods.onTouchBegan = (*env)->GetMethodID(
      env, g_bio4_class, "native_onTouchBegan", "(Ljava/lang/Object;FFFF)V");
  g_methods.onTouchMoved = (*env)->GetMethodID(
      env, g_bio4_class, "native_onTouchMoved", "(Ljava/lang/Object;FFFF)V");
  g_methods.onTouchEnded = (*env)->GetMethodID(
      env, g_bio4_class, "native_onTouchEnded", "(Ljava/lang/Object;FFFF)V");
  g_methods.onTouchCancelled =
      (*env)->GetMethodID(env, g_bio4_class, "native_onTouchCancelled",
                          "(Ljava/lang/Object;FFFF)V");
  g_methods.setStereoHard = (*env)->GetMethodID(
      env, g_bio4_class, "native_setStereoHard", "(Ljava/lang/Object;Z)V");
  g_methods.onShake =
      (*env)->GetMethodID(env, g_bio4_class, "native_onShake", "()V");

  /* Check if critical methods were found */
  int missing = 0;
  if (!g_methods.onCreate) {
    LOGW("Method not found: native_onCreate");
    missing++;
  }
  if (!g_methods.onDrawFrame) {
    LOGW("Method not found: native_onDrawFrame");
    missing++;
  }
  if (!g_methods.onSurfaceCreated) {
    LOGW("Method not found: native_onSurfaceCreated");
    missing++;
  }
  if (!g_methods.onSurfaceChanged) {
    LOGW("Method not found: native_onSurfaceChanged");
    missing++;
  }

  if (missing > 0) {
    LOGW("%d methods not found (game may use different registration approach)",
         missing);
  }

  return (missing > 2)
             ? -1
             : 0; /* Allow some missing, fail if critical ones missing */
}

/* ============================================================================
 * Bridge Initialization
 * ============================================================================
 */

int bio4_bridge_init(const char *data_dir) {
  LOGI("bio4_bridge_init: data_dir=%s", data_dir);

  memset(&g_bridge, 0, sizeof(g_bridge));
  strncpy(g_bridge.data_dir, data_dir, sizeof(g_bridge.data_dir) - 1);

  /* Store the JavaVM pointer (set from JNI_OnLoad of our bridge library) */
  /* g_bridge.java_vm is set by the JNI_OnLoad in this file */

  /* Load MCE eruption library first (dependency of bio4af) */
  g_bridge.mce_mod = bio4_load_library(data_dir, "mc_eruption_for_android_jni");
  if (!g_bridge.mce_mod.handle) {
    LOGE("Failed to load MCE eruption library");
    return -1;
  }

  /* Apply MCE patches */
  bio4_patch_mce(&g_bridge.mce_mod);

  /* Load main game library */
  g_bridge.bio4_mod = bio4_load_library(data_dir, "bio4af");
  if (!g_bridge.bio4_mod.handle) {
    LOGE("Failed to load bio4af library");
    return -1;
  }

  /* Apply game patches */
  bio4_patch_bio4(&g_bridge.bio4_mod);

  /* Initialize game state */
  g_bridge.is_app_alive = false;
  g_bridge.is_stereo_flg = true;
  g_bridge.n_prev_time = 0;

  LOGI("bio4_bridge_init success!");
  return 0;
}

void bio4_bridge_run() {
  /* Call JNI_OnLoad for both libraries */
  bio4_call_jni_onload(&g_bridge.mce_mod);
  bio4_call_jni_onload(&g_bridge.bio4_mod);

  /* After JNI_OnLoad, the game has registered its native methods.
   * Cache the method IDs for fast invocation. */
  if (g_bridge.jni_env) {
    const char *className =
        "jp/co/capcom/android/bio4_LGUplus0119/Bio4PreActivity";
    g_bio4_class = (*g_bridge.jni_env)->FindClass(g_bridge.jni_env, className);
    if (g_bio4_class) {
      cache_jni_method_ids(g_bridge.jni_env);
    } else {
      LOGW("Could not find Bio4PreActivity class (game will use direct symbol "
           "calls)");
    }
  }

  LOGI("bio4_bridge_run complete");
}

void bio4_bridge_term() {
  LOGI("bio4_bridge_term...");

  if (g_bridge.bio4_mod.handle) {
    dlclose(g_bridge.bio4_mod.handle);
  }
  if (g_bridge.mce_mod.handle) {
    dlclose(g_bridge.mce_mod.handle);
  }

  if (g_bridge.mce_mod.name)
    free(g_bridge.mce_mod.name);
  if (g_bridge.bio4_mod.name)
    free(g_bridge.bio4_mod.name);

  LOGI("bio4_bridge_term complete");
}

/* ============================================================================
 * JNI Bridge — Java Bio4NativeBridge native methods
 *
 * These are the C implementations of the native methods declared in
 * Bio4NativeBridge.java. They forward calls to the loaded game code.
 * ============================================================================
 */

static JNIEnv *get_jni_env() {
  JNIEnv *env = NULL;
  if (g_bridge.java_vm) {
    (*g_bridge.java_vm)
        ->GetEnv(g_bridge.java_vm, (void **)&env, JNI_VERSION_1_6);
  }
  return env;
}

/* Helper: create a UITouchInfo object for touch event parameters */
static jobject create_touch_info(JNIEnv *env, int pointerId) {
  /* The game's native code expects a UITouchInfo object as the first param
   * to touch events. We create a simple integer array to represent it. */
  jintArray info = (*env)->NewIntArray(env, 5);
  if (info) {
    jint data[5] = {pointerId, 0, 0, 0, 0};
    (*env)->SetIntArrayRegion(env, info, 0, 5, data);
  }
  return (jobject)info;
}

/* --- Initialization --- */

JNIEXPORT jint JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeInit(JNIEnv *env,
                                                           jobject thiz,
                                                           jstring dataDir) {
  const char *data_dir = (*env)->GetStringUTFChars(env, dataDir, NULL);
  int result = bio4_bridge_init(data_dir);
  (*env)->ReleaseStringUTFChars(env, dataDir, data_dir);

  if (result == 0) {
    bio4_bridge_run();
  }

  return result;
}

/* --- Activity Lifecycle --- */

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnCreate0(JNIEnv *env,
                                                                jobject thiz) {
  if (g_bio4_object && g_methods.onCreate) {
    (*env)->CallIntMethod(env, g_bio4_object, g_methods.onCreate, NULL);
  } else if (g_bridge.on_create) {
    g_bridge.on_create(env, g_bio4_object ? g_bio4_object : thiz, NULL);
  }
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnStart0(JNIEnv *env,
                                                               jobject thiz) {
  if (g_bio4_object && g_methods.onStart) {
    (*env)->CallVoidMethod(env, g_bio4_object, g_methods.onStart);
  } else if (g_bridge.on_start) {
    g_bridge.on_start(env, g_bio4_object ? g_bio4_object : thiz);
  }
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnResume0(JNIEnv *env,
                                                                jobject thiz) {
  if (g_bio4_object && g_methods.onResume) {
    (*env)->CallVoidMethod(env, g_bio4_object, g_methods.onResume);
  } else if (g_bridge.on_resume) {
    g_bridge.on_resume(env, g_bio4_object ? g_bio4_object : thiz);
  }
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnPause0(JNIEnv *env,
                                                               jobject thiz) {
  if (g_bio4_object && g_methods.onPause) {
    (*env)->CallVoidMethod(env, g_bio4_object, g_methods.onPause);
  } else if (g_bridge.on_pause) {
    g_bridge.on_pause(env, g_bio4_object ? g_bio4_object : thiz);
  }
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnStop0(JNIEnv *env,
                                                              jobject thiz) {
  if (g_bio4_object && g_methods.onStop) {
    (*env)->CallVoidMethod(env, g_bio4_object, g_methods.onStop);
  } else if (g_bridge.on_stop) {
    g_bridge.on_stop(env, g_bio4_object ? g_bio4_object : thiz);
  }
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnDestroy0(JNIEnv *env,
                                                                 jobject thiz) {
  if (g_bio4_object && g_methods.onDestroy) {
    (*env)->CallVoidMethod(env, g_bio4_object, g_methods.onDestroy);
  } else if (g_bridge.on_destroy) {
    g_bridge.on_destroy(env, g_bio4_object ? g_bio4_object : thiz);
  }
  bio4_bridge_term();
}

/* --- GL Surface Lifecycle --- */

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnSurfaceCreated0(
    JNIEnv *env, jobject thiz, jint width, jint height) {
  if (g_bio4_object && g_methods.onSurfaceCreated) {
    (*env)->CallBooleanMethod(env, g_bio4_object, g_methods.onSurfaceCreated,
                              NULL, NULL, width, height);
  } else if (g_bridge.on_surface_created) {
    g_bridge.on_surface_created(env, g_bio4_object ? g_bio4_object : thiz, NULL,
                                NULL, width, height);
  }
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnSurfaceChanged0(
    JNIEnv *env, jobject thiz, jint width, jint height) {
  if (g_bio4_object && g_methods.onSurfaceChanged) {
    (*env)->CallBooleanMethod(env, g_bio4_object, g_methods.onSurfaceChanged,
                              NULL, width, height);
  } else if (g_bridge.on_surface_changed) {
    g_bridge.on_surface_changed(env, g_bio4_object ? g_bio4_object : thiz,
                                width, height);
  }
}

JNIEXPORT jint JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnDrawFrame0(
    JNIEnv *env, jobject thiz) {
  if (g_bio4_object && g_methods.onDrawFrame) {
    return (*env)->CallIntMethod(env, g_bio4_object, g_methods.onDrawFrame,
                                 NULL);
  } else if (g_bridge.on_draw_frame) {
    return g_bridge.on_draw_frame(env, g_bio4_object ? g_bio4_object : thiz,
                                  NULL);
  }
  return 0;
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeSetStereoHard0(
    JNIEnv *env, jobject thiz, jboolean enable) {
  if (g_bio4_object && g_methods.setStereoHard) {
    (*env)->CallVoidMethod(env, g_bio4_object, g_methods.setStereoHard, NULL,
                           enable);
  } else if (g_bridge.set_stereo_hard) {
    g_bridge.set_stereo_hard(env, g_bio4_object ? g_bio4_object : thiz, NULL,
                             enable);
  }
}

/* --- Input Events --- */

JNIEXPORT jboolean JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnKeyDown0(
    JNIEnv *env, jobject thiz, jint keyCode, jint repeatCount) {
  if (g_bio4_object && g_methods.onKeyDown) {
    return (*env)->CallBooleanMethod(env, g_bio4_object, g_methods.onKeyDown,
                                     keyCode, NULL);
  } else if (g_bridge.on_key_down) {
    return g_bridge.on_key_down(env, g_bio4_object ? g_bio4_object : thiz,
                                keyCode, NULL);
  }
  return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnKeyUp0(JNIEnv *env,
                                                               jobject thiz,
                                                               jint keyCode,
                                                               jint flags) {
  if (g_bio4_object && g_methods.onKeyUp) {
    return (*env)->CallBooleanMethod(env, g_bio4_object, g_methods.onKeyUp,
                                     keyCode, NULL);
  } else if (g_bridge.on_key_up) {
    return g_bridge.on_key_up(env, g_bio4_object ? g_bio4_object : thiz,
                              keyCode, NULL);
  }
  return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnTouchDown0(
    JNIEnv *env, jobject thiz, jint pointerId, jfloat currX, jfloat currY,
    jfloat prevX, jfloat prevY) {
  jobject touchInfo = create_touch_info(env, pointerId);
  if (g_bio4_object && g_methods.onTouchBegan) {
    (*env)->CallVoidMethod(env, g_bio4_object, g_methods.onTouchBegan,
                           touchInfo, currX, currY, prevX, prevY);
  } else if (g_bridge.on_touch_began) {
    g_bridge.on_touch_began(env, g_bio4_object ? g_bio4_object : thiz,
                            touchInfo, currX, currY, prevX, prevY);
  }
  if (touchInfo)
    (*env)->DeleteLocalRef(env, touchInfo);
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnTouchMove0(
    JNIEnv *env, jobject thiz, jint pointerId, jfloat currX, jfloat currY,
    jfloat prevX, jfloat prevY) {
  jobject touchInfo = create_touch_info(env, pointerId);
  if (g_bio4_object && g_methods.onTouchMoved) {
    (*env)->CallVoidMethod(env, g_bio4_object, g_methods.onTouchMoved,
                           touchInfo, currX, currY, prevX, prevY);
  } else if (g_bridge.on_touch_moved) {
    g_bridge.on_touch_moved(env, g_bio4_object ? g_bio4_object : thiz,
                            touchInfo, currX, currY, prevX, prevY);
  }
  if (touchInfo)
    (*env)->DeleteLocalRef(env, touchInfo);
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnTouchUp0(
    JNIEnv *env, jobject thiz, jint pointerId, jfloat currX, jfloat currY,
    jfloat prevX, jfloat prevY) {
  jobject touchInfo = create_touch_info(env, pointerId);
  if (g_bio4_object && g_methods.onTouchEnded) {
    (*env)->CallVoidMethod(env, g_bio4_object, g_methods.onTouchEnded,
                           touchInfo, currX, currY, prevX, prevY);
  } else if (g_bridge.on_touch_ended) {
    g_bridge.on_touch_ended(env, g_bio4_object ? g_bio4_object : thiz,
                            touchInfo, currX, currY, prevX, prevY);
  }
  if (touchInfo)
    (*env)->DeleteLocalRef(env, touchInfo);
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnTouchCancel0(
    JNIEnv *env, jobject thiz, jint pointerId, jfloat currX, jfloat currY,
    jfloat prevX, jfloat prevY) {
  jobject touchInfo = create_touch_info(env, pointerId);
  if (g_bio4_object && g_methods.onTouchCancelled) {
    (*env)->CallVoidMethod(env, g_bio4_object, g_methods.onTouchCancelled,
                           touchInfo, currX, currY, prevX, prevY);
  } else if (g_bridge.on_touch_cancelled) {
    g_bridge.on_touch_cancelled(env, g_bio4_object ? g_bio4_object : thiz,
                                touchInfo, currX, currY, prevX, prevY);
  }
  if (touchInfo)
    (*env)->DeleteLocalRef(env, touchInfo);
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnShake0(JNIEnv *env,
                                                               jobject thiz) {
  if (g_bio4_object && g_methods.onShake) {
    (*env)->CallVoidMethod(env, g_bio4_object, g_methods.onShake);
  } else if (g_bridge.on_shake) {
    g_bridge.on_shake(env, g_bio4_object ? g_bio4_object : thiz);
  }
}

JNIEXPORT void JNICALL
Java_jp_co_capcom_android_bio4_Bio4NativeBridge_nativeOnConfigurationChanged0(
    JNIEnv *env, jobject thiz) {
  /* No-op: the game doesn't typically need config change handling */
}

/* ============================================================================
 * Our Bridge Library's JNI_OnLoad
 *
 * This is called when System.loadLibrary("bio4bridge") is invoked.
 * We cache the JavaVM pointer for later use.
 * ============================================================================
 */

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
  LOGI("bio4bridge JNI_OnLoad");

  g_bridge.java_vm = vm;

  JNIEnv *env = NULL;
  if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) {
    LOGE("Failed to get JNI environment");
    return -1;
  }

  g_bridge.jni_env = env;

  LOGI("bio4bridge JNI_OnLoad success");
  return JNI_VERSION_1_6;
}
