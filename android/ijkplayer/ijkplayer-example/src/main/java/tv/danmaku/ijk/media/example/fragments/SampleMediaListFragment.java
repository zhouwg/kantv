 /*
 * Copyright (C) 2015 Bilibili
 * Copyright (C) 2015 Zhang Rui <bbcallen@gmail.com>
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

package tv.danmaku.ijk.media.example.fragments;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import tv.danmaku.ijk.media.example.R;
import tv.danmaku.ijk.media.example.activities.VideoActivity;

public class SampleMediaListFragment extends Fragment {
    private ListView mFileListView;
    private SampleMediaAdapter mAdapter;

    public static SampleMediaListFragment newInstance() {
        SampleMediaListFragment f = new SampleMediaListFragment();
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
                VideoActivity.intentTo(activity, url, name);
            }
        });
        // fetch key from remote http server directly
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h264_sample_sm4cbc/1.m3u8",  "h264 china sample sm4 cbc");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h265_sample_sm4cbc/1.m3u8",  "h265 china sample sm4 cbc");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h264_sm4cbc/1.m3u8", "h264 china sm4 cbc");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h265_sm4cbc/1.m3u8", "h265 china sm4 cbc");

        // HLS H264 SAMPLE-AES supportive is done
        // https://github.com/zhouwg/hijkplayer/issues/5
        // fetch key from remote http server directly
        // both  ok
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h264_sample_aes_video.m3u8", "h264 sample aes video only");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h264_sample_aes_audiovideo.m3u8", "h264 sample aes video&audio");

        //ok
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/ocnclear/ocn_clear.m3u8", "ocnclear/ocn.m3u8");

        // fetch license from ChinaDRM server, not support at the moment
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/ocn_sampleaes_chinadrm.m3u8", "ocn_sampleaes_chinadrm.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/aesbba/aesbba.m3u8", "aesbba/aesbba.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/aesbba/aesbba_24hour.m3u8", "aesbba/aesbba_24hour.m3u8");

        //HLS
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/AES-128/aes128_no_IV/pantos_aes.m3u8", "aes128_no_IV/pantos_aes.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/AES-128/rotation_key/key_rotation.m3u8", "rotation_key/key_rotation.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/sintel_clear/sintelclear.m3u8", "sintel_clear/sintelclear.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/sintel264/sintel264.m3u8", "sintel264/sintel264.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/HD1280P/HD1280P.m3u8", "HD1280P/HD1280P.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/Avengers/Avengers_2012.mp4", "Avengers/Avengers_2012.mp4");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/BigBuckBunny_HD1500/BigBuckBunnyHD500.m3u8", "BigBuckBunny_HD1500/BigBuckBunnyHD500.m3u8");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/clear/BigBuckBunny_HD1500/BigBuckBunnyHD1500.m3u8", "BigBuckBunny_HD1500/BigBuckBunnyHD1500.m3u8");

        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/bipbop_4x3_variant.m3u8", "bipbop basic master playlist");
        mAdapter.addItem("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear1/prog_index.m3u8", "bipbop basic 400x300 @ 232 kbps");

        //HTTP
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/html/test.flv", "mediaroot/html/test.flv");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/html/test.mp4", "mediaroot/html/test.mp4");
        mAdapter.addItem("http://192.168.0.100:81/mediaroot/html/sintel.mp4", "mediaroot/html/sintel.mp4");
        mAdapter.addItem("http://192.168.0.100:81/bigbuckbunnyh264.ts", "http://192.168.0.100:81/bigbuckbunnyh264.ts");
        mAdapter.addItem("http://192.168.0.100:81/bigbuckbunnyh265.ts", "http://192.168.0.100:81/bigbuckbunnyh265.ts");

        //RTSP
        mAdapter.addItem("rtsp://192.168.0.100:8000/testrtsp.ts", "rtsp://192.168.0.100:8000/testrtsp.ts");

        //DASH
        //ok
        mAdapter.addItem("http://dash.as.s3.amazonaws.com/media/sintel/out/sintel.mpd", "sintel/out/sintel.mpd");
        //Not yet implemented in FFmpeg, patches welcome
        mAdapter.addItem("http://dash.edgesuite.net/dash264/TestCases/1a/sony/SNE_DASH_SD_CASE1A_REVISED.mpd", "http://dash.edgesuite.net/dash264/TestCases/1a/sony/SNE_DASH_SD_CASE1A_REVISED.mpd");
        //ok
        mAdapter.addItem("http://dash.edgesuite.net/dash264/TestCases/1a/netflix/exMPD_BIP_TC1.mpd", "http://dash.edgesuite.net/dash264/TestCases/1a/netflix/exMPD_BIP_TC1.mpd");
        //ok
        mAdapter.addItem("http://dash.edgesuite.net/dash264/TestCases/2a/qualcomm/1/MultiResMPEG2.mpd", "http://dash.edgesuite.net/dash264/TestCases/2a/qualcomm/1/MultiResMPEG2.mpd");
        //ok
        mAdapter.addItem("http://dash.edgesuite.net/dash264/TestCases/3b/fraunhofer/heaac_stereo_with_video/Sintel/sintel_480p_heaac_stereo_sidx.mpd", "sintel_480p_heaac_stereo_sidx.mpd");
        //why this URL couldn't displayed in playback UI correctly?
        //route cause is lineLength should < 35
        //Failed to resolve hostname hls-lin-329.timewarnercable.com: No address associated with hostname
        mAdapter.addItem("http://hls-lin-329.timewarnercable.com/dash/DASH_720p60_SAP/manifest.mpd", "DASH_720p60_SAP/manifest.mpd");
        // 403 Forbidden
        mAdapter.addItem("http://dashas.castlabs.com/videos/files/bbb/Manifest.mpd", "files/bbb/Manifest.mpd");
        //Server returned 403 Forbidden (access denied)
        mAdapter.addItem("http://dashas.castlabs.com/videos/bytes/bbb/Manifest.mpd", "http://dashas.castlabs.com/videos/bytes/bbb/Manifest.mpd");
        //failure
        mAdapter.addItem("http://usp-demo.castlabs.com/live/channel1/channel1.isml/channel1.mpd", "http://usp-demo.castlabs.com/live/channel1/channel1.isml/channel1.mpd");

        //RTMP
        mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv13", "CCTV-13 News");

        //FLV
        mAdapter.addItem("http://zhibo.hkstv.tv/livestream/mutfysrq.flv", "mutfysrq.flv");

        mAdapter.addItem("https://stream7.iqilu.com/10339/upload_transcode/202002/18/20200218093206z8V1JuPlpe.mp4", "testmp4");
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

            return view;
        }

        final class ViewHolder {
            public TextView mNameTextView;
            public TextView mUrlTextView;
        }
    }
}
