package kantvai.tool.skinsupport.app;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.TypedArray;
import android.os.Build;

import androidx.core.view.ViewCompat;
import androidx.appcompat.view.ContextThemeWrapper;
import androidx.appcompat.widget.TintContextWrapper;
import androidx.appcompat.widget.VectorEnabledTintResources;

import android.util.AttributeSet;
import android.view.View;
import android.view.ViewParent;

import com.kantvai.kantvplayerlib.R;

import kantvai.tool.skinsupport.content.res.SkinCompatVectorResources;
import kantvai.tool.skinsupport.utils.Slog;
import kantvai.tool.skinsupport.widget.SkinCompatAutoCompleteTextView;
import kantvai.tool.skinsupport.widget.SkinCompatButton;
import kantvai.tool.skinsupport.widget.SkinCompatCheckBox;
import kantvai.tool.skinsupport.widget.SkinCompatCheckedTextView;
import kantvai.tool.skinsupport.widget.SkinCompatEditText;
import kantvai.tool.skinsupport.widget.SkinCompatFrameLayout;
import kantvai.tool.skinsupport.widget.SkinCompatImageButton;
import kantvai.tool.skinsupport.widget.SkinCompatImageView;
import kantvai.tool.skinsupport.widget.SkinCompatLinearLayout;
import kantvai.tool.skinsupport.widget.SkinCompatMultiAutoCompleteTextView;
import kantvai.tool.skinsupport.widget.SkinCompatProgressBar;
import kantvai.tool.skinsupport.widget.SkinCompatRadioButton;
import kantvai.tool.skinsupport.widget.SkinCompatRadioGroup;
import kantvai.tool.skinsupport.widget.SkinCompatRatingBar;
import kantvai.tool.skinsupport.widget.SkinCompatRelativeLayout;
import kantvai.tool.skinsupport.widget.SkinCompatScrollView;
import kantvai.tool.skinsupport.widget.SkinCompatSeekBar;
import kantvai.tool.skinsupport.widget.SkinCompatSpinner;
import kantvai.tool.skinsupport.widget.SkinCompatTextView;
import kantvai.tool.skinsupport.widget.SkinCompatToolbar;
import kantvai.tool.skinsupport.widget.SkinCompatView;

public class SkinAppCompatViewInflater implements SkinLayoutInflater, SkinWrapper {
    private static final String LOG_TAG = "SkinAppCompatViewInflater";

    public SkinAppCompatViewInflater() {
        SkinCompatVectorResources.getInstance();
    }

    @Override
    public View createView(Context context, String name, AttributeSet attrs) {
        View view = createViewFromFV(context, name, attrs);

        if (view == null) {
            view = createViewFromV7(context, name, attrs);
        }
        return view;
    }

    private View createViewFromFV(Context context, String name, AttributeSet attrs) {
        View view = null;
        if (name.contains(".")) {
            return null;
        }
        switch (name) {
            case "View":
                view = new SkinCompatView(context, attrs);
                break;
            case "LinearLayout":
                view = new SkinCompatLinearLayout(context, attrs);
                break;
            case "RelativeLayout":
                view = new SkinCompatRelativeLayout(context, attrs);
                break;
            case "FrameLayout":
                view = new SkinCompatFrameLayout(context, attrs);
                break;
            case "TextView":
                view = new SkinCompatTextView(context, attrs);
                break;
            case "ImageView":
                view = new SkinCompatImageView(context, attrs);
                break;
            case "Button":
                view = new SkinCompatButton(context, attrs);
                break;
            case "EditText":
                view = new SkinCompatEditText(context, attrs);
                break;
            case "Spinner":
                view = new SkinCompatSpinner(context, attrs);
                break;
            case "ImageButton":
                view = new SkinCompatImageButton(context, attrs);
                break;
            case "CheckBox":
                view = new SkinCompatCheckBox(context, attrs);
                break;
            case "RadioButton":
                view = new SkinCompatRadioButton(context, attrs);
                break;
            case "RadioGroup":
                view = new SkinCompatRadioGroup(context, attrs);
                break;
            case "CheckedTextView":
                view = new SkinCompatCheckedTextView(context, attrs);
                break;
            case "AutoCompleteTextView":
                view = new SkinCompatAutoCompleteTextView(context, attrs);
                break;
            case "MultiAutoCompleteTextView":
                view = new SkinCompatMultiAutoCompleteTextView(context, attrs);
                break;
            case "RatingBar":
                view = new SkinCompatRatingBar(context, attrs);
                break;
            case "SeekBar":
                view = new SkinCompatSeekBar(context, attrs);
                break;
            case "ProgressBar":
                view = new SkinCompatProgressBar(context, attrs);
                break;
            case "ScrollView":
                view = new SkinCompatScrollView(context, attrs);
                break;
            default:
                break;
        }
        return view;
    }

    private View createViewFromV7(Context context, String name, AttributeSet attrs) {
        View view = null;
        switch (name) {
            case "androidx.appcompat.widget.Toolbar":
                view = new SkinCompatToolbar(context, attrs);
                break;
            default:
                break;
        }
        return view;
    }

    @SuppressLint("RestrictedApi")
    @Override
    public Context wrapContext(Context context, View parent, AttributeSet attrs) {
        final boolean isPre21 = Build.VERSION.SDK_INT < 21;

        // We only want the View to inherit its context if we're running pre-v21
        final boolean inheritContext = isPre21 && shouldInheritContext(context, (ViewParent) parent);

        // We can emulate Lollipop's android:theme attribute propagating down the view hierarchy
        // by using the parent's context
        if (inheritContext && parent != null) {
            context = parent.getContext();
        }
        boolean readAndroidTheme = isPre21; /* Only read android:theme pre-L (L+ handles this anyway) */
        boolean readAppTheme = true; /* Read read app:theme as a fallback at all times for legacy reasons */
        @SuppressLint("RestrictedApi") boolean wrapContext = VectorEnabledTintResources.shouldBeUsed(); /* Only tint wrap the context if enabled */

        // We can emulate Lollipop's android:theme attribute propagating down the view hierarchy
        // by using the parent's context
        if (inheritContext && parent != null) {
            context = parent.getContext();
        }
        if (readAndroidTheme || readAppTheme) {
            // We then apply the theme on the context, if specified
            context = themifyContext(context, attrs, readAndroidTheme, readAppTheme);
        }
        if (wrapContext) {
            context = TintContextWrapper.wrap(context);
        }
        return context;
    }

    private boolean shouldInheritContext(Context context, ViewParent parent) {
        if (parent == null) {
            // The initial parent is null so just return false
            return false;
        }
        if (context instanceof Activity) {
            final View windowDecor = ((Activity) context).getWindow().getDecorView();
            while (true) {
                if (parent == null) {
                    // Bingo. We've hit a view which has a null parent before being terminated from
                    // the loop. This is (most probably) because it's the root view in an inflation
                    // call, therefore we should inherit. This works as the inflated layout is only
                    // added to the hierarchy at the end of the inflate() call.
                    return true;
                } else if (parent == windowDecor || !(parent instanceof View)
                        || ViewCompat.isAttachedToWindow((View) parent)) {
                    // We have either hit the window's decor view, a parent which isn't a View
                    // (i.e. ViewRootImpl), or an attached view, so we know that the original parent
                    // is currently added to the view hierarchy. This means that it has not be
                    // inflated in the current inflate() call and we should not inherit the context.
                    return false;
                }
                parent = parent.getParent();
            }
        }
        return false;
    }

    /**
     * Allows us to emulate the {@code android:theme} attribute for devices before L.
     */
    private static Context themifyContext(Context context, AttributeSet attrs,
                                          boolean useAndroidTheme, boolean useAppTheme) {
        final TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.View, 0, 0);
        int themeId = 0;
        if (useAndroidTheme) {
            // First try reading android:theme if enabled
            themeId = a.getResourceId(com.kantvai.kantvplayerlib.R.styleable.View_android_theme, 0);
        }
        if (useAppTheme && themeId == 0) {
            // ...if that didn't work, try reading app:theme (for legacy reasons) if enabled
            themeId = a.getResourceId(R.styleable.View_theme, 0);

            if (themeId != 0) {
                Slog.i(LOG_TAG, "app:theme is now deprecated. "
                        + "Please move to using android:theme instead.");
            }
        }
        a.recycle();

        if (themeId != 0 && (!(context instanceof ContextThemeWrapper)
                || ((ContextThemeWrapper) context).getThemeResId() != themeId)) {
            // If the context isn't a ContextThemeWrapper, or it is but does not have
            // the same theme as we need, wrap it in a new wrapper
            context = new ContextThemeWrapper(context, themeId);
        }
        return context;
    }

}
