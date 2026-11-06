package io.github.ravi9733.gta5port

import android.app.Activity
import android.os.Bundle
import android.graphics.Typeface
import android.view.Gravity
import android.widget.LinearLayout
import android.widget.TextView

class MainActivity : Activity() {
    companion object {
        init { System.loadLibrary("gta5port") }
    }

    private external fun nativePortStatus(): String

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER
            setPadding(32, 32, 32, 32)
            setBackgroundColor(0xFF10151D.toInt())
        }
        val title = TextView(this).apply {
            text = "GTA V Android Port"
            textSize = 26f
            setTypeface(null, Typeface.BOLD)
            setTextColor(0xFFFFFFFF.toInt())
            gravity = Gravity.CENTER
        }
        val status = TextView(this).apply {
            text = nativePortStatus()
            textSize = 16f
            setTextColor(0xFFD5DFEA.toInt())
            gravity = Gravity.CENTER
            setPadding(0, 24, 0, 0)
        }
        root.addView(title)
        root.addView(status)
        setContentView(root)
    }
}
