
//
//  TEST
/* TODO move this outside */
//

#define kChTestThreads 10

void send_test(mt_channel_t* ch)
{
    uint32_t counter = 0;
    while (1)
    {
	uint32_t* number = CAL(sizeof(uint32_t), NULL, NULL);
	*number          = counter;
	char success     = mt_channel_send(ch, number);
	if (success == 0)
	    REL(number);
	else
	    counter += 1;
	if (counter == UINT32_MAX - 1)
	    counter = 0;
	//            struct timespec time;
	//            time.tv_sec = 0;
	//            time.tv_nsec = rand() % 100000;
	//            nanosleep(&time , (struct timespec *)NULL);
    }
}

void recv_test(mt_channel_t* ch)
{
    uint32_t last = 0;
    while (1)
    {
	uint32_t* number = mt_channel_recv(ch);
	if (number != NULL)
	{
	    if (*number != last)
		printf("index error!!!");
	    REL(number);
	    last += 1;
	    if (last == UINT32_MAX - 1)
		last = 0;
	    if (last % 100000 == 0)
		printf("%zx OK %u %u", (size_t) ch, last, UINT32_MAX);
	    //                struct timespec time;
	    //                time.tv_sec = 0;
	    //                time.tv_nsec = rand() % 100000;
	    //                nanosleep(&time , (struct timespec *)NULL);
	}
    }
}

mt_channel_t** testarray;

void mt_channel_test()
{
    testarray = CAL(sizeof(mt_channel_t) * kChTestThreads, NULL, NULL);

    for (int index = 0; index < kChTestThreads; index++)
    {
	testarray[index] = mt_channel_new(100);
	/* pthread_t thread; */
	/* pthread_create(&thread, NULL, (void*)send_test, testarray[index]); */
	/* pthread_create(&thread, NULL, (void*)recv_test, testarray[index]); */
    }
}
