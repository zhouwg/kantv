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

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.view.Surface;
import java.util.Map;
import java.lang.ref.WeakReference;


final public class KANTVMgr
{
    private final static String TAG = "KANTVMgr";
    private final static int MAX_VERSION_SIZE = 256;
    private static byte[] mVersion = new byte[MAX_VERSION_SIZE];

    private EventHandler mEventHandler;
    private KANTVEventListener mEventListener;
    private EventCallback mCallback = new EventCallback();
    private int mNativeContext;


    private KANTVMgr() throws KANTVException
    {
        throw new KANTVException("private constructor");
    }


    public void release()
    {
        CDELog.d(TAG, "[Java] release");
        native_release();
    }


    private boolean versionCheck()
    {
        String javaVersion = CDEUtils.getKANTVAPKVersion();
        String nativeVersion = getVersion();
        CDELog.j(TAG, "[Java] JAR Version " + javaVersion);
        CDELog.j(TAG, "[Java] JNI Version " + nativeVersion);

        if (!nativeVersion.startsWith(javaVersion))
        {
            CDELog.j(TAG, "JAR's version does not match JNI's version: " + nativeVersion);
            return false;
        }
        return true;
    }


    public KANTVMgr(KANTVEventListener eventListener) throws KANTVException
    {
        if (!CDELibraryLoader.hasLoaded())
        {
            throw new KANTVException("JNI library not loaded");
        }

        if (!versionCheck())
        {
            String javaVersion = CDEUtils.getKANTVAPKVersion();
            String nativeVersion = getVersion();
            String errorMsg = "JAR's version " + javaVersion + " does not match JNI's version: " + nativeVersion;
            CDELog.j(TAG, errorMsg);
            //weiguo:uncomment it for production project
            //throw new KANTVException(errorMsg);
        }

        mEventListener = eventListener;

        Looper looper;
        if ((looper = Looper.myLooper()) != null)
        {
            mEventHandler = new EventHandler(this, looper);
        }
        else if ((looper = Looper.getMainLooper()) != null)
        {
            mEventHandler = new EventHandler(this, looper);
        }
        else
        {
            mEventHandler = null;
        }

        native_setup(new WeakReference<KANTVMgr>(this));
    }


    private class EventCallback implements KANTVEventInterface.KANTVEventCallback
    {
        public void onEventInfo(KANTVEventType event, int what, int arg1, int arg2, Object obj)
        {
            if (mEventListener != null)
            {
                mEventListener.onEvent(event, what, arg1, arg2, obj);
            }
        }
    }


    private class EventHandler extends Handler
    {
        private KANTVMgr mKANTVMgr;

        public EventHandler(KANTVMgr mgr, Looper looper)
        {
            super(looper);
            mKANTVMgr = mgr;
        }

        @Override
        public void handleMessage(Message msg)
        {
            try
            {
                if (0 == mKANTVMgr.mNativeContext)
                {
                    CDELog.j(TAG, "[Java] pls check JNI initialization");
                    return;
                }

                switch (msg.what)
                {
                case KANTVEvent.KANTV_INFO:
                    mCallback.onEventInfo(KANTVEventType.KANTV_EVENT_INFO, msg.what, msg.arg1, msg.arg2, msg.obj);
                    break;

                case KANTVEvent.KANTV_ERROR:
                    mCallback.onEventInfo(KANTVEventType.KANTV_EVENT_ERROR, msg.what, msg.arg1, msg.arg2, msg.obj);
                    break;

                default:
                    CDELog.j(TAG, "Unknown message type " + msg.what);
                    break;
                }
            }
            catch (Exception e)
            {
                CDELog.j(TAG, "exception occurred in event handler");
            }
        }
    }


    public static String getVersion()
    {
        int versionLen = getVersionDescription(mVersion, MAX_VERSION_SIZE);
        if (versionLen <= 0)
        {
            return "";
        }

        return new String(mVersion, 0, versionLen);
    }


    private static void postEventFromNative(Object KANTVmgr_ref,
                                            int what, int arg1, int arg2, Object obj)
    {
        KANTVMgr mgr = (KANTVMgr) ((WeakReference) KANTVmgr_ref).get();
        if (mgr.mEventHandler != null)
        {
            Message msg = mgr.mEventHandler.obtainMessage(what, arg1, arg2, obj);
            mgr.mEventHandler.sendMessage(msg);
        }
    }


    public void open()
    {
        native_open();
    }


    public void close()
    {
        native_close();
    }

    public native void native_open();
    public native void native_close();

    public native String getMgrVersion();

    public native Map<Integer, KANTVStreamProfile> getStreamProfiles(int stream);
    public native void setStreamFormat(int streamIndex, int encodeID, int width, int height, int fps, int pixelformat, int pattern, int enableFilter, int disableSurface);
    public native String formatToString(int format);
    public native int    formatFromString(String format);
    public native void setPreviewDisplay(Map surfaceMap);
    public native void setPreviewType(int previewType);

    public native void enablePreview(int streamIndex, Surface surface);
    public native void disablePreview(int streamIndex);

    public native void startPreview();
    public native void stopPreview();

    public native void startGraphicBenchmarkPreview();
    public native void stopGraphicBenchmarkPreview();

    public native void initASR();
    public native void finalizeASR();
    public native void startASR();
    public native void stopASR();

    private native static int getVersionDescription(byte[] byteSecurityToken, int bufSize);
    private native static final void native_init();
    private native final void native_setup(Object KANTVMgr_this);
    private native final void native_release();


    static
    {
        try
        {
            native_init();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            CDELog.j(TAG, "failed to initialize KANTVMgr: " + e.toString());
        }
    }

    public static native int kantv_anti_remove_rename_this_file();
}
