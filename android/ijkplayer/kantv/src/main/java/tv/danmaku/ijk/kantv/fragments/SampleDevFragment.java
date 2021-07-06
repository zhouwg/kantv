//2021-06-24, weiguo: this file only used in "developer mode"
//because it's replaced with ContentListFragment.java which
//load EPG infos from local xml file(remote EPG server not available in open source project)
//and then render/layout the EPG data accordingly

package tv.danmaku.ijk.kantv.fragments;

import android.app.Activity;
import android.content.Context;

import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import tv.danmaku.ijk.kantv.R;
import tv.danmaku.ijk.kantv.activities.VideoActivity;
import tv.danmaku.ijk.media.player.IjkLog;

import static tv.danmaku.ijk.kantv.content.MediaType.MEDIA_TV;


public class SampleDevFragment extends Fragment {
    private ListView mFileListView;
    private SampleMediaAdapter mAdapter;
    private static final String TAG = SampleDevFragment.class.getName();

    public static SampleDevFragment newInstance() {
        SampleDevFragment f = new SampleDevFragment();
        return f;
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        ViewGroup viewGroup = (ViewGroup) inflater.inflate(R.layout.fragment_file_list, container, false);
        mFileListView = (ListView) viewGroup.findViewById(R.id.file_list_view);
        return viewGroup;
    }

    @Override
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        final Activity activity = getActivity();

        mAdapter = new SampleMediaAdapter(activity);
        mFileListView.setAdapter(mAdapter);
        mFileListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, final int position, final long id) {
                SampleMediaItem item = mAdapter.getItem(position);
                String name = item.mName;
                String url = item.mUrl;
                IjkLog.d(TAG, "position: " + position + " id:" + id + " name:" + name + " url:" + url);
                //contents here are not MEDIA_TV, just make VideoActivity happy
                VideoActivity.intentTo(activity, url, name, MEDIA_TV);
            }
        });

        // hls
        // fetch license from drm server
        mAdapter.addItem("http://192.168.0.100:81/h264_10/1.m3u8", "http://192.168.0.100:81/h264_10/1.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/h265_10/1.m3u8", "http://192.168.0.100:81/h265_10/1.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/h264_100/1.m3u8", "http://192.168.0.100:81/h264_100/1.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/h265_100/1.m3u8", "http://192.168.0.100:81/h265_100/1.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/ocn_sampleaes_chinadrm.m3u8", "ocn_sampleaes_chinadrm.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/aesbba/aesbba.m3u8", "aesbba/aesbba.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/aesbba/aesbba_24hour.m3u8", "aesbba/aesbba_24hour.m3u8");

        // fetch key from http server
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h264_sample_sm4cbc/1.m3u8", "h264 china sample sm4 cbc");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h265_sample_sm4cbc/1.m3u8", "h265 china sample sm4 cbc");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h264_sm4cbc/1.m3u8", "h264 china sm4 cbc");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h265_sm4cbc/1.m3u8", "h265 china sm4 cbc");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h264_sample_aes_video.m3u8", "h264 sample aes video only");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h264_sample_aes_audiovideo.m3u8", "h264 sample aes video&audio");

        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/ocnSample_full.m3u8", "ocnSample_full.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/testamlogic/prog_index.m3u8", "OCN sample-aes HLS test content");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/aesbba/aesbba.m3u8", "aesbba/aesbba.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/aesbba/aesbba_24hour.m3u8", "aesbba/aesbba_24hour.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/EDream/AES_index_v.m3u8", "EDream/AES_index_v.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/EDream/AES_all.m3u8", "EDream/AES_all.m3u8");
        // ok
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/AES-128/aes128_no_IV/pantos_aes.m3u8", "aes128_no_IV/pantos_aes.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/AES-128/cbc/aes128_2_keys.m3u8", "cbc/aes128_2_keys.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/AES-128/rotation_key/key_rotation.m3u8", "rotation_key/key_rotation.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/supergirls/index.m3u8", "supergirls/index.m3u8");

        // hls clear content
        mAdapter.addItem("http://192.168.0.100:81/h265_clear/1.m3u8", "h265_clear/1.m3u8");

        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/ocnclear/ocn_clear.m3u8", "ocn_clear/ocn.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/sintel_clear/sintelclear.m3u8", "sintel_clear/sintelclear.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/ZhaoXin_BigTS/zhaoxin.m3u8", "bigts_clear/zhaoxin.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/ZhaoXin_segments/zhaoxinsegments.m3u8", "segs_clear/zhaoxinsegments.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/sintel264/sintel264.m3u8", "sintel264_clear/sintel264.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/HD1280P/HD1280P.m3u8", "HD1280P_clear/HD1280P.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/Avengers/Avengers_2012.mp4", "Avengers_clear/Avengers_2012.mp4");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/BigBuckBunny_HD1500/BigBuckBunnyHD500.m3u8", "BigBuckBunny_HD1500_clear/BigBuckBunnyHD500.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/BigBuckBunny_HD1500/BigBuckBunnyHD1500.m3u8", "BigBuckBunny_HD1500_clear/BigBuckBunnyHD1500.m3u8");

        // hls clear content from apple
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/bipbop_4x3_variant.m3u8", "bipbop basic master playlist");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear1/prog_index.m3u8", "bipbop basic 400x300 @ 232 kbps");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear2/prog_index.m3u8", "bipbop basic 640x480 @ 650 kbps");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear3/prog_index.m3u8", "bipbop basic 640x480 @ 1 Mbps");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear4/prog_index.m3u8", "bipbop basic 960x720 @ 2 Mbps");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear0/prog_index.m3u8", "bipbop basic 22.050Hz stereo @ 40 kbps");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/bipbop_16x9_variant.m3u8", "bipbop advanced master playlist");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/gear1/prog_index.m3u8", "bipbop advanced 416x234 @ 265 kbps");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/gear2/prog_index.m3u8", "bipbop advanced 640x360 @ 580 kbps");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/gear3/prog_index.m3u8", "bipbop advanced 960x540 @ 910 kbps");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/gear4/prog_index.m3u8", "bipbop advanced 1289x720 @ 1 Mbps");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/gear5/prog_index.m3u8", "bipbop advanced 1920x1080 @ 2 Mbps");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/gear0/prog_index.m3u8", "bipbop advanced 22.050Hz stereo @ 40 kbps");

        //hls HD
        mAdapter.addItem("http://ivi.bupt.edu.cn/hls/cctv1hd.m3u8", "CCTV1高清");
        mAdapter.addItem("http://ivi.bupt.edu.cn/hls/cctv3hd.m3u8", "CCTV3高清");
        mAdapter.addItem("http://ivi.bupt.edu.cn/hls/cctv5hd.m3u8", "CCTV5高清");
        mAdapter.addItem("http://ivi.bupt.edu.cn/hls/cctv5phd.m3u8", "CCTV5+高清");
        mAdapter.addItem("http://ivi.bupt.edu.cn/hls/cctv6hd.m3u8", "CCTV6高清");

        // http
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/purevideo.mp4", "http_purevideo.mp4");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/html/test.flv", "http_test.flv");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/html/test.mp4", "http_test.mp4");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/html/sintel.mp4", "http_sintel.mp4");
        mAdapter.addItem("http://192.168.0.100:81/bigbuckbunnyh264.ts", "http_bigbuckbunnyh264.ts");
        mAdapter.addItem("http://192.168.0.100:81/bigbuckbunnyh265.ts", "http_bigbuckbunnyh265.ts");
        mAdapter.addItem("http://9890.vod.myqcloud.com/9890_9c1fa3e2aea011e59fc841df10c92278.f20.mp4", "http_f20.mp4");

        // rtsp
        mAdapter.addItem("rtsp://192.168.0.100:8000/testrtsp.ts", "rtsp://192.168.0.100:8000/testrtsp.ts");
        mAdapter.addItem("rtsp://192.168.0.100:8000/bigbuckbunnyh265.ts", "rtsp://192.168.0.100:8000/bigbuckbunnyh265.ts");

        // dash
        // ok
        mAdapter.addItem("http://dash.edgesuite.net/dash264/TestCases/1a/netflix/exMPD_BIP_TC1.mpd", "netflix/exMPD_BIP_TC1.mpd");
        // Not yet implemented in FFmpeg, patches welcome
        mAdapter.addItem("http://dash.edgesuite.net/dash264/TestCases/1a/sony/SNE_DASH_SD_CASE1A_REVISED.mpd", "sony/SNE_DASH_SD_CASE1A_REVISED.mpd");
        // ok
        mAdapter.addItem("http://dash.edgesuite.net/dash264/TestCases/2a/qualcomm/1/MultiResMPEG2.mpd", "qualcomm/MultiResMPEG2.mpd");

        //rtmp
        mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv13", "rtmp-CCTV-13新闻");
        mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv16", "rtmp-CCTV-26CGTN");
    }

    final class SampleMediaItem {
        String mUrl;
        String mName;

        public SampleMediaItem(String url, String name) {
            mUrl = url;
            mName = name;
        }
    }

    final class SampleMediaAdapter extends ArrayAdapter<SampleMediaItem> {
        public SampleMediaAdapter(Context context) {
            super(context, android.R.layout.simple_list_item_2);
        }

        public void addItem(String url, String name) {
            add(new SampleMediaItem(url, name));
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View view = convertView;
            if (view == null) {
                LayoutInflater inflater = LayoutInflater.from(parent.getContext());
                view = inflater.inflate(android.R.layout.simple_list_item_2, parent, false);
            }

            ViewHolder viewHolder = (ViewHolder) view.getTag();
            if (viewHolder == null) {
                viewHolder = new ViewHolder();
                viewHolder.mNameTextView = (TextView) view.findViewById(android.R.id.text1);
                viewHolder.mUrlTextView = (TextView) view.findViewById(android.R.id.text2);
            }

            SampleMediaItem item = getItem(position);
            viewHolder.mNameTextView.setText(item.mName);
            viewHolder.mUrlTextView.setText(item.mUrl);
            viewHolder.mUrlTextView.setVisibility(View.INVISIBLE);

            return view;
        }

        final class ViewHolder {
            public TextView mNameTextView;
            public TextView mUrlTextView;
        }
    }
}
