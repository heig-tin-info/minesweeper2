# Les commandes

> **Objectif.** Donner une forme aux ordres du joueur pour que deux interfaces
> différentes puissent les envoyer au cœur sans qu'il sache d'où ils viennent,
> et écrire les deux ports d'entrée.
>
> **Code :** [src/app/commands.hpp](../src/app/commands.hpp),
> [src/app/ports.hpp](../src/app/ports.hpp),
> [src/app/session.cpp](../src/app/session.cpp)

## Le besoin

Le cœur est terminé et testé (étapes 9 à 14). Il faut maintenant le brancher
sur une console, puis sur une fenêtre. À l'[étape 6](06-ports-et-adaptateurs.md),
on a décidé que les deux interfaces passeraient par les mêmes **ports
d'entrée** : un pour ordonner, un pour lire. Reste à les écrire.

La question du jour : **quelle forme prend un ordre ?** La console lit la
ligne `B3`, la fenêtre reçoit un clic en `(130, 210)` pixels. Les deux veulent
dire « dévoile la case `Pos{5, 2}` ». Le cœur ne doit voir que cette
intention, jamais le clavier ni la souris.

## Les options

| Option | Idée | Ce qui coince |
| --- | --- | --- |
| A. Trois méthodes virtuelles | `reveal(Pos)`, `toggleFlag(Pos)`, `newGame()` sur le port | correct et direct ; mais l'ordre n'existe que le temps d'un appel : impossible de le comparer, de le stocker, de le produire par une fonction pure |
| B. Un `enum` + une position optionnelle | `UserIntent` : un `enum Action` et un `optional<Pos>` | permet d'écrire des absurdités que le cœur doit re-vérifier |
| C. Un type par commande | `Reveal{pos}`, `ToggleFlag{pos}`, `NewGame{}` réunis dans un `std::variant` | une indirection de plus (`std::visit`) |

L'option B mérite qu'on la regarde de près, car c'est la première qui vient à
l'esprit :

```cpp
enum class Action { Reveal, ToggleFlag, NewGame, Quit, Redraw };
struct UserIntent {
    Action action;
    std::optional<Pos> where;
};

UserIntent{Action::Reveal, std::nullopt};   // révéler... quoi ?
UserIntent{Action::NewGame, Pos{3, 3}};     // une position qui ne sert à rien
```

Le code qui reçoit l'intention doit tout re-vérifier à l'exécution. Et `Quit`
ou `Redraw` n'ont rien à faire là : ce sont des affaires d'interface, que le
cœur ignorerait de toute façon.

## La décision

L'option C. Chaque commande porte **exactement** les données dont elle a
besoin ; un `Reveal` sans position **ne compile pas**.

```cpp
struct Reveal     { Pos pos; bool operator==(const Reveal&) const = default; };
struct ToggleFlag { Pos pos; bool operator==(const ToggleFlag&) const = default; };
struct NewGame    { bool operator==(const NewGame&) const = default; };

using Command = std::variant<Reveal, ToggleFlag, NewGame>;
```

Les deux ports d'entrée sont deux **interfaces** séparées : l'une reçoit des
commandes, l'autre répond à des questions.

```cpp
class GameView {
public:
    virtual ~GameView() = default;
    virtual Size size() const = 0;
    virtual CellView cell(Pos p) const = 0;
    virtual Status status() const = 0;
    virtual int minesLeft() const = 0;
    virtual std::chrono::seconds elapsed() const = 0;
};

class CommandSink {
public:
    virtual ~CommandSink() = default;
    virtual void handle(const Command& command) = 0;
};
```

Pourquoi C plutôt que A, qui marcherait aussi ? Parce qu'avec un `variant`,
**la commande est une valeur**. On peut la fabriquer, la renvoyer, la
comparer, la stocker, *avant* de l'exécuter. Trois endroits du projet en
profitent :

1. **Traduire sans exécuter.** Les adaptateurs transforment une saisie en
   commande par des fonctions pures, qui ne touchent pas à la partie :
   `parseInput(line, size)` renvoie un `optional<Input>`,
   `SdlUi::translate(event)` un `optional<Command>`. Avec trois méthodes, le
   parseur devrait recevoir le port et l'appeler lui-même : comprendre la
   saisie et agir seraient mélangés. *Testable sans écran ?* Oui : on compare
   deux valeurs.
2. **Un espion de test en une ligne.** `RecordingSink` enregistre tout ce
   qu'il reçoit dans un `std::vector<Command>`, et le test compare la
   séquence d'un bloc.
3. **Le travail commun écrit une fois.** Celui qui réalise `handle()` relève
   l'état avant la commande, la délègue, puis fait ce qui est commun à toutes
   (le chronomètre, étape 17). Avec trois méthodes, ce cadre serait recopié
   trois fois. *Écrit deux fois ?* Non.

Le prix, honnêtement : une indirection, et le compromis classique appelé
*expression problem* :

| | ajouter une **opération** (journaliser, sérialiser…) | ajouter un **type** de commande |
| --- | --- | --- |
| `std::variant` | facile : un nouveau `visit` | retoucher chaque `visit` |
| méthodes virtuelles | retoucher chaque implémentation | facile : une méthode de plus |

Ici, les commandes sont peu nombreuses et stables, alors que le besoin de les
manipuler comme des données est réel : le `variant` l'emporte.

Enfin, `Quit` n'est **pas** une commande : quitter, c'est fermer l'interface,
pas modifier la partie. La console a donc son propre type
([notation.hpp](../src/adapters/console/notation.hpp)) :

```cpp
struct Quit {};
using Input = std::variant<Command, Quit>;
```

Chaque couche n'a dans ses types que ce qui la concerne.

## Les concepts

### Un type somme : `std::variant`

Un `std::variant<A, B, C>` contient **une** valeur, qui est soit un `A`, soit
un `B`, soit un `C`, et il sait toujours laquelle. C'est un *type somme*, par
opposition à une `struct` qui contient un `A` **et** un `B` (type produit).

```cpp
Command c = Reveal{{2, 3}};
std::holds_alternative<Reveal>(c);   // true
std::get<Reveal>(c).pos;             // {2, 3}
```

Pour agir selon la commande, on utilise `std::visit` avec un visiteur qui
traite chaque cas. L'avantage décisif sur un `switch` : si l'on ajoute une
commande au `variant` sans la traiter, **le programme ne compile plus**.

```cpp
std::visit(overloaded{
               [&](const Reveal& c) { game_.reveal(c.pos); },
               [&](const ToggleFlag& c) { game_.toggleFlag(c.pos); },
               [&](const NewGame&) { /* étape 17 */ },
           },
           command);
```

`overloaded` ([overloaded.hpp](../src/core/overloaded.hpp)) fabrique un
objet appelable à partir de plusieurs lambdas, en héritant de chacune :

```cpp
template <class... Fs>
struct overloaded : Fs... {
    using Fs::operator()...;
};
```

### Une interface est une classe abstraite pure

`GameView` et `CommandSink` ne contiennent que des méthodes virtuelles pures
et un destructeur virtuel : ce sont des **contrats** sans implémentation.

| Mot-clé | Rôle |
| --- | --- |
| `virtual` | la méthode est choisie **à l'exécution**, selon le type réel de l'objet |
| `= 0` | méthode **virtuelle pure** : la classe devient **abstraite**, on ne peut pas l'instancier |
| `virtual ~GameView() = default` | **indispensable** : sans lui, détruire un objet dérivé via un pointeur sur la base est un comportement indéfini |
| `override` | demande au compilateur de vérifier qu'on redéfinit bien une méthode virtuelle |
| `final` | interdit de dériver de cette classe |

Pour l'instant, **personne** ne réalise ces deux interfaces. Ce sera le rôle de
`Session`, à l'[étape 17](17-session.md).

### Deux petits ports plutôt qu'un gros

On aurait pu écrire une seule interface `GamePort` avec `handle()` *et* les
cinq questions. On en écrit deux, pour que chaque client ne voie que la
facette dont il a besoin : une fonction de dessin reçoit un `const GameView&`
et est **incapable** de modifier la partie. C'est le **principe de ségrégation
des interfaces** (le « I » de SOLID) : un client ne dépend pas de méthodes
qu'il n'utilise pas.

Cette coupure prolonge le **CQS** vu à l'[étape 11](11-la-partie.md) : une
méthode modifie *ou* répond, jamais les deux. Au niveau des ports, la règle
devient deux interfaces. On entend souvent « CQRS » pour cette idée ; c'est un
abus de langage :

| | CQS | CQRS |
| --- | --- | --- |
| Niveau | une méthode, une classe | tout le système |
| Modèle | **un seul** objet répond aux deux | un modèle d'**écriture** et des modèles de **lecture distincts** |
| Synchronisation | aucune | les modèles de lecture sont mis à jour, parfois avec retard |
| Utile pour | toujours | lectures et écritures aux besoins très différents |

Ici, le même objet répondra aux deux ports à partir du même `Game`. C'est du
CQS ; du vrai CQRS serait de la sur-ingénierie.

## Vérifier

- `ParsesEveryKindOfInput` ([console_tests.cpp](../tests/console_tests.cpp))
  compare des commandes comme des valeurs :
  `parseInput(" c3 ", size) == Input{Command{Reveal{{5, 2}}}}`. C'est
  possible grâce aux `operator==` déclarés `= default`.
- `TranslatesTypedLinesIntoCommands` (même fichier) branche la console sur un
  `RecordingSink` et vérifie la séquence reçue :
  `{Reveal{{1, 1}}, ToggleFlag{{0, 0}}, NewGame{}}`.
- `ForwardsCommandsToTheGame` ([session_tests.cpp](../tests/session_tests.cpp))
  vérifie que `handle(ToggleFlag{{0, 0}})` atteint bien la partie.

## À retenir

- **Un type par commande**, regroupés dans un `std::variant` : les commandes
  invalides deviennent impossibles à écrire.
- **Une commande est une valeur** : elle se fabrique, se compare et se stocke
  avant d'être exécutée. Le prix : une indirection, et l'ajout d'un type de
  commande touche chaque `visit`.
- `std::visit` + `overloaded` : le compilateur vérifie qu'aucun cas n'est
  oublié.
- Une **interface** est une classe abstraite pure, avec un **destructeur
  virtuel**. Utilisez `override` partout.
- **Deux petits ports** (lire / ordonner) plutôt qu'un gros : chaque client ne
  voit que sa facette.

## Exercices

1. Ajoutez `struct Chord { Pos pos; };` au `variant` sans rien changer
   d'autre. Quelles lignes ne compilent plus ? Est-ce une bonne nouvelle ?
2. `Game::reveal` ne renvoie rien. Comment l'interface sait-elle que la
   partie est perdue ?
3. Réécrivez `CommandSink` avec trois méthodes. Que deviennent `translate`,
   `parseInput` et `RecordingSink` ? Quels tests faut-il réécrire ?
