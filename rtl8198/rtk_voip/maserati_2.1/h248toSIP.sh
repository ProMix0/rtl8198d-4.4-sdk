#!/bin/sh

killall voip_gwdt
killall H248_carreraGT
killall mgapp
cp /etc/scripts/app.gwdt.sip /var/app.gwdt
/bin/voip_gwdt /var/app.gwdt &
