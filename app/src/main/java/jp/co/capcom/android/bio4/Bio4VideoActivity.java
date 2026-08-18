package jp.co.capcom.android.bio4;

import android.app.Activity;
import android.content.Intent;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.VideoView;

import java.io.File;

/**
 * Bio4VideoActivity — Plays FMV cutscene videos.
 *
 * The original game uses SceAvPlayer on Vita for video playback.
 * On Android, we use MediaPlayer with a VideoView for hardware-accelerated
 * video decoding. The video plays fullscreen in landscape, and the Activity
 * finishes when the video ends or the user taps to skip.
 *
 * In the original Vita port, this was implemented in source/video_player.c
 * using sceAvPlayer* APIs with OpenGL texture rendering. On Android,
 * MediaPlayer handles both video decoding and rendering natively.
 */
public class Bio4VideoActivity extends Activity {

    private static final String TAG = "Bio4Video";

    public static final String EXTRA_VIDEO_PATH = "video_path";
    public static final String EXTRA_VIDEO_VOLUME = "video_volume";

    private VideoView mVideoView;
    private float mVolume = 1.0f;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Fullscreen
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
        );

        mVideoView = new VideoView(this);
        setContentView(mVideoView);

        Intent intent = getIntent();
        String videoPath = intent.getStringExtra(EXTRA_VIDEO_PATH);
        mVolume = intent.getFloatExtra(EXTRA_VIDEO_VOLUME, 1.0f);

        if (videoPath == null || videoPath.isEmpty()) {
            Log.e(TAG, "No video path provided");
            finish();
            return;
        }

        // Set audio volume
        AudioManager audioManager = (AudioManager) getSystemService(AUDIO_SERVICE);
        int maxVolume = audioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        int targetVolume = (int) (maxVolume * mVolume);
        audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, targetVolume, 0);

        // Setup video playback
        File videoFile = new File(videoPath);
        if (videoFile.exists()) {
            mVideoView.setVideoPath(videoPath);
        } else {
            // Try as asset path
            mVideoView.setVideoPath(videoPath);
        }

        mVideoView.setOnCompletionListener(mp -> {
            Log.i(TAG, "Video playback completed");
            finish();
        });

        mVideoView.setOnErrorListener((mp, what, extra) -> {
            Log.e(TAG, "Video playback error: what=" + what + " extra=" + extra);
            finish();
            return true;
        });

        mVideoView.start();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mVideoView != null && mVideoView.isPlaying()) {
            mVideoView.pause();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mVideoView != null && !mVideoView.isPlaying()) {
            mVideoView.start();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mVideoView != null) {
            mVideoView.stopPlayback();
        }
    }

    @Override
    public void onBackPressed() {
        // Skip video on back press
        finish();
    }
}
