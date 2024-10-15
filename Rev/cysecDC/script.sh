#!/bin/bash

target_hash="040d7f7bd376bbb5cb0d08663325a79609dbffabe575995bfb003e009d82cb1c"
while IFS= read -r phrase
do
    phrase=$(echo -n "$phrase" | tr -d '\n')
    hash=$(echo -n "$phrase" | sha256sum | awk '{ print $1 }')
    if [ "$hash" == "$target_hash" ]; then
        echo "$phrase"
    fi
done < phrases.txt
