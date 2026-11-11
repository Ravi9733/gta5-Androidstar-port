package com.ravi.gta5port;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
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
    private LinearLayout mapRow;
    private TextView mapLabel;
    private Button playButton;
    private boolean nativeReady;
    private int selectedMode = 2;
    private int selectedMap = 0;
    private String selectedFolder = "";
    private boolean folderLayoutFound;
    private int controllerButtons;
    private float controllerLeftX, controllerLeftY, controllerRightX, controllerRightY;
    private float controllerLeftTrigger, controllerRightTrigger;

    private static native boolean nativeCreate(Surface surface);
    private static native boolean nativeResize(int width, int height);
    private static native boolean nativeRenderFrame();
    private static native void nativeDestroy();
    private static native void nativeUpdateController(float leftX, float leftY, float rightX, float rightY,
        float leftTrigger, float rightTrigger, int buttons, int lookDX, int lookDY, double elapsedSeconds);

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
        root.setBackground(PortTheme.poster(this, "posters/start-bg-ipad.jpg"));

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
        icon.setBackground(PortTheme.glass(14, getResources().getDisplayMetrics().density));
        header.addView(icon, new LinearLayout.LayoutParams(dp(54), dp(54)));
        LinearLayout titles = new LinearLayout(this);
        titles.setOrientation(LinearLayout.VERTICAL);
        titles.setPadding(dp(13), 0, 0, 0);
        TextView title = text("GTA V Port", 27, Color.WHITE, true);
        TextView subtitle = text("Fan project · plays your own game files", 12, PortTheme.SECONDARY, false);
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
        filesCard.addView(text("GAME FILES", 11, PortTheme.SECONDARY, true));
        filesDetail = text("", 14, Color.WHITE, false);
        filesDetail.setPadding(0, dp(5), 0, 0);
        filesCard.addView(filesDetail);
        filesCard.addView(text("Choose the folder containing your own GTA V files", 11, PortTheme.TERTIARY, false));
        filesCard.setOnClickListener(v -> chooseGameFolder());
        LinearLayout.LayoutParams fileParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        fileParams.bottomMargin = dp(13);
        left.addView(filesCard, fileParams);
        refreshFolderLabel();

        left.addView(text("START MODE", 12, PortTheme.SECONDARY, true));
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
        modeDetail = text("", 12, PortTheme.SECONDARY, false);
        modeDetail.setPadding(dp(2), dp(7), dp(2), dp(9));
        left.addView(modeDetail);

        mapLabel = text("MAP", 11, PortTheme.SECONDARY, true);
        left.addView(mapLabel);
        mapRow = new LinearLayout(this);
        mapRow.setOrientation(LinearLayout.HORIZONTAL);
        gtaMapButton = smallButton("GTA V");
        testMapButton = smallButton("GTA VI test map");
        mapRow.addView(gtaMapButton, new LinearLayout.LayoutParams(0, dp(38), 1));
        LinearLayout.LayoutParams testParams = new LinearLayout.LayoutParams(0, dp(38), 1);
        testParams.leftMargin = dp(8);
        mapRow.addView(testMapButton, testParams);
        left.addView(mapRow);
        gtaMapButton.setOnClickListener(v -> selectMap(0));
        testMapButton.setOnClickListener(v -> selectMap(1));

        playButton = new Button(this);
        playButton.setText("PLAY  ·  ENGINE NOT CONNECTED");
        playButton.setTextSize(13);
        playButton.setTextColor(Color.WHITE);
        playButton.setAllCaps(false);
        playButton.setEnabled(false);
        playButton.setBackground(PortTheme.primary(14, getResources().getDisplayMetrics().density));
        playButton.setAlpha(0.62f);
        LinearLayout.LayoutParams playParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, dp(54));
        playParams.topMargin = dp(17);
        left.addView(playButton, playParams);

        Button exitButton = smallButton("EXIT  ·  QUIT PORT");
        exitButton.setTextColor(Color.WHITE);
        exitButton.setOnClickListener(v -> finish());
        LinearLayout.LayoutParams exitParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, dp(44));
        exitParams.topMargin = dp(9);
        left.addView(exitButton, exitParams);

        LinearLayout right = new LinearLayout(this);
        right.setOrientation(LinearLayout.VERTICAL);
        right.setPadding(dp(16), dp(8), 0, dp(8));
        content.addView(right, new LinearLayout.LayoutParams(0,
            LinearLayout.LayoutParams.MATCH_PARENT, 0.56f));

        LinearLayout actions = new LinearLayout(this);
        actions.setGravity(Gravity.RIGHT);
        Button settings = smallButton("Settings");
        Button controls = smallButton("Controls layout");
        settings.setOnClickListener(v -> startActivity(new Intent(MainActivity.this, SettingsActivity.class)));
        controls.setOnClickListener(v -> startActivity(new Intent(MainActivity.this, ControlsActivity.class)));
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
        logCard.addView(text("PORT STATUS", 11, PortTheme.SECONDARY, true));
        runtimeStatus = text("", 13, 0xffe3e9f2, false);
        runtimeStatus.setPadding(0, dp(12), 0, 0);
        runtimeStatus.setGravity(Gravity.TOP);
        logCard.addView(runtimeStatus);
        status = text("Android foundation · arm64 · Vulkan", 11, PortTheme.TERTIARY, false);
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
            folderLayoutFound = false;
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
        folderLayoutFound = scanForGameData(Uri.parse(selectedFolder));
        filesDetail.setText(label + (folderLayoutFound
            ? "  ✓  ·  x64 + common found"
            : "  ·  selected, but x64/common not found"));
    }

    private boolean scanForGameData(Uri treeUri) {
        try {
            String rootId = DocumentsContract.getTreeDocumentId(treeUri);
            java.util.HashMap<String, String> folders = listChildFolders(treeUri, rootId);
            if (folders.containsKey("x64") && folders.containsKey("common")) return true;
            String dataId = folders.get("data");
            if (dataId == null) return false;
            java.util.HashMap<String, String> dataFolders = listChildFolders(treeUri, dataId);
            return dataFolders.containsKey("x64") && dataFolders.containsKey("common");
        } catch (Exception ignored) {
            return false;
        }
    }

    private java.util.HashMap<String, String> listChildFolders(Uri treeUri, String documentId) {
        java.util.HashMap<String, String> folders = new java.util.HashMap<>();
        Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(treeUri, documentId);
        String[] columns = {
            DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_DISPLAY_NAME,
            DocumentsContract.Document.COLUMN_MIME_TYPE
        };
        try (Cursor cursor = getContentResolver().query(children, columns, null, null, null)) {
            if (cursor == null) return folders;
            int idColumn = cursor.getColumnIndex(DocumentsContract.Document.COLUMN_DOCUMENT_ID);
            int nameColumn = cursor.getColumnIndex(DocumentsContract.Document.COLUMN_DISPLAY_NAME);
            int typeColumn = cursor.getColumnIndex(DocumentsContract.Document.COLUMN_MIME_TYPE);
            while (cursor.moveToNext()) {
                if (idColumn < 0 || nameColumn < 0 || typeColumn < 0) continue;
                String type = cursor.getString(typeColumn);
                if (!DocumentsContract.Document.MIME_TYPE_DIR.equals(type)) continue;
                String name = cursor.getString(nameColumn);
                String id = cursor.getString(idColumn);
                if (name != null && id != null) folders.put(name.toLowerCase(java.util.Locale.ROOT), id);
            }
        } catch (Exception ignored) {}
        return folders;
    }

    private void selectMode(int mode) {
        selectedMode = mode;
        getSharedPreferences(PREFS, MODE_PRIVATE).edit().putInt(KEY_MODE, mode).apply();
        refreshMode();
    }

    private void refreshMode() {
        if (storyButton == null || sandboxButton == null) return;
        storyButton.setBackground(selectedMode == 1 ? PortTheme.selected(12, getResources().getDisplayMetrics().density) : PortTheme.glassButton(12, getResources().getDisplayMetrics().density));
        sandboxButton.setBackground(selectedMode == 2 ? PortTheme.selected(12, getResources().getDisplayMetrics().density) : PortTheme.glassButton(12, getResources().getDisplayMetrics().density));
        modeDetail.setText(selectedMode == 1
            ? "The campaign with missions. Continues your latest save, or starts from the Prologue."
            : "Free roam anywhere on the map with every weapon. No missions.");
        boolean sandbox = selectedMode == 2;
        mapLabel.setVisibility(sandbox ? View.VISIBLE : View.GONE);
        mapRow.setVisibility(sandbox ? View.VISIBLE : View.GONE);
        findViewById(android.R.id.content).invalidate();
    }

    private void selectMap(int map) {
        selectedMap = map;
        getSharedPreferences(PREFS, MODE_PRIVATE).edit().putInt(KEY_MAP, map).apply();
        refreshMap();
    }

    private void refreshMap() {
        if (gtaMapButton == null || testMapButton == null) return;
        gtaMapButton.setBackground(selectedMap == 0 ? PortTheme.selected(18, getResources().getDisplayMetrics().density) : PortTheme.glassButton(18, getResources().getDisplayMetrics().density));
        testMapButton.setBackground(selectedMap == 1 ? PortTheme.selected(18, getResources().getDisplayMetrics().density) : PortTheme.glassButton(18, getResources().getDisplayMetrics().density));
    }

    private void refreshRuntimeStatus() {
        if (runtimeStatus == null) return;
        String folder = selectedFolder.isEmpty() ? "• Game files: not selected" : (folderLayoutFound ? "• Game folder: x64 + common detected" : "• Game folder: saved; layout not verified");
        String vulkan = nativeReady ? "• Vulkan: device and swapchain initialized" : "• Vulkan: waiting for Android surface";
        runtimeStatus.setText(folder + "\n" + vulkan
            + "\n• WASM engine: not connected"
            + "\n• Vulkan game renderer: not connected"
            + "\n• Gamepad mapping: added; engine link pending"
            + "\n• Touch layout editor: added; live input pending"
            + "\n• Gyro / Self Radio: pending"
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
        button.setBackground(PortTheme.glassButton(18, getResources().getDisplayMetrics().density));
        return button;
    }

    private LinearLayout card() {
        LinearLayout panel = new LinearLayout(this);
        panel.setBackground(PortTheme.glass(16, getResources().getDisplayMetrics().density));
        return panel;
    }

    private TextView text(String value, float size, int color, boolean bold) {
        TextView view = new TextView(this);
        view.setText(value);
        view.setTextSize(size);
        view.setTextColor(color);
        view.setTypeface(Typeface.create("sans-serif-rounded", bold ? Typeface.BOLD : Typeface.NORMAL));
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
        boolean framePresented = nativeReady && nativeRenderFrame();
        refreshRuntimeStatus();
        if (status != null) status.setText(nativeReady
            ? (framePresented ? "Vulkan clear-pass presented · game renderer integration pending"
                : "Vulkan initialized · first frame presentation failed")
            : "Vulkan initialization unavailable on this device");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (nativeReady && (!nativeResize(width, height) || !nativeRenderFrame())) {
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

    private int controllerBit(int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A: return 0;
            case KeyEvent.KEYCODE_BUTTON_B: return 1;
            case KeyEvent.KEYCODE_BUTTON_X: return 2;
            case KeyEvent.KEYCODE_BUTTON_Y: return 3;
            case KeyEvent.KEYCODE_BUTTON_L1: return 4;
            case KeyEvent.KEYCODE_BUTTON_R1: return 5;
            case KeyEvent.KEYCODE_BUTTON_START: return 6;
            case KeyEvent.KEYCODE_DPAD_UP: return 7;
            case KeyEvent.KEYCODE_DPAD_DOWN: return 8;
            case KeyEvent.KEYCODE_DPAD_LEFT: return 9;
            case KeyEvent.KEYCODE_DPAD_RIGHT: return 10;
            default: return -1;
        }
    }

    private void sendController(float leftX, float leftY, float rightX, float rightY,
                                float leftTrigger, float rightTrigger) {
        nativeUpdateController(leftX, leftY, rightX, rightY, leftTrigger, rightTrigger,
            controllerButtons, 0, 0, 1.0 / 60.0);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        int bit = controllerBit(keyCode);
        if (bit >= 0) {
            controllerButtons |= (1 << bit);
            sendController(controllerLeftX, controllerLeftY, controllerRightX, controllerRightY,
                controllerLeftTrigger, controllerRightTrigger);
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        int bit = controllerBit(keyCode);
        if (bit >= 0) {
            controllerButtons &= ~(1 << bit);
            sendController(0, 0, 0, 0, 0, 0);
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        if ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK
            && event.getAction() == MotionEvent.ACTION_MOVE) {
            controllerLeftX = event.getAxisValue(MotionEvent.AXIS_X);
            controllerLeftY = -event.getAxisValue(MotionEvent.AXIS_Y);
            controllerRightX = event.getAxisValue(MotionEvent.AXIS_Z);
            controllerRightY = -event.getAxisValue(MotionEvent.AXIS_RZ);
            if (Math.abs(controllerRightX) < 0.001f)
                controllerRightX = event.getAxisValue(MotionEvent.AXIS_RX);
            if (Math.abs(controllerRightY) < 0.001f)
                controllerRightY = -event.getAxisValue(MotionEvent.AXIS_RY);
            controllerLeftTrigger = Math.max(0f, Math.min(1f, Math.max(
                event.getAxisValue(MotionEvent.AXIS_LTRIGGER), event.getAxisValue(MotionEvent.AXIS_BRAKE))));
            controllerRightTrigger = Math.max(0f, Math.min(1f, Math.max(
                event.getAxisValue(MotionEvent.AXIS_RTRIGGER), event.getAxisValue(MotionEvent.AXIS_GAS))));
            sendController(controllerLeftX, controllerLeftY, controllerRightX, controllerRightY,
                controllerLeftTrigger, controllerRightTrigger);
            return true;
        }
        return super.onGenericMotionEvent(event);
    }

    @Override
    protected void onDestroy() {
        nativeDestroy();
        nativeReady = false;
        super.onDestroy();
    }
}
