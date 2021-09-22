#!/bin/bash

CURR_DIR=$(pwd)

PROF_FILE=~/.bashrc
source $PROF_FILE

yum list installed devtoolset-8 &> /dev/null 
if [[ $? != 0 ]]; then
    echo "devtoolset-8 is not installed! Exiting..."
fi 

eval "$(conda shell.bash hook)"

conda info &> /dev/null 
if [ $? != 0 ]; then
    echo "Conda is not installed! Exiting..."
    exit
fi

CONDA_ENV=sosa-compiler
LIST=$(conda env list)
if [[ "$LIST" == *${CONDA_ENV}*  ]]; then 
    conda env remove -n ${CONDA_ENV}
fi

conda env create -f conda.yml
conda activate $CONDA_ENV

WORK_DIR=~
cd $WORK_DIR

VER=1.77.0
wget https://boostorg.jfrog.io/artifactory/main/release/${VER}/source/boost_${VER//./_}.tar.gz
tar -xvf ${WORK_DIR}/boost_${VER//./_}.tar.gz
mv boost_${VER//./_} boost 
cd boost 
./bootstrap.sh --prefix=${WORK_DIR}/boost --with-libraries=log,system
./b2 install
rm ${WORK_DIR}/boost_${VER//./_}.tar.gz

if [[ ${LD_LIBRARY_PATH} != *${WORK_DIR}/boost/lib* ]]; then
    echo \# Auto-added by sosa-compiler/setup.sh >> ${PROF_FILE}
    echo export LD_LIBRARY_PATH=\${LD_LIBRARY_PATH}:${WORK_DIR}/boost/lib >> ${PROF_FILE}
fi

cd $CURR_DIR