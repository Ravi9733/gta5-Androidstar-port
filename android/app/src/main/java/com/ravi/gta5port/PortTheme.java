package com.ravi.gta5port;

import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;

/** Android translation of the original iOS Theme palette and glass surfaces. */
final class PortTheme {
    static final int NIGHT = 0xff080d1a;
    static final int NIGHT_BLUE = 0xff101b31;
    static final int ACCENT = 0xffff9e2e;
    static final int ACCENT_DEEP = 0xfff25c29;
    static final int TEXT = 0xffffffff;
    static final int SECONDARY = 0xadffffff;
    static final int TERTIARY = 0x73ffffff;
    static final int PANEL_FILL = 0x1cffffff;
    static final int PANEL_STROKE = 0x1fffffff;
    static final int ROW_SELECTED = 0x24ffffff;
    static final int OK = 0xff4ddb78;
    static final int WARNING = 0xffffc743;
    static final int PROBLEM = 0xffff6b61;

    private PortTheme() {}

    static GradientDrawable backdrop() {
        return new GradientDrawable(GradientDrawable.Orientation.TL_BR,
            new int[]{0xff182b48, 0xff080d1a, 0xff090d18, 0xff24172a});
    }

    static GradientDrawable glass(float radiusDp, float density) {
        GradientDrawable shape = new GradientDrawable();
        shape.setColor(PANEL_FILL);
        shape.setCornerRadius(radiusDp * density);
        shape.setStroke(Math.max(1, Math.round(density)), PANEL_STROKE);
        return shape;
    }

    static GradientDrawable glassButton(float radiusDp, float density) {
        GradientDrawable shape = new GradientDrawable();
        shape.setColor(0x20ffffff);
        shape.setCornerRadius(radiusDp * density);
        shape.setStroke(Math.max(1, Math.round(density)), 0x30ffffff);
        return shape;
    }

    static GradientDrawable selected(float radiusDp, float density) {
        GradientDrawable shape = new GradientDrawable();
        shape.setColor(0x35ff9e2e);
        shape.setCornerRadius(radiusDp * density);
        shape.setStroke(Math.max(1, Math.round(density)), 0x99ffb45c);
        return shape;
    }

    static GradientDrawable primary(float radiusDp, float density) {
        GradientDrawable shape = new GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT,
            new int[]{ACCENT, ACCENT_DEEP});
        shape.setCornerRadius(radiusDp * density);
        return shape;
    }
}
