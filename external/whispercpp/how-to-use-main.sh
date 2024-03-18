ffmpeg -y  -i ./samples/a13.wav -ac 2 -ar 16000 new.wav
./main  -m ./models/ggml-base.bin  -f ./new.wav
