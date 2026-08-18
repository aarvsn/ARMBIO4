package jp.co.capcom.android.bio4;

import android.content.Context;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;

/**
 * Bio4NativeBridge — Singleton bridge between Java and native game code.
 *
 * This class is responsible for:
 *   1. Loading the game's native shared libraries (libbio4af.so, libmc_eruption_for_android_jni.so)
 *   2. Calling JNI_OnLoad for library initialization
 *   3. Forwarding Activity lifecycle events to native code
 *   4. Forwarding input events (touch, key, accelerometer) to native code
 *   5. Applying runtime patches to the loaded game code
 *
 * In the original Vita port, all of this was done through a custom so_linker + so_jni
 * reimplementation. On Android, we can use the standard System.loadLibrary / dlopen
 * and the native JNI bridge handles the rest.
 */
public class Bio4NativeBridge {

    private static final String TAG = "Bio4Bridge";

    private static Bio4NativeBridge sInstance = null;

    private boolean mInitialized = false;
    private Context mContext;

    // Native library handles
    private long mMceHandle = 0;
    private long mBio4Handle = 0;

    // Singleton
    public static synchronized Bio4NativeBridge getInstance() {
        if (sInstance == null) {
            sInstance = new Bio4NativeBridge();
        }
        return sInstance;
    }

    private Bio4NativeBridge() {}

    /**
     * Initialize the bridge — load native libraries and call JNI_OnLoad.
     *
     * @return true if initialization succeeded
     */
    public boolean init(Context context) {
        if (mInitialized) return true;

        mContext = context.getApplicationContext();

        try {
            // Load our bridge library first
            System.loadLibrary("bio4bridge");

            // Then load the game libraries via the native loader
            // (which uses dlopen to load from the game data directory)
            GameDataReader dataReader = GameDataReader.getInstance(context);
            String dataDir = dataReader.getDataDir().getAbsolutePath();

            int ret = nativeInit(dataDir);
            if (ret != 0) {
                Log.e(TAG, "nativeInit failed: " + ret);
                return false;
            }

            mInitialized = true;
            Log.i(TAG, "Bio4NativeBridge initialized successfully");

        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load native bridge library", e);
            return false;
        }

        return true;
    }

    public boolean isInitialized() {
        return mInitialized;
    }

    // --- Activity Lifecycle Callbacks ---

    public void nativeOnCreate() {
        if (mInitialized) nativeOnCreate0();
    }

    public void nativeOnStart() {
        if (mInitialized) nativeOnStart0();
    }

    public void nativeOnResume() {
        if (mInitialized) nativeOnResume0();
    }

    public void nativeOnPause() {
        if (mInitialized) nativeOnPause0();
    }

    public void nativeOnStop() {
        if (mInitialized) nativeOnStop0();
    }

    public void nativeOnDestroy() {
        if (mInitialized) nativeOnDestroy0();
    }

    public void nativeOnSurfaceCreated(int width, int height) {
        if (mInitialized) nativeOnSurfaceCreated0(width, height);
    }

    public void nativeOnSurfaceChanged(int width, int height) {
        if (mInitialized) nativeOnSurfaceChanged0(width, height);
    }

    public int nativeOnDrawFrame() {
        if (mInitialized) return nativeOnDrawFrame0();
        return 0;
    }

    public void nativeSetStereoHard(boolean enable) {
        if (mInitialized) nativeSetStereoHard0(enable);
    }

    // --- Input Events ---

    /**
     * Forward a key-down event to native code.
     * Maps Android keycodes to the game's expected keycodes.
     */
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (mInitialized) {
            return nativeOnKeyDown0(keyCode, event.getRepeatCount());
        }
        return false;
    }

    /**
     * Forward a key-up event to native code.
     */
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (mInitialized) {
            return nativeOnKeyUp0(keyCode, 0);
        }
        return false;
    }

    /**
     * Forward a touch event to native code.
     * Handles multi-touch with pointer ID tracking.
     */
    public boolean onTouchEvent(MotionEvent event) {
        if (!mInitialized) return false;

        int action = event.getActionMasked();
        int pointerIndex = event.getActionIndex();
        int pointerId = event.getPointerId(pointerIndex);
        float x = event.getX(pointerIndex);
        float y = event.getY(pointerIndex);

        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
                nativeOnTouchDown0(pointerId, x, y, x, y);
                return true;

            case MotionEvent.ACTION_MOVE:
                // Send move for all active pointers
                for (int i = 0; i < event.getPointerCount(); i++) {
                    int pid = event.getPointerId(i);
                    float cx = event.getX(i);
                    float cy = event.getY(i);
                    // History for prev position
                    float px, py;
                    if (event.getHistorySize() > 0) {
                        int histIdx = event.getHistorySize() - 1;
                        px = event.getHistoricalX(i, histIdx);
                        py = event.getHistoricalY(i, histIdx);
                    } else {
                        px = cx;
                        py = cy;
                    }
                    nativeOnTouchMove0(pid, cx, cy, px, py);
                }
                return true;

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
                nativeOnTouchUp0(pointerId, x, y, x, y);
                return true;

            case MotionEvent.ACTION_CANCEL:
                nativeOnTouchCancel0(pointerId, x, y, x, y);
                return true;
        }

        return false;
    }

    /**
     * Forward shake/motion event to native code.
     */
    public void onShake() {
        if (mInitialized) nativeOnShake0();
    }

    // --- Native Methods ---

    // Initialization
    private native int nativeInit(String dataDir);

    // Activity lifecycle
    private native void nativeOnCreate0();
    private native void nativeOnStart0();
    private native void nativeOnResume0();
    private native void nativeOnPause0();
    private native void nativeOnStop0();
    private native void nativeOnDestroy0();

    // GL surface lifecycle
    private native void nativeOnSurfaceCreated0(int width, int height);
    private native void nativeOnSurfaceChanged0(int width, int height);
    private native int  nativeOnDrawFrame0();
    private native void nativeSetStereoHard0(boolean enable);

    // Input events
    private native boolean nativeOnKeyDown0(int keyCode, int repeatCount);
    private native boolean nativeOnKeyUp0(int keyCode, int flags);
    private native void nativeOnTouchDown0(int pointerId, float currX, float currY, float prevX, float prevY);
    private native void nativeOnTouchMove0(int pointerId, float currX, float currY, float prevX, float prevY);
    private native void nativeOnTouchUp0(int pointerId, float currX, float currY, float prevX, float prevY);
    private native void nativeOnTouchCancel0(int pointerId, float currX, float currY, float prevX, float prevY);
    private native void nativeOnShake0();

    // Configuration
    public native void nativeOnConfigurationChanged0();
}
