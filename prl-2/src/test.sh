#!/bin/bash
#@FILE: test.sh

# Parallel and Distributed Algorithms course
# Project 2
# Preparation, compilation and execution script
# Author: Martin Cagas <xcagas01@stud.fit.vutbr.cz>

# Push the current directory on the stack
# Create a temporary compilation directory
pushd $(pwd) > /dev/null
TMP=$(mktemp -d -p $(pwd)) || exit 1
cd ${TMP}

# No other files were permitted to be turned in this assignment,
# and since Bash is rather insufficient for the required preparation,
# the Python scripts are included in the bash script.

# Create the program's input file from the script's input and get count of input items
echo -e "my_list = \"${1}\".split(',')"         >  prep_input.py
echo -e "with open('input', 'w') as f:"         >> prep_input.py
echo -e "\tfor my_item in my_list:"             >> prep_input.py
echo -e "\t\tf.write(\"%s\\\n\" % my_item)"     >> prep_input.py
echo -e "print(len(my_list))"                   >> prep_input.py

item_count=$(python prep_input.py)

# Validate input length and print output for the input of length 2,
# since the output will constant in that case.
if [[ ${item_count} -lt 2 ]]
then
    popd > /dev/null
    rm -rf ${TMP}
    exit 1
elif [[ ${item_count} -lt 3 ]]
then
    echo "_,v"
    popd > /dev/null
    rm -rf ${TMP}
    exit 0
fi

# Calculate the optimal number of CPUs
# The the algorthm is optimal, if the price meets the condition
#   log(N) < n/N, where:
#   'N' is the total number of CPUs
#   'n' is the number of inputs
# For the pre-scan to work, both the work done sequentially on one
# CPU and the work done in parallel among CPUs has to operate on
# a binary tree. Hence the count of CPUs and items per CPU needs
# to be the power of two.
echo -e "from math import log, ceil"            >  prep_cpus.py
echo -e "n = ${item_count}"                     >> prep_cpus.py
echo -e "n = 2 ** ceil(log(n, 2))"              >> prep_cpus.py
echo -e "N = 2"                                 >> prep_cpus.py
echo -e "while ((N * log(N, 2)) < n):"          >> prep_cpus.py
echo -e "\tprev_N = N"                          >> prep_cpus.py
echo -e "\tN *= 2"                              >> prep_cpus.py
echo -e "else:"                                 >> prep_cpus.py
echo -e "\tprint(int(prev_N))"                  >> prep_cpus.py

proc_count=$(python prep_cpus.py)

# Inline Python script for parsing the first out of all input items
base_altitude=$(python -c "print(\"${1}\".split(',')[0])")

#echo "Item count:" ${item_count}
#echo "Proc count:" ${proc_count}
#echo "Base altitude:" ${base_altitude}

# MPI compile and run
mpic++ --prefix /usr/local/share/OpenMPI -o vid ../vid.cpp
mpirun --prefix /usr/local/share/OpenMPI -np ${proc_count} vid ${item_count} ${base_altitude}

# Pop the original directory from the stack and clean up
popd > /dev/null
rm -rf ${TMP}

exit 0
