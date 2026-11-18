/* INPUT.C - Safe input helpers used by every menu and form. */

/* Read one complete line. This avoids the common scanf/fgets problems. */
static int readLine(char text[], int size)
{
    size_t length;

    if (fgets(text, size, stdin) == NULL)
    {
        inputEnded = 1;
        return 0;
    }

    length = strlen(text);
    if (length > 0 && text[length - 1] == '\n')
    {
        text[length - 1] = '\0';
    }
    else
    {
        /* The buffer may be exactly full. Read one more character to see
           whether it was just the Enter key or whether the input was longer. */
        int ch = getchar();
        if (ch != '\n' && ch != EOF)
        {
            while (ch != '\n' && ch != EOF)
                ch = getchar();
            text[0] = '\0';
            printf("Input was too long. Please try again.\n");
        }
    }

    /* Remove leading and trailing spaces so a blank name is not accepted. */
    {
        size_t start = 0;
        size_t end = strlen(text);
        while (start < end && isspace((unsigned char)text[start]))
            start++;
        while (end > start && isspace((unsigned char)text[end - 1]))
            end--;
        if (start > 0)
            memmove(text, text + start, end - start);
        text[end - start] = '\0';
    }
    return 1;
}

/* Read an integer and repeat until it is inside the requested range. */
int getIntInRange(const char prompt[], int min, int max)
{
    char line[100];
    char *end;
    long value;

    while (!inputEnded)
    {
        printf("%s", prompt);
        if (!readLine(line, sizeof(line)))
            return min;

        value = strtol(line, &end, 10);
        while (*end != '\0' && isspace((unsigned char)*end))
            end++;

        if (line[0] == '\0' || end == line || *end != '\0')
        {
            printf("Please enter a whole number.\n");
        }
        else if (value < min || value > max)
        {
            printf("Please enter a value from %d to %d.\n", min, max);
        }
        else
        {
            return (int)value;
        }
    }
    return min;
}

/* Read a decimal number and check its range. */
float getFloatInRange(const char prompt[], float min, float max)
{
    char line[100];
    char *end;
    double value;

    while (!inputEnded)
    {
        printf("%s", prompt);
        if (!readLine(line, sizeof(line)))
            return min;

        value = strtod(line, &end);
        while (*end != '\0' && isspace((unsigned char)*end))
            end++;

        if (line[0] == '\0' || end == line || *end != '\0' || !isfinite(value))
            printf("Please enter a valid number.\n");
        else if (value < min || value > max)
            printf("Please enter a value from %.2f to %.2f.\n", min, max);
        else
            return (float)value;
    }
    return min;
}

/* Read a non-empty line. The pipe character is not allowed because it is
   used as the separator in our simple text save files. */
void getText(const char prompt[], char text[], int size)
{
    int i;
    int valid;

    do
    {
        printf("%s", prompt);
        if (!readLine(text, size))
        {
            text[0] = '\0';
            return;
        }

        valid = 1;
        if (text[0] == '\0')
            valid = 0;

        for (i = 0; text[i] != '\0'; i++)
        {
            if (text[i] == '|')
                valid = 0;
        }

        if (!valid)
            printf("Please enter some text. The | character is not allowed.\n");
    } while (!valid && !inputEnded);
}

/* Phone numbers are stored as text so a leading zero is not lost. */
void getPhone(const char prompt[], char phone[])
{
    int i;
    int valid;

    do
    {
        getText(prompt, phone, 16);
        valid = (strlen(phone) >= 7 && strlen(phone) <= 15);

        for (i = 0; phone[i] != '\0'; i++)
        {
            if (!isdigit((unsigned char)phone[i]))
                valid = 0;
        }

        if (!valid && !inputEnded)
            printf("Phone number must contain 7 to 15 digits only.\n");
    } while (!valid && !inputEnded);
}

/* Check a date and also reject impossible dates such as 2026-02-31. */
int parseDate(const char date[], struct tm *result)
{
    int year, month, day;
    struct tm value;
    time_t converted;

    if (strlen(date) != 10 || date[4] != '-' || date[7] != '-')
        return 0;

    if (sscanf(date, "%4d-%2d-%2d", &year, &month, &day) != 3)
        return 0;

    if (year < 2020 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31)
        return 0;

    memset(&value, 0, sizeof(value));
    value.tm_year = year - 1900;
    value.tm_mon = month - 1;
    value.tm_mday = day;
    value.tm_isdst = -1;

    converted = mktime(&value);
    if (converted == (time_t)-1)
        return 0;

    /* mktime adjusts invalid dates, so compare its result with the input. */
    if (value.tm_year != year - 1900 || value.tm_mon != month - 1 || value.tm_mday != day)
        return 0;

    if (result != NULL)
        *result = value;
    return 1;
}

void getDate(const char prompt[], char date[])
{
    struct tm checkDate;

    do
    {
        getText(prompt, date, 11);
        if (!parseDate(date, &checkDate) && !inputEnded)
            printf("Use a real date in YYYY-MM-DD format (example: 2026-12-05).\n");
        else
            break;
    } while (!inputEnded);
}

/* Read a 24-hour time such as 10:30 and return minutes after midnight. */
int getTimeMinutes(const char prompt[])
{
    char line[30];
    int hour, minute;
    char extra;

    while (!inputEnded)
    {
        printf("%s", prompt);
        if (!readLine(line, sizeof(line)))
            return 0;

        if (strlen(line) == 5 &&
            sscanf(line, "%2d:%2d%c", &hour, &minute, &extra) == 2 &&
            hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59)
        {
            return hour * 60 + minute;
        }
        printf("Enter time in 24-hour HH:MM format, for example 18:30.\n");
    }
    return 0;
}

void minutesToText(int minutes, char text[])
{
    int hour;
    int minute;

    /* Keep the values in a valid clock range before formatting the string. */
    if (minutes < 0)
        minutes = 0;
    if (minutes > 1439)
        minutes = 1439;

    hour = minutes / 60;
    minute = minutes % 60;
    snprintf(text, 6, "%02d:%02d", hour, minute);
}

/* Case-insensitive comparison for booking IDs, phone numbers and titles. */
int equalsIgnoreCase(const char a[], const char b[])
{
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0')
    {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
            return 0;
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

void toLowerText(char text[])
{
    int i;
    for (i = 0; text[i] != '\0'; i++)
        text[i] = (char)tolower((unsigned char)text[i]);
}
