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
  preamble: |
    \setlength{\parskip}{0.6em plus 0.2em minus 0.1em}
    \setmonofont{DejaVu Sans Mono}[Scale=MatchLowercase]
    \setlist[itemize,1,2,3,4,5,6,7,8,9]{label=\textendash}
  callouts:
    style: fancy
  slots:
    frontmatter: Le démineur, une vitrine de conception objet
---

{include}(README.md)

{include}(01-le-demineur.md)

{include}(02-vue-ensemble.md)

{include}(03-objets-valeur.md)

{include}(04-encapsulation-invariants.md)

{include}(05-agregat.md)

{include}(06-templates.md)

{include}(07-polymorphisme-ports.md)

{include}(08-commandes-requetes.md)

{include}(09-projection-dto.md)

{include}(10-couche-application.md)

{include}(11-adaptateurs.md)

{include}(12-raii.md)

{include}(13-tests.md)

{include}(14-avant-apres.md)

{include}(../steps/README.md)

{include}(../steps/01-positions/README.md)

{raw latex}(\appendix)

{include}(regles-du-jeu.md)

{include}(uml.md)
