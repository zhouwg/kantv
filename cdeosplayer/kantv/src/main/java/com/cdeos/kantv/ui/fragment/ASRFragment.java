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
 package com.cdeos.kantv.ui.fragment;

 import android.annotation.SuppressLint;
 import android.app.Activity;
 import android.content.Context;
 import android.content.res.Resources;
 import android.media.MediaPlayer;
 import android.text.Html;
 import android.text.method.LinkMovementMethod;
 import android.widget.Button;
 import android.widget.EditText;
 import android.widget.LinearLayout;
 import android.widget.TextView;
 import android.widget.Toast;

 import androidx.annotation.NonNull;
 import androidx.appcompat.app.AppCompatActivity;

 import com.cdeos.kantv.mvp.impl.ASRPresenterImpl;
 import com.cdeos.kantv.mvp.presenter.ASRPresenter;
 import com.cdeos.kantv.mvp.view.ASRView;
 import com.cdeos.kantv.R;
 import com.cdeos.kantv.base.BaseMvpFragment;
 import com.cdeos.kantv.utils.Settings;

 import java.io.File;
 import java.io.FileNotFoundException;
 import java.io.IOException;
 import java.io.RandomAccessFile;
 import java.nio.ByteBuffer;
 import java.nio.ByteOrder;

 import butterknife.BindView;
 import cdeos.media.player.CDEAssetLoader;
 import cdeos.media.player.CDELog;
 import cdeos.media.player.CDEUtils;

 import org.deepspeech.libdeepspeech.DeepSpeechModel;


 public class ASRFragment extends BaseMvpFragment<ASRPresenter> implements ASRView {
     @BindView(R.id.asr_layout)
     LinearLayout layout;

     private static final String TAG = ASRFragment.class.getName();
     DeepSpeechModel _m = null;
     EditText _audioFile;
     TextView _asrInfo;
     TextView _pageInfo;
     TextView _decodedString;
     TextView _tfliteStatus;

     Button _startInference;

     final int BEAM_WIDTH = 50;

     private String modelFileName = "deepspeech-0.9.3-models.tflite";
     private String audioFileName = "audio.wav";

     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;


     public static ASRFragment newInstance() {
         return new ASRFragment();
     }

     @NonNull
     @Override
     protected ASRPresenter initPresenter() {
         return new ASRPresenterImpl(this, this);
     }

     @Override
     protected int initPageLayoutId() {
         return R.layout.fragment_asr;
     }

     @SuppressLint("CheckResult")
     @Override
     public void initView() {
         long beginTime = 0;
         long endTime = 0;
         beginTime = System.currentTimeMillis();

         mActivity = getActivity();
         mContext = mActivity.getBaseContext();
         mSettings = new Settings(mContext);
         mSettings.updateUILang((AppCompatActivity) getActivity());
         Resources res = mActivity.getResources();

         _tfliteStatus = (TextView) mActivity.findViewById(R.id.tfliteStatus);
         _decodedString = (TextView) mActivity.findViewById(R.id.decodedString);
         _audioFile = (EditText) mActivity.findViewById(R.id.audioFile);

         _audioFile.setEnabled(false);
         _audioFile.setText("Audio file:" + audioFileName);

         _startInference = (Button) mActivity.findViewById(R.id.btnStartInference);
         _startInference.setOnClickListener(v -> {
             CDELog.j(TAG, "start ASR");
             this.newModel(CDEUtils.getDataPath(mContext) + modelFileName);
             if ((this._m != null) && (this._m.isInitOK())) {
                 this.playAudioFile();
                 this.doInference(CDEUtils.getDataPath(mContext) + audioFileName);
             }
         });

         _asrInfo = (TextView) mActivity.findViewById(R.id.asrinfo);
         _pageInfo = (TextView) mActivity.findViewById(R.id.pageinfo);


         //the dependent files(audio file audio.wav and model file deepspeech-0.9.3-models.tflite) of DeepSpeech are located at directory $PROJECT_ROOT_PATH/prebuilts
         //uncomment following lines and copy dependent files of DeepSpeech from $PROJECT_ROOT_PATH/prebuilts  to $PROJECT_ROOT_PATH/cdeosplayer/kantv/src/main/assets
         //the generated apk would be very big(about 90M)
         //CDEAssetLoader.copyAssetFile(mContext, modelFileName, CDEUtils.getDataPath(mContext) + modelFileName);
         //CDEAssetLoader.copyAssetFile(mContext, audioFileName, CDEUtils.getDataPath(mContext) + audioFileName);
         //the generated apk would be about 38M if you don't uncomment above two lines and download model file in "ASR Setting" was required when running the APK on phone
         
         //network bandwidth of default kantvserver is very low due to insufficient fund. so you should upload the dependent files of DeepSpeech to your local kantvserver
         //according to steps in $PROJECT_ROOT_PATH/README.md

         _asrInfo.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);


         String info = mContext.getString(R.string.asr_tip);
         _pageInfo.setText(Html.fromHtml(info));
         _pageInfo.setMovementMethod(LinkMovementMethod.getInstance());

         if (this._m != null) {
             info = "ASR engine version:" + this._m.getVersion();
         } else {
             info = mActivity.getBaseContext().getString(R.string.asr_engine_not_initialize);
         }
         _asrInfo.setText(info);


         File file = new File(CDEUtils.getDataPath(mContext) + modelFileName);
         if (file.exists()) {
             this.newModel(CDEUtils.getDataPath(mContext) + modelFileName);
         } else {
             CDELog.j(TAG, "model file not exist:" + file.getAbsolutePath());
             _asrInfo.setText("DeepSpeech's model file not exist");
         }

         endTime = System.currentTimeMillis();
         CDELog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
     }

     @Override
     public void initListener() {

     }


     @Override
     public void onDestroy() {
         super.onDestroy();
         if (this._m != null) {
             CDELog.j(TAG, "free ASR engine");
             this._m.freeModel();
         }
     }

     @Override
     public void onResume() {
         super.onResume();
     }


     @Override
     public void onStop() {
         super.onStop();
     }

     private char readLEChar(RandomAccessFile f) throws IOException {
         byte b1 = f.readByte();
         byte b2 = f.readByte();
         return (char) ((b2 << 8) | b1);
     }

     private int readLEInt(RandomAccessFile f) throws IOException {
         byte b1 = f.readByte();
         byte b2 = f.readByte();
         byte b3 = f.readByte();
         byte b4 = f.readByte();
         return (int) ((b1 & 0xFF) | (b2 & 0xFF) << 8 | (b3 & 0xFF) << 16 | (b4 & 0xFF) << 24);
     }

     private void newModel(String tfliteModel) {
         File file = new File(tfliteModel);
         if (!file.exists()) {
             CDELog.j(TAG, "model file not found:" + tfliteModel);
             Toast.makeText(mActivity, mActivity.getBaseContext().getString(R.string.please_download_asr_model), Toast.LENGTH_SHORT).show();
             return;
         } else {
             CDELog.j(TAG, "model file found:" + tfliteModel);
         }
         this._tfliteStatus.setText("Initialize ASR engine...");
         if (this._m == null) {
             // sphinx-doc: java_ref_model_start
             this._m = new DeepSpeechModel(tfliteModel);
             if (this._m.isInitOK()) {
                 this._m.setBeamWidth(BEAM_WIDTH);
             } else {
                 CDELog.j(TAG, "ASR engine initialized failed");
                 this._m = null;
                 String asrAudioFileName = CDEUtils.getDataPath(mContext) + audioFileName;
                 String asrModelFileName = CDEUtils.getDataPath(mContext) + modelFileName;
                 File asrAudioFile = new File(asrAudioFileName);
                 File asrModelFile = new File(asrModelFileName);
                 if (asrAudioFile.exists())
                     asrAudioFile.delete();
                 if (asrModelFile.exists())
                     asrModelFile.delete();

                 _asrInfo.setText(mActivity.getBaseContext().getString(R.string.please_download_asr_model));
                 return;
             }
         }
         this._tfliteStatus.setText("ASR engine initialized ok");

         String info;
         if (this._m != null) {
             info = "ASR engine:" + this._m.getVersion();
         } else {
             info = "ASR engine not initialized";
         }
         _asrInfo.setText(info);
     }

     private void doInference(String audioFile) {
         long inferenceExecTime = 0;

         this._startInference.setEnabled(false);

         this._tfliteStatus.setText("Extracting audio features ...");

         try {
             RandomAccessFile wave = new RandomAccessFile(audioFile, "r");

             wave.seek(20);
             char audioFormat = this.readLEChar(wave);
             assert (audioFormat == 1); // 1 is PCM

             wave.seek(22);
             char numChannels = this.readLEChar(wave);
             assert (numChannels == 1); // MONO

             wave.seek(24);
             int sampleRate = this.readLEInt(wave);
             assert (sampleRate == this._m.sampleRate()); // desired sample rate

             wave.seek(34);
             char bitsPerSample = this.readLEChar(wave);
             assert (bitsPerSample == 16); // 16 bits per sample

             wave.seek(40);
             int bufferSize = this.readLEInt(wave);
             assert (bufferSize > 0);

             wave.seek(44);
             byte[] bytes = new byte[bufferSize];
             wave.readFully(bytes);

             short[] shorts = new short[bytes.length / 2];
             // to turn bytes to shorts as either big endian or little endian.
             ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(shorts);

             this._tfliteStatus.setText("Running inference ...");

             long inferenceStartTime = System.currentTimeMillis();

             String decoded = this._m.stt(shorts, shorts.length);

             inferenceExecTime = System.currentTimeMillis() - inferenceStartTime;

             this._decodedString.setText("ASR result:" + decoded);

         } catch (FileNotFoundException ex) {

         } catch (IOException ex) {

         } finally {

         }
         this._tfliteStatus.setText("ASR cost: " + inferenceExecTime + " milliseconds");

         this._startInference.setEnabled(true);
     }

     public void playAudioFile() {
         try {
             MediaPlayer mediaPlayer = new MediaPlayer();
             mediaPlayer.setDataSource(CDEUtils.getDataPath(mContext) + audioFileName);
             mediaPlayer.prepare();
             mediaPlayer.start();
         } catch (IOException ex) {
             CDELog.j(TAG, "failed:" + ex.toString());
         } catch (Exception ex) {
             CDELog.j(TAG, "failed:" + ex.toString());
         }
     }


     public static native int kantv_anti_tamper();

 }
