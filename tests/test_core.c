/*
   Small automated tests for the important project rules.
   Compile with: gcc -std=c99 -Wall -Wextra -DCINEMA_TEST -I. tests/test_core.c main.c -o test_core
*/
#include "../backend/project.h"
#include <assert.h>

static void resetTestData(void)
{
    int i;
    movieCount = 0;
    showCount = 0;
    customerCount = 0;
    bookingCount = 0;
    waitCount = 0;
    nextBookingNumber = 1;
    for (i = 0; i < MAX_SCREENS; i++)
    {
        screens[i].id = i;
        screens[i].rows = ROWS;
        screens[i].cols = COLS;
        sprintf(screens[i].name, "Screen %d", i + 1);
    }
    initAllSeats();
    initCombos();
    if (bookings == NULL || waitList == NULL)
        assert(initDynamicLists());
}

static void addTestCustomer(int id, const char *name, const char *phone)
{
    customers[customerCount].id = id;
    strcpy(customers[customerCount].name, name);
    strcpy(customers[customerCount].phone, phone);
    customers[customerCount].points = 0;
    customerCount++;
}

int main(void)
{
    int s1, s2, s3, s4;
    int index;
    int screen;
    int suggested;
    int i;
    char date[11];
    char nextDate[11];
    struct tm parsed;
    time_t stamp;

    resetTestData();
    stamp = time(NULL) + 15 * 86400;
    strftime(date, sizeof(date), "%Y-%m-%d", localtime(&stamp));
    assert(loadAll() == 0); /* Missing files on first run are handled. */
    loadSampleMovies();
    addTestCustomer(1, "Test Customer", "03001234567");
    addTestCustomer(2, "Waiting Customer", "03007654321");
    assert(isDuplicatePhone("03001234567") == 1);
    assert(isDuplicatePhone("03123456789") == 0);
    assert(isValidSeat(-1, 0) == 0 && isValidSeat(5, 7) == 1);

    /* Input/date checks. */
    assert(parseDate("2026-12-01", &parsed) == 1);
    assert(parseDate("2026-02-31", &parsed) == 0);
    assert(parseDate("bad-date", &parsed) == 0);
    assert(equalsIgnoreCase("bk0001", "BK0001") == 1);
    assert(equalsIgnoreCase("SAVE10", "save10") == 1);

    /* The first three shows at the same time use exactly three screens. */
    s1 = createShow(0, date, 10 * 60, 0, -1);
    s2 = createShow(1, date, 10 * 60, 0, -1);
    s3 = createShow(2, date, 10 * 60, 0, -1);
    assert(s1 == 0 && s2 == 1 && s3 == 2);
    assert(shows[s1].screenId == 0);
    assert(shows[s2].screenId == 1);
    assert(shows[s3].screenId == 2);
    assert(createShow(0, date, 10 * 60 + 15, 0, -1) == -1);
    assert(isScreenAvailable(0, date, 12 * 60 + 47, movies[0].duration) == 0);
    assert(isScreenAvailable(0, date, 12 * 60 + 48, movies[0].duration) == 1);
    assert(findNextAvailableTime(0, date, 10 * 60, &screen, &suggested) == 1);
    assert(suggested > 10 * 60);
    assert(screen >= 0 && screen < MAX_SCREENS);

    /* A later show can reuse an existing physical screen after the clean-up gap. */
    s4 = createShow(0, date, 13 * 60, 0, -1);
    assert(s4 == 3);
    assert(shows[s4].screenId == 0);
    assert(showCount == 4);

    /* Seat availability is show-specific; a double booking is rejected. */
    index = createBooking(1, s1, 0, 0, 0.0f, 0.0f);
    assert(index == 0);
    assert(seats[shows[s1].screenId][s1][0][0].status == 1);
    assert(createBooking(1, s1, 0, 0, 0.0f, 0.0f) == -1);
    assert(isSeatFree(s1, 0, 0) == 0);
    assert(isSeatFree(s2, 0, 0) == 1);

    /* Dynamic pricing: weekday morning Silver is $500 less 10%. */
    assert(getTicketPrice(s1, 1) > 449.9f && getTicketPrice(s1, 1) < 450.1f);
    assert(getPromoPercent("save10") == 10.0f);
    assert(getPromoPercent("NOTACODE") < 0.0f);

    /* Grow the booking array past its initial capacity and verify search. */
    for (i = 1; i < 8; i++)
        assert(createBooking(1, s1, i / COLS, i % COLS, 100.0f, 10.0f) >= 0);
    assert(bookingCount == 8);
    assert(bookingCapacity >= 10);
    assert(findBookingById("bk0001") >= 0);
    assert(calcLoyaltyPoints(1, bookingCount - 1) > 0);
    assert(bookings[1].foodCost == 100.0f);
    assert(bookings[1].discount > 54.9f && bookings[1].discount < 55.1f);
    assert(bookings[1].totalPaid > 494.9f && bookings[1].totalPaid < 495.1f);

    /* Fill the first show past the 50% and 80% demand thresholds. */
    for (i = 0; i < ROWS && countBookedSeats(s1) < 25; i++)
    {
        int col;
        for (col = 0; col < COLS && countBookedSeats(s1) < 25; col++)
            if (isSeatFree(s1, i, col))
                assert(createBooking(1, s1, i, col, 0.0f, 0.0f) >= 0);
    }
    assert(countBookedSeats(s1) == 25);
    assert(getTicketPrice(s1, 1) > 494.9f); /* 50% demand surcharge */
    for (i = 0; i < ROWS && countBookedSeats(s1) < 39; i++)
    {
        int col;
        for (col = 0; col < COLS && countBookedSeats(s1) < 39; col++)
            if (isSeatFree(s1, i, col))
                assert(createBooking(1, s1, i, col, 0.0f, 0.0f) >= 0);
    }
    assert(countBookedSeats(s1) == 39);
    assert(getTicketPrice(s1, 1) > 562.4f); /* 80% high-demand surcharge */
    for (i = 0; i < ROWS; i++)
    {
        int col;
        for (col = 0; col < COLS; col++)
            if (isSeatFree(s1, i, col))
                assert(createBooking(1, s1, i, col, 0.0f, 0.0f) >= 0);
    }
    assert(isShowSoldOut(s1) == 1);

    /* Cancellation applies the refund tier and automatically promotes the queue. */
    assert(addToWaitList(2, s1) == 1);
    {
        float refund = cancelBookingRecord(0, 30);
        assert(refund > 404.9f && refund < 405.1f);
    }
    assert(bookings[0].active == 0);
    assert(waitCount == 0);
    assert(seats[shows[s1].screenId][s1][0][0].status == 1);
    assert(bookingCount == 49);

    /* Fill the second show completely and check the sold-out rule. */
    for (i = 0; i < ROWS; i++)
    {
        int col;
        for (col = 0; col < COLS; col++)
            if (isSeatFree(s2, i, col))
                assert(createBooking(1, s2, i, col, 0.0f, 0.0f) >= 0);
    }
    assert(isShowSoldOut(s2) == 1);
    assert(addToWaitList(2, s3) == 0); /* A non-sold-out show has no queue. */

    /* Waiting list grows and a promotion claims a seat on the sold-out show. */
    for (i = 0; i < 7; i++)
    {
        char phone[16];
        char name[40];
        sprintf(phone, "031100000%02d", i);
        sprintf(name, "Queue Customer %d", i);
        addTestCustomer(10 + i, name, phone);
        assert(addToWaitList(10 + i, s2) == 1);
    }
    assert(waitCount == 7);
    assert(waitCapacity >= 10);
    assert(addToWaitList(2, s2) == 1);
    assert(waitCount == 8);

    /* Cancelling one ticket from the sold-out show auto-promotes the queue. */
    index = -1;
    for (i = 0; i < bookingCount; i++)
        if (bookings[i].showId == s2 && bookings[i].active)
        {
            index = i;
            break;
        }
    assert(index >= 0);
    assert(cancelBookingRecord(index, 0) == 0.0f);
    assert(waitCount == 7);
    assert(bookings[bookingCount - 1].customerId == 10);
    assert(isShowSoldOut(s2) == 1);

    /* Refund tiers. */
    assert(calculateRefund(100.0f, 30) > 89.9f);
    assert(calculateRefund(100.0f, 12) > 49.9f && calculateRefund(100.0f, 12) < 50.1f);
    assert(calculateRefund(100.0f, 3) > 24.9f && calculateRefund(100.0f, 3) < 25.1f);
    assert(calculateRefund(100.0f, 0) == 0.0f);

    /* A date in the future gives a positive remaining-hours result. */
    stamp = time(NULL) + 86400 * 10;
    {
        struct tm *future = localtime(&stamp);
        strftime(nextDate, sizeof(nextDate), "%Y-%m-%d", future);
    }
    s4 = createShow(1, nextDate, 20 * 60, 1, -1);
    assert(s4 >= 0);
    assert(hoursUntilShow(s4) > 0);
    assert(getTicketPrice(s4, 1) > 689.9f && getTicketPrice(s4, 1) < 690.1f);

    /* Save and reload all records, including seats, customers and wait list. */
    saveAll();
    movieCount = showCount = customerCount = bookingCount = waitCount = 0;
    initAllSeats();
    assert(loadAll() == 1);
    assert(movieCount == 3);
    assert(showCount == 5);
    assert(customerCount == 9);
    assert(bookingCount == 98);
    assert(waitCount == 7);
    assert(seats[shows[0].screenId][0][0][0].status == 1);
    assert(findCustomerByPhone("03001234567") >= 0);

    sortMoviesByRevenue();
    sortShowsByOccupancy();
    printf("\nALL CORE TESTS PASSED.\n");
    free(bookings);
    free(waitList);
    return 0;
}
