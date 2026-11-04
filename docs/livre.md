---
press:
  template: book
  title: Le démineur
  subtitle: Une vitrine de conception objet en C++
  authors:
    - name: Yves Chevallier
  date: commit
  language: french
  paper: a4
  base_level: chapter
  # Paragraphs are not indented; a vertical space separates them instead.
  # The book template resets \parskip after the fragments, so the spacing is
  # set in the preamble, which comes last. The monospace font has the
  # box-drawing characters of the console grid. TeXSmith's list package sets
  # English bullets with enumitem, overriding babel-french: every itemize
  # level gets the French en dash back.
  typesetting:
    paragraph:
      indent: false
  # The table of contents stops at chapters: every chapter has the same
  # seven sections, listing them would only add noise.
  preamble: |
    \renewcommand{\changelocaltocdepth}[1]{\addtocontents{toc}{\protect\setcounter{tocdepth}{0}}\setcounter{tocdepth}{0}}
    \setlength{\parskip}{0.6em plus 0.2em minus 0.1em}
    \setmonofont{DejaVu Sans Mono}[Scale=MatchLowercase]
    \setlist[itemize,1,2,3,4,5,6,7,8,9]{label=\textendash}
  callouts:
    style: fancy
  slots:
    frontmatter: Le démineur, une démarche de conception objet
---


{raw latex}(\part{Analyser})

{include}(01-le-demineur.md)

{include}(02-elements-du-jeu.md)

{include}(03-cas-d-utilisation.md)

{include}(04-scenario-boite-noire.md)

{raw latex}(\part{Décider l'architecture})

{include}(05-choisir-une-architecture.md)

{include}(06-ports-et-adaptateurs.md)

{include}(07-sens-des-flux.md)

{include}(08-modules.md)

{raw latex}(\part{Construire le cœur})

{include}(09-positions.md)

{include}(10-champ-de-mines.md)

{include}(11-la-partie.md)

{include}(12-grille-generique.md)

{include}(13-ouverture-en-cascade.md)

{include}(14-ce-que-voit-l-interface.md)

{raw latex}(\part{L'application})

{include}(15-commandes.md)

{include}(16-hasard.md)

{include}(17-session.md)

{raw latex}(\part{Les adaptateurs})

{include}(18-terminal.md)

{include}(19-lanceur.md)

{include}(20-fenetre-sdl.md)

{include}(21-raii.md)

{raw latex}(\part{Vérifier et prendre du recul})

{include}(22-tests.md)

{include}(23-avant-apres.md)

{include}(24-et-maintenant.md)

{raw latex}(\part{Le labo})

{include}(../steps/README.md)

{include}(../steps/01-positions/README.md)

{raw latex}(\appendix)

{include}(regles-du-jeu.md)

{include}(uml.md)

{include}(README.md)
