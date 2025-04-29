/*
 * Copyright (C) 2015 Bilibili
 * Copyright (C) 2015 Zhang Rui <bbcallen@gmail.com>
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

import android.annotation.TargetApi;
import android.graphics.SurfaceTexture;
import android.os.Build;
import android.view.Surface;
import android.view.SurfaceHolder;

@TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
public class TextureMediaPlayer extends MediaPlayerProxy implements IMediaPlayer, ISurfaceTextureHolder {
    private SurfaceTexture mSurfaceTexture;
    private ISurfaceTextureHost mSurfaceTextureHost;

    public TextureMediaPlayer(IMediaPlayer backEndMediaPlayer) {
        super(backEndMediaPlayer);
    }

    public void releaseSurfaceTexture() {
        if (mSurfaceTexture != null) {
            if (mSurfaceTextureHost != null) {
                mSurfaceTextureHost.releaseSurfaceTexture(mSurfaceTexture);
            } else {
                mSurfaceTexture.release();
            }
            mSurfaceTexture = null;
        }
    }

    //--------------------
    // IMediaPlayer
    //--------------------
    @Override
    public void reset() {
        super.reset();
        releaseSurfaceTexture();
    }

    @Override
    public void release() {
        super.release();
        releaseSurfaceTexture();
    }

    @Override
    public void setDisplay(SurfaceHolder sh) {
        if (mSurfaceTexture == null)
            super.setDisplay(sh);
    }

    @Override
    public void setSurface(Surface surface) {
        if (mSurfaceTexture == null)
            super.setSurface(surface);
    }

    //--------------------
    // ISurfaceTextureHolder
    //--------------------

    @Override
    public void setSurfaceTexture(SurfaceTexture surfaceTexture) {
        if (mSurfaceTexture == surfaceTexture)
            return;

        releaseSurfaceTexture();
        mSurfaceTexture = surfaceTexture;
        if (surfaceTexture == null) {
            super.setSurface(null);
        } else {
            super.setSurface(new Surface(surfaceTexture));
        }
    }

    @Override
    public SurfaceTexture getSurfaceTexture() {
        return mSurfaceTexture;
    }

    @Override
    public void setSurfaceTextureHost(ISurfaceTextureHost surfaceTextureHost) {
        mSurfaceTextureHost = surfaceTextureHost;
    }
}
