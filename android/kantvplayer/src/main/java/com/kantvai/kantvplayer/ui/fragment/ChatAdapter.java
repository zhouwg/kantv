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
 * During streaming, appendToLast() updates the trailing assistant row
 * DIRECTLY (raw append + periodic markwon full re-render), without
 * notifyItemChanged - because every rebind was producing a visible
 * flicker (TextView re-measure on every chunk). The direct path is
 * enabled by caching the currently-bound AssistantViewHolder in
 * mStreamingHolder; if that cache is empty (e.g. the row was just
 * inserted and not yet bound, or the user scrolled it off-screen and
 * the holder was recycled) we fall back to notifyItemChanged.
 */
package com.kantvai.kantvplayer.ui.fragment;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Typeface;
import android.net.Uri;
import android.text.Editable;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
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

import kantvai.media.player.KANTVLog;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

import kantvai.media.player.KANTVLog;
import kantvai.tool.markwon.io.noties.markwon.Markwon;

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

    // The AssistantViewHolder that's currently bound to the streaming
    // (last) row, and the position it sits at. Cached so appendToLast()
    // can update the row's TextView directly (editable.append + copy
    // spans) WITHOUT calling notifyItemChanged - which is what causes
    // the visible "flicker" during inference (a re-bind triggers a full
    // TextView re-measure, even when the text grew by a single token).
    //
    // Set by AssistantViewHolder.bind() and cleared by its
    // onViewRecycled() (so a scrolled-off holder can't be reused as the
    // "live" one) and by markLastComplete / markLastError / clear
    // (so a finished row is never accidentally re-stamped).
    private AssistantViewHolder mStreamingHolder = null;
    private int mStreamingPosition = RecyclerView.NO_POSITION;

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
        int idx = mMessages.size() - 1;
        KANTVLog.g(TAG, "addUserMessage: idx=" + idx
                + " role=USER state=COMPLETE textLen=" + msg.getText().length()
                + " att=" + msg.attachmentType
                + " size=" + mMessages.size());
        notifyItemInserted(idx);
    }

    /**
     * Reserve a streaming assistant row and return its index. The fragment
     * calls this immediately before firing inference, so the very first
     * chunk already has somewhere to land.
     */
    public int addAssistantPlaceholder() {
        // Defensive guard: if the previous assistant message is still
        // in STREAMING state when a new turn starts, force it into
        // COMPLETE now. This is the root cause of the reported
        // "second question and answer land in the previous assistant
        // bubble" symptom:
        //
        //   1. markLastComplete() is ONLY called from the native
        //      callback path that detects a line starting with
        //      "llama-timings" (see AIResearchFragment L927-L948).
        //      If the native side never emits that line for a given
        //      model/backend combination (e.g. the small
        //      gemma-4-E2B-it-Q4_0 model on a code path that exits
        //      through a different timing string, or any path that
        //      errors out before the timings line is printed), A1
        //      stays in STREAMING state forever.
        //
        //   2. A stale STREAMING row is dangerous because it still
        //      "looks like" the active streaming row to bind() - if
        //      RecyclerView rebinds it for any reason (layout pass,
        //      scroll, notifyItemRangeChanged), the STREAMING branch
        //      in AssistantViewHolder.bind() can re-register
        //      mStreamingHolder = this on a non-trailing position.
        //      A subsequent appendToLast() chunk then either lands
        //      on the wrong TextView (via a race window where the
        //      safety gate in bind() hasn't fired yet) or triggers
        //      a rebind storm.
        //
        //   3. Forcing the previous STREAMING row to COMPLETE here
        //      BEFORE we add the new placeholder guarantees:
        //        - the data model has at most one STREAMING row
        //          (the new one) at all times
        //        - mStreamingHolder is null at the moment we add
        //          the placeholder, so the next bind() on the new
        //          row has a clean slate to register
        //        - the markwon.setMarkdown() render fires on the
        //          stale row's NEXT rebind, so the user still
        //          sees the previous response fully rendered
        //
        // The cost is one extra notifyItemChanged() per turn start
        // in the rare markLastComplete()-missed case, which is
        // negligible compared to the bug it fixes.
        // Scan ALL messages (not just the last one) for any STREAMING
        // assistant row. This is critical because handleSend() adds the
        // USER message BEFORE calling addAssistantPlaceholder(), so the
        // last message is always USER and a naive "check last message"
        // guard would miss a stale STREAMING assistant from the previous
        // inference that was never flipped to COMPLETE (e.g. when
        // markLastComplete() was not called because the native side
        // didn't emit the "llama-timings" cue).
        for (int i = mMessages.size() - 1; i >= 0; i--) {
            ChatMessage msg = mMessages.get(i);
            if (msg.role == Role.ASSISTANT && msg.state == State.STREAMING) {
                KANTVLog.g(TAG, "addAssistantPlaceholder: GUARD forcing STREAMING row at idx="
                        + i + " into COMPLETE (markLastComplete was missed)");
                msg.state = State.COMPLETE;
                mStreamingHolder = null;
                mStreamingPosition = RecyclerView.NO_POSITION;
                notifyItemChanged(i);
                break;
            }
        }

        ChatMessage msg = new ChatMessage(Role.ASSISTANT);
        msg.text.append("🤔 Thinking...");
        mMessages.add(msg);
        int idx = mMessages.size() - 1;
        notifyItemInserted(idx);
        // The freshly-inserted row hasn't been bound yet, so the streaming
        // VH cache is invalid. AssistantViewHolder.bind() will repopulate
        // it as soon as RecyclerView hands it the new row.
        //
        // NOTE: there is a known footgun here - if the previous assistant
        // row was never flipped to COMPLETE (e.g. the native side skipped
        // the "llama-timings" cue, or the app was backgrounded mid-stream
        // and the next turn fires before markLastComplete was delivered),
        // it is still in STREAMING state. When RecyclerView later rebinds
        // that older row (e.g. on a layout pass, or on scroll), the
        // STREAMING branch in bind() used to unconditionally re-register
        // mStreamingHolder = this, which would point subsequent chunks
        // at the wrong row. The bind() guard added below
        // (only register when position == mMessages.size()-1) closes
        // that hole. We still log here so a fresh repro of the
        // "Q2 + A2 land in A1's bubble" symptom tells us the
        // incoming state of the cache.
        KANTVLog.g(TAG, "addAssistantPlaceholder: idx=" + idx
                + " size=" + mMessages.size()
                + " prevStreamingHolder=" + (mStreamingHolder != null ? "non-null" : "null")
                + " prevStreamingPos=" + mStreamingPosition);
        mStreamingHolder = null;
        mStreamingPosition = idx;
        return idx;
    }

    /**
     * Append a streaming chunk to the most recent assistant message.
     *
     * Render path (the trick the user actually sees):
     *   1. data-model first: always write the chunk to mMessages[last].text
     *      so any future re-bind gets the complete picture.
     *   2. immediate raw append: if the holder is alive, append the
     *      chunk straight to the live TextView so the user sees the
     *      token grow in real time. This is the "real-time growth" half of the
     *      contract. Without it the TextView only refreshed every
     *      FULL_RENDER_INTERVAL_MS (200ms) and the user perceived that
     *      as "no markdown until inference ends".
     *   3. periodic markwon re-render: every FULL_RENDER_INTERVAL_MS
     *      call markwon.setMarkdown() on the full accumulated text to
     *      convert any markdown markers that have completed since the
     *      last render (the most common case is a `**` that opened
     *      several chunks ago and is just now closed). This is the
     *      "markdown rendered mid-stream" half of the contract.
     *
     * The two paths cooperate: the immediate raw append guarantees the
     * user sees the text grow, and the periodic re-render guarantees
     * the markers get formatted. There is a small window (up to
     * FULL_RENDER_INTERVAL_MS) where the TextView shows "rendered
     * Spannable + raw chunk" side by side, but the next re-render
     * collapses it back to a single coherent Spannable.
     *
     * Falls back to notifyItemChanged() if the holder is gone (e.g.
     * the user scrolled the row off-screen and it was recycled, or the
     * ViewHolder hasn't been bound yet because the new row was just
     * inserted this frame).
     */
    public void appendToLast(String chunk) {
        if (chunk == null || chunk.isEmpty() || mMessages.isEmpty()) {
            return;
        }
        int lastIdx = mMessages.size() - 1;
        ChatMessage last = mMessages.get(lastIdx);
        if (last.role != Role.ASSISTANT) {
            // Out-of-order token; we silently drop. The fragment's contract
            // is to never call append outside an active inference, so this
            // would only happen if a leftover callback fires after a clear.
            KANTVLog.g(TAG, "appendToLast: last role is not ASSISTANT, drop chunk");
            return;
        }
        // The placeholder is "🤔 Thinking...", replace it on the first
        // real chunk so the user doesn't see the placeholder for too long.
        // This must run on the data buffer regardless of which render path
        // we take below, because the data buffer is the source of truth
        // for any future re-bind.
        if (last.text.toString().startsWith("🤔 Thinking")) {
            last.text.setLength(0);
        }
        last.text.append(chunk);

        // Direct-update path: the holder is alive, bound to the streaming
        // row, and its TextView is still attached. We (a) append the
        // chunk to the TextView's underlying Editable/Spannable right
        // now so the user sees the new characters instantly, then (b)
        // try the periodic markwon re-render which catches any
        // cross-chunk markdown that just completed.
        AssistantViewHolder vh = mStreamingHolder;
        if (vh != null
                && mStreamingPosition == mMessages.size() - 1
                && vh.textView != null
                && vh.textView.isAttachedToWindow()) {
            // Throttled happy-path log: only on the first chunk after
            // a (re)bind, so we can see "this is the chunk that landed
            // on the trailing-row holder" once per turn, not once per
            // token. mLastHappyLogPos != lastIdx is our cheap "we've
            // moved to a new turn" gate.
            if (vh.mLastHappyLogPos != lastIdx) {
                vh.mLastHappyLogPos = lastIdx;
                KANTVLog.g(TAG, "appendToLast: HAPPY path, lastIdx=" + lastIdx
                        + " cachedPos=" + mStreamingPosition
                        + " dataLen=" + last.text.length()
                        + " chunkLen=" + chunk.length());
            }
            vh.appendRawChunk(chunk);
            vh.maybePeriodicFullRender();
            return;
        }
        // Direct-update path was rejected. Most common reason: the new
        // trailing row was just inserted and its bind() hasn't run yet
        // (mStreamingHolder is null), OR the cached holder is for an
        // older position. Either way we fall through to notifyItemChanged
        // which forces a real rebind. Log the cause once per fallback so
        // a fresh repro of the "second Q+A land in the previous bubble"
        // symptom gives us data on the path actually being taken.
        KANTVLog.g(TAG, "appendToLast: fallback to notifyItemChanged, lastIdx=" + lastIdx
                + " cachedHolder=" + (vh != null ? "non-null" : "null")
                + " cachedPos=" + mStreamingPosition
                + " size=" + mMessages.size()
                + " cachedAttached=" + (vh != null && vh.textView != null
                        ? vh.textView.isAttachedToWindow() : false));
        // Fallback: the row was just inserted and not yet bound, or the
        // holder was recycled (scrolled off-screen). Either way, we have
        // to go through the normal notify -> bind path so the new row
        // materialises correctly. This path is the one that historically
        // caused the per-chunk flicker; it now fires only on cold-start
        // and on scroll, both rare.
        notifyItemChanged(mMessages.size() - 1);
    }

    /** Flip the most recent assistant message into COMPLETE state. */
    public void markLastComplete() {
        if (mMessages.isEmpty()) {
            return;
        }
        ChatMessage last = mMessages.get(mMessages.size() - 1);
        if (last.role != Role.ASSISTANT) {
            KANTVLog.g(TAG, "markLastComplete: last role is not ASSISTANT, no-op (size="
                    + mMessages.size() + ")");
            return;
        }
        KANTVLog.g(TAG, "markLastComplete: lastIdx=" + (mMessages.size() - 1)
                + " cachedHolder=" + (mStreamingHolder != null ? "non-null" : "null")
                + " cachedPos=" + mStreamingPosition);
        last.state = State.COMPLETE;
        // The streaming VH cache is now stale: COMPLETE re-binds the
        // row to do a full markwon.setMarkdown, and the old holder
        // reference must not be used to apply further streaming chunks.
        mStreamingHolder = null;
        mStreamingPosition = RecyclerView.NO_POSITION;
        notifyItemChanged(mMessages.size() - 1);
    }

    /** Flip the most recent assistant message into ERROR state. */
    public void markLastError() {
        if (mMessages.isEmpty()) {
            return;
        }
        ChatMessage last = mMessages.get(mMessages.size() - 1);
        if (last.role != Role.ASSISTANT) {
            KANTVLog.g(TAG, "markLastError: last role is not ASSISTANT, no-op (size="
                    + mMessages.size() + ")");
            return;
        }
        KANTVLog.g(TAG, "markLastError: lastIdx=" + (mMessages.size() - 1)
                + " cachedHolder=" + (mStreamingHolder != null ? "non-null" : "null")
                + " cachedPos=" + mStreamingPosition);
        last.state = State.ERROR;
        mStreamingHolder = null;
        mStreamingPosition = RecyclerView.NO_POSITION;
        notifyItemChanged(mMessages.size() - 1);
    }

    /** Wipe the whole conversation. */
    public void clear() {
        int n = mMessages.size();
        KANTVLog.g(TAG, "clear: removing " + n + " messages");
        if (n == 0) {
            return;
        }
        mMessages.clear();
        // After clear, no row is "the streaming row" - any further
        // appendToLast() call from a leftover native callback must fall
        // back to the data-layer check (it does, via mMessages.isEmpty()
        // and the role check at the top of appendToLast()).
        mStreamingHolder = null;
        mStreamingPosition = RecyclerView.NO_POSITION;
        notifyItemRangeRemoved(0, n);
    }

    // ---------------------------------------------------------------------
    // Multi-turn history packaging for native (KANTV_CHAT_V1 protocol)
    // ---------------------------------------------------------------------

    /**
     * Pack the conversation history into the KANTV_CHAT_V1 wire format
     * consumed by the C++ side (llm-inference.cpp). The resulting string
     * is passed to ggml_jni.llm_inference() as the `prompt` argument;
     * llama_inference_main detects the "KANTV_CHAT_V1\n" magic prefix,
     * parses the (role, content) pairs back into common_chat_msg, and
     * applies the model's own chat template (read from the gguf
     * metadata) via common_chat_templates_apply().
     *
     * Why not just pre-format the prompt on the Java side?
     *   The previous Java-side manual template approach (see
     *   getHistoryPrompt()) worked for Gemma-3 / Qwen2.5 but produced
     *   the "model echoes the entire prompt" regression on the small
     *   gemma-4-E2B-it-Q4_0 model. The model was in -no-cnv mode
     *   (raw text completion), so when it received a structured prompt
     *   like "&lt;start_of_turn&gt;user...&lt;end_of_turn&gt;\n&lt;start_of_turn&gt;model\n"
     *   it just continued the pattern instead of recognizing the
     *   "model should respond now" signal. Routing through the C++
     *   chat-template path puts the model into its native chat mode
     *   (params.conversation_mode = ENABLED), which fixes this.
     *
     * Wire format:
     *   "KANTV_CHAT_V1\n" + ({role}\x1E{content}\x1E)*
     *   \x1E is the ASCII Record Separator (0x1E), chosen because
     *   it is a control character that never appears in normal user
     *   input. Messages are encoded in order (oldest first), with
     *   each role and content as plain UTF-8.
     *
     * Empty placeholder messages (State.STREAMING with text.length()==0)
     * and ERROR-state messages are skipped. The caller is expected
     * to have added a fresh user message (the one being sent) and an
     * empty assistant placeholder before calling; both will be encoded
     * or skipped appropriately.
     *
     * Returns the packed string, or an empty string if the conversation
     * has no non-empty, non-error messages.
     */
    public String formatHistoryForNative() {
        StringBuilder sb = new StringBuilder(mMessages.size() * 64);
        sb.append("KANTV_CHAT_V1\n");

        KANTVLog.g("KANTV", "formatHistoryForNative: processing " + mMessages.size() + " messages");

        for (int idx = 0; idx < mMessages.size(); idx++) {
            ChatMessage msg = mMessages.get(idx);
            if (msg.state == ChatMessage.State.ERROR) {
                KANTVLog.g("KANTV", "formatHistoryForNative: skipping ERROR message at idx=" + idx);
                continue;
            }
            String text = msg.getText();
            if (text == null || text.isEmpty()) {
                // Skip the empty assistant placeholder
                KANTVLog.g("KANTV", "formatHistoryForNative: skipping empty message at idx=" + idx);
                continue;
            }
            String roleStr;
            switch (msg.role) {
                case USER:      roleStr = "user";      break;
                case ASSISTANT: roleStr = "assistant"; break;
                case SYSTEM:    roleStr = "system";    break;
                default:        continue;  // unknown role
            }

            // Debug: log message before sanitization
            String msgPreview = text.length() > 100 ? text.substring(0, 100) + "..." : text;
            KANTVLog.g("KANTV", "formatHistoryForNative [" + idx + "] " + roleStr + " (len=" + text.length() + "): " + msgPreview.replace("\n", "\\n"));

            // Sanitize assistant history of Gemma-2 <|channel> blocks so
            // the model does not see the private thinking process or the
            // channel markers from previous turns. For models that don't
            // emit such markers this is a no-op (the markers are not
            // present in the text).
            if (msg.role == Role.ASSISTANT) {
                text = sanitizeGemmaChannelBlocks(text);
                // Debug: log after sanitization
                String sanitizedPreview = text.length() > 100 ? text.substring(0, 100) + "..." : text;
                KANTVLog.g("KANTV", "formatHistoryForNative [" + idx + "] sanitized (len=" + text.length() + "): " + sanitizedPreview.replace("\n", "\\n"));
            }
            sb.append(roleStr);
            sb.append('\u001E');
            sb.append(text);
            sb.append('\u001E');
        }

        KANTVLog.g("KANTV", "formatHistoryForNative: final history length=" + sb.length());
        return sb.toString();
    }

    /**
     * Strip Gemma-2 / Gemma-4 thinking blocks from an assistant response.
     * Returns clean answer text only (thinking content is stripped).
     * This text is used as conversation history sent back to the LLM.
     *
     * Gemma format (raw, before Java-side filtering):
     *   <|channel>thought\n...thinking...<channel|>\n...answer...
     *   <|channel>final\n...answer...<channel|>
     *
     * After Java-side filtering, the text may look like:
     *   💭 **Thinking:** \n...thinking...\n\n---\n\n...answer...
     *
     * This function handles BOTH formats and returns ONLY the clean answer.
     * Thinking content is DISCARDED (not sent back to the model).
     *
     * ROBUSTNESS: After sanitization, if any <|channel prefix remains
     * in the result (indicating the filter missed some markers), the
     * raw-format stripper is applied recursively as a safety net.
     */
    static String sanitizeGemmaChannelBlocks(String text) {
        if (text == null || text.isEmpty()) {
            return "";
        }
        final String channelOpen    = "<|channel>";
        final String channelPrefix  = "<|channel";  // partial prefix for detection
        final String thoughtLiteral  = "thought";
        final String finalLiteral    = "final";
        final String channelClose    = "<channel|>";

        // Debug: log input
        String inputPreview = text.length() > 300 ? text.substring(0, 300) + "..." : text;
        KANTVLog.g("KANTV", "sanitizeGemmaChannelBlocks input (len=" + text.length() + "): " + inputPreview.replace("\n", "\\n"));

        // First, handle the display format (after Java-side filtering):
        // Strip "💭 **Thinking:** " prefix and "\n\n---\n\n" separator
        String displayPrefix = "💭 **Thinking:** ";
        String separator = "\n\n---\n\n";

        // Check if text contains our display format
        if (text.contains(separator)) {
            KANTVLog.g("KANTV", "sanitizeGemmaChannelBlocks: found separator, using display format path");
            // Use lastIndexOf to handle the edge case where the answer
            // content itself might contain the separator string
            int sepIdx = text.lastIndexOf(separator);
            String answerPart = text.substring(sepIdx + separator.length());

            // Trim leading newlines from answer
            if (answerPart.startsWith("\n")) answerPart = answerPart.substring(1);
            if (answerPart.startsWith("\r")) answerPart = answerPart.substring(1);

            String result = answerPart.trim();

            // VERIFICATION: Ensure no <|channel prefix remains in the result.
            // If the display-format sanitization missed some markers
            // (e.g., the answer was incomplete or format was unexpected),
            // fall back to the raw-format stripper.
            if (result.contains(channelPrefix)) {
                KANTVLog.g("KANTV", "sanitizeGemmaChannelBlocks: WARNING - channel prefix remains"
                        + " after display-format sanitization, applying raw stripper as fallback");
                result = stripRawChannelBlocks(result, channelOpen, thoughtLiteral,
                        finalLiteral, channelClose, channelPrefix);
            }

            String outputPreview = result.length() > 300 ? result.substring(0, 300) + "..." : result;
            KANTVLog.g("KANTV", "sanitizeGemmaChannelBlocks output (len=" + result.length() + "): " + outputPreview.replace("\n", "\\n"));
            return result;
        }

        // If text has display prefix but no separator (incomplete response),
        // strip the prefix and return what's left
        if (text.contains(displayPrefix)) {
            KANTVLog.g("KANTV", "sanitizeGemmaChannelBlocks: found display prefix but no separator");
            int prefixIdx = text.indexOf(displayPrefix);
            StringBuilder out = new StringBuilder();
            if (prefixIdx > 0) {
                out.append(text, 0, prefixIdx);
            }
            String result = out.toString().trim();

            // VERIFICATION: Check for channel markers in remaining text
            if (result.contains(channelPrefix)) {
                KANTVLog.g("KANTV", "sanitizeGemmaChannelBlocks: WARNING - channel prefix remains"
                        + " after prefix-only sanitization, applying raw stripper");
                result = stripRawChannelBlocks(result, channelOpen, thoughtLiteral,
                        finalLiteral, channelClose, channelPrefix);
            }

            KANTVLog.g("KANTV", "sanitizeGemmaChannelBlocks output (prefix only, len=" + result.length() + "): " + result.replace("\n", "\\n"));
            return result;
        }

        // Handle raw format (if any channel markers remain)
        KANTVLog.g("KANTV", "sanitizeGemmaChannelBlocks: using raw format path (no display format found)");
        String result = stripRawChannelBlocks(text, channelOpen, thoughtLiteral,
                finalLiteral, channelClose, channelPrefix);

        String outputPreview2 = result.length() > 300 ? result.substring(0, 300) + "..." : result;
        KANTVLog.g("KANTV", "sanitizeGemmaChannelBlocks output (raw, len=" + result.length() + "): " + outputPreview2.replace("\n", "\\n"));
        return result;
    }

    /**
     * Core raw-format stripper. Walks through the text, removes all
     * <|channel>thought blocks, preserves <|channel>final content,
     * and returns the clean result.
     *
     * Also recursively strips any remaining channel markers until
     * none remain, as a defense against nested or repeated markers.
     */
    private static String stripRawChannelBlocks(String text, String channelOpen,
                                                 String thoughtLiteral, String finalLiteral,
                                                 String channelClose, String channelPrefix) {
        StringBuilder out = new StringBuilder(text.length());
        int i = 0;
        while (i < text.length()) {
            int openIdx = text.indexOf(channelOpen, i);
            if (openIdx < 0) {
                out.append(text, i, text.length());
                break;
            }
            // Append text between current position and channel marker
            out.append(text, i, openIdx);

            // Check block type
            String afterOpen = text.substring(openIdx + channelOpen.length(),
                    Math.min(openIdx + channelOpen.length() + 20, text.length()));
            int closeIdx = text.indexOf(channelClose, openIdx + channelOpen.length());
            if (closeIdx < 0) {
                // Unclosed block - just append and break
                out.append(text, openIdx, text.length());
                break;
            }
            boolean isThought = afterOpen.startsWith(thoughtLiteral);
            boolean isFinal   = afterOpen.startsWith(finalLiteral);

            if (isFinal) {
                // Keep the content inside <|channel>final...<channel|>
                int afterFinalTag = openIdx + channelOpen.length() + finalLiteral.length();
                if (afterFinalTag < text.length() && text.charAt(afterFinalTag) == '\n') {
                    afterFinalTag++;
                } else if (afterFinalTag < text.length() && text.charAt(afterFinalTag) == '\r') {
                    afterFinalTag++;
                    if (afterFinalTag < text.length() && text.charAt(afterFinalTag) == '\n') afterFinalTag++;
                }
                out.append(text, afterFinalTag, closeIdx);
            }
            // For thought blocks: DISCARD entirely (do not include in history)

            i = closeIdx + channelClose.length();
            if (i < text.length() && text.charAt(i) == '\r') i++;
            if (i < text.length() && text.charAt(i) == '\n') i++;
        }
        String result = out.toString().trim();

        // RECURSIVE SAFETY: If channel markers still remain, apply the
        // stripper again. This handles the case where the first pass
        // left markers behind (e.g., nested or non-standard format).
        if (result.contains(channelPrefix)) {
            KANTVLog.g("KANTV", "sanitizeGemmaChannelBlocks: recursive pass needed,"
                    + " channel prefix still present after raw stripper (len=" + result.length() + ")");
            result = stripRawChannelBlocks(result, channelOpen, thoughtLiteral,
                    finalLiteral, channelClose, channelPrefix);
        }

        return result;
    }

    public int getMessageCount() {
        return mMessages.size();
    }

    /**
     * @return an unmodifiable view of the underlying message list. Used
     * by AIResearchFragment to fold the conversation into a single
     * chat-template-formatted prompt before each inference.
     */
    public List<ChatMessage> getMessages() {
        return java.util.Collections.unmodifiableList(mMessages);
    }

    // ---------------------------------------------------------------------
    // Chat-template prompt assembly
    // ---------------------------------------------------------------------

    /**
     * Build the full multi-turn prompt string for the given model, ready
     * to be passed to ggml_jni.llm_inference(). Iterates over the
     * conversation history ({@link #mMessages}) and emits:
     *   - BOS prefix if the model requests one (the model's
     *     {@link KANTVAIModel#getBos()} field)
     *   - one user/model turn per ChatMessage, formatted with the
     *     model's per-turn delimiters
     *   - a trailing generation-prompt suffix so the model knows to
     *     produce an assistant response (no leading whitespace; the
     *     model expects the suffix to start a fresh turn)
     *
     * The returned String is suitable as the `prompt` argument of
     * `ggmljava.llm_inference(model_path, prompt, ...)`. The native
     * side uses -no-cnv (raw text) so this manually-formatted prompt
     * is the only place the model sees the role markers.
     *
     * Stream-in-progress and error messages are included: STREAMING
     * messages are formatted using whatever text has accumulated so far
     * (so a user can continue a conversation even if the previous
     * assistant turn is still rendering); ERROR messages are skipped to
     * avoid confusing the model with "[error] ..." text.
     *
     * If {@code model} is null, falls back to the "User:/Assistant:"
     * plain-text format (the KANTVAIModel field defaults).
     */
    public String getHistoryPrompt(kantvai.ai.KANTVAIModel model) {
        if (mMessages.isEmpty()) {
            // No history at all - return an empty prompt with just the
            // generation suffix so the model still produces output. The
            // caller (AIResearchFragment) will normally guard against
            // empty input before calling, so this is a defensive path.
            return (model != null ? model.getGenerationPrompt() : "Assistant: ");
        }

        String userOpen   = (model != null) ? model.getUserOpen()   : "User: ";
        String userClose  = (model != null) ? model.getUserClose()  : "\n";
        String modelOpen  = (model != null) ? model.getModelOpen()  : "Assistant: ";
        String modelClose = (model != null) ? model.getModelClose() : "\n";
        String bos        = (model != null) ? model.getBos()        : "";
        String genPrompt  = (model != null) ? model.getGenerationPrompt() : "Assistant: ";

        StringBuilder sb = new StringBuilder(mMessages.size() * 64);
        if (bos != null && !bos.isEmpty()) {
            sb.append(bos);
        }

        for (ChatMessage msg : mMessages) {
            // Skip ERROR messages - their "[error] ..." text would
            // confuse the model on the next turn.
            if (msg.state == ChatMessage.State.ERROR) {
                continue;
            }
            // Skip empty messages: the assistant bubble is added as
            // a placeholder (State.STREAMING with text.length()==0)
            // right before the inference is fired, and including
            // it would produce "<start_of_turn>model\n<end_of_turn>\n"
            // in the prompt - an empty assistant turn that adds no
            // context and looks like a bug to the model. The
            // generationPrompt suffix at the end of the formatted
            // string already tells the model "respond now".
            String text = msg.getText();
            if (text == null || text.isEmpty()) {
                continue;
            }
            switch (msg.role) {
                case USER:
                    sb.append(userOpen);
                    sb.append(text);
                    sb.append(userClose);
                    break;
                case ASSISTANT:
                    sb.append(modelOpen);
                    sb.append(text);
                    sb.append(modelClose);
                    break;
                case SYSTEM:
                    // SYSTEM role: prepend the user-content but don't
                    // wrap in user/model delimiters. The model's
                    // expected position for system content is before
                    // the first user turn and the model may or may
                    // not have a dedicated system marker in its
                    // template. For now, emit as a leading "user"
                    // turn so the model at least sees the context -
                    // the proper system-prompt handling can be added
                    // later if a specific model needs it.
                    sb.append(userOpen);
                    sb.append(text);
                    sb.append(userClose);
                    break;
            }
        }

        sb.append(genPrompt);
        return sb.toString();
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
        KANTVLog.g(TAG, "onCreateViewHolder: viewType=" + viewType);
        if (viewType == TYPE_USER) {
            return new UserViewHolder(inflater.inflate(R.layout.item_chat_user, parent, false));
        }
        return new AssistantViewHolder(inflater.inflate(R.layout.item_chat_assistant, parent, false));
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
        ChatMessage msg = mMessages.get(position);
        KANTVLog.g(TAG, "onBindViewHolder: position=" + position
                + " role=" + msg.role + " state=" + msg.state
                + " textLen=" + msg.getText().length()
                + " vhClass=" + holder.getClass().getSimpleName());
        if (holder instanceof UserViewHolder) {
            ((UserViewHolder) holder).bind(msg);
        } else if (holder instanceof AssistantViewHolder) {
            // Pass the position so the holder can register itself as the
            // "live streaming holder" with the adapter when the row is
            // the trailing STREAMING row. Without this, appendToLast()
            // would have to fall back to notifyItemChanged() on every
            // chunk (and that's what causes the per-chunk flicker).
            ((AssistantViewHolder) holder).bind(msg, position);
        }
    }

    @Override
    public void onViewRecycled(@NonNull RecyclerView.ViewHolder holder) {
        KANTVLog.g(TAG, "onViewRecycled: vhClass=" + holder.getClass().getSimpleName()
                + " position=" + holder.getAdapterPosition());
        super.onViewRecycled(holder);
        // AssistantViewHolder owns the cleanup of mStreamingHolder
        // because it knows its own identity. Doing the null-out here
        // in the adapter would force a downcast and is more error-prone.
        if (holder instanceof AssistantViewHolder) {
            ((AssistantViewHolder) holder).onAdapterRecycled();
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
                        KANTVLog.g(TAG, "failed to load attachment: " + e.toString());
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

    // Top-level debug switch. 0 = all debug code is compiled out
    // (dump skipped, header-stripping off). 1 = write streaming
    // snapshots to /sdcard/Android/data/.../chat_dumps/ AND strip
    // `## ...` markdown headers so they don't render as huge text
    // in the chat bubble. The two are intentionally tied together:
    // when something looks wrong in the chat, flip this to 1, pull
    // the dump, debug, flip back to 0.
    private static final int debug = 0;

    // Strip leading "#" / "##" / "###" markers so the LLM's
    // markdown headings (which Markwon would render as 1.5-2x sized
    // bold, breaking the chat bubble layout) collapse to plain text.
    // Only applied to the text that goes to Markwon; the original
    // msg.getText() in the dump is untouched. The regex is bounded
    // to line start + 1-6 hashes + spaces so it does NOT eat '#'
    // chars in the middle of a sentence (e.g. "issue #123" or
    // "C# language"). Lives on the outer class because non-static
    // inner classes can't have static non-constant fields.
    private static final java.util.regex.Pattern HEADER_STRIP =
            java.util.regex.Pattern.compile("(?m)^#{1,6}\\s+");

    /**
     * Heuristic: auto-close an unclosed {@code **} bold marker in a
     * paragraph.
     *
     * <p>The LLM streams markdown tokens 5-20 chars at a time, and a
     * {@code **xxx**} bold phrase is often split as {@code **} / {@code xxx}
     * / {@code **} across three separate chunks. The periodic full re-render
     * in maybePeriodicFullRender() catches this once the closing
     * {@code **} arrives, but a much more common glitch is the LLM
     * EMITTING an unclosed {@code **} in the first place &mdash; the most
     * frequent example we see is the LLM writing a section header like
     * {@code **xxx:} (one opening {@code **}, no closing on the line).
     * commonmark-markwon follows the spec and leaves unclosed {@code **}
     * runs as literal text, so the user sees raw asterisks in the bubble.
     *
     * <p>This preprocessor scans the text paragraph by paragraph. A
     * paragraph is bounded by blank lines ({@code \n\n}). For every
     * paragraph with an <b>odd</b> count of {@code **} (i.e. at
     * least one unclosed opening marker), it walks the paragraph
     * line by line and, for any line that has exactly one
     * {@code **} with non-whitespace content after it up to the
     * newline, appends a closing {@code **} at the end of the
     * line. That turns {@code **xxx:} into {@code **xxx:**},
     * which markwon then renders as a bold run.
     *
     * <p><b>Why paragraph-level first, not just line-level:</b>
     * commonmark allows emphasis to span lines within a single
     * paragraph (e.g. {@code **xxx\nyyy**} is valid and renders
     * as a single bold run across the newline). A pure line-level
     * pass would misread {@code **xxx} (1 {@code **}) and
     * {@code yyy**} (1 {@code **}) as TWO unclosed openers and
     * auto-close each, producing {@code **xxx**\nyyy**} which
     * markwon parses as one closed bold + one unclosed. The
     * paragraph check (count=2 in the paragraph, even, leave it
     * alone) preserves the cross-line bold.
     *
     * <p>Conservative rules (intentionally NOT aggressive):
     * <ul>
     *   <li>Skip paragraphs with 0 or 2+ {@code **}: don't
     *       second-guess normal balanced or nested patterns the
     *       LLM might have produced correctly.</li>
     *   <li>Skip lines where the lone {@code **} is at end-of-line:
     *       e.g. {@code "xxx**"} could be the opening of a multi-line
     *       emphasis; leave it alone.</li>
     *   <li>Skip lines where the lone {@code **} is followed by
     *       only whitespace: same reason &mdash; could be trailing
     *       punctuation that the LLM hasn't finished yet.</li>
     * </ul>
     *
     * <p>This preprocessor runs only on the text that goes to markwon;
     * the original {@code msg.getText()} in the data model is
     * untouched, so the raw token stream is still preserved if
     * anything goes wrong downstream.
     */
    private static String preprocessUnclosedBold(String text) {
        if (text == null || text.isEmpty()) {
            return text;
        }
        // Paragraph-level: only paragraphs with ODD `**` count
        // are candidates for repair. Even-count paragraphs are
        // already balanced (or follow a pattern we trust the
        // parser to handle, e.g. cross-line `**xxx\nyyy**`).
        String[] paragraphs = text.split("\n\n", -1);
        StringBuilder sb = new StringBuilder(text.length() + 16);
        for (int p = 0; p < paragraphs.length; p++) {
            String para = paragraphs[p];
            int paraCount = 0;
            int idx = 0;
            while ((idx = para.indexOf("**", idx)) != -1) {
                paraCount++;
                idx += 2;
            }
            if (paraCount % 2 == 0) {
                // Even count - leave the paragraph alone. This is
                // what protects cross-line bold like
                // `**xxx\nyyy**` (2 `**` in one paragraph).
                sb.append(para);
            } else {
                // Odd count - there's at least one unclosed opener.
                // Walk the paragraph line by line and auto-close
                // any line that has a lone opening `**` with
                // content after it.
                sb.append(autoCloseInParagraph(para));
            }
            if (p < paragraphs.length - 1) {
                sb.append("\n\n");
            }
        }
        return sb.toString();
    }

    /**
     * Per-paragraph helper: walk a single paragraph line by line
     * and append a closing {@code **} to any line that has exactly
     * one {@code **} with non-whitespace content after it.
     */
    private static String autoCloseInParagraph(String para) {
        String[] lines = para.split("\n", -1);
        StringBuilder sb = new StringBuilder(para.length() + 16);
        for (int i = 0; i < lines.length; i++) {
            String line = lines[i];
            int lineCount = 0;
            int idx = 0;
            while ((idx = line.indexOf("**", idx)) != -1) {
                lineCount++;
                idx += 2;
            }
            if (lineCount == 1) {
                int pos = line.indexOf("**");
                String after = line.substring(pos + 2);
                if (!after.trim().isEmpty()) {
                    sb.append(line);
                    sb.append("**");
                    if (i < lines.length - 1) {
                        sb.append('\n');
                    }
                    continue;
                }
            }
            sb.append(line);
            if (i < lines.length - 1) {
                sb.append('\n');
            }
        }
        return sb.toString();
    }

    class AssistantViewHolder extends RecyclerView.ViewHolder {
        final TextView textView;
        final TextView timeView;

        // Debug dump state. When debug == 1, every bind() writes a
        // snapshot of the rendered text + span list to
        // DUMP_DIR/state.txt and a PNG of the TextView's current
        // pixels to DUMP_DIR/frame_*.png, throttled to one sample
        // per DUMP_INTERVAL_MS. The user pulls these files off-device
        // to diagnose transient streaming render bugs (e.g. headers
        // being rendered at the wrong size during mid-stream
        // rebinds). Old files can be cleared with
        //   adb shell rm -rf <DUMP_DIR>
        // from a host shell.
        private static final long   DUMP_INTERVAL_MS = 200L; // 5 fps
        // DUMP_DIR lives under the app's external files dir so we don't
        // need WRITE_EXTERNAL_STORAGE on Android 10+. Pull with:
        //   adb pull /sdcard/Android/data/com.kantvai.kantvplayer/files/chat_dumps
        private static final String DUMP_DIR_NAME   = "chat_dumps";
        private final File mDumpDir;
        private long mLastDumpTime = 0L;
        private int  mDumpFrameSeq = 0;

        // Strip leading "#" / "##" / "###" markers so the LLM's
        // markdown headings (which Markwon would render as 1.5-2x
        // sized bold, breaking the chat bubble layout) collapse to
        // plain text. Only applied to the text that goes to Markwon;
        // the original msg.getText() in the dump is untouched. The
        // regex is bounded to line start + 1-6 hashes + spaces so it
        // does NOT eat '#' chars in the middle of a sentence (e.g.
        // "issue #123" or "C# language").
        // (debug dump state below. HEADER_STRIP itself lives on the
        // outer ChatAdapter class because Java doesn't allow `static`
        // non-constant fields in non-static inner classes.)

        // Length of text last rendered to this TextView. We need to know
        // this across binds so the streaming path can append only the
        // chars that arrived since the last bind() (cheaper than
        // setText() which triggers a full re-layout) and so the
        // COMPLETE path knows to start from a clean slate.
        private int mLastRenderedLength = 0;

        // Diagnostic: tracks which "lastIdx" we last printed a happy-path
        // appendToLast log for. Used by appendToLast() to print one log
        // line per turn ("the first chunk of the new turn landed on
        // the trailing-row holder") instead of one per token. Reset in
        // onAdapterRecycled() so a recycled holder for a new turn logs
        // again.
        private int mLastHappyLogPos = -1;

        // Periodic full re-render gate. The maybePeriodicFullRender()
        // method only catches markdown that is SELF-CONTAINED in the
        // data-model text at re-render time (e.g., "**bold**" that
        // has both `**` markers present, or "`code`" that closed).
        // When the LLM tokenizes mid-pattern - "**" in one chunk,
        // "bold" in the next, "**" in another - none of those
        // individual chunks parses to a complete span. The fix is to
        // re-render the full accumulated data-model text every
        // FULL_RENDER_INTERVAL_MS so the user sees markdown formatting
        // within 200ms of the closing marker arriving, instead of
        // waiting for the COMPLETE transition.
        //
        // To close that gap we re-render the FULL accumulated text
        // with markwon.setMarkdown() every FULL_RENDER_INTERVAL_MS
        // milliseconds. 5Hz is fast enough to feel real-time but
        // not so fast that the periodic layout pass is perceived
        // as flicker. Crucially, the re-render goes straight to
        // textView.setText() - it does NOT go through
        // notifyItemChanged, so the per-chunk rebind that caused
        // the original flicker does NOT come back.
        //
        // For very long texts (> FULL_RENDER_MAX_CHARS) we skip the
        // periodic re-render to avoid UI-thread jank; the incremental
        // path still catches self-contained markdown, and the final
        // COMPLETE full render covers the cross-chunk patterns.
        private static final long FULL_RENDER_INTERVAL_MS = 200L;
        private static final int  FULL_RENDER_MAX_CHARS    = 20000;
        private long mLastFullRenderTime = 0L;

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

        /**
         * Three render paths, plus a streaming-holder registration:
         *
         * STREAMING -> plain text recovery path. We use
         *   setText(msg.getText()) to seed the TextView with whatever
         *   the data model already has, then subsequent chunks arrive
         *   through appendRawChunk() (immediate raw append to the
         *   TextView's buffer) + maybePeriodicFullRender() (every
         *   FULL_RENDER_INTERVAL_MS a markwon re-render of the full
         *   data-model text). The two together give the user both
         *   real-time text growth AND mid-stream markdown formatting
         *   without a per-chunk rebind (the rebind was the flicker).
         *   When bind() runs for a STREAMING row we also register
         *   ourselves as the adapter's "live streaming holder" so
         *   subsequent appendToLast() calls can find us without
         *   going through notifyItemChanged().
         *
         * COMPLETE -> full Markwon render exactly once. This is where
         *   **bold**, list bullets, links, etc. become formatted
         *   *correctly* (including any cross-chunk `**` pairs that
         *   the periodic re-render may have missed because the
         *   closing `**` landed in the same chunk that triggered
         *   the COMPLETE transition). The TextView's text is now a
         *   SpannableString from Markwon.
         *
         * ERROR -> plain text with the "⚠ " prefix. The warning
         *   emoji would otherwise be parsed by Markwon as a list
         *   marker.
         *
         * @param position the adapter position this row is being bound
         *   to. Required so we can register as the streaming holder.
         */
        void bind(ChatMessage msg, int position) {
            KANTVLog.g(TAG, "AssistantViewHolder.bind: position=" + position
                    + " state=" + msg.state
                    + " textLen=" + msg.getText().length()
                    + " size=" + mMessages.size()
                    + " cachedHolder=" + (mStreamingHolder != null ? "non-null" : "null")
                    + " cachedPos=" + mStreamingPosition);
            if (msg.state == State.STREAMING) {
                // Always do a full setText() in bind(). We used to try to
                // be clever with "append if currentLen > 0 && fullLen >
                // mLastRenderedLength", but that breaks when this holder
                // gets recycled for a NEW message whose data is shorter
                // than the previous one (e.g. placeholder "..." bound to
                // a holder whose TextView still shows the last COMPLETE
                // response's rendered text). The "old text + new text"
                // concatenation that resulted made the second inference's
                // output appear inside the first one's bubble.
                //
                // The streaming-update path is appendRawChunk() +
                // maybePeriodicFullRender() (called from appendToLast
                // when mStreamingHolder is alive). bind() is only the
                // recovery path for "holder was recycled and rebound" -
                // and for that case a clean setText is the correct
                // thing to do.
                //
                // Note: we deliberately do NOT call markwon.setMarkdown
                // here even though it's available. Reason: this bind
                // path fires on the first chunk (when the row was just
                // inserted and not yet stamped by maybePeriodicFullRender),
                // and we want the user's very first visible text to be
                // the raw LLM token stream with no markwon re-parse
                // hitch. The first real chunk's markwon formatting is
                // applied by maybePeriodicFullRender() shortly after,
                // and the full re-render happens on COMPLETE.
                textView.setText(msg.getText());
                mLastRenderedLength = msg.getText().length();
                // Force a periodic full re-render on the first chunk
                // after bind. Without this reset, a recycled holder
                // inheriting a recent mLastFullRenderTime would delay
                // the first cross-chunk catch-up for up to 200ms.
                mLastFullRenderTime = 0L;
                // SAFETY GATE: only register as the "live" streaming
                // holder if this row is actually the trailing row.
                //
                // Background: if the previous assistant row was never
                // flipped to COMPLETE (e.g. markLastComplete() missed
                // because the native side skipped the "llama-timings"
                // detection, or the user fired a second turn before
                // the first finished), that older row is still in
                // STREAMING state. When RecyclerView later rebinds it
                // (on a layout pass, on scroll, on any notify pass),
                // the STREAMING branch USED to unconditionally
                // overwrite mStreamingHolder = this with the OLDER
                // holder and mStreamingPosition = the OLDER position.
                // Subsequent appendToLast() calls then passed the
                // direct-update path with the older holder, and the
                // raw chunks got appended onto the older TextView's
                // buffer - producing the "Q2 and A2 land at the end
                // of A1" symptom the user reported.
                //
                // The fix is to refuse to claim the streaming-holder
                // slot when our position is not the trailing row. The
                // older row still renders correctly (textView.setText
                // above replaces its content with the data-model
                // text), but it will not be the destination for new
                // chunks - those will route through the trailing
                // row's holder once it binds, or through the
                // notifyItemChanged fallback in appendToLast.
                if (position == mMessages.size() - 1) {
                    mStreamingHolder = this;
                    mStreamingPosition = position;
                } else {
                    KANTVLog.g(TAG, "bind: STREAMING row at position " + position
                            + " is NOT the trailing row (size=" + mMessages.size()
                            + "); refusing to register as active streaming holder");
                    if (mStreamingHolder == this) {
                        mStreamingHolder = null;
                        mStreamingPosition = RecyclerView.NO_POSITION;
                    }
                }
            } else if (msg.state == State.COMPLETE && mMarkwon != null) {
                // Two preprocessors run on the COMPLETE text:
                //   1. HEADER_STRIP (only when debug==1) drops
                //      leading '#' headers so the chat bubble
                //      doesn't end up with 1.5-2x sized text.
                //   2. preprocessUnclosedBold auto-closes lone
                //      `**` on a line so the LLM's
                //      "**Summary:" pattern actually renders bold
                //      instead of leaking the asterisks.
                // Both run on the text that goes to markwon; the
                // data model is untouched.
                String textForMarkwon = msg.getText();
                if (debug == 1) {
                    textForMarkwon = HEADER_STRIP.matcher(textForMarkwon).replaceAll("");
                }
                textForMarkwon = preprocessUnclosedBold(textForMarkwon);
                mMarkwon.setMarkdown(textView, textForMarkwon);
                mLastRenderedLength = 0;
                // COMPLETE flips out of streaming: clear the holder
                // cache so no further appendToLast() tries to stamp
                // this row directly. (markLastComplete() already
                // nulled these out, but a recycle-and-rebind at the
                // same position could otherwise leave them stale.)
                if (mStreamingHolder == this) {
                    mStreamingHolder = null;
                    mStreamingPosition = RecyclerView.NO_POSITION;
                }
            } else if (msg.state == State.ERROR) {
                textView.setText("⚠ " + msg.getText());
                mLastRenderedLength = 0;
                if (mStreamingHolder == this) {
                    mStreamingHolder = null;
                    mStreamingPosition = RecyclerView.NO_POSITION;
                }
            } else {
                // No markwon supplied - fall back to plain text
                // so the bubble still shows something.
                textView.setText(msg.getText());
                mLastRenderedLength = 0;
                if (mStreamingHolder == this) {
                    mStreamingHolder = null;
                    mStreamingPosition = RecyclerView.NO_POSITION;
                }
            }
            String caret = (msg.state == State.STREAMING) ? " ▌" : "";
            timeView.setText(formatTime(msg.timestamp) + caret);
            if (debug == 1) {
                maybeDumpStreamingState(msg);
            }
        }

        /**
         * Append the raw chunk to the TextView's underlying buffer so
         * the user sees the new characters appear in real time. NO
         * markwon work happens here - this is the "fast path" that
         * guarantees the bubble grows chunk-by-chunk even between
         * markwon re-renders.
         *
         * Why this is needed at all: before this method existed, the
         * only path that updated the TextView was the periodic
         * markwon re-render in maybePeriodicFullRender(), which fires
         * at most every FULL_RENDER_INTERVAL_MS (200ms). That meant
         * during a 200ms window the TextView simply did not change,
         * and the user perceived that as "no markdown until inference
         * ends" (they'd see the empty / placeholder bubble, then a
         * 200ms pause, then the formatted result). The instant
         * editable.append() here is what makes the bubble actually
         * grow on every chunk arrival.
         *
         * Three cases for the TextView's current buffer:
         *   - SpannableStringBuilder: rare in practice, but if the
         *     bind() path happened to land on one we can append in
         *     place and keep all existing spans.
         *   - Spannable (typically a SpannableString from
         *     markwon.setMarkdown): the rendered text is here with
         *     the markdown markers already stripped and replaced by
         *     spans. We can't append onto a SpannableString in place
         *     (its internal char array is fixed), so we copy it into
         *     a SpannableStringBuilder, append the raw chunk, and
         *     setText() it back. The existing spans survive the copy
         *     (SSB's append(Spannable) preserves them).
         *   - Plain CharSequence (initial bind state): just call
         *     textView.append() which goes through the regular
         *     Editable path.
         *
         * The "rendered Spannable + raw chunk" mixed state that
         * results from case 2 is intentional and transient. The
         * next maybePeriodicFullRender() call (within
         * FULL_RENDER_INTERVAL_MS) will markwon.setMarkdown() the
         * full data-model text and the mixed state collapses to a
         * single coherent rendered Spannable.
         */
        void appendRawChunk(String chunk) {
            if (chunk == null || chunk.isEmpty()) {
                return;
            }
            CharSequence cs = textView.getText();
            if (cs instanceof SpannableStringBuilder) {
                // In-place append. Existing spans (from markwon) are
                // preserved by the SSB's append(CharSequence) overload.
                ((SpannableStringBuilder) cs).append(chunk);
            } else if (cs instanceof Spannable) {
                // markwon.setMarkdown() produced a SpannableString. We
                // must copy to a mutable SSB before appending the raw
                // chunk, otherwise the new chars land outside any
                // span (which is what we want, they're raw text) but
                // we'd also lose the ability to extend existing
                // spans if the chunk happens to land inside a
                // formatted region.
                SpannableStringBuilder ssb = new SpannableStringBuilder(cs);
                ssb.append(chunk);
                textView.setText(ssb);
            } else {
                // First time through: the TextView still has the
                // initial setText() from bind(). Just append.
                textView.append(chunk);
            }
        }

        /**
         * Trigger a periodic full markwon re-render of the trailing
         * assistant row. Called by ChatAdapter.appendToLast() on every
         * chunk arrival, but only does work when at least
         * FULL_RENDER_INTERVAL_MS (200ms, 5Hz) has passed since the
         * last re-render.
         *
         * The re-render source is the DATA MODEL
         * (mMessages[mStreamingPosition].getText()), NOT the
         * TextView's current buffer. The data model is the only
         * place that still holds the original LLM output with the
         * markdown markers (** etc.) intact - the TextView's buffer
         * becomes the *rendered* Spannable after the first
         * markwon.setMarkdown() call, and reparsing that stripped
         * rendered text would find no markdown syntax to apply.
         *
         * This is the per-chunk "no-flicker" entry point: the
         * re-render goes straight to textView.setText(Spannable)
         * and does NOT go through notifyItemChanged, so the
         * per-chunk rebind that caused the original flicker does
         * not come back.
         *
         * The bound mLastFullRenderTime gate keeps the cost at
         * one markwon.setMarkdown per FULL_RENDER_INTERVAL_MS
         * regardless of how many chunks arrive in that window.
         * Bounds checks guard against stale mStreamingPosition
         * during the brief window between addAssistantPlaceholder
         * and the placeholder's first bind().
         */
        void maybePeriodicFullRender() {
            if (mMarkwon == null) {
                return;
            }
            long now = System.currentTimeMillis();
            if (now - mLastFullRenderTime <= FULL_RENDER_INTERVAL_MS) {
                return;
            }
            // Bounds check: mStreamingPosition may be stale (e.g. the
            // user just sent a new prompt and the placeholder row is
            // still being inserted). Don't crash on an OOB index.
            if (mStreamingPosition < 0
                    || mStreamingPosition >= mMessages.size()) {
                return;
            }
            ChatMessage msg = mMessages.get(mStreamingPosition);
            if (msg == null || msg.role != Role.ASSISTANT) {
                return;
            }
            String fullText = msg.getText();
            // Skip the re-render for very long texts (>20K chars) to
            // avoid UI-thread jank; the COMPLETE bind will do the
            // final full render and the user still sees the raw text
            // accumulate in the meantime (thanks to appendRawChunk).
            if (fullText.length() >= FULL_RENDER_MAX_CHARS) {
                return;
            }
            // Auto-close lone `**` on a line so the LLM's
            // "**Summary:" / similar unclosed-bold patterns actually
            // render as bold mid-stream instead of leaking the
            // asterisks. The data model is unchanged - this only
            // touches the text we hand to markwon. The cost is one
            // pass over the text per 200ms window, which is small
            // relative to markwon's own parse+render cost.
            String textForMarkwon = preprocessUnclosedBold(fullText);
            mMarkwon.setMarkdown(textView, textForMarkwon);
            mLastFullRenderTime = now;
        }

        // (applyIncrementalMarkwonSpans was removed: the incremental
        // path can't survive markwon.setMarkdown() replacing the
        // Editable with marker-stripped rendered text. The
        // appendRawChunk + periodic full re-render pair above is
        // the only correct combination: immediate raw append for
        // real-time growth, periodic re-render for cross-chunk
        // markdown. See the comment in appendRawChunk for the
        // full reasoning.)

        /**
         * Called by ChatAdapter.onViewRecycled() when this holder is
         * about to be reused for a different position (or for a
         * different message type). If we were the live streaming
         * holder, clear the adapter's reference so a future
         * appendToLast() doesn't accidentally stamp the wrong row.
         */
        void onAdapterRecycled() {
            KANTVLog.g(TAG, "onAdapterRecycled: wasStreamingHolder="
                    + (mStreamingHolder == this)
                    + " cachedPos=" + mStreamingPosition);
            if (mStreamingHolder == this) {
                mStreamingHolder = null;
                mStreamingPosition = RecyclerView.NO_POSITION;
            }
            // Reset per-holder streaming state. After recycling, this
            // VH will go through bind() again with a fresh message,
            // and the existing mLastRenderedLength / Editable state
            // must not bleed across messages.
            mLastRenderedLength = 0;
            mLastHappyLogPos = -1;
            // Force the next chunk that lands on this (rebound) holder
            // to go through the full setText() / setMarkdown() path
            // instead of the raw-append fast path. Without this reset,
            // a recycled holder that was previously in STREAMING
            // state (mLastFullRenderTime recently bumped) would let
            // the first chunk of the new turn take the "raw append
            // onto existing TextView contents" branch in
            // appendRawChunk, which on a holder whose TextView still
            // shows the previous response's rendered text produces
            // the "second question and answer land in the previous
            // bubble" symptom reported by users.
            mLastFullRenderTime = 0L;
            // Clear the TextView's editable buffer. bind() will
            // overwrite this with the new message's text on the
            // rebound; clearing here just makes the recycled state
            // honest so a stray appendToLast() between recycling
            // and rebinding can't stitch new chars onto the old
            // Spannable.
            if (textView != null) {
                textView.setText("");
            }
        }

        // -----------------------------------------------------------------
        // Debug dump helpers
        // -----------------------------------------------------------------

        private void maybeDumpStreamingState(ChatMessage msg) {
            if (mDumpDir == null) {
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

                // Spans live on the TextView's CharSequence, not on
                // the ChatMessage's buffer. We accept any Spannable
                // (Editable / SpannableString / SpannableStringBuilder)
                // since Markwon hands us back a SpannableString after
                // setMarkdown(). Dump whatever it has applied.
                CharSequence cs = textView.getText();
                if (cs instanceof Spannable) {
                    Spannable sp = (Spannable) cs;
                    int len = sp.length();

                    StyleSpan[] styles = sp.getSpans(0, len, StyleSpan.class);
                    fw.write("StyleSpan count=" + styles.length + "\n");
                    for (StyleSpan s : styles) {
                        int st = sp.getSpanStart(s);
                        int en = sp.getSpanEnd(s);
                        fw.write("  style=" + styleName(s.getStyle())
                                + " [" + st + "," + en + ")"
                                + " text=\"" + safeSlice(text, st, en) + "\"\n");
                    }

                    RelativeSizeSpan[] sizes = sp.getSpans(0, len, RelativeSizeSpan.class);
                    fw.write("RelativeSizeSpan count=" + sizes.length + "\n");
                    for (RelativeSizeSpan s : sizes) {
                        int st = sp.getSpanStart(s);
                        int en = sp.getSpanEnd(s);
                        fw.write("  scale=" + s.getSizeChange()
                                + " [" + st + "," + en + ")"
                                + " text=\"" + safeSlice(text, st, en) + "\"\n");
                    }

                    TypefaceSpan[] faces = sp.getSpans(0, len, TypefaceSpan.class);
                    fw.write("TypefaceSpan count=" + faces.length + "\n");
                    for (TypefaceSpan s : faces) {
                        int st = sp.getSpanStart(s);
                        int en = sp.getSpanEnd(s);
                        fw.write("  family=\"" + s.getFamily() + "\""
                                + " [" + st + "," + en + ")"
                                + " text=\"" + safeSlice(text, st, en) + "\"\n");
                    }

                    BackgroundColorSpan[] bgs = sp.getSpans(0, len, BackgroundColorSpan.class);
                    fw.write("BackgroundColorSpan count=" + bgs.length + "\n");
                    for (BackgroundColorSpan s : bgs) {
                        int st = sp.getSpanStart(s);
                        int en = sp.getSpanEnd(s);
                        fw.write("  color=" + Integer.toHexString(s.getBackgroundColor())
                                + " [" + st + "," + en + ")"
                                + " text=\"" + safeSlice(text, st, en) + "\"\n");
                    }
                } else {
                    fw.write("(textView.getText() is not Spannable: " + cs.getClass().getName() + ")\n");
                }
                fw.write("\n");
            } catch (Throwable t) {
                KANTVLog.g(TAG, "dumpTextAndSpans failed: " + t);
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
                KANTVLog.g(TAG, "dumpBitmap failed: " + t);
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

    /**
     * Classify a span produced by Markwon's incremental parse as
     * "inline" (safe to copy onto the live Editable when a fresh
     * streaming chunk arrives) or "paragraph" (must be skipped,
     * because its layout depends on surrounding context that the
     * chunk doesn't carry).
     *
     * Inline spans cover 90% of what an LLM emits inline -
     * **bold**, *italic*, `code`, [link] - and the per-character
     * offset of each is self-contained. Paragraph spans
     * (HeadingSpan, BulletListItemSpan, CodeBlockSpan, etc.) use
     * LeadingMarginSpan / LineHeightSpan whose geometry is computed
     * relative to the whole document, so naively copying them onto
     * a single chunk's range would draw at the wrong indent.
     * The COMPLETE full re-render takes care of those.
     *
     * Implemented as a class-name match rather than instanceof to
     * avoid importing every concrete span class. The downside is
     * that a *new* Markwon span class with a non-matching name
     * would be skipped by default - that's the safe default, since
     * the worst case is "no inline formatting on this chunk" rather
     * than "geometry corruption on the whole bubble".
     */
    private static boolean isInlineSpan(Object span) {
        if (span == null) {
            return false;
        }
        String cn = span.getClass().getName();
        // Paragraph-level spans - skip.
        if (cn.contains("ListItem")
                || cn.contains("CodeBlock")
                || cn.contains("BlockQuote")
                || cn.contains("Heading")
                || cn.contains("TableSpan")
                || cn.contains("ThematicBreak")
                || cn.endsWith(".TextLayoutSpan")
                || cn.endsWith(".TextViewSpan")
                || cn.endsWith(".LastLineSpacingSpan")) {
            return false;
        }
        // Inline spans - copy.
        return cn.endsWith(".StyleSpan")
                || cn.endsWith(".TypefaceSpan")
                || cn.endsWith(".BackgroundColorSpan")
                || cn.endsWith(".ForegroundColorSpan")
                || cn.endsWith(".URLSpan")
                || cn.endsWith(".StrongEmphasisSpan")
                || cn.endsWith(".EmphasisSpan")
                || cn.endsWith(".CodeSpan")
                || cn.endsWith(".CustomTypefaceSpan")
                || cn.endsWith(".StrikethroughSpan");
    }

    private String formatTime(long ts) {
        return mTimeFormat.format(new Date(ts));
    }
}
