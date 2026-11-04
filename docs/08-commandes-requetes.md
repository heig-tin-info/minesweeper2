# Commandes et requêtes

> **Objectif.** Séparer ce qui **change** l'état de ce qui le **lit**, et
> représenter les commandes par des types qui rendent les erreurs impossibles.
>
> **Code :** [src/app/commands.hpp](../src/app/commands.hpp),
> [src/app/session.cpp](../src/app/session.cpp)

## Le principe CQS

> Une méthode est soit une **commande** (elle change l'état et ne renvoie
> rien), soit une **requête** (elle renvoie une information et ne change
> rien). Jamais les deux.
>
> Bertrand Meyer, *Command-Query Separation*

Dans `Game`, la séparation est visible au premier coup d'œil :

```cpp
// Commandes : non-const, renvoient void
void reveal(Pos p);
void toggleFlag(Pos p);

// Requêtes : const, renvoient une valeur
Status status() const;
CellView cell(Pos p) const;
```

Le mot-clé `const` fait respecter la règle par le compilateur : une requête
**ne peut pas** modifier l'objet. On peut appeler une requête autant de fois
qu'on veut, dans n'importe quel ordre, sans risque. C'est ce qui permet à
l'interface SDL de redessiner 5 fois par seconde sans conséquence.

Au niveau des ports, la séparation devient deux interfaces :
[`CommandSink`](../src/app/ports.hpp) pour écrire, [`GameView`](../src/app/ports.hpp)
pour lire. Une fonction de dessin qui reçoit un `const GameView&` est
**incapable** de modifier la partie.

## CQS n'est pas CQRS

On entend souvent « CQRS » (*Command Query Responsibility Segregation*) pour
cette idée. C'est un abus de langage. CQRS est une architecture bien plus
lourde :

| | CQS | CQRS |
| --- | --- | --- |
| Niveau | une méthode, une classe | tout le système |
| Modèle | **un seul** objet répond aux deux | un modèle d'**écriture** et un ou plusieurs modèles de **lecture distincts** |
| Synchronisation | aucune, c'est le même objet | les modèles de lecture sont mis à jour (souvent par des événements, parfois avec retard) |
| Utile pour | toujours | lectures et écritures aux besoins très différents (gros volumes, vues multiples) |

Ici, `Session` répond aux deux ports à partir du **même** `Game`. C'est du
CQS, et pour un démineur, c'est exactement ce qu'il faut. Du vrai CQRS serait
de la sur-ingénierie.

## Représenter une commande

Une première idée serait :

```cpp
enum class Action { Reveal, ToggleFlag, NewGame, Quit, Redraw };
struct UserIntent {
    Action action;
    std::optional<Pos> where;
};
```

Ce type permet d'écrire des **absurdités** :

```cpp
UserIntent{Action::Reveal, std::nullopt};   // révéler... quoi ?
UserIntent{Action::NewGame, Pos{3, 3}};     // une position qui ne sert à rien
```

Le code qui reçoit l'intention doit donc tout re-vérifier à l'exécution. Et
`Quit` ou `Redraw` n'ont rien à faire là : ce sont des affaires d'interface,
que le cœur ignorerait de toute façon.

## Un type par commande

```cpp
struct Reveal     { Pos pos; };
struct ToggleFlag { Pos pos; };
struct NewGame    {};

using Command = std::variant<Reveal, ToggleFlag, NewGame>;
```

Chaque commande porte **exactement** les données dont elle a besoin. Un
`Reveal` sans position **ne compile pas**. C'est le principe :

> **Rendre les états illégaux impossibles à représenter.**

Toute une famille de vérifications à l'exécution disparaît, remplacée par le
système de types.

## `std::variant` : un type somme

Un `std::variant<A, B, C>` contient **une** valeur, qui est **soit** un `A`,
**soit** un `B`, **soit** un `C`. Il sait toujours laquelle. C'est ce qu'on
appelle un *type somme* (ou union étiquetée), par opposition à une `struct`
qui contient un `A` **et** un `B` **et** un `C` (type produit).

```cpp
Command c = Reveal{{2, 3}};
std::holds_alternative<Reveal>(c);   // true
std::get<Reveal>(c).pos;             // {2, 3}
```

## `std::visit` : traiter chaque cas

Pour agir selon la commande reçue, `Session::handle` utilise `std::visit` :

```cpp
void Session::handle(const Command& command) {
    const Status before = game_.status();
    std::visit(overloaded{
                   [&](const Reveal& c) { game_.reveal(c.pos); },
                   [&](const ToggleFlag& c) { game_.toggleFlag(c.pos); },
                   [&](const NewGame&) {
                       game_ = Game{config_.size, config_.mines, layout_};
                       stopwatch_ = {};
                   },
               },
               command);
    updateStopwatch(before, game_.status());
}
```

`std::visit` appelle la lambda qui correspond au type actuellement contenu.
L'avantage décisif sur un `switch` : si l'on ajoute une commande `Chord` au
`variant` sans la traiter ici, **le programme ne compile plus**. Le compilateur
vérifie que tous les cas sont couverts.

### Le petit utilitaire `overloaded`

```cpp
template <class... Fs>
struct overloaded : Fs... {
    using Fs::operator()...;
};
```

Il fabrique **un** objet appelable à partir de plusieurs lambdas, en héritant
de chacune (une lambda est un objet avec un `operator()`). Le `using` rend
toutes les surcharges visibles en même temps. C'est un idiome courant depuis
C++17, défini dans [overloaded.hpp](../src/core/overloaded.hpp).

## Pourquoi pas trois méthodes ?

On pourrait se passer du `variant` et donner trois méthodes au port :

```cpp
class CommandSink {
public:
    virtual void reveal(Pos p) = 0;
    virtual void toggleFlag(Pos p) = 0;
    virtual void newGame() = 0;
};
```

Ce serait plus direct et tout à fait correct. Ce qui change avec
`handle(const Command&)`, c'est que **la commande devient une valeur** et non
plus un appel : on peut la fabriquer, la renvoyer, la comparer, la stocker,
*avant* de l'exécuter. Le projet s'en sert à trois endroits.

**Traduire sans exécuter.** Les adaptateurs transforment une saisie en
commande avec des fonctions pures, qui ne touchent pas à la partie :

```cpp
std::optional<Command> SdlUi::translate(const SDL_Event& event) const;
std::optional<Input> parseInput(std::string_view line, Size size);
```

Avec trois méthodes, le parseur devrait recevoir le `CommandSink` et
l'appeler lui-même : comprendre la saisie et agir seraient mélangés. Ici, le
test du parseur compare simplement deux valeurs :

```cpp
EXPECT_EQ(parseInput(" c3 ", size), (Input{Command{Reveal{{5, 2}}}}));
```

**Un espion de test en une ligne.** La doublure `RecordingSink`
([chapitre 13](13-tests.md)) enregistre tout ce qu'elle reçoit :

```cpp
void handle(const Command& command) override { received.push_back(command); }
```

et le test vérifie la séquence d'un bloc :

```cpp
const std::vector<Command> expected{Reveal{{1, 1}}, ToggleFlag{{0, 0}}, NewGame{}};
EXPECT_EQ(sink.received, expected);
```

Avec trois méthodes, il faudrait trois redéfinitions et un format maison pour
noter les appels dans l'ordre.

**Le travail commun écrit une fois.** `Session::handle` relève le statut avant
la commande, la délègue, puis met à jour le chronomètre. Avec trois méthodes,
ce cadre serait recopié dans chacune, ou confié à un utilitaire qu'il ne faut
pas oublier d'appeler.

Une valeur peut aussi être mise en file d'attente, journalisée, rejouée ou
envoyée sur le réseau. Le jeu n'en a pas besoin aujourd'hui, mais la porte est
ouverte.

### Le prix à payer

- Une indirection : `std::visit` et `overloaded` sont moins lisibles qu'un
  simple appel de méthode.
- Aucun gain sur l'exhaustivité : ajouter une méthode virtuelle pure force,
  elle aussi, toutes les implémentations à suivre.
- Le compromis classique, appelé *expression problem* :

| | ajouter une **opération** (journaliser, sérialiser…) | ajouter un **type** de commande |
| --- | --- | --- |
| `std::variant` | facile : un nouveau `visit`, rien à modifier | retoucher chaque `visit` |
| méthodes virtuelles | retoucher chaque implémentation du port | facile : une nouvelle méthode |

Ici, les commandes sont peu nombreuses et stables, alors que le besoin de les
manipuler comme des données (traduction, tests) est réel : le `variant`
l'emporte. Si les commandes changeaient souvent et que personne n'avait besoin
de les stocker ni de les comparer, trois méthodes seraient un choix défendable.

## Ce qui ne passe pas par les commandes

`Quit` n'est pas une commande de l'application : quitter, c'est fermer
l'interface, pas modifier la partie. La console a donc son **propre** type
([notation.hpp](../src/adapters/console/notation.hpp)) :

```cpp
struct Quit {};
using Input = std::variant<Command, Quit>;
```

Une ligne tapée est soit une commande pour l'application, soit une affaire
interne à la console. Chaque couche n'a dans ses types que ce qui la concerne.

## À retenir

- **CQS** : une méthode modifie *ou* répond, jamais les deux. `const` le fait
  respecter.
- **CQRS** est une architecture à modèles séparés. Ne l'invoquez pas pour
  du simple CQS.
- **Un type par commande**, regroupés dans un `std::variant` : les commandes
  invalides deviennent impossibles à écrire.
- **`std::visit`** + `overloaded` : un traitement par cas, et le compilateur
  vérifie qu'aucun n'est oublié.
- **Une commande qui est une valeur** se fabrique, se compare et se stocke
  avant d'être exécutée : traduction pure dans les adaptateurs, espion de
  test trivial. Le prix : une indirection, et l'ajout d'un type de commande
  touche chaque `visit`.

## Pour réfléchir

1. Ajoutez `struct Chord { Pos pos; };` au `variant` sans rien changer
   d'autre. Quelles lignes ne compilent plus ? Est-ce une bonne nouvelle ?
2. `Game::reveal` ne renvoie rien. Comment l'interface sait-elle que la
   partie est perdue ?
3. Réécrivez `CommandSink` avec trois méthodes. Que deviennent `translate`,
   `parseInput` et `RecordingSink` ? Quels tests faut-il réécrire ?
