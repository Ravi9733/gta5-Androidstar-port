package io.github.ravi9733.gta5port

import android.content.Context
import android.graphics.Color
import android.view.Gravity
import android.view.SurfaceView
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.TextView

/**
 * Pixel-faithful 16:9 Five Star Port reference-screen host.
 *
 * The reference artwork is loaded as drawable/fivestar_dashboard (1536x864).
 * Invisible hit targets sit above the artwork; they do not redraw or recolor it.
 */
class DarkNeonDashboard(
    context: Context,
    private val gameSurface: SurfaceView,
    onImportMods: () -> Unit,
    onRefreshMods: () -> Unit,
    onCheckGameWasm: () -> Unit
) : FrameLayout(context) {
    val statusText = TextView(context)
    val modsText = TextView(context)
    val gameWasmText = TextView(context)
    val performanceText = TextView(context)

    private val artwork = ImageView(context)

    init {
        setBackgroundColor(Color.BLACK)
        clipChildren = true
        clipToPadding = true

        // Keep the supplied 16:9 artwork untouched: no tint, crop, blur, or overlays.
        artwork.apply {
            scaleType = ImageView.ScaleType.FIT_XY
            adjustViewBounds = false
            val imageId = resources.getIdentifier("fivestar_dashboard", "drawable", context.packageName)
            if (imageId != 0) setImageResource(imageId)
            else contentDescription = "Missing drawable/fivestar_dashboard.webp reference artwork"
            importantForAccessibility = ImageView.IMPORTANT_FOR_ACCESSIBILITY_NO
        }
        addView(artwork, LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT))

        // Hidden state holders preserve the existing MainActivity integration without
        // placing debug text over the exact reference design.
        statusText.visibility = GONE
        modsText.visibility = GONE
        gameWasmText.visibility = GONE
        performanceText.visibility = GONE
        addView(statusText)
        addView(modsText)
        addView(gameWasmText)
        addView(performanceText)

        // Transparent, percentage-positioned hit targets. The screenshot remains the UI.
        addHit(0.00f, 0.33f, 0.14f, 0.08f) { statusText.text = "Home" }
        addHit(0.00f, 0.41f, 0.14f, 0.08f) { onImportMods() }
        addHit(0.00f, 0.49f, 0.14f, 0.08f) { onCheckGameWasm() }
        addHit(0.00f, 0.57f, 0.14f, 0.08f) { onRefreshMods() }
        addHit(0.00f, 0.65f, 0.14f, 0.08f) { statusText.text = "Credits" }

        // Main five cards in the supplied image.
        addHit(0.12f, 0.42f, 0.21f, 0.34f) {
            statusText.text = "Story Mode selected — game runtime is not linked yet."
            gameSurface.visibility = VISIBLE
        }
        addHit(0.32f, 0.44f, 0.16f, 0.32f) {
            statusText.text = "Sandbox selected — game runtime is not linked yet."
            gameSurface.visibility = VISIBLE
        }
        addHit(0.48f, 0.44f, 0.16f, 0.32f) {
            statusText.text = "Performance selected"
        }
        addHit(0.64f, 0.44f, 0.17f, 0.32f) { onImportMods() }
        addHit(0.81f, 0.44f, 0.17f, 0.32f) {
            statusText.text = "Self Radio selected"
        }

        // Top-right quick actions: About, Settings and Exit.
        addHit(0.72f, 0.02f, 0.09f, 0.07f) { statusText.text = "Five Star Port v1.6.2" }
        addHit(0.81f, 0.02f, 0.10f, 0.07f) { onCheckGameWasm() }
        addHit(0.91f, 0.02f, 0.08f, 0.07f) { statusText.text = "Exit requested" }

        // Keep the Vulkan surface available but hidden until Story/Sandbox is selected.
        gameSurface.visibility = GONE
        addView(gameSurface, LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT))
    }

    private fun addHit(
        left: Float,
        top: Float,
        width: Float,
        height: Float,
        action: () -> Unit
    ) {
        val hit = TextView(context).apply {
            text = ""
            alpha = 0.01f
            isClickable = true
            isFocusable = true
            setBackgroundColor(Color.WHITE)
            setOnClickListener { action() }
            contentDescription = "Dashboard action"
        }
        addView(hit, LayoutParams(1, 1, Gravity.TOP or Gravity.LEFT))
        hit.tag = floatArrayOf(left, top, width, height)
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        for (index in 0 until childCount) {
            val child = getChildAt(index)
            val rect = child.tag as? FloatArray ?: continue
            val lp = child.layoutParams as LayoutParams
            lp.width = (w * rect[2]).toInt().coerceAtLeast(1)
            lp.height = (h * rect[3]).toInt().coerceAtLeast(1)
            lp.leftMargin = (w * rect[0]).toInt()
            lp.topMargin = (h * rect[1]).toInt()
            child.layoutParams = lp
        }
    }
}
