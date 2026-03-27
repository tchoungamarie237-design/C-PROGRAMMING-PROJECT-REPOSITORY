#include <stdio.h>
#include <string.h>
#include "person.h"  // create a header for your struct

#define FILE_NAME "people.csv"

// Save a person to CSV
void save_person_to_csv(Person p) {
    FILE *file = fopen(FILE_NAME, "a"); // append mode

    if (file == NULL) {
        printf("Error opening file!\n");
        return;
    }

    fprintf(file, "%d,%s,%d,%s,%d\n",
            p.id,
            p.name,
            p.age,
            p.social_class,
            p.side);

    fclose(file);
}

// Load all persons (basic display)
void load_people_from_csv() {
    FILE *file = fopen(FILE_NAME, "r");

    if (file == NULL) {
        printf("No data file found.\n");
        return;
    }

    Person p;

    while (fscanf(file, "%d,%99[^,],%d,%49[^,],%d\n",
                  &p.id,
                  p.name,
                  &p.age,
                  p.social_class,
                  &p.side) == 5) {

        display_Person(p);
    }

    fclose(file);
}
