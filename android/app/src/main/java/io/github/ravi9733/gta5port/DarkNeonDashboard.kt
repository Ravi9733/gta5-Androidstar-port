package io.github.ravi9733.gta5port

import android.app.AlertDialog
import android.content.Context
import android.graphics.Color
import android.view.Gravity
import android.view.SurfaceView
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.TextView
import android.widget.Toast

/**
 * Reference-artwork dashboard. The supplied screenshot is the visible UI;
 * transparent hit areas provide navigation without drawing over its artwork.
 */
class DarkNeonDashboard(
    context: Context,
    private val gameSurface: SurfaceView,
    private val onImportMods: () -> Unit,
    private val onRefreshMods: () -> Unit,
    private val onCheckGameWasm: () -> Unit
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

        artwork.apply {
            // Preserve the supplied image's aspect ratio; never crop, tint or blur it.
            scaleType = ImageView.ScaleType.FIT_CENTER
            adjustViewBounds = false
            val imageId = resources.getIdentifier("fivestar_dashboard", "drawable", context.packageName)
            if (imageId != 0) setImageResource(imageId)
            else {
                contentDescription = "Missing drawable/fivestar_dashboard.png"
                Toast.makeText(context, "Dashboard artwork missing", Toast.LENGTH_LONG).show()
            }
            importantForAccessibility = ImageView.IMPORTANT_FOR_ACCESSIBILITY_NO
        }
        addView(artwork, LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT))

        // Keep existing activity data bindings, but do not overlay debug text on the design.
        listOf(statusText, modsText, gameWasmText, performanceText).forEach {
            it.visibility = GONE
            addView(it)
        }

        // Left navigation: Home, Mods, Settings, Extras, Credits.
        addHit("Home", 0.015f, 0.305f, 0.13f, 0.075f) {
            gameSurface.visibility = GONE
            Toast.makeText(context, "Home", Toast.LENGTH_SHORT).show()
        }
        addHit("Mods", 0.015f, 0.385f, 0.13f, 0.075f) { showModsMenu() }
        addHit("Settings", 0.015f, 0.465f, 0.13f, 0.075f) { showSettings() }
        addHit("Extras", 0.015f, 0.545f, 0.13f, 0.075f) { showExtras() }
        addHit("Credits", 0.015f, 0.625f, 0.13f, 0.075f) { showCredits() }

        // Main dashboard cards. The renderer is a clear/present bootstrap only;
        // never cover the dashboard with a black SurfaceView pretending gameplay works.
        addHit("Story", 0.145f, 0.385f, 0.165f, 0.36f) { unavailable("Story Mode") }
        addHit("Sandbox", 0.315f, 0.385f, 0.165f, 0.36f) { unavailable("Sandbox") }
        addHit("Performance", 0.485f, 0.385f, 0.165f, 0.36f) { showPerformance() }
        addHit("Mods and Patches", 0.655f, 0.385f, 0.165f, 0.36f) { showModsMenu() }
        addHit("Self Radio", 0.825f, 0.385f, 0.165f, 0.36f) { unavailable("Self Radio") }

        // Top-right quick actions.
        addHit("About", 0.72f, 0.015f, 0.085f, 0.075f) { showCredits() }
        addHit("Game files", 0.81f, 0.015f, 0.095f, 0.075f) { onCheckGameWasm() }
        addHit("Exit", 0.91f, 0.015f, 0.08f, 0.075f) {
            AlertDialog.Builder(context).setTitle("Exit Five Star Port")
                .setMessage("Close the app?")
                .setNegativeButton("Cancel", null)
                .setPositiveButton("Exit") { _, _ -> (context as? android.app.Activity)?.finish() }
                .show()
        }

        gameSurface.visibility = GONE
        // Add it below hit targets and keep it hidden until a real game runtime exists.
        addView(gameSurface, 0, LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT))
    }

    private fun addHit(
        label: String,
        left: Float,
        top: Float,
        width: Float,
        height: Float,
        action: () -> Unit
    ) {
        val hit = TextView(context).apply {
            text = ""
            alpha = 0f
            isClickable = true
            isFocusable = true
            contentDescription = label
            setOnClickListener { action() }
        }
        addView(hit, LayoutParams(1, 1, Gravity.TOP or Gravity.LEFT))
        hit.tag = floatArrayOf(left, top, width, height)
    }

    private fun showModsMenu() {
        AlertDialog.Builder(context)
            .setTitle("Mods & Patches")
            .setItems(arrayOf("Import mod / ZIP / RPF", "Refresh imported mods", "Check game WASM")) { _, which ->
                when (which) {
                    0 -> onImportMods()
                    1 -> onRefreshMods()
                    2 -> onCheckGameWasm()
                }
            }
            .setNegativeButton("Close", null)
            .show()
    }

    private fun showSettings() {
        AlertDialog.Builder(context)
            .setTitle("Settings")
            .setItems(arrayOf("Graphics status", "Vulkan status", "Game files / WASM")) { _, which ->
                when (which) {
                    0 -> showPerformance()
                    1 -> Toast.makeText(context, "Vulkan renderer status is shown in diagnostics; GTA gameplay renderer is not linked.", Toast.LENGTH_LONG).show()
                    2 -> onCheckGameWasm()
                }
            }
            .setNegativeButton("Close", null)
            .show()
    }

    private fun showExtras() {
        AlertDialog.Builder(context)
            .setTitle("Extras")
            .setItems(arrayOf("Import mods", "Refresh mod list", "Check game files")) { _, which ->
                when (which) {
                    0 -> onImportMods()
                    1 -> onRefreshMods()
                    2 -> onCheckGameWasm()
                }
            }
            .setNegativeButton("Close", null)
            .show()
    }

    private fun showCredits() {
        AlertDialog.Builder(context)
            .setTitle("Five Star Port")
            .setMessage("Android port interface. The current build includes the dashboard and Vulkan surface bootstrap; GTA V gameplay/runtime integration is not yet connected.")
            .setPositiveButton("OK", null)
            .show()
    }

    private fun showPerformance() {
        AlertDialog.Builder(context)
            .setTitle("Performance")
            .setMessage("Live Android memory, UI FPS and Vulkan diagnostics are collected by the activity. These values do not represent GTA V gameplay FPS until the game runtime is integrated.")
            .setPositiveButton("OK", null)
            .show()
    }

    private fun unavailable(feature: String) {
        Toast.makeText(context, "$feature is not available yet: GTA V gameplay runtime is not connected.", Toast.LENGTH_LONG).show()
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
