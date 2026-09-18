// SPDX-License-Identifier: GPL-2.0-or-later

package org.dolphinemu.dolphinemu.features.input.model

import org.dolphinemu.dolphinemu.features.input.model.controlleremu.EmulatedController
import java.io.File

object DefaultProfileManager {
    enum class ControllerFamily {
        GAMECUBE,
        WIIMOTE
    }

    fun isDefaultProfile(profilePath: String): Boolean = isDefaultProfileNative(profilePath)

    fun setDefaultProfile(profilePath: String, enabled: Boolean, family: ControllerFamily): Boolean =
        setDefaultProfileNative(profilePath, enabled, family.ordinal)

    fun removeDefaultProfile(profilePath: String): Boolean =
        removeDefaultProfileNative(profilePath)

    fun getProfiles(family: ControllerFamily): List<File> =
        getProfilePathsNative(family.ordinal).map(::File).sortedBy { it.name.lowercase() }

    fun getProfileDevice(profile: File): String? = getProfileDeviceNative(profile.path)

    fun hasDefaultProfileForDevice(
        device: String,
        excludingProfilePath: String,
        family: ControllerFamily
    ): Boolean = hasDefaultProfileForDeviceNative(
        device, excludingProfilePath, family.ordinal
    )

    fun applyDefaultProfiles(family: ControllerFamily) {
        applyDefaultProfilesNative(family.ordinal)
    }

    fun applyDefaultProfileToController(
        controller: EmulatedController,
        device: String,
        family: ControllerFamily
    ): Boolean = applyDefaultProfileToControllerNative(
        controller, device, family.ordinal
    )

    fun profileName(profile: File): String = profile.nameWithoutExtension

    fun deviceDisplayName(device: String): String {
        val secondSlash = device.indexOf('/', device.indexOf('/') + 1)
        return if (secondSlash >= 0 && secondSlash + 1 < device.length)
            device.substring(secondSlash + 1)
        else
            device
    }

    private external fun getProfilePathsNative(family: Int): Array<String>
    private external fun getProfileDeviceNative(profilePath: String): String?
    private external fun isDefaultProfileNative(profilePath: String): Boolean
    private external fun removeDefaultProfileNative(profilePath: String): Boolean
    private external fun setDefaultProfileNative(
        profilePath: String, enabled: Boolean, family: Int
    ): Boolean
    private external fun hasDefaultProfileForDeviceNative(
        device: String, excludingProfilePath: String, family: Int
    ): Boolean
    private external fun applyDefaultProfilesNative(family: Int)
    private external fun applyDefaultProfileToControllerNative(
        controller: EmulatedController, device: String, family: Int
    ): Boolean
}
