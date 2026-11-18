/* STORAGE.C - Save and load the catalogue, schedule, seats and customers. */

void rebuildSeatsFromBookings(void)
{
    int i;
    int screen;

    initAllSeats();
    for (i = 0; i < showCount; i++)
        if (shows[i].movieId >= 0)
            initSeatsForShow(i);

    for (i = 0; i < bookingCount; i++)
    {
        if (bookings[i].active && bookings[i].showId >= 0 && bookings[i].showId < showCount &&
            isValidSeat(bookings[i].row, bookings[i].col))
        {
            screen = shows[bookings[i].showId].screenId;
            seats[screen][bookings[i].showId][bookings[i].row][bookings[i].col].status = 1;
        }
    }
}

/* Save each type of information in a small readable text file. */
void saveAll(void)
{
    FILE *file;
    int i, screen, row, col;
    int success = 1;

    file = fopen("movies.txt", "w");
    if (file == NULL)
    {
        printf("Could not write movies.txt.\n");
        success = 0;
    }
    else
    {
        fprintf(file, "%d\n", movieCount);
        for (i = 0; i < movieCount; i++)
            fprintf(file, "%d|%s|%d|%.2f\n", movies[i].id, movies[i].title,
                    movies[i].duration, movies[i].revenue);
        fclose(file);
    }

    file = fopen("shows.txt", "w");
    if (file == NULL)
    {
        printf("Could not write shows.txt.\n");
        success = 0;
    }
    else
    {
        fprintf(file, "%d\n", showCount);
        for (i = 0; i < showCount; i++)
            fprintf(file, "%d|%d|%d|%s|%d|%d|%d\n",
                    shows[i].id, shows[i].screenId, shows[i].movieId,
                    shows[i].date, shows[i].startMinutes,
                    shows[i].dayType, shows[i].duration);
        fclose(file);
    }

    file = fopen("seats.txt", "w");
    if (file == NULL)
    {
        printf("Could not write seats.txt.\n");
        success = 0;
    }
    else
    {
        fprintf(file, "%d\n", showCount);
        for (i = 0; i < showCount; i++)
        {
            if (shows[i].movieId < 0)
                continue;
            screen = shows[i].screenId;
            for (row = 0; row < ROWS; row++)
                for (col = 0; col < COLS; col++)
                    fprintf(file, "%d|%d|%d|%d|%d\n", screen, i, row, col,
                            seats[screen][i][row][col].status);
        }
        fclose(file);
    }

    file = fopen("customers.txt", "w");
    if (file == NULL)
    {
        printf("Could not write customers.txt.\n");
        success = 0;
    }
    else
    {
        fprintf(file, "%d\n", customerCount);
        for (i = 0; i < customerCount; i++)
            fprintf(file, "%d|%s|%s|%d\n", customers[i].id,
                    customers[i].name, customers[i].phone, customers[i].points);
        fclose(file);
    }

    file = fopen("bookings.txt", "w");
    if (file == NULL)
    {
        printf("Could not write bookings.txt.\n");
        success = 0;
    }
    else
    {
        fprintf(file, "%d|%d\n", bookingCount, nextBookingNumber);
        for (i = 0; i < bookingCount; i++)
            fprintf(file, "%s|%d|%d|%d|%d|%.2f|%.2f|%.2f|%.2f|%d\n",
                    bookings[i].id, bookings[i].customerId, bookings[i].showId,
                    bookings[i].row, bookings[i].col, bookings[i].ticketPrice,
                    bookings[i].foodCost, bookings[i].discount,
                    bookings[i].totalPaid, bookings[i].active);
        fclose(file);
    }

    file = fopen("waitlist.txt", "w");
    if (file == NULL)
    {
        printf("Could not write waitlist.txt.\n");
        success = 0;
    }
    else
    {
        fprintf(file, "%d\n", waitCount);
        for (i = 0; i < waitCount; i++)
            fprintf(file, "%d|%d\n", waitList[i].customerId, waitList[i].showId);
        fclose(file);
    }

    if (success)
        printf("\nAll cinema data saved successfully.\n");
    else
        printf("\nSome files could not be saved. Check the folder permissions.\n");
}

/* Load the saved files. A missing optional file is handled safely. */
int loadAll(void)
{
    FILE *file;
    int count;
    int i, screen, showId, row, col, status;
    int savedBookingNumber;

    file = fopen("movies.txt", "r");
    if (file == NULL)
    {
        printf("No movie data file found. Loading the sample movie catalogue.\n");
        return 0;
    }

    if (fscanf(file, "%d", &count) != 1 || count < 0 || count > MAX_MOVIES)
    {
        fclose(file);
        printf("movies.txt is damaged. Loading sample movies instead.\n");
        return 0;
    }

    movieCount = count;
    for (i = 0; i < movieCount; i++)
    {
        if (fscanf(file, " %d|%49[^|]|%d|%f", &movies[i].id,
                   movies[i].title, &movies[i].duration, &movies[i].revenue) != 4 ||
            movies[i].id != i || movies[i].duration < 30 || movies[i].duration > 300 ||
            !isfinite(movies[i].revenue) || movies[i].revenue < 0.0f)
        {
            fclose(file);
            movieCount = 0;
            printf("movies.txt contains invalid data. Loading sample movies.\n");
            return 0;
        }
    }
    fclose(file);

    /* Show file is optional on a first or partially saved run. */
    file = fopen("shows.txt", "r");
    if (file == NULL)
    {
        showCount = 0;
        printf("shows.txt is missing. Starting with an empty schedule.\n");
    }
    else
    {
        if (fscanf(file, "%d", &count) != 1 || count < 0 || count > MAX_SHOWS)
        {
            showCount = 0;
            printf("shows.txt is damaged. Starting with an empty schedule.\n");
        }
        else
        {
            showCount = count;
            for (i = 0; i < showCount; i++)
            {
                if (fscanf(file, " %d|%d|%d|%10[^|]|%d|%d|%d",
                           &shows[i].id, &shows[i].screenId, &shows[i].movieId,
                           shows[i].date, &shows[i].startMinutes,
                           &shows[i].dayType, &shows[i].duration) != 7 ||
                    shows[i].id != i || shows[i].screenId < 0 || shows[i].screenId >= MAX_SCREENS ||
                    shows[i].movieId < 0 || shows[i].movieId >= movieCount ||
                    shows[i].startMinutes < 0 || shows[i].startMinutes >= 1440 ||
                    shows[i].duration < 30 || shows[i].duration > 300 ||
                    shows[i].duration != movies[shows[i].movieId].duration ||
                    (shows[i].dayType != 0 && shows[i].dayType != 1) ||
                    !parseDate(shows[i].date, NULL))
                {
                    showCount = i;
                    printf("Invalid show record found. Remaining schedule records were ignored.\n");
                    break;
                }
                shows[i].bookedSeats = 0;
                shows[i].totalSeats = ROWS * COLS;
                initSeatsForShow(i);
            }
        }
        fclose(file);
    }

    /* Load customers. */
    file = fopen("customers.txt", "r");
    customerCount = 0;
    if (file != NULL)
    {
        if (fscanf(file, "%d", &count) == 1 && count >= 0 && count <= MAX_CUSTOMERS)
        {
            for (i = 0; i < count; i++)
            {
                Customer item;
                int phoneValid = 1;
                int phoneIndex;
                if (fscanf(file, " %d|%39[^|]|%15[^|]|%d", &item.id,
                           item.name, item.phone, &item.points) != 4)
                    break;
                if (item.id <= 0 || item.points < 0)
                    continue;
                if (strlen(item.phone) < 7 || strlen(item.phone) > 15)
                    phoneValid = 0;
                for (phoneIndex = 0; item.phone[phoneIndex] != '\0'; phoneIndex++)
                    if (!isdigit((unsigned char)item.phone[phoneIndex]))
                        phoneValid = 0;
                if (!phoneValid || isDuplicatePhone(item.phone))
                    continue;
                customers[customerCount++] = item;
            }
        }
        fclose(file);
    }
    else
        printf("customers.txt is missing. No saved customers were loaded.\n");

    /* Load bookings and restore the next booking number. */
    file = fopen("bookings.txt", "r");
    bookingCount = 0;
    if (file != NULL)
    {
        if (fscanf(file, "%d|%d", &count, &savedBookingNumber) == 2 &&
            count >= 0 && count <= 100000 && savedBookingNumber > 0)
        {
            while (bookingCapacity < count)
                if (!growBookings())
                    break;

            for (i = 0; i < count && bookingCount < bookingCapacity; i++)
            {
                Booking item;
                if (fscanf(file, " %11[^|]|%d|%d|%d|%d|%f|%f|%f|%f|%d",
                           item.id, &item.customerId, &item.showId, &item.row,
                           &item.col, &item.ticketPrice, &item.foodCost,
                           &item.discount, &item.totalPaid, &item.active) != 10)
                    break;
                int duplicateSeat = 0;
                int oldIndex;
                if (item.showId < 0 || item.showId >= showCount ||
                    !isValidSeat(item.row, item.col) ||
                    (item.active != 0 && item.active != 1) ||
                    !isfinite(item.ticketPrice) || !isfinite(item.foodCost) ||
                    !isfinite(item.discount) || !isfinite(item.totalPaid) ||
                    item.ticketPrice < 0.0f || item.foodCost < 0.0f ||
                    item.discount < 0.0f || item.totalPaid < 0.0f)
                    continue;
                {
                    int customerExists = 0;
                    int customerIndex;
                    for (customerIndex = 0; customerIndex < customerCount; customerIndex++)
                        if (customers[customerIndex].id == item.customerId)
                            customerExists = 1;
                    if (!customerExists)
                        continue;
                }
                if (item.active)
                    for (oldIndex = 0; oldIndex < bookingCount; oldIndex++)
                        if (bookings[oldIndex].active &&
                            bookings[oldIndex].showId == item.showId &&
                            bookings[oldIndex].row == item.row &&
                            bookings[oldIndex].col == item.col)
                            duplicateSeat = 1;
                if (!duplicateSeat)
                    bookings[bookingCount++] = item;
            }
            nextBookingNumber = savedBookingNumber;
        }
        fclose(file);
    }
    else
        printf("bookings.txt is missing. No saved bookings were loaded.\n");

    /* Load seat status. If it is missing, booking records rebuild the map. */
    initAllSeats();
    for (i = 0; i < showCount; i++)
        initSeatsForShow(i);

    file = fopen("seats.txt", "r");
    if (file != NULL)
    {
        if (fscanf(file, "%d", &count) == 1 && count >= 0 && count <= MAX_SHOWS)
        {
            /* Each row in this file stores one seat's screen/show/row/column. */
            while (fscanf(file, " %d|%d|%d|%d|%d", &screen, &showId,
                          &row, &col, &status) == 5)
            {
                if (screen >= 0 && screen < MAX_SCREENS &&
                    showId >= 0 && showId < showCount &&
                    shows[showId].screenId == screen && isValidSeat(row, col) &&
                    (status == 0 || status == 1))
                    seats[screen][showId][row][col].status = status;
            }
        }
        fclose(file);
    }
    else
        printf("seats.txt is missing. Seat states will be rebuilt from bookings.\n");

    /* Active bookings are the final authority: they must occupy their seats. */
    for (i = 0; i < bookingCount; i++)
        if (bookings[i].active)
        {
            screen = shows[bookings[i].showId].screenId;
            seats[screen][bookings[i].showId][bookings[i].row][bookings[i].col].status = 1;
        }

    for (i = 0; i < showCount; i++)
        countBookedSeats(i);

    /* Load waiting-list entries. */
    file = fopen("waitlist.txt", "r");
    waitCount = 0;
    if (file != NULL)
    {
        if (fscanf(file, "%d", &count) == 1 && count >= 0 && count <= 100000)
        {
            while (waitCapacity < count)
                if (!growWaitList())
                    break;
            for (i = 0; i < count && waitCount < waitCapacity; i++)
            {
                WaitEntry item;
                if (fscanf(file, " %d|%d", &item.customerId, &item.showId) != 2)
                    break;
                {
                    int customerExists = 0;
                    int customerIndex;
                    for (customerIndex = 0; customerIndex < customerCount; customerIndex++)
                        if (customers[customerIndex].id == item.customerId)
                            customerExists = 1;
                    if (customerExists && item.showId >= 0 && item.showId < showCount &&
                        isShowSoldOut(item.showId))
                        waitList[waitCount++] = item;
                }
            }
        }
        fclose(file);
    }

    /* Recalculate loyalty points so the saved profile matches active bookings. */
    for (i = 0; i < customerCount; i++)
        updateCustomerPoints(&customers[i].points,
                             calcLoyaltyPoints(customers[i].id, bookingCount - 1));

    printf("Saved cinema data loaded successfully.\n");
    return 1;
}
