#!/system/bin/sh
BASE_DIR=$(dirname $0)

while [ ! -n "$(pgrep -f surfaceflinger)" ] ; do
	sleep 1;
done

true > /dev/jank.message
chmod 0666 /dev/jank.message

magiskpolicy --live "allow surfaceflinger * * *"
/system/bin/injector -p $(pgrep -f surfaceflinger) -so /system/lib64/libCuJankDetector.so -symbols start_hook > ${BASE_DIR}/inject.log

sleep 60
if [ -e ${BASE_DIR}/.system_crashed ] ; then
	rm -f ${BASE_DIR}/.system_crashed
fi
