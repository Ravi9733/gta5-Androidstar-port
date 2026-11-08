package io.github.ravi9733.gta5port

import android.app.AlertDialog
import android.content.Context
import android.graphics.Color
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
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
    private val neon = 0xFFB7FF4A.toInt()
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
        heading.addView(label("FIVE STAR PORT", 20f, Color.WHITE, true))
        heading.addView(label("GRAND THEFT AUTO V  •  ANDROID EDITION", 10f, 0xFFB7FF4A.toInt(), true))
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
        right.addView(scroll, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
        page.orientation = LinearLayout.VERTICAL
        scroll.addView(page, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))

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
        page.addView(label("WELCOME TO LOS SANTOS", 11f, 0xFFB7FF4A.toInt(), true))
        page.addView(label("YOUR CITY. YOUR STORY.", 25f, Color.WHITE, true), margin(0, 3, 0, 8))
        val hero = FrameLayout(context).apply {
            background = gradient(intArrayOf(0xFF34204D.toInt(), 0xFF11101D.toInt()), 18)
            clipToOutline = true
            elevation = dp(4).toFloat()
        }
        hero.addView(LosSantosHeroArt(context), LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT))
        val heroCopy = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.BOTTOM
            setPadding(dp(18), dp(12), dp(18), dp(14))
        }
        heroCopy.addView(label("LOS SANTOS", 11f, 0xFFB7FF4A.toInt(), true))
        heroCopy.addView(label("THE CITY IS YOURS", 22f, Color.WHITE, true), margin(0, 3, 0, 0))
        heroCopy.addView(label("FIVE STAR PORT  •  MOBILE EXPERIENCE", 9f, 0xFFE3D9F1.toInt(), true), margin(0, 2, 0, 0))
        hero.addView(heroCopy, FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT))
        page.addView(hero, LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, dp(210)).apply {
            bottomMargin = dp(12)
        })
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
            cardRow.addView(card, LinearLayout.LayoutParams(0, dp(142), 1f).apply {
                if (index > 0) leftMargin = dp(8)
            })
        }
        page.addView(cardRow, margin(0, 0, 0, 12))
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
                        1 -> 0xFF59316F.toInt()
                        2 -> 0xFF303D68.toInt()
                        3 -> 0xFF49315F.toInt()
                        4 -> 0xFF65305B.toInt()
                        else -> 0xFF28566A.toInt()
                    },
                    0xFF171020.toInt()
                ), 15
            )
            isClickable = true
            isFocusable = true
            elevation = dp(3).toFloat()
        }
        val art = FrameLayout(context).apply {
            background = transparentRounded(10)
            clipToOutline = true
        }
        art.addView(CardArtView(context, index), LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT))
        card.addView(art, LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f).apply {
            bottomMargin = dp(8)
        })
        card.addView(label(title, 12f, Color.WHITE, true))
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

/** Cinematic neon Los Santos poster artwork drawn natively (not a screenshot background). */
private class LosSantosHeroArt(context: Context) : View(context) {
    private val p = Paint(Paint.ANTI_ALIAS_FLAG)
    private fun fill(c: Canvas, color: Int, l: Float, t: Float, r: Float, b: Float) {
        p.color = color; p.shader = null; c.drawRect(l, t, r, b, p)
    }
    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat(); val h = height.toFloat()
        if (w <= 0f || h <= 0f) return
        // Deep violet dusk, warm horizon and atmospheric glow.
        p.shader = android.graphics.LinearGradient(0f, 0f, 0f, h,
            intArrayOf(0xFF090B20.toInt(), 0xFF38234F.toInt(), 0xFFB64E79.toInt(), 0xFFFF9864.toInt()),
            floatArrayOf(0f, .42f, .76f, 1f), android.graphics.Shader.TileMode.CLAMP)
        canvas.drawRect(0f, 0f, w, h, p); p.shader = null
        p.shader = android.graphics.RadialGradient(w*.72f, h*.49f, h*.58f,
            intArrayOf(0x88FFB56A.toInt(), 0x33EF5B9C, 0x00100A20),
            null, android.graphics.Shader.TileMode.CLAMP)
        canvas.drawRect(0f, 0f, w, h, p); p.shader = null
        // Distant mountain ridges.
        val ridge = Path().apply {
            moveTo(0f,h*.68f); lineTo(w*.12f,h*.48f); lineTo(w*.22f,h*.59f)
            lineTo(w*.38f,h*.40f); lineTo(w*.53f,h*.59f); lineTo(w*.70f,h*.43f)
            lineTo(w*.83f,h*.57f); lineTo(w,h*.45f); lineTo(w,h); lineTo(0f,h); close()
        }
        p.color = 0xFF28213F.toInt(); canvas.drawPath(ridge,p)
        // Sunset disk with soft halo.
        p.shader = android.graphics.RadialGradient(w*.78f,h*.43f,h*.24f,
            intArrayOf(0xFFFFE0A0.toInt(),0xFFFF9C72.toInt(),0x00FF8B88),
            null,android.graphics.Shader.TileMode.CLAMP)
        canvas.drawCircle(w*.78f,h*.43f,h*.24f,p); p.shader=null
        p.color = 0xFFFFC18B.toInt(); canvas.drawCircle(w*.78f,h*.43f,h*.105f,p)
        // Palm silhouettes frame the poster.
        p.color = 0xFF111322.toInt()
        val palmXs = floatArrayOf(.045f,.93f)
        for (x in palmXs) {
            val px=x*w
            p.strokeWidth=w*.008f; p.style=Paint.Style.STROKE
            canvas.drawLine(px,h*.18f,px+(if(x<.5f) .025f else -.02f)*w,h*.94f,p)
            p.style=Paint.Style.FILL
            val tx=px+(if(x<.5f) .025f else -.02f)*w; val ty=h*.18f
            for (a in -2..2) {
                val leaf=Path().apply { moveTo(tx,ty); quadTo(tx+a*w*.045f,ty-h*.13f,tx+a*w*.095f,ty-h*.10f) }
                p.strokeWidth=w*.009f; p.style=Paint.Style.STROKE; canvas.drawPath(leaf,p); p.style=Paint.Style.FILL
            }
        }
        // Dense downtown skyline with lit windows and a few tall towers.
        val buildings = floatArrayOf(
            0f,.57f,.07f,.89f, .055f,.49f,.12f,.90f, .105f,.61f,.17f,.91f,
            .16f,.53f,.23f,.91f, .22f,.43f,.28f,.91f, .275f,.58f,.34f,.92f,
            .33f,.49f,.40f,.92f, .39f,.36f,.455f,.92f, .45f,.51f,.52f,.92f,
            .515f,.45f,.58f,.92f, .575f,.56f,.64f,.92f, .635f,.40f,.70f,.92f,
            .695f,.53f,.76f,.92f, .755f,.58f,.82f,.92f, .815f,.46f,.88f,.92f,
            .875f,.55f,.94f,.92f, .935f,.62f,1f,.92f
        )
        p.color=0xFF111426.toInt()
        var i=0
        while(i<buildings.size) {
            val l=buildings[i]*w; val t=buildings[i+1]*h; val r=buildings[i+2]*w; val b=buildings[i+3]*h
            canvas.drawRect(l,t,r,b,p)
            p.color=0xFFFFC77D.toInt()
            val cols=((r-l)/(w*.018f)).toInt().coerceAtLeast(1)
            val rows=((b-t)/(h*.055f)).toInt().coerceAtLeast(1)
            for(cx in 0 until cols) for(cy in 0 until rows) {
                if ((cx*3+cy*5+i)%4!=0) canvas.drawRect(l+w*.006f+cx*w*.018f,t+h*.025f+cy*h*.055f,
                    l+w*.011f+cx*w*.018f,t+h*.038f+cy*h*.055f,p)
            }
            p.color=0xFF111426.toInt(); i+=4
        }
        // Neon boulevard and reflected light leading into the city.
        val road=Path().apply { moveTo(w*.40f,h); lineTo(w*.49f,h*.84f); lineTo(w*.72f,h*.84f); lineTo(w*.98f,h); close() }
        p.shader=android.graphics.LinearGradient(0f,h*.83f,0f,h,
            intArrayOf(0xFF282039.toInt(),0xFF080B16.toInt()),null,android.graphics.Shader.TileMode.CLAMP)
        canvas.drawPath(road,p); p.shader=null
        p.color=0xAAE64BBA.toInt(); p.strokeWidth=h*.012f
        canvas.drawLine(w*.49f,h*.85f,w*.40f,h,p)
        p.color=0xAA9EFF54.toInt(); canvas.drawLine(w*.72f,h*.85f,w*.98f,h,p)
        // Three dramatic foreground character silhouettes, rim-lit in neon.
        val people=floatArrayOf(.27f,.43f,.59f)
        for((idx,x) in people.withIndex()) {
            val cx=x*w; val headY=h*(if(idx==1).57f else .60f); val radius=h*(if(idx==1).055f else .047f)
            p.color=if(idx==1) 0xFF080B15.toInt() else 0xFF11101B.toInt()
            canvas.drawCircle(cx,headY,radius,p)
            val body=Path().apply {
                moveTo(cx-radius*.8f,headY+radius*.7f)
                lineTo(cx-radius*1.65f,h*.96f); lineTo(cx+radius*1.65f,h*.96f)
                lineTo(cx+radius*.8f,headY+radius*.7f); close()
            }
            canvas.drawPath(body,p)
            p.color=if(idx==1) 0xFFB7FF4A.toInt() else 0xFFE66BB8.toInt()
            p.strokeWidth=w*.003f; p.style=Paint.Style.STROKE; canvas.drawPath(body,p); p.style=Paint.Style.FILL
        }
        // Foreground sports car with glass, wheels, headlamps and neon underglow.
        p.setShadowLayer(h*.035f,0f,0f,0xFFB64EFF.toInt())
        p.color=0xFF080B13.toInt()
        val car=Path().apply {
            moveTo(w*.54f,h*.91f); lineTo(w*.60f,h*.81f); lineTo(w*.70f,h*.77f)
            lineTo(w*.82f,h*.79f); lineTo(w*.90f,h*.88f); lineTo(w*.97f,h*.91f)
            lineTo(w*.98f,h*.98f); lineTo(w*.52f,h*.98f); close()
        }
        canvas.drawPath(car,p); p.clearShadowLayer()
        p.color=0xFF272A40.toInt()
        val glass=Path().apply { moveTo(w*.63f,h*.82f); lineTo(w*.71f,h*.79f); lineTo(w*.81f,h*.81f); lineTo(w*.85f,h*.87f); lineTo(w*.61f,h*.87f); close() }
        canvas.drawPath(glass,p)
        p.color=0xFFB7FF4A.toInt(); canvas.drawRoundRect(w*.91f,h*.90f,w*.97f,h*.925f,h*.01f,h*.01f,p)
        p.color=0xFFFF5CA8.toInt(); canvas.drawRoundRect(w*.53f,h*.91f,w*.58f,h*.93f,h*.01f,h*.01f,p)
        p.color=0xFF05070E.toInt()
        canvas.drawCircle(w*.63f,h*.98f,h*.045f,p); canvas.drawCircle(w*.87f,h*.98f,h*.045f,p)
        p.color=0xFF9EFF54.toInt(); p.style=Paint.Style.STROKE; p.strokeWidth=h*.008f
        canvas.drawCircle(w*.63f,h*.98f,h*.025f,p); canvas.drawCircle(w*.87f,h*.98f,h*.025f,p); p.style=Paint.Style.FILL
        // Fine poster grain / scanline accents and purple edge glow.
        p.color=0x22FFFFFF
        var y=0f
        while(y<h) { canvas.drawLine(0f,y,w,y,p); y+=h*.035f }
        p.color=0xFFB94EFF.toInt(); p.style=Paint.Style.STROKE; p.strokeWidth=resources.displayMetrics.density
        val edge=18f*resources.displayMetrics.density
        canvas.drawRoundRect(RectF(1f,1f,w-1f,h-1f),edge,edge,p); p.style=Paint.Style.FILL
    }
}
private class CardArtView(context: Context, private val variant: Int) : View(context) {
    private val p = Paint(Paint.ANTI_ALIAS_FLAG)
    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat(); val h = height.toFloat()
        val palettes = arrayOf(
            intArrayOf(0xFF9C4B78.toInt(), 0xFF24172F.toInt()),
            intArrayOf(0xFF4C76A5.toInt(), 0xFF17223A.toInt()),
            intArrayOf(0xFF9A63C5.toInt(), 0xFF20172F.toInt()),
            intArrayOf(0xFFB34D82.toInt(), 0xFF29162C.toInt()),
            intArrayOf(0xFF3C9A9A.toInt(), 0xFF14242D.toInt())
        )
        val colors = palettes[(variant - 1).coerceIn(0, 4)]
        p.shader = android.graphics.LinearGradient(0f, 0f, w, h, colors[0], colors[1],
            android.graphics.Shader.TileMode.CLAMP)
        canvas.drawRect(0f, 0f, w, h, p); p.shader = null
        p.color = 0x553B204F
        canvas.drawCircle(w*.76f, h*.30f, h*.42f, p)
        p.color = 0xFFB7FF4A.toInt()
        p.textSize = h*.34f
        p.typeface = Typeface.create("sans-serif-black", Typeface.BOLD)
        val glyphs = arrayOf("V", "◇", "⚡", "✦", "♫")
        canvas.drawText(glyphs[(variant - 1).coerceIn(0, 4)], w*.08f, h*.62f, p)
        p.color = 0x55FFFFFF
        canvas.drawRect(0f, h*.84f, w, h, p)
    }
}
