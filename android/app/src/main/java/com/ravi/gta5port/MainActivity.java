package com.ravi.gta5port;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.view.Gravity;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public final class MainActivity extends Activity implements SurfaceHolder.Callback {
    private static final int REQUEST_GAME_FOLDER = 4101;
    private static final String PREFS = "gta5_android_port";
    private static final String KEY_FOLDER = "game_folder_uri";
    private static final String KEY_MODE = "game_mode";
    private static final String KEY_MAP = "sandbox_map";

    static {
        System.loadLibrary("gta5_android");
    }

    private TextView filesDetail;
    private TextView modeDetail;
    private TextView status;
    private TextView runtimeStatus;
    private Button storyButton;
    private Button sandboxButton;
    private Button gtaMapButton;
    private Button testMapButton;
    private Button playButton;
    private boolean nativeReady;
    private int selectedMode = 2;
    private int selectedMap = 0;
    private String selectedFolder = "";

    private static native boolean nativeCreate(Surface surface);
    private static native boolean nativeResize(int width, int height);
    private static native void nativeDestroy();

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
        );

        SharedPreferences prefs = getSharedPreferences(PREFS, MODE_PRIVATE);
        selectedFolder = prefs.getString(KEY_FOLDER, "");
        selectedMode = prefs.getInt(KEY_MODE, 2);
        selectedMap = prefs.getInt(KEY_MAP, 0);

        FrameLayout root = new FrameLayout(this);
        root.setBackground(new GradientDrawable(
            GradientDrawable.Orientation.TL_BR,
            new int[]{Color.rgb(10, 18, 32), Color.rgb(5, 9, 17), Color.rgb(13, 19, 31)}
        ));

        SurfaceView surface = new SurfaceView(this);
        surface.getHolder().addCallback(this);
        root.addView(surface, new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT
        ));

        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.HORIZONTAL);
        content.setPadding(dp(26), dp(22), dp(26), dp(18));
        root.addView(content, new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT
        ));

        ScrollView leftScroll = new ScrollView(this);
        leftScroll.setFillViewport(true);
        leftScroll.setVerticalScrollBarEnabled(false);
        LinearLayout left = new LinearLayout(this);
        left.setOrientation(LinearLayout.VERTICAL);
        left.setPadding(0, dp(8), dp(16), dp(8));
        leftScroll.addView(left);
        content.addView(leftScroll, new LinearLayout.LayoutParams(0,
            LinearLayout.LayoutParams.MATCH_PARENT, 0.44f));

        LinearLayout header = new LinearLayout(this);
        header.setGravity(Gravity.CENTER_VERTICAL);
        header.setPadding(0, 0, 0, dp(12));
        TextView icon = new TextView(this);
        icon.setText("V");
        icon.setTextColor(Color.WHITE);
        icon.setTextSize(27);
        icon.setTypeface(Typeface.DEFAULT_BOLD);
        icon.setGravity(Gravity.CENTER);
        icon.setBackground(round(Color.rgb(27, 39, 58), dp(14), Color.rgb(78, 94, 121)));
        header.addView(icon, new LinearLayout.LayoutParams(dp(54), dp(54)));
        LinearLayout titles = new LinearLayout(this);
        titles.setOrientation(LinearLayout.VERTICAL);
        titles.setPadding(dp(13), 0, 0, 0);
        TextView title = text("GTA V Port", 27, Color.WHITE, true);
        TextView subtitle = text("Fan project · plays your own game files", 12, 0xffaab5c7, false);
        titles.addView(title);
        titles.addView(subtitle);
        header.addView(titles, new LinearLayout.LayoutParams(0,
            LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        left.addView(header);

        LinearLayout filesCard = card();
        filesCard.setOrientation(LinearLayout.VERTICAL);
        filesCard.setPadding(dp(15), dp(13), dp(15), dp(13));
        filesCard.setClickable(true);
        filesCard.setFocusable(true);
        filesCard.addView(text("GAME FILES", 11, 0xffaab5c7, true));
        filesDetail = text("", 14, Color.WHITE, false);
        filesDetail.setPadding(0, dp(5), 0, 0);
        filesCard.addView(filesDetail);
        filesCard.addView(text("Choose the folder containing your own GTA V files", 11, 0xff9aa8bb, false));
        filesCard.setOnClickListener(v -> chooseGameFolder());
        LinearLayout.LayoutParams fileParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        fileParams.bottomMargin = dp(13);
        left.addView(filesCard, fileParams);
        refreshFolderLabel();

        left.addView(text("START MODE", 12, 0xffaab5c7, true));
        LinearLayout modes = new LinearLayout(this);
        modes.setOrientation(LinearLayout.HORIZONTAL);
        storyButton = modeButton("Story mode");
        sandboxButton = modeButton("Sandbox");
        modes.addView(storyButton, new LinearLayout.LayoutParams(0, dp(54), 1));
        LinearLayout.LayoutParams sandboxParams = new LinearLayout.LayoutParams(0, dp(54), 1);
        sandboxParams.leftMargin = dp(9);
        modes.addView(sandboxButton, sandboxParams);
        left.addView(modes);
        storyButton.setOnClickListener(v -> selectMode(1));
        sandboxButton.setOnClickListener(v -> selectMode(2));
        modeDetail = text("", 12, 0xffaab5c7, false);
        modeDetail.setPadding(dp(2), dp(7), dp(2), dp(9));
        left.addView(modeDetail);

        TextView mapLabel = text("MAP", 11, 0xffaab5c7, true);
        left.addView(mapLabel);
        LinearLayout maps = new LinearLayout(this);
        maps.setOrientation(LinearLayout.HORIZONTAL);
        gtaMapButton = smallButton("GTA V");
        testMapButton = smallButton("GTA VI test map");
        maps.addView(gtaMapButton, new LinearLayout.LayoutParams(0, dp(38), 1));
        LinearLayout.LayoutParams testParams = new LinearLayout.LayoutParams(0, dp(38), 1);
        testParams.leftMargin = dp(8);
        maps.addView(testMapButton, testParams);
        left.addView(maps);
        gtaMapButton.setOnClickListener(v -> selectMap(0));
        testMapButton.setOnClickListener(v -> selectMap(1));

        playButton = new Button(this);
        playButton.setText("PLAY  ·  ENGINE NOT CONNECTED");
        playButton.setTextSize(13);
        playButton.setTextColor(0xffd4dbe6);
        playButton.setAllCaps(false);
        playButton.setEnabled(false);
        playButton.setBackground(round(0xff394252, dp(14), 0xff596579));
        LinearLayout.LayoutParams playParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, dp(54));
        playParams.topMargin = dp(17);
        left.addView(playButton, playParams);

        LinearLayout right = new LinearLayout(this);
        right.setOrientation(LinearLayout.VERTICAL);
        right.setPadding(dp(16), dp(8), 0, dp(8));
        content.addView(right, new LinearLayout.LayoutParams(0,
            LinearLayout.LayoutParams.MATCH_PARENT, 0.56f));

        LinearLayout actions = new LinearLayout(this);
        actions.setGravity(Gravity.RIGHT);
        Button settings = smallButton("Settings");
        Button controls = smallButton("Controls layout");
        settings.setEnabled(false);
        controls.setEnabled(false);
        actions.addView(settings, new LinearLayout.LayoutParams(dp(112), dp(40)));
        LinearLayout.LayoutParams controlsParams = new LinearLayout.LayoutParams(dp(132), dp(40));
        controlsParams.leftMargin = dp(8);
        actions.addView(controls, controlsParams);
        right.addView(actions);

        LinearLayout logCard = card();
        logCard.setOrientation(LinearLayout.VERTICAL);
        logCard.setPadding(dp(16), dp(15), dp(16), dp(15));
        LinearLayout.LayoutParams logParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, 0, 1);
        logParams.topMargin = dp(14);
        right.addView(logCard, logParams);
        logCard.addView(text("PORT STATUS", 11, 0xffaab5c7, true));
        runtimeStatus = text("", 13, 0xffe3e9f2, false);
        runtimeStatus.setPadding(0, dp(12), 0, 0);
        runtimeStatus.setGravity(Gravity.TOP);
        logCard.addView(runtimeStatus);
        status = text("Android foundation · arm64 · Vulkan", 11, 0xff9aa8bb, false);
        status.setPadding(0, dp(12), 0, 0);
        right.addView(status);

        setContentView(root);
        refreshMode();
        refreshMap();
        refreshRuntimeStatus();
    }

    private void chooseGameFolder() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
            | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
            | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        startActivityForResult(intent, REQUEST_GAME_FOLDER);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_GAME_FOLDER || resultCode != RESULT_OK || data == null) return;
        Uri uri = data.getData();
        if (uri == null) return;
        int flags = data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        try {
            getContentResolver().takePersistableUriPermission(uri, flags);
        } catch (SecurityException ignored) {
            // The selected folder remains visible for this session if the provider won't persist access.
        }
        selectedFolder = uri.toString();
        getSharedPreferences(PREFS, MODE_PRIVATE).edit().putString(KEY_FOLDER, selectedFolder).apply();
        refreshFolderLabel();
        refreshRuntimeStatus();
    }

    private void refreshFolderLabel() {
        if (filesDetail == null) return;
        if (selectedFolder.isEmpty()) {
            filesDetail.setText("No game folder selected");
            return;
        }
        String label = selectedFolder;
        try {
            Uri uri = Uri.parse(selectedFolder);
            String documentId = DocumentsContract.getTreeDocumentId(uri);
            int split = documentId.lastIndexOf(':');
            label = split >= 0 ? documentId.substring(split + 1) : documentId;
            if (label.isEmpty()) label = "Selected game folder";
        } catch (Exception ignored) {
            label = "Selected game folder";
        }
        filesDetail.setText(label + "  ✓");
    }

    private void selectMode(int mode) {
        selectedMode = mode;
        getSharedPreferences(PREFS, MODE_PRIVATE).edit().putInt(KEY_MODE, mode).apply();
        refreshMode();
    }

    private void refreshMode() {
        if (storyButton == null || sandboxButton == null) return;
        storyButton.setBackground(round(selectedMode == 1 ? 0xff344a68 : 0xff172235, dp(12),
            selectedMode == 1 ? 0xff86a7d8 : 0xff3a4a62));
        sandboxButton.setBackground(round(selectedMode == 2 ? 0xff344a68 : 0xff172235, dp(12),
            selectedMode == 2 ? 0xff86a7d8 : 0xff3a4a62));
        modeDetail.setText(selectedMode == 1
            ? "The campaign with missions. Continues your latest save, or starts from the Prologue."
            : "Free roam anywhere on the map with every weapon. No missions.");
        boolean sandbox = selectedMode == 2;
        gtaMapButton.setVisibility(sandbox ? View.VISIBLE : View.GONE);
        testMapButton.setVisibility(sandbox ? View.VISIBLE : View.GONE);
        findViewById(android.R.id.content).invalidate();
    }

    private void selectMap(int map) {
        selectedMap = map;
        getSharedPreferences(PREFS, MODE_PRIVATE).edit().putInt(KEY_MAP, map).apply();
        refreshMap();
    }

    private void refreshMap() {
        if (gtaMapButton == null || testMapButton == null) return;
        gtaMapButton.setBackground(round(selectedMap == 0 ? 0xff344a68 : 0xff172235, dp(18),
            selectedMap == 0 ? 0xff86a7d8 : 0xff3a4a62));
        testMapButton.setBackground(round(selectedMap == 1 ? 0xff344a68 : 0xff172235, dp(18),
            selectedMap == 1 ? 0xff86a7d8 : 0xff3a4a62));
    }

    private void refreshRuntimeStatus() {
        if (runtimeStatus == null) return;
        String folder = selectedFolder.isEmpty() ? "• Game files: not selected" : "• Game folder: permission saved";
        String vulkan = nativeReady ? "• Vulkan: device and swapchain initialized" : "• Vulkan: waiting for Android surface";
        runtimeStatus.setText(folder + "\n" + vulkan
            + "\n• WASM engine: not connected"
            + "\n• Vulkan game renderer: not connected"
            + "\n• Touch controls / audio: not connected"
            + "\n\nGame files are not included. The selected folder is saved through Android's document provider.");
    }

    private Button modeButton(String label) {
        Button button = new Button(this);
        button.setText(label);
        button.setTextSize(13);
        button.setTextColor(Color.WHITE);
        button.setAllCaps(false);
        return button;
    }

    private Button smallButton(String label) {
        Button button = new Button(this);
        button.setText(label);
        button.setTextSize(11);
        button.setTextColor(0xffdce4f0);
        button.setAllCaps(false);
        button.setBackground(round(0xff172235, dp(18), 0xff3a4a62));
        return button;
    }

    private LinearLayout card() {
        LinearLayout panel = new LinearLayout(this);
        panel.setBackground(round(0xc91a2639, dp(16), 0x884b5d79));
        return panel;
    }

    private TextView text(String value, float size, int color, boolean bold) {
        TextView view = new TextView(this);
        view.setText(value);
        view.setTextSize(size);
        view.setTextColor(color);
        if (bold) view.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        return view;
    }

    private GradientDrawable round(int color, int radius, int stroke) {
        GradientDrawable shape = new GradientDrawable();
        shape.setColor(color);
        shape.setCornerRadius(radius);
        shape.setStroke(dp(1), stroke);
        return shape;
    }

    private int dp(float value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        nativeReady = nativeCreate(holder.getSurface());
        refreshRuntimeStatus();
        if (status != null) status.setText(nativeReady
            ? "Vulkan surface initialized · renderer integration pending"
            : "Vulkan initialization unavailable on this device");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (nativeReady && !nativeResize(width, height)) {
            nativeReady = false;
            refreshRuntimeStatus();
            if (status != null) status.setText("Vulkan surface resize failed");
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        nativeDestroy();
        nativeReady = false;
        refreshRuntimeStatus();
        if (status != null) status.setText("Vulkan surface released");
    }

    @Override
    protected void onDestroy() {
        nativeDestroy();
        nativeReady = false;
        super.onDestroy();
    }
}
