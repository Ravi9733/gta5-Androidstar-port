package io.github.ravi9733.gta5port

import android.content.Context
import android.net.Uri
import android.provider.OpenableColumns

/**
 * Lightweight validation of the user-supplied Five Star Port game.wasm.
 * This checks the WebAssembly header only; it does not execute the game.
 */
object AndroidGameProbe {
    data class Result(
        val displayName: String,
        val sizeBytes: Long?,
        val validWasmHeader: Boolean,
        val version: String
    )

    fun inspect(context: Context, uri: Uri): Result {
        var name = uri.lastPathSegment?.substringAfterLast('/') ?: "game.wasm"
        var size: Long? = null
        context.contentResolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE), null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) {
                val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                val sizeIndex = cursor.getColumnIndex(OpenableColumns.SIZE)
                if (nameIndex >= 0) name = cursor.getString(nameIndex) ?: name
                if (sizeIndex >= 0 && !cursor.isNull(sizeIndex)) size = cursor.getLong(sizeIndex)
            }
        }
        val header = ByteArray(8)
        var count = 0
        context.contentResolver.openInputStream(uri)?.use { input ->
            while (count < header.size) {
                val read = input.read(header, count, header.size - count)
                if (read <= 0) break
                count += read
            }
        } ?: throw IllegalArgumentException("Android could not open the selected file")
        val valid = count == 8 &&
            header[0] == 0x00.toByte() && header[1] == 0x61.toByte() &&
            header[2] == 0x73.toByte() && header[3] == 0x6d.toByte() &&
            header[4] == 0x01.toByte() && header[5] == 0x00.toByte() &&
            header[6] == 0x00.toByte() && header[7] == 0x00.toByte()
        val version = if (count == 8) {
            (header[4].toInt() and 0xff).toString() + "." +
                (header[5].toInt() and 0xff) + "." +
                (header[6].toInt() and 0xff) + "." +
                (header[7].toInt() and 0xff)
        } else "unknown"
        return Result(name, size, valid, version)
    }
}
