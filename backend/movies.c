/* MOVIES.C - Add, list and initialize the movie catalogue. */

void loadSampleMovies(void)
{
    /* Sample movies are provided, but no shows are pre-filled.
       The administrator creates showtimes for the actual cinema day. */
    movies[0].id = 0;
    strcpy(movies[0].title, "Inception");
    movies[0].duration = 148;
    movies[0].revenue = 0.0f;

    movies[1].id = 1;
    strcpy(movies[1].title, "Interstellar");
    movies[1].duration = 169;
    movies[1].revenue = 0.0f;

    movies[2].id = 2;
    strcpy(movies[2].title, "Joyland");
    movies[2].duration = 126;
    movies[2].revenue = 0.0f;

    movieCount = 3;
    printf("\nSample movie catalogue loaded. No showtimes have been booked.\n");
}

void addMovie(void)
{
    Movie *newMovie;

    if (movieCount >= MAX_MOVIES)
    {
        printf("Movie catalogue is full (maximum %d movies).\n", MAX_MOVIES);
        return;
    }

    newMovie = &movies[movieCount];
    newMovie->id = movieCount;
    getText("Enter movie title: ", newMovie->title, sizeof(newMovie->title));
    if (inputEnded)
        return;

    newMovie->duration = getIntInRange("Movie duration in minutes (30-300): ", 30, 300);
    if (inputEnded)
        return;

    newMovie->revenue = 0.0f;
    movieCount++;
    printf("\nMovie added successfully. Movie ID: %d\n", newMovie->id);
}

void viewMovies(void)
{
    int i;

    printHeader("MOVIE CATALOGUE");
    if (movieCount == 0)
    {
        printf("No movies have been added yet.\n");
        return;
    }

    printf("%-5s %-32s %-12s %12s\n", "ID", "MOVIE TITLE", "DURATION", "REVENUE");
    printf("--------------------------------------------------------------------\n");
    for (i = 0; i < movieCount; i++)
    {
        printf("%-5d %-32.32s %-12d Rs. %9.2f\n",
               movies[i].id, movies[i].title, movies[i].duration, movies[i].revenue);
    }
}
