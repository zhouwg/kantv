 /*
  * Copyright (C) 2021 zhouwg <zhouwg2000@gmail.com>
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
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.util.EntityUtils;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import tv.danmaku.ijk.media.example.R;
import tv.danmaku.ijk.media.example.activities.VideoActivity;
import tv.danmaku.ijk.media.example.content.MediaType;
import tv.danmaku.ijk.media.example.content.AssetLoader;
import tv.danmaku.ijk.media.example.content.ContentDescriptor;
import tv.danmaku.ijk.media.example.content.IjkUrlType;
import tv.danmaku.ijk.media.player.IjkLog;

import static tv.danmaku.ijk.media.example.content.MediaType.MEDIA_TV;
import static tv.danmaku.ijk.media.example.content.MediaType.MEDIA_RADIO;
import static tv.danmaku.ijk.media.example.content.MediaType.MEDIA_MOVIE;


 public class ContentListFragment extends Fragment {
    private LinearLayout layout;
    private MediaType mMediaType = MEDIA_TV;
    private static String TAG = ContentListFragment.class.getName();
    private List<ContentDescriptor> mContentList = new ArrayList<ContentDescriptor>();

    public static ContentListFragment newInstance(MediaType type) {
        ContentListFragment f = new ContentListFragment();
        f.setMediaType(type);
        return f;
    }

    public void setMediaType(MediaType type) {
        mMediaType = type;
        IjkLog.d(TAG, "mediatype : " + type.toString());
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        ViewGroup viewGroup = (ViewGroup) inflater.inflate(R.layout.fragment_content, container, false);
        layout = (LinearLayout) viewGroup.findViewById(R.id.media_content_layout);
        return viewGroup;
    }

    @Override
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        final Activity activity = getActivity();

        Resources res = getResources();
        Context context = activity.getBaseContext();
        String xmlURI = "tv.xml";
        if (MEDIA_TV == mMediaType) {
            xmlURI = "tv.xml";
        } else if (MEDIA_RADIO == mMediaType) {
            xmlURI = "radio.xml";
        } else if (MEDIA_MOVIE == mMediaType) {
            xmlURI = "movie.xml";
        } else {
            xmlURI = "tv.xml";
        }
        IjkLog.d(TAG, "xmlURI: " + xmlURI);
        AssetLoader.copyAssetFile(context, xmlURI, AssetLoader.getDataPath(context) + xmlURI);
        IjkLog.d(TAG, "asset path:" + AssetLoader.getDataPath(context) + xmlURI);
        mContentList = EPGXmlParser.getContentDescriptors(AssetLoader.getDataPath(context) + xmlURI);
        IjkLog.d(TAG, "content counts:" + mContentList.size());

        if (0 == mContentList.size()) {
            Button contentButton = new Button(activity);
            contentButton.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
            contentButton.setText("parse failed with " + AssetLoader.getDataPath(context)+ xmlURI);
            layout.addView(contentButton);
            return;
        } else {
            for (int index = 0; index < mContentList.size(); index++) {
                ContentDescriptor descriptor = mContentList.get(index);

                Button contentButton = new Button(activity);
                Drawable rightIcon = descriptor.getDrawable(res);
                Drawable leftIcon = null;
                if (descriptor.getIsProtected()) {
                    leftIcon = res.getDrawable(android.R.drawable.ic_lock_lock);
                }
                contentButton.setCompoundDrawablesWithIntrinsicBounds(leftIcon, null, rightIcon, null);
                contentButton.setText(descriptor.getName());

                contentButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        String name = descriptor.getName();
                        String url = descriptor.getUrl();
                        IjkLog.d(TAG, "name:" + name + " url:" + url + " mediatype: " + mMediaType.toString());
                        VideoActivity.intentTo(activity, url, name, mMediaType);
                    }
                });
                layout.addView(contentButton);
            }
        }
    }

    static class EPGXmlParser {
        private String getXmlFromUrl(String url) {
            IjkLog.d(TAG, "url:" + url);
            if (url.startsWith("http://") || url.startsWith("https://")) {
                String xml = null;
                try {
                    DefaultHttpClient httpClient = new DefaultHttpClient();
                    HttpResponse httpResponse = httpClient.execute(new HttpGet(url));
                    HttpEntity httpEntity = httpResponse.getEntity();
                    xml = EntityUtils.toString(httpEntity, "UTF-8");
                } catch (Exception e) {
                    e.printStackTrace();
                    IjkLog.d(TAG, "getXmlFromUrl failed:" + e.getMessage());
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
                    IjkLog.d(TAG, "getXmlFromUrl failed:" + e.getMessage());
                } catch (IOException e) {
                    e.printStackTrace();
                    IjkLog.d(TAG, "getXmlFromUrl failed:" + e.getMessage());
                }

                try {
                    return new String(fileContent, encoding);
                } catch (UnsupportedEncodingException e) {
                    e.printStackTrace();
                    IjkLog.d(TAG, "getXmlFromUrl failed:" + e.getMessage());
                    return null;
                }
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
                IjkLog.d(TAG, "getDomElement failed:" + e.getMessage());
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

        public static List<ContentDescriptor> getContentDescriptors(String url) {
            List<ContentDescriptor> descriptors = new ArrayList<ContentDescriptor>();
            try {
                EPGXmlParser parser = new EPGXmlParser();
                String xml = parser.getXmlFromUrl(url);
                Document doc = parser.getDomElement(xml);
                NodeList nl = doc.getElementsByTagName("entry");
                IjkLog.d(TAG, "media counts:" + nl.getLength());
                for (int i = 0; i < nl.getLength(); i++) {
                    Node n = nl.item(i);
                    Element e = (Element) n;
                    Element linkElement = (Element) e.getElementsByTagName("link").item(0);
                    String contentTypeString = linkElement.getAttribute("urltype");

                    IjkUrlType urlType = null;
                    if (contentTypeString.equals("hls")) {
                        urlType = IjkUrlType.HLS;
                    } else if (contentTypeString.equals("rtmp")) {
                        urlType = IjkUrlType.RTMP;
                    } else if (contentTypeString.equals("dash")) {
                        urlType = IjkUrlType.DASH;
                    } else {
                        IjkLog.d(TAG, "unknown url type, set to FILE");
                        urlType = IjkUrlType.FILE;
                    }
                    boolean isProtected = false;
                    String protectedString = linkElement.getAttribute("protected");
                    if (protectedString.equals("yes")) {
                        isProtected = true;
                    }

                    String licenseURL = linkElement.getAttribute("laurl");
                    if (licenseURL.length() > 0 && licenseURL.startsWith("http")) {
                        IjkLog.d(TAG, "License/Authentication URL: " + licenseURL);
                    }

                    ContentDescriptor descriptor = new ContentDescriptor(
                            parser.getValue(e, "title"),
                            linkElement.getAttribute("href"),
                            urlType,
                            isProtected, licenseURL);
                    descriptors.add(descriptor);
                }
            } catch (Exception e) {
                IjkLog.d(TAG, "getContentDescriptors fatal error" + e.getMessage());
                descriptors.clear();
            }
            return descriptors;
        }
    }

}

