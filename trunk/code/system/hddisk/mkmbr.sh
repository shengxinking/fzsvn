#!/usr/bin/env bash

echo "1 56000 83 *" > .tmpsfdisk
echo "56001 56000 83" >> .tmpsfdisk

dd if=/dev/zero of=mbr.img bs=512 count=1
sudo sfdisk -q -f -uS mbr.img < .tmpsfdisk

rm .tmpsfdisk

