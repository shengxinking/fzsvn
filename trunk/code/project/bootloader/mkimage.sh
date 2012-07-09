#!/bin/bash

if [ -f cf_512M.img ]; then
	echo "image file cf_512M.img is exist"
else
	bximage -hd -mode=flat -size=512 -q cf_512M.img
fi


