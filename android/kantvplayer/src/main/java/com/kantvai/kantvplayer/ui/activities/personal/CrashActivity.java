package com.kantvai.kantvplayer.ui.activities.personal;

import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.ui.weight.dialog.CrashDialog;

import cat.ereza.customactivityoncrash.CustomActivityOnCrash;
import cat.ereza.customactivityoncrash.config.CaocConfig;

public class CrashActivity extends AppCompatActivity {

    private CaocConfig mCrashConfig;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_crash);

        mCrashConfig = CustomActivityOnCrash.getConfigFromIntent(getIntent());
        if (mCrashConfig == null) {
            finish();
        }

        initListener();
    }

    private void initListener() {
        findViewById(R.id.crash_restart_bt).setOnClickListener(v ->
                CustomActivityOnCrash.restartApplication(CrashActivity.this, mCrashConfig));

        findViewById(R.id.crash_log_bt).setOnClickListener(v -> {
            String crashLog = CustomActivityOnCrash.getAllErrorDetailsFromIntent(CrashActivity.this, getIntent());
            new CrashDialog(this, crashLog).show();
        });

    }
}
