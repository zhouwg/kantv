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

 import java.io.ByteArrayOutputStream;
 import java.io.IOException;
 import java.io.InputStream;
 import java.io.OutputStream;
 import java.net.HttpURLConnection;
 import java.net.URL;
 import java.security.KeyManagementException;
 import java.security.NoSuchAlgorithmException;
 import java.security.SecureRandom;

 import javax.net.ssl.HostnameVerifier;
 import javax.net.ssl.HttpsURLConnection;
 import javax.net.ssl.SSLContext;
 import javax.net.ssl.SSLSession;
 import javax.net.ssl.TrustManager;


 public class KANTVHttpsURLConnection {
     private static final String TAG = KANTVHttpsURLConnection.class.getName();
     public static final int HTTP_GETMETHOD_WITHJWT = 1;
     public static final int HTTP_POSTMETHOD_WITHJWT = 2;
     public static final int HTTP_POSTMETHOD_WITHOUTJWT = 3;

     private String jwtStr = null;

     private SSLContext sslcontext = null;
     private TrustManager[] trustManager = {new KANTVX509TrustManager()};
     private HostnameVerifier ignoreHostnameVerifier = new HostnameVerifier() {
         public boolean verify(String s, SSLSession sslsession) {
             return true;
         }
     };

     public KANTVHttpsURLConnection() {
         try {
             sslcontext = SSLContext.getInstance("SSL");
             sslcontext.init(null, trustManager, new SecureRandom());

             HttpsURLConnection.setDefaultHostnameVerifier(ignoreHostnameVerifier);
             HttpsURLConnection.setDefaultSSLSocketFactory(sslcontext.getSocketFactory());

         } catch (NoSuchAlgorithmException | KeyManagementException e) {
             e.printStackTrace();
             KANTVLog.j(TAG, "init failed:" + e.toString());
         }
     }

     public int doHttpRequest(int requestType, String url, int requestBodyLength, byte[] requestBody, KANTVDataEntity dataEntity) {
         int result = 0;

         if (HTTP_GETMETHOD_WITHJWT == requestType) {
             result = doHttpGet(url, dataEntity);

         } else if (HTTP_POSTMETHOD_WITHJWT == requestType) {
             result = doHttpPostWithJWT(url, requestBodyLength, requestBody, dataEntity, this.jwtStr);
         } else if (HTTP_POSTMETHOD_WITHOUTJWT == requestType) {
             result = doHttpPost(url, requestBodyLength, requestBody, dataEntity);
         }

         return result;
     }


     private int doHttpGet(String url, KANTVDataEntity dataEntity) {
         int result = 0;

         URL myUrl = null;
         HttpURLConnection conn = null;
         try {
             myUrl = new URL(url);
             conn = (HttpURLConnection) myUrl.openConnection();

             conn.setRequestMethod("GET");

             conn.setConnectTimeout(12000);

             dataEntity.setResult(conn.getResponseCode());

             //Check response code before reading the data.
             if (200 == conn.getResponseCode()) {
                 InputStream in = conn.getInputStream();
                 byte[] responseBody = readStream(in);
                 in.close();
                 dataEntity.setDataLength(responseBody.length);
                 dataEntity.setData(responseBody);
             }


         } catch (IOException e) {
             e.printStackTrace();
             KANTVLog.j(TAG, "error:" + e.toString());
         }

         return result;
     }

     public int doHttpPost(String url, int requestBodyLength, byte[] requestBody, KANTVDataEntity dataEntity) {
         return doHttpPostWithJWT(url, requestBodyLength, requestBody, dataEntity, null);
     }


     private int doHttpPostWithJWT(String url, int requestBodyLength, byte[] requestBody, KANTVDataEntity dataEntity, String jwt) {
         int result = 0;

         URL myUrl = null;
         HttpURLConnection conn = null;
         try {
             myUrl = new URL(url);
             conn = (HttpURLConnection) myUrl.openConnection();

             conn.setRequestMethod("POST");

             conn.setConnectTimeout(12000);

             conn.setDoInput(true);
             conn.setDoOutput(true);
             conn.setUseCaches(false);

             conn.setRequestProperty("Content-Type", "application/json;charset=UTF-8");
             conn.setRequestProperty("Accept", "application/json");

             if (null != jwt) {
                 conn.setRequestProperty("Authorization", jwt);
             }

             OutputStream outputStream = conn.getOutputStream();
             outputStream.write(requestBody, 0, requestBodyLength);
             outputStream.flush();
             outputStream.close();

             dataEntity.setResult(conn.getResponseCode());

             if (200 == conn.getResponseCode()) {
                 InputStream in = conn.getInputStream();
                 byte[] responseBody = readStream(in);
                 in.close();
                 dataEntity.setDataLength(responseBody.length);
                 dataEntity.setData(responseBody);
             }


         } catch (IOException e) {
             KANTVLog.j(TAG, "doHttpPostWithJWT exception: " + e);
             result = -1;
         } catch (Exception e) {
             KANTVLog.j(TAG, "doHttpPostWithJWT exception: " + e);
             result = -2;
         }

         return result;
     }


     private byte[] readStream(InputStream in) throws IOException {
         ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
         byte[] buffer = new byte[188 * 2048]; //188 bytes per ts packet
         int length = 0;

         while ((length = in.read(buffer)) != -1) {
             outputStream.write(buffer, 0, length);
         }

         byte[] data = outputStream.toByteArray();
         outputStream.close();

         return data;
     }

     public static native int kantv_anti_remove_rename_this_file();

 }
