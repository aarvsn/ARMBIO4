/**
 * patch.h — Runtime patches for the game's native code.
 *
 * These patches fix bugs and optimize performance in the original
 * game binaries (libbio4af.so and libmc_eruption_for_android_jni.so).
 *
 * Ported from the Vita project's source/patch.c, which hooked:
 *   - JavaCall::callStaticCharPtrMethod — bug fix for memory leak
 *   - PLT_Time::GetMilliSeconds — optimization to avoid per-frame JNI lookup
 *
 * On Android, we apply the same patches using dlsym + manual hooking
 * or by overriding the symbols at load time.
 */

#ifndef ARMBIO4_PATCH_H
#define ARMBIO4_PATCH_H

#include "bio4_bridge.h"

/**
 * Apply patches to the MCE eruption library.
 * Currently no patches needed.
 */
int bio4_patch_mce(bio4_module_t *mod);

/**
 * Apply patches to the main bio4af game library.
 * Fixes:
 *   - JavaCall::callStaticCharPtrMethod memory leak
 *   - PLT_Time::GetMilliSeconds per-frame JNI overhead
 */
int bio4_patch_bio4(bio4_module_t *mod);

#endif /* ARMBIO4_PATCH_H */
