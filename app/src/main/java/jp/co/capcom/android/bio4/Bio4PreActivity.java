package jp.co.capcom.android.bio4;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.Toast;

import java.io.File;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Bio4PreActivity — Main game Activity.
 *
 * This Activity mirrors the original Android game's Bio4PreActivity lifecycle.
 * It loads the native game libraries (libbio4af.so, libmc_eruption_for_android_jni.so)
 * and drives the game through its standard Activity lifecycle callbacks:
 *   onCreate → onStart → onResume → onSurfaceCreated → onSurfaceChanged → onDrawFrame (loop)
 *
 * The game's native code uses JNI callbacks that map to this Activity's methods.
 * All rendering is done via OpenGL ES on a GLSurfaceView.
 */
public class Bio4PreActivity extends Activity {

    private static final String TAG = "Bio4";

    // Native library names
    private static final String LIB_MCE = "mc_eruption_for_android_jni";
    private static final String LIB_BIO4 = "bio4af";

    // Game state
    private boolean mIsAppAlive = false;
    private boolean mIsStereoFlg = true;
    private long m_nPrevTime = 0;
    private boolean mIsLaunchHeapCheck = true;
    private String mArmAID = "AQ00114230";

    // UI
    private Bio4GLSurfaceView mGLSurfaceView;
    private Bio4Renderer mRenderer;
    private FrameLayout mContainer;

    // Sensors
    private SensorManager mSensorManager;
    private AccelerometerBridge mAccelerometerBridge;
    private boolean mHasAccelerometer = false;

    // Bridge to native
    private Bio4NativeBridge mNativeBridge;

    // Target frame time: ~22fps (44ms per frame) — matches original game timing
    private static final long TARGET_FRAME_TIME_MS = 44;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "Main onCreate");

        // Fullscreen immersive mode
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        );

        // Verify game data exists
        if (!verifyGameData()) {
            Toast.makeText(this, R.string.error_no_data, Toast.LENGTH_LONG).show();
            finish();
            return;
        }

        // Initialize native bridge (loads .so libraries)
        mNativeBridge = Bio4NativeBridge.getInstance();
        if (!mNativeBridge.isInitialized()) {
            if (!mNativeBridge.init(this)) {
                Toast.makeText(this, R.string.error_no_so, Toast.LENGTH_LONG).show();
                finish();
                return;
            }
        }

        // Setup UI
        setContentView(R.layout.activity_main);
        mContainer = findViewById(R.id.game_container);

        // Create renderer
        mRenderer = new Bio4Renderer(this, mNativeBridge);

        // Create GLSurfaceView with OpenGL ES 2.0
        mGLSurfaceView = new Bio4GLSurfaceView(this, mRenderer);
        mContainer.addView(mGLSurfaceView);

        // Initialize accelerometer
        setupAccelerometer();

        m_nPrevTime = System.currentTimeMillis();
        Log.i(TAG, "Main onCreate complete");
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.i(TAG, "Main onStart");
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        Log.i(TAG, "Main onRestart");
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.i(TAG, "Main onResume");
        if (mGLSurfaceView != null) {
            mGLSurfaceView.onResume();
        }
        if (mHasAccelerometer && mAccelerometerBridge != null) {
            mAccelerometerBridge.register();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.i(TAG, "Main onPause");
        if (mGLSurfaceView != null) {
            mGLSurfaceView.onPause();
        }
        if (mHasAccelerometer && mAccelerometerBridge != null) {
            mAccelerometerBridge.unregister();
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.i(TAG, "Main onStop");
        mIsStereoFlg = false;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "Main onDestroy");
        mIsAppAlive = false;
        if (mNativeBridge != null) {
            mNativeBridge.nativeOnDestroy();
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        // Don't recreate Activity on config change — game handles orientation
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (mNativeBridge != null && mNativeBridge.onKeyDown(keyCode, event)) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (mNativeBridge != null && mNativeBridge.onKeyUp(keyCode, event)) {
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        if (mNativeBridge != null && mNativeBridge.onTouchEvent(ev)) {
            return true;
        }
        if (mGLSurfaceView != null) {
            mGLSurfaceView.onTouchEvent(ev);
        }
        return super.dispatchTouchEvent(ev);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            // Re-apply immersive mode
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            );
        }
    }

    /**
     * Verify that the game data files are present.
     * The game expects its data directory to contain the native .so libraries
     * and asset files.
     */
    private boolean verifyGameData() {
        File dataDir = getGameDataReader().getDataDir();
        if (!dataDir.exists()) {
            Log.e(TAG, "Game data directory not found: " + dataDir.getAbsolutePath());
            return false;
        }

        // Check for the main game library
        File bio4so = new File(dataDir, "libbio4af.so");
        File mceSo = new File(dataDir, "libmc_eruption_for_android_jni.so");
        if (!bio4so.exists() || !mceSo.exists()) {
            Log.e(TAG, "Game native libraries not found in: " + dataDir.getAbsolutePath());
            // Also check jniLibs bundled with APK
            return true; // Allow to proceed; bridge will verify
        }

        return true;
    }

    /**
     * Get the game data directory reader.
     */
    public GameDataReader getGameDataReader() {
        return GameDataReader.getInstance(this);
    }

    /**
     * Setup accelerometer for motion/shake detection.
     */
    private void setupAccelerometer() {
        mSensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        if (mSensorManager != null) {
            Sensor accel = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            if (accel != null) {
                mHasAccelerometer = true;
                mAccelerometerBridge = new AccelerometerBridge(mSensorManager, accel, mNativeBridge);
            }
        }
    }

    /**
     * Launch the video player Activity for FMV cutscenes.
     */
    public void playVideo(String filePath, float volume) {
        Intent intent = new Intent(this, Bio4VideoActivity.class);
        intent.putExtra(Bio4VideoActivity.EXTRA_VIDEO_PATH, filePath);
        intent.putExtra(Bio4VideoActivity.EXTRA_VIDEO_VOLUME, volume);
        startActivity(intent);
    }

    // --- Accessors for native bridge ---

    public boolean isAppAlive() { return mIsAppAlive; }
    public void setAppAlive(boolean alive) { mIsAppAlive = alive; }
    public boolean isStereoFlg() { return mIsStereoFlg; }
    public void setStereoFlg(boolean stereo) { mIsStereoFlg = stereo; }
    public Bio4NativeBridge getNativeBridge() { return mNativeBridge; }
    public Bio4GLSurfaceView getGLSurfaceView() { return mGLSurfaceView; }
}
