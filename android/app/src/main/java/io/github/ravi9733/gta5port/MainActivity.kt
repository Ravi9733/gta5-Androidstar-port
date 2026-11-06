package io.github.ravi9733.gta5port

import android.app.Activity
import android.os.Bundle
import android.view.Choreographer
import android.view.Gravity
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.LinearLayout
import android.widget.TextView

class MainActivity : Activity(), SurfaceHolder.Callback, Choreographer.FrameCallback {
    companion object { init { System.loadLibrary("gta5port") } }
    private external fun nativeVulkanStatus(): String
    private external fun nativeAttachSurface(surface: Surface): String
    private external fun nativeResizeSurface(width: Int, height: Int)
    private external fun nativeRenderFrame()
    private external fun nativeDetachSurface()
    private lateinit var status: TextView
    private var resumed = false
    private var surfaceReady = false
    private var rendering = false

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
            setTextColor(0xFFFFFFFF.toInt())
            gravity = Gravity.CENTER
        }
        status = TextView(this).apply {
            text = "Preparing Vulkan renderer…"
            textSize = 14f
            setTextColor(0xFFD5DFEA.toInt())
            gravity = Gravity.CENTER
            setPadding(0, 14, 0, 14)
        }
        val view = SurfaceView(this).apply { holder.addCallback(this@MainActivity) }
        root.addView(title)
        root.addView(status)
        root.addView(view, LinearLayout.LayoutParams(-1, 0, 1f))
        setContentView(root)
    }

    private fun startFrames() {
        if (!resumed || !surfaceReady || rendering) return
        rendering = true
        Choreographer.getInstance().postFrameCallback(this)
    }

    private fun stopFrames() {
        rendering = false
        Choreographer.getInstance().removeFrameCallback(this)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        status.text = nativeAttachSurface(holder.surface)
        surfaceReady = true
        startFrames()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        nativeResizeSurface(width, height)
        status.text = nativeVulkanStatus()
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        surfaceReady = false
        stopFrames()
        nativeDetachSurface()
        status.text = "Vulkan surface closed"
    }

    override fun doFrame(frameTimeNanos: Long) {
        if (!rendering || !resumed || !surfaceReady) return
        nativeRenderFrame()
        Choreographer.getInstance().postFrameCallback(this)
    }

    override fun onPause() {
        resumed = false
        stopFrames()
        super.onPause()
    }

    override fun onResume() {
        super.onResume()
        resumed = true
        startFrames()
    }

    override fun onDestroy() {
        stopFrames()
        if (surfaceReady) {
            surfaceReady = false
            nativeDetachSurface()
        }
        super.onDestroy()
    }
}
