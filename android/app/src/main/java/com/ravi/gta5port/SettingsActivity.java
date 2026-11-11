package com.ravi.gta5port;

import android.app.Activity;
import android.os.Bundle;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.text.InputType;
import android.view.Gravity;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;

public final class SettingsActivity extends Activity {
    private static final String PREFS = "gta5_android_port";
    private static final int[] PRESET_VALUES = {-1, 0, 1, 2, 3, 4, 5};
    private static final String[] PRESETS = {
        "Ultra Performance", "Battery", "Performance", "Balanced", "Quality", "Ultra", "Max"
    };
    private int[] fpsValues;
    private String[] fpsLabels;

    private Spinner presetSpinner;
    private Spinner fpsSpinner;
    private Spinner touchSpinner;
    private SeekBar resolution;
    private TextView resolutionValue;
    private Switch sharperSwitch;
    private EditText memoryCap;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
        );

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(22), dp(18), dp(22), dp(18));
        root.setBackground(PortTheme.poster(this, "posters/settings-bg-iphone.jpg"));

        Button back = new Button(this);
        back.setText("‹   Back");
        back.setTextColor(Color.WHITE);
        back.setAllCaps(false);
        back.setGravity(Gravity.LEFT | Gravity.CENTER_VERTICAL);
        back.setBackgroundColor(Color.TRANSPARENT);
        back.setOnClickListener(v -> finish());
        root.addView(back, new LinearLayout.LayoutParams(-1, dp(44)));

        TextView heading = text("Settings", 26, Color.WHITE, true);
        heading.setPadding(0, dp(4), 0, dp(12));
        root.addView(heading);

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        root.addView(scroll, new LinearLayout.LayoutParams(-1, 0, 1));

        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(column);

        column.addView(section("GRAPHICS"));
        presetSpinner = spinner(PRESETS);
        addSetting(column, "Graphics preset", "Ultra Performance to Max", presetSpinner);

        LinearLayout resolutionRow = new LinearLayout(this);
        resolutionRow.setOrientation(LinearLayout.HORIZONTAL);
        resolutionValue = text("100%", 14, 0xffe5ebf5, true);
        resolutionRow.addView(text("Resolution scale", 14, Color.WHITE, true),
            new LinearLayout.LayoutParams(0, dp(36), 1));
        resolutionRow.addView(resolutionValue);
        column.addView(resolutionRow);
        resolution = new SeekBar(this);
        resolution.setMax(50);
        resolution.setProgress(50);
        column.addView(resolution, new LinearLayout.LayoutParams(-1, dp(42)));
        resolution.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                resolutionValue.setText((50 + progress) + "%");
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        boolean supports40 = supports40FPS();
        fpsValues = supports40 ? new int[]{30, 40, 60, 0} : new int[]{30, 60, 0};
        fpsLabels = supports40 ? new String[]{"30 FPS", "40 FPS", "60 FPS", "Unlocked"}
            : new String[]{"30 FPS", "60 FPS", "Unlocked"};
        fpsSpinner = spinner(fpsLabels);
        addSetting(column, "Frame rate limit", "Choose the target frame rate", fpsSpinner);
        sharperSwitch = new Switch(this);
        sharperSwitch.setText("Sharper upscaling");
        sharperSwitch.setTextColor(Color.WHITE);
        column.addView(sharperSwitch, new LinearLayout.LayoutParams(-1, dp(48)));

        column.addView(section("DEVICE"));
        memoryCap = new EditText(this);
        memoryCap.setSingleLine(true);
        memoryCap.setInputType(InputType.TYPE_CLASS_NUMBER);
        memoryCap.setHint("0 = automatic memory limit (MB)");
        memoryCap.setHintTextColor(PortTheme.TERTIARY);
        memoryCap.setTextColor(Color.WHITE);
        memoryCap.setBackground(panel());
        memoryCap.setPadding(dp(12), 0, dp(12), 0);
        addSetting(column, "Memory cap (MB)", "0 keeps the automatic device limit", memoryCap);

        column.addView(section("CONTROLS"));
        touchSpinner = spinner(new String[]{"Situational touch controls", "Controller-style buttons"});
        addSetting(column, "Touch control scheme", "The iOS port supports editable touch layouts", touchSpinner);

        TextView note = text("These preferences are saved locally. They will take effect when the Android engine and input layers are connected.", 12, PortTheme.SECONDARY, false);
        note.setPadding(0, dp(16), 0, dp(12));
        column.addView(note);

        Button save = new Button(this);
        save.setText("Save settings");
        save.setAllCaps(false);
        save.setTextColor(Color.WHITE);
        save.setBackground(PortTheme.primary(24, getResources().getDisplayMetrics().density));
        root.addView(save, new LinearLayout.LayoutParams(-1, dp(50)));
        save.setOnClickListener(v -> saveSettings());

        setContentView(root);
        loadSettings();
    }

    private void loadSettings() {
        android.content.SharedPreferences prefs = getSharedPreferences(PREFS, MODE_PRIVATE);
        presetSpinner.setSelection(indexOf(PRESET_VALUES, prefs.getInt("graphicsPreset", 1)));
        fpsSpinner.setSelection(indexOf(fpsValues, prefs.getInt("frameRateCap", 60)));
        resolution.setProgress(Math.max(0, Math.min(50,
            (int)Math.round((prefs.getFloat("resolutionScale", 1.0f) - 0.5f) * 100))));
        resolutionValue.setText((50 + resolution.getProgress()) + "%");
        sharperSwitch.setChecked(prefs.getBoolean("sharperUpscaling", false));
        memoryCap.setText(String.valueOf(prefs.getInt("memoryCapMB", 0)));
        touchSpinner.setSelection("controller".equals(prefs.getString("touchScheme", "situational")) ? 1 : 0);
    }

    private void saveSettings() {
        int preset = PRESET_VALUES[presetSpinner.getSelectedItemPosition()];
        int fps = fpsValues[fpsSpinner.getSelectedItemPosition()];
        int cap = 0;
        try { cap = Math.max(0, Integer.parseInt(memoryCap.getText().toString().trim())); }
        catch (NumberFormatException ignored) {}
        String touch = touchSpinner.getSelectedItemPosition() == 1 ? "controller" : "situational";
        getSharedPreferences(PREFS, MODE_PRIVATE).edit()
            .putInt("graphicsPreset", preset)
            .putInt("frameRateCap", fps)
            .putFloat("resolutionScale", (50 + resolution.getProgress()) / 100.0f)
            .putBoolean("sharperUpscaling", sharperSwitch.isChecked())
            .putInt("memoryCapMB", cap)
            .putString("touchScheme", touch)
            .apply();
        android.widget.Toast.makeText(this,
            "Saved. Engine integration is still in progress.", android.widget.Toast.LENGTH_LONG).show();
    }

    private boolean supports40FPS() {
        float maximum = 0;
        for (android.view.Display.Mode mode : getWindowManager().getDefaultDisplay().getSupportedModes())
            maximum = Math.max(maximum, mode.getRefreshRate());
        return maximum >= 120.0f;
    }

    private int indexOf(int[] values, int target) {
        for (int i = 0; i < values.length; i++) if (values[i] == target) return i;
        return 0;
    }

    private void addSetting(LinearLayout parent, String title, String detail, View control) {
        LinearLayout group = new LinearLayout(this);
        group.setOrientation(LinearLayout.VERTICAL);
        group.setPadding(dp(12), dp(9), dp(12), dp(8));
        group.setBackground(panel());
        TextView heading = text(title, 14, Color.WHITE, true);
        group.addView(heading);
        TextView description = text(detail, 11, PortTheme.SECONDARY, false);
        description.setPadding(0, dp(3), 0, dp(6));
        group.addView(description);
        group.addView(control, new LinearLayout.LayoutParams(-1, dp(44)));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(-1, -2);
        params.bottomMargin = dp(10);
        parent.addView(group, params);
    }

    private TextView section(String title) {
        TextView label = text(title, 11, PortTheme.SECONDARY, true);
        label.setPadding(dp(2), dp(15), 0, dp(8));
        return label;
    }

    private Spinner spinner(String[] options) {
        Spinner spinner = new Spinner(this);
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this,
            android.R.layout.simple_spinner_item, options);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);
        return spinner;
    }

    private TextView text(String value, float size, int color, boolean bold) {
        TextView view = new TextView(this);
        view.setText(value);
        view.setTextSize(size);
        view.setTextColor(color);
        if (bold) view.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        return view;
    }

    private GradientDrawable panel() {
        return PortTheme.glass(14, getResources().getDisplayMetrics().density);
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
}
