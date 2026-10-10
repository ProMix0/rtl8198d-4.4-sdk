#!/bin/sh

killall voip_gwdt
killall VoIP_maserati
cp /etc/scripts/app.gwdt.h248 /var/app.gwdt
/bin/voip_gwdt /var/app.gwdt &
