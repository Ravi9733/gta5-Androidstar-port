package io.github.ravi9733.gta5port

import android.app.AlertDialog
import android.content.Context
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.View
import android.view.SurfaceView
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast

/**
 * Native, interactive dashboard. Every label, card, panel and navigation item is
 * a real Android view; the reference screenshot is not displayed as an image.
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

    private val page = LinearLayout(context)
    private val accent = Color.rgb(190, 86, 255)
    private val muted = Color.rgb(166, 153, 190)
    private val panel = Color.rgb(25, 18, 39)
    private val navItems = mutableListOf<TextView>()
    private var activePage = "Home"

    init {
        setBackgroundColor(Color.rgb(9, 7, 17))
        clipChildren = true
        clipToPadding = true
        gameSurface.visibility = GONE

        val shell = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            setPadding(dp(14), dp(12), dp(14), dp(12))
            background = gradient(intArrayOf(Color.rgb(10, 7, 20), Color.rgb(23, 10, 38)), 0)
        }
        addView(shell, LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT))

        val rail = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.TOP or Gravity.CENTER_HORIZONTAL
            setPadding(dp(8), dp(12), dp(8), dp(8))
            background = gradient(intArrayOf(Color.rgb(25, 15, 40), Color.rgb(15, 11, 27)), 22)
        }
        shell.addView(rail, LinearLayout.LayoutParams(dp(148), LayoutParams.MATCH_PARENT))

        val brand = label("FIVE STAR", 18f, Color.WHITE, true).apply {
            gravity = Gravity.CENTER
            letterSpacing = 0.10f
        }
        rail.addView(brand, LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, dp(48)))
        rail.addView(label("GTA V PORT", 10f, accent, true).apply {
            gravity = Gravity.CENTER
            letterSpacing = 0.18f
        }, LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, dp(25)))
        addDivider(rail)

        listOf("⌂" to "Home", "✦" to "Mods", "⚙" to "Settings", "▣" to "Extras", "ⓘ" to "Credits").forEach { (icon, name) ->
            val item = TextView(context).apply {
                text = "$icon   $name"
                textSize = 14f
                typeface = Typeface.DEFAULT_BOLD
                gravity = Gravity.CENTER_VERTICAL
                setPadding(dp(12), 0, dp(6), 0)
                setTextColor(if (name == "Home") Color.WHITE else muted)
                background = if (name == "Home") gradient(intArrayOf(0xFF7135A8.toInt(), 0xFF3A1D5C.toInt()), 13) else transparentRounded(13)
                isClickable = true
                isFocusable = true
                setOnClickListener { showPage(name) }
            }
            navItems.add(item)
            rail.addView(item, LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, dp(48)).apply {
                topMargin = dp(5)
            })
        }

        val railSpacer = View(context)
        rail.addView(railSpacer, LinearLayout.LayoutParams(1, 0, 1f))
        rail.addView(label("LOS SANTOS", 10f, muted, true).apply { gravity = Gravity.CENTER }, LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, dp(28)))
        rail.addView(label("ANDROID EDITION", 9f, accent, true).apply { gravity = Gravity.CENTER }, LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, dp(20)))

        val right = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(2), dp(2), dp(2))
        }
        shell.addView(right, LinearLayout.LayoutParams(0, LayoutParams.MATCH_PARENT, 1f).apply {
            leftMargin = dp(14)
        })

        val top = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }
        right.addView(top, LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, dp(54)))
        val heading = LinearLayout(context).apply { orientation = LinearLayout.VERTICAL }
        heading.addView(label("GRAND THEFT AUTO V", 19f, Color.WHITE, true))
        heading.addView(label("ANDROID PORT  •  v1.6.2", 10f, accent, true))
        top.addView(heading, LinearLayout.LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f))
        listOf("ABOUT", "GAME FILES", "EXIT").forEach { title ->
            val b = smallButton(title)
            top.addView(b, LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, dp(34)).apply { leftMargin = dp(7) })
            b.setOnClickListener {
                when (title) {
                    "ABOUT" -> showCredits()
                    "GAME FILES" -> onCheckGameWasm()
                    else -> AlertDialog.Builder(context).setTitle("Exit Five Star Port")
                        .setMessage("Close the app?")
                        .setNegativeButton("Cancel", null)
                        .setPositiveButton("Exit") { _, _ -> (context as? android.app.Activity)?.finish() }.show()
                }
            }
        }
        addDivider(right)

        val scroll = ScrollView(context).apply {
            isFillViewport = true
            clipToPadding = false
            setPadding(0, dp(8), 0, dp(4))
        }
        right.addView(scroll, LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f))
        page.orientation = LinearLayout.VERTICAL
        scroll.addView(page, ScrollView.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT))

        val footer = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(12), 0, dp(12), 0)
            background = gradient(intArrayOf(0xFF21152F.toInt(), 0xFF140E20.toInt()), 12)
        }
        right.addView(footer, LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, dp(38)))
        footer.addView(label("LOS SANTOS  •  READY", 10f, accent, true), LinearLayout.LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f))
        footer.addView(label("RAM  •  ADRENO 730  •  60 FPS TARGET", 10f, muted, true))

        showPage("Home")
        addView(gameSurface, 0, LayoutParams(1, 1))
    }

    private fun showPage(name: String) {
        activePage = name
        navItems.forEach { item ->
            val selected = item.text.toString().endsWith(name)
            item.setTextColor(if (selected) Color.WHITE else muted)
            item.background = if (selected) gradient(intArrayOf(0xFF7135A8.toInt(), 0xFF3A1D5C.toInt()), 13) else transparentRounded(13)
        }
        page.removeAllViews()
        when (name) {
            "Home" -> buildHome()
            "Mods" -> buildMods()
            "Settings" -> buildSettings()
            "Extras" -> buildExtras()
            "Credits" -> buildCredits()
        }
    }

    private fun buildHome() {
        page.addView(label("WELCOME BACK", 12f, accent, true))
        page.addView(label("Choose your experience", 25f, Color.WHITE, true), margin(0, 4, 0, 12))
        val cards = listOf(
            Triple("STORY MODE", "Start the single-player experience", "STORY"),
            Triple("SANDBOX", "Explore the open-world mode", "SANDBOX"),
            Triple("PERFORMANCE", "Device and renderer diagnostics", "PERFORMANCE"),
            Triple("MODS & PATCHES", "Import and manage your add-ons", "MODS"),
            Triple("SELF RADIO", "Your local music library", "RADIO")
        )
        val cardRow = LinearLayout(context).apply { orientation = LinearLayout.HORIZONTAL }
        cards.forEachIndexed { index, data ->
            val card = makeCard(data.first, data.second, index + 1)
            cardRow.addView(card, LinearLayout.LayoutParams(0, dp(158), 1f).apply {
                if (index > 0) leftMargin = dp(7)
            })
        }
        page.addView(cardRow, margin(0, 4, 0, 12))
        val info = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
        }
        info.addView(infoPanel("GAME FILES", "Select your own game data", { onCheckGameWasm() }), LinearLayout.LayoutParams(0, dp(78), 1f))
        info.addView(infoPanel("MOD LIBRARY", "Import ZIP / RPF packages", { showPage("Mods") }), LinearLayout.LayoutParams(0, dp(78), 1f).apply { leftMargin = dp(10) })
        page.addView(info)
        page.addView(statusPanel(), margin(0, 10, 0, 0))
    }

    private fun makeCard(title: String, subtitle: String, index: Int): View {
        val card = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.BOTTOM
            setPadding(dp(13), dp(12), dp(13), dp(12))
            background = gradient(
                intArrayOf(
                    when (index) {
                        1 -> 0xFF45205E.toInt()
                        2 -> 0xFF242C55.toInt()
                        3 -> 0xFF34214D.toInt()
                        4 -> 0xFF51244E.toInt()
                        else -> 0xFF253B52.toInt()
                    },
                    0xFF171020.toInt()
                ), 17
            )
            isClickable = true
            isFocusable = true
            elevation = dp(3).toFloat()
        }
        val glyph = arrayOf("▶", "◇", "⚡", "✦", "♫")[(index - 1).coerceIn(0, 4)]
        card.addView(label(glyph, 25f, Color.WHITE, true), LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f))
        card.addView(label(title, 13f, Color.WHITE, true))
        card.addView(label(subtitle, 10f, 0xFFD0C4DC.toInt(), false), margin(0, 4, 0, 0))
        card.setOnClickListener {
            when (index) {
                1 -> unavailable("Story Mode")
                2 -> unavailable("Sandbox")
                3 -> showPage("Settings")
                4 -> showPage("Mods")
                5 -> showRadio()
            }
        }
        return card
    }

    private fun buildMods() {
        page.addView(label("MODS & PATCHES", 24f, Color.WHITE, true))
        page.addView(label("Import and inspect your add-on files", 12f, muted), margin(0, 4, 0, 14))
        page.addView(actionCard("IMPORT MOD / ZIP / RPF", "Choose an archive or mod file from device storage", "IMPORT") { onImportMods() })
        page.addView(actionCard("REFRESH MOD LIBRARY", "Scan the app's imported-mods folder again", "REFRESH") { onRefreshMods() }, margin(0, 9, 0, 0))
        page.addView(actionCard("GAME WASM FILE", "Select and validate the game runtime file", "CHECK") { onCheckGameWasm() }, margin(0, 9, 0, 0))
        page.addView(textPanel("IMPORTED FILES", modsText), margin(0, 12, 0, 0))
        page.addView(textPanel("IMPORT STATUS", statusText), margin(0, 8, 0, 0))
    }

    private fun buildSettings() {
        page.addView(label("SETTINGS", 24f, Color.WHITE, true))
        page.addView(label("Device, graphics and runtime information", 12f, muted), margin(0, 4, 0, 14))
        page.addView(actionCard("GRAPHICS & PERFORMANCE", "View live UI frame rate, memory and GPU diagnostics", "VIEW") { showPerformance() })
        page.addView(actionCard("VULKAN RENDERER", "Check renderer initialization status", "STATUS") {
            AlertDialog.Builder(context).setTitle("Vulkan status")
                .setMessage(statusText.text.toString() + "\n\nThe current renderer is a surface bootstrap; GTA V gameplay rendering is not integrated.")
                .setPositiveButton("OK", null).show()
        }, margin(0, 9, 0, 0))
        page.addView(actionCard("GAME DATA", "Select and validate your own WASM file", "SELECT") { onCheckGameWasm() }, margin(0, 9, 0, 0))
        page.addView(textPanel("GAME FILE VALIDATION", gameWasmText), margin(0, 12, 0, 0))
    }

    private fun buildExtras() {
        page.addView(label("EXTRAS", 24f, Color.WHITE, true))
        page.addView(label("Tools and optional content", 12f, muted), margin(0, 4, 0, 14))
        page.addView(actionCard("MOD IMPORTER", "Add ZIP, RPF and supported mod files", "OPEN") { onImportMods() })
        page.addView(actionCard("REFRESH CONTENT", "Update the imported file list", "REFRESH") { onRefreshMods() }, margin(0, 9, 0, 0))
        page.addView(actionCard("CHECK GAME FILES", "Validate a selected WASM file", "CHECK") { onCheckGameWasm() }, margin(0, 9, 0, 0))
        page.addView(actionCard("SELF RADIO", "Open local audio picker", "OPEN") { showRadio() }, margin(0, 9, 0, 0))
    }

    private fun buildCredits() {
        page.addView(label("CREDITS", 24f, Color.WHITE, true))
        page.addView(label("FIVE STAR PORT  •  ANDROID INTERFACE", 12f, accent, true), margin(0, 4, 0, 14))
        page.addView(textPanel("ABOUT THIS BUILD", label(
            "This is an Android dashboard and Vulkan surface bootstrap. It does not include Rockstar game files. GTA V gameplay runtime, world rendering, Story Mode and Sandbox are not connected yet. Use your own legally obtained game data.",
            13f, Color.WHITE, false
        )))
        page.addView(actionCard("PROJECT STATUS", "View current Android renderer status", "VIEW") {
            AlertDialog.Builder(context).setTitle("Project status")
                .setMessage(statusText.text.toString()).setPositiveButton("OK", null).show()
        }, margin(0, 10, 0, 0))
    }

    private fun statusPanel(): View = textPanel("SYSTEM STATUS", statusText)
    private fun textPanel(title: String, body: TextView): View {
        val box = panelBox()
        box.addView(label(title, 11f, accent, true))
        body.setTextColor(Color.WHITE)
        body.textSize = 11f
        body.setPadding(0, dp(7), 0, 0)
        box.addView(body, LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT))
        return box
    }

    private fun infoPanel(title: String, detail: String, click: () -> Unit): View {
        val box = panelBox().apply { isClickable = true; setOnClickListener { click() } }
        box.addView(label(title, 11f, accent, true))
        box.addView(label(detail, 10f, Color.WHITE), margin(0, 4, 0, 0))
        return box
    }

    private fun actionCard(title: String, detail: String, action: String, click: () -> Unit): View {
        val row = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(14), dp(10), dp(12), dp(10))
            background = gradient(intArrayOf(0xFF291B3B.toInt(), 0xFF191222.toInt()), 14)
            isClickable = true
            setOnClickListener { click() }
        }
        val text = LinearLayout(context).apply { orientation = LinearLayout.VERTICAL }
        text.addView(label(title, 13f, Color.WHITE, true))
        text.addView(label(detail, 10f, muted), margin(0, 4, 0, 0))
        row.addView(text, LinearLayout.LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f))
        row.addView(smallButton(action))
        return row
    }

    private fun panelBox(): LinearLayout = LinearLayout(context).apply {
        orientation = LinearLayout.VERTICAL
        setPadding(dp(14), dp(12), dp(14), dp(12))
        background = gradient(intArrayOf(0xFF241831.toInt(), 0xFF171020.toInt()), 14)
    }

    private fun smallButton(text: String): Button = Button(context).apply {
        this.text = text
        textSize = 9f
        isAllCaps = true
        setTextColor(Color.WHITE)
        background = gradient(intArrayOf(0xFF8D42C1.toInt(), 0xFF562B80.toInt()), 10)
        minHeight = 0
        minimumHeight = 0
        minWidth = 0
        minimumWidth = 0
        setPadding(dp(9), 0, dp(9), 0)
        stateListAnimator = null
    }

    private fun showPerformance() {
        AlertDialog.Builder(context).setTitle("Performance diagnostics")
            .setMessage(performanceText.text.toString().ifBlank { "Waiting for live device metrics…" } +
                "\n\nThese are Android UI/device metrics, not GTA gameplay FPS.")
            .setPositiveButton("OK", null).show()
    }

    private fun showRadio() {
        val intent = android.content.Intent(android.content.Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(android.content.Intent.CATEGORY_OPENABLE)
            type = "audio/*"
        }
        try {
            (context as? android.app.Activity)?.startActivityForResult(intent, 7103)
        } catch (_: Exception) {
            Toast.makeText(context, "Audio picker is unavailable on this device.", Toast.LENGTH_LONG).show()
        }
    }

    private fun showCredits() {
        AlertDialog.Builder(context).setTitle("Five Star Port")
            .setMessage("Android dashboard interface. Game files are not included. GTA V gameplay integration is still under development.")
            .setPositiveButton("OK", null).show()
    }

    private fun unavailable(feature: String) {
        AlertDialog.Builder(context).setTitle(feature)
            .setMessage("The dashboard action is wired, but the GTA V gameplay runtime is not connected in this build.")
            .setPositiveButton("OK", null).show()
    }

    private fun label(text: String, size: Float, color: Int, bold: Boolean = false): TextView =
        TextView(context).apply {
            this.text = text
            textSize = size
            setTextColor(color)
            if (bold) typeface = Typeface.create("sans-serif", Typeface.BOLD)
        }

    private fun addDivider(parent: LinearLayout) {
        parent.addView(View(context).apply { setBackgroundColor(0x553F3153) },
            LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, dp(1)).apply {
                topMargin = dp(8); bottomMargin = dp(8)
            })
    }

    private fun margin(l: Int, t: Int, r: Int, b: Int) =
        LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT).apply {
            setMargins(dp(l), dp(t), dp(r), dp(b))
        }

    private fun gradient(colors: IntArray, radius: Int): GradientDrawable =
        GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM, colors).apply {
            cornerRadius = dp(radius).toFloat()
            setStroke(dp(1), 0x665D3B79)
        }

    private fun transparentRounded(radius: Int) = GradientDrawable().apply {
        cornerRadius = dp(radius).toFloat()
        setColor(Color.TRANSPARENT)
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()
}
