#!/bin/bash -x

export PROJECT_NAME="uni-ktx-software"
export ROLLOUT_WAIT_TIME=360
export CURRENT_BRANCH=`git branch | grep \* | cut -d ' ' -f2`