package kantvai.tool.skinsupport.design.app;

import android.content.Context;
import androidx.annotation.NonNull;
import android.util.AttributeSet;
import android.view.View;

import kantvai.tool.skinsupport.app.SkinLayoutInflater;
import kantvai.tool.skinsupport.design.widget.SkinMaterialAppBarLayout;
import kantvai.tool.skinsupport.design.widget.SkinMaterialBottomNavigationView;
import kantvai.tool.skinsupport.design.widget.SkinMaterialCollapsingToolbarLayout;
import kantvai.tool.skinsupport.design.widget.SkinMaterialCoordinatorLayout;
import kantvai.tool.skinsupport.design.widget.SkinMaterialFloatingActionButton;
import kantvai.tool.skinsupport.design.widget.SkinMaterialNavigationView;
import kantvai.tool.skinsupport.design.widget.SkinMaterialTabLayout;
import kantvai.tool.skinsupport.design.widget.SkinMaterialTextInputEditText;
import kantvai.tool.skinsupport.design.widget.SkinMaterialTextInputLayout;

/**
 * Created by ximsfei on 2017/1/13.
 */
public class SkinMaterialViewInflater implements SkinLayoutInflater {
    @Override
    public View createView(@NonNull Context context, final String name, @NonNull AttributeSet attrs) {
        if ("androidx.coordinatorlayout.widget.CoordinatorLayout".equals(name)) {
            return new SkinMaterialCoordinatorLayout(context, attrs);
        }
        if (!name.startsWith("com.google.android.material.")) {
            return null;
        }
        View view = null;
        switch (name) {
            case "com.google.android.material.appbar.AppBarLayout":
                view = new SkinMaterialAppBarLayout(context, attrs);
                break;
            case "com.google.android.material.tabs.TabLayout":
                view = new SkinMaterialTabLayout(context, attrs);
                break;
            case "com.google.android.material.textfield.TextInputLayout":
                view = new SkinMaterialTextInputLayout(context, attrs);
                break;
            case "com.google.android.material.textfield.TextInputEditText":
                view = new SkinMaterialTextInputEditText(context, attrs);
                break;
            case "com.google.android.material.navigation.NavigationView":
                view = new SkinMaterialNavigationView(context, attrs);
                break;
            case "com.google.android.material.floatingactionbutton.FloatingActionButton":
                view = new SkinMaterialFloatingActionButton(context, attrs);
                break;
            case "com.google.android.material.bottomnavigation.BottomNavigationView":
                view = new SkinMaterialBottomNavigationView(context, attrs);
                break;
            case "com.google.android.material.appbar.CollapsingToolbarLayout":
                view = new SkinMaterialCollapsingToolbarLayout(context, attrs);
                break;
            default:
                break;
        }
        return view;
    }
}
