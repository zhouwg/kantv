 /*
  * Copyright (c) Project KanTV. 2021-2023. All rights reserved.
  *
  * Copyright (c) 2024- KanTV Authors. All Rights Reserved.
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
package cdeos.media.player;

import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Parcel;
import android.os.Parcelable;


public class CDEContentDescriptor implements Parcelable {
    private static final String TAG = CDEContentDescriptor.class.getName();
    private String mName; // program name
    private String mURL;  // program url
    private CDEUrlType mUrlType;
    private boolean mIsProtected; //DRM protected content
    private boolean mIsLive;      //Live or VOD
    private String mLicenseURL;   //DRM license
    private String mDrmScheme;    //DRM scheme

    private String mTVGID;        //tvg id;
    private String mTVGLogoURL;   //tvg logo URL
    private String mTVGLogoName;  //tvg logo name


    private String mCountry;
    private String mLanguage;
    private String mIdString;
    private String mLogo;       //TV logo if mMediaType == MEDIA_TV
    private String mPoster;     //Movie poster if mMediaType == MEDIA_MOVIE
    private String mGroupTitle; //category of content
    private CDEMediaType mMediaType;

    public CDEContentDescriptor(String name, String url, CDEUrlType urlType) {
        this.mName = name;
        this.mURL = url;
        this.mUrlType = urlType;
    }

    public CDEContentDescriptor(String tvglogoURL, String groupTitle, String name, String url) {
        this.mTVGLogoURL = tvglogoURL;
        this.mGroupTitle = groupTitle;
        this.mName = name;
        this.mURL = url;
        this.mUrlType = CDEUrlType.HLS;
    }

    public CDEContentDescriptor(String tvglogoURL, String tvglogoName, String groupTitle, String name, String url) {
        this.mTVGLogoURL = tvglogoURL;
        this.mTVGLogoName = tvglogoName;
        this.mGroupTitle = groupTitle;
        this.mName = name;
        this.mURL = url;
        this.mUrlType = CDEUrlType.HLS;
    }

    public CDEContentDescriptor(String name, String url, CDEUrlType urlType, boolean isProtected, boolean isLive, String drmScheme, String licenseURL) {
        this.mName = name;
        this.mURL = url;
        //CDELog.j(TAG, "url : " + this.mURL);
        this.mUrlType = urlType;
        this.mIsProtected = isProtected;
        this.mIsLive      = isLive;
        this.mDrmScheme   = drmScheme;
        this.mLicenseURL = licenseURL;
    }

    public CDEContentDescriptor(String name, String url, CDEUrlType urlType, boolean isProtected, boolean isLive, String drmScheme, String licenseURL, String moviePoster) {
        this.mName = name;
        this.mURL = url;
        //CDELog.j(TAG, "url : " + this.mURL);
        this.mUrlType = urlType;
        this.mIsProtected = isProtected;
        this.mIsLive      = isLive;
        this.mDrmScheme   = drmScheme;
        this.mLicenseURL = licenseURL;
        this.mPoster = moviePoster;
    }

    public CDEContentDescriptor(String name, String url, CDEUrlType urlType, CDEMediaType mediaType, boolean isProtected, boolean isLive, String drmScheme, String licenseURL) {
        this.mName = name;
        this.mURL = url;
        //CDELog.j(TAG, "url : " + this.mURL);
        this.mUrlType = urlType;
        this.mMediaType = mediaType;
        this.mIsProtected = isProtected;
        this.mIsLive      = isLive;
        this.mDrmScheme   = drmScheme;
        this.mLicenseURL = licenseURL;
    }

    public String getName() {
        return mName;
    }

    public String getLicenseURL() {
        return mLicenseURL;
    }

    public String getDrmScheme() {
        return mDrmScheme;
    }

    public String getUrl() {
        //CDELog.j(TAG, "url : " + this.mURL);
        return mURL;
    }

    public String getPoster() { return mPoster; }

    public CDEUrlType getType() {
        switch (mUrlType) {
            case HLS:
                return CDEUrlType.HLS;
            case RTMP:
                return CDEUrlType.RTMP;
            case DASH:
                return CDEUrlType.DASH;
            case MKV:
                return CDEUrlType.MKV;
            case TS:
                return CDEUrlType.TS;
            case MP4:
                return CDEUrlType.MP4;
            case MP3:
                return CDEUrlType.MP3;
            case OGG:
                return CDEUrlType.OGG;
            case AAC:
                return CDEUrlType.AAC;
            case AC3:
                return CDEUrlType.AC3;

            case FILE:
            default:
                return CDEUrlType.FILE;
        }
    }

    public boolean getIsProtected() {
        return mIsProtected;
    }

    public boolean getIsLive() {
        return mIsLive;
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
        arg0.writeValue(Boolean.valueOf(mIsLive));
        arg0.writeString(mDrmScheme);
        arg0.writeString(mLicenseURL);
        arg0.writeString(mPoster);
    }

    public static final Parcelable.Creator<CDEContentDescriptor> CREATOR = new Parcelable.Creator<CDEContentDescriptor>() {
        public CDEContentDescriptor createFromParcel(Parcel in) {
            return new CDEContentDescriptor(in);
        }

        public CDEContentDescriptor[] newArray(int size) {
            return new CDEContentDescriptor[size];
        }
    };

    private CDEContentDescriptor(Parcel in) {
        mName = in.readString();
        mURL = in.readString();
        mUrlType = (CDEUrlType) in.readValue(null);
        mIsProtected = (Boolean) in.readValue(null);
        mIsLive = (Boolean) in.readValue(null);
        mDrmScheme = in.readString();
        mLicenseURL = in.readString();
        mPoster = in.readString();
    }

    public Drawable getDrawable(Resources res, int resID) {
        Drawable icon = null;
        if (mUrlType == CDEUrlType.HLS) {
            //icon = res.getDrawable(R.drawable.hls);
            icon = res.getDrawable(resID);
        } else if (mUrlType == CDEUrlType.RTMP) {
            //icon = res.getDrawable(R.drawable.rtmp);
            icon = res.getDrawable(resID);
        } else if (mUrlType == CDEUrlType.DASH) {
            //icon = res.getDrawable(R.drawable.dash);
            icon = res.getDrawable(resID);
        } else if (mUrlType == CDEUrlType.FILE) {
            //icon = res.getDrawable(R.drawable.dash);
            icon = res.getDrawable(resID);
        } else {
            CDELog.d(TAG, "unknown type:" + mUrlType.toString());
            //icon = res.getDrawable(R.drawable.file);
            icon = null;
        }
        if (icon == null) {
            CDELog.d(TAG, "can't get drawable, pls check");
        }
        return icon;
    }

    public static native int kantv_anti_tamper();
}