#!/bin/bash

if ( {
     sudo apt-get update         # 2>&1
     sudo apt-get install -y zsh # 2>&1
   } > install-zsh.txt
   )
then
  echo "install-zsh: OK"
else
  echo "install-zsh: FAILED!"
fi
