/*
 * Copyright (c) 2024- KanTV Authors
 */
package com.kantvai.kantvplayer.ui.fragment.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TextView;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import com.kantvai.kantvplayer.R;

/**
 * Custom Preference used as a non-selectable header for the LLM Setting page.
 *
 * Renders {@code preference_llm_setting_header.xml} and exposes setter
 * methods that {@link LLMSettingFragment} can call to refresh device info,
 * selected-model info, and resident-status text.
 *
 * The setters cache the values so the data is re-applied every time the
 * RecyclerView binds the view (important because the view holder is reused
 * for other preferences after scrolling).
 */
public class LLMSettingHeaderPreference extends Preference {
    private TextView txtDeviceInfo;
    private TextView txtModelInfo;
    private TextView txtResidentStatus;

    private String cachedDeviceInfo;
    private String cachedModelInfo;
    private String cachedResidentStatus;

    public LLMSettingHeaderPreference(Context context) {
        super(context);
        setLayoutResource(R.layout.preference_llm_setting_header);
    }

    public LLMSettingHeaderPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setLayoutResource(R.layout.preference_llm_setting_header);
    }

    public LLMSettingHeaderPreference(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        setLayoutResource(R.layout.preference_llm_setting_header);
    }

    public LLMSettingHeaderPreference(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        setLayoutResource(R.layout.preference_llm_setting_header);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        // Make the whole row non-clickable so it behaves as a static header.
        View itemView = holder.itemView;
        if (itemView != null) {
            itemView.setClickable(false);
            itemView.setFocusable(false);
        }
        txtDeviceInfo = (TextView) holder.findViewById(R.id.llmHeaderDeviceInfo);
        txtModelInfo = (TextView) holder.findViewById(R.id.llmHeaderModelInfo);
        txtResidentStatus = (TextView) holder.findViewById(R.id.llmHeaderResidentStatus);

        // Re-apply cached values so recycled holders get the right data.
        if (cachedDeviceInfo != null) {
            txtDeviceInfo.setText(cachedDeviceInfo);
        }
        if (cachedModelInfo != null) {
            txtModelInfo.setText(cachedModelInfo);
        }
        if (cachedResidentStatus != null) {
            txtResidentStatus.setText(cachedResidentStatus);
        }
    }

    public void setDeviceInfo(String text) {
        cachedDeviceInfo = text;
        if (txtDeviceInfo != null && text != null) {
            txtDeviceInfo.setText(text);
        }
    }

    public void setModelInfo(String text) {
        cachedModelInfo = text;
        if (txtModelInfo != null && text != null) {
            txtModelInfo.setText(text);
        }
    }

    public void setResidentStatus(String text) {
        cachedResidentStatus = text;
        if (txtResidentStatus != null && text != null) {
            txtResidentStatus.setText(text);
        }
    }
}
