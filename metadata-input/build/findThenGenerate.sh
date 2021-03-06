#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 [input HDF file directory] [output DB directory]"
    exit 1
fi

# Get the absolute path of this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)"

DBDIR=$2                                        # Where the database will be stored
findFiles="$SCRIPT_DIR/findFiles"               # Where the findFiles script is located
INDIR=$1                                        # Where the input HDF files are located
DATADIR="$SCRIPT_DIR/../data"                   # Where to store intermediate results of the findFiles query
PYTHON_MODULE="python"                          # The site-specific python module to load

#_____PYTHON DEPENDENCY RESOLUTIONS__________#
# The fusionBuilDB python script requires:
# 1. Docopt
# 2. pytz
# 3. ???
#
# A virtual environment has been provided in externLib to resolve these dependencies. Users can generate this
# virtual environment by using the configureEnv.sh script in the basicFusion/util directory. 

source "$SCRIPT_DIR"/../../externLib/BFpyEnv/bin/activate
export PYTHONPATH="$SCRIPT_DIR"/../../externLib/BFpyEnv/lib/python2.7/site-packages
#____________________________________________#


if [ 0 -eq 1 ]; then
    eval $findFiles "$INDIR"

    if [ $? -ne 0 ]; then
        echo "$findFiles returned with exit status of $?."
        echo "Exiting script."
        exit 1
    fi

    for i in ASTER MODIS MISR MOPITT CERES; do
       gzip -f "$DATADIR"/$i.list 
        if [ $? -ne 0 ]; then
            echo "gzip returned with exit status of $?."
            echo "Exiting script."
            exit 1
        fi

    done
fi

python ./fusionBuildDB -o -q --discards=discards.txt --anomalies=errors.txt "$DBDIR" "$DATADIR"/Orbit_Path_Time.txt.gz "$DATADIR"/MODIS.list.gz "$DATADIR"/ASTER.list.gz "$DATADIR"/MOPITT.list.gz "$DATADIR"/MISR.list.gz "$DATADIR"/CERES.list.gz

if [ $? -ne 0 ]; then
    echo "fusionBuildDB returned with exit status of $?."
    echo "Exiting script."
    exit 1
fi

