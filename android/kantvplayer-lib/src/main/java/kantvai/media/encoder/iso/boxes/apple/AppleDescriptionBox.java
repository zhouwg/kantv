package kantvai.media.encoder.iso.boxes.apple;

/**
 *
 */
public final class AppleDescriptionBox extends AbstractAppleMetaDataBox {
    public static final String TYPE = "desc";


    public AppleDescriptionBox() {
        super(TYPE);
        appleDataBox = AppleDataBox.getStringAppleDataBox();
    }

}