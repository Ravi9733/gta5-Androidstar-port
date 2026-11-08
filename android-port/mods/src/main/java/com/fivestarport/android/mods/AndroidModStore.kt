package com.fivestarport.android.mods

import org.json.JSONObject
import java.io.File
import java.io.IOException

/**
 * Reads and manages imported mod packages in the app-private Mods directory.
 * Enabling a mod only changes metadata; the game runtime must still consume
 * the enabled directories through its native asset-overlay implementation.
 */
class AndroidModStore(private val modsRoot: File) {

    data class Mod(
        val id: String,
        val name: String,
        val directory: File,
        val enabled: Boolean,
        val fileCount: Int,
        val unpackedBytes: Long
    )

    fun list(): List<Mod> {
        if (!modsRoot.isDirectory) return emptyList()
        return modsRoot.listFiles()
            ?.asSequence()
            ?.filter { it.isDirectory && !it.name.startsWith(".staging-") }
            ?.mapNotNull { readMod(it) }
            ?.sortedBy { it.name.lowercase() }
            ?.toList()
            ?: emptyList()
    }

    fun setEnabled(id: String, enabled: Boolean) {
        val mod = findById(id) ?: throw IOException("Mod not found.")
        val manifestFile = File(mod, "android-mod.json")
        val manifest = readManifest(manifestFile)
        manifest.put("enabled", enabled)
        manifestFile.writeText(manifest.toString(2), Charsets.UTF_8)
    }

    fun remove(id: String) {
        val mod = findById(id) ?: throw IOException("Mod not found.")
        if (!mod.deleteRecursively()) throw IOException("Could not remove the mod.")
    }

    private fun findById(id: String): File? =
        list().firstOrNull { it.id == id }?.directory

    private fun readMod(directory: File): Mod? {
        val manifestFile = File(directory, "android-mod.json")
        val json = try { readManifest(manifestFile) } catch (_: Exception) { return null }
        val id = json.optString("id").takeIf { it.isNotBlank() } ?: return null
        return Mod(
            id = id,
            name = json.optString("name", directory.name),
            directory = directory,
            enabled = json.optBoolean("enabled", false),
            fileCount = json.optInt("fileCount", 0),
            unpackedBytes = json.optLong("unpackedBytes", 0L)
        )
    }

    private fun readManifest(file: File): JSONObject {
        if (!file.isFile) throw IOException("Mod manifest is missing.")
        return JSONObject(file.readText(Charsets.UTF_8))
    }
}
