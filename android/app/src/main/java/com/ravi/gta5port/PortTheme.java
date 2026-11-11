package com.ravi.gta5port;

import android.content.Context;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.view.Gravity;

import java.io.InputStream;

/** Android translation of the original iOS theme and poster artwork. */
final class PortTheme {
    static final int NIGHT = 0xff080d1a;
    static final int NIGHT_BLUE = 0xff101b31;
    static final int ACCENT = 0xffff9e2e;
    static final int ACCENT_DEEP = 0xfff25c29;
    static final int TEXT = 0xffffffff;
    static final int SECONDARY = 0xadffffff;
    static final int TERTIARY = 0x73ffffff;
    static final int PANEL_FILL = 0x2affffff;
    static final int PANEL_STROKE = 0x42ffffff;
    static final int ROW_SELECTED = 0x24ffffff;
    static final int OK = 0xff4ddb78;
    static final int WARNING = 0xffffc743;
    static final int PROBLEM = 0xffff6b61;

    private PortTheme() {}

    static Drawable poster(Context context, String assetPath) {
        try (InputStream input = context.getAssets().open(assetPath)) {
            BitmapDrawable image = new BitmapDrawable(context.getResources(),
                BitmapFactory.decodeStream(input));
            image.setGravity(Gravity.FILL);
            GradientDrawable sideShade = new GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT,
                new int[]{0xe6080d1a, 0xa6080d1a, 0x52080d1a, 0x10080d1a});
            GradientDrawable bottomShade = new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM,
                new int[]{0x00080d1a, 0x12080d1a, 0x99080d1a});
            return new LayerDrawable(new android.graphics.drawable.Drawable[]{image, sideShade, bottomShade});
        } catch (Exception ignored) {
            return backdrop();
        }
    }

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
        shape.setColor(0x28ffffff);
        shape.setCornerRadius(radiusDp * density);
        shape.setStroke(Math.max(1, Math.round(density)), 0x55ffffff);
        return shape;
    }

    static GradientDrawable selected(float radiusDp, float density) {
        GradientDrawable shape = new GradientDrawable();
        shape.setColor(0x45ff9e2e);
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
