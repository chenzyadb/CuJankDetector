#!/system/bin/sh

ui_print ""
ui_print "** WARNING: INJECTING INTO SYSTEM PROCESS"
ui_print "** MAY CAUSE SYSTEM CRASHES!!!"
ui_print ""

ui_print "- Check Environment."
if [ "$(getprop ro.build.version.sdk)" -lt 31 || "$(getprop ro.product.cpu.abi)" != "arm64-v8a" ]; then
	abort "- Your device does not meet the requirement, Abort."
fi

if [ -n "$KSU_VER" ]; then
	ui_print "- Detected KernelSU ${KSU_VER}."
	echo "allow surfaceflinger * * *" >"${MODPATH}/sepolicy.rule"
fi

ui_print "- Set Permission."

chmod -R 0777 "$MODPATH"
chcon -t "surfaceflinger" "${MODPATH}/system/bin/injector"
chcon -t "surfaceflinger" "${MODPATH}/system/lib64/libCuJankDetector.so"
