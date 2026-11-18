/* PRICING.C - Ticket prices, food combos, receipts, sorting and searching. */

float getBasePrice(int category)
{
    switch (category)
    {
        case 1: return 500.0f;   /* Silver */
        case 2: return 800.0f;   /* Gold */
        case 3: return 1200.0f;  /* Platinum */
        default: return 0.0f;
    }
}

int getSurgeLevel(float occupancy)
{
    if (occupancy > 80.0f)
        return 2;               /* High demand: +25% */
    else if (occupancy > 50.0f)
        return 1;               /* Medium demand: +10% */
    else
        return 0;
}

/* Dynamic pricing uses the four requested factors:
   seat category, weekday/weekend, showtime and current demand. */
float getTicketPrice(int showId, int category)
{
    float price;
    float occupancy;
    int startTime;

    if (showId < 0 || showId >= showCount)
        return 0.0f;

    /* First choose the price for the seat category. */
    if (category == 1)
        price = 500.0f;
    else if (category == 2)
        price = 800.0f;
    else if (category == 3)
        price = 1200.0f;
    else
        return 0.0f;

    startTime = shows[showId].startMinutes;

    /* Nested conditions combine day type and showtime. */
    if (shows[showId].dayType == 1)
    {
        if (startTime < 12 * 60)
            price = price * 1.20f * 0.90f;  /* Weekend morning */
        else if (startTime >= 17 * 60)
            price = price * 1.20f * 1.15f;  /* Weekend evening */
        else
            price = price * 1.20f;         /* Weekend afternoon */
    }
    else
    {
        if (startTime < 12 * 60)
            price = price * 0.90f;         /* Weekday morning */
        else if (startTime >= 17 * 60)
            price = price * 1.15f;         /* Weekday evening */
        else
        {
            /* Weekday afternoon keeps the base category price. */
        }
    }

    /* Demand surge is applied after the day/time price. */
    occupancy = getOccupancy(showId);
    if (occupancy > 80.0f)
        price = price * 1.25f;
    else if (occupancy > 50.0f)
        price = price * 1.10f;

    return price;
}

void showPriceBoard(int showId)
{
    float occupancy;
    int surge;

    if (showId < 0 || showId >= showCount)
        return;

    occupancy = getOccupancy(showId);
    surge = getSurgeLevel(occupancy);

    printHeader("TICKET PRICE BOARD");
    printf("Movie: %s | Screen: %d | Date: %s\n",
           movies[shows[showId].movieId].title, shows[showId].screenId + 1,
           shows[showId].date);
    printf("Current occupancy: %.1f%%\n", occupancy);
    if (surge == 2)
        printf("Demand: HIGH (+25%%)\n");
    else if (surge == 1)
        printf("Demand: MEDIUM (+10%%)\n");
    else
        printf("Demand: NORMAL\n");

    printf("\nSilver (Rows A-B)   Rs. %.2f\n", getTicketPrice(showId, 1));
    printf("Gold (Rows C-D)     Rs. %.2f\n", getTicketPrice(showId, 2));
    printf("Platinum (Rows E-F) Rs. %.2f\n", getTicketPrice(showId, 3));
}

void initCombos(void)
{
    combos[0].id = 1;
    strcpy(combos[0].name, "Popcorn + Drink");
    combos[0].price = 350.0f;

    combos[1].id = 2;
    strcpy(combos[1].name, "Nachos Combo");
    combos[1].price = 450.0f;

    combos[2].id = 3;
    strcpy(combos[2].name, "Family Combo");
    combos[2].price = 900.0f;

    combos[3].id = 4;
    strcpy(combos[3].name, "Snack Box");
    combos[3].price = 300.0f;

    strcpy(promoCodes[0], "SAVE10");
    promoPercent[0] = 10.0f;
    strcpy(promoCodes[1], "CINEMA20");
    promoPercent[1] = 20.0f;
    strcpy(promoCodes[2], "STUDENT15");
    promoPercent[2] = 15.0f;
}

float selectFoodCombo(void)
{
    int choice;
    int quantity;
    float total;

    printHeader("FOOD AND DRINK COMBOS");
    printf("0. No food\n");
    printf("1. %-22s Rs. %.2f\n", combos[0].name, combos[0].price);
    printf("2. %-22s Rs. %.2f\n", combos[1].name, combos[1].price);
    printf("3. %-22s Rs. %.2f\n", combos[2].name, combos[2].price);
    printf("4. %-22s Rs. %.2f\n", combos[3].name, combos[3].price);

    choice = getIntInRange("Choose a combo: ", 0, MAX_COMBOS);
    if (choice == 0 || inputEnded)
        return 0.0f;

    quantity = getIntInRange("Quantity (1-10): ", 1, 10);
    if (inputEnded)
        return 0.0f;

    total = combos[choice - 1].price * quantity;
    printf("Food total: Rs. %.2f\n", total);
    return total;
}

/* Return the discount percentage, or -1 for an invalid code. */
float getPromoPercent(const char code[])
{
    int i;
    for (i = 0; i < MAX_PROMOS; i++)
    {
        if (equalsIgnoreCase(code, promoCodes[i]))
            return promoPercent[i];
    }
    return -1.0f;
}

void printReceipt(int bookingIndex)
{
    Booking *booking;
    Show *show;
    char timeText[6];

    if (bookingIndex < 0 || bookingIndex >= bookingCount)
        return;

    booking = &bookings[bookingIndex];
    show = &shows[booking->showId];
    minutesToText(show->startMinutes, timeText);

    printHeader("CINEMA MULTIPLEX - BOOKING RECEIPT");
    printf("Booking ID : %s\n", booking->id);
    printf("Customer ID: %d\n", booking->customerId);
    printf("Movie      : %s\n", movies[show->movieId].title);
    printf("Show       : %s at %s | Screen %d\n",
           show->date, timeText, show->screenId + 1);
    printf("Seat       : %c%d\n", 'A' + booking->row, booking->col + 1);
    printf("------------------------------------------\n");
    printf("Ticket price : Rs. %8.2f\n", booking->ticketPrice);
    printf("Food and drink: Rs. %8.2f\n", booking->foodCost);
    printf("Discount     : Rs. %8.2f\n", booking->discount);
    printf("------------------------------------------\n");
    printf("TOTAL PAID   : Rs. %8.2f\n", booking->totalPaid);
    printf("Thank you. Enjoy your movie!\n");
}

void updateRevenue(int movieId, float amount)
{
    int i;
    for (i = 0; i < movieCount; i++)
    {
        if (movies[i].id == movieId)
        {
            movies[i].revenue += amount;
            if (movies[i].revenue < 0.0f)
                movies[i].revenue = 0.0f;
            return;
        }
    }
}

/* Sort movie indexes, not the movie records. This keeps Movie IDs stable. */
void sortMoviesByRevenue(void)
{
    Movie sorted[MAX_MOVIES];
    Movie temp;
    int i, j;

    if (movieCount <= 0 || movieCount > MAX_MOVIES)
    {
        printf("No valid movies to sort.\n");
        return;
    }

    /* Sort a copy so the Movie IDs used by shows never change. */
    for (i = 0; i < movieCount; i++)
        sorted[i] = movies[i];

    for (i = 0; i < movieCount - 1; i++)
        for (j = 0; j < movieCount - i - 1; j++)
            if (sorted[j].revenue < sorted[j + 1].revenue)
            {
                temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }

    printHeader("MOVIES SORTED BY TOTAL REVENUE");
    for (i = 0; i < movieCount; i++)
        printf("%2d. %-30s Rs. %.2f\n", i + 1,
               sorted[i].title, sorted[i].revenue);
}

/* Sort show indexes by occupancy without changing their real IDs. */
void sortShowsByOccupancy(void)
{
    int order[MAX_SHOWS];
    int count = 0;
    int i, j, temp;

    for (i = 0; i < showCount; i++)
        if (shows[i].movieId >= 0)
            order[count++] = i;

    if (count == 0)
    {
        printf("No shows to sort.\n");
        return;
    }

    for (i = 0; i < count - 1; i++)
        for (j = 0; j < count - i - 1; j++)
            if (getOccupancy(order[j]) < getOccupancy(order[j + 1]))
            {
                temp = order[j];
                order[j] = order[j + 1];
                order[j + 1] = temp;
            }

    printHeader("SHOWS SORTED BY OCCUPANCY");
    for (i = 0; i < count; i++)
    {
        int id = order[i];
        printf("Show %-3d | Screen %d | %-24s | %5.1f%% full\n",
               id, shows[id].screenId + 1,
               movies[shows[id].movieId].title, getOccupancy(id));
    }
}

void searchBooking(void)
{
    char query[20];
    int index;

    getText("Enter booking ID (example BK0001): ", query, sizeof(query));
    if (inputEnded)
        return;

    index = findBookingById(query);
    if (index < 0)
        printf("No booking found with that ID.\n");
    else
    {
        printf("Booking %s | Customer %d | Show %d | Seat %c%d | %s | Rs. %.2f\n",
               bookings[index].id, bookings[index].customerId,
               bookings[index].showId, 'A' + bookings[index].row,
               bookings[index].col + 1,
               bookings[index].active ? "ACTIVE" : "CANCELLED",
               bookings[index].totalPaid);
    }
}

void searchCustomer(void)
{
    char phone[16];
    int index;

    getPhone("Enter customer phone number: ", phone);
    if (inputEnded)
        return;

    index = findCustomerByPhone(phone);
    if (index < 0)
        printf("No customer found with that phone number.\n");
    else
        printf("Customer %d | %s | Phone: %s | Loyalty points: %d\n",
               customers[index].id, customers[index].name,
               customers[index].phone, customers[index].points);
}

void searchMovie(void)
{
    char query[50];
    char title[50];
    int i, found = 0;

    getText("Enter all or part of a movie title: ", query, sizeof(query));
    if (inputEnded)
        return;
    toLowerText(query);

    for (i = 0; i < movieCount; i++)
    {
        strcpy(title, movies[i].title);
        toLowerText(title);
        if (strstr(title, query) != NULL)
        {
            printf("Movie %d: %s | %d minutes | Revenue Rs. %.2f\n",
                   movies[i].id, movies[i].title,
                   movies[i].duration, movies[i].revenue);
            found = 1;
        }
    }
    if (!found)
        printf("No movie title matched your search.\n");
}
