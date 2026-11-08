package io.github.ravi9733.gta5port

import android.app.Activity
import android.app.ActivityManager
import android.os.Build
import android.os.SystemClock
import android.content.Intent
import android.os.Bundle
import android.net.Uri
import android.os.Handler
import android.os.HandlerThread
import android.view.Choreographer
import android.view.Gravity
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.LinearLayout
import android.widget.TextView
import java.util.concurrent.atomic.AtomicBoolean

class MainActivity : Activity(), SurfaceHolder.Callback, Choreographer.FrameCallback {
    companion object { private const val REQUEST_IMPORT_MOD = 7101; private const val REQUEST_CHECK_WASM = 7102; init { System.loadLibrary("gta5port") } }
    private external fun nativeVulkanStatus(): String
    private external fun nativeAvailableMemoryEstimate(): Long
    private external fun nativeAttachSurface(surface: Surface): String
    private external fun nativeResizeSurface(width: Int, height: Int)
    private external fun nativeRenderFrame()
    private external fun nativeDetachSurface()
    private lateinit var status: TextView
    private lateinit var modList: TextView
    private lateinit var gameWasmText: TextView
    private lateinit var performanceText: TextView
    private var perfWindowStartMs = 0L
    private var perfFrameCount = 0
    private lateinit var gameSurface: SurfaceView
    private var resumed = false
    private var surfaceReady = false
    private var rendering = false
    private var statusFrameCounter = 0
    private lateinit var renderThread: HandlerThread
    private lateinit var renderHandler: Handler
    private val framePending = AtomicBoolean(false)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        renderThread = HandlerThread("GTA5-Vulkan").also { it.start() }
        renderHandler = Handler(renderThread.looper)
        gameSurface = SurfaceView(this).apply { holder.addCallback(this@MainActivity) }
        val dashboard = DarkNeonDashboard(
            context = this,
            gameSurface = gameSurface,
            onImportMods = { openModPicker() },
            onRefreshMods = { refreshModList() },
            onCheckGameWasm = { openWasmPicker() }
        )
        status = dashboard.statusText
        modList = dashboard.modsText
        gameWasmText = dashboard.gameWasmText
        performanceText = dashboard.performanceText
        refreshModList()
        setContentView(dashboard)
    }

    private fun openModPicker() {
        val picker = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
        }
        startActivityForResult(picker, REQUEST_IMPORT_MOD)
    }

    private fun openWasmPicker() {
        val picker = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
        }
        startActivityForResult(picker, REQUEST_CHECK_WASM)
    }

    @Deprecated("Uses the platform document picker for broad Android compatibility")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode != Activity.RESULT_OK) return
        val resultIntent = data ?: return
        val uri: Uri = resultIntent.data ?: return
        if (requestCode == REQUEST_CHECK_WASM) {
            try {
                val grantFlags = resultIntent.flags and Intent.FLAG_GRANT_READ_URI_PERMISSION
                contentResolver.takePersistableUriPermission(uri, grantFlags)
            } catch (_: SecurityException) {
                // Temporary document grant: validation can still run for this session.
            }
            Thread {
                try {
                    val result = AndroidGameProbe.inspect(this, uri)
                    runOnUiThread {
                        val size = result.sizeBytes?.let { android.text.format.Formatter.formatShortFileSize(this, it) } ?: "size unknown"
                        gameWasmText.text = if (result.validWasmHeader) {
                            "Header valid · " + result.displayName + " · " + size + " · WASM " + result.version + ". Runtime not linked."
                        } else {
                            "Invalid WASM header · " + result.displayName
                        }
                        gameWasmText.setTextColor(if (result.validWasmHeader) 0xFFB8FF35.toInt() else 0xFFFF6B6B.toInt())
                        getSharedPreferences("game_engine", MODE_PRIVATE).edit()
                            .putString("game_wasm_uri", uri.toString())
                            .putBoolean("game_wasm_header_valid", result.validWasmHeader)
                            .apply()
                    }
                } catch (error: Exception) {
                    runOnUiThread { gameWasmText.text = "WASM check failed: " + (error.message ?: "unknown error") }
                }
            }.start()
            return
        }
        if (requestCode != REQUEST_IMPORT_MOD) return
        Thread {
            try {
                val imported = AndroidModStore.importFile(this, uri)
                runOnUiThread {
                    status.text = "Imported " + imported.displayName + " (" + imported.bytes + " bytes). Engine loading is not connected yet."
                    refreshModList()
                }
            } catch (error: Exception) {
                runOnUiThread { status.text = "Mod import failed: " + (error.message ?: "unknown error") }
            }
        }.start()
    }

    private fun refreshModList() {
        try {
            val files = AndroidModStore.list(this)
            modList.text = if (files.isEmpty()) {
                "Imported mods: 0"
            } else {
                "Imported mods: ${files.size}\n" + files.take(8).joinToString("\n") { file ->
                    val kind = when (file.extension.lowercase()) {
                        "zip", "7z", "rar" -> "archive"
                        "oiv" -> "OpenIV package"
                        "asi", "dll", "so" -> "native plugin (not loadable yet)"
                        "rpf" -> "RPF archive (not mounted yet)"
                        else -> "file"
                    }
                    "• ${file.name} — ${android.text.format.Formatter.formatShortFileSize(this, file.length())} — ${kind}"
                } + if (files.size > 8) "\n…and ${files.size - 8} more" else ""
            }
        } catch (error: Exception) {
            modList.text = "Could not read imported mods: ${error.message ?: "unknown error"}"
        }
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
        surfaceReady = true
        renderHandler.post {
            val message = nativeAttachSurface(holder.surface)
            runOnUiThread { if (surfaceReady) status.text = message }
        }
        startFrames()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        renderHandler.post {
            nativeResizeSurface(width, height)
            val message = nativeVulkanStatus()
            runOnUiThread { if (surfaceReady) status.text = message }
        }
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        surfaceReady = false
        stopFrames()
        renderHandler.post { nativeDetachSurface() }
        status.text = "Vulkan surface closed"
    }

    override fun doFrame(frameTimeNanos: Long) {
        if (!rendering || !resumed || !surfaceReady) return
        perfFrameCount++
        val nowMs = SystemClock.elapsedRealtime()
        if (perfWindowStartMs == 0L) perfWindowStartMs = nowMs
        if (nowMs - perfWindowStartMs >= 1000L) {
            val uiFps = perfFrameCount * 1000 / (nowMs - perfWindowStartMs).coerceAtLeast(1L)
            perfFrameCount = 0
            perfWindowStartMs = nowMs
            val memory = ActivityManager.MemoryInfo()
            (getSystemService(ACTIVITY_SERVICE) as ActivityManager).getMemoryInfo(memory)
            val availableMb = memory.availMem / (1024L * 1024L)
            val nativeEstimateMb = nativeAvailableMemoryEstimate() / (1024L * 1024L)
            val totalMb = memory.totalMem / (1024L * 1024L)
            val gpu = Build.HARDWARE.ifBlank { "Adreno / Vulkan" }
            val vulkan = if (status.text.contains("Vulkan swapchain ready", ignoreCase = true)) "READY" else "CHECKING"
            performanceText.text = "RAM ${availableMb}MB system free / ${totalMb}MB  •  process headroom estimate ${nativeEstimateMb}MB  •  GPU $gpu  •  VULKAN $vulkan  •  UI FPS $uiFps"
        }
        // Vulkan waits and driver calls stay off the Android UI thread. Drop a
        // frame instead of queueing work when the GPU/driver is still busy.
        if (framePending.compareAndSet(false, true)) {
            renderHandler.post {
                try {
                    nativeRenderFrame()
                    statusFrameCounter++
                    if (statusFrameCounter >= 30) {
                        statusFrameCounter = 0
                        val latest = nativeVulkanStatus()
                        runOnUiThread {
                            if (surfaceReady && status.text.toString() != latest) status.text = latest
                        }
                    }
                } finally {
                    framePending.set(false)
                }
            }
        }
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
            renderHandler.post { nativeDetachSurface() }
        }
        renderThread.quitSafely()
        super.onDestroy()
    }
}
