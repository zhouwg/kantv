#author:nihui, https://github.com/nihui/ncnn_on_esp32/blob/master/image/img2c.py
#!/usr/bin/env python3

import cv2
import os
import sys
import numpy as np
import glob

shape = (28, 28)
if __name__ == '__main__':

    im = cv2.imread(sys.argv[1], 0)
    im = cv2.resize(im, shape)

    img = im.flatten()

    print(img.shape, img.dtype)

    with open(os.getcwd() + '/fs.h', 'w+') as f:
        print('const int IMAGE_H = %d;' % shape[0], file=f)
        print('const int IMAGE_W = %d;' % shape[1], file=f)
        print('const unsigned char IN_IMG[]  __attribute__((aligned(128))) ={', file=f)
        counter = 0
        for pixel in img:
            counter += 1
            print('0x%x, ' % (255 - pixel), end='', file=f)
            if counter % 8 == 0:
                print(file=f)
        print(file=f)
        print('};', file=f)
