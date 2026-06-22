#!/bin/bash
set -e

IMAGE="glass_can_app"
APP="docker.glass_can_app"

sudo systemctl stop $APP
sudo docker build -t $IMAGE .

sudo systemctl start $APP
