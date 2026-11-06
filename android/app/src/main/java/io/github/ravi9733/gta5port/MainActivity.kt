package io.github.ravi9733.gta5port

import android.app.Activity
import android.os.Bundle
import android.graphics.Typeface
import android.view.Gravity
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.LinearLayout
import android.widget.TextView

class MainActivity : Activity(), SurfaceHolder.Callback {
    companion object {
        init { System.loadLibrary("gta5port") }
    }

    private external fun nativeVulkanStatus(): String
    private external fun nativeAttachSurface(surface: Surface): String
    private external fun nativeDetachSurface()
    private lateinit var status: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER
            setPadding(28, 28, 28, 28)
            setBackgroundColor(0xFF10151D.toInt())
        }
        val title = TextView(this).apply {
            text = "GTA V Android Port"
            textSize = 25f
            setTypeface(null, Typeface.BOLD)
            setTextColor(0xFFFFFFFF.toInt())
            gravity = Gravity.CENTER
        }
        status = TextView(this).apply {
            text = "Initializing Vulkan check…"
            textSize = 15f
            setTextColor(0xFFD5DFEA.toInt())
            gravity = Gravity.CENTER
            setPadding(0, 18, 0, 18)
        }
        val surfaceView = SurfaceView(this).apply {
            holder.addCallback(this@MainActivity)
            setBackgroundColor(0xFF080B10.toInt())
        }
        root.addView(title)
        root.addView(status)
        root.addView(surfaceView, LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f
        ))
        setContentView(root)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        status.text = nativeAttachSurface(holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // Vulkan swapchain sizing will be connected when the renderer is added.
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        nativeDetachSurface()
        status.text = nativeVulkanStatus()
    }
}
