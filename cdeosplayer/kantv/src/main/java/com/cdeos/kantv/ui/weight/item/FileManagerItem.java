package com.cdeos.kantv.ui.weight.item;

import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.cdeos.kantv.R;
import com.cdeos.kantv.bean.FileManagerBean;
import com.cdeos.kantv.ui.weight.dialog.FileManagerDialog;
import com.cdeos.kantv.utils.interf.AdapterItem;

import java.io.File;

import butterknife.BindView;


public class FileManagerItem implements AdapterItem<FileManagerBean> {
    @BindView(R.id.iv)
    ImageView iv;
    @BindView(R.id.tv)
    TextView tv;

    private View mView;
    private FileManagerDialog.OnItemClickListener listener;

    public FileManagerItem(FileManagerDialog.OnItemClickListener listener){
        this.listener = listener;
    }

    @Override
    public int getLayoutResId() {
        return R.layout.item_file_manager;
    }

    @Override
    public void initItemViews(View itemView) {
        mView = itemView;
    }

    @Override
    public void onSetViews() {

    }

    @Override
    public void onUpdateViews(final FileManagerBean model, int position) {
        if (model.getFile() == null){
            mView.setVisibility(View.GONE);
            return;
        }

        if (model.isFolder() && model.hasParent()){
            iv.setImageResource(R.drawable.ic_chevron_left_dark);
        }else if (model.isFolder() && !model.hasParent()){
            iv.setImageResource(R.drawable.ic_folder_dark);
        }else {
            iv.setImageResource(R.drawable.ic_xml_file);
        }

        tv.setText(model.getName());

        mView.setOnClickListener(v -> {
            if (model.hasParent()){
                File parentFile = model.getFile().getParentFile();
                if (parentFile != null)
                    listener.onItemClick(parentFile.getAbsolutePath(), true);
            }else if(model.isFolder()){
                listener.onItemClick(model.getFile().getAbsolutePath(), true);
            }else {
                listener.onItemClick(model.getFile().getAbsolutePath(), false);
            }
        });
    }
}
