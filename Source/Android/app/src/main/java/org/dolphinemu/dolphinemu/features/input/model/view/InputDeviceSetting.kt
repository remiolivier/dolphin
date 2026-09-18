// SPDX-License-Identifier: GPL-2.0-or-later

package org.dolphinemu.dolphinemu.features.input.model.view

import android.content.Context
import org.dolphinemu.dolphinemu.features.input.model.ControllerInterface
import org.dolphinemu.dolphinemu.features.input.model.DefaultProfileManager
import org.dolphinemu.dolphinemu.features.input.model.controlleremu.EmulatedController
import org.dolphinemu.dolphinemu.features.settings.model.Settings
import org.dolphinemu.dolphinemu.features.settings.model.view.StringSingleChoiceSetting

class InputDeviceSetting(
    context: Context,
    titleId: Int,
    descriptionId: Int,
    private val controller: EmulatedController,
    private val family: DefaultProfileManager.ControllerFamily,
    private val onProfileLoaded: () -> Unit = {}
) : StringSingleChoiceSetting(context, null, titleId, descriptionId, arrayOf(), arrayOf(), null) {
    init {
        refreshChoicesAndValues()
    }

    override val selectedChoice: String
        get() = controller.getDefaultDevice()

    override val selectedValue: String
        get() = controller.getDefaultDevice()

    override fun setSelectedValue(settings: Settings, selection: String) {
        if (DefaultProfileManager.applyDefaultProfileToController(controller, selection, family))
            onProfileLoaded()
        else
            controller.setDefaultDevice(selection)
    }

    override fun refreshChoicesAndValues() {
        val devices = ControllerInterface.getAllDeviceStrings()

        choices = devices
        values = devices
    }

    override val isEditable: Boolean = true

    override fun canClear(): Boolean = true

    override fun clear(settings: Settings) = setSelectedValue(settings, "")
}
