#include <stdio.h>

int main()
{
    FILE *fichier;
    char ligne[100];

    // Ouvrir le fichier en mode lecture
    fichier = fopen("script.conf", "r");

    // Vérifier si le fichier est ouvert avec succès
    if (fichier == NULL)
    {
        printf("Impossible d'ouvrir le fichier.\n");
        return 1;
    }

    // Lire chaque ligne du fichier
    while (fscanf(fichier, "%99[^\n]\n", ligne) != EOF)
    {
        printf("%s\n", ligne); // Afficher la ligne lue
    }

    // Fermer le fichier
    fclose(fichier);

    return 0;
}
