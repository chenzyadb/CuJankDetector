#!/system/bin/sh
BASE_DIR=$(dirname $0)

if [ ! -e ${BASE_DIR}/.system_crashed ] ; then
    true > ${BASE_DIR}/.system_crashed
else 
    true > ${BASE_DIR}/disable
fi
