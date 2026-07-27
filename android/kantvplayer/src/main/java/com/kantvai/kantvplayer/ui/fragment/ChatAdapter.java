/*
 * Copyright (c) 2024- KanTV Authors
 *
 * RecyclerView adapter for the AI research screen chat UI.
 *
 * Two view types - user (right bubble, optional image/audio attachment) and
 * assistant (left bubble, streamed text). The streaming use case is the
 * tricky one: the native side keeps pushing tokens at us through a Handler,
 * and we want to keep the same row growing rather than recreating it. The
 * contract is:
 *
 *   1. caller calls addAssistantPlaceholder() right before firing inference
 *   2. native callback calls appendToLast(chunk) for every token chunk
 *   3. native callback calls markLastComplete() (or markLastError()) when
 *      the inference is done
 *
 * The adapter does NOT mutate the list during append - it only notifies a
 * single row change so RecyclerView can rebind the affected ViewHolder.
 */
package com.kantvai.kantvplayer.ui.fragment;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Typeface;
import android.net.Uri;
import android.text.Editable;
import android.text.style.BackgroundColorSpan;
import android.text.style.RelativeSizeSpan;
import android.text.style.StyleSpan;
import android.text.style.TypefaceSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.ui.fragment.ChatMessage.AttachmentType;
import com.kantvai.kantvplayer.ui.fragment.ChatMessage.Role;
import com.kantvai.kantvplayer.ui.fragment.ChatMessage.State;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

import io.noties.markwon.Markwon;
import kantvai.media.player.KANTVLog;

public class ChatAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    private static final String TAG = ChatAdapter.class.getSimpleName();

    private static final int TYPE_USER      = 1;
    private static final int TYPE_ASSISTANT = 2;

    private final Context mContext;
    private final List<ChatMessage> mMessages = new ArrayList<>();
    private final SimpleDateFormat mTimeFormat =
            new SimpleDateFormat("HH:mm:ss", Locale.US);
    // Markdown renderer for completed assistant bubbles. STREAMING/ERROR
    // bubbles stay on plain TextView.setText() so the user doesn't see
    // mid-stream re-render flicker, and so the "..." placeholder isn't
    // parsed as markdown. May be null if the caller doesn't supply one.
    private final Markwon mMarkwon;

    public ChatAdapter(Context context) {
        this(context, null);
    }

    public ChatAdapter(Context context, Markwon markwon) {
        mContext = context;
        mMarkwon = markwon;
    }

    // ---------------------------------------------------------------------
    // Mutation API used by the Fragment and the native callback
    // ---------------------------------------------------------------------

    /** Add a finished user turn (text + optional attachment). */
    public void addUserMessage(String text, String attachmentPath, AttachmentType attachmentType) {
        ChatMessage msg = new ChatMessage(Role.USER);
        if (text != null) {
            msg.text.append(text);
        }
        msg.attachmentPath = attachmentPath;
        msg.attachmentType = attachmentType != null ? attachmentType : AttachmentType.NONE;
        msg.state = State.COMPLETE;
        mMessages.add(msg);
        notifyItemInserted(mMessages.size() - 1);
    }

    /**
     * Reserve a streaming assistant row and return its index. The fragment
     * calls this immediately before firing inference, so the very first
     * chunk already has somewhere to land.
     */
    public int addAssistantPlaceholder() {
        ChatMessage msg = new ChatMessage(Role.ASSISTANT);
        msg.text.append("...");
        mMessages.add(msg);
        int idx = mMessages.size() - 1;
        notifyItemInserted(idx);
        return idx;
    }

    /** Append a streaming chunk to the most recent assistant message. */
    public void appendToLast(String chunk) {
        if (chunk == null || chunk.isEmpty() || mMessages.isEmpty()) {
            return;
        }
        ChatMessage last = mMessages.get(mMessages.size() - 1);
        if (last.role != Role.ASSISTANT) {
            // Out-of-order token; we silently drop. The fragment's contract
            // is to never call append outside an active inference, so this
            // would only happen if a leftover callback fires after a clear.
            KANTVLog.j(TAG, "appendToLast: last role is not ASSISTANT, drop chunk");
            return;
        }
        // The placeholder is a "..." dot, replace it on the first real chunk
        // so the user doesn't see the placeholder for too long.
        if (last.text.length() == 3 && last.text.toString().equals("...")) {
            last.text.setLength(0);
        }
        last.text.append(chunk);
        notifyItemChanged(mMessages.size() - 1);
    }

    /** Flip the most recent assistant message into COMPLETE state. */
    public void markLastComplete() {
        if (mMessages.isEmpty()) {
            return;
        }
        ChatMessage last = mMessages.get(mMessages.size() - 1);
        if (last.role != Role.ASSISTANT) {
            return;
        }
        last.state = State.COMPLETE;
        notifyItemChanged(mMessages.size() - 1);
    }

    /** Flip the most recent assistant message into ERROR state. */
    public void markLastError() {
        if (mMessages.isEmpty()) {
            return;
        }
        ChatMessage last = mMessages.get(mMessages.size() - 1);
        if (last.role != Role.ASSISTANT) {
            return;
        }
        last.state = State.ERROR;
        notifyItemChanged(mMessages.size() - 1);
    }

    /** Wipe the whole conversation. */
    public void clear() {
        int n = mMessages.size();
        if (n == 0) {
            return;
        }
        mMessages.clear();
        notifyItemRangeRemoved(0, n);
    }

    public int getMessageCount() {
        return mMessages.size();
    }

    // ---------------------------------------------------------------------
    // RecyclerView.Adapter
    // ---------------------------------------------------------------------

    @Override
    public int getItemViewType(int position) {
        return mMessages.get(position).role == Role.USER ? TYPE_USER : TYPE_ASSISTANT;
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        LayoutInflater inflater = LayoutInflater.from(parent.getContext());
        if (viewType == TYPE_USER) {
            return new UserViewHolder(inflater.inflate(R.layout.item_chat_user, parent, false));
        }
        return new AssistantViewHolder(inflater.inflate(R.layout.item_chat_assistant, parent, false));
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
        ChatMessage msg = mMessages.get(position);
        if (holder instanceof UserViewHolder) {
            ((UserViewHolder) holder).bind(msg);
        } else if (holder instanceof AssistantViewHolder) {
            ((AssistantViewHolder) holder).bind(msg);
        }
    }

    @Override
    public int getItemCount() {
        return mMessages.size();
    }

    // ---------------------------------------------------------------------
    // ViewHolders
    // ---------------------------------------------------------------------

    class UserViewHolder extends RecyclerView.ViewHolder {
        final TextView textView;
        final ImageView attachmentView;
        final TextView timeView;

        UserViewHolder(View itemView) {
            super(itemView);
            textView = itemView.findViewById(R.id.chatUserText);
            attachmentView = itemView.findViewById(R.id.chatUserAttachment);
            timeView = itemView.findViewById(R.id.chatUserTime);
        }

        void bind(ChatMessage msg) {
            textView.setText(msg.getText());
            timeView.setText(formatTime(msg.timestamp));

            if (msg.attachmentPath != null && !msg.attachmentPath.isEmpty()
                    && msg.attachmentType != AttachmentType.NONE) {
                attachmentView.setVisibility(View.VISIBLE);
                if (msg.attachmentType == AttachmentType.IMAGE) {
                    try {
                        attachmentView.setImageURI(Uri.fromFile(new File(msg.attachmentPath)));
                    } catch (Exception e) {
                        KANTVLog.j(TAG, "failed to load attachment: " + e.toString());
                        attachmentView.setImageResource(android.R.drawable.ic_menu_gallery);
                    }
                } else {
                    // Audio - show a generic audio glyph
                    attachmentView.setImageResource(android.R.drawable.ic_media_play);
                }
            } else {
                attachmentView.setVisibility(View.GONE);
            }
        }
    }

    class AssistantViewHolder extends RecyclerView.ViewHolder {
        final TextView textView;
        final TextView timeView;

        // Debug dump state. When enabled, every bind() writes a snapshot
        // of the rendered text + span list to DUMP_DIR/state.txt and a
        // PNG of the TextView's current pixels to DUMP_DIR/frame_*.png,
        // throttled to one sample per DUMP_INTERVAL_MS. The user pulls
        // these files off-device to diagnose transient streaming render
        // bugs (e.g. headers being rendered at the wrong size during
        // mid-stream rebinds). Toggle the flag here to disable; old
        // files can be cleared with
        //   adb shell rm -rf <DUMP_DIR>
        // from a host shell.
        private static final boolean DUMP_ENABLED   = true;
        private static final long   DUMP_INTERVAL_MS = 200L; // 5 fps
        // DUMP_DIR lives under the app's external files dir so we don't
        // need WRITE_EXTERNAL_STORAGE on Android 10+. Pull with:
        //   adb pull /sdcard/Android/data/com.kantvai.kantvplayer/files/chat_dumps
        private static final String DUMP_DIR_NAME   = "chat_dumps";
        private final File mDumpDir;
        private long mLastDumpTime = 0L;
        private int  mDumpFrameSeq = 0;

        AssistantViewHolder(View itemView) {
            super(itemView);
            textView = itemView.findViewById(R.id.chatAssistantText);
            timeView = itemView.findViewById(R.id.chatAssistantTime);
            File base = mContext.getExternalFilesDir(null);
            mDumpDir = (base != null) ? new File(base, DUMP_DIR_NAME) : null;
            if (mDumpDir != null && !mDumpDir.exists()) {
                // mkdirs() returns false if it already exists or if
                // creation failed; we don't care either way - the dump
                // code below tolerates a missing dir by silently
                // skipping the write.
                mDumpDir.mkdirs();
            }
        }

        void bind(ChatMessage msg) {
            // Always render with Markwon, even during streaming, so
            // the user sees **bold** / lists / headings as the tokens
            // arrive rather than a wall of asterisks that suddenly
            // formats when the row flips to COMPLETE. Re-parse cost
            // is bounded by the 80ms throttle in
            // AIResearchFragment.enqueueStreamingChunk - we re-bind at
            // most ~12.5 times per second, not per-token. ERROR state
            // stays plain so the warning prefix isn't parsed as
            // markdown.
            if (msg.state == State.ERROR) {
                textView.setText("⚠ " + msg.getText());
            } else if (mMarkwon != null) {
                mMarkwon.setMarkdown(textView, msg.getText());
            } else {
                textView.setText(msg.getText());
            }
            String caret = (msg.state == State.STREAMING) ? " ▌" : "";
            timeView.setText(formatTime(msg.timestamp) + caret);
            maybeDumpStreamingState(msg);
        }

        // -----------------------------------------------------------------
        // Debug dump helpers
        // -----------------------------------------------------------------

        private void maybeDumpStreamingState(ChatMessage msg) {
            if (!DUMP_ENABLED || mDumpDir == null) {
                return;
            }
            long now = System.currentTimeMillis();
            if (now - mLastDumpTime < DUMP_INTERVAL_MS) {
                return;
            }
            mLastDumpTime = now;
            mDumpFrameSeq++;
            dumpTextAndSpans(msg, now, mDumpFrameSeq);
            dumpBitmap(now, mDumpFrameSeq);
        }

        private void dumpTextAndSpans(ChatMessage msg, long ts, int seq) {
            File out = new File(mDumpDir, "state.txt");
            FileWriter fw = null;
            try {
                fw = new FileWriter(out, true);
                String text = msg.getText();
                fw.write("=== seq=" + seq + " ts=" + ts
                        + " state=" + msg.state
                        + " length=" + text.length() + " ===\n");
                // Truncate the text body so a 500-token response doesn't
                // blow up the log; the spans below still reference real
                // character offsets so the user can see the structure.
                String body = text.replace("\n", "\\n");
                if (body.length() > 2000) {
                    body = body.substring(0, 2000) + "...(+" + (text.length() - 2000) + ")";
                }
                fw.write("TEXT: " + body + "\n");

                // Spans live on the TextView's Editable, not on the
                // ChatMessage's buffer. We dump whatever Markwon or our
                // own code has applied at this point.
                CharSequence cs = textView.getText();
                if (cs instanceof Editable) {
                    Editable editable = (Editable) cs;
                    int len = editable.length();

                    StyleSpan[] styles = editable.getSpans(0, len, StyleSpan.class);
                    fw.write("StyleSpan count=" + styles.length + "\n");
                    for (StyleSpan s : styles) {
                        int st = editable.getSpanStart(s);
                        int en = editable.getSpanEnd(s);
                        fw.write("  style=" + styleName(s.getStyle())
                                + " [" + st + "," + en + ")"
                                + " text=\"" + safeSlice(text, st, en) + "\"\n");
                    }

                    RelativeSizeSpan[] sizes = editable.getSpans(0, len, RelativeSizeSpan.class);
                    fw.write("RelativeSizeSpan count=" + sizes.length + "\n");
                    for (RelativeSizeSpan s : sizes) {
                        int st = editable.getSpanStart(s);
                        int en = editable.getSpanEnd(s);
                        fw.write("  scale=" + s.getSizeChange()
                                + " [" + st + "," + en + ")"
                                + " text=\"" + safeSlice(text, st, en) + "\"\n");
                    }

                    TypefaceSpan[] faces = editable.getSpans(0, len, TypefaceSpan.class);
                    fw.write("TypefaceSpan count=" + faces.length + "\n");
                    for (TypefaceSpan s : faces) {
                        int st = editable.getSpanStart(s);
                        int en = editable.getSpanEnd(s);
                        fw.write("  family=\"" + s.getFamily() + "\""
                                + " [" + st + "," + en + ")"
                                + " text=\"" + safeSlice(text, st, en) + "\"\n");
                    }

                    BackgroundColorSpan[] bgs = editable.getSpans(0, len, BackgroundColorSpan.class);
                    fw.write("BackgroundColorSpan count=" + bgs.length + "\n");
                    for (BackgroundColorSpan s : bgs) {
                        int st = editable.getSpanStart(s);
                        int en = editable.getSpanEnd(s);
                        fw.write("  color=" + Integer.toHexString(s.getBackgroundColor())
                                + " [" + st + "," + en + ")"
                                + " text=\"" + safeSlice(text, st, en) + "\"\n");
                    }
                } else {
                    fw.write("(textView.getText() is not Editable: " + cs.getClass().getName() + ")\n");
                }
                fw.write("\n");
            } catch (Throwable t) {
                KANTVLog.j(TAG, "dumpTextAndSpans failed: " + t);
            } finally {
                if (fw != null) {
                    try { fw.close(); } catch (Throwable ignore) {}
                }
            }
        }

        private void dumpBitmap(long ts, int seq) {
            int w = textView.getWidth();
            int h = textView.getHeight();
            if (w <= 0 || h <= 0) {
                return; // not laid out yet
            }
            Bitmap bmp = null;
            FileOutputStream fos = null;
            try {
                bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
                Canvas canvas = new Canvas(bmp);
                // White background so the PNG looks like the screen
                // rather than transparent pixels.
                canvas.drawColor(0xFFFFFFFF);
                textView.draw(canvas);
                File out = new File(mDumpDir, "frame_" + seq + "_" + ts + ".png");
                fos = new FileOutputStream(out);
                bmp.compress(Bitmap.CompressFormat.PNG, 100, fos);
            } catch (Throwable t) {
                KANTVLog.j(TAG, "dumpBitmap failed: " + t);
            } finally {
                if (fos != null) {
                    try { fos.close(); } catch (Throwable ignore) {}
                }
                if (bmp != null) {
                    bmp.recycle();
                }
            }
        }
    }

    // styleName / safeSlice live on the outer class because Java does
    // not allow `static` on methods inside a non-static inner class
    // (only on compile-time constants). AssistantViewHolder inherits
    // the outer scope and calls them unqualified.
    private static String styleName(int style) {
        switch (style) {
            case Typeface.BOLD:        return "BOLD";
            case Typeface.ITALIC:      return "ITALIC";
            case Typeface.BOLD_ITALIC: return "BOLD_ITALIC";
            default:                   return "0x" + Integer.toHexString(style);
        }
    }

    private static String safeSlice(String text, int start, int end) {
        if (text == null) return "";
        if (start < 0) start = 0;
        if (end > text.length()) end = text.length();
        if (start >= end) return "";
        return text.substring(start, end).replace("\n", "\\n");
    }

    private String formatTime(long ts) {
        return mTimeFormat.format(new Date(ts));
    }
}
