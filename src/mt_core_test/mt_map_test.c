
// tests map

#ifdef DEBUG
void mt_map_test()
{
    printf("MAP TEST SESSION START");
    printf("1 CREATE EMPTY");
    mt_map_t* m1 = mt_map_new();
    printf("2 DELETE EMPTY");
    REL(m1);
    printf("3 ADDING DATA");
    mt_map_t* m2 = mt_map_new();
    mt_map_put(m2, "fakk", "fakkvalue");
    mt_map_put(m2, "makk", "makkvalue");
    mt_map_put(m2, "takk", "takkvalue");
    mt_map_put(m2, "kakk", "kakkvalue");
    printf("4 GETTING DATA");
    printf(" VALUE FOR makk : %s", (char*) mt_map_get(m2, "makk"));
    printf("5 SETTING DATA TO NULL");
    mt_map_put(m2, "takk", NULL);
    printf(" VALUE FOR takk : %s", (char*) mt_map_get(m2, "takk"));
    printf("MAP TEST SESSION END");
}
#endif
