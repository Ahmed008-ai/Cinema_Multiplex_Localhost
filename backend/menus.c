/* MENUS.C - The user-facing console interface. */

void printHeader(const char title[])
{
    printf("\n+======================================================================+\n");
    printf("| %-68.68s |\n", title);
    printf("+======================================================================+\n");
}

void printMainMenu(void)
{
    int i;
    int activeBookings = 0;

    for (i = 0; i < bookingCount; i++)
        if (bookings[i].active)
            activeBookings++;

    printHeader("CINEMA MULTIPLEX MANAGEMENT SYSTEM");
    printf("|  PHYSICAL SCREENS: %-2d  |  SHOW CAPACITY: %-3d show records          |\n",
           MAX_SCREENS, MAX_SHOWS);
    printf("|  MOVIES: %-3d  SHOWS: %-3d  CUSTOMERS: %-3d  ACTIVE BOOKINGS: %-3d |\n",
           movieCount, showCount, customerCount, activeBookings);
    printf("+----------------------------------------------------------------------+\n");
    printf("|  1. ADMIN PANEL                                                      |\n");
    printf("|  2. BOX OFFICE                                                      |\n");
    printf("|  3. CUSTOMER PORTAL                                                 |\n");
    printf("|  4. SAVE ALL DATA                                                   |\n");
    printf("|  5. EXIT                                                            |\n");
    printf("+======================================================================+\n");
}

void adminMenu(void)
{
    int choice;
    int reportChoice;

    do
    {
        printHeader("ADMIN PANEL");
        printf("1. Add Movie\n");
        printf("2. Schedule a Show\n");
        printf("3. View Movies\n");
        printf("4. View Show Schedule\n");
        printf("5. Change Weekday / Weekend Type\n");
        printf("6. Reports and Sorting\n");
        printf("7. Back to Main Menu\n");
        choice = getIntInRange("Select an option: ", 1, 7);

        switch (choice)
        {
            case 1:
                addMovie();
                break;
            case 2:
                addShow();
                break;
            case 3:
                viewMovies();
                break;
            case 4:
                viewShows();
                break;
            case 5:
                setDayType();
                break;
            case 6:
                printf("\n1. Movies by total revenue\n2. Shows by occupancy\n");
                reportChoice = getIntInRange("Choose report: ", 1, 2);
                if (reportChoice == 1)
                    sortMoviesByRevenue();
                else
                    sortShowsByOccupancy();
                break;
            case 7:
                printf("Returning to main menu...\n");
                break;
        }
    } while (choice != 7 && !inputEnded);
}

void boxOfficeMenu(void)
{
    int choice;
    int showId;
    int searchChoice;

    do
    {
        printHeader("BOX OFFICE");
        printf("1. Book a Ticket\n");
        printf("2. Cancel a Booking\n");
        printf("3. View Seat Map\n");
        printf("4. Search Records\n");
        printf("5. View Waiting List\n");
        printf("6. View Bookings for a Show\n");
        printf("7. Back to Main Menu\n");
        choice = getIntInRange("Select an option: ", 1, 7);

        switch (choice)
        {
            case 1:
                bookSeat();
                break;
            case 2:
                cancelBooking();
                break;
            case 3:
                showId = selectShow();
                if (showId >= 0)
                    displaySeatMap(showId);
                break;
            case 4:
                printf("\n1. Search Booking ID\n2. Search Customer Phone\n3. Search Movie Title\n");
                searchChoice = getIntInRange("Choose search type: ", 1, 3);
                if (searchChoice == 1)
                    searchBooking();
                else if (searchChoice == 2)
                    searchCustomer();
                else
                    searchMovie();
                break;
            case 5:
                showId = selectShow();
                if (showId >= 0)
                    viewWaitList(showId);
                break;
            case 6:
                showId = selectShow();
                if (showId >= 0)
                    viewBookingsOfShow(showId);
                break;
            case 7:
                printf("Returning to main menu...\n");
                break;
        }
    } while (choice != 7 && !inputEnded);
}

void customerMenu(void)
{
    int choice;
    int showId;

    do
    {
        printHeader("CUSTOMER PORTAL");
        printf("1. Register Customer\n");
        printf("2. View Shows and Ticket Prices\n");
        printf("3. Book a Ticket\n");
        printf("4. My Profile and Loyalty Points\n");
        printf("5. Back to Main Menu\n");
        choice = getIntInRange("Select an option: ", 1, 5);

        switch (choice)
        {
            case 1:
                registerCustomer();
                break;
            case 2:
                showId = selectShow();
                if (showId >= 0)
                    showPriceBoard(showId);
                break;
            case 3:
                bookSeat();
                break;
            case 4:
                viewCustomerProfile();
                break;
            case 5:
                printf("Returning to main menu...\n");
                break;
        }
    } while (choice != 5 && !inputEnded);
}
