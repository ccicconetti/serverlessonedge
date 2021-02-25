#!/bin/bash


if [ ! -d opencv ] ; then
  mkdir opencv
fi

opencv_models="haarcascade_eye_tree_eyeglasses.xml haarcascade_frontalface_default.xml"

for model in $opencv_models ; do
  if [ ! -r opencv/$model ] ; then
    wget -O opencv/$model https://raw.githubusercontent.com/opencv/opencv/master/data/haarcascades/$model
  fi
done

