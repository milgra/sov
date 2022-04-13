build/test-tree -c tst/config_a -w tst/workspace_a.json -t tst/tree_a.json -o tst/image_1.bmp
build/test-tree -c tst/config_b -w tst/workspace_a.json -t tst/tree_a.json -o tst/image_2.bmp
build/test-tree -c tst/config_a -w tst/workspace_b.json -t tst/tree_b.json -o tst/image_3.bmp
build/test-tree -c tst/config_b -w tst/workspace_b.json -t tst/tree_b.json -o tst/image_4.bmp
diff tst/image_1.bmp tst/original_1.bmp
diff tst/image_2.bmp tst/original_2.bmp
diff tst/image_3.bmp tst/original_3.bmp
diff tst/image_4.bmp tst/original_4.bmp
