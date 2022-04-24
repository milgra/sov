build/test_tree -c tst/config_a -w tst/workspace_a.json -t tst/tree_a.json -o tst/image_cfga_treea_test.bmp
build/test_tree -c tst/config_b -w tst/workspace_a.json -t tst/tree_a.json -o tst/image_cfgb_treea_test.bmp
build/test_tree -c tst/config_a -w tst/workspace_b.json -t tst/tree_b.json -o tst/image_cfga_treeb_test.bmp
build/test_tree -c tst/config_b -w tst/workspace_b.json -t tst/tree_b.json -o tst/image_cfgb_treeb_test.bmp
build/test_tree -c tst/config_a -w tst/workspace_empty.json -t tst/tree_empty.json -o tst/image_empty_test.bmp
build/test_tree -c tst/config_a -w tst/workspace_floating.json -t tst/tree_floating.json -o tst/image_floating_test.bmp
build/test_tree -c tst/config_a -w tst/workspace_feh.json -t tst/tree_feh.json -o tst/image_feh_test.bmp
diff tst/image_cfga_treea.bmp tst/image_cfga_treea_test.bmp
diff tst/image_cfgb_treea.bmp tst/image_cfgb_treea_test.bmp
diff tst/image_cfga_treeb.bmp tst/image_cfga_treeb_test.bmp
diff tst/image_cfgb_treeb.bmp tst/image_cfgb_treeb_test.bmp
diff tst/image_empty.bmp tst/image_empty_test.bmp
diff tst/image_feh.bmp tst/image_feh_test.bmp
diff tst/image_floating.bmp tst/image_floating_test.bmp
