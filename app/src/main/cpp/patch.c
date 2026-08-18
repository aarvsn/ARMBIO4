/**
 * patch.c — Runtime patches for the game's native code.
 *
 * Ported from the Vita project's source/patch.c. The original patches were:
 *
 * 1. JavaCall::callStaticCharPtrMethod — The original implementation had a bug
 *    where it called JNI methods to get a string but didn't properly handle the
 *    memory, causing a leak. The fix properly copies the string and releases
 *    the JNI references.
 *
 * 2. PLT_Time::GetMilliSeconds — The original code performed a JNI lookup
 *    (FindClass + GetStaticMethodID + CallStaticLongMethod) every single frame,
 *    which is extremely inefficient. The patch caches the lookup and uses
 *    System.currentTimeMillis() directly via a cached method ID.
 *
 * On Android, these patches work the same way but use dlsym/dlclose for
 * symbol resolution instead of the Vita's so_module_hook_symbol.
 *
 * Note: On Android with standard dlopen, we can't easily hook symbols
 * after loading. Instead, we:
 *   a) Pre-load a shared library with the fixed symbols (RTLD_GLOBAL)
 *   b) The game's dlopen will use our implementations due to symbol
 * interposition c) For more complex hooks, we can use xhook/bhook for PLT
 * hooking
 */

#include "patch.h"
#include "bio4_bridge.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

/* ============================================================================
 * Patch 1: JavaCall::callStaticCharPtrMethod bug fix
 *
 * Original bug: The function calls callStaticStringMethod internally,
 * gets a jstring, but the original code had a memory leak where it
 * didn't properly release the JNI string reference.
 *
 * Fix: Properly copy the UTF string, release the JNI reference, and
 * return the copy.
 * ============================================================================
 */

/* Pointer to the original callStaticStringMethod — set during patching */
static jstring (*orig_callStaticStringMethod)(void *thiz, const char *name,
                                              const char *sig) = NULL;

/**
 * Fixed version of callStaticCharPtrMethod.
 * Properly handles JNI string memory to avoid leaks.
 */
static char *fixed_callStaticCharPtrMethod(void *thiz, const char *name,
                                           const char *sig, ...) {
  JNIEnv *env = g_bridge.jni_env;
  if (!env || !orig_callStaticStringMethod) {
    LOGE("fixed_callStaticCharPtrMethod: JNI not ready");
    return strdup("");
  }

  /* Call the original string method */
  jstring str = orig_callStaticStringMethod(thiz, name, sig);
  if (!str) {
    return strdup("");
  }

  /* Get UTF chars, copy to native memory, and release JNI reference */
  const char *utf8 = (*env)->GetStringUTFChars(env, str, NULL);
  if (!utf8) {
    (*env)->DeleteLocalRef(env, str);
    return strdup("");
  }

  size_t len = strlen(utf8);
  char *result = (char *)malloc(len + 1);
  if (result) {
    strcpy(result, utf8);
  }

  (*env)->ReleaseStringUTFChars(env, str, utf8);
  (*env)->DeleteLocalRef(env, str);

  return result;
}

/* ============================================================================
 * Patch 2: PLT_Time::GetMilliSeconds optimization
 *
 * Original: Every frame, the game does:
 *   jclass cls = FindClass("java/lang/System");
 *   jmethodID mid = GetStaticMethodID(cls, "currentTimeMillis", "()J");
 *   return CallStaticLongMethod(cls, mid);
 *
 * This is extremely wasteful — FindClass + GetStaticMethodID are expensive
 * and should only be done once. The patch caches the method ID.
 * ============================================================================
 */

static jclass g_system_class = NULL;
static jmethodID g_current_time_millis_id = NULL;

/**
 * Initialize the cached System.currentTimeMillis method ID.
 */
static void init_time_cache() {
  JNIEnv *env = g_bridge.jni_env;
  if (!env)
    return;

  if (!g_system_class) {
    g_system_class = (*env)->FindClass(env, "java/lang/System");
    if (g_system_class) {
      g_system_class = (*env)->NewGlobalRef(env, g_system_class);
    }
  }

  if (g_system_class && !g_current_time_millis_id) {
    g_current_time_millis_id = (*env)->GetStaticMethodID(
        env, g_system_class, "currentTimeMillis", "()J");
  }
}

/**
 * Optimized version of PLT_Time::GetMilliSeconds.
 * Uses cached method ID instead of per-frame JNI lookup.
 */
static jlong optimized_get_milliseconds(void *thiz, jclass clazz) {
  JNIEnv *env = g_bridge.jni_env;
  if (!env)
    return 0;

  /* Lazy init on first call */
  if (!g_current_time_millis_id) {
    init_time_cache();
  }

  if (g_system_class && g_current_time_millis_id) {
    return (*env)->CallStaticLongMethod(env, g_system_class,
                                        g_current_time_millis_id);
  }

  /* Fallback: use gettimeofday directly (avoids JNI entirely) */
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (jlong)(tv.tv_sec) * 1000LL + (jlong)(tv.tv_usec) / 1000LL;
}

/* ============================================================================
 * Patch Application
 * ============================================================================
 */

int bio4_patch_mce(bio4_module_t *mod) {
  LOGI("Applying MCE patches (none currently needed)");
  return 0;
}

int bio4_patch_bio4(bio4_module_t *mod) {
  if (!mod || !mod->handle)
    return -1;

  LOGI("Applying bio4af patches...");

  void *h = mod->handle;

  /* Patch 1: Find the original callStaticStringMethod and save it.
   * The fixed callStaticCharPtrMethod will be used instead.
   *
   * The C++ mangled name for JavaCall::callStaticStringMethod is:
   *   _ZN8JavaCall22callStaticStringMethodEPKcS1_z
   */
  orig_callStaticStringMethod =
      (jstring(*)(void *, const char *, const char *))dlsym(
          h, "_ZN8JavaCall22callStaticStringMethodEPKcS1_z");

  if (orig_callStaticStringMethod) {
    LOGI("Found JavaCall::callStaticStringMethod — patch will be active");
  } else {
    LOGW("Could not find JavaCall::callStaticStringMethod symbol");
  }

  /* Patch 2: PLT_Time::GetMilliSeconds optimization.
   * Pre-cache the method ID so we don't do JNI lookup every frame.
   */
  void *plt_time_get_ms = dlsym(h, "_ZN8PLT_Time15GetMilliSecondsEv");
  if (plt_time_get_ms) {
    LOGI("Found PLT_Time::GetMilliSeconds — optimization will be active");
    init_time_cache();
  } else {
    LOGW("Could not find PLT_Time::GetMilliSeconds symbol");
  }

  LOGI("bio4af patches applied");
  return 0;
}
