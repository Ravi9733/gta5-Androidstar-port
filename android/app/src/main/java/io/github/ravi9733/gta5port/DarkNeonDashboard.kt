package io.github.ravi9733.gta5port

import android.content.Context
import android.graphics.Typeface
import android.view.Gravity
import android.view.View
import android.view.SurfaceView
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView

/** Landscape-first Five Star Port dashboard. Los Santos/GTA V visual identity only. */
class DarkNeonDashboard(
    context: Context,
    private val gameSurface: SurfaceView,
    onImportMods: () -> Unit,
    onRefreshMods: () -> Unit,
    onCheckGameWasm: () -> Unit
) : LinearLayout(context) {
    val statusText = TextView(context)
    val modsText = TextView(context)
    val gameWasmText = TextView(context)

    private val bg = 0xFF090B12.toInt()
    private val panel = 0xFF151827.toInt()
    private val purple = 0xFF9B5CFF.toInt()
    private val neon = 0xFFB8FF35.toInt()
    private val muted = 0xFFB6BED0.toInt()

    init {
        orientation = HORIZONTAL
        setBackgroundColor(bg)
        setPadding(18, 14, 18, 14)

        val nav = column(panel).apply {
            gravity = Gravity.TOP
            addView(label("FIVE STAR", 18f, neon, true))
            addView(label("PORT  /  GTA V", 10f, muted, true))
            addView(space(22))
            listOf("⌂  HOME", "▶  STORY MODE", "◈  SANDBOX", "▤  PERFORMANCE",
                "✦  MODS & PATCHES", "♫  SELF RADIO", "⚙  SETTINGS").forEachIndexed { index, item ->
                addView(navItem(item, index == 0))
            }
        }
        addView(nav, LayoutParams(dp(172), LayoutParams.MATCH_PARENT))

        val center = column(bg).apply {
            setPadding(dp(18), dp(2), dp(18), dp(2))
            addView(label("GRAND THEFT AUTO V", 23f, 0xFFFFFFFF.toInt(), true))
            addView(label("LOS SANTOS  •  SAN ANDREAS", 11f, neon, true))
            addView(space(10))
            val hero = column(panel).apply {
                gravity = Gravity.CENTER
                setPadding(dp(16), dp(12), dp(16), dp(12))
                addView(label("WELCOME TO LOS SANTOS", 20f, 0xFFFFFFFF.toInt(), true))
                addView(label("STORY MODE  /  SANDBOX", 11f, purple, true))
                addView(space(10))
                gameSurface.visibility = View.GONE
                addView(gameSurface, LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f))
                val play = Button(context).apply {
                    text = "▶   PLAY GTA V"
                    setTextColor(0xFF10120B.toInt())
                    setBackgroundColor(neon)
                    textSize = 16f
                    typeface = Typeface.DEFAULT_BOLD
                    setOnClickListener {
                        gameSurface.visibility = View.VISIBLE
                        statusText.text = "Starting renderer… game engine connection is still in development."
                    }
                }
                addView(play, LayoutParams(LayoutParams.MATCH_PARENT, dp(52)))
            }
            addView(hero, LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f))
            addView(space(8))
            statusText.text = "VULKAN  •  Checking device capability…"
            statusText.textSize = 11f
            statusText.setTextColor(neon)
            addView(statusText)
        }
        addView(center, LayoutParams(0, LayoutParams.MATCH_PARENT, 1f))

        val info = column(panel).apply {
            setPadding(dp(14), dp(14), dp(14), dp(14))
            addView(label("GAME INFO", 14f, 0xFFFFFFFF.toInt(), true))
            addView(space(12))
            addView(label("BUILD", 10f, muted, true))
            addView(label("Web 8b0b5899ed", 12f, 0xFFFFFFFF.toInt()))
            addView(space(10))
            addView(label("PLATFORM", 10f, muted, true))
            addView(label("Android  /  Vulkan", 12f, 0xFFFFFFFF.toInt()))
            addView(space(10))
            addView(label("GAME ENGINE FILE", 10f, muted, true))
            gameWasmText.text = "game.wasm: not checked"
            gameWasmText.textSize = 10f
            gameWasmText.setTextColor(muted)
            addView(gameWasmText)
            addView(Button(context).apply { text = "CHECK GAME.WASM"; setOnClickListener { onCheckGameWasm() } })
            addView(space(8))
            addView(label("MODS & PATCHES", 13f, neon, true))
            modsText.text = "Imported mods: loading…"
            modsText.textSize = 11f
            modsText.setTextColor(muted)
            addView(modsText, LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f))
            addView(Button(context).apply { text = "IMPORT MOD"; setOnClickListener { onImportMods() } })
            addView(Button(context).apply { text = "REFRESH"; setOnClickListener { onRefreshMods() } })
        }
        addView(info, LayoutParams(dp(220), LayoutParams.MATCH_PARENT))
    }

    private fun column(color: Int) = LinearLayout(context).apply {
        orientation = VERTICAL
        setBackgroundColor(color)
    }

    private fun label(value: String, size: Float, color: Int, bold: Boolean = false) =
        TextView(context).apply {
            text = value
            textSize = size
            setTextColor(color)
            if (bold) typeface = Typeface.DEFAULT_BOLD
            setPadding(0, dp(5), 0, dp(5))
        }

    private fun navItem(value: String, selected: Boolean) = TextView(context).apply {
        text = value
        textSize = 11f
        typeface = if (selected) Typeface.DEFAULT_BOLD else Typeface.DEFAULT
        setTextColor(if (selected) neon else muted)
        setPadding(dp(8), dp(13), dp(4), dp(13))
        if (selected) setBackgroundColor(0xFF252039.toInt())
    }

    private fun space(height: Int) = View(context).apply {
        layoutParams = LayoutParams(1, dp(height))
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()
}
