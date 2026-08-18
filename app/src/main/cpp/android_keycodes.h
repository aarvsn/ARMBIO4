/**
 * android_keycodes.h — Android key code constants for native input handling.
 *
 * These match the values in android.view.KeyEvent from the Android SDK.
 * Used by the native bridge to map input events to the game's expected
 * keycodes.
 */

#ifndef ARMBIO4_KEYCODES_H
#define ARMBIO4_KEYCODES_H

/* D-pad */
#define AKEYCODE_DPAD_UP 19
#define AKEYCODE_DPAD_DOWN 20
#define AKEYCODE_DPAD_LEFT 21
#define AKEYCODE_DPAD_RIGHT 22
#define AKEYCODE_DPAD_CENTER 23

/* Gamepad buttons (Xbox layout) */
#define AKEYCODE_BUTTON_A 96
#define AKEYCODE_BUTTON_B 97
#define AKEYCODE_BUTTON_C 98
#define AKEYCODE_BUTTON_X 99
#define AKEYCODE_BUTTON_Y 100
#define AKEYCODE_BUTTON_Z 101

/* Triggers & Bumpers */
#define AKEYCODE_BUTTON_L1 102
#define AKEYCODE_BUTTON_R1 103
#define AKEYCODE_BUTTON_L2 104
#define AKEYCODE_BUTTON_R2 105

/* Thumb buttons */
#define AKEYCODE_THUMBL 106
#define AKEYCODE_THUMBR 107

/* Start / Select */
#define AKEYCODE_BUTTON_START 108
#define AKEYCODE_BUTTON_SELECT 109

/* Misc */
#define AKEYCODE_BACK 4
#define AKEYCODE_MENU 82
#define AKEYCODE_ENTER 66
#define AKEYCODE_ESCAPE 111

/* Volume */
#define AKEYCODE_VOLUME_UP 24
#define AKEYCODE_VOLUME_DOWN 25

#endif /* ARMBIO4_KEYCODES_H */
