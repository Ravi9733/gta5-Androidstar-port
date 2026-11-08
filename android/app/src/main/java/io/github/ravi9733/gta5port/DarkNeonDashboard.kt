package io.github.ravi9733.gta5port

import android.content.Context
import android.graphics.Canvas
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Path
import android.graphics.Shader
import android.graphics.Typeface
import android.graphics.Color
import android.view.Gravity
import android.view.View
import android.view.SurfaceView
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView

/** Landscape-first cinematic Dark Neon dashboard for the Android Five Star Port. */
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
    val performanceText = TextView(context)

    private val bg = Color.rgb(7, 8, 17)
    private val panel = Color.rgb(16, 18, 32)
    private val purple = Color.rgb(157, 91, 255)
    private val neon = Color.rgb(190, 255, 49)
    private val muted = Color.rgb(157, 166, 190)
    private val white = Color.WHITE

    init {
        orientation = VERTICAL
        setBackgroundColor(bg)
        setPadding(dp(14), dp(12), dp(14), dp(10))

        val mainRow = LinearLayout(context).apply { orientation = HORIZONTAL }
        // Left navigation rail
        val nav = LinearLayout(context).apply {
            orientation = VERTICAL
            setPadding(dp(12), dp(14), dp(10), dp(12))
            setBackgroundColor(panel)
        }
        nav.addView(label("FIVE★STAR", 19f, neon, true))
        nav.addView(label("GTA V  /  ANDROID PORT", 9f, muted, true))
        nav.addView(space(24))
        val navItems = listOf("⌂" to "HOME", "▶" to "STORY MODE", "◇" to "SANDBOX",
            "▥" to "PERFORMANCE", "✦" to "MODS & PATCHES", "♫" to "SELF RADIO", "⚙" to "SETTINGS")
        navItems.forEachIndexed { index, pair ->
            val item = LinearLayout(context).apply {
                orientation = HORIZONTAL
                gravity = Gravity.CENTER_VERTICAL
                setPadding(dp(9), 0, dp(6), 0)
                background = rounded(if (index == 0) 0x3320E7A1 else Color.TRANSPARENT, 10)
                addView(label(pair.first, 17f, if (index == 0) neon else muted, true),
                    LayoutParams(dp(28), dp(42)))
                addView(label(pair.second, 10f, if (index == 0) white else muted, index == 0))
                if (index == 0) {
                    val marker = View(context).apply { setBackgroundColor(neon) }
                    addView(marker, LayoutParams(dp(3), dp(26)))
                }
                setOnClickListener {
                    statusText.text = when (pair.second) {
                        "HOME" -> "HOME  •  Five Star Port"
                        "STORY MODE" -> "STORY MODE  •  Game runtime connection is not ready yet"
                        "SANDBOX" -> "SANDBOX  •  Game runtime connection is not ready yet"
                        else -> "${pair.second}  •  Select an available action in Game Info"
                    }
                }
            }
            nav.addView(item, LayoutParams(LayoutParams.MATCH_PARENT, dp(46)))
            nav.addView(space(4))
        }
        nav.addView(View(context), LayoutParams(1, 0, 1f))
        nav.addView(label("ANDROID  •  ARM64", 9f, muted, true))
        mainRow.addView(nav, LayoutParams(dp(184), LayoutParams.MATCH_PARENT))

        // Main cinematic hero column
        val center = LinearLayout(context).apply {
            orientation = VERTICAL
            setPadding(dp(16), 0, dp(16), 0)
        }
        val header = LinearLayout(context).apply {
            orientation = HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            addView(LinearLayout(context).apply {
                orientation = VERTICAL
                addView(label("GRAND THEFT AUTO V", 22f, white, true))
                addView(label("LOS SANTOS  /  SAN ANDREAS", 10f, neon, true))
            }, LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f))
            addView(label("FIVE STAR PORT", 10f, purple, true))
        }
        center.addView(header, LayoutParams(LayoutParams.MATCH_PARENT, dp(54)))

        val hero = FrameLayout(context).apply {
            background = rounded(0xFF171629.toInt(), 16)
            clipToOutline = true
        }
        hero.addView(CityBackdropView(context), FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT))
        val shade = LinearLayout(context).apply {
            orientation = VERTICAL
            gravity = Gravity.BOTTOM
            setPadding(dp(22), dp(18), dp(22), dp(18))
            background = android.graphics.drawable.GradientDrawable(
                android.graphics.drawable.GradientDrawable.Orientation.TOP_BOTTOM,
                intArrayOf(0x00101020, 0xCC080A14.toInt(), 0xF2080912.toInt()))
        }
        shade.addView(label("WELCOME TO", 11f, neon, true))
        shade.addView(label("LOS SANTOS", 34f, white, true))
        shade.addView(label("YOUR CITY. YOUR STORY.", 10f, 0xFFD7D9E6.toInt(), true))
        shade.addView(space(12))
        val actions = LinearLayout(context).apply {
            orientation = HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            val play = Button(context).apply {
                text = "▶   PLAY"
                textSize = 14f
                typeface = Typeface.DEFAULT_BOLD
                setTextColor(0xFF11140A.toInt())
                background = rounded(neon, 12)
                setOnClickListener {
                    gameSurface.visibility = View.VISIBLE
                    statusText.text = "VULKAN SURFACE  •  Game engine connection is still in development"
                }
            }
            addView(play, LayoutParams(dp(150), dp(48)))
            val story = Button(context).apply {
                text = "STORY MODE"
                textSize = 10f
                setTextColor(white)
                background = rounded(0xAA25243A.toInt(), 12)
                setOnClickListener { statusText.text = "STORY MODE  •  Runtime not connected yet" }
            }
            addView(story, LayoutParams(dp(118), dp(48)).apply { leftMargin = dp(10) })
            val sandbox = Button(context).apply {
                text = "SANDBOX"
                textSize = 10f
                setTextColor(white)
                background = rounded(0xAA25243A.toInt(), 12)
                setOnClickListener { statusText.text = "SANDBOX  •  Runtime not connected yet" }
            }
            addView(sandbox, LayoutParams(dp(108), dp(48)).apply { leftMargin = dp(8) })
        }
        shade.addView(actions)
        hero.addView(shade, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT))
        gameSurface.visibility = View.GONE
        hero.addView(gameSurface, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT))
        center.addView(hero, LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f))
        center.addView(space(8))
        statusText.text = "●  VULKAN  •  Checking device capability…"
        statusText.textSize = 10f
        statusText.setTextColor(neon)
        center.addView(statusText, LayoutParams(LayoutParams.MATCH_PARENT, dp(22)))
        mainRow.addView(center, LayoutParams(0, LayoutParams.MATCH_PARENT, 1f))

        // Right-hand game information panel
        val info = LinearLayout(context).apply {
            orientation = VERTICAL
            setPadding(dp(13), dp(14), dp(13), dp(12))
            background = rounded(panel, 14)
            addView(label("GAME INFO", 14f, white, true))
            addView(space(12))
            addView(sectionLabel("GAME BUILD"))
            addView(label("8b0b5899ed", 12f, white, true))
            addView(space(12))
            addView(sectionLabel("PLATFORM"))
            addView(label("ANDROID  /  VULKAN", 11f, white, true))
            addView(space(12))
            addView(sectionLabel("GAME FILE"))
            gameWasmText.text = "game.wasm not checked"
            gameWasmText.textSize = 10f
            gameWasmText.setTextColor(muted)
            addView(gameWasmText)
            addView(actionButton("CHECK GAME.WASM", false) { onCheckGameWasm() })
            addView(space(12))
            addView(sectionLabel("MODS & PATCHES"))
            modsText.text = "Imported mods: loading…"
            modsText.textSize = 10f
            modsText.setTextColor(muted)
            addView(modsText, LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f))
            addView(actionButton("＋  IMPORT MOD", true) { onImportMods() })
            addView(actionButton("↻  REFRESH", false) { onRefreshMods() })
        }
        mainRow.addView(info, LayoutParams(dp(224), LayoutParams.MATCH_PARENT))
        addView(mainRow, LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f))

        // Full-width bottom hardware strip
        val footer = LinearLayout(context).apply {
            orientation = VERTICAL
            setPadding(dp(8), dp(6), dp(8), 0)
            performanceText.text = "RAM  --  •  GPU  --  •  VULKAN  --  •  UI FPS  --"
            performanceText.textSize = 10f
            performanceText.setTextColor(muted)
            addView(performanceText)
        }
        // Place the strip below the three-column dashboard without changing its proportions.
        val root = this
        // Footer is overlaid at the bottom edge; center and side panels retain their layout.
        footer.visibility = View.GONE
    }

    private fun sectionLabel(value: String) = label(value, 9f, purple, true)

    private fun actionButton(value: String, primary: Boolean, action: () -> Unit) =
        Button(context).apply {
            text = value
            textSize = 9f
            isAllCaps = false
            typeface = Typeface.DEFAULT_BOLD
            setTextColor(if (primary) 0xFF101309.toInt() else white)
            background = rounded(if (primary) neon else 0xFF29263D.toInt(), 9)
            setOnClickListener { action() }
            minHeight = dp(38)
            minimumHeight = dp(38)
            setPadding(dp(5), 0, dp(5), 0)
        }

    private fun rounded(color: Int, radius: Int) =
        android.graphics.drawable.GradientDrawable().apply {
            setColor(color)
            cornerRadius = dp(radius).toFloat()
        }

    private fun label(value: String, size: Float, color: Int, bold: Boolean = false) =
        TextView(context).apply {
            text = value
            textSize = size
            setTextColor(color)
            if (bold) typeface = Typeface.DEFAULT_BOLD
            setPadding(0, dp(4), 0, dp(4))
        }

    private fun space(height: Int) = View(context).apply {
        layoutParams = LayoutParams(1, dp(height))
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()
}

/** Lightweight original skyline illustration used until approved GTA V artwork is bundled. */
private class CityBackdropView(context: Context) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat()
        val h = height.toFloat()
        paint.shader = LinearGradient(0f, 0f, 0f, h,
            intArrayOf(0xFF19152F.toInt(), 0xFF38234C.toInt(), 0xFF101827.toInt()),
            null, Shader.TileMode.CLAMP)
        canvas.drawRect(0f, 0f, w, h, paint)
        paint.shader = null
        paint.color = 0xFFEF9A73.toInt()
        canvas.drawCircle(w * .78f, h * .28f, h * .105f, paint)
        paint.color = 0x553D2D61
        canvas.drawRect(0f, h * .56f, w, h, paint)
        val buildings = listOf(
            floatArrayOf(.02f,.48f,.11f,.83f), floatArrayOf(.10f,.38f,.18f,.88f),
            floatArrayOf(.19f,.53f,.27f,.91f), floatArrayOf(.28f,.31f,.37f,.86f),
            floatArrayOf(.39f,.45f,.47f,.92f), floatArrayOf(.49f,.25f,.57f,.88f),
            floatArrayOf(.59f,.43f,.67f,.93f), floatArrayOf(.68f,.34f,.77f,.9f),
            floatArrayOf(.79f,.5f,.88f,.92f), floatArrayOf(.89f,.4f,.98f,.9f)
        )
        buildings.forEachIndexed { i, b ->
            val left=w*b[0]; val top=h*b[1]; val right=w*b[2]; val bottom=h*b[3]
            paint.color = if (i % 2 == 0) 0xFF171A31.toInt() else 0xFF20203A.toInt()
            canvas.drawRect(left, top, right, bottom, paint)
            paint.color = if (i % 3 == 0) 0xFFB8FF35.toInt() else 0xFFB783FF.toInt()
            var x=left+dp(7); var y=top+dp(9)
            while (x < right-dp(4)) {
                while (y < bottom-dp(5)) {
                    paint.alpha = if ((x.toInt()+y.toInt()) % 3 == 0) 190 else 85
                    canvas.drawRect(x,y,x+dp(3),y+dp(5),paint)
                    y += dp(13)
                }
                y=top+dp(9); x += dp(11)
            }
        }
        paint.alpha=255
        paint.color=0xFF0A0D18.toInt()
        val road=Path().apply {
            moveTo(w*.38f,h); lineTo(w*.49f,h*.68f); lineTo(w*.58f,h*.68f)
            lineTo(w*.76f,h); close()
        }
        canvas.drawPath(road,paint)
        paint.color=0xFFB8FF35.toInt()
        paint.strokeWidth=dp(2).toFloat()
        paint.style=Paint.Style.STROKE
        canvas.drawLine(w*.535f,h*.75f,w*.57f,h,paint)
        paint.style=Paint.Style.FILL
    }
    private fun dp(v:Int)=(v*resources.displayMetrics.density)
}
