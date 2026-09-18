// SPDX-License-Identifier: GPL-2.0-or-later

package org.dolphinemu.dolphinemu.features.input.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.TextView
import androidx.core.view.setPadding
import com.google.android.material.bottomsheet.BottomSheetBehavior
import com.google.android.material.bottomsheet.BottomSheetDialogFragment
import com.google.android.material.checkbox.MaterialCheckBox
import org.dolphinemu.dolphinemu.R
import org.dolphinemu.dolphinemu.features.input.model.DefaultProfileManager
import java.io.File

class DefaultProfileDialog : BottomSheetDialogFragment() {
    private val family: DefaultProfileManager.ControllerFamily
        get() = requireArguments().getSerializable(ARG_FAMILY)
            as DefaultProfileManager.ControllerFamily
    private lateinit var content: LinearLayout

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        val padding = (20 * resources.displayMetrics.density).toInt()
        content = LinearLayout(requireContext()).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(padding)
        }
        return content
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        rebuild()

        BottomSheetBehavior.from(view.parent as View).state =
            BottomSheetBehavior.STATE_EXPANDED
    }

    private fun rebuild() {
        content.removeAllViews()

        content.addView(TextView(requireContext()).apply {
            text = getString(R.string.input_default_profiles)
            textSize = 22f
        })

        val profiles = DefaultProfileManager.getProfiles(family)
        if (profiles.isEmpty()) {
            content.addView(TextView(requireContext()).apply {
                text = getString(R.string.input_default_profile_configure_first)
            })
            return
        }

        profiles.forEach { profile ->
            val savedDevice = DefaultProfileManager.getProfileDevice(profile)
            val isDefault = DefaultProfileManager.isDefaultProfile(profile.path)
            val hasOtherDefault = savedDevice != null &&
                DefaultProfileManager.hasDefaultProfileForDevice(savedDevice, profile.path, family)

            content.addView(MaterialCheckBox(requireContext()).apply {
                text = buildProfileLabel(profile, savedDevice)
                isChecked = isDefault
                isEnabled = savedDevice != null && (isDefault || !hasOtherDefault)
                setOnCheckedChangeListener { _, checked ->
                    DefaultProfileManager.setDefaultProfile(profile.path, checked, family)
                    DefaultProfileManager.applyDefaultProfiles(family)
                    rebuild()
                }
            })
        }
    }

    private fun buildProfileLabel(profile: File, device: String?): String {
        val name = DefaultProfileManager.profileName(profile)
        if (device == null)
            return name

        return getString(
            R.string.input_default_profile_mapping,
            name,
            DefaultProfileManager.deviceDisplayName(device)
        )
    }

    companion object {
        private const val ARG_FAMILY = "family"

        fun create(family: DefaultProfileManager.ControllerFamily) =
            DefaultProfileDialog().apply {
                arguments = Bundle().apply {
                    putSerializable(ARG_FAMILY, family)
                }
            }
    }
}
