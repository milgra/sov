build/test_tree -c tst/config_a -w tst/workspace_a.json -t tst/tree_a.json -o tst/image_cfga_treea_test
build/test_tree -c tst/config_b -w tst/workspace_a.json -t tst/tree_a.json -o tst/image_cfgb_treea_test
build/test_tree -c tst/config_a -w tst/workspace_b.json -t tst/tree_b.json -o tst/image_cfga_treeb_test
build/test_tree -c tst/config_b -w tst/workspace_b.json -t tst/tree_b.json -o tst/image_cfgb_treeb_test
build/test_tree -c tst/config_a -w tst/workspace_empty.json -t tst/tree_empty.json -o tst/image_empty_test
build/test_tree -c tst/config_a -w tst/workspace_empty_6.json -t tst/tree_empty_6.json -o tst/image_empty_6_test
build/test_tree -c tst/config_a -w tst/workspace_floating.json -t tst/tree_floating.json -o tst/image_floating_test
build/test_tree -c tst/config_a -w tst/workspace_feh.json -t tst/tree_feh.json -o tst/image_feh_test
build/test_tree -c tst/config_a -w tst/workspace_rotated_multi.json -t tst/tree_rotated_multi.json -o tst/image_rotated_multi_test
diff tst/image_cfga_treea_o0.bmp tst/image_cfga_treea_test_o0.bmp
diff tst/image_cfgb_treea_o0.bmp tst/image_cfgb_treea_test_o0.bmp
diff tst/image_cfga_treeb_o0.bmp tst/image_cfga_treeb_test_o0.bmp
diff tst/image_cfgb_treeb_o0.bmp tst/image_cfgb_treeb_test_o0.bmp
diff tst/image_empty_o0.bmp tst/image_empty_test_o0.bmp
diff tst/image_empty_6_o0.bmp tst/image_empty_6_test_o0.bmp
diff tst/image_feh_o0.bmp tst/image_feh_test_o0.bmp
diff tst/image_floating_o0.bmp tst/image_floating_test_o0.bmp
diff tst/image_rotated_multi_o0.bmp tst/image_rotated_multi_test_o0.bmp
diff tst/image_rotated_multi_o1.bmp tst/image_rotated_multi_test_o1.bmp
