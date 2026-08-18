package jp.co.capcom.android.bio4;

import android.opengl.GLSurfaceView;
import android.util.Log;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Bio4Renderer — OpenGL ES renderer for the game.
 *
 * Drives the game's native rendering lifecycle:
 *   onSurfaceCreated → onSurfaceChanged → onDrawFrame (game loop)
 *
 * The game's native code (libbio4af.so) handles all OpenGL rendering internally.
 * This renderer simply forwards the lifecycle events to the native bridge.
 *
 * Frame pacing is done natively (the original game targets ~22fps with 44ms
 * frame timing), but we also ensure the GL context is properly configured.
 */
public class Bio4Renderer implements GLSurfaceView.Renderer {

    private static final String TAG = "Bio4Renderer";

    private final Bio4PreActivity mActivity;
    private final Bio4NativeBridge mNativeBridge;

    private int mWidth = 0;
    private int mHeight = 0;
    private boolean mSurfaceCreated = false;

    public Bio4Renderer(Bio4PreActivity activity, Bio4NativeBridge bridge) {
        mActivity = activity;
        mNativeBridge = bridge;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        Log.i(TAG, "onSurfaceCreated");

        // Call native onCreate first (game initialization)
        mNativeBridge.nativeOnCreate();

        // Call native onSurfaceCreated
        mNativeBridge.nativeOnSurfaceCreated(mWidth, mHeight);

        mSurfaceCreated = true;
        mActivity.setAppAlive(true);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log.i(TAG, "onSurfaceChanged: " + width + "x" + height);

        mWidth = width;
        mHeight = height;

        if (mSurfaceCreated) {
            mNativeBridge.nativeOnSurfaceChanged(width, height);
        }
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        // Main game rendering loop — called every frame by GLSurfaceView
        // The native code handles its own frame pacing (44ms target)

        int result = mNativeBridge.nativeOnDrawFrame();

        if (result == 1) {
            // Stereo mode toggle requested
            mNativeBridge.nativeSetStereoHard(mActivity.isStereoFlg());
        }
    }

    /**
     * Get the current surface dimensions.
     */
    public int getWidth() { return mWidth; }
    public int getHeight() { return mHeight; }
}
