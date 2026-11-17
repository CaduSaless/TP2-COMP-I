# USO_IA.md

Registro de cada uso relevante de ferramentas de IA generativa, no formato do TP1.

A implementação das etapas do gerador (leitura das expressões, verificações, conjuntos, condição LL(1), tradução das regras, léxico por autômato, leitura do arquivo de tokens e integração com o scanner do TP1) foi feita pelo grupo, sem IA. A IA foi usada só na fase final, para entender o código, acrescentar a árvore gráfica, completar os testes e melhorar a documentação. Os usos estão abaixo, na ordem em que aconteceram.

Em todos os casos a ferramenta foi a mesma: Claude Code (Anthropic), modelo Claude Sonnet 5.5.

## 1. Leitura e explicação do código

    Ferramenta: Claude Code (Anthropic), Claude Sonnet 5.5
    Trecho:     src/ (ebnf_parser.c, conjuntos.c, verificacao.c, ll1.c, codegen.c) e microc/scanner.c
    Finalidade: Pedimos uma explicação do que cada módulo faz (FIRST e FOLLOW por ponto fixo,
                recursão à esquerda pelo fecho transitivo, condições LL(1), tradução das regras e
                léxico por AFN e AFD), para estudar o código antes da apresentação.
    O que fiz:  Apenas consultado. Nenhuma linha foi alterada nesta etapa.

## 2. Árvore de derivação gráfica (opção --svg)

    Ferramenta: Claude Code (Anthropic), Claude Sonnet 5.5
    Trecho:     src/codegen.c: vetor rt_svg (código C que vai para o analisador gerado), as
                chamadas acrescentadas em arvore_entra e match, e a opção --svg em gerar_main
    Finalidade: Gerar um desenho da árvore de derivação em SVG, sem bibliotecas externas, para o
                ponto extra. A IA escreveu o código: registro dos nós em pré-ordem, cálculo das
                posições (cada subárvore guarda o limite esquerdo e o direito de cada nível) e
                escrita do arquivo, com escape de XML nos lexemas.
    O que fiz:  Usado integralmente. Testado com make test (a saída de 2 * x + (3 - y) é comparada
                com testes/arvore/expr.svg), compilado com -ansi -pedantic -Wall -Wextra sem
                avisos e verificado com valgrind --leak-check=full, sem vazamentos, nas gramáticas
                expr, json e Micro C. Também conferimos o desenho visualmente.

## 3. Gramática JSON e testes da árvore

    Ferramenta: Claude Code (Anthropic), Claude Sonnet 5.5
    Trecho:     testes/gramaticas/json.ebnf, testes/programas/json/, testes/esperado/json.gerador,
                testes/arvore/ e o bloco da árvore em testes/executar.sh
    Finalidade: Criar a terceira gramática válida exigida pelo enunciado (JSON simplificado), com 3
                programas aceitos e 3 rejeitados, e testes automáticos para --tree e --svg.
    O que fiz:  Usado integralmente. As saídas esperadas foram geradas pelo próprio analisador e
                conferidas uma a uma (mensagens, linhas e colunas).

## 4. Pequenos ajustes no código

    Ferramenta: Claude Code (Anthropic), Claude Sonnet 5.5
    Trecho:     src/verificacao.c (mensagem do aviso de não terminal inalcançável), comentários
                de cabeçalho dos arquivos de src/, microc/LEIAME.md e .gitignore
    Finalidade: Deixar o aviso igual ao exemplo do manual ("f.ebnf:6:3: aviso: nao terminal sobra
                inalcancavel a partir de lista") e trocar os comentários "TODO(aluno)" que
                restavam, porque o checklist de entrega pede que nenhum sobre.
    O que fiz:  Usado integralmente.

## 5. Documentação

    Ferramenta: Claude Code (Anthropic), Claude Sonnet 5.5
    Trecho:     README.md e este arquivo
    Finalidade: Reescrever o README em formato de passo a passo (preparar, compilar, testar, gerar,
                executar, ver a árvore) e organizar este registro.
    O que fiz:  Adaptado a partir do README que veio no pacote. As mensagens de erro, a contagem de
                testes e a regra de desempate do léxico citadas no README foram conferidas
                executando o gerador.
