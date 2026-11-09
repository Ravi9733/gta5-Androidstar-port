package io.github.ravi9733.gta5port

import android.app.AlertDialog
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.SurfaceView
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast

/**
 * Premium native Five Star Port dashboard.
 * All controls are Android views; the Los Santos hero is drawn locally on Canvas.
 */
class PremiumDashboard(
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

    private val page = LinearLayout(context)
    private val navItems = linkedMapOf<String, TextView>()
    private val purple = 0xFFB66CFF.toInt()
    private val lime = 0xFFC5FF63.toInt()
    private val muted = 0xFFA9A5B8.toInt()
    private val ink = 0xFF090B13.toInt()

    init {
        setBackgroundColor(ink)
        clipChildren = true
        gameSurface.visibility = GONE

        val shell = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            setPadding(dp(12), dp(10), dp(12), dp(10))
            background = GradientDrawable(
                GradientDrawable.Orientation.TL_BR,
                intArrayOf(0xFF090B13.toInt(), 0xFF151024.toInt(), 0xFF0B111B.toInt())
            )
        }
        addView(shell, LayoutParams(MATCH_PARENT, MATCH_PARENT))

        val rail = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
            setPadding(dp(9), dp(13), dp(9), dp(10))
            background = surface(intArrayOf(0xF01C1A2A.toInt(), 0xF0101420.toInt()), 22)
        }
        shell.addView(rail, LinearLayout.LayoutParams(dp(154), MATCH_PARENT))

        rail.addView(label("FIVE★", 23f, Color.WHITE, true).apply {
            gravity = Gravity.CENTER
            letterSpacing = .06f
        }, LinearLayout.LayoutParams(MATCH_PARENT, dp(42)))
        rail.addView(label("LOS SANTOS PORT", 9f, lime, true).apply {
            gravity = Gravity.CENTER
            letterSpacing = .12f
        }, LinearLayout.LayoutParams(MATCH_PARENT, dp(24)))
        divider(rail, dp(10))

        val nav = listOf(
            "HOME" to "⌂",
            "STORY MODE" to "◈",
            "SANDBOX" to "◇",
            "PERFORMANCE" to "⌁",
            "MODS" to "✦",
            "SETTINGS" to "⚙"
        )
        nav.forEach { (name, glyph) ->
            val item = TextView(context).apply {
                text = "$glyph   $name"
                textSize = 11f
                letterSpacing = .025f
                typeface = Typeface.create("sans-serif-medium", Typeface.NORMAL)
                gravity = Gravity.CENTER_VERTICAL
                setPadding(dp(11), 0, dp(5), 0)
                setTextColor(muted)
                background = surface(intArrayOf(0x001C1A2A, 0x00101020), 13)
                isClickable = true
                isFocusable = true
                setOnClickListener { openPage(name) }
            }
            navItems[name] = item
            rail.addView(item, LinearLayout.LayoutParams(MATCH_PARENT, dp(43)).apply {
                topMargin = dp(4)
            })
        }

        val spacer = View(context)
        rail.addView(spacer, LinearLayout.LayoutParams(1, 0, 1f))
        divider(rail, dp(8))
        rail.addView(label("ANDROID EDITION", 9f, muted, true).apply {
            gravity = Gravity.CENTER
            letterSpacing = .08f
        }, LinearLayout.LayoutParams(MATCH_PARENT, dp(23)))
        rail.addView(label("VULKAN • TOUCH", 8f, purple, true).apply {
            gravity = Gravity.CENTER
        }, LinearLayout.LayoutParams(MATCH_PARENT, dp(18)))

        val main = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(13), dp(1), 0, 0)
        }
        shell.addView(main, LinearLayout.LayoutParams(0, MATCH_PARENT, 1f).apply {
            leftMargin = dp(12)
        })

        val header = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }
        main.addView(header, LinearLayout.LayoutParams(MATCH_PARENT, dp(49)))
        val headerText = LinearLayout(context).apply { orientation = LinearLayout.VERTICAL }
        headerText.addView(label("GRAND THEFT AUTO V", 17f, Color.WHITE, true))
        headerText.addView(label("FIVE STAR PORT  /  ANDROID EXPERIENCE", 8f, lime, true).apply {
            letterSpacing = .08f
        })
        header.addView(headerText, LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f))
        header.addView(pill("YOUR GAME • YOUR FILES", purple), LinearLayout.LayoutParams(WRAP_CONTENT, dp(29)))
        divider(main, dp(6))

        val scroll = ScrollView(context).apply {
            isFillViewport = true
            clipToPadding = false
            setPadding(0, dp(8), 0, dp(8))
            overScrollMode = View.OVER_SCROLL_NEVER
        }
        main.addView(scroll, LinearLayout.LayoutParams(MATCH_PARENT, 0, 1f))
        page.orientation = LinearLayout.VERTICAL
        scroll.addView(page, ViewGroup.LayoutParams(MATCH_PARENT, WRAP_CONTENT))

        val footer = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(12), 0, dp(12), 0)
            background = surface(intArrayOf(0xEE1B1A29.toInt(), 0xEE111722.toInt()), 12)
        }
        main.addView(footer, LinearLayout.LayoutParams(MATCH_PARENT, dp(39)))
        footer.addView(label("●", 10f, lime, true), LinearLayout.LayoutParams(WRAP_CONTENT, WRAP_CONTENT))
        footer.addView(label("  SYSTEM MONITOR", 9f, Color.WHITE, true), LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f))
        footer.addView(label("LIVE DEVICE DATA", 8f, muted, true))

        showPage("HOME")
        addView(gameSurface, 0, LayoutParams(1, 1))
    }

    private fun showPage(name: String) {
        page.removeAllViews()
        when (name) {
            "HOME" -> buildHome()
            "STORY MODE", "SANDBOX" -> showLaunchInfo(name)
            "PERFORMANCE" -> buildPerformance()
            "MODS" -> buildMods()
            "SETTINGS" -> buildSettings()
        }
    }

    private fun openPage(name: String) {
        navItems.forEach { (key, item) ->
            val selected = key == name
            item.setTextColor(if (selected) Color.WHITE else muted)
            item.background = if (selected)
                surface(intArrayOf(0xFF59357D.toInt(), 0xFF302044.toInt()), 13, 0x887E5AB0.toInt())
            else surface(intArrayOf(0x001C1A2A, 0x00101020), 13, Color.TRANSPARENT)
        }
        showPage(name)
    }

    private fun buildHome() {
        page.addView(label("WELCOME BACK", 9f, lime, true).apply { letterSpacing = .18f })
        page.addView(label("THE CITY IS YOURS.", 24f, Color.WHITE, true), margin(0, 2, 0, 9))

        val hero = FrameLayout(context).apply {
            background = surface(intArrayOf(0xFF342044.toInt(), 0xFF151A2A.toInt()), 20)
            clipToOutline = true
            elevation = dp(3).toFloat()
        }
        hero.addView(LosSantosArt(context), LayoutParams(MATCH_PARENT, MATCH_PARENT))
        val shade = View(context).apply {
            background = GradientDrawable(
                GradientDrawable.Orientation.LEFT_RIGHT,
                intArrayOf(0xEE0A0B16.toInt(), 0x88100C20.toInt(), 0x00100C20)
            )
        }
        hero.addView(shade, LayoutParams(MATCH_PARENT, MATCH_PARENT))
        val copy = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(21), dp(12), dp(18), dp(12))
        }
        copy.addView(pill("LOS SANTOS • SAN ANDREAS", lime), LinearLayout.LayoutParams(WRAP_CONTENT, dp(25)))
        copy.addView(label("GRAND THEFT", 23f, Color.WHITE, true), margin(0, 9, 0, 0))
        copy.addView(label("AUTO V", 31f, Color.WHITE, true))
        copy.addView(label("YOUR NEXT ADVENTURE STARTS HERE", 9f, 0xFFE0D9EA.toInt(), true), margin(0, 4, 0, 0))
        hero.addView(copy, LayoutParams(MATCH_PARENT, MATCH_PARENT))
        page.addView(hero, LinearLayout.LayoutParams(MATCH_PARENT, dp(205)).apply {
            bottomMargin = dp(11)
        })

        val actionRow = LinearLayout(context).apply { orientation = LinearLayout.HORIZONTAL }
        actionRow.addView(modeCard("STORY MODE", "Continue your campaign", "PLAY STORY", lime) {
            showPage("STORY MODE")
        }, LinearLayout.LayoutParams(0, dp(104), 1f))
        actionRow.addView(modeCard("SANDBOX", "Explore Los Santos freely", "FREE ROAM", purple) {
            showPage("SANDBOX")
        }, LinearLayout.LayoutParams(0, dp(104), 1f).apply { leftMargin = dp(9) })
        page.addView(actionRow, margin(0, 0, 0, 10))

        val lower = LinearLayout(context).apply { orientation = LinearLayout.HORIZONTAL }
        lower.addView(infoCard("GAME FILES", "Select and validate game.wasm", "CHECK FILES") {
            onCheckGameWasm()
        }, LinearLayout.LayoutParams(0, dp(75), 1f))
        lower.addView(infoCard("MOD LIBRARY", "ZIP • RPF • add-ons", "OPEN MODS") {
            openPage("MODS")
        }, LinearLayout.LayoutParams(0, dp(75), 1f).apply { leftMargin = dp(9) })
        lower.addView(infoCard("PERFORMANCE", "Live device diagnostics", "VIEW STATS") {
            openPage("PERFORMANCE")
        }, LinearLayout.LayoutParams(0, dp(75), 1f).apply { leftMargin = dp(9) })
        page.addView(lower)
        page.addView(statusCard(), margin(0, 10, 0, 0))
    }

    private fun modeCard(title: String, detail: String, action: String, color: Int, click: () -> Unit): View {
        val card = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(14), dp(10), dp(12), dp(10))
            background = surface(intArrayOf(0xF0222030.toInt(), 0xF0141822.toInt()), 15, 0x555C5270)
            isClickable = true
            isFocusable = true
            setOnClickListener { click() }
        }
        card.addView(label(title, 12f, Color.WHITE, true))
        card.addView(label(detail, 9f, muted), margin(0, 3, 0, 5))
        card.addView(label("↗  $action", 9f, color, true))
        return card
    }

    private fun infoCard(title: String, detail: String, action: String, click: () -> Unit): View {
        val card = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(11), dp(9), dp(8), dp(8))
            background = surface(intArrayOf(0xE9232030.toInt(), 0xE9151924.toInt()), 13)
            isClickable = true
            setOnClickListener { click() }
        }
        card.addView(label(title, 9f, purple, true))
        card.addView(label(detail, 9f, Color.WHITE), margin(0, 3, 0, 0))
        card.addView(label(action, 8f, lime, true), margin(0, 4, 0, 0))
        return card
    }

    private fun statusCard(): View {
        val box = panel()
        box.addView(label("SESSION STATUS", 9f, purple, true))
        statusText.text = "Waiting for Vulkan surface status…"
        statusText.textSize = 10f
        statusText.setTextColor(Color.WHITE)
        box.addView(statusText, margin(0, 5, 0, 0))
        return box
    }

    private fun showLaunchInfo(mode: String) {
        page.addView(label(mode, 23f, Color.WHITE, true))
        page.addView(label("Launch readiness", 10f, lime, true), margin(0, 4, 0, 12))
        page.addView(infoPanel("GAME DATA", gameWasmText, "Select game.wasm") { onCheckGameWasm() })
        page.addView(infoPanel("RENDERER", statusText, "Vulkan status is reported by the native renderer"))
        page.addView(panel().apply {
            addView(label("GAMEPLAY CONNECTION", 10f, purple, true))
            addView(label(
                "This Android branch currently contains the native dashboard and Vulkan surface bootstrap. The GTA V gameplay runtime is not connected yet, so $mode cannot be launched from this build.",
                11f, Color.WHITE
            ), margin(0, 7, 0, 0))
        }, margin(0, 10, 0, 0))
    }

    private fun buildPerformance() {
        page.addView(label("PERFORMANCE", 23f, Color.WHITE, true))
        page.addView(label("Live Android device and renderer diagnostics", 10f, muted), margin(0, 4, 0, 12))
        performanceText.textSize = 11f
        performanceText.setTextColor(Color.WHITE)
        page.addView(infoPanel("DEVICE TELEMETRY", performanceText, "Metrics are refreshed by the activity"))
        page.addView(infoPanel("VULKAN RENDERER", statusText, "Status comes from native Vulkan initialization"), margin(0, 9, 0, 0))
        page.addView(panel().apply {
            addView(label("FPS NOTE", 10f, lime, true))
            addView(label("The displayed UI frame rate is not GTA gameplay FPS. In-game performance is unavailable until the game renderer is integrated.", 10f, muted), margin(0, 5, 0, 0))
        }, margin(0, 9, 0, 0))
    }

    private fun buildMods() {
        page.addView(label("MODS & PATCHES", 23f, Color.WHITE, true))
        page.addView(label("Manage files imported into this app", 10f, muted), margin(0, 4, 0, 12))
        page.addView(action("IMPORT MOD / ZIP / RPF", "Choose a mod file using Android's document picker", "IMPORT") { onImportMods() })
        page.addView(action("REFRESH LIBRARY", "Scan imported files again", "REFRESH") { onRefreshMods() }, margin(0, 8, 0, 0))
        page.addView(action("CHECK GAME.WASM", "Validate the selected game runtime file", "CHECK") { onCheckGameWasm() }, margin(0, 8, 0, 0))
        page.addView(infoPanel("IMPORTED FILES", modsText, "Imported files are listed here"), margin(0, 10, 0, 0))
        page.addView(infoPanel("IMPORT STATUS", statusText, "Latest import result"), margin(0, 8, 0, 0))
    }

    private fun buildSettings() {
        page.addView(label("SETTINGS", 23f, Color.WHITE, true))
        page.addView(label("App and runtime information", 10f, muted), margin(0, 4, 0, 12))
        page.addView(action("GAME DATA", "Select and validate your own game.wasm", "SELECT") { onCheckGameWasm() })
        page.addView(action("MOD STORAGE", "Open the in-app mod import screen", "OPEN") { openPage("MODS") }, margin(0, 8, 0, 0))
        page.addView(infoPanel("RENDERER STATUS", statusText, "Live native status"), margin(0, 10, 0, 0))
        page.addView(panel().apply {
            addView(label("VISUAL", 10f, purple, true))
            addView(label("Dark glass panels • reduced-motion transitions • landscape-first layout", 10f, Color.WHITE), margin(0, 5, 0, 0))
        }, margin(0, 8, 0, 0))
    }

    private fun action(title: String, detail: String, buttonText: String, click: () -> Unit): View {
        val row = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(13), dp(10), dp(11), dp(10))
            background = surface(intArrayOf(0xF0222030.toInt(), 0xF0141822.toInt()), 14)
            isClickable = true
            setOnClickListener { click() }
        }
        val text = LinearLayout(context).apply { orientation = LinearLayout.VERTICAL }
        text.addView(label(title, 11f, Color.WHITE, true))
        text.addView(label(detail, 9f, muted), margin(0, 3, 0, 0))
        row.addView(text, LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f))
        row.addView(pill(buttonText, lime))
        return row
    }

    private fun infoPanel(title: String, value: TextView, hint: String): View {
        val box = panel()
        box.addView(label(title, 9f, purple, true))
        value.textSize = 10f
        value.setTextColor(Color.WHITE)
        box.addView(value, margin(0, 5, 0, 0))
        box.addView(label(hint, 8f, muted), margin(0, 4, 0, 0))
        return box
    }

    private fun panel() = LinearLayout(context).apply {
        orientation = LinearLayout.VERTICAL
        setPadding(dp(13), dp(11), dp(13), dp(11))
        background = surface(intArrayOf(0xE9232030.toInt(), 0xE9151924.toInt()), 14, 0x555C5270)
    }

    private fun pill(text: String, color: Int): TextView = TextView(context).apply {
        this.text = text
        textSize = 8f
        letterSpacing = .06f
        typeface = Typeface.create("sans-serif-medium", Typeface.BOLD)
        gravity = Gravity.CENTER
        setTextColor(color)
        setPadding(dp(9), dp(4), dp(9), dp(4))
        background = surface(intArrayOf(0x332C2940, 0x22201D30), 30, 0x667B6C91)
    }

    private fun label(text: String, size: Float, color: Int, bold: Boolean = false) =
        TextView(context).apply {
            this.text = text
            textSize = size
            setTextColor(color)
            if (bold) typeface = Typeface.create("sans-serif", Typeface.BOLD)
        }

    private fun divider(parent: LinearLayout, top: Int) {
        parent.addView(View(context).apply { setBackgroundColor(0x445C536F) },
            LinearLayout.LayoutParams(MATCH_PARENT, dp(1)).apply {
                topMargin = top
                bottomMargin = top
            })
    }

    private fun margin(l: Int, t: Int, r: Int, b: Int) =
        LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply {
            setMargins(dp(l), dp(t), dp(r), dp(b))
        }

    private fun surface(colors: IntArray, radius: Int, stroke: Int = 0x665C536F) =
        GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM, colors).apply {
            cornerRadius = dp(radius).toFloat()
            setStroke(dp(1), stroke)
        }

    private fun dp(v: Int) = (v * resources.displayMetrics.density).toInt()

    private fun showLaunchInfo(mode: String) {
        AlertDialog.Builder(context)
            .setTitle("$mode unavailable")
            .setMessage("The native dashboard and Vulkan surface bootstrap are present, but GTA V gameplay integration is not connected in this Android build yet.")
            .setPositiveButton("OK", null)
            .show()
    }
}

/** Lightweight locally drawn Los Santos key art; no remote image or screenshot asset. */
private class LosSantosArt(context: Context) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat()
        val h = height.toFloat()
        if (w <= 0f || h <= 0f) return
        paint.shader = android.graphics.LinearGradient(0f, 0f, w, h,
            intArrayOf(0xFF12172B.toInt(), 0xFF51335D.toInt(), 0xFFD16C75.toInt(), 0xFFFFB36F.toInt()),
            null, android.graphics.Shader.TileMode.CLAMP)
        canvas.drawRect(0f, 0f, w, h, paint)
        paint.shader = null
        paint.color = 0x55FFC27B
        canvas.drawCircle(w * .82f, h * .27f, h * .20f, paint)
        paint.color = 0xFFFFD49A.toInt()
        canvas.drawCircle(w * .82f, h * .27f, h * .105f, paint)

        val skyline = floatArrayOf(
            .00f,.58f,.08f,.96f, .07f,.44f,.15f,.96f, .14f,.57f,.22f,.96f,
            .21f,.39f,.29f,.96f, .28f,.52f,.36f,.96f, .35f,.34f,.43f,.96f,
            .42f,.50f,.50f,.96f, .49f,.41f,.57f,.96f, .56f,.55f,.64f,.96f,
            .63f,.35f,.71f,.96f, .70f,.51f,.78f,.96f, .77f,.43f,.85f,.96f,
            .84f,.58f,.92f,.96f, .91f,.47f,1f,.96f
        )
        paint.color = 0xFF161827.toInt()
        var i = 0
        while (i < skyline.size) {
            val l = skyline[i] * w
            val t = skyline[i + 1] * h
            val r = skyline[i + 2] * w
            val b = skyline[i + 3] * h
            canvas.drawRect(l, t, r, b, paint)
            paint.color = 0xFFFFC77D.toInt()
            val cols = ((r - l) / (w * .022f)).toInt().coerceAtLeast(1)
            val rows = ((b - t) / (h * .08f)).toInt().coerceAtLeast(1)
            for (cx in 0 until cols) for (cy in 0 until rows) {
                if ((cx + cy * 2 + i) % 3 != 0) {
                    canvas.drawRect(l + w * .007f + cx * w * .022f, t + h * .025f + cy * h * .08f,
                        l + w * .013f + cx * w * .022f, t + h * .045f + cy * h * .08f, paint)
                }
            }
            paint.color = 0xFF161827.toInt()
            i += 4
        }
        val hills = Path().apply {
            moveTo(0f, h * .78f)
            lineTo(w * .18f, h * .61f)
            lineTo(w * .31f, h * .72f)
            lineTo(w * .48f, h * .57f)
            lineTo(w * .67f, h * .74f)
            lineTo(w, h * .60f)
            lineTo(w, h)
            lineTo(0f, h)
            close()
        }
        paint.color = 0xFF242039.toInt()
        canvas.drawPath(hills, paint)

        val road = Path().apply {
            moveTo(w * .42f, h)
            lineTo(w * .49f, h * .82f)
            lineTo(w * .72f, h * .82f)
            lineTo(w * .98f, h)
            close()
        }
        paint.shader = android.graphics.LinearGradient(0f, h * .82f, 0f, h,
            intArrayOf(0xFF3A2C49.toInt(), 0xFF0B101A.toInt()), null,
            android.graphics.Shader.TileMode.CLAMP)
        canvas.drawPath(road, paint)
        paint.shader = null
        paint.color = 0xCCB7FF63.toInt()
        paint.strokeWidth = dp(2).toFloat()
        canvas.drawLine(w * .49f, h * .84f, w * .42f, h, paint)
        paint.color = 0xCCDA74D4.toInt()
        canvas.drawLine(w * .72f, h * .84f, w * .98f, h, paint)
    }
    private fun dp(v: Int) = (v * resources.displayMetrics.density).toInt()
}
