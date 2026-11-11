package com.ravi.gta5port;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.view.Gravity;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.FrameLayout;
import android.widget.TextView;

public final class MainActivity extends Activity implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("gta5_android");
    }

    private TextView status;
    private boolean nativeReady;

    private static native boolean nativeCreate(Surface surface);
    private static native boolean nativeResize(int width, int height);
    private static native void nativeDestroy();

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().getDecorView().setSystemUiVisibility(
            5894 | 512 | 1024 | 4096
        );

        FrameLayout root = new FrameLayout(this);
        root.setBackgroundColor(Color.rgb(8, 12, 20));

        SurfaceView surface = new SurfaceView(this);
        surface.getHolder().addCallback(this);
        root.addView(surface, new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT
        ));

        TextView title = new TextView(this);
        title.setText("GTA V  •  ANDROID PORT");
        title.setTextColor(Color.WHITE);
        title.setTextSize(18);
        title.setGravity(Gravity.CENTER);
        title.setPadding(24, 18, 24, 18);
        FrameLayout.LayoutParams titleParams = new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.WRAP_CONTENT,
            FrameLayout.LayoutParams.WRAP_CONTENT,
            Gravity.TOP | Gravity.CENTER_HORIZONTAL
        );
        root.addView(title, titleParams);

        status = new TextView(this);
        status.setText("Preparing Vulkan surface…");
        status.setTextColor(Color.rgb(220, 226, 238));
        status.setTextSize(13);
        status.setGravity(Gravity.CENTER);
        status.setPadding(20, 12, 20, 12);
        FrameLayout.LayoutParams statusParams = new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.WRAP_CONTENT,
            FrameLayout.LayoutParams.WRAP_CONTENT,
            Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL
        );
        root.addView(status, statusParams);
        setContentView(root);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        nativeReady = nativeCreate(holder.getSurface());
        status.setText(nativeReady
            ? "Vulkan device and Android surface initialized"
            : "Vulkan initialization unavailable on this device");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (nativeReady && !nativeResize(width, height)) {
            nativeReady = false;
            status.setText("Vulkan surface resize failed");
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        nativeDestroy();
        nativeReady = false;
        status.setText("Vulkan surface released");
    }

    @Override
    protected void onDestroy() {
        nativeDestroy();
        nativeReady = false;
        super.onDestroy();
    }
}
