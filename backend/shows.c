/* SHOWS.C - Show scheduling for three fixed physical screens. */

/* Convert a show date and start time into the computer's time format. */
static time_t makeShowTime(const char date[], int startMinutes)
{
    struct tm value;

    if (!parseDate(date, &value))
        return (time_t)-1;

    value.tm_hour = startMinutes / 60;
    value.tm_min = startMinutes % 60;
    value.tm_sec = 0;
    value.tm_isdst = -1;
    return mktime(&value);
}

/* Check whether one physical screen is free for the whole movie.
   A 20-minute cleaning interval is kept between two shows. */
int isScreenAvailable(int screenId, const char date[], int startMinutes,
                      int duration)
{
    int i;
    time_t newStart;

    if (screenId < 0 || screenId >= MAX_SCREENS || duration <= 0)
        return 0;

    newStart = makeShowTime(date, startMinutes);
    if (newStart == (time_t)-1)
        return 0;

    for (i = 0; i < showCount; i++)
    {
        time_t oldStart;
        time_t oldEnd;
        time_t newEnd;

        if (shows[i].movieId < 0 || shows[i].screenId != screenId)
            continue;

        oldStart = makeShowTime(shows[i].date, shows[i].startMinutes);
        if (oldStart == (time_t)-1)
            continue;

        oldEnd = oldStart + (time_t)(shows[i].duration + CLEANING_MINUTES) * 60;
        newEnd = newStart + (time_t)(duration + CLEANING_MINUTES) * 60;

        /* If the new show starts before the old one, check its end.
           Otherwise check the old show's end. This keeps one cleaning gap. */
        if (newStart <= oldStart)
        {
            if (newEnd > oldStart)
                return 0;
        }
        else
        {
            if (oldEnd > newStart)
                return 0;
        }
    }
    return 1;
}

/* Create a show. preferredScreen = -1 means automatically choose a screen.
   Returns the new show ID, or -1 if no screen is available. */
int createShow(int movieId, const char date[], int startMinutes,
               int dayType, int preferredScreen)
{
    int screen;
    int selectedScreen = -1;
    Show *newShow;

    if (movieId < 0 || movieId >= movieCount || showCount >= MAX_SHOWS)
        return -1;
    if (!parseDate(date, NULL) || startMinutes < 0 || startMinutes >= 1440)
        return -1;
    if (makeShowTime(date, startMinutes) <= time(NULL))
        return -1;
    if (preferredScreen < -1 || preferredScreen >= MAX_SCREENS)
        return -1;
    if (dayType < 0 || dayType > 1)
        return -1;

    if (preferredScreen >= 0)
    {
        if (isScreenAvailable(preferredScreen, date, startMinutes, movies[movieId].duration))
            selectedScreen = preferredScreen;
    }
    else
    {
        for (screen = 0; screen < MAX_SCREENS; screen++)
        {
            if (isScreenAvailable(screen, date, startMinutes, movies[movieId].duration))
            {
                selectedScreen = screen;
                break;
            }
        }
    }

    if (selectedScreen == -1)
        return -1;

    newShow = &shows[showCount];
    newShow->id = showCount;
    newShow->screenId = selectedScreen;
    newShow->movieId = movieId;
    strcpy(newShow->date, date);
    newShow->startMinutes = startMinutes;
    newShow->dayType = dayType;
    newShow->duration = movies[movieId].duration;
    newShow->bookedSeats = 0;
    newShow->totalSeats = ROWS * COLS;

    initSeatsForShow(showCount);
    showCount++;
    return newShow->id;
}

/* Find the next 15-minute time on this date that has a free screen. */
int findNextAvailableTime(int movieId, const char date[], int startMinutes,
                          int *screenId, int *suggestedMinutes)
{
    int timeValue;
    int screen;

    if (movieId < 0 || movieId >= movieCount)
        return 0;

    for (timeValue = startMinutes + 15; timeValue <= 23 * 60 + 45; timeValue += 15)
    {
        if (makeShowTime(date, timeValue) <= time(NULL))
            continue;
        for (screen = 0; screen < MAX_SCREENS; screen++)
        {
            if (isScreenAvailable(screen, date, timeValue, movies[movieId].duration))
            {
                if (screenId != NULL)
                    *screenId = screen;
                if (suggestedMinutes != NULL)
                    *suggestedMinutes = timeValue;
                return 1;
            }
        }
    }
    return 0;
}

void addShow(void)
{
    int movieId;
    int dayType;
    int preferred;
    int showId;
    int screenId;
    int nextTime;
    int startMinutes;
    char date[11];
    char timeText[6];

    printHeader("SCHEDULE A MOVIE SHOW");
    if (movieCount == 0)
    {
        printf("Add a movie before scheduling a show.\n");
        return;
    }
    if (showCount >= MAX_SHOWS)
    {
        printf("The schedule has reached its %d-show project limit.\n", MAX_SHOWS);
        return;
    }

    viewMovies();
    movieId = getIntInRange("Select Movie ID: ", 0, movieCount - 1);
    if (inputEnded)
        return;

    getDate("Show date (YYYY-MM-DD): ", date);
    if (inputEnded)
        return;
    startMinutes = getTimeMinutes("Show start time (HH:MM, 24-hour clock): ");
    if (inputEnded)
        return;
    if (makeShowTime(date, startMinutes) <= time(NULL))
    {
        printf("Showtime must be in the future. Please choose a later date/time.\n");
        return;
    }

    printf("\nDay type: 1. Weekday   2. Weekend\n");
    dayType = getIntInRange("Select day type: ", 1, 2) - 1;
    if (inputEnded)
        return;

    printf("\nScreen selection: 0 = Automatic, 1-3 = Choose a screen\n");
    preferred = getIntInRange("Choose screen: ", 0, MAX_SCREENS);
    if (inputEnded)
        return;
    if (preferred > 0)
        preferred--;
    else
        preferred = -1;

    showId = createShow(movieId, date, startMinutes, dayType, preferred);
    if (showId >= 0)
    {
        minutesToText(startMinutes, timeText);
        printf("\nShow scheduled successfully!\n");
        printf("Show ID: %d | Screen: %d | Date: %s | Time: %s\n",
               showId, shows[showId].screenId + 1, date, timeText);
        return;
    }

    printf("\nThat screen/time is not available. No new screen was created.\n");
    if (findNextAvailableTime(movieId, date, startMinutes, &screenId, &nextTime))
    {
        minutesToText(nextTime, timeText);
        printf("Next available option: Screen %d at %s on %s.\n",
               screenId + 1, timeText, date);
        printf("Please add the show again using the suggested time.\n");
    }
    else
    {
        printf("No later showtime is available on this date. Try another date.\n");
    }
}

void viewShows(void)
{
    int i;
    char timeText[6];
    float occupancy;

    printHeader("DAILY SHOW SCHEDULE");
    if (showCount == 0)
    {
        printf("No shows are scheduled. Use Admin > Add Show to create one.\n");
        return;
    }

    printf("%-5s %-7s %-12s %-7s %-22s %-7s %-9s %-10s %-10s\n",
           "ID", "SCREEN", "DATE", "TIME", "MOVIE", "MINS", "DAY", "SEATS", "OCCUPANCY");
    printf("---------------------------------------------------------------------------------------------------------\n");

    for (i = 0; i < showCount; i++)
    {
        if (shows[i].movieId < 0)
            continue;
        minutesToText(shows[i].startMinutes, timeText);
        occupancy = getOccupancy(i);
        printf("%-5d %-7d %-12s %-7s %-22.22s %-7d %-9s %2d/%-6d %8.1f%%\n",
               shows[i].id, shows[i].screenId + 1, shows[i].date, timeText,
               movies[shows[i].movieId].title, shows[i].duration,
               shows[i].dayType == 1 ? "Weekend" : "Weekday",
               countBookedSeats(i), shows[i].totalSeats, occupancy);
    }
    printf("\nCleaning gap between shows: %d minutes. Physical screens: %d.\n",
           CLEANING_MINUTES, MAX_SCREENS);
}

int selectShow(void)
{
    int showId;

    if (showCount == 0)
    {
        printf("No shows are scheduled yet.\n");
        return -1;
    }

    viewShows();
    showId = getIntInRange("Enter Show ID: ", 0, showCount - 1);
    if (inputEnded || shows[showId].movieId < 0)
        return -1;
    return showId;
}

void setDayType(void)
{
    int showId;
    int choice;

    printHeader("CHANGE SHOW DAY TYPE");
    showId = selectShow();
    if (showId < 0)
        return;

    printf("1. Weekday\n2. Weekend\n");
    choice = getIntInRange("Choose day type: ", 1, 2);
    if (inputEnded)
        return;

    shows[showId].dayType = choice - 1;
    printf("Day type updated. Ticket prices will use the new day type.\n");
}
