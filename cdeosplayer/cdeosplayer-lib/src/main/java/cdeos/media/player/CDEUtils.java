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

import static java.lang.Math.abs;

import android.app.Activity;
import android.app.ActivityManager;
import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.media.MediaCodec;
import android.net.ConnectivityManager;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Debug;
import android.os.Environment;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Base64;
import android.util.DisplayMetrics;
import android.widget.Toast;

import com.alibaba.fastjson.JSON;
import com.alibaba.fastjson.JSONObject;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.conn.util.InetAddressUtils;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.util.EntityUtils;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.Closeable;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.nio.channels.FileChannel;
import java.security.MessageDigest;
import java.security.SecureRandom;
import java.text.SimpleDateFormat;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Queue;
import java.util.Random;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.regex.Pattern;

import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import cdeos.media.player.CDEMediaType;

import static cdeos.media.player.CDEMediaType.MEDIA_MOVIE;
import static cdeos.media.player.CDEMediaType.MEDIA_RADIO;
import static cdeos.media.player.CDEMediaType.MEDIA_TV;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.core.app.NotificationBuilderWithBuilderAccessor;


public class CDEUtils {
    private final static String TAG = CDEUtils.class.getName();

    private static String mKANTVMasterServer            = "www.cde-os.com";
    private static String mKANTVUpdateAPKUrl            = "http://www.cde-os.com/kantv/apk/kantv-latest.apk";
    private static String mKANTVUpdateAPKVersionUrl     = "http://www.cde-os.com/kantv/apk/kantv-version.txt";
    private static String mKANTVUpdateMainEPGUrl        = "http://www.cde-os.com/kantv/kantv.bin";
    private static String mKANTVServer                  = "www.cde-os.com";
    private static String mKANTVUMUrl                   = "http://www.cde-os.com:81/epg/uploadUM";
    private static String mNginxServerUrl               = "http://www.cde-os.com:81";
    private static String mApiGatewayServerUrl          = "http://www.cde-os.com:8888/wiseplay/getlicense";
    private static String mLocalEMS                     = "http://192.168.0.200:81/ems";

    private static String mKANTVAPKVersion              = "1.2.8";
    private static KANTVDRM mKANTVDRM                   = KANTVDRM.getInstance();

    public static final String INVALID_DEVICE_ID        = "000000000000000";
    public static final String INVALID_ANDROID_ID       = "1234567890abcde";
    public static String DEVICE_ID_FILE_NAME            = "device_id.dat";
    public static String DEVICE_ID_FILE_PATH            = " ";

    private static boolean mDisableUM                   = true;
    private static boolean mIsKanTVAPK                  = false;
    private static boolean mTVEpgUpdatedStatus          = false;
    private static boolean mMovieEpgUpdatedStatus       = false;
    private static boolean mRadioEpgUpdatedStatus       = false;
    public static final boolean USE_DECODER_EXTENSIONS  = false;
    private static long mBeginPlaybackTime              = 0;
    private static long mBeginRecordTime                = 0;
    private static long mFirstVideoFrameRenderedTime    = 0;


    public static final int PV_PLAYERENGINE__FFmpeg             = 0; //preferred engine
    public static final int PV_PLAYERENGINE__Exoplayer          = 1; //should be replaced with AndroidX Media3 in the future
    public static final int PV_PLAYERENGINE__AndroidMediaPlayer = 2; //AndroidMediaPlayer might be supported by some semiconductor company
    public static final int PV_PLAYERENGINE__GSTREAMER          = 3; //supported by many semiconductor company

    private static int mPlayEngine = PV_PLAYERENGINE__FFmpeg;

    public static final int PLAY_MODE_NORMAL                    = 0;
    public static final int PLAY_MODE_LOOP_CONTENT              = 1;
    public static final int PLAY_MODE_LOOP_LIST                 = 2;


    public static final int DECRYPT_NONE                        = 0;
    public static final int DECRYPT_SOFT                        = 1;
    public static final int DECRYPT_TEE_NONSVP                  = 2;
    public static final int DECRYPT_TEE_SVP                     = 3;
    public static final int DECRYPT_TEE                         = 4;
    private static int mDecryptMode                             = DECRYPT_SOFT;

    public static final int DRMPLUGIN_TYPE_SOFT                 = 1;
    public static final int DRMPLUGIN_TYPE_TEE_NONSVP           = 2;
    public static final int DRMPLUGIN_TYPE_TEE_SVP              = 3;
    private static int mDrmpluginType                           = DRMPLUGIN_TYPE_SOFT;

    private static boolean m_isNetworkAvailable             = true;

    public static final int DUMP_MODE_NONE                  = 0;
    public static final int DUMP_MODE_PARTIAL               = 1;
    public static final int DUMP_MODE_FULL                  = 2;
    public static final int DUMP_MODE_DATA_DECRYPT_FILE     = 3;
    public static final int DUMP_MODE_PERF_DECRYPT_TERMINAL = 4;
    public static final int DUMP_MODE_PERF_DECRYPT_FILE     = 5;
    public static final int DUMP_MODE_VIDEO_ES_FILE         = 6;
    public static final int DUMP_MODE_AUDIO_ES_FILE         = 7;


    public static final int VIDEO_TRACK = 0;
    public static final int AUDIO_TRACK = 1;

    private static int mStartPlayPos            = 0; //minutes
    private static boolean mDisableAudioTrack   = false;
    private static boolean mDisableVideoTrack   = false;


    public static final int NETWORK_WIFI    = 1;
    public static final int NETWORK_LAN     = 9;
    public static final int NETWORK_MOBILE  = 0;

    public static final int LINE_LENGTH_IN_PHONE_UI         = 30;
    public static final int LINE_LENGTH_IN_TV_UI            = 300;
    public static final int SCREEN_ORIENTATION_LANDSCAPE    = 0;
    public static final int SCREEN_ORIENTATION_PORTRAIT     = 1;

    public static final String MD5      = "MD5";
    public static final String SHA1     = "SHA-1";
    public static final String SHA256   = "SHA-256";


    private static int mScreenOrientation       = SCREEN_ORIENTATION_LANDSCAPE;
    private static int mNetworkType             = NETWORK_WIFI;
    private static String mNetworkIP            = " ";
    private static String mUniqueID             = "";
    private static String mJavaLayerUniqueID    = "unknown";

    public static final int TROUBLESHOOTING_MODE_DISABLE    = 0;
    public static final int TROUBLESHOOTING_MODE_INTERNAL   = 1;
    public static final int TROUBLESHOOTING_MODE_MAX        = 2;
    private static int mTroubleshootingMode = TROUBLESHOOTING_MODE_DISABLE;

    private static boolean mReleaseMode                     = true;
    private static boolean mDevelopmentMode                 = false;

    private static boolean mEnableDumpVideoES = false;
    private static boolean mEnableDumpAudioES = false;

    private static int mDumpDuration    = 10; //default dump duration is 10 seconds
    private static int mDumpSize        = 1024; //default dump size is 1024 Kbytes
    private static int mDumpCounts      = 100; //default dump counts is 100 packets

    private static int mRecordDuration              = 1; //default record duration is 1 minitues
    private static int mRecordSize                  = 20; //default record size is 20 Mbytes
    private static int mRecordMode                  = 0; //0:video, 1:audio, 2:video&audio
    private static boolean mIsTVRecording           = false;
    private static String  mRecordingFileName       = "";
    private static boolean mEnableRecordVideoES     = true;
    private static boolean mEnableRecordAudioES     = false;
    private static int mDiskThresholdFreeSize       = 500; // per Mbytes;
    private static int mRecordCountsFromServer      = 0;
    private static boolean mIsPaidUser              = false;
    private static IMediaPlayer mExoplayerInstance  = null;

    private static long mLoopPlayBeginTime          = 0;
    private static long mLoopPlayEndTime            = 0;
    private static long mLoopPlayCounts             = 0;
    private static long mLoopPlayErrorCounts        = 0;

    private static boolean mIsPermissionGranted     = false;
    private static boolean mUsingFFmpegCodec        = true;
    private static boolean mIsAPKForTV              = false;

    private static AtomicBoolean mCouldExitApp      = new AtomicBoolean(true);

    //borrow from Google Exoplayer
    /**
     * The Nil UUID as defined by <a
     * href="https://tools.ietf.org/html/rfc4122#section-4.1.7">RFC4122</a>.
     */
    public static final UUID UUID_NIL = new UUID(0L, 0L);
    /**
     * UUID for the W3C <a
     * href="https://w3c.github.io/encrypted-media/format-registry/initdata/cenc.html">Common PSSH
     * box</a>.
     */
    public static final UUID COMMON_PSSH_UUID = new UUID(0x1077EFECC0B24D02L, 0xACE33C1E52E2FB4BL);

    /**
     * UUID for the ClearKey DRM scheme.
     *
     * <p>ClearKey is supported on Android devices running Android 5.0 (API Level 21) and up.
     */
    public static final UUID CLEARKEY_UUID = new UUID(0xE2719D58A985B3C9L, 0x781AB030AF78D30EL);

    /**
     * UUID for the Widevine DRM scheme.
     *
     * <p>Widevine is supported on Android devices running Android 4.3 (API Level 18) and up.
     */
    public static final UUID WIDEVINE_UUID = new UUID(0xEDEF8BA979D64ACEL, 0xA3C827DCD51D21EDL);

    /**
     * UUID for the PlayReady DRM scheme.
     *
     * <p>PlayReady is supported on all AndroidTV devices. Note that most other Android devices do not
     * provide PlayReady support.
     */
    public static final UUID PLAYREADY_UUID = new UUID(0x9A04F07998404286L, 0xAB92E65BE0885F95L);

    /**
     * UUID for the ChinaDRM scheme
     */
    //software
    //public static final UUID CHINADRM_UUID = new UUID(0x70c1db9f66ae4127L, 0xbfc0bb1981694b67L);
    //TEE
    public static final UUID CHINADRM_UUID = new UUID(0x70c1db9f66ae4127L, 0xbfc0bb1981694b66L);

    /**
     * UUID for the WisePlay UUID
     */
    public static final UUID WISEPLAY_UUID = new UUID(0X3D5E6D359B9A41E8L, 0XB843DD3C6E72C42CL);

    /**
     * @see MediaCodec#CRYPTO_MODE_UNENCRYPTED
     */
    public static final int CRYPTO_MODE_UNENCRYPTED = MediaCodec.CRYPTO_MODE_UNENCRYPTED;
    /**
     * @see MediaCodec#CRYPTO_MODE_AES_CTR
     */
    public static final int CRYPTO_MODE_AES_CTR = MediaCodec.CRYPTO_MODE_AES_CTR;
    /**
     * @see MediaCodec#CRYPTO_MODE_AES_CBC
     */
    public static final int CRYPTO_MODE_AES_CBC = MediaCodec.CRYPTO_MODE_AES_CBC;
    public static final int CRYPTO_MODE_SM4_CTR = 3;
    public static final int CRYPTO_MODE_SM4_CBC = 4;


    public static final int DRM_SCHEME_CHINADRM     = 0;
    public static final int DRM_SCHEME_WISEPLAY     = 1;
    public static final int DRM_SCHEME_FAIRPLAY     = 2;
    public static final int DRM_SCHEME_PLAYREADY    = 3;
    public static final int DRM_SCHEME_WIDEVINE     = 4;
    public static final int DRM_SCHEME_MARLIN       = 5;
    public static final int DRM_SCHEME_MAX          = 6;

    public static final int SM4SampleEncryptionMod  = 1;
    public static final int SM4CBCEncryptionMod     = 3;
    public static final int AESSampleEncryptionMod  = 4;
    public static final int AESCBCEncryptionMod     = 5;
    public static final int SM4_0_EncryptionMod     = 6;
    public static final int SM4_1_EncryptionMod     = 7;
    public static final int AES_0_EncryptionMod     = 8;



    private static boolean mEnableMultiDRM      = false;
    private static boolean mEnableWisePlay      = false;
    private static String mDrmSchemeName        = "";
    private static String mDrmLicenseURL        = "";
    private static int mDrmScheme               = DRM_SCHEME_CHINADRM;
    private static int mEncryptScheme           = SM4SampleEncryptionMod;

    public static final int PLAYLIST_TYPE_UNKNOWN   = 0;
    public static final int PLAYLIST_TYPE_VOD       = 1;
    public static final int PLAYLIST_TYPE_LIVE      = 2;
    private static int mHLSPlaylistType             = PLAYLIST_TYPE_VOD;

    public static final int KEY_AES_128             = 0;
    public static final int KEY_SAMPLE_AES          = 1;
    public static final int KEY_SAMPLE_AES_0        = 2;
    public static final int KEY_AES_CBC             = 3;
    public static final int KEY_SAMPLE_SM4_CBC      = 4;     // 10%  encrypt
    public static final int KEY_SAMPLE_SM4_CBC_0    = 5;     // 0%   encrypt
    public static final int KEY_SAMPLE_SM4_CBC_1    = 6;     // 1%   encrypt
    public static final int KEY_SM4_CBC             = 7;     // 100% encrypt
    public static final int KEY_NONE                = 8;

    public static final int ES_H264                     = 0;
    public static final int ES_H264_SAMPLE_AES          = 1;
    public static final int ES_H264_SAMPLE_AES_0        = 2;
    public static final int ES_H264_SAMPLE_SM4_CBC      = 3;
    public static final int ES_H264_SAMPLE_SM4_CBC_0    = 4;
    public static final int ES_H264_SAMPLE_SM4_CBC_1    = 5;
    public static final int ES_H264_AES_CBC             = 6;
    public static final int ES_H264_SM4_CBC             = 7;
    public static final int ES_H265                     = 8;
    public static final int ES_H265_SAMPLE_AES          = 9;
    public static final int ES_H265_SAMPLE_AES_0        = 10;
    public static final int ES_H265_SAMPLE_SM4_CBC      = 11;
    public static final int ES_H265_SAMPLE_SM4_CBC_0    = 12;
    public static final int ES_H265_SAMPLE_SM4_CBC_1    = 13;
    public static final int ES_H265_AES_CBC             = 14;
    public static final int ES_H265_SM4_CBC             = 15;
    public static final int ES_UNKNOWN                  = 16;

    private static boolean mIsOfflineDRM = false;

    private static int mScreenWidth     = 768;
    private static int mScreenHeight    = 1024;

    public static void setAPKForTV(boolean isAPKForTV) {
        mIsAPKForTV = isAPKForTV;
    }

    public static void setScreenWidth(int width) {
        mScreenWidth = width;
    }

    public static void setScreenHeight(int height) {
        mScreenHeight = height;
    }

    public static int getScreenWidth() {
        return mScreenWidth;
    }

    public static int getmScreenHeight() {
        return mScreenHeight;
    }

    //FIXME
    public static boolean isRunningOnTV() {
        if (mScreenWidth > mScreenHeight)
            return true;
        else
            return false;
    }


    public static boolean isRunningOnAmlogicBox() {
        if (Build.BOARD.equals("p291_iptv")) {
            return true;
        }

        return false;
    }


    public static boolean isRunningOnCoocaaTV() {
        if (Build.MANUFACTURER.equals("Skyworth") && Build.DEVICE.equals("Hi3751V551") && Build.MODEL.equals("8H55_V7")) {
            return true;
        }

        return false;
    }


    //FIXME
    public static boolean isRunningOnTV(Activity activity) {
        DisplayMetrics dm = new DisplayMetrics();
        activity.getWindowManager().getDefaultDisplay().getMetrics(dm);
        int width = dm.widthPixels;
        int height = dm.heightPixels;
        float density = dm.density;
        int densityDpi = dm.densityDpi;
        int screenWidth = (int) (width / density);   // 屏幕宽度(dp)
        int screenHeight = (int) (height / density); // 屏幕高度(dp)
        CDELog.d(TAG, "screen width x height（pixel)= : " + width + "x" + height);
        CDELog.d(TAG, "screen width x height (dp)   = : " + screenWidth + "x" + screenHeight);
        //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR);
        CDELog.d(TAG, "device: " + Build.BOARD);


        if (false) {

            if (Build.BOARD.equals("p291_iptv")) {
                return true;
            }

            if (Build.BOARD.equals("bigfish") && Build.MANUFACTURER.equals("Hisilicon") && Build.DEVICE.equals("Hi3751V560")) {
                return true;
            }

            if (Build.BOARD.equals("bigfish") && Build.MANUFACTURER.equals("Skyworth") && Build.DEVICE.equals("Hi3751V551")) {
                return true;
            }
        }

        return mIsAPKForTV;
    }

    //FIXME
    public static boolean isRunningOnPhone() {
        boolean isRunningOnTV = isRunningOnTV();

        return !isRunningOnTV;
    }


    public static boolean isUseDecoderExtensions() {
        return USE_DECODER_EXTENSIONS;
    }

    public static class EPGXmlParser {
        private String getXmlFromUrl(String url) {
            //CDELog.d(TAG, "url:" + url);
            if (url.startsWith("http://") || url.startsWith("https://")) {
                String xml = null;
                try {
                    DefaultHttpClient httpClient = new DefaultHttpClient();
                    HttpResponse httpResponse = httpClient.execute(new HttpGet(url));
                    HttpEntity httpEntity = httpResponse.getEntity();
                    xml = EntityUtils.toString(httpEntity, "UTF-8");
                } catch (Exception e) {
                    e.printStackTrace();
                    CDELog.j(TAG, "getXmlFromUrl failed:" + e.getMessage());
                }
                return xml;
            } else {
                String encoding = "UTF-8";
                File file = new File(url);
                Long fileLength = file.length();
                byte[] fileContent = new byte[fileLength.intValue()];
                try {
                    FileInputStream in = new FileInputStream(file);
                    in.read(fileContent);
                    in.close();
                } catch (FileNotFoundException e) {
                    CDELog.j(TAG, "getXmlFromUrl failed:" + e.getMessage());
                } catch (IOException e) {
                    e.printStackTrace();
                    CDELog.j(TAG, "getXmlFromUrl failed:" + e.getMessage());
                }

                try {
                    return new String(fileContent, encoding);
                } catch (UnsupportedEncodingException ex) {
                    ex.printStackTrace();
                    CDELog.d(TAG, "getXmlFromUrl failed:" + ex.getMessage());
                    return null;
                }
            }
        }


        private String getXmlFromBytes(byte[] dataBuffer) {
            try {
                String encoding = "UTF-8";
                return new String(dataBuffer, encoding);
            } catch (Exception ex) {
                ex.printStackTrace();
                CDELog.d(TAG, "getXmlFromBytes failed:" + ex.getMessage());
                return null;
            }
        }

        private Document getDomElement(String xmlContent) {
            Document doc = null;
            DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
            try {
                DocumentBuilder db = dbf.newDocumentBuilder();
                InputSource is = new InputSource();
                is.setCharacterStream(new StringReader(xmlContent));
                doc = db.parse(is);
            } catch (Exception e) {
                CDELog.d(TAG, "getDomElement failed:" + e.getMessage());
                e.printStackTrace();
            }
            return doc;
        }

        private String getValue(Element item, String str) {
            NodeList n = item.getElementsByTagName(str);
            return this.getElementValue(n.item(0));
        }

        private final String getElementValue(Node elem) {
            Node child;
            if (elem != null) {
                if (elem.hasChildNodes()) {
                    for (child = elem.getFirstChild(); child != null; child = child.getNextSibling()) {
                        if (child.getNodeType() == Node.TEXT_NODE) {
                            return child.getNodeValue();
                        }
                    }
                }
            }
            return "";
        }

        public static List<CDEContentDescriptor> getContentDescriptors(byte[] databuffer) {
            EPGXmlParser parser = new EPGXmlParser();
            String xml = parser.getXmlFromBytes(databuffer);
            List<CDEContentDescriptor> descriptors = getContentDescriptorsFromXML(xml);
            return descriptors;
        }

        public static List<CDEContentDescriptor> getContentDescriptors(String url) {
            EPGXmlParser parser = new EPGXmlParser();
            String xml = parser.getXmlFromUrl(url);

            List<CDEContentDescriptor> descriptors = getContentDescriptorsFromXML(xml);
            return descriptors;
        }

        public static List<CDEContentDescriptor> getContentDescriptorsFromXML(String xml) {
            List<CDEContentDescriptor> descriptors = new ArrayList<CDEContentDescriptor>();
            try {
                EPGXmlParser parser = new EPGXmlParser();
                Document doc = parser.getDomElement(xml);
                if (doc == null) {
                    CDELog.j(TAG, "xml parse failed");
                    return null;
                }
                NodeList nl = doc.getElementsByTagName("entry");
                CDELog.d(TAG, "media counts:" + nl.getLength());
                boolean isProtected = false;
                boolean isLive = false;
                String title = null;
                String href = null;
                String moviePoster = null;
                String contentTypeString = null;
                String protectedString = null;
                String liveString = null;
                String licenseURL = null;
                String drmScheme = null;
                CDEUrlType urlType = null;
                for (int i = 0; i < nl.getLength(); i++) {
                    Element node = (Element) nl.item(i);
                    Element titleElement = (Element) node.getElementsByTagName("title").item(0);
                    ;
                    Element linkElement = (Element) node.getElementsByTagName("link").item(0);
                    Element uriElement = (Element) node.getElementsByTagName("uri").item(0);
                    Element posterElement = (Element) node.getElementsByTagName("poster").item(0);
                    Element drmSchemeElement = (Element) node.getElementsByTagName("drm_scheme").item(0);
                    Element drmLicenseUrlElement = (Element) node.getElementsByTagName("drm_license_url").item(0);

                    if (titleElement != null)
                        CDELog.d(TAG, "TAG name:" + titleElement.getTagName());
                    if (linkElement != null)
                        CDELog.d(TAG, "TAG name:" + linkElement.getTagName());
                    if (uriElement != null)
                        CDELog.d(TAG, "TAG name:" + uriElement.getTagName());
                    if (drmSchemeElement != null)
                        CDELog.d(TAG, "TAG name:" + drmSchemeElement.getTagName());
                    if (drmLicenseUrlElement != null)
                        CDELog.d(TAG, "TAG name:" + drmLicenseUrlElement.getTagName());

                    if (titleElement != null) {
                        title = titleElement.getFirstChild().getNodeValue();
                        title = title.trim();
                        CDELog.d(TAG, "title: " + title);
                    }
                    if (linkElement != null) {
                        href = linkElement.getAttribute("href");

                        String newUrl = null;
                        if (href != null)
                            newUrl = href.trim();

                        if (newUrl != null)
                            href = newUrl;

                        moviePoster = linkElement.getAttribute("poster");
                        contentTypeString = linkElement.getAttribute("urltype");
                        protectedString = linkElement.getAttribute("protected");
                        liveString = linkElement.getAttribute("live");
                        drmScheme = linkElement.getAttribute("drm_scheme");
                        licenseURL = linkElement.getAttribute("laurl");
                        if (licenseURL == null || licenseURL.isEmpty()) {
                            licenseURL = linkElement.getAttribute("drm_license_url");
                        }
                        CDELog.d(TAG, "href:" + href);
                        CDELog.d(TAG, "poster:" + moviePoster);
                        CDELog.d(TAG, "type:" + contentTypeString);
                        CDELog.d(TAG, "protected:" + protectedString);
                        CDELog.d(TAG, "live:" + liveString);
                        CDELog.d(TAG, "drm scheme:" + drmScheme);
                        CDELog.d(TAG, "drm license url:" + licenseURL);
                    } else {
                        //make Huawei's Wiseplay DRM contents happy
                        if (drmSchemeElement != null) {
                            drmScheme = drmSchemeElement.getFirstChild().getNodeValue();
                            CDELog.d(TAG, "drmScheme:" + drmScheme);
                            protectedString = "yes";
                        } else {
                            protectedString = "clear";
                        }

                        if (drmLicenseUrlElement != null) {
                            licenseURL = drmLicenseUrlElement.getFirstChild().getNodeValue();
                            CDELog.d(TAG, "licenseURL:" + licenseURL);
                        }

                        if (posterElement != null) {
                            moviePoster = posterElement.getFirstChild().getNodeValue();
                            CDELog.d(TAG, "moviePoster:" + moviePoster);
                        }

                        if (uriElement != null) {
                            href = uriElement.getFirstChild().getNodeValue();
                            CDELog.j(TAG, "href:" + href);

                            if (href != null) {
                                if (href.contains("mpd")) {
                                    contentTypeString = "dash";
                                } else if (href.contains("m3u8")) {
                                    contentTypeString = "hls";
                                } else if (href.contains("rtmp")) {
                                    contentTypeString = "rtmp";
                                } else if (href.contains("mkv")) {
                                    contentTypeString = "mkv";
                                } else if (href.contains("mp4")) {
                                    contentTypeString = "mp4";
                                } else if (href.contains("mp3")) {
                                    contentTypeString = "mp3";
                                } else if (href.contains("ts")) {
                                    contentTypeString = "ts";
                                } else if (href.contains("ogg")) {
                                    contentTypeString = "ogg";
                                } else if (href.contains("aac")) {
                                    contentTypeString = "aac";
                                } else if (href.contains("ac3")) {
                                    contentTypeString = "ac3";
                                } else {
                                    contentTypeString = "file";
                                }
                            } else {
                                CDELog.j(TAG, "href is null");
                                contentTypeString = "file";
                            }
                            CDELog.d(TAG, "content type:" + contentTypeString);
                        }
                    }

                    if (title == null) {
                        CDELog.j(TAG, "parse failed:can't find element title in xml content");
                        title = "invalid";
                    }
                    if (moviePoster == null) {
                        moviePoster = "test.jpg";
                    }
                    if (protectedString != null && protectedString.equals("yes")) {
                        isProtected = true;

                        if (drmScheme == null || drmScheme.isEmpty()) {
                            //drmScheme = "chinadrm"; //default is chinadrm
                        }
                    } else {
                        isProtected = false;
                    }

                    if (liveString != null && liveString.equals("yes")) {
                        isLive = true;
                    } else {
                        isLive = false;
                    }

                    if (licenseURL != null && licenseURL.length() > 0) {
                        CDELog.d(TAG, "License/Authentication URL: " + licenseURL);
                    }
                    if (contentTypeString.equals("hls")) {
                        urlType = CDEUrlType.HLS;
                    } else if (contentTypeString.equals("rtmp")) {
                        urlType = CDEUrlType.RTMP;
                    } else if (contentTypeString.equals("dash")) {
                        urlType = CDEUrlType.DASH;
                    } else if (contentTypeString.equals("mp4")) {
                        urlType = CDEUrlType.MP4;
                    } else if (contentTypeString.equals("ts")) {
                        urlType = CDEUrlType.TS;
                    } else if (contentTypeString.equals("mkv")) {
                        urlType = CDEUrlType.MKV;
                    } else if (contentTypeString.equals("mp3")) {
                        urlType = CDEUrlType.MP3;
                    } else if (contentTypeString.equals("ogg")) {
                        urlType = CDEUrlType.OGG;
                    } else if (contentTypeString.equals("aac")) {
                        urlType = CDEUrlType.AAC;
                    } else if (contentTypeString.equals("ac3")) {
                        urlType = CDEUrlType.AC3;
                    } else if (contentTypeString.equals("file")) {
                        urlType = CDEUrlType.FILE;
                    } else {
                        CDELog.d(TAG, "unknown url type, set to FILE");
                        urlType = CDEUrlType.FILE;
                    }
                    CDELog.d(TAG, "url type:" + urlType.toString());

                    CDEContentDescriptor descriptor = null;
                    if (moviePoster.length() > 0) {
                        //CDELog.j(TAG, "url:" + href);
                        descriptor = new CDEContentDescriptor(
                                title,
                                href,
                                urlType,
                                isProtected, isLive, drmScheme, licenseURL, moviePoster);
                    } else {
                        CDELog.d(TAG, "url:" + href);
                        descriptor = new CDEContentDescriptor(
                                title,
                                href,
                                urlType,
                                isProtected, isLive, drmScheme, licenseURL);
                    }
                    descriptors.add(descriptor);
                }
            } catch (Exception e) {
                CDELog.j(TAG, "getContentDescriptors failed:" + e.getMessage());
                descriptors.clear();
                return null;
            }
            return descriptors;
        }
    }

    public static String getDataPath(Context context) {
        return new StringBuilder("/data/data/").append(context.getPackageName()).append("/").toString();
    }

    /**
     * default path: /sdcard/kantv
     * Android 5.1: /sdcard/Android/data/com.cdeos.kantv/files/kantv
     */
    public static String getDataPath() {
        CDELog.d(TAG, "ANDROID SDK:" + String.valueOf(android.os.Build.VERSION.SDK_INT));
        CDELog.d(TAG, "OS Version:" + android.os.Build.VERSION.RELEASE);
        String state = Environment.getExternalStorageState();
        CDELog.d(TAG, "state: " + state);
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP_MR1) {
            CDELog.j(TAG, "Android version > API22：Android 5.1 (Android L)Lollipop_MR1");
            if (!state.equals(Environment.MEDIA_MOUNTED)) {
                CDELog.d(TAG, "can't read/write extern device");
                return null;
            }
        } else {
            CDELog.j(TAG, "Android version <= API22：Android 5.1 (Android L)Lollipop");
        }

        File sdcardPath = Environment.getExternalStorageDirectory();
        CDELog.j(TAG, "sdcardPath name:" + sdcardPath.getName() + ",sdcardPath:" + sdcardPath.getPath());
        CDELog.j(TAG, "sdcard free size:" + mKANTVDRM.ANDROID_JNI_GetDiskFreeSize(sdcardPath.getAbsolutePath()));
        String dataDirectoryPath = null;
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP_MR1) {
            dataDirectoryPath = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separator + "kantv";
        } else {
            dataDirectoryPath = Environment.getExternalStorageDirectory().getAbsolutePath() +
                    File.separator + "Android" + File.separator + "data" + File.separator + "com.cdeos.kantv" + File.separator + "files" + File.separator + "kantv";
        }
        DEVICE_ID_FILE_PATH = dataDirectoryPath + File.separator + DEVICE_ID_FILE_NAME;
        CDELog.j(TAG, "DEVICE_ID_FILE_PATH: " + DEVICE_ID_FILE_PATH);
        CDELog.j(TAG, "dataDirectoryPath: " + dataDirectoryPath);

        String dataFileName;
        File timestampFile = null;
        File configFile = null;
        File deviceIDFile = null;
        FileOutputStream out = null;
        boolean ret = false;

        try {
            File directoryPath = new File(dataDirectoryPath);
            if (!directoryPath.exists()) {
                CDELog.j(TAG, "create dir: " + dataDirectoryPath);
                directoryPath.mkdirs();
            } else {
                CDELog.j(TAG, dataDirectoryPath + " already exist");
            }
            SimpleDateFormat simpleDateFormat = new SimpleDateFormat("yyyy-MM-dd");
            SimpleDateFormat fullDateFormat = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
            Date date = new Date(System.currentTimeMillis());

            dataFileName = "kantv-timestamp-" + fullDateFormat.format(date) + ".txt";
            timestampFile = new File(dataDirectoryPath, dataFileName);
            configFile = new File(dataDirectoryPath, "config.json");
            deviceIDFile = new File(DEVICE_ID_FILE_PATH);
            CDELog.j(TAG, "timestamp file name:" + timestampFile.getName() + ",timestamp file path:" + timestampFile.getPath());
            CDELog.j(TAG, "cfg file name:" + configFile.getName() + ",file path:" + configFile.getPath());

            if (!deviceIDFile.exists()) {
                deviceIDFile.createNewFile();
            }

            if (!configFile.exists()) {
                CDELog.d(TAG, "create config.json to make drmplugin happy");
                configFile.createNewFile();
            }

            //if (!timestampFile.exists()) {
            //    timestampFile.createNewFile();
            //out = new FileOutputStream(timestampFile);
            //byte[] bytesArray = (fullDateFormat.format(date) + "\n\n").getBytes();
            //out.write(bytesArray);
            //out.flush();
            //out.close();
            // CDELog.j(TAG, "create kantv timestamp file success: " + timestampFile.getAbsolutePath());
            //} else {
            //    CDELog.j(TAG, "kantv timestamp file already exist: " + timestampFile.getAbsolutePath());
            //}
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            CDELog.j(TAG, "create kantv timestamp file failed: " + e.toString());
        } catch (IOException e) {
            e.printStackTrace();
            CDELog.j(TAG, "create kantv timestamp file failed: " + e.toString());
        }

        return dataDirectoryPath;
    }


    public static String getEPGLocalDataPath() {
        String dataPath = getDataPath();

        if (null == dataPath) {
            CDELog.w(TAG, "can't getEPGLocalDataPath");
            return null;
        }

        String dataDirectoryPath = dataPath + "/epg" + "/posters";
        //CDELog.d(TAG, "dataDirectoryPath: " + dataDirectoryPath);

        String dataFileName;
        File file = null;
        FileOutputStream out = null;
        boolean ret = false;

        try {
            File directoryPath = new File(dataDirectoryPath);
            if (!directoryPath.exists()) {
                directoryPath.mkdirs();
            }
            SimpleDateFormat simpleDateFormat = new SimpleDateFormat("yyyy-MM-dd");
            SimpleDateFormat fullDateFormat = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
            Date date = new Date(System.currentTimeMillis());

            dataFileName = simpleDateFormat.format(date) + "-epgtimestamp.txt";
            file = new File(dataDirectoryPath, dataFileName);
            //CDELog.d(TAG, "file name:" + file.getName() + ",file path:" + file.getPath());

            if (!file.exists()) {
                file.createNewFile();
                out = new FileOutputStream(file);

                byte[] bytesArray = (fullDateFormat.format(date) + "\n\n").getBytes();
                out.write(bytesArray);
                out.flush();
                out.close();
                CDELog.d(TAG, "create epg timestamp file success: " + file.getAbsolutePath());
            } else {
                //CDELog.d(TAG, "cdeepg timestamp file already exist: " + file.getAbsolutePath());
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            CDELog.d(TAG, "create epg timestamp file failed: " + e.toString());
        } catch (IOException e) {
            e.printStackTrace();
            CDELog.d(TAG, "create epg timestamp file failed: " + e.toString());
        }

        return dataDirectoryPath;
    }

    public static void saveBitmap(Activity activity, Bitmap bitmap, boolean bWithVideo) {
        String dataPath = getDataPath();

        if (null == dataPath) {
            CDELog.w(TAG, "can't saveBitmap");
            return;
        }

        String picDirectoryPath = dataPath + File.separator + "screenshots";
        CDELog.d(TAG, "picDirectoryPath: " + picDirectoryPath);

        String picFileName;
        File file = null;
        FileOutputStream out = null;
        boolean ret = false;

        try {
            File directoryPath = new File(picDirectoryPath);
            if (!directoryPath.exists()) {
                directoryPath.mkdirs();
            }

            SimpleDateFormat simpleDateFormat = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
            Date date = new Date(System.currentTimeMillis());

            if (bWithVideo) {
                picFileName = simpleDateFormat.format(date) + "-kantv-screenshot-video.jpg";
            } else {
                picFileName = simpleDateFormat.format(date) + "-kantv-screenshot-novideo.jpg";
            }
            file = new File(picDirectoryPath, picFileName);
            CDELog.d(TAG, "file name:" + file.getName() + ",file path:" + file.getPath());

            if (!file.exists())
                file.createNewFile();
            out = new FileOutputStream(file);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            CDELog.d(TAG, "save picture failed: " + e.toString());
        } catch (IOException e) {
            e.printStackTrace();
            CDELog.d(TAG, "save picture failed: " + e.toString());
        }

        try {
            //ret =  bitmap.compress(Bitmap.CompressFormat.PNG, 100, out);
            ret = bitmap.compress(Bitmap.CompressFormat.JPEG, 100, out);
            if (!bitmap.isRecycled())
                bitmap.recycle();
            out.flush();
            out.close();

            if (ret) {
                CDELog.d(TAG, "screenshot success saved to: " + file.getAbsolutePath());
                Toast.makeText(activity, "screenshot saved to： " + file.getAbsolutePath(), Toast.LENGTH_SHORT).show();
            }

        } catch (IOException e) {
            e.printStackTrace();
            CDELog.d(TAG, "screenshot failed: " + e.toString());
            Toast.makeText(activity, "screenshot failed： " + e.toString(), Toast.LENGTH_SHORT).show();
        }
    }

    public static void setNetworkIP(String ip) {
        mNetworkIP = ip;
    }

    public static String getNetworkIP() {
        return mNetworkIP;
    }

    public static void setNetworkType(int networkType) {
        mNetworkType = networkType;
    }

    public static int getNetworkType() {
        return mNetworkType;
    }

    public static String getNetworkTypeString() {
        switch (mNetworkType) {
            case NETWORK_LAN:
                return "LAN";
            case NETWORK_MOBILE:
                return "Mobile";
            case NETWORK_WIFI:
                return "WIFI";
            default:
                return "Unknown";
        }
    }

    //FIXME
    private static String getMobileIP() {
        /*
        //crash on phone
        try {
            String ipv4;
            ArrayList<NetworkInterface> nilist = Collections.list(NetworkInterface.getNetworkInterfaces());
            for (NetworkInterface ni: nilist)
            {
                ArrayList<InetAddress> ialist = Collections.list(ni.getInetAddresses());
                for (InetAddress address: ialist) {
                    if (!address.isLoopbackAddress() && InetAddressUtils.isIPv4Address(ipv4 = address.getHostAddress()))
                    {
                        return ipv4;
                    }
                }

            }

        } catch (SocketException e) {
            CDELog.j(TAG, "can't get ip: " + e.toString());
        }

        return null;

        */
        return getLANIP();
    }


    private static String getLANIP() {
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements(); ) {
                NetworkInterface intf = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements(); ) {
                    InetAddress inetAddress = enumIpAddr.nextElement();
                    if (!inetAddress.isLoopbackAddress() && (inetAddress instanceof Inet4Address)) {
                        return inetAddress.getHostAddress();
                    }
                }
            }
        } catch (SocketException e) {
            CDELog.j(TAG, "can't get ip: " + e.toString());
        }
        return null;
    }

    public static String IPIntToString(int ipInt) {
        StringBuilder sb = new StringBuilder();
        sb.append(ipInt & 0xFF).append(".");
        sb.append((ipInt >> 8) & 0xFF).append(".");
        sb.append((ipInt >> 16) & 0xFF).append(".");
        sb.append((ipInt >> 24) & 0xFF);
        return sb.toString();
    }

    public static boolean isNetworkAvailable() {
        return m_isNetworkAvailable;
    }

    public static void setNetworkAvailable( boolean bAvailable) {
        m_isNetworkAvailable = bAvailable;
    }

    public static boolean isNetworkAvailable(Activity activity) {
        String ip = null;
        ConnectivityManager connectivityManager = (ConnectivityManager) activity.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            CDELog.j(TAG, "SDK_INT < Android Q");
            try {
                if (connectivityManager != null) {
                    NetworkInfo activeNetworkInfo = connectivityManager.getActiveNetworkInfo();
                    if ((activeNetworkInfo != null) && activeNetworkInfo.isConnected()) {
                        CDELog.j(TAG, "network info: " + activeNetworkInfo.getExtraInfo());
                        if (activeNetworkInfo.getType() == ConnectivityManager.TYPE_WIFI) {
                            CDELog.j(TAG, "wifi network");
                            setNetworkType(NETWORK_WIFI);
                            WifiManager wifiManager = (WifiManager) activity.getSystemService(Context.WIFI_SERVICE);
                            if (wifiManager != null) {
                                WifiInfo wifiInfo = wifiManager.getConnectionInfo();
                                if (wifiInfo != null) {
                                    int ipAddress = wifiInfo.getIpAddress();
                                    if (ipAddress != 0) {
                                        ip = IPIntToString(ipAddress);
                                        CDELog.j(TAG, "ip: " + ip);
                                        setNetworkIP(ip);
                                    }
                                }
                            }
                        }
                        if (activeNetworkInfo.getType() == ConnectivityManager.TYPE_MOBILE) {
                            CDELog.j(TAG, "mobile network");
                            ip = getMobileIP();
                            if (ip != null) {
                                CDELog.j(TAG, "ip: " + ip);
                                setNetworkIP(ip);
                            }
                            setNetworkType(NETWORK_MOBILE);
                        }
                        if (activeNetworkInfo.getType() == ConnectivityManager.TYPE_ETHERNET) {
                            CDELog.j(TAG, "LAN network");
                            ip = getLANIP();
                            if (ip != null) {
                                CDELog.j(TAG, "ip: " + ip);
                                setNetworkIP(ip);
                            }
                            setNetworkType(NETWORK_LAN);
                        }
                        if (activeNetworkInfo.getState() == NetworkInfo.State.CONNECTED) {
                            return true;
                        } else {
                            return false;
                        }
                    }
                }
            } catch (Exception e) {
                CDELog.d(TAG, "failed to get network state : " + e.toString());
                return false;
            }
        } else {
            CDELog.j(TAG, "SDK_INT >= Android Q");
            try {
            NetworkCapabilities networkCapabilities = connectivityManager.getNetworkCapabilities(connectivityManager.getActiveNetwork());
            if (networkCapabilities == null) {
                CDELog.d(TAG, "network is not connected, pls check");
            } else if (networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)) {
                CDELog.j(TAG, "wifi network");
                setNetworkType(NETWORK_WIFI);
                WifiManager wifiManager = (WifiManager) activity.getSystemService(Context.WIFI_SERVICE);
                if (wifiManager != null) {
                    WifiInfo wifiInfo = wifiManager.getConnectionInfo();
                    if (wifiInfo != null) {
                        int ipAddress = wifiInfo.getIpAddress();
                        if (ipAddress != 0) {
                            ip = IPIntToString(ipAddress);
                            CDELog.j(TAG, "ip: " + ip);
                            setNetworkIP(ip);
                        }
                    }
                }
            } else if (networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR)) {
                CDELog.j(TAG, "mobile network");
                setNetworkType(NETWORK_MOBILE);
                ip = getMobileIP();
                if (ip != null) {
                    CDELog.j(TAG, "ip: " + ip);
                    setNetworkIP(ip);
                }
            } else if (networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET)) {
                CDELog.j(TAG, "LAN network");
                ip = getLANIP();
                if (ip != null) {
                    CDELog.j(TAG, "ip: " + ip);
                    setNetworkIP(ip);
                }
                setNetworkType(NETWORK_LAN);
            }

            NetworkInfo activeNetworkInfo = connectivityManager.getActiveNetworkInfo();
            if (activeNetworkInfo.getState() == NetworkInfo.State.CONNECTED) {
                return true;
            } else {
                return false;
            }
            } catch (Exception e) {
                CDELog.d(TAG, "failed to get network state : " + e.toString());
                return false;
            }

        }

        return false;
    }

    public static void dumpDeviceInfo() {
        CDELog.j(TAG, "******** Android build informations******\n");
        addBuildField("BOARD", Build.BOARD);
        addBuildField("BOOTLOADER", Build.BOOTLOADER);
        addBuildField("BRAND", Build.BRAND);
        addBuildField("CPU_ABI", Build.CPU_ABI);
        addBuildField("DEVICE", Build.DEVICE);
        addBuildField("DISPLAY", Build.DISPLAY);
        addBuildField("FINGERPRINT", Build.FINGERPRINT);
        addBuildField("HARDWARE", Build.HARDWARE);
        addBuildField("HOST", Build.HOST);
        addBuildField("ID", Build.ID);
        addBuildField("MANUFACTURER", Build.MANUFACTURER);
        addBuildField("MODEL", Build.MODEL);
        addBuildField("PRODUCT", Build.PRODUCT);
        addBuildField("SERIAL", Build.SERIAL);
        addBuildField("TAGS", Build.TAGS);
        addBuildField("TYPE", Build.TYPE);
        addBuildField("USER", Build.USER);
        addBuildField("ANDROID SDK", String.valueOf(android.os.Build.VERSION.SDK_INT));
        addBuildField("OS Version", android.os.Build.VERSION.RELEASE);
        CDELog.j(TAG, "******************************************\n");

    }

    private static void addBuildField(String name, String value) {
        CDELog.j(TAG, "  " + name + ": " + value + "\n");
    }

    public static String bytesToBinaryString(byte[] var0) {
        String var1 = "";

        for (int var2 = 0; var2 < var0.length; ++var2) {
            byte var3 = var0[var2];

            for (int var4 = 0; var4 < 8; ++var4) {
                int var5 = var3 >>> var4 & 1;
                var1 = var1 + var5;
            }

            if (var2 != var0.length - 1) {
                var1 = var1 + " ";
            }
        }

        return var1;
    }


    public static String bytesToHexString(byte[] src) {
        StringBuilder stringBuilder = new StringBuilder("");
        if (src == null || src.length <= 0) {
            return null;
        }
        for (int i = 0; i < src.length; i++) {
            int v = src[i] & 0xFF;
            String hv = Integer.toHexString(v);
            if (hv.length() < 2) {
                stringBuilder.append(0);
            }
            stringBuilder.append(hv);
        }
        return stringBuilder.toString();
    }


    public static String intsToHexString(int[] src) {
        StringBuilder stringBuilder = new StringBuilder("");
        if (src == null || src.length <= 0) {
            return null;
        }
        for (int i = 0; i < src.length; i++) {
            String hv = Integer.toHexString(src[i]);
            stringBuilder.append("0x" + hv + " ");
        }
        return stringBuilder.toString();
    }


    private static byte charToByte(char c) {
        return (byte) "0123456789ABCDEF".indexOf(c);
    }


    public static byte[] hexStringToBytes(String hexString) {
        if (hexString == null || hexString.equals("")) {
            return null;
        }
        hexString = hexString.toUpperCase();
        int length = hexString.length() / 2;
        char[] hexChars = hexString.toCharArray();
        byte[] d = new byte[length];
        for (int i = 0; i < length; i++) {
            int pos = i * 2;
            d[i] = (byte) (charToByte(hexChars[pos]) << 4 | charToByte(hexChars[pos + 1]));
        }
        return d;
    }


    public static void printHexString(byte[] b) {
        for (int i = 0; i < b.length; i++) {
            String hex = Integer.toHexString(b[i] & 0xFF);
            if (hex.length() == 1) {
                hex = '0' + hex;
            }
            System.out.print(hex.toUpperCase());
        }

    }

    public static byte[] getDigest(String text, String algorithm) {
        try {
            MessageDigest md = MessageDigest.getInstance(algorithm);
            md.update(text.getBytes("UTF-8"));
            byte[] bytes = md.digest();
            return bytes;
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    public static String getMD5(String str) {
        return new String(Base64.encode(getDigest(str, MD5), Base64.NO_PADDING | Base64.NO_WRAP | Base64.URL_SAFE));
    }


    public static void writeDataToSDCard(String path, String value) {
        try {
            if (isSdCardAvailable()) {
                File file = new File(path);
            }
        } catch (Exception e) {
            CDELog.d(TAG, "save data failed: " + e.toString());
        }
    }


    public static String readDataFromSDCard(String path) {
        try {
            if (isSdCardAvailable()) {
                File file = new File(path);
            }
        } catch (Exception e) {
            CDELog.d(TAG, "save data failed: " + e.toString());
        }
        return "unknown";
    }


    public static boolean isSdCardAvailable() {
        String dataDirectoryPath = null;
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP_MR1) {
            dataDirectoryPath = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separator + "kantv";
        } else {
            dataDirectoryPath = Environment.getExternalStorageDirectory().getAbsolutePath() +
                    File.separator + "Android" + File.separator + "data" + File.separator + "com.cdeos.kantv" + File.separator + "files" + File.separator + "kantv";
        }

        String state = Environment.getExternalStorageState();
        return (!TextUtils.isEmpty(state) && state.equals("mounted") && Environment.getExternalStorageDirectory() != null);
    }

    public static String getWifiMac() {
        try {
            Enumeration<NetworkInterface> enumeration = NetworkInterface.getNetworkInterfaces();
            if (enumeration == null) {
                return "unknown";
            }
            while (enumeration.hasMoreElements()) {
                NetworkInterface netInterface = enumeration.nextElement();
                if (netInterface.getName().equals("wlan0")) {
                    CDELog.d(TAG, "wifi mac: " + bytesToHexString(netInterface.getHardwareAddress()));
                    return bytesToHexString(netInterface.getHardwareAddress());
                }
            }
        } catch (Exception e) {
            CDELog.j(TAG, "can't get mac : " + e.toString());
        }
        return "unknown";
    }


    public static String getIMEI(Context context) {
        if (context != null) {
            try {
                TelephonyManager telephonyManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
                String deviceId = telephonyManager.getDeviceId();
                if (!TextUtils.isEmpty(deviceId) && !INVALID_DEVICE_ID.equals(deviceId)) {
                    CDELog.j(TAG, "IMEI: " + deviceId);
                    return deviceId;
                }
            } catch (Exception e) {
                CDELog.j(TAG, "can't get IMEI : " + e.toString());
                return "";
            }
        }
        return "";
    }

    public static String getDeviceName() {
        return KANTVDRM.getInstance().ANDROID_JNI_GetDeviceClearIDString();
    }


    public static String getAndroidID(Context context) {
        if (context != null) {
            try {
                String androidId = android.provider.Settings.Secure.getString(context.getContentResolver(), android.provider.Settings.Secure.ANDROID_ID);
                CDELog.j(TAG, "android id : " + androidId);
                if (!TextUtils.isEmpty(androidId) && !INVALID_ANDROID_ID.equals(androidId)) {
                    return androidId;
                }

            } catch (Exception e) {
                CDELog.j(TAG, "can't get AndroidID : " + e.toString());
                return "unknown";
            }
        }
        return "unknown";
    }


    public static String getRandomUUID() {
        String uuid = UUID.randomUUID().toString();
        CDELog.j(TAG, "random uuid : " + uuid);
        return uuid;
    }


    public static String loadDeviceID() {
        String deviceID = readDataFromSDCard(CDEUtils.DEVICE_ID_FILE_PATH);
        return deviceID;
    }

    private static String getID(Context context, int index) {
        switch (index) {
            case 0:
                return getIMEI(context);
            case 1:
                return getDeviceSerial();
            case 2:
                return getAndroidID(context);
            case 3:
                return getWifiMac();
            default:
                CDELog.d(TAG, "can't get id for index: " + index);
                return "";
        }
    }


    public static String generateUniqueID(Activity activity) {
        /*
        Context context = activity.getBaseContext();
        StringBuilder sb = new StringBuilder(256);
        for (int c = 0, i = 0; c < 3 && i < 4; i++) {
            String id = getID(context, i);
            if (!TextUtils.isEmpty(id)) {
                if (c > 0) {
                    sb.append('|');
                }
                sb.append(id);
                c++;
            }
        }

        if (sb.length() == 0) {
            CDELog.j(TAG, "can not get device unique id");
            throw new RuntimeException("can not get device unique id");
        }
        CDELog.j(TAG, "deviceID string: " + sb.toString());
        mUniqueID = getMD5(sb.toString());
         */

        mUniqueID = KANTVDRM.getInstance().ANDROID_JNI_GetDeviceIDString();

        return mUniqueID;
    }

    public static String getUniqueID() {
        /*
        if (!TextUtils.isEmpty(mUniqueID)) {
            return mUniqueID;
        } else {
            CDELog.g(TAG, "can't get device unique id, please check why?");
            return Build.UNKNOWN;
        }
        */
        return KANTVDRM.getInstance().ANDROID_JNI_GetDeviceIDString();
    }

    public static String getUniqueIDString() {
        return KANTVDRM.getInstance().ANDROID_JNI_GetDeviceClearIDString();
    }

    public static String getJavalayerUniqueID() {
        return mJavaLayerUniqueID;
    }

    public static void setJavalayerUniqueID(String idString) {
        mJavaLayerUniqueID = idString;
    }


    public static void copyAssetFile(Context context, String sourceFilePath, String destFilePath) {
        InputStream inStream = null;
        FileOutputStream outStream = null;
        try {
            inStream = context.getAssets().open(sourceFilePath);
            outStream = new FileOutputStream(destFilePath);

            int bytesRead = 0;
            byte[] buf = new byte[4096];
            while ((bytesRead = inStream.read(buf)) != -1) {
                outStream.write(buf, 0, bytesRead);
            }
        } catch (Exception e) {
            CDELog.d(TAG, "error: " + e.toString());
            e.printStackTrace();
        } finally {
            close(inStream);
            close(outStream);
        }
    }

    public static void close(InputStream is) {
        if (is == null) return;
        try {
            is.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void close(OutputStream os) {
        if (os == null) return;
        try {
            os.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static String readTextFromFile(String fileName) {
        File file = new File(fileName);
        BufferedReader reader = null;
        StringBuffer sbf = new StringBuffer();
        try {
            reader = new BufferedReader(new FileReader(file));
            String tempStr;
            while ((tempStr = reader.readLine()) != null) {
                sbf.append(tempStr);
            }
            reader.close();
            return sbf.toString();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e1) {
                    e1.printStackTrace();
                }
            }
        }
        return sbf.toString();
    }

    public static int getScreenOrientation() {
        return mScreenOrientation;
    }

    public static void setScreenOrientation(int screenOrientation) {
        mScreenOrientation = screenOrientation;
    }


    public static int lengthOfEnChStringInvalid(String value) {
        int valueLength = 0;
        //FIXME
        String chinese = "";//"[\u0391-\uFFE5]”;
        for (int i = 0; i < value.length(); i++) {
            String temp = value.substring(i, i + 1);
            if (temp.matches(chinese)) {
                valueLength += 2;
            } else {
                valueLength += 1;
            }
        }
        return valueLength;
    }



    public static int lengthOfEnChString(String value) {
        int length = 0;
        for (int i = 0; i < value.length(); i++) {
            int ascii = Character.codePointAt(value, i);
            if (ascii >= 0 && ascii <= 255)
                length++;
            else
                length += 2;

        }
        return length;
    }


    public static String convertLongString(String url, int lineLength) {
        String tmpUrl = "";
        if (isRunningOnTV()) {
            lineLength = LINE_LENGTH_IN_TV_UI;
        } else {
            if (mScreenOrientation == SCREEN_ORIENTATION_PORTRAIT) {
                //CDELog.d(TAG, "ScreenOrientation == SCREEN_ORIENTATION_PORTRAIT");
                lineLength = LINE_LENGTH_IN_PHONE_UI;
            } else if (mScreenOrientation == SCREEN_ORIENTATION_LANDSCAPE) {
                //CDELog.d(TAG, "ScreenOrientation == SCREEN_ORIENTATION_LANDSCAPE");
                lineLength = LINE_LENGTH_IN_TV_UI;
            } else {
                //CDELog.d(TAG, "ScreenOrientation == SCREEN_ORIENTATION_PORTRAIT");
                lineLength = LINE_LENGTH_IN_PHONE_UI;
            }
        }
        if (url != null) {
            int endPos = 0;
            int beginPos = 0;
            //TODO: mixed English & Chinese
            int otherLength = url.length();
            for (int i = 0; i < (url.length() / lineLength + 1); i++) {
                beginPos = i * lineLength;
                if (otherLength <= lineLength) {
                    endPos = url.length();
                    tmpUrl = tmpUrl + url.substring(beginPos, endPos);
                    break;
                } else {
                    endPos = beginPos + lineLength;
                    tmpUrl = tmpUrl + url.substring(beginPos, endPos) + "\r\n";
                }
                otherLength = otherLength - lineLength;
            }
        }

        return tmpUrl;
    }


    public static void setStartPlayPos(int startPlayPos) {
        mStartPlayPos = startPlayPos;
    }

    public static int getStartPlayPos() {
        return mStartPlayPos;
    }

    public static void setDisableAudioTrack(boolean bDisable) {
        mDisableAudioTrack = bDisable;
        CDELog.d(TAG, "disable audio track:" + mDisableAudioTrack);
    }

    public static boolean getDisableAudioTrack() {
        CDELog.d(TAG, "disable audio track:" + mDisableAudioTrack);
        return mDisableAudioTrack;
    }

    public static void setDisableVideoTrack(boolean bDisable) {
        mDisableVideoTrack = bDisable;
        CDELog.d(TAG, "disable video track:" + mDisableVideoTrack);
    }

    public static boolean getDisableVideoTrack() {
        CDELog.d(TAG, "disable video track:" + mDisableVideoTrack);
        return mDisableVideoTrack;
    }

    public static void setLocalEMS(String localEMS) {
        mLocalEMS = localEMS;
    }

    public static String getLocalEMS() {
        return mLocalEMS;
    }


    public static String getKANTVServer(Context context) {
        String epgURI;

        CDEAssetLoader.copyAssetFile(context, "config.json", CDEAssetLoader.getDataPath(context) + "config.json");
        String configString = CDEAssetLoader.readTextFromFile(CDEAssetLoader.getDataPath(context) + "config.json");
        JSONObject jsonObject = JSON.parseObject(configString);
        CDELog.d(TAG, "get kantvServer URL from config.json: " + jsonObject.getString("kantvServer"));
        epgURI = jsonObject.getString("kantvServer");
        if (epgURI != null) {
            return epgURI;
        } else {
            return CDEUtils.getKANTVServer();
        }
    }


    public static void setKANTVServer(String kantvserver) {
        CDELog.j(TAG, "set kantv server to :" + kantvserver);

        mKANTVServer = kantvserver;
    }

    public static String getKANTVServer() {
        return mKANTVServer;
    }

    public static String getKANTVMasterServer() {
        return mKANTVMasterServer;
    }

    public static String getKANTVMainEPGUrl() {
        return mKANTVUpdateMainEPGUrl;
    }
    public static void setKANTVUpdateAPKUrl(String updateAPKUrl) {
        mKANTVUpdateAPKUrl = updateAPKUrl;
    }

    public static String getKANTVUpdateAPKUrl() {
        return mKANTVUpdateAPKUrl;
    }

    public static void setKANTVUpdateVersionUrl(String updateAPKVersionUrl) {
        mKANTVUpdateAPKVersionUrl = updateAPKVersionUrl;
    }

    public static String getKANTVUpdateVersionUrl() {
        return mKANTVUpdateAPKVersionUrl;
    }

    public static void setKANTVAPKVersion(String version) {
        mKANTVAPKVersion = version;
    }

    public static String getKANTVAPKVersion() {
        return mKANTVAPKVersion;
    }

    public static void setNginxServerUrl(String nginxServerUrl) {
        CDELog.d(TAG, "set nginx server to :" + nginxServerUrl);
        mNginxServerUrl = nginxServerUrl;
    }

    public static String getNginxServerUrl() {
        return mNginxServerUrl;
    }

    public static void setApiGatewayServerUrl(String apiGatewayServerUrl) {
        CDELog.j(TAG, "set API gateway server to :" + apiGatewayServerUrl);
        mApiGatewayServerUrl = apiGatewayServerUrl;
    }

    public static String getApiGatewayServerUrl() {
        return mApiGatewayServerUrl;
    }

    public static void setEpgUpdatedStatus(CDEMediaType mediaType, boolean bUpdatedStatus) {
        CDELog.j(TAG, "set " + mediaType.toString() + " epgUpdatedStatus to:" + bUpdatedStatus);
        if (MEDIA_TV == mediaType) {
            mTVEpgUpdatedStatus = bUpdatedStatus;
        } else if (MEDIA_RADIO == mediaType) {
            mRadioEpgUpdatedStatus = bUpdatedStatus;
        } else if (MEDIA_MOVIE == mediaType) {
            mMovieEpgUpdatedStatus = bUpdatedStatus;
        } else {
            mTVEpgUpdatedStatus = bUpdatedStatus;
        }
    }

    public static boolean getEPGUpdatedStatus(CDEMediaType mediaType) {
        boolean epgUpdatedStatus = false;
        if (CDEMediaType.MEDIA_TV == mediaType) {
            epgUpdatedStatus = mTVEpgUpdatedStatus;
        } else if (CDEMediaType.MEDIA_RADIO == mediaType) {
            epgUpdatedStatus = mRadioEpgUpdatedStatus;
        } else if (CDEMediaType.MEDIA_MOVIE == mediaType) {
            epgUpdatedStatus = mMovieEpgUpdatedStatus;
        } else {
            epgUpdatedStatus = mTVEpgUpdatedStatus;
        }
        CDELog.d(TAG, "epgUpdatedStatus for " + mediaType.toString() + " is: " + epgUpdatedStatus);
        return epgUpdatedStatus;
    }


    public static String getVideoDuration(IMediaPlayer mediaPlayer) {
        String positionTimeString = " ";
        if (mediaPlayer == null)
            return "unknown";

        if (mediaPlayer.getDuration() <= 0) {
            return "unknown";
        }
        long positionTime = mediaPlayer.getDuration() / 1000;

        positionTimeString = String.valueOf(positionTime) + "secs" + " (";

        long hours = positionTime / 3600;
        long minutes = (positionTime - hours * 3600) / 60;
        long seconds = (positionTime - hours * 3600 - minutes * 60);
        long totalMinutes = positionTime / 60;
        positionTimeString = positionTimeString + String.valueOf(totalMinutes) + "M,  ";

        positionTimeString = positionTimeString + String.valueOf(hours) + "H" + String.valueOf(minutes) + "M" + String.valueOf(seconds) + "S" + ")";

        return positionTimeString;
    }

    public static String getVideoPositiontime(IMediaPlayer mediaPlayer) {
        String positionTimeString = " ";
        if (mediaPlayer == null)
            return "unknown";

        long positionTime = mediaPlayer.getCurrentPosition() / 1000;
        if (0 == positionTime) {
            return "unknown";
        }

        positionTimeString = String.valueOf(positionTime) + "secs" + " (";

        long hours = positionTime / 3600;
        long minutes = (positionTime - hours * 3600) / 60;
        long seconds = (positionTime - hours * 3600 - minutes * 60);
        long totalMinutes = positionTime / 60;
        positionTimeString = positionTimeString + String.valueOf(totalMinutes) + "M,  ";

        positionTimeString = positionTimeString + String.valueOf(hours) + "H" + String.valueOf(minutes) + "M" + String.valueOf(seconds) + "S" + ")";

        return positionTimeString;
    }


    public static String buildResolution(int width, int height, int sarNum, int sarDen) {
        StringBuilder sb = new StringBuilder();
        sb.append(width);
        sb.append(" x ");
        sb.append(height);

        if (sarNum > 1 || sarDen > 1) {
            sb.append("[");
            sb.append(sarNum);
            sb.append(":");
            sb.append(sarDen);
            sb.append("]");
        }

        return sb.toString();
    }

    public static String buildTimeMilli(long duration) {
        long total_seconds = duration / 1000;
        long hours = total_seconds / 3600;
        long minutes = (total_seconds % 3600) / 60;
        long seconds = total_seconds % 60;
        if (duration <= 0) {
            return "--:--";
        }
        if (hours >= 100) {
            return String.format(Locale.US, "%d:%02d:%02d", hours, minutes, seconds);
        } else if (hours > 0) {
            return String.format(Locale.US, "%02d:%02d:%02d", hours, minutes, seconds);
        } else {
            return String.format(Locale.US, "%02d:%02d", minutes, seconds);
        }
    }


    public static void setTroubleshootingMode(int troubleshootingMode) {
        if (troubleshootingMode >= TROUBLESHOOTING_MODE_MAX) {
            CDELog.j(TAG, "invalid troubleshooting mode: " + troubleshootingMode);
        }
        mTroubleshootingMode = troubleshootingMode;
    }

    public static int getTroubleshootingMode() {
        return mTroubleshootingMode;
    }

    public static boolean getTroubleShootingMode() {
        if (mTroubleshootingMode != TROUBLESHOOTING_MODE_DISABLE)
            return true;
        else
            return false;
    }

    public static boolean getExpertMode() {
        return mDevelopmentMode;
    }

    public static void setExpertMode(boolean expertMode) {
        mDevelopmentMode = expertMode;
    }

    public static void setReleaseMode(boolean releaseMode) {
        mReleaseMode = releaseMode;
    }

    public static boolean getReleaseMode() {
        return mReleaseMode;
    }


    public static String getTEEConfigFilePath(Context context) {
        //return "/vendor/etc/mediadrm";
        return "/tmp";
    }

    public static String getTEEConfigFile(Context context) {
        return getTEEConfigFilePath(context) + "/" + "config.json";
    }


    public static void storeTEEConfigFile(Context context) {
        CDELog.d(TAG, "store TEE config file, src: " + getTEEConfigFile(context) + " dst: " + CDEAssetLoader.getDataPath(context) + "config.json");
        CDEAssetLoader.copyAssetFile(context, getTEEConfigFile(context), CDEAssetLoader.getDataPath(context) + "config.json");
    }


    @Deprecated
    //FIXME: ANR
    public static void setProp(String command) {
        try {
            CDELog.d(TAG, "command: " + command);
            Process process = Runtime.getRuntime().exec(command);
            InputStreamReader inputStreamReader = new InputStreamReader(process.getInputStream());
            BufferedReader bufferedReader = new BufferedReader(inputStreamReader);
            //Integer.parseInt(bufferedReader.readLine());
            if (bufferedReader.readLine() == null) {
                CDELog.j(TAG, command + " success");
            } else {
                CDELog.j(TAG, command + " failed");
                CDELog.j(TAG, "command result:" + bufferedReader.readLine());
            }
        } catch (IOException e) {
            e.printStackTrace();
            CDELog.j(TAG, "error: " + e.toString());
        }
    }

    @Deprecated
    public static int getProp(String command) {
        try {
            CDELog.j(TAG, "command: " + command);
            Process process = Runtime.getRuntime().exec(command);
            InputStreamReader inputStreamReader = new InputStreamReader(process.getInputStream());
            BufferedReader bufferedReader = new BufferedReader(inputStreamReader);
            if (bufferedReader == null) {
                CDELog.j(TAG, command + " failed");
                return -1;
            }
            String commandResult = bufferedReader.readLine();
            if (commandResult == null) {
                CDELog.j(TAG, command + " failed");
                return -1;
            } else {
                CDELog.j(TAG, "command result: " + commandResult);
                CDELog.j(TAG, command + " return: " + Integer.parseInt(commandResult));
                return Integer.parseInt(commandResult);
            }
        } catch (IOException e) {
            e.printStackTrace();
            CDELog.j(TAG, "error: " + e.toString());
            return -1;
        }
    }


    public static int isClearContent() {
        return mKANTVDRM.ANDROID_DRM_IsClearContent();
    }


    public static void setDevMode(String assetPath, boolean bDevMode, int dumpMode, int playEngine, int dumpDuration, int dumpSize, int dumpCounts) {
        CDELog.j(TAG, "set dev mode: " + bDevMode + " , dump mode: " + dumpMode + " , play engine: " + playEngine + " , drm system: " + getDrmName(getDrmScheme())
                + " , dump duration: " + dumpDuration + " , dump size: " + dumpSize + " , dump counts: " + dumpCounts);
        mKANTVDRM.ANDROID_JNI_SetDevMode(assetPath, bDevMode, dumpMode, playEngine, CDEUtils.getDrmScheme(), dumpDuration, dumpSize, dumpCounts);
    }

    public static void setDevMode(String assetPath, boolean bDevMode, int dumpMode, int playEngine, int drmScheme, int dumpDuration, int dumpSize, int dumpCounts) {
        CDELog.j(TAG, "set dev mode: " + bDevMode + " , dump mode: " + dumpMode + " , play engine: " + playEngine + " , drm system: " + getDrmName(drmScheme)
                + " , dump duration: " + dumpDuration + " , dump size: " + dumpSize + " , dump counts: " + dumpCounts);
        mKANTVDRM.ANDROID_JNI_SetDevMode(assetPath, bDevMode, dumpMode, playEngine, drmScheme, dumpDuration, dumpSize, dumpCounts);
    }


    public static void setRecordConfig(String recordPath, int recordMode, int recordFormat, int recordVideoCodec, int recordMaxDuration, int recordMaxSize) {
        CDELog.j(TAG, "record path " + recordPath + ", record mode: " + recordMode + " , record video codec: " + recordVideoCodec + " ,record format: " + recordFormat
                + " , record duration: " + recordMaxDuration + " , record size: " + recordMaxSize);
        mKANTVDRM.ANDROID_JNI_SetRecordConfig(recordPath, recordMode, recordFormat, recordVideoCodec, recordMaxDuration, recordMaxSize);
    }

    public static KANTVDRM getKANTVDRMInstance() {
        return mKANTVDRM;
    }


    public static void setDecryptMode(int decryptMode) {
        CDELog.d(TAG, "set decrypt mode to " + decryptMode + " (" + getDecryptModeString(decryptMode) + ")");

        if ((decryptMode != DECRYPT_NONE) && (decryptMode != DECRYPT_SOFT) && (decryptMode != DECRYPT_TEE_NONSVP) && (decryptMode != DECRYPT_TEE_SVP) && (decryptMode != DECRYPT_TEE)) {
            CDELog.j(TAG, "invalid decrypt mode: " + decryptMode);
            return;
        }
        mDecryptMode = decryptMode;
        mKANTVDRM.ANDROID_JNI_SetDecryptMode(decryptMode);
    }


    public static int getDecryptMode() {
        int decryptMode = mKANTVDRM.ANDROID_JNI_GetDecryptMode();
        mDecryptMode = decryptMode;
        return mDecryptMode;
    }


    public static String getDecryptModeString(int decryptMode) {
        switch (decryptMode) {
            case DECRYPT_NONE:
                return "NO-Decrypt";
            case DECRYPT_SOFT:
                return "Software-Decrypt";
            case DECRYPT_TEE_NONSVP:
                return "TEE-nonSVP";
            case DECRYPT_TEE_SVP:
                return "TEE-SVP";
            case DECRYPT_TEE:
                return "TEE";
            default:
                return "unknown";
        }
    }

    public static int getDrmpluginType() {
        int drmpluginType = mKANTVDRM.ANDROID_JNI_GetDrmpluginType();
        mDrmpluginType = drmpluginType;
        return mDrmpluginType;
    }

    public static String getDrmpluginTypeString(int drmpluginType) {
        switch (drmpluginType) {
            case DRMPLUGIN_TYPE_SOFT:
                return "Software-Decrypt";
            case DRMPLUGIN_TYPE_TEE_NONSVP:
                return "TEE-nonSVP";
            case DRMPLUGIN_TYPE_TEE_SVP:
                return "TEE-SVP";
            default:
                return "unknown";
        }
    }


    public static String getDrminfoString(Activity activity, IMediaPlayer mediaPlayer, int drmInfoID, int encryptInfoID, int decryptID) {
        String drmInfo = null;

        if (mediaPlayer instanceof AndroidMediaPlayer) {
            drmInfo = "Encrypt content(" + activity.getBaseContext().getString(drmInfoID) + ": " + CDEUtils.getDrmSchemeName()
                    + " , " + activity.getBaseContext().getString(decryptID)
                    + ": " + CDEUtils.getDrmpluginTypeString(CDEUtils.getDrmpluginType()) + ")";
        } else if (mediaPlayer instanceof FFmpegMediaPlayer) {
            drmInfo = "Encyrypt content(" + activity.getBaseContext().getString(drmInfoID) + ": " + CDEUtils.getDrmSchemeName()
                    + " , " + activity.getBaseContext().getString(decryptID)
                    + ": " + CDEUtils.getDecryptModeString(CDEUtils.getDecryptMode()) + ")";
        } else {
            if (CDEUtils.getDecryptMode() == CDEUtils.DECRYPT_TEE) {
                drmInfo = "Encrypt content(" + activity.getBaseContext().getString(drmInfoID) + ": " + CDEUtils.getDrmSchemeName()
                        + " , " + activity.getBaseContext().getString(encryptInfoID)
                        + ": " + CDEUtils.getEncryptSchemeName(CDEUtils.getEncryptScheme())
                        + " , " + activity.getBaseContext().getString(decryptID)
                        + ": " + CDEUtils.getDrmpluginTypeString(CDEUtils.getDrmpluginType()) + ")";
            } else {
                drmInfo = "Encrypt content(" + activity.getBaseContext().getString(drmInfoID) + ": " + CDEUtils.getDrmSchemeName()
                        + " , " + activity.getBaseContext().getString(encryptInfoID)
                        + ": " + CDEUtils.getEncryptSchemeName(CDEUtils.getEncryptScheme())
                        + " , " + activity.getBaseContext().getString(decryptID)
                        + ": " + CDEUtils.getDecryptModeString(CDEUtils.getDecryptMode()) + ")";
            }
        }

        return drmInfo;
    }


    public static String getAmlogicKernelCFGContent(String kernelCFGFile) {
        String result = " ";
        if (!isRunningOnAmlogicBox())
            return result;

        try {
            if (new File(kernelCFGFile).exists()) {
                BufferedReader brKernelCFG = new BufferedReader(new FileReader(new File(kernelCFGFile)));
                String aLine;

                if (brKernelCFG != null) {
                    while ((aLine = brKernelCFG.readLine()) != null) {
                        result = result + aLine + "\n";
                    }
                    brKernelCFG.close();
                }
            }
        } catch (Exception ex) {
            ex.printStackTrace();
            CDELog.j(TAG, "error occoured: " + ex.toString());
        }

        return result;
    }


    public static String getAmlogicKernelCFGContentWithLines(String kernelCFGFile, int lineCounts) {
        String result = " ";
        if (!isRunningOnAmlogicBox())
            return result;

        int lines = 0;
        try {
            if (new File(kernelCFGFile).exists()) {
                BufferedReader brKernelCFG = new BufferedReader(new FileReader(new File(kernelCFGFile)));
                String aLine;

                if (brKernelCFG != null) {
                    while ((aLine = brKernelCFG.readLine()) != null) {
                        lines++;
                        if (lines > lineCounts) {
                            brKernelCFG.close();
                            break;
                        }
                        result = result + aLine + "\n";
                    }
                    brKernelCFG.close();
                }
            }
        } catch (Exception ex) {
            ex.printStackTrace();
            CDELog.j(TAG, "error occoured: " + ex.toString());
        }

        return result;
    }


    private static String getAmlogicCodecMMInfoField(String codecMMInfo, String field_name) {
        if (codecMMInfo == null || field_name == null) {
            return " ";
        }

        String findStr = field_name + "=";
        int stringStart = codecMMInfo.indexOf(findStr);
        if (stringStart < 0) {
            findStr = field_name + "=";
            stringStart = codecMMInfo.indexOf(findStr);
            if (stringStart < 0) {
                return " ";
            }
        }
        int start = stringStart + findStr.length();
        int end = codecMMInfo.indexOf("\n", start);

        return codecMMInfo.substring(start, end);
    }

    public static String getAmlogicCodecMMInfo() {
        String amlogicCodecMMConfigFile = "/sys/class/codec_mm/config";
        String codecMMInfo = getAmlogicKernelCFGContent(amlogicCodecMMConfigFile);
        if ((codecMMInfo == null) || (codecMMInfo.isEmpty()))
            return " ";

        String default_tvp_size = getAmlogicCodecMMInfoField(codecMMInfo, "default_tvp_size");
        String default_tvp_4k_size = getAmlogicCodecMMInfoField(codecMMInfo, "default_tvp_4k_size");
        String default_cma_res_size = getAmlogicCodecMMInfoField(codecMMInfo, "default_cma_res_size");
        String amlogicCodecMMInfoString = "codec_mm.default_tvp_size            :" + default_tvp_size + " (" + ((Integer.parseInt(default_tvp_size)) >> 20) + "MB)" + "\r\n"
                + "codec_mm.default_tvp_4k_size     :" + default_tvp_4k_size + " (" + ((Integer.parseInt(default_tvp_4k_size)) >> 20) + "MB)" + "\r\n"
                + "codec_mm.default_cma_res_size :" + default_cma_res_size + " (" + ((Integer.parseInt(default_cma_res_size)) >> 20) + "MB)" + "\r\n";

        CDELog.d(TAG, "default_tvp_size       :" + default_tvp_size);
        CDELog.d(TAG, "default_tvp_4k_size    :" + default_tvp_4k_size);
        CDELog.d(TAG, "default_cma_res_size   : " + default_cma_res_size);

        return amlogicCodecMMInfoString;
    }

    public static String getAmlogicTVPEnableInfo() {
        String amlogicCodecTVPEanbleConfigFile = "/sys/class/codec_mm/tvp_enable";
        String tvpEnableInfo = getAmlogicKernelCFGContent(amlogicCodecTVPEanbleConfigFile);
        if ((tvpEnableInfo == null) || (tvpEnableInfo.isEmpty()))
            return " ";

        String findStr = "tvp_flag" + "=";
        int stringStart = tvpEnableInfo.indexOf(findStr);
        if (stringStart < 0) {
            findStr = "tvp_flag" + "=";
            stringStart = tvpEnableInfo.indexOf(findStr);
            if (stringStart < 0) {
                return " ";
            }
        }
        int start = stringStart + findStr.length();
        int end = tvpEnableInfo.indexOf("\n", start);

        String amlogicCodecTVPInfoString = "tvp_flag=" + tvpEnableInfo.substring(start, end)
                + "  ( 0: disable tvp; 1: enable tvp for 1080p; 2:enable tvp for 4k )" + "\r\n";

        return amlogicCodecTVPInfoString;
    }


    public static String formatedDurationMilli(long duration) {
        if (duration >= 1000) {
            return String.format(Locale.US, "%.2f sec", ((float) duration) / 1000);
        } else {
            return String.format(Locale.US, "%d msec", duration);
        }
    }

    public static String formatedSize(long bytes) {

        if (bytes <= 0) {
            return "0 Bytes";
        }

        float bytes_per_sec = ((float) bytes) ;
        if (bytes_per_sec >= 1024 * 1024) {
            return String.format(Locale.US, "%.2f MB", ((float) bytes_per_sec) / 1024 / 1024);
        } else if (bytes_per_sec >= 1024) {
            return String.format(Locale.US, "%.1f KB", ((float) bytes_per_sec) / 1024);
        } else {
            return String.format(Locale.US, "%d Bytes", (long) bytes_per_sec);
        }
    }


    public static String formatedBitRate(long bytes, long elapsed_milli) {
        if (elapsed_milli <= 0) {
            return "0 b/s";
        }

        if (bytes <= 0) {
            return "0 b/s";
        }

        float bits_per_sec = ((float) (bytes * 8)) * 1000.f / elapsed_milli;
        if (bits_per_sec >= 1000 * 1000) {
            return String.format(Locale.US, "%.2f Mb/s", ((float) bits_per_sec) / 1000 / 1000);
        } else if (bits_per_sec >= 1000) {
            return String.format(Locale.US, "%.1f Kb/s", ((float) bits_per_sec) / 1000);
        } else {
            return String.format(Locale.US, "%d b/s", (long) bits_per_sec);
        }
    }


    public static String formatedSpeed(long bytes, long elapsed_milli) {
        if (elapsed_milli <= 0) {
            return "0 B/s";
        }

        if (bytes <= 0) {
            return "0 B/s";
        }

        float bytes_per_sec = ((float) bytes) * 1000.f / elapsed_milli;
        if (bytes_per_sec >= 1000 * 1000) {
            return String.format(Locale.US, "%.2f MB/s", ((float) bytes_per_sec) / 1000 / 1000);
        } else if (bytes_per_sec >= 1000) {
            return String.format(Locale.US, "%.1f KB/s", ((float) bytes_per_sec) / 1000);
        } else {
            return String.format(Locale.US, "%d B/s", (long) bytes_per_sec);
        }
    }


    public static void setEnableDumpVideoES(boolean bEnableDumpES) {
        mEnableDumpVideoES = bEnableDumpES;
    }

    public static boolean getEnableDumpVideoES() {
        return mEnableDumpVideoES;
    }

    public static void setEnableDumpAudioES(boolean bEnableDumpES) {
        mEnableDumpAudioES = bEnableDumpES;
    }

    public static boolean getEnableDumpAudioES() {
        return mEnableDumpAudioES;
    }


    public static void setDumpDuration(int dumpDuration) {
        mDumpDuration = dumpDuration;
    }

    public static int getDumpDuration() {
        return mDumpDuration;
    }

    public static void setDumpSize(int dumpSize) {
        mDumpSize = dumpSize;
    }

    public static int getDumpSize() {
        return mDumpSize;
    }

    public static void setDumpCounts(int dumpCounts) {
        mDumpCounts = dumpCounts;
    }

    public static int getDumpCounts() {
        return mDumpCounts;
    }


    public static void createPerfFile(String filename) {
        try {
            File destFile = new File(filename);
            if (!destFile.exists()) {
                destFile.createNewFile();
            }
        } catch (Exception e) {
            CDELog.j(TAG, "error: " + e.toString());
            e.printStackTrace();
        }
    }

    public static int getRandomNumberInRange(int min, int max) {
        if (min >= max) {
            CDELog.j(TAG, "invalid param");
            return 0;
        }

        Random random = new Random();
        return random.nextInt((max - min) + 1) + min;
    }

    public static long perfGetPlaybackBeginTime() {
        mBeginPlaybackTime = System.currentTimeMillis();
        return mBeginPlaybackTime;
    }

    public static long getPlaybackBeginTime() {
        if (mBeginPlaybackTime == 0) {
            CDELog.j(TAG, "it shouldn't happen, perfGetPlaybackBeginTime should be called firstly");
        }
        return mBeginPlaybackTime;
    }

    public static long perfGetRecordBeginTime() {
        mBeginRecordTime = System.currentTimeMillis();
        return mBeginRecordTime;
    }
    public static long getRecordBeginTime() {
        if (mBeginRecordTime == 0) {
            CDELog.j(TAG, "it shouldn't happen, perfGetRecordBeginTime should be called firstly");
        }
        return mBeginRecordTime;
    }

    public static long perfGetRenderedTimeOfFirstVideoFrame() {
        mFirstVideoFrameRenderedTime = System.currentTimeMillis();
        return mFirstVideoFrameRenderedTime;
    }

    public static long perfGetFirstVideoFrameTime() {
        return mFirstVideoFrameRenderedTime - mBeginPlaybackTime;
    }


    public static void perfResetLoopPlayBeginTime() {
        mLoopPlayBeginTime = 0;
        mLoopPlayCounts = 0;
        mLoopPlayErrorCounts = 0;
    }

    public static long perfGetLoopPlayBeginTime() {
        if (0 == mLoopPlayBeginTime) {
            mLoopPlayBeginTime = System.currentTimeMillis();
        }
        return mLoopPlayBeginTime;
    }


    public static long perfGetLoopPlayElapseTime() {
        return System.currentTimeMillis() - mLoopPlayBeginTime;
    }

    public static String perfGetLoopPlayElapseTimeString() {
        long elapseTime = System.currentTimeMillis() - mLoopPlayBeginTime;
        elapseTime /= 1000;

        long hours = elapseTime / 3600;
        long minutes = (elapseTime - hours * 3600) / 60;
        long seconds = (elapseTime - hours * 3600 - minutes * 60);
        String tmpString = " ";
        if (elapseTime > 3600) {
            tmpString = String.valueOf(hours) + "H" + String.valueOf(minutes) + "M" + String.valueOf(seconds) + "S";
        } else if (elapseTime > 60) {
            tmpString = String.valueOf(minutes) + "M" + String.valueOf(seconds) + "S";
        } else {
            tmpString = String.valueOf(seconds) + "S";
        }

        return tmpString;
    }

    public static long perfGetLoopPlayCounts() {
        return mLoopPlayCounts + 1;
    }

    public static long perfGetLoopPlayErrorCounts() {
        return mLoopPlayErrorCounts;
    }

    public static void perfUpdateLoopPlayCounts() {
        mLoopPlayCounts += 1;
    }

    public static void perfUpdateLoopPlayErrorCounts() {
        mLoopPlayErrorCounts += 1;
    }

    public static boolean isPermissionGranted() {
        return mIsPermissionGranted;
    }

    public static boolean getPermissionGranted() {
        return mIsPermissionGranted;
    }

    public static void setPermissionGranted(boolean isPermissionGranted) {
        mIsPermissionGranted = isPermissionGranted;
    }


    public static String getRunningServicesInfo(Context context) {
        StringBuffer serviceInfo = new StringBuffer();
        final ActivityManager activityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningServiceInfo> services = activityManager.getRunningServices(100);

        Iterator<ActivityManager.RunningServiceInfo> l = services.iterator();
        while (l.hasNext()) {
            ActivityManager.RunningServiceInfo si = (ActivityManager.RunningServiceInfo) l.next();
            serviceInfo.append("pid: ").append(si.pid);
            serviceInfo.append("\nprocess: ").append(si.process);
            serviceInfo.append("\nservice: ").append(si.service);
            serviceInfo.append("\nname: ").append(si.getClass().toString());
            serviceInfo.append("\n\n");
        }
        return serviceInfo.toString();
    }

    public static boolean getUsingFFmpegCodec() {
        return mUsingFFmpegCodec;
    }

    public static void setUsingFFmpegCodec(boolean isUsingFFmpegCodec) {
        mUsingFFmpegCodec = isUsingFFmpegCodec;
    }

    public static boolean getEnableMultiDRM() {
        return mEnableMultiDRM;
    }

    public static void setEnableMultiDRM(boolean enableMultiDRM) {
        mEnableMultiDRM = enableMultiDRM;
    }

    public static boolean getEnableWisePlay() {
        return mEnableWisePlay;
    }

    public static void setEnableWisePlay(boolean enableWisePlay) {
        mEnableWisePlay = enableWisePlay;
    }

    public static void setDrmSchemeName(String drmSchemeName) {
        mDrmSchemeName = drmSchemeName;
    }

    public static String getDrmSchemeName() {
        return mDrmSchemeName;
    }

    public static String getDrmName(UUID schemeUuid) {
        if (schemeUuid.equals(COMMON_PSSH_UUID)) {
            return "cenc";
        } else if (schemeUuid.equals(CLEARKEY_UUID)) {
            return "clearkey";
        } else if (schemeUuid.equals(PLAYREADY_UUID)) {
            return "playready";
        } else if (schemeUuid.equals(WIDEVINE_UUID)) {
            return "widevine";
        } else if (schemeUuid.equals(CHINADRM_UUID)) {
            return "chinadrm";
        } else if (schemeUuid.equals(WISEPLAY_UUID)) {
            return "wiseplay";
        } else if (schemeUuid.equals(UUID_NIL)) {
            return "universal";
        } else {
            return "unknown (" + schemeUuid + ")";
        }
    }

    public static String getDrmName(int drmScheme) {
        switch (drmScheme) {
            case DRM_SCHEME_CHINADRM:
                return "chinadrm";
            case DRM_SCHEME_WISEPLAY:
                return "wiseplay";
            case DRM_SCHEME_FAIRPLAY:
                return "fairplay";
            case DRM_SCHEME_PLAYREADY:
                return "playready";
            case DRM_SCHEME_WIDEVINE:
                return "widevine";
            case DRM_SCHEME_MARLIN:
                return "marlin";
            default:
                break;
        }
        return "unknown drm scheme";
    }

    public static void setDrmScheme(int drmScheme) {
        CDELog.d(TAG, "set drm scheme to " + drmScheme + " (" + getDrmName(drmScheme) + ")");

        if (drmScheme > DRM_SCHEME_MAX || drmScheme < 0) {
            CDELog.j(TAG, "invalid drm scheme: " + drmScheme);
            return;
        }
        mDrmScheme = drmScheme;
    }

    public static int getDrmScheme() {
        return mDrmScheme;
    }

    public static String getEncryptSchemeName(int encryptScheme) {
        switch (encryptScheme) {
            case SM4SampleEncryptionMod:
                return "sample-sm4-cbc";
            case SM4CBCEncryptionMod:
                return "sm4-cbc";
            case AES_0_EncryptionMod:
            case AESSampleEncryptionMod:
                return "sample-aes-cbc";
            case AESCBCEncryptionMod:
                return "aes-cbc";
            case SM4_0_EncryptionMod:
                return "sample-sm4-cbc-0";
            case SM4_1_EncryptionMod:
                return "sample-sm4-cbc-1";
            default:
                break;
        }

        return "unknown";
    }

    public static void setEncryptScheme(int encryptScheme) {
        CDELog.j(TAG, "set encrypt scheme to " + encryptScheme + " (" + getEncryptSchemeName(encryptScheme) + ")");
        mEncryptScheme = encryptScheme;
    }

    public static int getEncryptScheme() {
        int encryptScheme = mKANTVDRM.ANDROID_JNI_GetEncryptScheme();
        mEncryptScheme = encryptScheme;
        return mEncryptScheme;
    }

    public static void setDrmLicenseURL(String drmLicenseURL) {
        mDrmLicenseURL = drmLicenseURL;
    }

    public static String getDrmLicenseURL() {
        return mDrmLicenseURL;
    }

    public static void setHLSPlaylistType(int playlistType) {
        if (playlistType < PLAYLIST_TYPE_UNKNOWN)
            playlistType = PLAYLIST_TYPE_UNKNOWN;
        if (playlistType > PLAYLIST_TYPE_LIVE)
            playlistType = PLAYLIST_TYPE_LIVE;

        mHLSPlaylistType = playlistType;
    }

    public static int getHLSPlaylistType() {
        return mHLSPlaylistType;
    }

    public static String getHLSPlaylistTypeString() {
        switch (mHLSPlaylistType) {
            case PLAYLIST_TYPE_UNKNOWN:
                return "unknown";
            case PLAYLIST_TYPE_LIVE:
                return "live";
            case PLAYLIST_TYPE_VOD:
            default:
                return "vod";
        }
    }

    public static String getPlayEngineName(int playEngine) {
        switch (playEngine) {
            case PV_PLAYERENGINE__FFmpeg:
                return "ffmpeg";
            case PV_PLAYERENGINE__Exoplayer:
                return "exoplayer";
            case PV_PLAYERENGINE__AndroidMediaPlayer:
                return "androidmediaplayer";
            case PV_PLAYERENGINE__GSTREAMER:
                return "gstreamer";
            default:
                return "unknown";
        }
    }

    public static void setPlayEngine(int playEngine) {
        mPlayEngine = playEngine;
    }

    public static int getPlayEngine() {
        return mPlayEngine;
    }


    //https://github.com/google/ExoPlayer/pull/2921/files
    public static final byte[] AES_CBC(byte[] content, byte[] keyBytes, byte[] iv, boolean bEncrypt) {
        try {
            KeyGenerator keyGenerator = KeyGenerator.getInstance("AES");
            keyGenerator.init(128, new SecureRandom(keyBytes));

            SecretKey key = new SecretKeySpec(keyBytes, "AES");
            Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");
            bEncrypt = false;
            if (bEncrypt)
                cipher.init(Cipher.ENCRYPT_MODE, key, new IvParameterSpec(iv));
            else
                cipher.init(Cipher.DECRYPT_MODE, key, new IvParameterSpec(iv));

            byte[] result = cipher.doFinal(content);
            return result;
        } catch (Exception e) {
            System.out.println("exception:" + e.toString());
            CDELog.j(TAG, "AES_CBC failed:" + e.toString());
        }
        return null;
    }

    public static boolean isOfflineDRM() {
        return mIsOfflineDRM;
    }

    public static void setOfflineDRM(boolean bOfflineDRM) {
        mIsOfflineDRM = bOfflineDRM;
    }

    public static String base64Decode(String base64String) {
        try {
            byte[] base64Content = Base64.decode(base64String.getBytes(), Base64.NO_WRAP);
            String tmpString = new String(base64Content, "utf-8");
            CDELog.j(TAG, "base64 decode: " + base64String + " ====> " + tmpString);
            return tmpString;
        } catch (Exception e) {
            CDELog.j(TAG, "base64Decode error:" + e.toString());
        }
        return "unknown base64 content";
    }

    public static void copyFile(File source, File dest) throws IOException {
        FileChannel inputChannel = null;
        FileChannel outputChannel = null;
        try {
            inputChannel = new FileInputStream(source).getChannel();
            outputChannel = new FileOutputStream(dest).getChannel();
            outputChannel.transferFrom(inputChannel, 0, inputChannel.size());
        } catch (IOException ex) {
            CDELog.j(TAG, "copy file failed: " + ex.toString());
        } finally {
            inputChannel.close();
            outputChannel.close();
        }
    }

    public static void copyFile(String srcFilePath, String destFilePath) {
        CDELog.j(TAG, "src path: " + srcFilePath);
        CDELog.j(TAG, "dst path: " + destFilePath);
        InputStream inStream = null;
        FileOutputStream outStream = null;
        try {
            File destFile = new File(destFilePath);
            if (!destFile.exists()) {
                destFile.createNewFile();
            }
            inStream = new FileInputStream(srcFilePath);
            outStream = new FileOutputStream(destFilePath);

            int bytesRead = 0;
            byte[] buf = new byte[2048];
            while ((bytesRead = inStream.read(buf)) != -1) {
                outStream.write(buf, 0, bytesRead);
            }
        } catch (Exception e) {
            CDELog.j(TAG, "error: " + e.toString());
            e.printStackTrace();
        } finally {
            close(inStream);
            close(outStream);
        }
    }


    public static void bytesToFile(byte[] bytes, String filePath, String fileName) {
        BufferedOutputStream bos = null;
        FileOutputStream fos = null;
        File file = null;
        CDELog.j(TAG, "filePath:" + filePath + ",fileName:" + fileName);
        try {

            file = new File(filePath + fileName);
            CDELog.j(TAG, "full file name:" + file.getAbsolutePath());
            if (!file.getParentFile().exists()) {
                file.getParentFile().mkdirs();
            }
            fos = new FileOutputStream(file);
            bos = new BufferedOutputStream(fos);
            bos.write(bytes);
        } catch (Exception e) {
            e.printStackTrace();
            CDELog.j(TAG, "failed:" + e.toString());
        } finally {
            if (bos != null) {
                try {
                    bos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                    CDELog.j(TAG, "failed:" + e.toString());
                }
            }
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                    CDELog.j(TAG, "failed:" + e.toString());
                }
            }
        }
    }


    public static boolean isLinebreak(int c) {
        return c == '\n' || c == '\r';
    }

    public static void closeQuietly(@Nullable Closeable closeable) {
        try {
            if (closeable != null) {
                closeable.close();
            }
        } catch (IOException e) {
            CDELog.j(TAG, e.toString());
        }
    }


    //borrow from Google Exoplayer
    private static final String PLAYLIST_HEADER = "#EXTM3U";
    private static final String TAG_PREFIX = "#EXT";

    private static final String TAG_VERSION = "#EXT-X-VERSION";
    private static final String TAG_PLAYLIST_TYPE = "#EXT-X-PLAYLIST-TYPE";
    private static final String TAG_DEFINE = "#EXT-X-DEFINE";
    private static final String TAG_SERVER_CONTROL = "#EXT-X-SERVER-CONTROL";
    private static final String TAG_STREAM_INF = "#EXT-X-STREAM-INF";
    private static final String TAG_PART_INF = "#EXT-X-PART-INF";
    private static final String TAG_PART = "#EXT-X-PART";
    private static final String TAG_I_FRAME_STREAM_INF = "#EXT-X-I-FRAME-STREAM-INF";
    private static final String TAG_IFRAME = "#EXT-X-I-FRAMES-ONLY";
    private static final String TAG_MEDIA = "#EXT-X-MEDIA";
    private static final String TAG_TARGET_DURATION = "#EXT-X-TARGETDURATION";
    private static final String TAG_DISCONTINUITY = "#EXT-X-DISCONTINUITY";
    private static final String TAG_DISCONTINUITY_SEQUENCE = "#EXT-X-DISCONTINUITY-SEQUENCE";
    private static final String TAG_PROGRAM_DATE_TIME = "#EXT-X-PROGRAM-DATE-TIME";
    private static final String TAG_INIT_SEGMENT = "#EXT-X-MAP";
    private static final String TAG_INDEPENDENT_SEGMENTS = "#EXT-X-INDEPENDENT-SEGMENTS";
    private static final String TAG_MEDIA_DURATION = "#EXTINF";
    private static final String TAG_MEDIA_SEQUENCE = "#EXT-X-MEDIA-SEQUENCE";
    private static final String TAG_START = "#EXT-X-START";
    private static final String TAG_ENDLIST = "#EXT-X-ENDLIST";
    private static final String TAG_KEY = "#EXT-X-KEY";
    private static final String TAG_SESSION_KEY = "#EXT-X-SESSION-KEY";
    private static final String TAG_BYTERANGE = "#EXT-X-BYTERANGE";
    private static final String TAG_GAP = "#EXT-X-GAP";
    private static final String TAG_SKIP = "#EXT-X-SKIP";
    private static final String TAG_PRELOAD_HINT = "#EXT-X-PRELOAD-HINT";
    private static final String TAG_RENDITION_REPORT = "#EXT-X-RENDITION-REPORT";

    private static final String TYPE_AUDIO = "AUDIO";
    private static final String TYPE_VIDEO = "VIDEO";
    private static final String TYPE_SUBTITLES = "SUBTITLES";
    private static final String TYPE_CLOSED_CAPTIONS = "CLOSED-CAPTIONS";
    private static final String TYPE_PART = "PART";
    private static final String TYPE_MAP = "MAP";

    private static boolean checkPlaylistHeader(BufferedReader reader) throws IOException {
        int last = reader.read();
        if (last == 0xEF) {
            if (reader.read() != 0xBB || reader.read() != 0xBF) {
                return false;
            }
            // The playlist contains a Byte Order Mark, which gets discarded.
            last = reader.read();
        }
        last = skipIgnorableWhitespace(reader, true, last);
        int playlistHeaderLength = PLAYLIST_HEADER.length();
        for (int i = 0; i < playlistHeaderLength; i++) {
            if (last != PLAYLIST_HEADER.charAt(i)) {
                return false;
            }
            last = reader.read();
        }
        last = skipIgnorableWhitespace(reader, false, last);
        return CDEUtils.isLinebreak(last);
    }

    private static int skipIgnorableWhitespace(BufferedReader reader, boolean skipLinebreaks, int c)
            throws IOException {
        while (c != -1 && Character.isWhitespace(c) && (skipLinebreaks || !CDEUtils.isLinebreak(c))) {
            c = reader.read();
        }
        return c;
    }



    public static List<CDEContentDescriptor> loadIPTVProgram(Context mContext, String epgUrl) {
        List<CDEContentDescriptor> descriptors = new ArrayList<CDEContentDescriptor>();
        long handleBeginTime = 0;
        long handleEndTime = 0;

        if (epgUrl == null) {
            CDELog.j(TAG, "epg url is empty");
            return null;
        }
        File epgLocalFile = null;
        if (epgUrl.startsWith("http://")) {
            handleBeginTime = System.currentTimeMillis();
            CDELog.j(TAG, "iptv url:" + epgUrl);
            int index = epgUrl.lastIndexOf('/');
            if (index == -1) {
                CDELog.j(TAG, "error, pls check");
                return null;
            }

            String epgFileName = epgUrl.substring(index + 1);
            CDELog.j(TAG, "epgFileName:" + epgFileName);
            Uri uri = Uri.parse(epgUrl);
            epgLocalFile = new File(CDEUtils.getDataPath(mContext), epgFileName);
            CDELog.j(TAG, "epgLocalFile: " + epgLocalFile.getAbsolutePath());
            //if (!epgLocalFile.exists())
            if (true) {
                int downloadResult = mKANTVDRM.ANDROID_JNI_DownloadFile(uri.toString(), epgLocalFile.getAbsolutePath());
                CDELog.j(TAG, "download file " + epgLocalFile.getAbsolutePath() + " result:" + downloadResult);
                if (0 != downloadResult) {
                    CDELog.j(TAG, "can't download url" + epgUrl);
                    return null;
                }
            } else {
                CDELog.j(TAG, "iptv epg file exist" + epgLocalFile.getAbsolutePath());
            }
            handleEndTime = System.currentTimeMillis();

            CDELog.j(TAG, "download iptv epg data cost: " + (handleEndTime - handleBeginTime) + " milliseconds");

            CDELog.j(TAG, "epg download successfully");
        } else {
            epgLocalFile = new File(epgUrl);
        }
        try {
            File file = new File(epgLocalFile.getAbsolutePath());
            int fileLength = (int) file.length();
            CDELog.j(TAG, "file len:" + fileLength);
            byte[] fileContent = new byte[fileLength];
            FileInputStream in = new FileInputStream(file);
            in.read(fileContent);
            in.close();
            file.delete();
            CDELog.j(TAG, "file length:" + fileLength);
            ByteArrayInputStream inputStream = new ByteArrayInputStream(fileContent);
            BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
            Queue<String> extraLines = new ArrayDeque<>();
            String line;

            try {
                if (!checkPlaylistHeader(reader)) {
                    CDELog.j(TAG, "it shouldn't happen, pls check why?");
                    CDELog.j(TAG, "Input does not start with the #EXTM3U header.");
                    file.delete();
                    return null;
                }
                while ((line = reader.readLine()) != null) {
                    String tvglogoURL = null;
                    String tvglogoName = null;
                    String groupTitle = null;
                    String name = null;
                    String url = null;
                    CDEContentDescriptor descriptor = null;
                    line = line.trim();
                    //CDELog.j(TAG, "line=" + line);
                    if (line.isEmpty()) {
                        // Do nothing.
                        CDELog.j(TAG, "line is empty");
                    } else if (line.startsWith(TAG_MEDIA_DURATION)) {
                        //CDELog.j(TAG, "line=" + line);
                        int commaIndex = line.lastIndexOf(',');
                        if (commaIndex != -1) {
                            name = line.substring(commaIndex + 1);
                            //CDELog.j(TAG, "name=" + name);
                        }
                        String[] tokens = line.split(" ");
                        if ((tokens != null) && (tokens.length > 3)) {
                            CDELog.j(TAG, "tokens counts:" + tokens.length);
                            tvglogoURL = tokens[2];
                            groupTitle = tokens[3];
                        } else {
                            CDELog.j(TAG, "tokens counts:" + tokens.length);
                            CDELog.j(TAG, "sepcial line:" + line);
                            if (2 == tokens.length) {
                                CDELog.j(TAG, "customized epg");
                                name = tokens[1];
                                CDELog.j(TAG, "name:" + name);
                            }
                        }


                        line = reader.readLine();
                        line = line.trim();
                        CDELog.j(TAG, "line=" + line);
                        if (line.startsWith("/")) {
                            url = "http://" + CDEUtils.getKANTVServer() + line;
                            CDELog.j(TAG, "customized epg:" + url);
                        } else {
                            //url = line.trim();
                            url = line;
                        }
                        if (line.startsWith("#EXTVLCOPT")) {
                            CDELog.j(TAG, "skip this line=" + line);
                            line = reader.readLine();
                            url = line.trim();
                        }
                        //CDELog.j(TAG, "url=" + url);
                        if ((name != null) && (url != null)) {
                            descriptor = new CDEContentDescriptor(tvglogoURL, groupTitle, name, url);
                            descriptors.add(descriptor);
                        }
                        //extraLines.add(line);

                    } else if (line.startsWith("#EXTVLCOPT")) {
                        CDELog.j(TAG, "skip this line=" + line);
                    } else {
                        //extraLines.add(line);
                        CDELog.j(TAG, "line=" + line);
                        CDELog.j(TAG, "should not happened");
                    }
                }
            } finally {
                CDELog.j(TAG, "end parse");
                CDEUtils.closeQuietly(reader);
            }
        } catch (IOException ex) {
            CDELog.j(TAG, "failed to handle file:" + epgLocalFile.getAbsolutePath() + ",reason:" + ex.toString());
            return null;
        }


        CDELog.j(TAG, "iptv items:" + descriptors.size());
        handleEndTime = System.currentTimeMillis();

        CDELog.j(TAG, "load iptv epg data cost: " + (handleEndTime - handleBeginTime) + " milliseconds");

        return descriptors;
    }


    public static boolean isAppDebuggable(Context context) {
        return (context.getApplicationInfo().flags & 2) != 0;
    }


    public static boolean isDebuggerAttached() {
        return Debug.isDebuggerConnected() || Debug.waitingForDebugger();
    }

    public static boolean isEmulator(Context context) {
        String url = "tel:" + "123456";
        Intent intent = new Intent();
        intent.setData(Uri.parse(url));
        intent.setAction(Intent.ACTION_DIAL);
        boolean canResolveIntent = intent.resolveActivity(context.getPackageManager()) != null;

        return Build.FINGERPRINT.startsWith("generic")
                || Build.FINGERPRINT.toLowerCase().contains("vbox")
                || Build.FINGERPRINT.toLowerCase().contains("test-keys")
                || Build.MODEL.contains("google_sdk")
                || Build.MODEL.contains("Emulator")
                /*
                || Build.SERIAL.equalsIgnoreCase("unknown")
                 */
                || Build.SERIAL.equalsIgnoreCase("android")
                || Build.MODEL.contains("Android SDK built for x86")
                || Build.MANUFACTURER.contains("Genymotion")
                || (Build.BRAND.startsWith("generic") && Build.DEVICE.startsWith("generic"))
                || "google_sdk".equals(Build.PRODUCT)
                || ((TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE))
                .getNetworkOperatorName().toLowerCase().equals("android")
                || (!canResolveIntent)
                || Build.PRODUCT.contains("sdk")
                || Build.HARDWARE.contains("goldfish")
                || Build.HARDWARE.contains("ranchu")
                || Settings.Secure.getString(context.getContentResolver(), "android_id") == null
                || isFeatures()
                || checkIsNotRealPhone()
                ;
    }



    public static boolean notHasBlueTooth() {
        BluetoothAdapter ba = BluetoothAdapter.getDefaultAdapter();
        if (ba == null) {
            return true;
        } else {
            String name = ba.getName();
            if (TextUtils.isEmpty(name)) {
                return true;
            } else {
                return false;
            }
        }
    }


    public static boolean isFeatures() {
        return Build.FINGERPRINT.startsWith("generic")
                || Build.FINGERPRINT.toLowerCase().contains("vbox")
                || Build.FINGERPRINT.toLowerCase().contains("test-keys")
                || Build.MODEL.contains("google_sdk")
                || Build.MODEL.contains("Emulator")
                || Build.MODEL.contains("Android SDK built for x86")
                || Build.MANUFACTURER.contains("Genymotion")
                || (Build.BRAND.startsWith("generic") && Build.DEVICE.startsWith("generic"))
                || "google_sdk".equals(Build.PRODUCT);
    }


    public static boolean checkIsNotRealPhone() {
        String cpuInfo = readCpuInfo();
        if ((cpuInfo.contains("intel") || cpuInfo.contains("amd"))) {
            return true;
        }
        return false;
    }


    public static String readCpuInfo() {
        String result = "";
        try {
            String[] args = {"/system/bin/cat", "/proc/cpuinfo"};
            ProcessBuilder cmd = new ProcessBuilder(args);

            Process process = cmd.start();
            StringBuffer sb = new StringBuffer();
            String readLine = "";
            BufferedReader responseReader = new BufferedReader(new InputStreamReader(process.getInputStream(), "utf-8"));
            while ((readLine = responseReader.readLine()) != null) {
                sb.append(readLine);
            }
            responseReader.close();
            result = sb.toString().toLowerCase();
        } catch (IOException ex) {
            CDELog.j(TAG, "error:" + ex.toString());
        }
        return result;
    }


    public static boolean isMediaFile(String fileName) {
        int dotIndex = fileName.lastIndexOf('.');
        if (-1 == dotIndex)
            return false;
        String fileExtension = fileName.substring(dotIndex + 1);
        //CDELog.j(TAG, "fileExtension: " + fileExtension);
        if (fileExtension.contains("dat"))
            return false;

        switch (fileExtension.toLowerCase()) {
            case "3gp":
            case "avi":
            case "flv":
            case "mp4":
            case "m4v":
            case "mkv":
            case "mov":
            case "qt":
            case "mpeg":
            case "mpg":
            case "mpe":
            case "rm":
            case "rmvb":
            case "wmv":
            case "asf":
            case "asx":
            case "vob":
            case "mp3":
            case "ogg":
            case "aac":
            case "ac3":
            case "wav":
            case "ts":
            case "swf":
            //case "m3u8":
                //CDELog.j(TAG, "filename " + fileName + " is media file");
                return true;
            default:
                //CDELog.j(TAG, "filename " + fileName + " is not media file");
                return false;
        }
    }

    public static void exitAPK(Activity activity) {
        umExitApp();

        if (Build.VERSION.SDK_INT >= 16 && Build.VERSION.SDK_INT < 21) {
            activity.finishAffinity();
        } else if (Build.VERSION.SDK_INT >= 21) {
            activity.finishAffinity();
        } else {
            activity.finish();
        }
        System.exit(0);
    }

    public static String getKanTVUMServer() {
        return mKANTVUMUrl;
    }

    public static void  setKanTVUMServer(String url) {
        mKANTVUMUrl = url;
    }

    public static void updateKANTVServer(String serverName) {
        CDEUtils.setKANTVServer(serverName);
        String updateApkUrl = "http://" + serverName + "/kantv/apk/kantv-latest.apk";
        String updateApkVersionUrl = "http://" + serverName + "/kantv/apk/kantv-version.txt";
        String updateUMUrl = "http://" + serverName + ":81/" + "epg" + "/uploadUM";
        {
            CDEUtils.setKANTVUpdateAPKUrl(updateApkUrl);
            CDEUtils.setKANTVUpdateVersionUrl(updateApkVersionUrl);
            CDEUtils.setKanTVUMServer(updateUMUrl);
            CDELog.j(TAG, "modify kantv update apk url:" + CDEUtils.getKANTVUpdateAPKUrl());
            CDELog.j(TAG, "modify kantv update apk version url:" + CDEUtils.getKANTVUpdateVersionUrl());
            CDELog.j(TAG, "modify kantv UM url:" + CDEUtils.getKanTVUMServer());
        }
    }

    //user monitor or user's behavior analysis
    public static void umLauchApp() {
        try {
            String userMonitorData = null;
            userMonitorData = "{" +
                    "\"deviceID\":\"" + CDEUtils.getUniqueID() + "\","
                    + "\"deviceName\":\"" + CDEUtils.getDeviceName() + "\","
                    + "\"type\":\"" + "lauchapp" + "\","
                    + "\"version\":\"" + CDEUtils.getKANTVAPKVersion() + "\""
                    + "}";

            JSONObject jsonUMObject = JSON.parseObject(userMonitorData);
            CDELog.d(TAG, "jsonUMObject: " + jsonUMObject);
            int requestBodyLength = userMonitorData.length();
            byte[] requestBody = userMonitorData.getBytes();
            String url = getKanTVUMServer();
            CDELog.j(TAG, "http post request to: " + url);
            CDELog.j(TAG, "http post content: " + new String(requestBody));

            if (isDisableUM())
                return;

            KANTVDRM chinaDRM = KANTVDRM.getInstance();
            CDEDataEntity dataEntity = new CDEDataEntity();
            int result = chinaDRM.ANDROID_JNI_HttpPost(url, null, requestBody, requestBodyLength, dataEntity);
            if (dataEntity != null && dataEntity.getData() != null) {
                CDELog.j(TAG, "http post response: http code:" + dataEntity.getResult() + ", content:" + new String(dataEntity.getData()));
            }
            if (result != 0) {
                CDELog.j(TAG, "http post to " + url + " failed");
            }
        } catch (Exception e) {
            CDELog.j(TAG, "UM failed:" + e.toString());
        }
    }

    public static void umExitApp() {
        try {
            String userMonitorData = null;
            userMonitorData = "{" +
                    "\"deviceID\":\"" + CDEUtils.getUniqueID() + "\","
                    + "\"deviceName\":\"" + CDEUtils.getDeviceName() + "\","
                    + "\"type\":\"" + "exitapp" + "\","
                    + "\"version\":\"" + CDEUtils.getKANTVAPKVersion() + "\""
                    + "}";

            JSONObject jsonUMObject = JSON.parseObject(userMonitorData);
            CDELog.d(TAG, "jsonUMObject: " + jsonUMObject);
            int requestBodyLength = userMonitorData.length();
            byte[] requestBody = userMonitorData.getBytes();
            String url = getKanTVUMServer();
            CDELog.j(TAG, "http post request to: " + url);
            CDELog.j(TAG, "http post content: " + new String(requestBody));

            if (isDisableUM())
                return;

            KANTVDRM chinaDRM = KANTVDRM.getInstance();
            CDEDataEntity dataEntity = new CDEDataEntity();
            int result = chinaDRM.ANDROID_JNI_HttpPost(url, null, requestBody, requestBodyLength, dataEntity);
            if (dataEntity != null && dataEntity.getData() != null) {
                CDELog.j(TAG, "http post response: http code:" + dataEntity.getResult() + ", content:" + new String(dataEntity.getData()));
            }
            if (result != 0) {
                CDELog.j(TAG, "http post to " + url + " failed");
            }
        } catch (Exception e) {
            CDELog.j(TAG, "UM failed:" + e.toString());
        }
    }


    public static void umStartPlayback(String videoTitle, String videoPath) {
        try {
            String regEx = "[`~!@#$%^&*()+=|{}:;\\\\[\\\\].<>/?~！@（）——+|{}【】‘；：”“’。，、？']";
            videoTitle = Pattern.compile(regEx).matcher(videoTitle).replaceAll("").trim();
            SimpleDateFormat dateformat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            long startPlayTime = CDEUtils.getPlaybackBeginTime();
            String startPlayTimeString = dateformat.format(startPlayTime);
            String userMonitorData = "{" +
                    "\"deviceID\":\"" + CDEUtils.getUniqueID() + "\","
                    + "\"deviceName\":\"" + CDEUtils.getDeviceName() + "\","
                    + "\"type\":\"" + "startplayback" + "\","
                    + "\"version\":\"" + CDEUtils.getKANTVAPKVersion() + "\","
                    /*+ "\"startPlayTime\":\"" + startPlayTime + "\","*/
                    + "\"startPlayTimeString\":\"" + startPlayTimeString + "\","
                    + "\"videoTitle\": \"" + videoTitle.trim() + "\","
                    + "\"videoPath\": \"" + videoPath.trim() + "\""
                    + "}";
            CDELog.j(TAG, "device unique id:" + CDEUtils.getUniqueID());

            JSONObject jsonObject = JSON.parseObject(userMonitorData);
            CDELog.j(TAG, "jsonObject: " + jsonObject);
            int requestBodyLength = userMonitorData.length();
            byte[] requestBody = userMonitorData.getBytes();
            String url = getKanTVUMServer();
            CDELog.j(TAG, "http post request to: " + url);
            CDELog.j(TAG, "http post content: " + new String(requestBody));

            if (isDisableUM())
                return;

            KANTVDRM chinaDRM = KANTVDRM.getInstance();
            CDEDataEntity dataEntity = new CDEDataEntity();
            int result = chinaDRM.ANDROID_JNI_HttpPost(url, null, requestBody, requestBodyLength, dataEntity);
            if (dataEntity != null && dataEntity.getData() != null) {
                CDELog.j(TAG, "http post response: http code:" + dataEntity.getResult() + ", content:" + new String(dataEntity.getData()));
            }
            if (result != 0) {
                CDELog.j(TAG, "http post to " + url + " failed");
            }
        } catch (Exception e) {
            CDELog.j(TAG, "UM failed:" + e.toString());
        }
    }


    public static void umStopPlayback(String videoTitle, String videoPath) {
        try {
            long startPlayTime = CDEUtils.getPlaybackBeginTime();
            SimpleDateFormat dateformat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            String startPlayTimeString = dateformat.format(startPlayTime);
            long endPlayTime = System.currentTimeMillis();
            String endPlayTimeString = dateformat.format(endPlayTime);
            long vodPlayedTime = (System.nanoTime() - CDEUtils.perfGetPlaybackBeginTime()) / 1000000; //secs
            String userMonitorData = null;

            userMonitorData = "{" +
                    "\"deviceID\":\"" + CDEUtils.getUniqueID() + "\","
                    + "\"deviceName\":\"" + CDEUtils.getDeviceName() + "\","
                    + "\"type\":\"" + "stopplayback" + "\","
                    + "\"version\":\"" + CDEUtils.getKANTVAPKVersion() + "\","
                    + "\"endPlayTimeString\":\"" + endPlayTimeString + "\","
                    + "\"videoTitle\": \"" + videoTitle.trim() + "\","
                    + "\"videoPath\": \"" + videoPath.trim() + "\""
                    + "}";
            JSONObject jsonObject = JSON.parseObject(userMonitorData);
            CDELog.d(TAG, "jsonObject: " + jsonObject);
            int requestBodyLength = userMonitorData.length();
            byte[] requestBody = userMonitorData.getBytes();
            String url = getKanTVUMServer();
            CDELog.j(TAG, "um url:" + url);
            CDELog.j(TAG, "http post request to: " + url);
            CDELog.j(TAG, "http post content: " + new String(requestBody));

            if (isDisableUM())
                return;

            KANTVDRM chinaDRM = KANTVDRM.getInstance();
            CDEDataEntity dataEntity = new CDEDataEntity();
            int result = chinaDRM.ANDROID_JNI_HttpPost(url, null, requestBody, requestBodyLength, dataEntity);
            if (dataEntity != null && dataEntity.getData() != null) {
                CDELog.j(TAG, "http post response: http code:" + dataEntity.getResult() + ", content:" + new String(dataEntity.getData()));
            }
            if (result != 0) {
                CDELog.j(TAG, "http post to " + url + " failed");
            }
        } catch (Exception e) {
            CDELog.j(TAG, "UM failed:" + e.toString());
        }
    }


    public static void umStartRecord(String recordingFileName) {
        try {
            SimpleDateFormat dateformat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            long startRecordTime = CDEUtils.getRecordBeginTime();
            String startRecordTimeString = dateformat.format(startRecordTime);
            String userMonitorData = "{" +
                    "\"deviceID\":\"" + CDEUtils.getUniqueID() + "\","
                    + "\"deviceName\":\"" + CDEUtils.getDeviceName() + "\","
                    + "\"type\":\"" + "startrecord" + "\","
                    + "\"version\":\"" + CDEUtils.getKANTVAPKVersion() + "\","
                    + "\"startRecordTimeString\":\"" + startRecordTimeString + "\","
                    + "\"recordFileName\": \"" + recordingFileName.trim() + "\""
                    + "}";
            CDELog.j(TAG, "device unique id:" + CDEUtils.getUniqueID());

            JSONObject jsonObject = JSON.parseObject(userMonitorData);
            CDELog.j(TAG, "jsonObject: " + jsonObject);
            int requestBodyLength = userMonitorData.length();
            byte[] requestBody = userMonitorData.getBytes();
            String url = getKanTVUMServer();
            CDELog.j(TAG, "http post request to: " + url);
            CDELog.j(TAG, "http post content: " + new String(requestBody));

            if (isDisableUM())
                return;

            KANTVDRM chinaDRM = KANTVDRM.getInstance();
            CDEDataEntity dataEntity = new CDEDataEntity();
            int result = chinaDRM.ANDROID_JNI_HttpPost(url, null, requestBody, requestBodyLength, dataEntity);
            if (dataEntity != null && dataEntity.getData() != null) {
                CDELog.j(TAG, "http post response: http code:" + dataEntity.getResult() + ", content:" + new String(dataEntity.getData()));
            }
            if (result != 0) {
                CDELog.j(TAG, "http post to " + url + " failed");
            }
        } catch (Exception e) {
            CDELog.j(TAG, "UM failed:" + e.toString());
        }

        CDEUtils.umUpdateRecordInfo();
    }


    public static void umStopRecord(String recordingFileName) {
        try {
            long startRecordTime = CDEUtils.getRecordBeginTime();
            SimpleDateFormat dateformat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            String startRecordTimeString = dateformat.format(startRecordTime);
            long endRecordTime = System.currentTimeMillis();
            String endRecordTimeString = dateformat.format(endRecordTime);
            long vodRecordTime = (System.nanoTime() - CDEUtils.perfGetRecordBeginTime()) / 1000000; //secs
            String userMonitorData = null;

            userMonitorData = "{" +
                    "\"deviceID\":\"" + CDEUtils.getUniqueID() + "\","
                    + "\"deviceName\":\"" + CDEUtils.getDeviceName() + "\","
                    + "\"type\":\"" + "stoprecord" + "\","
                    + "\"version\":\"" + CDEUtils.getKANTVAPKVersion() + "\","
                    + "\"startRecordTimeString\":\"" + startRecordTimeString + "\","
                    + "\"recordFileName\": \"" + recordingFileName.trim() + "\","
                    + "\"endRecordTimeString\":\"" + endRecordTimeString + "\""
                    + "}";
            JSONObject jsonObject = JSON.parseObject(userMonitorData);
            CDELog.d(TAG, "jsonObject: " + jsonObject);
            int requestBodyLength = userMonitorData.length();
            byte[] requestBody = userMonitorData.getBytes();
            String url = getKanTVUMServer();
            CDELog.j(TAG, "http post request to: " + url);
            CDELog.j(TAG, "http post content: " + new String(requestBody));

            if (isDisableUM())
                return;

            KANTVDRM chinaDRM = KANTVDRM.getInstance();
            CDEDataEntity dataEntity = new CDEDataEntity();
            int result = chinaDRM.ANDROID_JNI_HttpPost(url, null, requestBody, requestBodyLength, dataEntity);
            if (dataEntity != null && dataEntity.getData() != null) {
                CDELog.j(TAG, "http post response: http code:" + dataEntity.getResult() + ", content:" + new String(dataEntity.getData()));
            }
            if (result != 0) {
                CDELog.j(TAG, "http post to " + url + " failed");
            }
        } catch (Exception e) {
            CDELog.j(TAG, "UM failed:" + e.toString());
        }
    }


    public static void umUpdateRecordInfo() {
        try {
            String userMonitorData = null;
            int isPaidUser = 0;
            if (getIsPaidUser())
                isPaidUser = 1;

            userMonitorData = "{" +
                    "\"deviceID\":\"" + CDEUtils.getUniqueID() + "\","
                    + "\"deviceName\":\"" + CDEUtils.getDeviceName() + "\","
                    + "\"type\":\"" + "updaterecord" + "\","
                    + "\"version\":\"" + CDEUtils.getKANTVAPKVersion() + "\","
                    + "\"isPaidUser\": \"" + isPaidUser + "\","
                    + "\"recordCounts\":\"" + 1 + "\""
                    + "}";
            JSONObject jsonObject = JSON.parseObject(userMonitorData);
            CDELog.d(TAG, "jsonObject: " + jsonObject);
            int requestBodyLength = userMonitorData.length();
            byte[] requestBody = userMonitorData.getBytes();
            String url = getKanTVUMServer();
            CDELog.j(TAG, "http post request to: " + url);
            CDELog.j(TAG, "http post content: " + new String(requestBody));

            if (isDisableUM())
                return;

            KANTVDRM chinaDRM = KANTVDRM.getInstance();
            CDEDataEntity dataEntity = new CDEDataEntity();
            int result = chinaDRM.ANDROID_JNI_HttpPost(url, null, requestBody, requestBodyLength, dataEntity);
            if (dataEntity != null && dataEntity.getData() != null) {
                CDELog.j(TAG, "http post response: http code:" + dataEntity.getResult() + ", content:" + new String(dataEntity.getData()));
            }
            if (result != 0) {
                CDELog.j(TAG, "http post to " + url + " failed");
            }
        } catch (Exception e) {
            CDELog.j(TAG, "UM failed:" + e.toString());
        }
    }


    public static void umGetRecordInfo() {
        try {
            long startTime = System.currentTimeMillis();
            String userMonitorData = null;

            userMonitorData = "{" +
                    "\"deviceID\":\"" + CDEUtils.getUniqueID() + "\","
                    + "\"deviceName\":\"" + CDEUtils.getDeviceName() + "\","
                    + "\"type\":\"" + "getrecordinfo" + "\","
                    + "\"version\":\"" + CDEUtils.getKANTVAPKVersion() + "\""
                    + "}";
            JSONObject jsonObject = JSON.parseObject(userMonitorData);
            CDELog.d(TAG, "jsonObject: " + jsonObject);
            int requestBodyLength = userMonitorData.length();
            byte[] requestBody = userMonitorData.getBytes();
            String url = getKanTVUMServer();
            CDELog.j(TAG, "http post request to: " + url);
            CDELog.j(TAG, "http post content: " + new String(requestBody));

            if (isDisableUM())
                return;

            KANTVDRM chinaDRM = KANTVDRM.getInstance();
            CDEDataEntity dataEntity = new CDEDataEntity();
            int result = chinaDRM.ANDROID_JNI_HttpPost(url, null, requestBody, requestBodyLength, dataEntity);
            if (dataEntity != null && dataEntity.getData() != null) {
                CDELog.j(TAG, "http post response: http code:" + dataEntity.getResult() + ", content:" + new String(dataEntity.getData()));
                JSONObject jsonResultObject = JSON.parseObject(new String(dataEntity.getData()));
                int recordCounts = jsonResultObject.getInteger("recordCounts");
                int isPaidUser   = jsonResultObject.getInteger("isPaidUser");
                int maxCounts = jsonResultObject.getInteger("maxCounts");
                CDELog.j(TAG, "server return record counts:" + recordCounts);
                CDELog.j(TAG, "server return isPaidUser:" + isPaidUser);
                CDELog.j(TAG, "server return maxCounts:" + maxCounts);
                setRecordCountsFromServer(recordCounts);
                setIsPaidUser((1 == isPaidUser) ? true : false);

            }
            if (result != 0) {
                CDELog.j(TAG, "http post to " + url + " failed");
            }
            long endTime   = System.currentTimeMillis();
            CDELog.j(TAG, "getrecordinfo cost time " + (endTime - startTime) + " milliseconds\n\n\n\n");
        } catch (Exception e) {
            CDELog.j(TAG, "UM failed:" + e.toString());
            setRecordCountsFromServer(0);
            setIsPaidUser(false);
        }
    }

    //TODO
    public static void umStartGraphicBenchmark()
    {

    }

    public static void setRecordCountsFromServer(int recordCounts) {
        mRecordCountsFromServer = recordCounts;
    }
    public static int getRecordCountsFromServer() {
        return mRecordCountsFromServer;
    }
    public static void setIsPaidUser(boolean isPaidUser) {
        mIsPaidUser = isPaidUser;
    }
    public static boolean getIsPaidUser() {
        return mIsPaidUser;
    }


    public static void umRecordBenchmark(int benchmarkItems, int benchmarkDurationsInMillseconds, int durationPerItemInSeconds) {
        try {
            SimpleDateFormat dateformat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            long benchmarkTime = System.currentTimeMillis();
            String benchmarkTimeString = dateformat.format(benchmarkTime);
            String userMonitorData = "{" +
                    "\"deviceID\":\"" + CDEUtils.getUniqueID() + "\","
                    + "\"deviceName\":\"" + CDEUtils.getDeviceName() + "\","
                    + "\"type\":\"" + "recordbenchmark" + "\","
                    + "\"version\":\"" + CDEUtils.getKANTVAPKVersion() + "\","
                    + "\"benchmarkTimeString\":\"" + benchmarkTimeString + "\","
                    + "\"benchmarkItems\": \"" + benchmarkItems + "\","
                    + "\"benchmarkDurations\": \"" + benchmarkDurationsInMillseconds + "\","
                    + "\"benchmarkDurationPerItem\": \"" + durationPerItemInSeconds + "\""
                    + "}";
            CDELog.j(TAG, "device unique id:" + CDEUtils.getUniqueID());

            JSONObject jsonObject = JSON.parseObject(userMonitorData);
            CDELog.j(TAG, "jsonObject: " + jsonObject);
            int requestBodyLength = userMonitorData.length();
            byte[] requestBody = userMonitorData.getBytes();
            String url = getKanTVUMServer();
            CDELog.j(TAG, "http post request to: " + url);
            CDELog.j(TAG, "http post content: " + new String(requestBody));

            if (isDisableUM())
                return;

            KANTVDRM chinaDRM = KANTVDRM.getInstance();
            CDEDataEntity dataEntity = new CDEDataEntity();
            int result = chinaDRM.ANDROID_JNI_HttpPost(url, null, requestBody, requestBodyLength, dataEntity);
            if (dataEntity != null && dataEntity.getData() != null) {
                CDELog.j(TAG, "http post response: http code:" + dataEntity.getResult() + ", content:" + new String(dataEntity.getData()));
            }
            if (result != 0) {
                CDELog.j(TAG, "http post to " + url + " failed");
            }
        } catch (Exception e) {
            CDELog.j(TAG, "UM failed:" + e.toString());
        }
    }


    public static void rawToWave(String file, float[] data, int samplerate) throws IOException {
        // creating the empty wav file.
        File waveFile = new File(file);
        waveFile.createNewFile();
        //following block is converting raw to wav.
        DataOutputStream output = null;
        try {
            output = new DataOutputStream(new FileOutputStream(waveFile));
            // WAVE header
            // chunk id
            writeString(output, "RIFF");
            // chunk size
            writeInt(output, 36 + data.length * 2);
            // format
            writeString(output, "WAVE");
            // subchunk 1 id
            writeString(output, "fmt ");
            // subchunk 1 size
            writeInt(output, 16);
            // audio format (1 = PCM)
            writeShort(output, (short) 1);
            // number of channels
            writeShort(output, (short) 1);
            // sample rate
            writeInt(output, samplerate);
            // byte rate
            writeInt(output, samplerate * 2);
            // block align
            writeShort(output, (short) 2);
            // bits per sample
            writeShort(output, (short) 16);
            // subchunk 2 id
            writeString(output, "data");
            // subchunk 2 size
            writeInt(output, data.length * 2);
            short[] short_data = FloatArray2ShortArray(data);
            for (int i = 0; i < short_data.length; i++) {
                writeShort(output, short_data[i]);
            }
        } finally {
            if (output != null) {
                output.close();
            }
        }
    }

    private static void writeInt(final DataOutputStream output, final int value) throws IOException {
        output.write(value);
        output.write(value >> 8);
        output.write(value >> 16);
        output.write(value >> 24);
    }

    private static void writeShort(final DataOutputStream output, final short value) throws IOException {
        output.write(value);
        output.write(value >> 8);
    }

    private static void writeString(final DataOutputStream output, final String value) throws IOException {
        for (int i = 0; i < value.length(); i++) {
            output.write(value.charAt(i));
        }
    }

    public static short[] FloatArray2ShortArray(float[] values) {
        float mmax = (float) 0.01;
        short[] ret = new short[values.length];

        for (int i = 0; i < values.length; i++) {
            if (abs(values[i]) > mmax) {
                mmax = abs(values[i]);
            }
        }

        for (int i = 0; i < values.length; i++) {
            values[i] = values[i] * (32767 / mmax);
            ret[i] = (short) (values[i]);
        }
        return ret;
    }

    public static boolean getTVRecording() {
        return mIsTVRecording;
    }

    public static void setTVRecording(boolean bRecording) {
        mIsTVRecording = bRecording;
    }


    public static void setRecordDuration(int recordDuration) {
        mRecordDuration = recordDuration;
    }

    public static int getRecordDuration() {
        return mRecordDuration;
    }


    public static void setRecordSize(int recordSize) {
        mRecordSize = recordSize;
    }

    public static int getRecordSize() {
        return mRecordSize;
    }


    public static void setEnableRecordVideoES(boolean bEnableRecordES) {
        mEnableRecordVideoES = bEnableRecordES;
    }

    public static boolean getEnableRecordVideoES() {
        return mEnableRecordVideoES;
    }

    public static void setEnableRecordAudioES(boolean bEnableRecordES) {
        mEnableRecordAudioES = bEnableRecordES;
    }

    public static boolean getEnableRecordAudioES() {
        return mEnableRecordAudioES;
    }

    public static int getRecordMode() {
        return mRecordMode;
    }

    public static void setRecordMode(int mode) {
        mRecordMode = mode;
    }

    public static String getRecordModeString(int recordMode) {
        switch (recordMode) {
            case 0:
                return "视频音频数据都录制";
            case 1:
                return "只录制视频数据";
            default:
                return "Unknown record mode";
        }
    }


    public static String getRecordFormatString(int recordFormat) {
        switch (recordFormat) {
            case 0:
                return "MP4";
            case 1:
                return "TS";
            default:
                return "Unknown record format";
        }
    }


    public static String getRecordCodecString(int recordCodec) {
        switch (recordCodec) {
            case 0:
                return "H264";
            case 1:
                return "H265";
            case 2:
                return "H266";
            case 3:
                return "AV1";           /* intel svt-av1 */
            case 4:
                return "Google AV1";    /* aom-av1 */
            default:
                return "Unknown video codec";
        }
    }


    public static void setRecordingFileName(String name) {
        mRecordingFileName = name;
    }
    public static String getRecordingFileName() {
        return mRecordingFileName;
    }

    public static int getDiskThresholdFreeSize() {
        return mDiskThresholdFreeSize;
    }

    public static void setDiskThresholdFreeSize(int size) {
        if (size < 500)
            size = 500;
        mDiskThresholdFreeSize = size;
    }


    public static void setExoplayerInstance(IMediaPlayer instance) {
        mExoplayerInstance = instance;
    }


    public static IMediaPlayer getExoplayerInstance() {
        return mExoplayerInstance;
    }


    public static String getDeviceSerial() {
        if (!TextUtils.isEmpty(Build.SERIAL)) {
            String deviceSerial = Build.SERIAL + Build.HOST + Build.FINGERPRINT;
            CDELog.j(TAG, "device info : " + deviceSerial);
            return deviceSerial;
        }
        CDELog.j(TAG, "can't get device serial");
        return "";
    }


    private boolean isRooted()
    {
        boolean res = false;
        try
        {
            if ((!new File("/system/bin/su").exists()) &&
                    (!new File("/system/xbin/su").exists()))
            {
                res = false;
            }
            else
            {
                res = true;
            }
            ;
        }
        catch (Exception e)
        {
            CDELog.j(TAG, "exception occurred");
        }
        return res;
    }


    public static void setCouldExitApp(boolean bEnabled) {
        mCouldExitApp.set(bEnabled);
    }

    public static boolean getCouldExitApp() {
        return mCouldExitApp.get();
    }

    public static boolean isDisableUM(){
        return mDisableUM;
    }

    public static native int kantv_anti_tamper();
}