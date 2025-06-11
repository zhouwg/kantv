package kantvai.media.encoder.magicfilter.advanced;

import android.opengl.GLES20;

import kantvai.media.encoder.magicfilter.utils.MagicFilterType;

import kantvai.media.encoder.magicfilter.base.gpuimage.GPUImageFilter;
import com.kantvai.kantvplayerlib.R;

/**
 * Created by Administrator on 2016/5/22.
 */
public class MagicBeautyFilter extends GPUImageFilter{
    private int mSingleStepOffsetLocation;

    public MagicBeautyFilter(){
        super(MagicFilterType.BEAUTY, R.raw.beauty);
    }

    protected void onInit() {
        super.onInit();
        mSingleStepOffsetLocation = GLES20.glGetUniformLocation(getProgram(), "singleStepOffset");
    }

    @Override
    public void onInputSizeChanged(final int width, final int height) {
        super.onInputSizeChanged(width, height);
        setFloatVec2(mSingleStepOffsetLocation, new float[] {2.0f / width, 2.0f / height});
    }
}
