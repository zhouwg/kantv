package kantvai.tool.markwon.io.noties.markwon.core.factory;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import kantvai.tool.markwon.io.noties.markwon.MarkwonConfiguration;
import kantvai.tool.markwon.io.noties.markwon.RenderProps;
import kantvai.tool.markwon.io.noties.markwon.SpanFactory;
import kantvai.tool.markwon.io.noties.markwon.core.CoreProps;
import kantvai.tool.markwon.io.noties.markwon.core.spans.LinkSpan;

public class LinkSpanFactory implements SpanFactory {
    @Nullable
    @Override
    public Object getSpans(@NonNull MarkwonConfiguration configuration, @NonNull RenderProps props) {
        return new LinkSpan(
                configuration.theme(),
                CoreProps.LINK_DESTINATION.require(props),
                configuration.linkResolver()
        );
    }
}
