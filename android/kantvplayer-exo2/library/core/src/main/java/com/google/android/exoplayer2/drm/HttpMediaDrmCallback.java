/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.google.android.exoplayer2.drm;

import static android.util.Base64.decode;

import android.annotation.SuppressLint;
import android.media.DeniedByServerException;
import android.media.MediaDrm;
import android.media.NotProvisionedException;
import android.media.ResourceBusyException;
import android.media.UnsupportedSchemeException;
import android.net.Uri;
import android.os.Build;
import android.text.TextUtils;
import android.util.Base64;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

import com.google.android.exoplayer2.C;
import com.google.android.exoplayer2.drm.ExoMediaDrm.KeyRequest;
import com.google.android.exoplayer2.drm.ExoMediaDrm.ProvisionRequest;
import com.google.android.exoplayer2.upstream.DataSourceInputStream;
import com.google.android.exoplayer2.upstream.DataSpec;
import com.google.android.exoplayer2.upstream.HttpDataSource;
import com.google.android.exoplayer2.upstream.HttpDataSource.InvalidResponseCodeException;
import com.google.android.exoplayer2.upstream.StatsDataSource;
import com.google.android.exoplayer2.util.Assertions;
import com.google.android.exoplayer2.util.Util;
import com.google.common.collect.ImmutableMap;

import java.nio.charset.StandardCharsets;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import kantvai.media.player.KANTVDataEntity;
import kantvai.media.player.KANTVHttpsURLConnection;
import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;
import com.alibaba.fastjson.JSON;
import com.alibaba.fastjson.JSONObject;

/** A {@link MediaDrmCallback} that makes requests using {@link HttpDataSource} instances. */
public final class HttpMediaDrmCallback implements MediaDrmCallback {
  private static final String TAG = HttpMediaDrmCallback.class.getName();

  private static final int MAX_MANUAL_REDIRECTS = 5;

  private final HttpDataSource.Factory dataSourceFactory;
  @Nullable private final String defaultLicenseUrl;
  private final boolean forceDefaultLicenseUrl;
  private final Map<String, String> keyRequestProperties;

  /**
   * @param defaultLicenseUrl The default license URL. Used for key requests that do not specify
   *     their own license URL. May be {@code null} if it's known that all key requests will specify
   *     their own URLs.
   * @param dataSourceFactory A factory from which to obtain {@link HttpDataSource} instances.
   */
  public HttpMediaDrmCallback(
      @Nullable String defaultLicenseUrl, HttpDataSource.Factory dataSourceFactory) {
    this(defaultLicenseUrl, /* forceDefaultLicenseUrl= */ false, dataSourceFactory);
  }

  /**
   * @param defaultLicenseUrl The default license URL. Used for key requests that do not specify
   *     their own license URL, or for all key requests if {@code forceDefaultLicenseUrl} is set to
   *     true. May be {@code null} if {@code forceDefaultLicenseUrl} is {@code false} and if it's
   *     known that all key requests will specify their own URLs.
   * @param forceDefaultLicenseUrl Whether to force use of {@code defaultLicenseUrl} for key
   *     requests that include their own license URL.
   * @param dataSourceFactory A factory from which to obtain {@link HttpDataSource} instances.
   */
  public HttpMediaDrmCallback(
      @Nullable String defaultLicenseUrl,
      boolean forceDefaultLicenseUrl,
      HttpDataSource.Factory dataSourceFactory) {
    Assertions.checkArgument(!(forceDefaultLicenseUrl && TextUtils.isEmpty(defaultLicenseUrl)));
    this.dataSourceFactory = dataSourceFactory;
    this.defaultLicenseUrl = defaultLicenseUrl;
    this.forceDefaultLicenseUrl = forceDefaultLicenseUrl;
    this.keyRequestProperties = new HashMap<>();
  }

  /**
   * Sets a header for key requests made by the callback.
   *
   * @param name The name of the header field.
   * @param value The value of the field.
   */
  public void setKeyRequestProperty(String name, String value) {
    Assertions.checkNotNull(name);
    Assertions.checkNotNull(value);
    synchronized (keyRequestProperties) {
      keyRequestProperties.put(name, value);
    }
  }

  /**
   * Clears a header for key requests made by the callback.
   *
   * @param name The name of the header field.
   */
  public void clearKeyRequestProperty(String name) {
    Assertions.checkNotNull(name);
    synchronized (keyRequestProperties) {
      keyRequestProperties.remove(name);
    }
  }

  /** Clears all headers for key requests made by the callback. */
  public void clearAllKeyRequestProperties() {
    synchronized (keyRequestProperties) {
      keyRequestProperties.clear();
    }
  }

  @Override
  public byte[] executeProvisionRequest(UUID uuid, ProvisionRequest request)
      throws MediaDrmCallbackException {
    //weiguo
    KANTVLog.j(TAG, "executeProvisionRequest for uuid: " + uuid.toString());
    if (KANTVUtils.getDecryptMode() == KANTVUtils.DECRYPT_TEE) {
      if (uuid.equals(C.CHINADRM_UUID)) {
        String url = request.getDefaultUrl();
        KANTVLog.j(TAG, "url: " + url);
        return executePost(
                dataSourceFactory,
                url,
                request.getData(),
                /* requestProperties= */ Collections.emptyMap());
      }
    }

    String url =
        request.getDefaultUrl() + "&signedRequest=" + Util.fromUtf8Bytes(request.getData());
    return executePost(
        dataSourceFactory,
        url,
        /* httpBody= */ null,
        /* requestProperties= */ Collections.emptyMap());
  }

  @Override
  public byte[] executeKeyRequest(UUID uuid, KeyRequest request) throws MediaDrmCallbackException {
    //weiguo
    KANTVLog.j(TAG, "executeKeyRequest for uuid: " + uuid.toString() + "(" + KANTVUtils.getDrmName(uuid) + ")");

    String url = request.getLicenseServerUrl();
    if (forceDefaultLicenseUrl || TextUtils.isEmpty(url)) {
      url = defaultLicenseUrl;
    }

    //weiguo
    KANTVLog.j(TAG, "url: " + url);

    if (TextUtils.isEmpty(url)) {
      throw new MediaDrmCallbackException(
          new DataSpec.Builder().setUri(Uri.EMPTY).build(),
          Uri.EMPTY,
          /* responseHeaders= */ ImmutableMap.of(),
          /* bytesLoaded= */ 0,
          /* cause= */ new IllegalStateException("No license URL"));
    }
    Map<String, String> requestProperties = new HashMap<>();
    // Add standard request properties for supported schemes.
    String contentType =
        C.PLAYREADY_UUID.equals(uuid)
            ? "text/xml"
            : (C.CLEARKEY_UUID.equals(uuid) ? "application/json" : "application/octet-stream");

    //weiguo
    if (C.CHINADRM_UUID.equals(uuid)) {
      //if (KANTVUtils.getDecryptMode() == KANTVUtils.DECRYPT_TEE)
      {
        contentType = "application/json";
      }
    }

    //weiguo
    if (C.WISEPLAY_UUID.equals(uuid)) {
      //if (KANTVUtils.getDecryptMode() == KANTVUtils.DECRYPT_TEE)
      {
        contentType = "application/json";
      }
    }
    KANTVLog.d(TAG, "contentType: " + contentType);

    requestProperties.put("Content-Type", contentType);
    if (C.PLAYREADY_UUID.equals(uuid)) {
      requestProperties.put(
          "SOAPAction", "http://schemas.microsoft.com/DRM/2007/03/protocols/AcquireLicense");
    }

    //weiguo
    if (C.CHINADRM_UUID.equals(uuid)) {
        //fairplay, jwt, ...
    }

    // Add additional request properties.
    synchronized (keyRequestProperties) {
      requestProperties.putAll(keyRequestProperties);
    }
    try {
      String tmpString = new String(request.getData(), "utf-8");
      KANTVLog.d(TAG, "request data:(" + request.getData().length + ") " + tmpString);
    } catch (Exception e) {
      KANTVLog.j(TAG, "error:" + e.toString());
    }
    KANTVLog.d(TAG, "requestProperties:" + requestProperties.size());
    //weiguo
    if (KANTVUtils.getEnableWisePlay() && C.WISEPLAY_UUID.equals(uuid)) {
        //wiseplay
    }

    return executePost(dataSourceFactory, url, request.getData(), requestProperties);
  }


  private static byte[] executePost(
      HttpDataSource.Factory dataSourceFactory,
      String url,
      @Nullable byte[] httpBody,
      Map<String, String> requestProperties)
      throws MediaDrmCallbackException {
    StatsDataSource dataSource = new StatsDataSource(dataSourceFactory.createDataSource());
    int manualRedirectCount = 0;
    DataSpec dataSpec =
        new DataSpec.Builder()
            .setUri(url)
            .setHttpRequestHeaders(requestProperties)
            .setHttpMethod(DataSpec.HTTP_METHOD_POST)
            .setHttpBody(httpBody)
            .setFlags(DataSpec.FLAG_ALLOW_GZIP)
            .build();
    DataSpec originalDataSpec = dataSpec;
    try {
      while (true) {
        DataSourceInputStream inputStream = new DataSourceInputStream(dataSource, dataSpec);
        try {
          return Util.toByteArray(inputStream);
        } catch (InvalidResponseCodeException e) {
          KANTVLog.j(TAG, "failed:" + e.toString());
          @Nullable String redirectUrl = getRedirectUrl(e, manualRedirectCount);
          if (redirectUrl == null) {
            throw e;
          }
          manualRedirectCount++;
          dataSpec = dataSpec.buildUpon().setUri(redirectUrl).build();
        } finally {
          Util.closeQuietly(inputStream);
        }
      }
    } catch (Exception e) {
      KANTVLog.j(TAG, "failed:" + e.toString());
      throw new MediaDrmCallbackException(
          originalDataSpec,
          Assertions.checkNotNull(dataSource.getLastOpenedUri()),
          dataSource.getResponseHeaders(),
          dataSource.getBytesRead(),
          /* cause= */ e);
    }
  }

  @Nullable
  private static String getRedirectUrl(
      InvalidResponseCodeException exception, int manualRedirectCount) {
    // For POST requests, the underlying network stack will not normally follow 307 or 308
    // redirects automatically. Do so manually here.
    boolean manuallyRedirect =
        (exception.responseCode == 307 || exception.responseCode == 308)
            && manualRedirectCount < MAX_MANUAL_REDIRECTS;
    if (!manuallyRedirect) {
      return null;
    }
    Map<String, List<String>> headerFields = exception.headerFields;
    if (headerFields != null) {
      @Nullable List<String> locationHeaders = headerFields.get("Location");
      if (locationHeaders != null && !locationHeaders.isEmpty()) {
        return locationHeaders.get(0);
      }
    }
    return null;
  }
}
