package com.ravi.gta5port;

import android.app.Activity;
import android.os.Bundle;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public final class ControlsActivity extends Activity {
    private static final String PREFS = "gta5_android_port";
    private static final String KEY_LAYOUT = "controlsLayout";
    private static final String[] GROUPS = {"On foot", "In a vehicle", "Aircraft", "Controller buttons"};
    private FrameLayout stage;
    private Spinner groupSpinner;
    private JSONObject saved = new JSONObject();
    private final List<ControlItem> items = new ArrayList<>();
    private String activeGroup = "foot";

    private static final class ControlItem {
        final String name;
        final String label;
        final float x, y;
        final boolean essential;
        TextView view;
        ControlItem(String name, String label, float x, float y, boolean essential) {
            this.name = name; this.label = label; this.x = x; this.y = y; this.essential = essential;
        }
    }

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);

        try {
            String raw = getSharedPreferences(PREFS, MODE_PRIVATE).getString(KEY_LAYOUT, "{}");
            saved = new JSONObject(raw == null ? "{}" : raw);
        } catch (Exception ignored) { saved = new JSONObject(); }

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(12), dp(8), dp(12), dp(10));
        root.setBackground(new GradientDrawable(GradientDrawable.Orientation.TL_BR,
            new int[]{0xff101b2c, 0xff060a12}));

        LinearLayout toolbar = new LinearLayout(this);
        toolbar.setGravity(Gravity.CENTER_VERTICAL);
        Button back = smallButton("‹ Back");
        back.setOnClickListener(v -> finish());
        toolbar.addView(back);
        groupSpinner = new Spinner(this);
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, GROUPS);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        groupSpinner.setAdapter(adapter);
        LinearLayout.LayoutParams groupParams = new LinearLayout.LayoutParams(0, dp(44), 1);
        groupParams.leftMargin = dp(12);
        toolbar.addView(groupSpinner, groupParams);
        Button reset = smallButton("Reset");
        toolbar.addView(reset);
        Button done = smallButton("Done");
        toolbar.addView(done);
        root.addView(toolbar);

        stage = new FrameLayout(this);
        stage.setBackground(round(0x88202d40, dp(16), 0xff42536b));
        LinearLayout.LayoutParams stageParams = new LinearLayout.LayoutParams(-1, 0, 1);
        stageParams.topMargin = dp(10);
        stageParams.bottomMargin = dp(8);
        root.addView(stage, stageParams);

        TextView hint = new TextView(this);
        hint.setText("Drag controls to move them. Positions are saved for the Android touch-input port.");
        hint.setTextColor(0xffaab5c7);
        hint.setTextSize(11);
        root.addView(hint);

        groupSpinner.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
            @Override public void onItemSelected(android.widget.AdapterView<?> parent, View view, int position, long id) {
                activeGroup = new String[]{"foot", "vehicle", "aircraft", "pad"}[position];
                showGroup();
            }
            @Override public void onNothingSelected(android.widget.AdapterView<?> parent) {}
        });
        reset.setOnClickListener(v -> resetGroup());
        done.setOnClickListener(v -> { saveLayout(); finish(); });
        setContentView(root);
        showGroup();
    }

    private void showGroup() {
        if (stage == null) return;
        stage.post(() -> {
            stage.removeAllViews();
            items.clear();
            List<ControlItem> defaults = defaultsFor(activeGroup);
            for (ControlItem item : defaults) {
                TextView control = new TextView(this);
                control.setText(item.label);
                control.setTextColor(Color.WHITE);
                control.setTextSize(12);
                control.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
                control.setGravity(Gravity.CENTER);
                control.setBackground(round(0xb52a3c54, dp(54), 0xff8db7e8));
                control.setElevation(dp(5));
                item.view = control;
                float[] pos = readPosition(item);
                int size = activeGroup.equals("pad") ? dp(52) : dp(68);
                FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(size, size);
                item.view.setTag(pos);
                stage.addView(control, lp);
                control.post(() -> place(item, pos[0], pos[1]));
                attachDrag(item);
                items.add(item);
            }
        });
    }

    private List<ControlItem> defaultsFor(String group) {
        List<ControlItem> out = new ArrayList<>();
        if (group.equals("foot")) {
            out.add(new ControlItem("move", "MOVE", .12f, .72f, true));
            out.add(new ControlItem("look", "LOOK", .88f, .72f, true));
            out.add(new ControlItem("fire", "FIRE", .84f, .28f, true));
            out.add(new ControlItem("aim", "AIM", .72f, .30f, true));
            out.add(new ControlItem("enter", "ENTER", .95f, .53f, true));
            out.add(new ControlItem("sprint", "RUN", .25f, .52f, false));
            out.add(new ControlItem("weapon", "WEAPON", .50f, .82f, false));
            out.add(new ControlItem("pause", "PAUSE", .50f, .08f, true));
        } else if (group.equals("vehicle")) {
            out.add(new ControlItem("steer", "STEER", .12f, .72f, true));
            out.add(new ControlItem("look", "LOOK", .88f, .72f, true));
            out.add(new ControlItem("gas", "GAS", .94f, .49f, true));
            out.add(new ControlItem("brake", "BRAKE", .82f, .49f, true));
            out.add(new ControlItem("handbrake", "HANDBRAKE", .70f, .68f, false));
            out.add(new ControlItem("fire", "FIRE", .76f, .27f, true));
            out.add(new ControlItem("pause", "PAUSE", .50f, .08f, true));
        } else if (group.equals("aircraft")) {
            out.add(new ControlItem("flight_left", "FLIGHT", .13f, .70f, true));
            out.add(new ControlItem("flight_right", "LOOK", .86f, .70f, true));
            out.add(new ControlItem("throttle_up", "UP", .94f, .42f, false));
            out.add(new ControlItem("throttle_down", "DOWN", .82f, .42f, false));
            out.add(new ControlItem("air_fire", "FIRE", .75f, .25f, true));
            out.add(new ControlItem("pause", "PAUSE", .50f, .08f, true));
        } else {
            out.add(new ControlItem("pad_ls", "L-STICK", .12f, .70f, true));
            out.add(new ControlItem("pad_rs", "R-STICK", .65f, .76f, true));
            out.add(new ControlItem("pad_a", "A", .88f, .55f, false));
            out.add(new ControlItem("pad_b", "B", .95f, .68f, false));
            out.add(new ControlItem("pad_x", "X", .80f, .68f, false));
            out.add(new ControlItem("pad_y", "Y", .88f, .82f, false));
            out.add(new ControlItem("pad_lb", "LB", .14f, .14f, false));
            out.add(new ControlItem("pad_rb", "RB", .86f, .14f, false));
            out.add(new ControlItem("pad_menu", "MENU", .50f, .10f, true));
        }
        return out;
    }

    private float[] readPosition(ControlItem item) {
        try {
            JSONArray a = saved.optJSONArray(activeGroup + "." + item.name);
            if (a != null && a.length() >= 2) {
                float x = (float)a.getDouble(0), y = (float)a.getDouble(1);
                return new float[]{clamp(x, .04f, .96f), clamp(y, .06f, .94f)};
            }
        } catch (Exception ignored) {}
        return new float[]{item.x, item.y};
    }

    private void place(ControlItem item, float nx, float ny) {
        if (item.view == null || stage.getWidth() == 0 || stage.getHeight() == 0) return;
        float x = nx * stage.getWidth() - item.view.getWidth() / 2f;
        float y = ny * stage.getHeight() - item.view.getHeight() / 2f;
        item.view.setX(clamp(x, 0, stage.getWidth() - item.view.getWidth()));
        item.view.setY(clamp(y, 0, stage.getHeight() - item.view.getHeight()));
        item.view.setTag(new float[]{nx, ny});
    }

    private void attachDrag(ControlItem item) {
        item.view.setOnTouchListener(new View.OnTouchListener() {
            float downX, downY, originX, originY;
            @Override public boolean onTouch(View view, MotionEvent event) {
                switch (event.getActionMasked()) {
                    case MotionEvent.ACTION_DOWN:
                        downX = event.getRawX(); downY = event.getRawY();
                        originX = view.getX(); originY = view.getY();
                        view.setAlpha(.82f);
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        view.setX(clamp(originX + event.getRawX() - downX, 0, stage.getWidth() - view.getWidth()));
                        view.setY(clamp(originY + event.getRawY() - downY, 0, stage.getHeight() - view.getHeight()));
                        return true;
                    case MotionEvent.ACTION_UP:
                    case MotionEvent.ACTION_CANCEL:
                        view.setAlpha(1f);
                        float nx = (view.getX() + view.getWidth() / 2f) / Math.max(1, stage.getWidth());
                        float ny = (view.getY() + view.getHeight() / 2f) / Math.max(1, stage.getHeight());
                        view.setTag(new float[]{clamp(nx, .04f, .96f), clamp(ny, .06f, .94f)});
                        saveLayout();
                        return true;
                    default: return true;
                }
            }
        });
    }

    private void saveLayout() {
        try {
            for (ControlItem item : items) {
                Object tag = item.view.getTag();
                if (tag instanceof float[]) {
                    float[] p = (float[])tag;
                    JSONArray values = new JSONArray();
                    values.put(p[0]); values.put(p[1]); values.put(1.0); values.put(0);
                    saved.put(activeGroup + "." + item.name, values);
                }
            }
            getSharedPreferences(PREFS, MODE_PRIVATE).edit().putString(KEY_LAYOUT, saved.toString()).apply();
        } catch (Exception ignored) {}
    }

    private void resetGroup() {
        List<String> remove = new ArrayList<>();
        java.util.Iterator<String> keys = saved.keys();
        while (keys.hasNext()) {
            String key = keys.next();
            if (key.startsWith(activeGroup + ".")) remove.add(key);
        }
        for (String key : remove) saved.remove(key);
        getSharedPreferences(PREFS, MODE_PRIVATE).edit().putString(KEY_LAYOUT, saved.toString()).apply();
        showGroup();
    }

    private Button smallButton(String label) {
        Button button = new Button(this);
        button.setText(label);
        button.setTextColor(Color.WHITE);
        button.setTextSize(11);
        button.setAllCaps(false);
        button.setBackground(round(0xff1a2639, dp(12), 0xff4b5d79));
        return button;
    }

    private GradientDrawable round(int color, int radius, int stroke) {
        GradientDrawable shape = new GradientDrawable();
        shape.setColor(color);
        shape.setCornerRadius(radius);
        shape.setStroke(dp(1), stroke);
        return shape;
    }

    private float clamp(float value, float min, float max) { return Math.max(min, Math.min(max, value)); }
    private int dp(float value) { return Math.round(value * getResources().getDisplayMetrics().density); }
}
