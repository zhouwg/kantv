#!/usr/bin/env bash

format_code() {
    find ./ -type f -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.h' | xargs -i clang-format -i {}
}

format_code || { echo 'Formatting failed' ; exit 1; }
