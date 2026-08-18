package jp.co.capcom.android.bio4;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.Log;
import android.view.MotionEvent;

/**
 * Bio4GLSurfaceView — Custom OpenGL ES surface view for the game.
 *
 * Uses OpenGL ES 2.0 context (the game uses GLESv1_CM and GLESv2 via the
 * unified EGL context). Touch events are forwarded directly to the native
 * bridge since the game uses multi-touch input for gameplay.
 *
 * The GLSurfaceView is set to RENDERMODE_CONTINUOUSLY so the game loop
 * runs at the display's refresh rate, with frame pacing handled in the
 * native onDrawFrame callback.
 */
public class Bio4GLSurfaceView extends GLSurfaceView {

    private static final String TAG = "Bio4GL";

    private final Bio4Renderer mRenderer;
    private Bio4NativeBridge mNativeBridge;

    public Bio4GLSurfaceView(Context context, Bio4Renderer renderer) {
        super(context);

        mRenderer = renderer;

        // Request OpenGL ES 2.0 context
        setEGLContextClientVersion(2);

        // Preserve EGL context on pause (so game doesn't lose GL resources)
        setPreserveEGLContextOnPause(true);

        // Set the renderer
        setRenderer(mRenderer);

        // Continuous rendering — the game drives its own frame timing
        setRenderMode(RENDERMODE_CONTINUOUSLY);

        // Focusable for key events
        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    /**
     * Set the native bridge for input forwarding.
     */
    public void setNativeBridge(Bio4NativeBridge bridge) {
        mNativeBridge = bridge;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (mNativeBridge != null) {
            mNativeBridge.onTouchEvent(event);
            return true;
        }
        return super.onTouchEvent(event);
    }

    @Override
    public boolean onKeyDown(int keyCode, android.view.KeyEvent event) {
        if (mNativeBridge != null && mNativeBridge.onKeyDown(keyCode, event)) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, android.view.KeyEvent event) {
        if (mNativeBridge != null && mNativeBridge.onKeyUp(keyCode, event)) {
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }
}
