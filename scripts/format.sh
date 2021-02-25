#!/bin/bash

(find -name "*.h" ; find -name "*.cpp") | xargs clang-format-5.0 -style=file -i
