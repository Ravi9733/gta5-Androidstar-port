package com.fivestarport.android.mods

import android.content.ContentResolver
import android.net.Uri
import org.json.JSONObject
import java.io.BufferedInputStream
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.util.UUID
import java.util.zip.ZipInputStream

/**
 * Android-native first stage of the Five Star Port mod installer.
 *
 * Imports ZIP files selected through Android's Storage Access Framework.
 * It preserves PC game-relative paths, but does not itself patch or hook the game.
 */
class AndroidModImporter(
    private val resolver: ContentResolver,
    private val modsRoot: File,
    private val limits: Limits = Limits()
) {
    data class Limits(
        val maxEntries: Int = 100_000,
        val maxEntryBytes: Long = 4L * 1024 * 1024 * 1024,
        val maxTotalBytes: Long = 12L * 1024 * 1024 * 1024
    )

    data class ImportedMod(
        val id: String,
        val displayName: String,
        val directory: File,
        val fileCount: Int,
        val unpackedBytes: Long
    )

    fun importZip(uri: Uri, displayName: String): ImportedMod {
        require(uri.scheme == "content" || uri.scheme == "file") {
            "Choose a mod archive through Android's file picker."
        }
        if (!modsRoot.exists() && !modsRoot.mkdirs()) {
            throw IOException("Cannot create the private Mods directory.")
        }

        val id = UUID.randomUUID().toString()
        val staging = File(modsRoot, ".staging-$id")
        val finalDir = File(modsRoot, safeFolderName(displayName, id))
        if (!staging.mkdirs()) throw IOException("Cannot create mod staging directory.")

        var count = 0
        var total = 0L
        try {
            val input = resolver.openInputStream(uri)
                ?: throw IOException("Android could not open the selected archive.")
            ZipInputStream(BufferedInputStream(input)).use { zip ->
                while (true) {
                    val entry = zip.nextEntry ?: break
                    count++
                    if (count > limits.maxEntries) throw IOException("Archive contains too many entries.")

                    val relative = safeRelativePath(entry.name)
                    if (relative.isEmpty()) {
                        zip.closeEntry()
                        continue
                    }
                    val target = File(staging, relative)
                    val canonicalRoot = staging.canonicalFile.toPath()
                    val canonicalTarget = target.canonicalFile.toPath()
                    if (!canonicalTarget.startsWith(canonicalRoot)) {
                        throw IOException("Archive entry escapes the Mods directory.")
                    }

                    if (entry.isDirectory) {
                        if (!target.exists() && !target.mkdirs()) throw IOException("Cannot create mod folder.")
                    } else {
                        target.parentFile?.let {
                            if (!it.exists() && !it.mkdirs()) throw IOException("Cannot create mod folder.")
                        }
                        var entryBytes = 0L
                        FileOutputStream(target).use { output ->
                            val buffer = ByteArray(64 * 1024)
                            while (true) {
                                val read = zip.read(buffer)
                                if (read < 0) break
                                entryBytes += read
                                total += read
                                if (entryBytes > limits.maxEntryBytes || total > limits.maxTotalBytes) {
                                    throw IOException("Archive exceeds the safe extraction size limit.")
                                }
                                output.write(buffer, 0, read)
                            }
                        }
                    }
                    zip.closeEntry()
                }
            }

            val manifest = JSONObject()
                .put("schemaVersion", 1)
                .put("id", id)
                .put("name", displayName.trim().ifEmpty { "Imported mod" })
                .put("enabled", false)
                .put("sourceFormat", "zip")
                .put("fileCount", count)
                .put("unpackedBytes", total)
                .put("gamePathConvention", "pc-relative")
            File(staging, "android-mod.json").writeText(manifest.toString(2), Charsets.UTF_8)

            if (finalDir.exists()) throw IOException("A mod with this generated folder name already exists.")
            if (!staging.renameTo(finalDir)) throw IOException("Could not finalize the imported mod.")
            return ImportedMod(id, displayName, finalDir, count, total)
        } catch (failure: Exception) {
            staging.deleteRecursively()
            throw failure
        }
    }

    private fun safeRelativePath(raw: String): String {
        val normalized = raw.replace('\\', '/')
        if (normalized.startsWith('/') || normalized.startsWith("//") || normalized.contains('\u0000')) {
            throw IOException("Archive contains an absolute or invalid path.")
        }
        val parts = normalized.split('/')
        if (parts.any { it == ".." || it == "." || it.contains(':') }) {
            throw IOException("Archive contains an unsafe path.")
        }
        return parts.filter { it.isNotBlank() }.joinToString(File.separator)
    }

    private fun safeFolderName(name: String, id: String): String {
        val cleaned = name.trim()
            .replace(Regex("[^A-Za-z0-9._ -]"), "_")
            .trim('.', ' ')
            .take(48)
            .ifEmpty { "Mod" }
        return "$cleaned-$id"
    }
}
