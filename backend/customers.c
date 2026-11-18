/* CUSTOMERS.C - Registration, refunds, waiting list and loyalty points. */

int isDuplicatePhone(const char phone[])
{
    return findCustomerByPhone(phone) >= 0;
}

int findCustomerByPhone(const char phone[])
{
    int i;
    for (i = 0; i < customerCount; i++)
    {
        if (equalsIgnoreCase(customers[i].phone, phone))
            return i;
    }
    return -1;
}

void registerCustomer(void)
{
    Customer *customer;

    printHeader("CUSTOMER REGISTRATION");
    if (customerCount >= MAX_CUSTOMERS)
    {
        printf("Customer list is full (maximum %d customers).\n", MAX_CUSTOMERS);
        return;
    }

    customer = &customers[customerCount];
    customer->id = customerCount + 1;
    getText("Customer full name: ", customer->name, sizeof(customer->name));
    if (inputEnded)
        return;
    getPhone("Phone number (digits only): ", customer->phone);
    if (inputEnded)
        return;

    if (isDuplicatePhone(customer->phone))
    {
        printf("This phone number is already registered. Registration cancelled.\n");
        return;
    }

    customer->points = 0;
    customerCount++;
    printf("\nRegistration successful! Your customer ID is %d.\n", customer->id);
}

/* Refund tiers: 24+ hours = 90%, 6-23 = 50%, 1-5 = 25%, less than 1 = 0%. */
float calculateRefund(float amount, int hoursRemaining)
{
    if (hoursRemaining >= 24)
        return amount * 0.90f;
    else if (hoursRemaining >= 6)
        return amount * 0.50f;
    else if (hoursRemaining >= 1)
        return amount * 0.25f;
    else
        return 0.0f;
}

/* Calculate the actual hours until the scheduled show using the PC clock. */
int hoursUntilShow(int showId)
{
    struct tm showDate;
    time_t showTime;
    double secondsLeft;

    if (showId < 0 || showId >= showCount)
        return 0;
    if (!parseDate(shows[showId].date, &showDate))
        return 0;

    showDate.tm_hour = shows[showId].startMinutes / 60;
    showDate.tm_min = shows[showId].startMinutes % 60;
    showDate.tm_sec = 0;
    showDate.tm_isdst = -1;
    showTime = mktime(&showDate);
    if (showTime == (time_t)-1)
        return 0;

    secondsLeft = difftime(showTime, time(NULL));
    if (secondsLeft <= 0)
        return 0;
    return (int)(secondsLeft / 3600.0);
}

int addToWaitList(int customerId, int showId)
{
    int i;

    if (showId < 0 || showId >= showCount || customerId <= 0)
        return 0;
    if (!isShowSoldOut(showId))
    {
        printf("The waiting list is only available when a show is sold out.\n");
        return 0;
    }

    /* Do not allow the same customer to join the same show's queue twice. */
    for (i = 0; i < waitCount; i++)
        if (waitList[i].customerId == customerId && waitList[i].showId == showId)
        {
            printf("This customer is already in the waiting list.\n");
            return 0;
        }

    if (waitCount >= waitCapacity && !growWaitList())
        return 0;

    waitList[waitCount].customerId = customerId;
    waitList[waitCount].showId = showId;
    waitCount++;
    printf("Added to waiting list at position %d.\n", waitCount);
    return 1;
}

void viewWaitList(int showId)
{
    int i;
    int position = 0;

    printHeader("SHOW WAITING LIST");
    for (i = 0; i < waitCount; i++)
    {
        if (waitList[i].showId == showId)
        {
            position++;
            printf("%d. Customer ID: %d\n", position, waitList[i].customerId);
        }
    }
    if (position == 0)
        printf("No customers are waiting for this show.\n");
}

/* When a seat becomes free, the first person waiting for that show gets it. */
void promoteFromWaitList(int showId, int row, int col)
{
    int i = 0;
    int customerIndex;
    int bookingIndex;
    int customerId;

    if (!isSeatFree(showId, row, col))
        return;

    /* Keep checking the same queue position after removing an invalid entry. */
    while (i < waitCount)
    {
        if (waitList[i].showId != showId)
        {
            i++;
            continue;
        }

        customerId = waitList[i].customerId;
        customerIndex = -1;
        for (bookingIndex = 0; bookingIndex < customerCount; bookingIndex++)
            if (customers[bookingIndex].id == customerId)
            {
                customerIndex = bookingIndex;
                break;
            }

        if (customerIndex >= 0)
        {
            bookingIndex = createBooking(customerId, showId, row, col, 0.0f, 0.0f);
            if (bookingIndex < 0)
            {
                printf("Could not promote the next customer. They remain in the queue.\n");
                return;
            }
            printf("\n[WAITING LIST] Customer %d has been promoted. New booking: %s\n",
                   customerId, bookings[bookingIndex].id);
        }
        else
        {
            printf("Removing an invalid customer from the waiting list.\n");
        }

        /* Remove this entry. If it was invalid, try the next person too. */
        for (; i < waitCount - 1; i++)
            waitList[i] = waitList[i + 1];
        waitCount--;

        if (customerIndex >= 0)
            return;
    }
}

/* Cancel one booking record. This separate function makes the rules easy
   to test without typing through the interactive menu. It returns -1 if the
   booking cannot be cancelled; otherwise it returns the refund amount. */
float cancelBookingRecord(int bookingIndex, int hoursRemaining)
{
    int showId;
    int row;
    int col;
    int customerId;
    int customerIndex;
    int movieId;
    float refund;

    if (bookingIndex < 0 || bookingIndex >= bookingCount ||
        !bookings[bookingIndex].active)
        return -1.0f;

    /* Copy the values before wait-list promotion, because the booking array
       may grow and move to a new memory location. */
    showId = bookings[bookingIndex].showId;
    row = bookings[bookingIndex].row;
    col = bookings[bookingIndex].col;
    customerId = bookings[bookingIndex].customerId;
    movieId = shows[showId].movieId;
    refund = calculateRefund(bookings[bookingIndex].totalPaid, hoursRemaining);

    bookings[bookingIndex].active = 0;
    freeSeat(showId, row, col);
    updateRevenue(movieId, -refund);

    /* Recalculate the customer's points after cancelling the booking. */
    for (customerIndex = 0; customerIndex < customerCount; customerIndex++)
        if (customers[customerIndex].id == customerId)
        {
            updateCustomerPoints(&customers[customerIndex].points,
                                 calcLoyaltyPoints(customerId, bookingCount - 1));
            break;
        }

    /* Give the freed seat to the first customer in the queue, if any. */
    promoteFromWaitList(showId, row, col);
    return refund;
}

void cancelBooking(void)
{
    char bookingId[20];
    int index;
    int hours;
    int confirm;
    float refund;
    char savedId[12];

    printHeader("CANCEL A BOOKING");
    getText("Enter booking ID: ", bookingId, sizeof(bookingId));
    if (inputEnded)
        return;

    index = findBookingById(bookingId);
    if (index < 0 || !bookings[index].active)
    {
        printf("Active booking not found. Check the booking ID.\n");
        return;
    }

    hours = hoursUntilShow(bookings[index].showId);
    refund = calculateRefund(bookings[index].totalPaid, hours);
    strcpy(savedId, bookings[index].id);

    printf("Hours remaining before show: %d\n", hours);
    printf("Original payment: Rs. %.2f\n", bookings[index].totalPaid);
    printf("Refund amount: Rs. %.2f\n", refund);
    confirm = getIntInRange("Confirm cancellation? 1. Yes  0. No: ", 0, 1);
    if (confirm != 1 || inputEnded)
    {
        printf("Cancellation not made.\n");
        return;
    }

    refund = cancelBookingRecord(index, hours);
    if (refund < 0.0f)
    {
        printf("The booking could not be cancelled.\n");
        return;
    }

    printf("Booking %s cancelled successfully. Refund: Rs. %.2f\n", savedId, refund);
}

/* Recursive accumulation: each active booking adds one point per Rs. 100. */
int calcLoyaltyPoints(int customerId, int index)
{
    int points;

    if (index < 0)
        return 0;

    points = 0;
    if (bookings[index].customerId == customerId && bookings[index].active)
        points = (int)(bookings[index].totalPaid / 100.0f);

    return points + calcLoyaltyPoints(customerId, index - 1);
}

int getLoyaltyTier(int points)
{
    if (points >= 50)
        return 2;               /* Gold */
    else if (points >= 20)
        return 1;               /* Silver */
    else
        return 0;               /* Bronze */
}

/* Pointer parameter updates the original customer's points. */
void updateCustomerPoints(int *points, int newPoints)
{
    if (points != NULL)
        *points = newPoints;
}

void viewCustomerProfile(void)
{
    char phone[16];
    int index;
    int points;
    int i;
    int tier;

    printHeader("CUSTOMER PROFILE AND LOYALTY");
    getPhone("Enter registered phone number: ", phone);
    if (inputEnded)
        return;

    index = findCustomerByPhone(phone);
    if (index < 0)
    {
        printf("Customer not found.\n");
        return;
    }

    points = calcLoyaltyPoints(customers[index].id, bookingCount - 1);
    updateCustomerPoints(&customers[index].points, points);
    tier = getLoyaltyTier(points);

    printf("Name: %s\nPhone: %s\nPoints: %d\n",
           customers[index].name, customers[index].phone, points);
    if (tier == 2)
        printf("Loyalty tier: GOLD\n");
    else if (tier == 1)
        printf("Loyalty tier: SILVER\n");
    else
        printf("Loyalty tier: BRONZE\n");

    printf("\nBooking history:\n");
    for (i = 0; i < bookingCount; i++)
        if (bookings[i].customerId == customers[index].id)
            printf("%s | Show %d | Seat %c%d | %s | Rs. %.2f\n",
                   bookings[i].id, bookings[i].showId,
                   'A' + bookings[i].row, bookings[i].col + 1,
                   bookings[i].active ? "ACTIVE" : "CANCELLED",
                   bookings[i].totalPaid);
}
