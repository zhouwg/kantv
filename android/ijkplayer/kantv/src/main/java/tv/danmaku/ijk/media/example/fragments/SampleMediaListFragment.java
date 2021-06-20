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
import tv.danmaku.ijk.media.example.application.MediaType;
import tv.danmaku.ijk.media.player.IjkLog;

import static tv.danmaku.ijk.media.example.application.MediaType.MEDIA_MOVIE;
import static tv.danmaku.ijk.media.example.application.MediaType.MEDIA_TV;

 public class SampleMediaListFragment extends Fragment {
    private ListView mFileListView;
    private SampleMediaAdapter mAdapter;
    private MediaType mMediaType = MEDIA_TV;
    private static String TAG = SampleMediaListFragment.class.getName();

    public static SampleMediaListFragment newInstance(MediaType type) {
        SampleMediaListFragment f = new SampleMediaListFragment();
        f.setMediaType(type);
        return f;
    }

    public void setMediaType(MediaType type) {
        mMediaType = type;
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
                VideoActivity.intentTo(activity, url, name);
            }
        });

        if (MEDIA_TV == mMediaType) {
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv13", "CCTV-13新闻");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv16", "CGTN-English");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv1", "CCTV-1综合");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv2", "CCTV-2财经");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv3", "CCTV-3综艺");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv4", "CCTV-4中文国际");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv5", "CCTV-5体育");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv6", "CCTV-6电影");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv7", "CCTV-7军事农业");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv8", "CCTV-8电视剧");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv9", "CCTV-9记录");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv10", "CCTV-10科教");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv11", "CCTV-11戏曲");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv12", "CCTV-12社会与法");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv14", "CCTV-14少儿");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cctv15", "CCTV-15音乐");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/sjdltv", "CCTV-世界地理");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/fyyytv", "CCTV-风云音乐");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/fyzqtv", "CCTV-风云足球");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/fyjctv", "CCTV-风云剧场");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/hjjctv", "CCTV-怀旧剧场");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/gfjstv", "CCTV-国防军事");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/dyjctv", "CCTV-第一剧场");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cetv1", "中国教育-1");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cetv2", "中国教育-2");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cetv3", "中国教育-3");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cetv4", "中国教育-4");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/sdetv", "山东教育");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/gdtv", "广东卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/gxtv", "广西卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/gstv", "甘肃卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/gztv", "贵州卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/hbtv", "湖北卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/hunantv", "湖南卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/hebtv", "河北卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/hntv", "河南卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/hljtv", "黑龙江卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/jstv", "江苏卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/jxtv", "江西卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/jltv", "吉林卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/lntv", "辽宁卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/nmtv", "内蒙古卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/nxtv", "宁夏卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/qhtv", "青海卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/sctv", "四川卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/sdtv", "山东卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/sxrtv", "山西卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/sxtv", "陕西卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cetv1", "中国教育-1");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cetv3", "中国教育-3");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/cetv4", "中国教育-4");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/chcatv", "CHC ATV 动作电影");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/chctv", "CHC CTV 家庭影院");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/chchd", "CHC CHD 高清电影");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/natlgeo", "国家地理频道");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/discovery", "探索频道");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/channelv", "Channel[V]");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/starsports", "Star Sports");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/startv", "星空卫视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/fhdy", "凤凰卫视电影台");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/fhzx", "凤凰卫视资讯台");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/fhzw", "凤凰卫视中文台");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/documentaryhd", "全纪实高清");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/gedocu", "金鹰纪实高清");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/docuchina", "上海纪实高清");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/btv9", "BTV-9 北京新闻");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/btv8", "BTV-8 北京青年");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/btv7", "BTV-7 北京生活");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/btv6", "BTV-6 北京体育");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/btv5", "BTV-5 北京财经");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/btv4", "BTV-4 北京影视");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/btv3", "BTV-3 北京科教");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/btv2", "BTV-2 北京文艺");
            mAdapter.addItem("rtmp://58.200.131.2:1935/livetv/btv1", "BTV-1 北京卫视");
            mAdapter.addItem("http://zhibo.hkstv.tv/livestream/mutfysrq.flv", "HKS TV");
        } else if (MEDIA_MOVIE== mMediaType) {
            //TODO: add one or two or three high-score / classic online movies every week
            mAdapter.addItem("https://v3.dious.cc//20210416/nIYS8a98/index.m3u8", "Nobody(小人物)");
            mAdapter.addItem("https://vod3.buycar5.cn//20210416/AxCh7eY4/1000kb/hls/index.m3u8", "Nobody(小人物) -- AES encrypted");
            mAdapter.addItem("https://vod.bunediy.com/20200426/kd4Th5ZQ/index.m3u8", "The Shawshank Redemption(肖申克的救赎) -- AES encrypted");
        } else { //local test content for study DRM system
            // fetch key from remote http server directly
            mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h264_sample_sm4cbc/1.m3u8",  "h264 china sample sm4 cbc");
            mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h265_sample_sm4cbc/1.m3u8",  "h265 china sample sm4 cbc");
            mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h264_sm4cbc/1.m3u8", "h264 china sm4 cbc");
            mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h265_sm4cbc/1.m3u8", "h265 china sm4 cbc");
            mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h264_sample_aes_video.m3u8", "h264 sample aes video only");
            mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/h264_sample_aes_audiovideo.m3u8", "h264 sample aes video&audio");
            mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/ocnclear/ocn_clear.m3u8", "ocnclear/ocn.m3u8");
            // fetch license from ChinaDRM server
            mAdapter.addItem("http://192.168.0.100:81/mediaroot/ocn/ocn_sampleaes_chinadrm.m3u8", "ocn_sampleaes_chinadrm.m3u8");
            mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/aesbba/aesbba.m3u8", "aesbba/aesbba.m3u8");
            mAdapter.addItem("http://192.168.0.100:81/mediaroot/tee/aesbba/aesbba_24hour.m3u8", "aesbba/aesbba_24hour.m3u8");
        }
        //TODO: fetch ChinaDRM encrypted EPG data or movie list from remote server, and render the decrypted data in kantv apk
        //https://www.deepepg.com/epg

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
                viewHolder.mUrlTextView  = (TextView) view.findViewById(android.R.id.text2);
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
