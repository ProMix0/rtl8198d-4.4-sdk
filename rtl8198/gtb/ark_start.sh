#!/bin/sh

ark_startup() {
	while ps | grep startup | grep -v grep > /dev/null
	do
		sleep 1
	done
	sleep 20

	[ -f /var/app/ark/gtb_agent ] && /bin/sh /var/app/ark/gtb_agent
}

# Always reset ARK version
[ -d /var/app/ark ] && rm -rf /var/app/ark
tar xf /etc/gtb/SmartRouter.Realtek.tar -C /var/app/

# ARK_MODE=1: start ARK when booting time
ark_mode=$(flash get GTB_ARK_MODE | awk '{split($0,a,"="); print a[2]}')
[ ${ark_mode} -eq 1 ] && ark_startup &

