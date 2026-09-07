# LibPIC

Petite bibliothèque C de composants réutilisables pour microcontrôleurs Microchip PIC,
destinée principalement aux projets compilés avec XC8.

L'objectif est de regrouper les composants logiciels communs afin d'éviter de dupliquer
leur code entre les différents projets.

## Composants

La bibliothèque contient notamment :

- **commun** — structures et types communs (`include/libpic/commun.h`)
- **input_t** — gestion des entrées, avec anti-rebond (`include/libpic/input.h`)
- **input_group_t** — gestion de groupe des entrées (`include/libpic/input_group.h`)
- **input_pattern** — détection de motifs d'entrées (appui court, appui long, double
  appui) (`include/libpic/input_pattern.h`)
- **output_t** — gestion des sorties (`include/libpic/output.h`)
- **output_group_t** — gestion de groupe des sorties (`include/libpic/output_group.h`)
- **timer_t** — gestion de temporisations non bloquantes (`include/libpic/timer.h`)
- **trigger_t** — gestion de déclenchements et événements (`include/libpic/trigger.h`)

La bibliothèque est amenée à évoluer au fur et à mesure des besoins des projets.

## Utilisation

Chaque composant peut être inclus individuellement (par exemple
`#include "libpic/timer.h"`), ou l'ensemble de la bibliothèque peut être inclus via
l'en-tête unique `#include "libpic.h"`.

Les composants sont écrits en C99 portable (sans dépendance à une plateforme
particulière) : le code spécifique à la broche physique (lecture/écriture) est
fourni par le projet appelant sous forme de fonctions de rappel (`read_pin_fn`,
`write_pin_fn`), ce qui permet d'utiliser la bibliothèque aussi bien avec XC8 sur
cible PIC que sur un hôte de développement pour les tests.

## Compilation et tests

La bibliothèque et ses tests unitaires se compilent avec CMake :

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
