package com.cdeos.kantv.bean;

import com.cdeos.kantv.ui.weight.dialog.FileManagerDialog.OnExtraClickListener;

public class FileManagerExtraItem {
        private String extraText;
        private OnExtraClickListener listener;

        public FileManagerExtraItem(String extraText, OnExtraClickListener listener) {
            this.extraText = extraText;
            this.listener = listener;
        }

        public String getExtraText() {
            return extraText;
        }

        public void setExtraText(String extraText) {
            this.extraText = extraText;
        }

        public OnExtraClickListener getListener() {
            return listener;
        }

        public void setListener(OnExtraClickListener listener) {
            this.listener = listener;
        }
    }