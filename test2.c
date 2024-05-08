#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATH_LENGTH 256

int main()
{
    FILE *file = fopen("script.conf", "r");
    if (file == NULL)
    {
        fprintf(stderr, "Impossible d'ouvrir le fichier.\n");
        return 1;
    }

    char type[MAX_PATH_LENGTH];
    char pathname[MAX_PATH_LENGTH];

    // Lecture du type
    if (fgets(type, MAX_PATH_LENGTH, file) == NULL)
    {
        fprintf(stderr, "Erreur lors de la lecture du type.\n");
        fclose(file);
        return 1;
    }

    // Supprimer le retour à la ligne de la fin du type
    type[strcspn(type, "\r\n")] = '\0';

    // Vérification du type
    if (strcmp(type, "int") != 0)
    {
        fprintf(stderr, "Type non reconnu : %s\n", type);
        fclose(file);
        return 1;
    }

    // Lecture des pathnames
    while (fgets(pathname, MAX_PATH_LENGTH, file) != NULL)
    {
        // Supprimer le retour à la ligne de la fin du pathname
        pathname[strcspn(pathname, "\r\n")] = '\0';
        printf("Pathname : %s\n", pathname);
    }

    fclose(file);
    return 0;
}
