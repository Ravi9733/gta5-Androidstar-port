package io.github.ravi9733.gta5port

import android.content.ContentResolver
import android.content.Context
import android.net.Uri
import android.provider.OpenableColumns
import java.io.File
import java.io.FileOutputStream

/**
 * Android counterpart of the iOS Mods import storage.
 * Imported packages are preserved in app-private storage. The current Vulkan
 * bootstrap does not yet contain the GTA engine loader, so storage is not
 * represented as proof that imported mods are active in-game.
 */
object AndroidModStore {
    data class Imported(val file: File, val displayName: String, val bytes: Long)

    fun root(context: Context): File =
        File(context.getExternalFilesDir(null) ?: context.filesDir, "Mods/Imported").apply {
            if (!exists() && !mkdirs()) throw IllegalStateException("Cannot create Mods folder")
        }

    fun importFile(context: Context, uri: Uri): Imported {
        val resolver = context.contentResolver
        val displayName = queryName(resolver, uri) ?: "mod-" + System.currentTimeMillis()
        val destination = uniqueFile(root(context), sanitize(displayName))
        val input = resolver.openInputStream(uri)
            ?: throw IllegalArgumentException("Android could not open the selected file")
        input.use { source -> FileOutputStream(destination).use { output -> source.copyTo(output) } }
        return Imported(destination, displayName, destination.length())
    }

    fun list(context: Context): List<File> =
        root(context).listFiles()?.filter { it.isFile }?.sortedBy { it.name.lowercase() } ?: emptyList()

    private fun queryName(resolver: ContentResolver, uri: Uri): String? {
        resolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) {
                val index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                if (index >= 0) return cursor.getString(index)
            }
        }
        return uri.lastPathSegment?.substringAfterLast('/')
    }

    private fun sanitize(value: String): String {
        val cleaned = value.substringAfterLast('/').substringAfterLast('\\')
            .replace(Regex("[\\\\/:*?\"<>|\\u0000-\\u001F]"), "_").trim().trim('.')
        return cleaned.take(180).ifBlank { "imported-mod" }
    }

    private fun uniqueFile(folder: File, name: String): File {
        val first = File(folder, name)
        if (!first.exists()) return first
        val dot = name.lastIndexOf('.')
        val base = if (dot > 0) name.substring(0, dot) else name
        val ext = if (dot > 0) name.substring(dot) else ""
        var index = 2
        while (true) {
            val candidate = File(folder, base + " (" + index + ")" + ext)
            if (!candidate.exists()) return candidate
            index++
        }
    }
}
