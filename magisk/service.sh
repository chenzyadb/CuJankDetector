#!/system/bin/sh
BASE_DIR=$(dirname "$0")

if [ "$(getprop ro.build.version.sdk)" -lt 31 || "$(getprop ro.product.cpu.abi)" != "arm64-v8a" ]; then
	exit 0
fi

echo "" >/dev/jank.message
if [ ! -f "/dev/jank.message" ]; then
	exit 0
fi
chmod 0666 /dev/jank.message

while [ ! -n "$(pgrep -f surfaceflinger)" ]; do
	sleep 1
done

if [ -f /data/adb/magisk/magiskpolicy ]; then
	magiskpolicy --live "allow surfaceflinger * * *"
fi

sleep 5
/system/bin/injector -p "$(pgrep -f surfaceflinger)" -l "/system/lib64/libCuJankDetector.so" > "${BASE_DIR}/inject.log" 2>&1

sleep 30
if [ -f "${BASE_DIR}/.system_crashed" ]; then
	rm -f "${BASE_DIR}/.system_crashed"
fi
