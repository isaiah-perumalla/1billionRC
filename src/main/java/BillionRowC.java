import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.Paths;
import java.util.concurrent.TimeUnit;

public class BillionRowC {

    public static final int PAGE_SIZE = 4096;

    static void main(String[] args) throws IOException {

        final var filePath = Paths.get(System.getProperty("input.file", "./measurements.txt"));
        System.out.println("reading " + filePath);

        ByteBuffer buffer = ByteBuffer.allocateDirect(PAGE_SIZE);
        try (final var channel = FileChannel.open(filePath)) {
            final long start = System.nanoTime();
            long totalRead = 0;
            long chkSum = 0;
            do {
                buffer.clear();
                int read = channel.read(buffer);
                if (read <= 0) {
                    break;
                }
                totalRead += read;
                if (read > 7) {
                    buffer.flip(); //flip to do the read from start
                    chkSum += buffer.getLong();
                }
            } while(true);
            System.out.println("sum= " + chkSum + "; read total MBytes " + totalRead/1000000.0 );
            System.out.println("Total time milliseconds " + TimeUnit.NANOSECONDS.toMillis(System.nanoTime() - start));
        }

    }
}