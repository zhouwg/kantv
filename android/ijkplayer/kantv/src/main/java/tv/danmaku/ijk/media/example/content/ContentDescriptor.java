 /*
  * Copyright (C) 2021 zhouwg <zhouwg2000@gmail.com>
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *      http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */

package tv.danmaku.ijk.media.example.content;

import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Parcel;
import android.os.Parcelable;

import tv.danmaku.ijk.media.example.R;

public class ContentDescriptor implements Parcelable {
    private String mName; //tv name or movie name
    private String mURL;
    private IjkUrlType mUrlType;
    private boolean mIsProtected; //DRM protected content
    private String mLicenseURL;   //DRM license

    //TODO: for future use, should interact with remote EPG server?
    private String mCountry;
    private String mLanguage;
    private String mIdString;
    private String mLogo;       //TV logo if mMediaType == MEDIA_TV
    private String mGroupTitle; //catagory of content
    private MediaType mMediaType;

    public ContentDescriptor(String name, String url, IjkUrlType urlType) {
        this.mName = name;
        this.mURL = url;
        this.mUrlType = urlType;
    }

    public ContentDescriptor(String name, String url, IjkUrlType urlType, boolean isProtected, String licenseURL) {
        this.mName = name;
        this.mURL = url;
        this.mUrlType = urlType;
        this.mIsProtected = isProtected;
        this.mLicenseURL = licenseURL;
    }

    public ContentDescriptor(String name, String url, IjkUrlType urlType, MediaType mediaType, boolean isProtected, String licenseURL) {
        this.mName = name;
        this.mURL = url;
        this.mUrlType = urlType;
        this.mMediaType = mediaType;
        this.mIsProtected = isProtected;
        this.mLicenseURL = licenseURL;
    }

    public String getName() {
        return mName;
    }

    public String getLicenseURL() {
        return mLicenseURL;
    }

    public String getUrl() {
        return mURL;
    }

    public IjkUrlType getType() {
        switch (mUrlType) {
            case HLS:
                return IjkUrlType.HLS;
            case RTMP:
                return IjkUrlType.RTMP;
            case DASH:
                return IjkUrlType.DASH;
            case FILE:
            default:
                return IjkUrlType.FILE;
        }
    }

    public boolean getIsProtected() {
        return mIsProtected;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel arg0, int arg1) {
        arg0.writeString(mName);
        arg0.writeString(mURL);
        arg0.writeValue(mUrlType);
        arg0.writeValue(Boolean.valueOf(mIsProtected));
        arg0.writeString(mLicenseURL);
    }

    public static final Parcelable.Creator<ContentDescriptor> CREATOR = new Parcelable.Creator<ContentDescriptor>() {
        public ContentDescriptor createFromParcel(Parcel in) {
            return new ContentDescriptor(in);
        }

        public ContentDescriptor[] newArray(int size) {
            return new ContentDescriptor[size];
        }
    };

    private ContentDescriptor(Parcel in) {
        mName = in.readString();
        mURL = in.readString();
        mUrlType = (IjkUrlType) in.readValue(null);
        mIsProtected = (Boolean) in.readValue(null);
        mLicenseURL = in.readString();
    }

    public Drawable getDrawable(Resources res) {
        Drawable icon = null;

        if (mUrlType == IjkUrlType.HLS) {
            icon = res.getDrawable(R.drawable.hls);
        } else if (mUrlType == IjkUrlType.RTMP) {
            icon = res.getDrawable(R.drawable.rtmp);
        } else if (mUrlType == IjkUrlType.DASH) {
            icon = res.getDrawable(R.drawable.dash);
        } else {
            icon = res.getDrawable(R.drawable.file);
        }

        return icon;
    }
}