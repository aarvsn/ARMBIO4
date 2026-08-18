package jp.co.capcom.android.bio4;

import android.os.Build;
import android.util.Log;

/**
 * DeviceInfo — Provides device information to the native game code.
 *
 * The original Android game queries various device properties for
 * graphics quality settings, stereo mode support, and compatibility.
 * This class mirrors the original DeviceInfo JNI class.
 */
public class DeviceInfo {

    private static final String TAG = "DeviceInfo";

    /**
     * Get the device model name.
     */
    public static String getDeviceModel() {
        return Build.MODEL;
    }

    /**
     * Get the device manufacturer.
     */
    public static String getDeviceManufacturer() {
        return Build.MANUFACTURER;
    }

    /**
     * Get the Android version string.
     */
    public static String getOsVersion() {
        return Build.VERSION.RELEASE;
    }

    /**
     * Get the SDK version number.
     */
    public static int getSdkVersion() {
        return Build.VERSION.SDK_INT;
    }

    /**
     * Check if the device supports stereo rendering (3D mode).
     * Most modern devices don't have auto-stereoscopic displays,
     * so this defaults to false.
     */
    public static boolean supportsStereo() {
        // The original game was for LG Optimus 3D and similar devices
        // with auto-stereoscopic displays. Modern devices don't have this.
        return false;
    }

    /**
     * Get the device's CPU ABI.
     */
    public static String getCpuAbi() {
        return Build.CPU_ABI;
    }

    /**
     * Get the number of available CPU cores.
     */
    public static int getNumCores() {
        return Runtime.getRuntime().availableProcessors();
    }

    /**
     * Get total device memory in MB.
     */
    public static long getTotalMemoryMB() {
        Runtime runtime = Runtime.getRuntime();
        // totalMemory() is the heap, not device memory
        // Use maxMemory() as a proxy
        return runtime.maxMemory() / (1024 * 1024);
    }
}
