/* SEATS.C - Four-dimensional seat array and ticket booking functions. */

/* Prepare every seat in the 4-D array. Each show uses its own seat map. */
void initAllSeats(void)
{
    int screen, show, row, col;

    for (screen = 0; screen < MAX_SCREENS; screen++)
    {
        for (show = 0; show < MAX_SHOWS; show++)
        {
            for (row = 0; row < ROWS; row++)
            {
                for (col = 0; col < COLS; col++)
                {
                    seats[screen][show][row][col].status = 0;
                    if (row <= 1)
                        seats[screen][show][row][col].category = 1;
                    else if (row <= 3)
                        seats[screen][show][row][col].category = 2;
                    else
                        seats[screen][show][row][col].category = 3;
                }
            }
        }
    }
}

/* Clear the seat map for one newly created show. */
void initSeatsForShow(int showId)
{
    int row, col, screen;

    if (showId < 0 || showId >= MAX_SHOWS)
        return;
    screen = shows[showId].screenId;

    for (row = 0; row < ROWS; row++)
    {
        for (col = 0; col < COLS; col++)
        {
            seats[screen][showId][row][col].status = 0;
            if (row <= 1)
                seats[screen][showId][row][col].category = 1;
            else if (row <= 3)
                seats[screen][showId][row][col].category = 2;
            else
                seats[screen][showId][row][col].category = 3;
        }
    }
}

int isValidSeat(int row, int col)
{
    return row >= 0 && row < ROWS && col >= 0 && col < COLS;
}

int isSeatFree(int showId, int row, int col)
{
    int screen;

    if (showId < 0 || showId >= showCount || !isValidSeat(row, col))
        return 0;
    screen = shows[showId].screenId;
    return seats[screen][showId][row][col].status == 0;
}

/* Count seats from the matrix instead of trusting an old counter. */
int countBookedSeats(int showId)
{
    int row, col, count = 0;
    int screen;

    if (showId < 0 || showId >= showCount)
        return 0;

    screen = shows[showId].screenId;
    for (row = 0; row < ROWS; row++)
        for (col = 0; col < COLS; col++)
            if (seats[screen][showId][row][col].status == 1)
                count++;

    shows[showId].bookedSeats = count;
    return count;
}

float getOccupancy(int showId)
{
    int booked;
    if (showId < 0 || showId >= showCount || shows[showId].totalSeats <= 0)
        return 0.0f;

    booked = countBookedSeats(showId);
    return (float)booked * 100.0f / (float)shows[showId].totalSeats;
}

int isShowSoldOut(int showId)
{
    if (showId < 0 || showId >= showCount)
        return 0;
    return countBookedSeats(showId) >= shows[showId].totalSeats;
}

/* Pass the show ID and seat coordinates so the correct seat is updated. */
void freeSeat(int showId, int row, int col)
{
    int screen;

    if (showId < 0 || showId >= showCount || !isValidSeat(row, col))
        return;

    screen = shows[showId].screenId;
    seats[screen][showId][row][col].status = 0;
    countBookedSeats(showId);
}

/* Add a booking record to the growing booking array. */
int addBooking(int customerId, int showId, int row, int col,
               float ticketPrice, float foodCost, float discount)
{
    Booking *booking;
    float total;

    if (bookingCount >= bookingCapacity && !growBookings())
        return 0;
    if (bookings == NULL)
        return 0;

    booking = &bookings[bookingCount];
    sprintf(booking->id, "BK%04d", nextBookingNumber++);
    booking->customerId = customerId;
    booking->showId = showId;
    booking->row = row;
    booking->col = col;
    booking->ticketPrice = ticketPrice;
    booking->foodCost = foodCost;
    booking->discount = discount;
    total = ticketPrice + foodCost - discount;
    if (total < 0.0f)
        total = 0.0f;
    booking->totalPaid = total;
    booking->active = 1;

    bookingCount++;
    return 1;
}

/* This function is shared by the menu and the automated tests.
   It checks the seat, calculates the price, saves the booking and then
   marks the seat as booked. It returns the booking's array index. */
int createBooking(int customerId, int showId, int row, int col,
                  float foodCost, float promoPercentValue)
{
    int customerIndex = -1;
    int i;
    int category;
    float ticketPrice;
    float discount;
    int bookingIndex;

    if (showId < 0 || showId >= showCount || shows[showId].movieId < 0)
        return -1;
    if (!isValidSeat(row, col) || !isSeatFree(showId, row, col))
        return -1;
    if (foodCost < 0.0f || promoPercentValue < 0.0f || promoPercentValue > 100.0f)
        return -1;

    for (i = 0; i < customerCount; i++)
    {
        if (customers[i].id == customerId)
        {
            customerIndex = i;
            break;
        }
    }
    if (customerIndex == -1)
        return -1;

    category = seats[shows[showId].screenId][showId][row][col].category;
    ticketPrice = getTicketPrice(showId, category);
    discount = (ticketPrice + foodCost) * promoPercentValue / 100.0f;

    if (!addBooking(customerId, showId, row, col, ticketPrice, foodCost, discount))
        return -1;

    bookingIndex = bookingCount - 1;
    seats[shows[showId].screenId][showId][row][col].status = 1;
    countBookedSeats(showId);
    updateRevenue(shows[showId].movieId, bookings[bookingIndex].totalPaid);

    /* The recursive function totals points from active bookings. */
    updateCustomerPoints(&customers[customerIndex].points,
                         calcLoyaltyPoints(customerId, bookingCount - 1));
    return bookingIndex;
}

int findBookingById(const char bookingId[])
{
    int i;
    for (i = 0; i < bookingCount; i++)
    {
        if (equalsIgnoreCase(bookings[i].id, bookingId))
            return i;
    }
    return -1;
}

void viewBookingsOfShow(int showId)
{
    int i;
    int found = 0;

    printf("\nACTIVE BOOKINGS FOR SHOW %d\n", showId);
    for (i = 0; i < bookingCount; i++)
    {
        if (bookings[i].showId == showId && bookings[i].active)
        {
            printf("%-8s Customer %-4d Seat %c%d  Total Rs. %.2f\n",
                   bookings[i].id, bookings[i].customerId,
                   'A' + bookings[i].row, bookings[i].col + 1,
                   bookings[i].totalPaid);
            found = 1;
        }
    }
    if (!found)
        printf("No active bookings for this show.\n");
}

/* Show the seat map with clear row letters and 1-based column numbers. */
void displaySeatMap(int showId)
{
    int row, col, screen;
    char timeText[6];

    if (showId < 0 || showId >= showCount)
    {
        printf("Invalid show ID.\n");
        return;
    }

    screen = shows[showId].screenId;
    minutesToText(shows[showId].startMinutes, timeText);

    printHeader("LIVE SEAT MAP");
    printf("Movie: %s | Screen: %d | Date: %s | Time: %s\n\n",
           movies[shows[showId].movieId].title, screen + 1,
           shows[showId].date, timeText);
    printf("                         SCREEN\n");
    printf("                 ---------------------\n");
    printf("       ");
    for (col = 0; col < COLS; col++)
        printf(" %2d ", col + 1);
    printf("\n");

    for (row = 0; row < ROWS; row++)
    {
        printf("       %c  ", 'A' + row);
        for (col = 0; col < COLS; col++)
        {
            if (seats[screen][showId][row][col].status == 0)
                printf("[ ] ");
            else
                printf("[X] ");
        }
        if (row <= 1)
            printf(" Silver");
        else if (row <= 3)
            printf(" Gold");
        else
            printf(" Platinum");
        printf("\n");
    }
    printf("\n[ ] Available    [X] Booked\n");
    printf("Rows A-B: Silver | C-D: Gold | E-F: Platinum\n");
}

/* Interactive booking flow. */
void bookSeat(void)
{
    int showId;
    int row;
    int col;
    int customerIndex;
    int bookingIndex;
    char rowText[10];
    char phone[16];
    char promoCode[30];
    float foodCost;
    float promoValue;

    printHeader("BOOK A TICKET");
    showId = selectShow();
    if (showId < 0)
        return;

    if (isShowSoldOut(showId))
    {
        printf("This show is SOLD OUT.\n");
        printf("Would you like to join the waiting list? 1. Yes  0. No\n");
        if (getIntInRange("Choose: ", 0, 1) == 1)
        {
            getPhone("Registered customer phone: ", phone);
            customerIndex = findCustomerByPhone(phone);
            if (customerIndex < 0)
                printf("Customer not found. Please register first.\n");
            else if (addToWaitList(customers[customerIndex].id, showId))
                printf("You have been added to the waiting list.\n");
        }
        return;
    }

    displaySeatMap(showId);

    while (!inputEnded)
    {
        do
        {
            getText("Select row (A-F): ", rowText, sizeof(rowText));
            if (strlen(rowText) == 1)
            {
                row = toupper((unsigned char)rowText[0]) - 'A';
                if (isValidSeat(row, 0))
                    break;
            }
            printf("Please enter one row letter from A to F.\n");
        } while (!inputEnded);
        if (inputEnded)
            return;

        col = getIntInRange("Select seat number (1-8): ", 1, COLS) - 1;
        if (inputEnded)
            return;

        if (isSeatFree(showId, row, col))
            break;
        printf("That seat is already booked. Please choose another seat.\n");
    }
    if (inputEnded)
        return;

    getPhone("Customer phone number: ", phone);
    if (inputEnded)
        return;
    customerIndex = findCustomerByPhone(phone);
    if (customerIndex < 0)
    {
        printf("Customer not registered. Register through Customer Portal first.\n");
        return;
    }

    foodCost = selectFoodCombo();
    if (inputEnded)
        return;

    do
    {
        getText("Promo code (or NONE): ", promoCode, sizeof(promoCode));
        if (equalsIgnoreCase(promoCode, "NONE"))
        {
            promoValue = 0.0f;
            break;
        }
        promoValue = getPromoPercent(promoCode);
        if (promoValue < 0.0f)
            printf("Invalid promo code. Try again or enter NONE.\n");
    } while (promoValue < 0.0f && !inputEnded);
    if (inputEnded)
        return;

    bookingIndex = createBooking(customers[customerIndex].id, showId,
                                 row, col, foodCost, promoValue);
    if (bookingIndex < 0)
    {
        printf("Booking could not be completed. No seat has been reserved.\n");
        return;
    }

    printReceipt(bookingIndex);
}

