package kantvai.tool.skinsupport.app;

import android.graphics.drawable.Drawable;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.LayoutInflaterCompat;
import androidx.appcompat.app.AppCompatActivity;

import kantvai.tool.skinsupport.SkinCompatManager;
import kantvai.tool.skinsupport.content.res.SkinCompatThemeUtils;
import kantvai.tool.skinsupport.content.res.SkinCompatVectorResources;
import kantvai.tool.skinsupport.observe.SkinObservable;
import kantvai.tool.skinsupport.observe.SkinObserver;

import static kantvai.tool.skinsupport.widget.SkinCompatHelper.INVALID_ID;
import static kantvai.tool.skinsupport.widget.SkinCompatHelper.checkResourceId;

/**
 * Created by ximsfei on 17-1-8.
 */
@Deprecated
public class SkinCompatActivity extends AppCompatActivity implements SkinObserver {

    private SkinCompatDelegate mSkinDelegate;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        LayoutInflaterCompat.setFactory2(getLayoutInflater(), getSkinDelegate());
        super.onCreate(savedInstanceState);
        updateStatusBarColor();
        updateWindowBackground();
    }

    @NonNull
    public SkinCompatDelegate getSkinDelegate() {
        if (mSkinDelegate == null) {
            mSkinDelegate = SkinCompatDelegate.create(this);
        }
        return mSkinDelegate;
    }

    @Override
    protected void onResume() {
        super.onResume();
        SkinCompatManager.getInstance().addObserver(this);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        SkinCompatManager.getInstance().deleteObserver(this);
    }

    protected void updateStatusBarColor() {
    }

    protected void updateWindowBackground() {
        int windowBackgroundResId = SkinCompatThemeUtils.getWindowBackgroundResId(this);
        if (checkResourceId(windowBackgroundResId) != INVALID_ID) {
            Drawable drawable = SkinCompatVectorResources.getDrawableCompat(this, windowBackgroundResId);
            if (drawable != null) {
                getWindow().setBackgroundDrawable(drawable);
            }
        }
    }

    @Override
    public void updateSkin(SkinObservable observable, Object o) {
        updateStatusBarColor();
        updateWindowBackground();
        getSkinDelegate().applySkin();
    }
}
