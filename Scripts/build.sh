#!/bin/zsh

swift build \
  -Xcc -I$(swiftly use --print-location)/usr/include \
  -Xcc -Wno-elaborated-enum-base

