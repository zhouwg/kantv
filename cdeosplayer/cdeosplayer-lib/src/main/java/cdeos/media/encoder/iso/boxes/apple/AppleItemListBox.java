package cdeos.media.encoder.iso.boxes.apple;

import cdeos.media.encoder.mp4parser.AbstractContainerBox;

/**
 * undocumented iTunes MetaData Box.
 */
public class AppleItemListBox extends AbstractContainerBox {
    public static final String TYPE = "ilst";

    public AppleItemListBox() {
        super(TYPE);
    }

}
