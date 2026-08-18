package jp.co.capcom.android.bio4;

import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;

/**
 * AccelerometerBridge — Bridges Android accelerometer sensor data to the game.
 *
 * The original Android game uses Accelerometer class (via JNI) for motion/shake
 * detection. This bridge reads the hardware accelerometer and triggers the
 * native onShake callback when the device is shaken.
 *
 * Shake detection uses a simple threshold on the acceleration magnitude delta
 * from gravity, matching the original game's behavior.
 */
public class AccelerometerBridge implements SensorEventListener {

    private static final String TAG = "Bio4Accel";

    // Shake detection threshold (m/s^2)
    private static final float SHAKE_THRESHOLD = 12.0f;
    // Minimum time between shake events (ms)
    private static final long SHAKE_COOLDOWN_MS = 500;

    private final SensorManager mSensorManager;
    private final Sensor mAccelerometer;
    private final Bio4NativeBridge mNativeBridge;

    private long mLastShakeTime = 0;
    private float mLastX = 0, mLastY = 0, mLastZ = 0;
    private boolean mRegistered = false;

    public AccelerometerBridge(SensorManager sensorManager, Sensor accelerometer, Bio4NativeBridge bridge) {
        mSensorManager = sensorManager;
        mAccelerometer = accelerometer;
        mNativeBridge = bridge;
    }

    public void register() {
        if (!mRegistered) {
            mSensorManager.registerListener(this, mAccelerometer, SensorManager.SENSOR_DELAY_GAME);
            mRegistered = true;
        }
    }

    public void unregister() {
        if (mRegistered) {
            mSensorManager.unregisterListener(this);
            mRegistered = false;
        }
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        if (event.sensor.getType() != Sensor.TYPE_ACCELEROMETER) return;

        float x = event.values[0];
        float y = event.values[1];
        float z = event.values[2];

        // Calculate acceleration delta from last sample
        float deltaX = Math.abs(x - mLastX);
        float deltaY = Math.abs(y - mLastY);
        float deltaZ = Math.abs(z - mLastZ);

        mLastX = x;
        mLastY = y;
        mLastZ = z;

        // Check for shake
        if (deltaX + deltaY + deltaZ > SHAKE_THRESHOLD) {
            long now = System.currentTimeMillis();
            if (now - mLastShakeTime > SHAKE_COOLDOWN_MS) {
                mLastShakeTime = now;
                if (mNativeBridge != null) {
                    mNativeBridge.onShake();
                }
            }
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        // Not used
    }
}
