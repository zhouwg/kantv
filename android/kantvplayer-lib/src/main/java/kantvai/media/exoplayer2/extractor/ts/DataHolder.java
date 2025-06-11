package kantvai.media.exoplayer2.extractor.ts;


import java.util.Arrays;

public class DataHolder {

  public byte[] dataBuffer;
  public int dataLength;

  public DataHolder(int initialCapacity) {

    dataBuffer = new byte[initialCapacity];
  }

  /**
   * Resets the buffer, clearing any data that it holds.
   */
  public void reset() {
    dataLength = 0;
  }

  /**
   * Called to pass data.
   *
   * @param data Holds the data being passed.
   * @param offset The offset of the data in {@code data}.
   * @param limit The limit of the data in {@code data} to be put into buffer..
   */
  public void append(byte[] data, int offset, int limit) {

    int readLength = limit - offset;
    if (dataBuffer.length < dataLength + readLength) {
      dataBuffer = Arrays.copyOf(dataBuffer, (dataLength + readLength) * 2);
    }
    System.arraycopy(data, offset, dataBuffer, dataLength, readLength);
    dataLength += readLength;
  }

}
