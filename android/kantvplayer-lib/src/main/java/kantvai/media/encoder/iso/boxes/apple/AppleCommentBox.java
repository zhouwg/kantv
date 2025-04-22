package kantvai.media.encoder.iso.boxes.apple;

/**
 * itunes MetaData comment box.
 */
public final class AppleCommentBox extends AbstractAppleMetaDataBox {
    public static final String TYPE = "\u00a9cmt";


    public AppleCommentBox() {
        super(TYPE);
        appleDataBox = AppleDataBox.getStringAppleDataBox();
    }


}
