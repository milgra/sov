#ifndef mt_channel_test_h
#define mt_channel_test_h

void mt_channel_test_main();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_channel.c"
#include "mt_log.c"
#include "mt_map.c"

#define kChTestThreads 20
#define kChTestMax 60000

static int success = 1;

void* send_test(void* chp)
{
    mt_channel_t* ch      = chp;
    uint32_t      counter = 0;
    while (1)
    {
	if (success == 0)
	    break;

	uint32_t* number = CAL(sizeof(uint32_t), NULL, NULL);
	*number          = counter;
	char sent        = mt_channel_send(ch, number);

	if (sent == 0)
	{
	    REL(number);
	}
	else
	{
	    counter += 1;
	    if (counter == kChTestMax)
		break;
	}

	//            struct timespec time;
	//            time.tv_sec = 0;
	//            time.tv_nsec = rand() % 100000;
	//            nanosleep(&time , (struct timespec *)NULL);
    }

    return NULL;
}

void* recv_test(void* chp)
{
    mt_channel_t* ch   = chp;
    uint32_t      last = 0;
    while (1)
    {
	if (success == 0)
	    break;

	uint32_t* number = mt_channel_recv(ch);
	if (number != NULL)
	{
	    if (*number != last)
	    {
		success = 0;
		/* printf("index error!!! %u %u %i\n", *number, last, success); */
		break;
	    }

	    REL(number);
	    last += 1;

	    if (last == kChTestMax)
		break;

	    //                struct timespec time;
	    //                time.tv_sec = 0;
	    //                time.tv_nsec = rand() % 100000;
	    //                nanosleep(&time , (struct timespec *)NULL);
	}
    }

    return NULL;
}

mt_channel_t** testarray;

void mt_channel_test_main()
{
    mt_log_debug("testing mt_channel");

    testarray = CAL(sizeof(mt_channel_t) * kChTestThreads, NULL, NULL);

    pthread_t* threads = CAL(sizeof(pthread_t) * kChTestThreads * 2, NULL, NULL);

    uint32_t threadidx = 0;

    for (int index = 0; index < kChTestThreads; index++)
    {
	testarray[index] = mt_channel_new(100);
	pthread_create(&threads[threadidx++], NULL, send_test, testarray[index]);
	pthread_create(&threads[threadidx++], NULL, recv_test, testarray[index]);
    }

    for (uint32_t index = 0; index < threadidx; index++)
    {
	pthread_join(threads[index], NULL);
    }

    REL(threads);

    assert(success == 1);
}

#endif
