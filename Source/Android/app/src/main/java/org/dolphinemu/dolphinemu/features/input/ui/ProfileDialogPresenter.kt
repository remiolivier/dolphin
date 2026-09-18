// SPDX-License-Identifier: GPL-2.0-or-later

package org.dolphinemu.dolphinemu.features.input.ui

import android.content.Context
import android.content.DialogInterface
import android.view.LayoutInflater
import androidx.fragment.app.DialogFragment
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.dolphinemu.dolphinemu.R
import org.dolphinemu.dolphinemu.databinding.DialogInputStringBinding
import org.dolphinemu.dolphinemu.features.input.model.DefaultProfileManager
import org.dolphinemu.dolphinemu.features.settings.ui.MenuTag
import org.dolphinemu.dolphinemu.features.settings.ui.SettingsActivityView
import java.io.File
import java.util.Locale

class ProfileDialogPresenter {
    private val context: Context?
    private val dialog: DialogFragment?
    private val menuTag: MenuTag

    constructor(menuTag: MenuTag) {
        context = null
        dialog = null
        this.menuTag = menuTag
    }

    constructor(dialog: DialogFragment, menuTag: MenuTag) {
        context = dialog.context
        this.dialog = dialog
        this.menuTag = menuTag
    }

    fun getProfileNames(stock: Boolean): Array<String> {
        val profiles = File(getProfileDirectoryPath(stock)).listFiles { file: File ->
            !file.isDirectory && file.name.endsWith(EXTENSION)
        } ?: return emptyArray()

        return profiles.map { it.name.substring(0, it.name.length - EXTENSION.length) }
            .sortedBy { it.lowercase(Locale.getDefault()) }
            .toTypedArray()
    }

    fun loadProfile(profileName: String, stock: Boolean) {
        val controller = menuTag.correspondingEmulatedController
        val profilePath = getProfilePath(profileName, stock)

        if (!controller.hasMappings()) {
            controller.loadProfile(profilePath, true)
            (dialog!!.requireActivity() as SettingsActivityView).onControllerSettingsChanged()
            dialog.dismiss()
            return
        }

        MaterialAlertDialogBuilder(context!!)
            .setMessage(context.getString(R.string.input_profile_existing_mappings, profileName))
            .setPositiveButton(R.string.input_profile_replace_mappings) { _, _ ->
                controller.loadProfile(profilePath, true)
                (dialog!!.requireActivity() as SettingsActivityView).onControllerSettingsChanged()
                dialog.dismiss()
            }
            .setNeutralButton(R.string.input_profile_keep_existing) { _, _ ->
                controller.loadProfile(profilePath, false)
                (dialog!!.requireActivity() as SettingsActivityView).onControllerSettingsChanged()
                dialog.dismiss()
            }
            .setNegativeButton(android.R.string.cancel, null)
            .show()
    }

    fun saveProfile(profileName: String) {
        val profilePath = getProfilePath(profileName, false)
        if (!File(profilePath).exists()) {
            menuTag.correspondingEmulatedController.saveProfile(profilePath)
            dialog!!.dismiss()
        } else {
            MaterialAlertDialogBuilder(context!!)
                .setMessage(context.getString(R.string.input_profile_confirm_save, profileName))
                .setPositiveButton(R.string.yes) { _: DialogInterface?, _: Int ->
                    menuTag.correspondingEmulatedController.saveProfile(profilePath)
                    dialog!!.dismiss()
                }
                .setNegativeButton(R.string.no, null)
                .show()
        }
    }

    fun saveProfileAndPromptForName() {
        val inflater = LayoutInflater.from(context)
        val binding = DialogInputStringBinding.inflate(inflater)
        val input = binding.input

        MaterialAlertDialogBuilder(context!!)
            .setView(binding.root)
            .setPositiveButton(android.R.string.ok) { _: DialogInterface?, _: Int ->
                saveProfile(input.text.toString())
            }
            .setNegativeButton(android.R.string.cancel, null)
            .show()
    }

    fun deleteProfile(profileName: String) {
        MaterialAlertDialogBuilder(context!!)
            .setMessage(context.getString(R.string.input_profile_confirm_delete, profileName))
            .setPositiveButton(R.string.yes) { _: DialogInterface?, _: Int ->
                val profilePath = getProfilePath(profileName, false)
                DefaultProfileManager.removeDefaultProfile(profilePath)
                File(profilePath).delete()
                dialog!!.dismiss()
            }
            .setNegativeButton(R.string.no, null)
            .show()
    }

    private fun getProfileDirectoryPath(stock: Boolean): String {
        val controller = menuTag.correspondingEmulatedController
        return if (stock) {
            controller.getSysProfileDirectoryPath()
        } else {
            controller.getUserProfileDirectoryPath()
        }
    }

    private fun getProfilePath(profileName: String, stock: Boolean): String =
        getProfileDirectoryPath(stock) + profileName + EXTENSION

    companion object {
        private const val EXTENSION = ".ini"
    }
}
