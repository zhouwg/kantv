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
        KANTVLog.d(TAG, "[Java] release");
        native_release();
    }


    private boolean versionCheck()
    {
        String javaVersion = KANTVUtils.getKANTVAPKVersion();
        String nativeVersion = getVersion();
        KANTVLog.j(TAG, "[Java] JAR Version " + javaVersion);
        KANTVLog.j(TAG, "[Java] JNI Version " + nativeVersion);

        if (!nativeVersion.startsWith(javaVersion))
        {
            KANTVLog.j(TAG, "JAR's version does not match JNI's version: " + nativeVersion);
            return false;
        }
        return true;
    }


    public KANTVMgr(KANTVEventListener eventListener) throws KANTVException
    {
        if (!KANTVLibraryLoader.hasLoaded())
        {
            throw new KANTVException("JNI library not loaded");
        }

        if (!versionCheck())
        {
            String javaVersion = KANTVUtils.getKANTVAPKVersion();
            String nativeVersion = getVersion();
            String errorMsg = "JAR's version " + javaVersion + " does not match JNI's version: " + nativeVersion;
            KANTVLog.j(TAG, errorMsg);
            //weiguo:uncomment it
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
                    KANTVLog.j(TAG, "[Java] pls check JNI initialization");
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
                    KANTVLog.j(TAG, "Unknown message type " + msg.what);
                    break;
                }
            }
            catch (Exception e)
            {
                KANTVLog.j(TAG, "exception occurred in event handler");
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
            KANTVLog.j(TAG, "failed to initialize KANTVMgr: " + e.toString());
        }
    }

    public static native int kantv_anti_remove_rename_this_file();
}
