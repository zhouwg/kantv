// https://github.com/chaohengxing/VoisePlayingIcon

package com.kantvai.kantvplayer.player.ffplayer.media;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Handler;
import android.os.Message;
import androidx.annotation.Nullable;
import android.util.AttributeSet;

import android.view.View;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import com.kantvai.kantvplayer.R;

import kantvai.media.player.KANTVDensityUtils;


/**
 * Created by chaohx on 2017/9/19.
 */

public class VoisePlayingIcon extends View {

    private final static String TAG = VoisePlayingIcon.class.getName();
    //画笔
    private Paint paint;

    //跳动指针的集合
    private List<Pointer> pointers;

    //跳动指针的数量
    private int pointerNum = 40; //default is 40

    //逻辑坐标 原点
    private float basePointX;
    private float basePointY;

    //指针间的间隙  默认5dp
    private float pointerPadding;

    //每个指针的宽度 默认3dp
    private float pointerWidth = 4; //default is 4

    //指针的颜色
    //private int pointerColor = Color.RED;
    private int pointerColor = Color.parseColor("#FF6565");

    //控制开始/停止
    private boolean isPlaying = false;

    //子线程
    private Thread myThread;

    //指针波动速率
    private int pointerSpeed;


    public VoisePlayingIcon(Context context) {
        super(context);
        init();
    }

    public VoisePlayingIcon(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        //取出自定义属性
        TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.voisePlayingIconAttr);
        //pointerColor = ta.getColor(R.styleable.voisePlayingIconAttr_pointer_color, Color.RED);
        //pointerNum = ta.getInt(R.styleable.voisePlayingIconAttr_pointer_num, 4);//指针的数量，默认为4
        //pointerWidth = DensityUtils.dp2px(getContext(),ta.getFloat(R.styleable.voisePlayingIconAttr_pointer_width, 5f));//指针的宽度，默认5dp
        pointerSpeed = ta.getInt(R.styleable.voisePlayingIconAttr_pointer_speed, 40);
        init();
    }

    public VoisePlayingIcon(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.voisePlayingIconAttr);
        pointerColor = ta.getColor(R.styleable.voisePlayingIconAttr_pointer_color, Color.RED);
        pointerNum = ta.getInt(R.styleable.voisePlayingIconAttr_pointer_num, 4);
        pointerWidth = KANTVDensityUtils.dp2px(getContext(), ta.getFloat(R.styleable.voisePlayingIconAttr_pointer_width, 5f));
        pointerSpeed = ta.getInt(R.styleable.voisePlayingIconAttr_pointer_speed, 40);
        init();
    }

    /**
     * 初始化画笔与指针的集合
     */
    private void init() {
        paint = new Paint();
        paint.setAntiAlias(true);
        paint.setColor(pointerColor);
        pointers = new ArrayList<>();
    }


    /**
     * 在onLayout中做一些，宽高方面的初始化
     *
     * @param changed
     * @param left
     * @param top
     * @param right
     * @param bottom
     */
    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        //获取逻辑原点的，也就是画布左下角的坐标。这里减去了paddingBottom的距离
        basePointY = getHeight() - getPaddingBottom();
        Random random = new Random();
        if (pointers != null)
            pointers.clear();
        for (int i = 0; i < pointerNum; i++) {
            //创建指针对象，利用0~1的随机数 乘以 可绘制区域的高度。作为每个指针的初始高度。
            pointers.add(new Pointer((float) (0.1 * (random.nextInt(10) + 1) * (getHeight() - getPaddingBottom() - getPaddingTop()))));
        }
        //计算每个指针之间的间隔  总宽度 - 左右两边的padding - 所有指针占去的宽度  然后再除以间隔的数量
        pointerPadding = (getWidth() - getPaddingLeft() - getPaddingRight() - pointerWidth * pointerNum) / (pointerNum - 1);
    }


    /**
     * 开始绘画
     *
     * @param canvas
     */
    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        //将x坐标移动到逻辑原点，也就是左下角
        basePointX = 0f + getPaddingLeft();
        //循环绘制每一个指针。
        for (int i = 0; i < pointers.size(); i++) {
            //绘制指针，也就是绘制矩形
            canvas.drawRect(basePointX,
                    basePointY - pointers.get(i).getHeight(),
                    basePointX + pointerWidth,
                    basePointY,
                    paint);
            basePointX += (pointerPadding + pointerWidth);
        }
    }

    /**
     * 开始播放
     */
    public void start() {
        if (!isPlaying) {
            if (myThread == null) {//开启子线程
                myThread = new Thread(new MyRunnable());
                myThread.start();
            }
            isPlaying = true;//控制子线程中的循环
        }
    }

    /**
     * 停止子线程，并刷新画布
     */
    public void stop() {
        isPlaying = false;
        invalidate();
    }

    /**
     * 处理子线程发出来的指令，然后刷新布局
     */
    private Handler myHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            invalidate();
        }
    };

    /**
     * 子线程，循环改变每个指针的高度
     */
    public class MyRunnable implements Runnable {

        @Override
        public void run() {

            for (float i = 0; i < Integer.MAX_VALUE; ) {//创建一个死循环，每循环一次i+0.1
                try {
                    for (int j = 0; j < pointers.size(); j++) { //循环改变每个指针高度
                        float rate = (float) Math.abs(Math.sin(i + j));//利用正弦有规律的获取0~1的数。
                        pointers.get(j).setHeight((basePointY - getPaddingTop()) * rate); //rate 乘以 可绘制高度，来改变每个指针的高度
                    }
                    Thread.sleep(pointerSpeed);//休眠一下下，可自行调节
                    if (isPlaying) { //控制开始/暂停
                        myHandler.sendEmptyMessage(0);
                        i += 0.1;
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }


    /**
     * 指针对象
     */
    public class Pointer {
        private float height;

        public Pointer(float height) {
            this.height = height;
        }

        public float getHeight() {
            return height;
        }

        public void setHeight(float height) {
            this.height = height;
        }
    }
}

