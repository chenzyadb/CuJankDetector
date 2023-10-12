#!/system/bin/sh

ui_print "- CuJankDetector Magisk Module"
ui_print ""
ui_print "** WARNING: INJECTING INTO SYSTEM PROCESS"
ui_print "MAY CAUSE SYSTEM CRASHES!!!"
ui_print ""

ui_print "- Check Environnment."
ANDROID_SDK=$(getprop ro.build.version.sdk)
DEVICE_ABI=$(getprop ro.product.cpu.abi)
if [ ${ANDROID_SDK} -lt 31 || ${DEVICE_ABI} != "arm64-v8a" ] ; then
	abort "- Your device does not meet the requirement, Abort."
fi

ui_print "- Extract Files."
unzip -o "${ZIPFILE}" -x 'META-INF/*' -d ${MODPATH} >&2

ui_print "- Set Permission."
chmod -R 0777 ${MODPATH}

ui_print "- Install Finished."
