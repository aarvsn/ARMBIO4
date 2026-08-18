package jp.co.capcom.android.bio4;

import android.content.Context;
import android.os.Environment;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * GameDataReader — Manages the game data directory and asset extraction.
 *
 * The original Android game stores its data in the app's private data directory.
 * This class provides access to the game data directory where the native .so
 * libraries and asset files are located.
 *
 * Data can be sourced from:
 *   1. Bundled assets (extracted from APK on first launch)
 *   2. External storage (user copies game data manually)
 *   3. App private data directory
 *
 * The data directory structure should be:
 *   dataDir/
 *     libbio4af.so                          — main game native library
 *     libmc_eruption_for_android_jni.so     — MCE eruption JNI library
 *     assets/                              — game assets (textures, sounds, etc.)
 *     cache/                               — cache directory
 */
public class GameDataReader {

    private static final String TAG = "Bio4Data";

    private static GameDataReader sInstance = null;

    private final Context mContext;
    private File mDataDir;
    private File mAssetsDir;
    private File mCacheDir;
    private boolean mDataExtracted = false;

    public static synchronized GameDataReader getInstance(Context context) {
        if (sInstance == null) {
            sInstance = new GameDataReader(context);
        }
        return sInstance;
    }

    private GameDataReader(Context context) {
        mContext = context.getApplicationContext();

        // Primary: app private data directory
        mDataDir = new File(mContext.getFilesDir(), "Bio4");

        // Fallback: external storage
        if (!mDataDir.exists()) {
            File external = new File(Environment.getExternalStorageDirectory(), "Bio4");
            if (external.exists()) {
                mDataDir = external;
            }
        }

        // Alternative: ux0:data/Bio4/ equivalent on Android
        if (!mDataDir.exists()) {
            File altData = new File("/sdcard/data/Bio4");
            if (altData.exists()) {
                mDataDir = altData;
            }
        }

        mAssetsDir = new File(mDataDir, "assets");
        mCacheDir = new File(mDataDir, "cache");

        // Ensure directories exist
        mDataDir.mkdirs();
        mAssetsDir.mkdirs();
        mCacheDir.mkdirs();
    }

    /**
     * Get the game data directory.
     */
    public File getDataDir() {
        return mDataDir;
    }

    /**
     * Get the game assets directory.
     */
    public File getAssetsDir() {
        return mAssetsDir;
    }

    /**
     * Get the cache directory.
     */
    public File getCacheDir() {
        return mCacheDir;
    }

    /**
     * Get the path to the main game library.
     */
    public File getBio4Library() {
        return new File(mDataDir, "libbio4af.so");
    }

    /**
     * Get the path to the MCE eruption JNI library.
     */
    public File getMceLibrary() {
        return new File(mDataDir, "libmc_eruption_for_android_jni.so");
    }

    /**
     * Check if game data exists.
     */
    public boolean hasGameData() {
        return mDataDir.exists() && mDataDir.isDirectory();
    }

    /**
     * Check if native libraries exist.
     */
    public boolean hasNativeLibraries() {
        return getBio4Library().exists() && getMceLibrary().exists();
    }

    /**
     * Extract assets from the APK to the data directory on first launch.
     * This handles any bundled game data that should be accessible via
     * file paths (the native code uses fopen, not AAssetManager).
     */
    public synchronized boolean extractAssetsIfNeeded() {
        if (mDataExtracted) return true;

        File marker = new File(mDataDir, ".extracted");
        if (marker.exists()) {
            mDataExtracted = true;
            return true;
        }

        try {
            String[] assetList = mContext.getAssets().list("");
            if (assetList != null && assetList.length > 0) {
                for (String asset : assetList) {
                    if (asset.startsWith("lib") || asset.endsWith(".so")) {
                        copyAsset(asset, new File(mDataDir, asset));
                    }
                }
            }

            // Copy assets subdirectory
            String[] subAssets = mContext.getAssets().list("assets");
            if (subAssets != null) {
                for (String asset : subAssets) {
                    copyAsset("assets/" + asset, new File(mAssetsDir, asset));
                }
            }

            marker.createNewFile();
            mDataExtracted = true;
            Log.i(TAG, "Assets extracted successfully");
            return true;

        } catch (IOException e) {
            Log.e(TAG, "Failed to extract assets", e);
            return false;
        }
    }

    private void copyAsset(String assetName, File destFile) throws IOException {
        InputStream is = null;
        OutputStream os = null;
        try {
            is = mContext.getAssets().open(assetName);
            os = new FileOutputStream(destFile);
            byte[] buffer = new byte[8192];
            int read;
            while ((read = is.read(buffer)) != -1) {
                os.write(buffer, 0, read);
            }
        } finally {
            if (is != null) try { is.close(); } catch (IOException e) {}
            if (os != null) try { os.close(); } catch (IOException e) {}
        }
    }
}
