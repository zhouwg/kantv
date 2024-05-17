package cdeos.media.encoder.iso.boxes.apple;

import cdeos.media.encoder.iso.IsoTypeReader;
import cdeos.media.encoder.iso.Utf8;
import cdeos.media.encoder.mp4parser.AbstractFullBox;

import java.nio.ByteBuffer;

/**
 * Apple Meaning box. Allowed as subbox of "----" box.
 *
 * @see cdeos.media.encoder.iso.boxes.apple.AppleGenericBox
 */
public final class AppleMeanBox extends AbstractFullBox {
    public static final String TYPE = "mean";
    private String meaning;

    public AppleMeanBox() {
        super(TYPE);
    }

    protected long getContentSize() {
        return 4 + Utf8.utf8StringLengthInBytes(meaning);
    }

    @Override
    public void _parseDetails(ByteBuffer content) {
        parseVersionAndFlags(content);
        meaning = IsoTypeReader.readString(content, content.remaining());
    }

    @Override
    protected void getContent(ByteBuffer byteBuffer) {
        writeVersionAndFlags(byteBuffer);
        byteBuffer.put(Utf8.convert(meaning));
    }

    public String getMeaning() {
        return meaning;
    }

    public void setMeaning(String meaning) {
        this.meaning = meaning;
    }


}
