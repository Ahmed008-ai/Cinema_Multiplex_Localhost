/* MEMORY.C - Small dynamic arrays for bookings and the waiting queue. */

/* Start both lists with a small capacity. They grow when required. */
int initDynamicLists(void)
{
    bookingCapacity = 5;
    waitCapacity = 5;

    bookings = (Booking *)malloc(bookingCapacity * sizeof(Booking));
    waitList = (WaitEntry *)malloc(waitCapacity * sizeof(WaitEntry));

    if (bookings == NULL || waitList == NULL)
    {
        free(bookings);
        free(waitList);
        bookings = NULL;
        waitList = NULL;
        return 0;
    }
    return 1;
}

/* Double the booking array when it becomes full. */
int growBookings(void)
{
    Booking *newList;
    int newCapacity = bookingCapacity * 2;

    newList = (Booking *)realloc(bookings, newCapacity * sizeof(Booking));
    if (newList == NULL)
    {
        printf("Not enough memory to add another booking.\n");
        return 0;
    }

    bookings = newList;
    bookingCapacity = newCapacity;
    printf("[INFO] Booking list expanded to %d records.\n", bookingCapacity);
    return 1;
}

/* Double the waiting queue when it becomes full. */
int growWaitList(void)
{
    WaitEntry *newList;
    int newCapacity = waitCapacity * 2;

    newList = (WaitEntry *)realloc(waitList, newCapacity * sizeof(WaitEntry));
    if (newList == NULL)
    {
        printf("Not enough memory to add another waiting-list entry.\n");
        return 0;
    }

    waitList = newList;
    waitCapacity = newCapacity;
    printf("[INFO] Waiting list expanded to %d entries.\n", waitCapacity);
    return 1;
}
