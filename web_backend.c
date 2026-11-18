/*
   CINEMA MULTIPLEX - LOCAL WEB API BRIDGE
   ------------------------------------------------------------
   This small file exposes the existing beginner-friendly C backend
   to the local HTML/CSS/JavaScript website.

   IMPORTANT:
   - The business rules remain in the existing backend C modules.
   - This file only validates web form values, calls those functions,
     and returns JSON for the browser.
   - The Python standard-library server loads this file as a DLL.
*/
#include "backend/project.h"
#include <stdarg.h>

#ifdef _WIN32
#define API __declspec(dllexport)
#else
#define API __attribute__((visibility("default")))
#endif

/* The same shared arrays and counters used by backend/main.c. */
Movie movies[MAX_MOVIES];
Screen screens[MAX_SCREENS];
Show shows[MAX_SHOWS];
Seat seats[MAX_SCREENS][MAX_SHOWS][ROWS][COLS];
Customer customers[MAX_CUSTOMERS];
FoodCombo combos[MAX_COMBOS];
char promoCodes[MAX_PROMOS][20];
float promoPercent[MAX_PROMOS];
Booking *bookings = NULL;
WaitEntry *waitList = NULL;
int movieCount = 0, showCount = 0, customerCount = 0;
int bookingCount = 0, bookingCapacity = 0, waitCount = 0, waitCapacity = 0;
int nextBookingNumber = 1, inputEnded = 0;

/* Include the same modules used by the console build. */
#include "backend/input.c"
#include "backend/memory.c"
#include "backend/movies.c"
#include "backend/shows.c"
#include "backend/seats.c"
#include "backend/pricing.c"
#include "backend/customers.c"
#include "backend/storage.c"
#include "backend/menus.c"

#define JSON_CAPACITY (2 * 1024 * 1024)
static char jsonBuffer[JSON_CAPACITY];
static size_t jsonLength = 0;
static int initialized = 0;

static void jsonReset(void)
{
    jsonLength = 0;
    jsonBuffer[0] = '\0';
}

static void jsonAdd(const char *format, ...)
{
    va_list args;
    int written;
    if (jsonLength >= JSON_CAPACITY - 1) return;
    va_start(args, format);
    written = vsnprintf(jsonBuffer + jsonLength,
                        JSON_CAPACITY - jsonLength, format, args);
    va_end(args);
    if (written > 0)
    {
        size_t amount = (size_t)written;
        if (amount >= JSON_CAPACITY - jsonLength)
            jsonLength = JSON_CAPACITY - 1;
        else
            jsonLength += amount;
    }
}

static void jsonString(const char *value)
{
    const unsigned char *p = (const unsigned char *)(value ? value : "");
    jsonAdd("\"");
    while (*p && jsonLength < JSON_CAPACITY - 8)
    {
        if (*p == '"' || *p == '\\')
        {
            jsonAdd("\\%c", *p);
        }
        else if (*p == '\n') jsonAdd("\\n");
        else if (*p == '\r') jsonAdd("\\r");
        else if (*p == '\t') jsonAdd("\\t");
        else if (*p < 32) jsonAdd(" ");
        else jsonAdd("%c", *p);
        p++;
    }
    jsonAdd("\"");
}

static int validText(const char *text, int maxLength)
{
    int i;
    if (!text || !text[0] || (int)strlen(text) > maxLength) return 0;
    for (i = 0; text[i]; i++)
        if (text[i] == '|' || text[i] == '\n' || text[i] == '\r') return 0;
    return 1;
}

static int validPhone(const char *phone)
{
    int i, length;
    if (!phone) return 0;
    length = (int)strlen(phone);
    if (length < 7 || length > 15) return 0;
    for (i = 0; i < length; i++)
        if (!isdigit((unsigned char)phone[i])) return 0;
    return 1;
}

static int customerIndexById(int id)
{
    int i;
    for (i = 0; i < customerCount; i++)
        if (customers[i].id == id) return i;
    return -1;
}

API int api_init(void)
{
    int i;
    if (initialized) return 1;

    for (i = 0; i < MAX_SCREENS; i++)
    {
        screens[i].id = i;
        screens[i].rows = ROWS;
        screens[i].cols = COLS;
        sprintf(screens[i].name, "Screen %d", i + 1);
    }

    initAllSeats();
    initCombos();
    if (!initDynamicLists()) return 0;

    /* Existing save files are loaded from the application folder. */
    if (!loadAll()) loadSampleMovies();
    initialized = 1;
    return 1;
}

API const char *api_get_state(void)
{
    int i, j, r, c, active, tier, count;
    char timeText[6];
    api_init();
    jsonReset();

    jsonAdd("{\"ok\":true,\"meta\":{\"screenCount\":%d,\"rows\":%d,\"cols\":%d,\"cleaningMinutes\":%d,\"movieCount\":%d,\"showCount\":%d,\"customerCount\":%d,\"bookingCount\":%d},",
            MAX_SCREENS, ROWS, COLS, CLEANING_MINUTES,
            movieCount, showCount, customerCount, bookingCount);

    jsonAdd("\"movies\":[");
    for (i = 0; i < movieCount; i++)
    {
        if (i) jsonAdd(",");
        jsonAdd("{\"id\":%d,\"title\":", movies[i].id);
        jsonString(movies[i].title);
        jsonAdd(",\"duration\":%d,\"revenue\":%.2f}",
                movies[i].duration, movies[i].revenue);
    }
    jsonAdd("],\"shows\":[");
    for (i = 0; i < showCount; i++)
    {
        if (i) jsonAdd(",");
        minutesToText(shows[i].startMinutes, timeText);
        active = countBookedSeats(i);
        jsonAdd("{\"id\":%d,\"movieId\":%d,\"movieTitle\":",
                shows[i].id, shows[i].movieId);
        jsonString(movies[shows[i].movieId].title);
        jsonAdd(",\"screenId\":%d,\"date\":", shows[i].screenId);
        jsonString(shows[i].date);
        jsonAdd(",\"time\":"); jsonString(timeText);
        jsonAdd(",\"minutes\":%d,\"dayType\":%d,\"duration\":%d,\"booked\":%d,\"total\":%d,\"occupancy\":%.1f,\"prices\":[%.2f,%.2f,%.2f],\"seats\":[",
                shows[i].startMinutes, shows[i].dayType, shows[i].duration,
                active, ROWS * COLS, getOccupancy(i),
                getTicketPrice(i, 1), getTicketPrice(i, 2), getTicketPrice(i, 3));
        for (r = 0; r < ROWS; r++)
        {
            if (r) jsonAdd(",");
            jsonAdd("[");
            for (c = 0; c < COLS; c++)
            {
                if (c) jsonAdd(",");
                jsonAdd("%d", seats[shows[i].screenId][i][r][c].status);
            }
            jsonAdd("]");
        }
        jsonAdd("]}");
    }

    jsonAdd("],\"customers\":[");
    for (i = 0; i < customerCount; i++)
    {
        if (i) jsonAdd(",");
        count = 0;
        for (j = 0; j < bookingCount; j++)
            if (bookings[j].customerId == customers[i].id && bookings[j].active) count++;
        tier = getLoyaltyTier(customers[i].points);
        jsonAdd("{\"id\":%d,\"name\":", customers[i].id);
        jsonString(customers[i].name);
        jsonAdd(",\"phone\":"); jsonString(customers[i].phone);
        jsonAdd(",\"points\":%d,\"tier\":", customers[i].points);
        jsonString(tier == 2 ? "Gold" : (tier == 1 ? "Silver" : "Bronze"));
        jsonAdd(",\"activeBookings\":%d}", count);
    }

    jsonAdd("],\"bookings\":[");
    for (i = 0; i < bookingCount; i++)
    {
        Show *s = &shows[bookings[i].showId];
        int ci = customerIndexById(bookings[i].customerId);
        if (i) jsonAdd(",");
        minutesToText(s->startMinutes, timeText);
        jsonAdd("{\"id\":"); jsonString(bookings[i].id);
        jsonAdd(",\"customerId\":%d,\"customerName\":", bookings[i].customerId);
        jsonString(ci >= 0 ? customers[ci].name : "Unknown customer");
        jsonAdd(",\"showId\":%d,\"movieTitle\":", bookings[i].showId);
        jsonString(movies[s->movieId].title);
        jsonAdd(",\"date\":"); jsonString(s->date);
        jsonAdd(",\"time\":"); jsonString(timeText);
        jsonAdd(",\"screen\":%d,\"seat\":", s->screenId + 1);
        { char seatName[32]; snprintf(seatName, sizeof(seatName), "%c%d", 'A' + bookings[i].row, bookings[i].col + 1); jsonString(seatName); }
        jsonAdd(",\"ticketPrice\":%.2f,\"foodCost\":%.2f,\"discount\":%.2f,\"total\":%.2f,\"active\":%s}",
                bookings[i].ticketPrice, bookings[i].foodCost,
                bookings[i].discount, bookings[i].totalPaid,
                bookings[i].active ? "true" : "false");
    }

    jsonAdd("],\"waitlist\":[");
    for (i = 0; i < waitCount; i++)
    {
        int ci = customerIndexById(waitList[i].customerId);
        if (i) jsonAdd(",");
        jsonAdd("{\"customerId\":%d,\"showId\":%d,\"customerName\":",
                waitList[i].customerId, waitList[i].showId);
        jsonString(ci >= 0 ? customers[ci].name : "Unknown customer");
        jsonAdd("}");
    }

    jsonAdd("],\"combos\":[");
    for (i = 0; i < MAX_COMBOS; i++)
    {
        if (i) jsonAdd(",");
        jsonAdd("{\"id\":%d,\"name\":", combos[i].id);
        jsonString(combos[i].name);
        jsonAdd(",\"price\":%.2f}", combos[i].price);
    }
    jsonAdd("],\"promos\":[");
    for (i = 0; i < MAX_PROMOS; i++)
    {
        if (i) jsonAdd(",");
        jsonAdd("{\"code\":"); jsonString(promoCodes[i]);
        jsonAdd(",\"percent\":%.1f}", promoPercent[i]);
    }
    jsonAdd("]}");
    return jsonBuffer;
}

API const char *api_add_movie(const char *title, int duration)
{
    api_init(); jsonReset();
    if (!validText(title, 49))
        return "{\"ok\":false,\"message\":\"Enter a valid movie title (maximum 49 characters).\"}";
    if (duration < 30 || duration > 300)
        return "{\"ok\":false,\"message\":\"Movie duration must be between 30 and 300 minutes.\"}";
    if (movieCount >= MAX_MOVIES)
        return "{\"ok\":false,\"message\":\"Movie catalogue is full.\"}";
    movies[movieCount].id = movieCount;
    strcpy(movies[movieCount].title, title);
    movies[movieCount].duration = duration;
    movies[movieCount].revenue = 0.0f;
    movieCount++;
    saveAll();
    jsonAdd("{\"ok\":true,\"message\":\"Movie added successfully.\",\"id\":%d}", movieCount - 1);
    return jsonBuffer;
}

API const char *api_register_customer(const char *name, const char *phone)
{
    api_init(); jsonReset();
    if (!validText(name, 39))
        return "{\"ok\":false,\"message\":\"Enter a valid customer name.\"}";
    if (!validPhone(phone))
        return "{\"ok\":false,\"message\":\"Phone number must contain 7 to 15 digits.\"}";
    if (isDuplicatePhone(phone))
        return "{\"ok\":false,\"message\":\"This phone number is already registered.\"}";
    if (customerCount >= MAX_CUSTOMERS)
        return "{\"ok\":false,\"message\":\"Customer list is full.\"}";
    customers[customerCount].id = customerCount + 1;
    strcpy(customers[customerCount].name, name);
    strcpy(customers[customerCount].phone, phone);
    customers[customerCount].points = 0;
    customerCount++;
    saveAll();
    jsonAdd("{\"ok\":true,\"message\":\"Customer registered successfully.\",\"id\":%d}", customerCount);
    return jsonBuffer;
}

API const char *api_add_show(int movieId, const char *date, int startMinutes,
                             int dayType, int preferredScreen)
{
    int id, nextScreen = -1, nextMinutes = -1;
    char nextText[6];
    api_init(); jsonReset();
    if (movieId < 0 || movieId >= movieCount || !date || dayType < 0 || dayType > 1 ||
        startMinutes < 0 || startMinutes >= 1440 || preferredScreen < -1 || preferredScreen >= MAX_SCREENS)
        return "{\"ok\":false,\"message\":\"Please check the movie, date, time and screen values.\"}";
    id = createShow(movieId, date, startMinutes, dayType, preferredScreen);
    if (id >= 0)
    {
        saveAll();
        jsonAdd("{\"ok\":true,\"message\":\"Show scheduled successfully.\",\"id\":%d,\"screen\":%d}", id, shows[id].screenId + 1);
        return jsonBuffer;
    }
    jsonAdd("{\"ok\":false,\"message\":\"That time is unavailable. The cinema keeps exactly three physical screens.\"");
    if (findNextAvailableTime(movieId, date, startMinutes, &nextScreen, &nextMinutes))
    {
        minutesToText(nextMinutes, nextText);
        jsonAdd(",\"suggestion\":{\"screen\":%d,\"minutes\":%d,\"time\":", nextScreen + 1, nextMinutes);
        jsonString(nextText);
        jsonAdd("}");
    }
    jsonAdd("}");
    return jsonBuffer;
}

API const char *api_book(int customerId, int showId, int row, int col,
                         int comboId, int quantity, const char *promoCode)
{
    int ci, index;
    float foodCost = 0.0f, promo = 0.0f;
    api_init(); jsonReset();
    ci = customerIndexById(customerId);
    if (ci < 0) return "{\"ok\":false,\"message\":\"Please select a registered customer.\"}";
    if (showId < 0 || showId >= showCount) return "{\"ok\":false,\"message\":\"Please select a valid show.\"}";
    if (!isValidSeat(row, col)) return "{\"ok\":false,\"message\":\"That seat is outside the seat map.\"}";
    if (!isSeatFree(showId, row, col)) return "{\"ok\":false,\"message\":\"That seat has already been booked. Please choose another.\"}";
    if (comboId < 0 || comboId > MAX_COMBOS || quantity < 0 || quantity > 10)
        return "{\"ok\":false,\"message\":\"Invalid food combo selection.\"}";
    if (comboId == 0) quantity = 0;
    if (comboId > 0 && quantity < 1)
        return "{\"ok\":false,\"message\":\"Choose a quantity from 1 to 10.\"}";
    if (comboId > 0) foodCost = combos[comboId - 1].price * quantity;
    if (promoCode && promoCode[0] && strcmp(promoCode, "NONE") != 0)
    {
        promo = getPromoPercent(promoCode);
        if (promo < 0.0f) return "{\"ok\":false,\"message\":\"Invalid promo code. Try SAVE10, CINEMA20 or STUDENT15.\"}";
    }
    index = createBooking(customerId, showId, row, col, foodCost, promo);
    if (index < 0)
        return "{\"ok\":false,\"message\":\"Booking could not be completed. Check the seat and show availability.\"}";
    saveAll();
    jsonAdd("{\"ok\":true,\"message\":\"Your booking is confirmed.\",\"bookingId\":");
    jsonString(bookings[index].id);
    jsonAdd(",\"total\":%.2f,\"ticket\":%.2f,\"food\":%.2f,\"discount\":%.2f}",
            bookings[index].totalPaid, bookings[index].ticketPrice,
            bookings[index].foodCost, bookings[index].discount);
    return jsonBuffer;
}

API const char *api_join_waitlist(int customerId, int showId)
{
    api_init(); jsonReset();
    if (customerIndexById(customerId) < 0 || showId < 0 || showId >= showCount)
        return "{\"ok\":false,\"message\":\"Select a valid customer and show.\"}";
    if (!isShowSoldOut(showId))
        return "{\"ok\":false,\"message\":\"The waiting list is available only when the show is sold out.\"}";
    if (!addToWaitList(customerId, showId))
        return "{\"ok\":false,\"message\":\"Could not add customer. They may already be in the queue.\"}";
    saveAll();
    jsonAdd("{\"ok\":true,\"message\":\"Added to the waiting list.\",\"position\":%d}", waitCount);
    return jsonBuffer;
}

API const char *api_cancel(const char *bookingId)
{
    int index, hours;
    float refund;
    api_init(); jsonReset();
    if (!bookingId || !bookingId[0])
        return "{\"ok\":false,\"message\":\"Enter a booking ID.\"}";
    index = findBookingById(bookingId);
    if (index < 0 || !bookings[index].active)
        return "{\"ok\":false,\"message\":\"Active booking not found.\"}";
    hours = hoursUntilShow(bookings[index].showId);
    refund = cancelBookingRecord(index, hours);
    if (refund < 0.0f) return "{\"ok\":false,\"message\":\"Cancellation failed.\"}";
    saveAll();
    jsonAdd("{\"ok\":true,\"message\":\"Booking cancelled. Any eligible waiting-list customer was promoted automatically.\",\"refund\":%.2f,\"hoursRemaining\":%d}", refund, hours);
    return jsonBuffer;
}

API const char *api_save(void)
{
    api_init();
    saveAll();
    return "{\"ok\":true,\"message\":\"All cinema records have been saved.\"}";
}

API void api_shutdown(void)
{
    if (initialized)
    {
        saveAll();
        free(bookings); bookings = NULL;
        free(waitList); waitList = NULL;
        initialized = 0;
    }
}
