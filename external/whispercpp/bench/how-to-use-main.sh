ffmpeg -y  -i ./raw.wav -ac 2 -ar 16000 new.wav
./main  -m ./models/ggml-base.bin  -f ./new.wav
