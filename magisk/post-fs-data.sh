#!/system/bin/sh
BASE_DIR=$(dirname "$0")

if [ ! -f "${BASE_DIR}/.system_crashed" ]; then
    true >"${BASE_DIR}/.system_crashed"
else
    rm -f "${BASE_DIR}/.system_crashed"
    true >"${BASE_DIR}/disable"
fi
