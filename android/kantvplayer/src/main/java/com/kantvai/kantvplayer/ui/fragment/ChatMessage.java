/*
 * Copyright (c) 2024- KanTV Authors
 *
 * Single chat turn in the AI research screen. A turn belongs to either the
 * user (input box, optional image/audio attachment) or the assistant (model
 * output, may stream in chunks before reaching a final state).
 *
 * Why a separate class instead of String[]: the chat UI needs to:
 *   - distinguish roles for left/right bubble alignment
 *   - stream assistant output chunk-by-chunk without re-creating the row
 *   - show a "▌" caret while streaming
 *   - optionally render a thumbnail in the user bubble when an image is
 *     attached
 */
package com.kantvai.kantvplayer.ui.fragment;

public class ChatMessage {
    public enum Role {
        USER,
        ASSISTANT,
        SYSTEM
    }

    public enum State {
        /** Assistant message, native side is still emitting tokens. */
        STREAMING,
        /** Terminal state, the message is fully rendered. */
        COMPLETE,
        /** Inference failed, render an error tag instead of normal text. */
        ERROR
    }

    public enum AttachmentType {
        NONE,
        IMAGE,
        AUDIO
    }

    public final Role role;
    public State state;
    public final StringBuilder text;
    public final long timestamp;
    public String attachmentPath;        // null when no attachment
    public AttachmentType attachmentType;

    public ChatMessage(Role role) {
        this.role = role;
        this.text = new StringBuilder();
        this.timestamp = System.currentTimeMillis();
        this.attachmentPath = null;
        this.attachmentType = AttachmentType.NONE;
        // User messages are complete the moment they are typed. Assistant
        // messages start streaming and are flipped to COMPLETE / ERROR when
        // the native callback signals the end of inference.
        this.state = (role == Role.ASSISTANT) ? State.STREAMING : State.COMPLETE;
    }

    /** Append a streaming chunk from the native side. Cheap, no notify. */
    public void append(String chunk) {
        if (chunk == null) {
            return;
        }
        text.append(chunk);
    }

    public String getText() {
        return text.toString();
    }
}
