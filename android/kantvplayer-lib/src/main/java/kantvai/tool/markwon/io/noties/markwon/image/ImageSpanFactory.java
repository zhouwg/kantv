package kantvai.tool.markwon.io.noties.markwon.image;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import kantvai.tool.markwon.io.noties.markwon.MarkwonConfiguration;
import kantvai.tool.markwon.io.noties.markwon.RenderProps;
import kantvai.tool.markwon.io.noties.markwon.SpanFactory;

public class ImageSpanFactory implements SpanFactory {
    @Nullable
    @Override
    public Object getSpans(@NonNull MarkwonConfiguration configuration, @NonNull RenderProps props) {
        return new AsyncDrawableSpan(
                configuration.theme(),
                new AsyncDrawable(
                        ImageProps.DESTINATION.require(props),
                        configuration.asyncDrawableLoader(),
                        configuration.imageSizeResolver(),
                        ImageProps.IMAGE_SIZE.get(props)
                ),
                AsyncDrawableSpan.ALIGN_BOTTOM,
                ImageProps.REPLACEMENT_TEXT_IS_LINK.get(props, false)
        );
    }
}
