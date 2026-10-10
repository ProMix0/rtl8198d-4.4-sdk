#!/bin/sh

mkdir -p /var/ther
cp -f /etc/ther.conf /var/ther/conf

killall ther_control
ther_control &
