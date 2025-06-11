 /*
  * Copyright (c) Project KanTV. 2021-2023
  *
  * Copyright (c) 2024- KanTV Authors
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to
  * deal in the Software without restriction, including without limitation the
  * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
  * sell copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  * IN THE SOFTWARE.
  */
package kantvai.media.player;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.text.InputType;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import java.io.IOException;
import java.net.URI;
import java.net.URL;
import java.util.ArrayList;

import kantvai.media.player.KANTVLog;


public abstract class KANTVMediaGridAdapter<T> extends BaseAdapter {
    private static final String TAG = KANTVMediaGridAdapter.class.getName();
    private ArrayList<T> mData;
    private int mLayoutRes;
    private Activity mActivity;

    public KANTVMediaGridAdapter() {
    }

    public KANTVMediaGridAdapter(ArrayList<T> mData, int mLayoutRes, Activity mActivity) {
        this.mData = mData;
        this.mLayoutRes = mLayoutRes;
        this.mActivity = mActivity;
    }

    @Override
    public int getCount() {
        return mData != null ? mData.size() : 0;
    }

    @Override
    public T getItem(int position) {
        return mData.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder = ViewHolder.bind(mActivity, parent.getContext(), convertView, parent, mLayoutRes, position);
        bindView(holder, getItem(position));
        return holder.getItemView();
    }

    public abstract void bindView(ViewHolder holder, T obj);

    public void add(T data) {
        if (mData == null) {
            mData = new ArrayList<>();
        }
        mData.add(data);
        notifyDataSetChanged();
    }

    public void add(int position, T data) {
        if (mData == null) {
            mData = new ArrayList<>();
        }
        mData.add(position, data);
        notifyDataSetChanged();
    }

    public void remove(T data) {
        if (mData != null) {
            mData.remove(data);
        }
        notifyDataSetChanged();
    }

    public void remove(int position) {
        if (mData != null) {
            mData.remove(position);
        }
        notifyDataSetChanged();
    }

    public void clear() {
        if (mData != null) {
            mData.clear();
        }
        notifyDataSetChanged();
    }


    public static class ViewHolder {

        private SparseArray<View> mViews;
        private View item;
        private int position;
        private Context context;
        private Activity activity;


        private ViewHolder(Activity activity, Context context, ViewGroup parent, int layoutRes) {
            mViews = new SparseArray<>();
            this.context = context;
            this.activity = activity;
            View convertView = LayoutInflater.from(context).inflate(layoutRes, parent, false);
            convertView.setTag(this);
            item = convertView;
        }


        public static ViewHolder bind(Activity activity, Context context, View convertView, ViewGroup parent,
                                      int layoutRes, int position) {
            ViewHolder holder;
            if (convertView == null) {
                holder = new ViewHolder(activity, context, parent, layoutRes);
            } else {
                holder = (ViewHolder) convertView.getTag();
                holder.item = convertView;
            }
            holder.position = position;
            return holder;
        }

        @SuppressWarnings("unchecked")
        public <T extends View> T getView(int id) {
            T t = (T) mViews.get(id);
            if (t == null) {
                t = (T) item.findViewById(id);
                mViews.put(id, t);
            }
            return t;
        }


        public View getItemView() {
            return item;
        }


        public int getItemPosition() {
            return position;
        }


        public ViewHolder setText(int id, CharSequence text) {
            View view = getView(id);
            if (view instanceof TextView) {
                ((TextView)view).setText(text);
            }
            return this;
        }


        public ViewHolder setImageResource(int id, int drawableRes) {
            View view = getView(id);
            if (view instanceof ImageView) {
                ((ImageView) view).setImageResource(drawableRes);
            } else {
                view.setBackgroundResource(drawableRes);
            }
            return this;
        }

        public ViewHolder setImageResource(String resourceName, int drawableRes) {
            KANTVLog.j(TAG, "resource name:" + resourceName);
            Resources res = activity.getResources();
            int id = res.getIdentifier(resourceName, "mipmap", activity.getPackageName());
            View view = getView(id);
            if (view instanceof ImageView) {
                ((ImageView) view).setImageResource(drawableRes);
            } else {
                view.setBackgroundResource(drawableRes);
            }
            return this;
        }


        public ViewHolder setImageResource(int id, String imgUri) {
            View view = getView(id);
            //KANTVLog.d(TAG, "load img(which download from network) from local uri: " + imgUri);
            Bitmap bitmap = BitmapFactory.decodeFile(imgUri);
            if (view instanceof ImageView) {
                ((ImageView) view).setImageBitmap(bitmap);
            } else {
                Drawable drawable = null;
                drawable =  new BitmapDrawable(bitmap);
                view.setBackground(drawable);
            }
            return this;
        }

        public ViewHolder setImageResource(int id, byte[] bytes, int length) {
            View view = getView(id);
            KANTVLog.g(TAG, "load img from bytes, length " + length + " len " + bytes.length);
            //FIXME: unknown issue here
            //Bitmap bitmap = BitmapFactory.decodeByteArray(bytes, 0, length);
            //FIXME: this is workaround
            Bitmap bitmap = BitmapFactory.decodeFile("/data/data/com.kantvai.kantvplayer/text2image.bmp");
            KANTVLog.g(TAG, "bitmap.width " + bitmap.getWidth() + " height:" + bitmap.getHeight());
            if (view instanceof ImageView) {
                ((ImageView) view).setImageBitmap(bitmap);
            } else {
                Drawable drawable =  new BitmapDrawable(bitmap);
                view.setBackground(drawable);
            }
            return this;
        }


        public ViewHolder setOnClickListener(int id, View.OnClickListener listener) {
            getView(id).setOnClickListener(listener);
            return this;
        }


        public ViewHolder setVisibility(int id, int visible) {
            getView(id).setVisibility(visible);
            return this;
        }


        public ViewHolder setTag(int id, Object obj) {
            getView(id).setTag(obj);
            return this;
        }

    }

}