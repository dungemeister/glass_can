#!bin/sh

set -e
SOURCE_PATH=$(dirname "$0")

. ${SOURCE_PATH}/.env

${SOURCE_PATH}/build/glass_can
