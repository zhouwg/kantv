package cdeos.media.encoder.iso.boxes;

import cdeos.media.encoder.iso.IsoTypeReader;
import cdeos.media.encoder.iso.Utf8;
import cdeos.media.encoder.mp4parser.AbstractFullBox;

import java.nio.ByteBuffer;

/**
 *
 */
public class XmlBox extends AbstractFullBox {
    String xml = "";
    public static final String TYPE = "xml ";

    public XmlBox() {
        super(TYPE);
    }

    public String getXml() {
        return xml;
    }

    public void setXml(String xml) {
        this.xml = xml;
    }

    @Override
    protected long getContentSize() {
        return 4 + Utf8.utf8StringLengthInBytes(xml);
    }

    @Override
    public void _parseDetails(ByteBuffer content) {
        parseVersionAndFlags(content);
        xml = IsoTypeReader.readString(content, content.remaining());
    }

    @Override
    protected void getContent(ByteBuffer byteBuffer) {
        writeVersionAndFlags(byteBuffer);
        byteBuffer.put(Utf8.convert(xml));
    }

    @Override
    public String toString() {
        return "XmlBox{" +
                "xml='" + xml + '\'' +
                '}';
    }
}
