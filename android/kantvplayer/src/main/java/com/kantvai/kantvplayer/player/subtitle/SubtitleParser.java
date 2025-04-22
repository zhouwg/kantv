package com.kantvai.kantvplayer.player.subtitle;

import com.blankj.utilcode.util.StringUtils;
import com.blankj.utilcode.util.ToastUtils;
import com.kantvai.kantvplayer.player.subtitle.util.FatalParsingException;
import com.kantvai.kantvplayer.player.subtitle.util.SubtitleFormat;
import com.kantvai.kantvplayer.player.subtitle.util.TimedTextFileFormat;
import com.kantvai.kantvplayer.player.subtitle.util.TimedTextObject;

import java.io.File;
import java.io.IOException;
import java.nio.charset.UnsupportedCharsetException;


public class SubtitleParser{
    private String subtitlePath;

    public SubtitleParser(String path){
        this.subtitlePath = path;
    }

    public TimedTextObject parser() {
        try {
            if (!StringUtils.isEmpty(subtitlePath)) {
                File subtitleFile = new File(subtitlePath);
                if (subtitleFile.exists()) {
                    TimedTextFileFormat format = SubtitleFormat.format(subtitlePath);
                    if (format != null) {
                        TimedTextObject subtitleObj = format.parseFile(subtitleFile);
                        if (subtitleObj != null && subtitleObj.captions.size() > 0) {
                            return subtitleObj;
                        }else {
                            ToastUtils.showShort("parse subtitle file failure");
                        }
                    } else {
                        ToastUtils.showShort("invalid subtitle file");
                    }
                } else {
                    ToastUtils.showShort("subtitle file not exist");
                }
            } else {
                ToastUtils.showShort("invalid url of subtitle：" + subtitlePath);
            }
        } catch (FatalParsingException | IOException e) {
            e.printStackTrace();
            ToastUtils.showShort("parse subtitle failure");
        } catch (UnsupportedCharsetException ex) {
            ex.printStackTrace();
            ToastUtils.showShort("parse subtitle failure");
        }
        return null;
    }

//    private Charset getCharset(String filePath){
//        String encoding = "UTF-8";
//        InputStream inputStream = null;
//        BufferedInputStream bis = null;
//        try {
//            File file = new File(filePath);
//            inputStream = new FileInputStream(file);
//            bis = new BufferedInputStream(inputStream);
//            int code = (bis.read() << 8) + bis.read();
//            switch (code) {
//                case 0xefbb:
//                    encoding = "UTF-8";
//                    break;
//                case 0xfffe:
//                    encoding = "Unicode";
//                    break;
//                case 0xfeff:
//                    encoding = "UTF-16BE";
//                    break;
//                case 0x5c75:
//                    encoding = "ASCII";
//                    break;
//            }
//            LogUtils.e("encode of subtitle file："+encoding);
//        }catch (Exception e){
//            e.printStackTrace();
//            LogUtils.e("parse subtitle failure："+e);
//        }finally {
//            try {
//                if (bis != null)
//                    bis.close();
//                if (inputStream != null)
//                    inputStream.close();
//            }catch (IOException e){
//                e.printStackTrace();
//            }
//
//        }
//        return Charset.forName(encoding);
//    }
}
