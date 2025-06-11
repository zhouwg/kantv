package kantvai.tool.skinsupport.design.widget;

import android.content.Context;
import com.google.android.material.appbar.AppBarLayout;
import android.util.AttributeSet;

import kantvai.tool.skinsupport.widget.SkinCompatBackgroundHelper;
import kantvai.tool.skinsupport.widget.SkinCompatSupportable;

/**
 * Created by ximsfei on 2017/1/13.
 */

public class SkinMaterialAppBarLayout extends AppBarLayout implements SkinCompatSupportable {
    private SkinCompatBackgroundHelper mBackgroundTintHelper;

    public SkinMaterialAppBarLayout(Context context) {
        this(context, null);
    }

    public SkinMaterialAppBarLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        mBackgroundTintHelper = new SkinCompatBackgroundHelper(this);
        mBackgroundTintHelper.loadFromAttributes(attrs, 0);
    }

    @Override
    public void applySkin() {
        if (mBackgroundTintHelper != null) {
            mBackgroundTintHelper.applySkin();
        }
    }

}
