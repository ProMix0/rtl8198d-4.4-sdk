#!/bin/sh

IS_248=`flash voip | grep "X_CT_SERVERTYPE" | grep -c "2"`

if [ -f /etc/scripts/app.gwdt.h248 ]; then
    if [ ${IS_248} -eq 1 ]; then
    	echo "Use H.248"
    	cp /etc/scripts/app.gwdt.h248 /var/app.gwdt
#        /bin/mgapp &
    else	
    	echo "USE SIP"
    	cp /etc/scripts/app.gwdt.sip /var/app.gwdt
    fi	
else
    cp /etc/scripts/app.gwdt.sip /var/app.gwdt
fi
