#!/bin/bash

cd ..

draw -s tst/stylea -t tst/a_tree.json -w tst/a_workspace.json -r tst/result/a_result_a.bmp
draw -s tst/styleb -t tst/a_tree.json -w tst/a_workspace.json -r tst/result/a_result_b.bmp

build/draw -s tst/stylea -t tst/b_tree.json -w tst/b_workspace.json -r tst/result/b_result_a.bmp
build/draw -s tst/styleb -t tst/b_tree.json -w tst/b_workspace.json -r tst/result/b_result_b.bmp

build/draw -s tst/stylea -t tst/empty_tree.json -w tst/empty_workspace.json -r tst/result/empty_result_a.bmp
build/draw -s tst/styleb -t tst/empty_tree.json -w tst/empty_workspace.json -r tst/result/empty_result_b.bmp

build/draw -s tst/stylea -t tst/empty_6_tree.json -w tst/empty_6_workspace.json -r tst/result/empty_6_result_a.bmp
build/draw -s tst/styleb -t tst/empty_6_tree.json -w tst/empty_6_workspace.json -r tst/result/empty_6_result_b.bmp

build/draw -s tst/stylea -t tst/feh_tree.json -w tst/feh_workspace.json -r tst/result/feh_result_a.bmp
build/draw -s tst/styleb -t tst/feh_tree.json -w tst/feh_workspace.json -r tst/result/feh_result_b.bmp

build/draw -s tst/stylea -t tst/floating_tree.json -w tst/floating_workspace.json -r tst/result/floating_result_a.bmp
build/draw -s tst/styleb -t tst/floating_tree.json -w tst/floating_workspace.json -r tst/result/floating_result_b.bmp

build/draw -s tst/stylea -t tst/rotated_tree.json -w tst/rotated_workspace.json -r tst/result/rotated_result_a.bmp
build/draw -s tst/styleb -t tst/rotated_tree.json -w tst/rotated_workspace.json -r tst/result/rotated_result_b.bmp

diff tst/master tst/result

error=$?
if [ $error -eq 0 ]
then
    exit 0
elif [ $error -eq 1 ]
then
    exit 1
else
    exit 1
fi
